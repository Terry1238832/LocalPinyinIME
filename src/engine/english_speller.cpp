#include "english_speller.h"

#include "typo_correction.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <system_error>

namespace localpinyin {
namespace {

// Edit costs in milli-nats (1000 is about e times less likely), on the same
// scale as a word's log count, so a word twice as common outweighs about 700.
// Measured on held-out Common Voice sentences with slips typed into them
// (docs/ENGLISH_INPUT.md).
constexpr int kMissingApostrophe = 1000;  // dont for don't
constexpr int kSwap = 4500;               // recieve
constexpr int kDoubled = 4000;            // untill, or ocur for occur
constexpr int kVowel = 5000;              // seperate
constexpr int kNeighbor = 5000;           // the key next to it
constexpr int kSound = 5500;              // c/k, c/s, s/z
constexpr int kNeighborExtra = 5000;      // a neighbor pressed too
constexpr int kExtra = 6500;
constexpr int kMissingVowel = 5500;
constexpr int kMissing = 6500;
constexpr int kDistant = 8000;
// Space corrects only when the best word is this much likelier than the next
// one (about 4.5 times).
constexpr int kCorrectionMargin = 1500;
constexpr std::uint32_t kMinCorrectionCount = 3;
constexpr std::uint32_t kCommonNameCount = 400;
// A word as typed gives way to its contraction (cant → can't) only when that
// is this many times as common.
constexpr std::uint32_t kContractionRatio = 30;
constexpr size_t kMaxKey = 64;
constexpr size_t kMaxTyped = 32;
constexpr size_t kCompletions = 3;
constexpr size_t kOtherCorrections = 2;
// A learned word ranks among completions like a fairly common word.
constexpr std::uint32_t kUserWordCount = 200;

bool is_vowel(char ch) noexcept {
    return ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u';
}

bool is_upper(char ch) noexcept { return ch >= 'A' && ch <= 'Z'; }
bool is_lower(char ch) noexcept { return ch >= 'a' && ch <= 'z'; }
char to_lower(char ch) noexcept { return is_upper(ch) ? static_cast<char>(ch - 'A' + 'a') : ch; }
char to_upper(char ch) noexcept { return is_lower(ch) ? static_cast<char>(ch - 'a' + 'A') : ch; }

std::string lowercase(std::string_view text) {
    std::string result(text);
    for (char& ch : result) ch = to_lower(ch);
    return result;
}

// Letters with apostrophes between them (don't, o'clock), as typed.
bool is_spelling(std::string_view text) noexcept {
    if (text.empty() || text.size() > kMaxKey || text.front() == '\'' || text.back() == '\'') return false;
    return std::all_of(text.begin(), text.end(), [](char ch) { return is_lower(ch) || is_upper(ch) || ch == '\''; });
}

bool neighbors(char a, char b) noexcept {
    return a && b && keyboard_neighbors(static_cast<wchar_t>(a), static_cast<wchar_t>(b));
}

bool sound_pair(char a, char b) noexcept {
    const auto pair = [&](char x, char y) { return (a == x && b == y) || (a == y && b == x); };
    return pair('c', 'k') || pair('c', 's') || pair('s', 'z') || pair('i', 'y') || pair('g', 'j');
}

int substitution(char typed, char wanted) noexcept {
    if (typed == wanted) return 0;
    if (typed == '\'' || wanted == '\'') return kDistant;
    if (is_vowel(typed) && is_vowel(wanted)) return kVowel;
    if (neighbors(typed, wanted)) return kNeighbor;
    if (sound_pair(typed, wanted)) return kSound;
    return kDistant;
}

// typed[at] pressed but not wanted.
int extra(std::string_view typed, size_t at) noexcept {
    const char key = typed[at];
    const char before = at > 0 ? typed[at - 1] : 0;
    const char after = at + 1 < typed.size() ? typed[at + 1] : 0;
    if (key == '\'' || key == before || key == after) return kDoubled;
    if (neighbors(key, before) || neighbors(key, after)) return kNeighborExtra;
    return kExtra;
}

// wanted[at] not typed.
int missing(std::string_view wanted, size_t at) noexcept {
    const char key = wanted[at];
    if (key == '\'') return kMissingApostrophe;
    if ((at > 0 && wanted[at - 1] == key) || (at + 1 < wanted.size() && wanted[at + 1] == key)) return kDoubled;
    return is_vowel(key) ? kMissingVowel : kMissing;
}

// How many edits the typed letters may be from a word Space corrects to: none
// for one or two letters, one for three or four, about two beyond.
int budget_for(size_t letters) noexcept {
    if (letters <= 2) return 0;
    if (letters <= 4) return kMissing;
    if (letters <= 7) return 10000;
    return 13000;
}

// The written form with the capitals of what was typed: HELLO for HEL,
// Hello for Hel. A form with capitals of its own (English, iPhone) keeps them.
std::string with_case(std::string_view form, std::string_view typed) {
    std::string result(form);
    const size_t letters = static_cast<size_t>(std::count_if(typed.begin(), typed.end(), [](char ch) { return ch != '\''; }));
    const bool shouting = letters >= 2 && std::none_of(typed.begin(), typed.end(), is_lower);
    if (shouting) {
        for (char& ch : result) ch = to_upper(ch);
    } else if (!typed.empty() && is_upper(typed.front()) &&
               std::none_of(result.begin(), result.end(), is_upper) && !result.empty()) {
        result.front() = to_upper(result.front());
    }
    return result;
}

// OK, NASA: two or more letters, all capitals.
bool is_abbreviation(std::string_view form) noexcept {
    return std::count_if(form.begin(), form.end(), is_upper) >= 2 && std::none_of(form.begin(), form.end(), is_lower);
}

// Words whose capitals Space puts in by itself: I and its contractions, days,
// months and the commonest languages and nationalities. Other names may be
// part of an address or user name (john.smith), so they are only offered.
bool always_capitalized(std::string_view key) noexcept {
    static constexpr std::string_view kWords[] = {
        "i", "i'm", "i've", "i'd", "i'll",
        "monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday",
        "january", "february", "april", "june", "july", "august", "september", "october", "november", "december",
        "english", "chinese", "japanese", "korean", "french", "german", "spanish", "italian", "russian",
        "portuguese", "arabic", "american", "british", "canadian", "australian", "european", "asian", "african",
        "christmas", "internet",
    };
    return std::find(std::begin(kWords), std::end(kWords), key) != std::end(kWords);
}

bool starts_with(std::string_view text, std::string_view prefix) noexcept {
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

}  // namespace

bool EnglishSpeller::load(const std::filesystem::path& words_path, const std::filesystem::path& terms_path) {
    struct Entry {
        std::string key;
        std::string written;
        std::uint32_t count = 0;
        bool offered = true;
        bool term = false;
    };
    std::vector<Entry> entries;
    const auto read_lines = [](const std::filesystem::path& path, auto&& on_fields) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return false;
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF) line.erase(0, 3);
            if (line.empty() || line[0] == '#') continue;
            std::vector<std::string_view> fields;
            std::string_view rest(line);
            while (true) {
                const size_t tab = rest.find('\t');
                fields.push_back(rest.substr(0, tab));
                if (tab == std::string_view::npos) break;
                rest.remove_prefix(tab + 1);
            }
            on_fields(fields);
        }
        return true;
    };
    const auto number = [](std::string_view text) -> std::uint32_t {
        std::uint64_t value = 0;
        for (const char ch : text) {
            if (ch < '0' || ch > '9') return 0;
            value = std::min<std::uint64_t>(value * 10 + static_cast<std::uint64_t>(ch - '0'), 0xFFFFFFFFu);
        }
        return static_cast<std::uint32_t>(value);
    };
    const bool loaded = read_lines(words_path, [&](const std::vector<std::string_view>& fields) {
        if (fields.size() < 2 || !is_spelling(fields[0])) return;
        entries.push_back({lowercase(fields[0]), std::string(fields[0]), number(fields[1]),
                           fields.size() < 3 || fields[2] != "V"});
    });
    if (!loaded) return false;
    if (!terms_path.empty()) {
        read_lines(terms_path, [&](const std::vector<std::string_view>& fields) {
            // key, written form, weight (1-100)
            if (fields.size() < 3 || !is_spelling(fields[1]) || lowercase(fields[1]) != fields[0] || fields[1] == fields[0]) return;
            entries.push_back({std::string(fields[0]), std::string(fields[1]), std::min<std::uint32_t>(number(fields[2]), 100), true, true});
        });
    }
    // One word per key, the dictionary's. A term of the same key gives a word
    // written with capitals (JavaScript) the term's, which Space then puts
    // in; an everyday word (rust, word, python) stays lowercase.
    std::stable_sort(entries.begin(), entries.end(), [](const Entry& left, const Entry& right) { return left.key < right.key; });
    keys_.clear();
    words_.clear();
    forms_.clear();
    for (const auto& entry : entries) {
        if (!words_.empty() && key(words_.size() - 1) == entry.key) {
            if (entry.term && forms_.count(static_cast<std::uint32_t>(words_.size() - 1))) {
                words_.back().capitalize = true;
                forms_.insert_or_assign(static_cast<std::uint32_t>(words_.size() - 1), entry.written);
            }
            continue;
        }
        Word word;
        word.offset = static_cast<std::uint32_t>(keys_.size());
        word.length = static_cast<std::uint8_t>(entry.key.size());
        word.offered = entry.offered;
        word.capitalize = entry.term || always_capitalized(entry.key);
        word.count = entry.count;
        keys_ += entry.key;
        if (entry.written != entry.key) forms_.emplace(static_cast<std::uint32_t>(words_.size()), entry.written);
        words_.push_back(word);
    }
    keys_.shrink_to_fit();
    words_.shrink_to_fit();
    return !words_.empty();
}

void EnglishSpeller::open_user_words(const std::filesystem::path& path) {
    user_path_ = path;
    user_time_ = {};
    user_size_ = 0;
    user_words_.clear();
    read_user_words();
}

void EnglishSpeller::read_user_words() {
    spelled_.clear();
    std::error_code error;
    if (user_path_.empty()) return;
    const auto time = std::filesystem::last_write_time(user_path_, error);
    if (error) {
        user_words_.clear();
        return;
    }
    const auto size = std::filesystem::file_size(user_path_, error);
    user_time_ = time;
    user_size_ = error ? 0 : size;
    user_words_.clear();
    std::ifstream file(user_path_, std::ios::binary);
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF) line.erase(0, 3);
        if (is_spelling(line)) user_words_.insert_or_assign(lowercase(line), line);
    }
}

void EnglishSpeller::refresh_user_words_if_changed() {
    if (user_path_.empty()) return;
    std::error_code error;
    const auto time = std::filesystem::last_write_time(user_path_, error);
    if (error) {
        if (!user_words_.empty()) user_words_.clear();
        return;
    }
    const auto size = std::filesystem::file_size(user_path_, error);
    if (time != user_time_ || (!error && size != user_size_)) read_user_words();
}

bool EnglishSpeller::learn(std::string_view word) {
    if (!is_spelling(word)) return false;
    spelled_.clear();
    const std::string key = lowercase(word);
    if (const auto found = user_words_.find(key); found != user_words_.end() && found->second == word) return false;
    if (const size_t index = find(key); index != npos && form(index) == word) return false;
    user_words_.insert_or_assign(key, std::string(word));
    if (user_path_.empty()) return true;
    std::error_code error;
    std::filesystem::create_directories(user_path_.parent_path(), error);
    std::ofstream file(user_path_, std::ios::binary | std::ios::app);
    file << word << '\n';
    file.close();
    user_time_ = std::filesystem::last_write_time(user_path_, error);
    user_size_ = std::filesystem::file_size(user_path_, error);
    return true;
}

bool EnglishSpeller::forget(std::string_view word) {
    if (user_words_.erase(lowercase(word)) == 0) return false;
    spelled_.clear();
    if (user_path_.empty()) return true;
    std::vector<std::string> words;
    words.reserve(user_words_.size());
    for (const auto& [key, written] : user_words_) words.push_back(written);
    std::sort(words.begin(), words.end());
    std::error_code error;
    const auto temporary = std::filesystem::path(user_path_).concat(".tmp");
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        for (const auto& written : words) file << written << '\n';
    }
    std::filesystem::rename(temporary, user_path_, error);
    if (error) std::filesystem::remove(temporary, error);
    user_time_ = std::filesystem::last_write_time(user_path_, error);
    user_size_ = std::filesystem::file_size(user_path_, error);
    return true;
}

size_t EnglishSpeller::find(std::string_view wanted) const noexcept {
    size_t low = 0;
    size_t high = words_.size();
    while (low < high) {
        const size_t middle = low + (high - low) / 2;
        if (key(middle) < wanted) low = middle + 1;
        else high = middle;
    }
    return low < words_.size() && key(low) == wanted ? low : npos;
}

std::string EnglishSpeller::form(size_t index) const {
    const auto found = forms_.find(static_cast<std::uint32_t>(index));
    return found != forms_.end() ? found->second : std::string(key(index));
}

int EnglishSpeller::score(size_t index, int cost) const noexcept {
    return static_cast<int>(std::lround(1000.0 * std::log(static_cast<double>(words_[index].count) + 1.0))) - cost;
}

bool EnglishSpeller::knows(std::string_view typed) const {
    if (!is_spelling(typed)) return false;
    std::string wanted = lowercase(typed);
    if (user_words_.count(wanted) || find(wanted) != npos) return true;
    if (wanted.size() > 2 && wanted.compare(wanted.size() - 2, 2, "'s") == 0) {
        wanted.resize(wanted.size() - 2);
        return user_words_.count(wanted) || find(wanted) != npos;
    }
    return false;
}

size_t EnglishSpeller::apostrophe_variant(std::string_view wanted) const {
    size_t best = npos;
    for (size_t at = 1; at < wanted.size(); ++at) {
        std::string variant;
        variant.reserve(wanted.size() + 1);
        variant.append(wanted.substr(0, at)).push_back('\'');
        variant.append(wanted.substr(at));
        const size_t index = find(variant);
        if (index != npos && words_[index].offered && (best == npos || words_[index].count > words_[best].count))
            best = index;
    }
    return best;
}

std::vector<EnglishSpeller::Scored> EnglishSpeller::corrections(std::string_view typed, int budget) const {
    std::vector<Scored> found;
    const size_t columns = typed.size() + 1;
    if (budget <= 0 || typed.empty() || typed.size() > kMaxTyped) return found;
    // rows[depth][t]: the cost of typing typed[0..t) for the first `depth`
    // letters of a word (a weighted Damerau-Levenshtein distance). Words
    // are sorted, so the rows of the prefix a word shares with the one before
    // are kept, and a prefix already over budget skips every word under it.
    std::vector<int> rows((kMaxKey + 1) * columns);
    const auto row = [&](size_t depth) { return rows.data() + depth * columns; };
    row(0)[0] = 0;
    for (size_t t = 1; t < columns; ++t) row(0)[t] = row(0)[t - 1] + extra(typed, t - 1);
    std::string_view previous;
    size_t valid = 0;
    size_t index = 0;
    while (index < words_.size()) {
        const std::string_view word = key(index);
        size_t depth = 0;
        const size_t shared = std::min({word.size(), previous.size(), valid});
        while (depth < shared && word[depth] == previous[depth]) ++depth;
        bool over = false;
        for (; depth < word.size(); ++depth) {
            const size_t d = depth + 1;
            int* current = row(d);
            const int* above = row(d - 1);
            current[0] = above[0] + missing(word, depth);
            int lowest = current[0];
            for (size_t t = 1; t < columns; ++t) {
                int cost = above[t - 1] + substitution(typed[t - 1], word[depth]);
                cost = std::min(cost, current[t - 1] + extra(typed, t - 1));
                cost = std::min(cost, above[t] + missing(word, depth));
                if (d >= 2 && t >= 2 && typed[t - 1] == word[depth - 1] && typed[t - 2] == word[depth] &&
                    typed[t - 1] != typed[t - 2])
                    cost = std::min(cost, row(d - 2)[t - 2] + kSwap);
                current[t] = cost;
                lowest = std::min(lowest, cost);
            }
            if (lowest > budget) {
                over = true;
                break;
            }
        }
        if (over) {
            // Every word starting with word[0..depth] is over budget too.
            const std::string_view prefix = word.substr(0, depth + 1);
            size_t low = index + 1;
            size_t high = words_.size();
            while (low < high) {
                const size_t middle = low + (high - low) / 2;
                if (key(middle).substr(0, prefix.size()) <= prefix) low = middle + 1;
                else high = middle;
            }
            previous = word;
            valid = depth;
            index = low;
            continue;
        }
        const int cost = row(word.size())[typed.size()];
        if (cost <= budget && cost > 0 && words_[index].offered) found.push_back({index, cost, score(index, cost)});
        previous = word;
        valid = word.size();
        ++index;
    }
    std::sort(found.begin(), found.end(), [](const Scored& left, const Scored& right) {
        return left.score != right.score ? left.score > right.score : left.index < right.index;
    });
    return found;
}

std::vector<size_t> EnglishSpeller::completions(std::string_view prefix, size_t limit) const {
    std::vector<size_t> found;
    size_t low = 0;
    size_t high = words_.size();
    while (low < high) {
        const size_t middle = low + (high - low) / 2;
        if (key(middle) < prefix) low = middle + 1;
        else high = middle;
    }
    for (size_t index = low; index < words_.size() && starts_with(key(index), prefix); ++index) {
        if (words_[index].offered && words_[index].length > prefix.size()) found.push_back(index);
    }
    const size_t keep = std::min(limit, found.size());
    std::partial_sort(found.begin(), found.begin() + static_cast<std::ptrdiff_t>(keep), found.end(),
                      [&](size_t left, size_t right) {
                          return words_[left].count != words_[right].count ? words_[left].count > words_[right].count
                                                                           : left < right;
                      });
    found.resize(keep);
    return found;
}

EnglishSpeller::Result EnglishSpeller::lookup(std::string_view typed) const {
    using Kind = Candidate::Kind;
    Result result;
    if (typed.empty()) return result;
    const std::string as_typed(typed);
    const auto add = [&](std::string text, Kind kind) {
        if (text.empty() || std::any_of(result.candidates.begin(), result.candidates.end(),
                                        [&](const Candidate& candidate) { return candidate.text == text; }))
            return;
        result.candidates.push_back({std::move(text), kind});
    };
    if (!is_spelling(typed) || typed.size() > kMaxTyped) {
        add(as_typed, Kind::AsTyped);
        return result;
    }
    const std::string wanted = lowercase(typed);
    const size_t letters = static_cast<size_t>(std::count_if(typed.begin(), typed.end(), [](char ch) { return ch != '\''; }));
    const bool shouting = letters >= 2 && std::none_of(typed.begin(), typed.end(), is_lower);

    // Completions: the user's words first among equals, then the commonest.
    std::vector<std::pair<std::uint32_t, std::string>> longer;
    for (const auto& [key, written] : user_words_) {
        if (key.size() > wanted.size() && starts_with(key, wanted)) longer.emplace_back(kUserWordCount, written);
    }
    for (const size_t index : completions(wanted, kCompletions)) longer.emplace_back(words_[index].count, form(index));
    std::stable_sort(longer.begin(), longer.end(), [](const auto& left, const auto& right) { return left.first > right.first; });
    const auto add_completions = [&]() {
        size_t added = 0;
        for (const auto& [count, written] : longer) {
            if (added == kCompletions) break;
            const size_t before = result.candidates.size();
            add(with_case(written, typed), Kind::Completion);
            if (result.candidates.size() > before) ++added;
        }
    };

    // A known word: as typed, with its own capitals (english → English), or
    // its far more common contraction (cant → can't).
    std::string known;
    std::uint32_t known_count = 0;
    bool possessive = false;
    bool capitalize = true;
    size_t abbreviation = npos;
    if (const auto user = user_words_.find(wanted); user != user_words_.end()) {
        known = user->second;
        known_count = kUserWordCount;
    } else if (const size_t index = find(wanted); index != npos && (shouting || !is_abbreviation(form(index)))) {
        known = form(index);
        known_count = words_[index].count;
        capitalize = words_[index].capitalize;
    } else if (index != npos) {
        // Typed in lowercase, an abbreviation (OK, NAD) is offered but not put
        // in: nad is far more likely and.
        abbreviation = index;
    } else if (wanted.size() > 2 && wanted.compare(wanted.size() - 2, 2, "'s") == 0) {
        const std::string base = wanted.substr(0, wanted.size() - 2);
        possessive = true;
        if (const auto user = user_words_.find(base); user != user_words_.end()) known = user->second + "'s";
        else if (const size_t index = find(base); index != npos) known = form(index) + "'s";
        known_count = kUserWordCount;
    }
    if (!known.empty()) {
        std::string head = with_case(known, typed);
        // john stays john; John is offered.
        std::string capitals;
        if (!capitalize && lowercase(head) == wanted && head != as_typed && std::none_of(typed.begin(), typed.end(), is_upper)) {
            capitals = head;
            head = as_typed;
        }
        if (!shouting && !possessive) {
            const size_t contraction = apostrophe_variant(wanted);
            if (contraction != npos && words_[contraction].count >= kContractionRatio * std::max<std::uint32_t>(known_count, 1))
                head = with_case(form(contraction), typed);
        }
        result.corrects = head != as_typed;
        add(head, result.corrects ? Kind::Correction : Kind::AsTyped);
        add(as_typed, Kind::AsTyped);
        if (!capitals.empty()) add(capitals, Kind::Correction);
        add_completions();
        return result;
    }

    // Not a word. A missing apostrophe is the likeliest slip of all (im,
    // dont); otherwise the nearest common words, which Space takes only when
    // one clearly leads. Shouted letters (NASA) are left alone.
    std::vector<Scored> found;
    if (!shouting) {
        if (const size_t contraction = apostrophe_variant(wanted); contraction != npos)
            found.push_back({contraction, kMissingApostrophe, std::numeric_limits<int>::max()});
        for (const auto& scored : corrections(wanted, budget_for(letters))) {
            if (found.empty() || scored.index != found.front().index) found.push_back(scored);
        }
    }
    const bool apostrophe = !found.empty() && found.front().score == std::numeric_limits<int>::max();
    result.corrects = !found.empty() &&
                      (apostrophe || found.size() == 1 || found[0].score - found[1].score >= kCorrectionMargin);
    // A rare word is no likelier than a word missing from the list, and a
    // capitalized one is most likely a name (Pantanal): Space corrects it, or
    // corrects anything into a name, only for a single slip such as Teh or
    // into a common name (Febuary).
    if (result.corrects && !apostrophe) {
        const size_t best = found.front().index;
        const bool name = is_upper(form(best).front());
        const bool slight = found.front().cost <= kSwap;
        if (words_[best].count < kMinCorrectionCount ||
            ((is_upper(typed.front()) || name) && !slight && !(name && words_[best].count >= kCommonNameCount)))
            result.corrects = false;
    }
    if (result.corrects) add(with_case(form(found.front().index), typed), Kind::Correction);
    add(as_typed, Kind::AsTyped);
    if (abbreviation != npos) add(form(abbreviation), Kind::Correction);
    size_t others = 0;
    for (size_t at = result.corrects ? 1 : 0; at < found.size() && others < kOtherCorrections; ++at) {
        const size_t before = result.candidates.size();
        add(with_case(form(found[at].index), typed), Kind::Correction);
        if (result.candidates.size() > before) ++others;
    }
    add_completions();
    return result;
}

const std::string& EnglishSpeller::spelled(const std::string& word) const {
    if (const auto found = spelled_.find(word); found != spelled_.end()) return found->second;
    if (spelled_.size() > 4096) spelled_.clear();
    const Result result = lookup(word);
    std::string text = result.corrects && !result.candidates.empty() ? result.candidates.front().text : word;
    return spelled_.emplace(word, std::move(text)).first->second;
}

namespace {

struct Piece {
    std::string text;
    bool word = false;
};

bool ascii_letter(char ch) { return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'); }

std::string lower(std::string_view text) {
    std::string result(text);
    for (char& ch : result) if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
    return result;
}

bool all_lower(std::string_view text) {
    return std::none_of(text.begin(), text.end(), [](char ch) { return ch >= 'A' && ch <= 'Z'; });
}

// Joins a word to what comes before it: part of an address, a path or a name.
bool joining(char ch) {
    return ascii_letter(ch) || (ch >= '0' && ch <= '9') || std::string_view(".@-_/\\:#~+=&%").find(ch) != std::string_view::npos;
}

// Where a sentence ends: . ! ? not after an abbreviation (Mr. Smith, e.g.).
bool ends_sentence(const std::vector<Piece>& pieces, size_t gap) {
    if (gap == 0) return false;
    const std::string& before = pieces[gap - 1].text;
    if (pieces[gap - 1].word || before.empty()) return false;
    const auto mark = before.find_first_of(".!?");
    if (mark == std::string::npos || before.find_first_not_of(".!?\"')]") < mark) return false;
    // A space after it (example.com is an address, not two sentences).
    if (before.find_first_of(" \t\r\n", mark) == std::string::npos) return false;
    if (before[mark] != '.' || gap < 2 || !pieces[gap - 2].word) return true;
    static const char* const kAbbreviations[] = {"mr", "mrs", "ms", "dr", "prof", "sr", "jr", "st", "vs", "etc", "eg", "ie",
                                                 "e", "g", "i", "no", "fig", "approx", "dept", "inc", "ltd", "co", "mt"};
    const std::string word = lower(pieces[gap - 2].text);
    return std::none_of(std::begin(kAbbreviations), std::end(kAbbreviations), [&](const char* item) { return word == item; });
}

std::string with_case_of(std::string_view model, std::string text) {
    if (!model.empty() && model[0] >= 'A' && model[0] <= 'Z' && !text.empty() && text[0] >= 'a' && text[0] <= 'z')
        text[0] = static_cast<char>(text[0] - 'a' + 'A');
    return text;
}

}  // namespace

std::string EnglishSpeller::correct_sentence(std::string_view typed, bool sentence_start, bool joined,
                                             std::vector<std::string>* spelling) const {
    // Words (letters and apostrophes) and what lies between them.
    std::vector<Piece> pieces;
    for (size_t at = 0; at < typed.size();) {
        const bool word = ascii_letter(typed[at]);
        size_t end = at;
        while (end < typed.size() && (word ? (ascii_letter(typed[end]) || typed[end] == '\'') : !ascii_letter(typed[end]))) ++end;
        pieces.push_back({std::string(typed.substr(at, end - at)), word});
        at = end;
    }
    // Spelling, word by word, as Space would put each in.
    for (size_t index = 0; index < pieces.size(); ++index) {
        if (!pieces[index].word) continue;
        const bool attached = index == 0 ? joined : !pieces[index - 1].text.empty() && joining(pieces[index - 1].text.back());
        if (attached) continue;
        const std::string& fixed = spelled(pieces[index].text);
        if (fixed != pieces[index].text) {
            if (spelling && lower(fixed) != lower(pieces[index].text)) spelling->push_back(pieces[index].text);
            pieces[index].text = fixed;
        }
    }
    // The next word after the gap at `index`, or npos; whether only spaces lie between.
    const auto next_word = [&](size_t index, bool* spaced) {
        if (index + 2 < pieces.size() && pieces[index + 1].text.find_first_not_of(' ') == std::string::npos) {
            if (spaced) *spaced = true;
            return index + 2;
        }
        return static_cast<size_t>(-1);
    };
    for (size_t index = 0; index < pieces.size(); ++index) {
        if (!pieces[index].word) continue;
        const std::string word = lower(pieces[index].text);
        bool spaced = false;
        const size_t next = next_word(index, &spaced);
        const std::string following = next == static_cast<size_t>(-1) ? std::string() : lower(pieces[next].text);
        if (next == static_cast<size_t>(-1)) {
            if (word == "alot") pieces[index].text = with_case_of(pieces[index].text, "a lot");
            continue;
        }
        // Fixed phrases.
        if ((word == "could" || word == "should" || word == "would" || word == "must" || word == "might") && following == "of")
            pieces[next].text = "have";
        else if (word == "your" && following == "welcome")
            pieces[index].text = with_case_of(pieces[index].text, "you're");
        else if (word == "its") {
            static const char* const kAfter[] = {"a", "an", "been", "not", "going", "time", "ok", "okay", "so", "just", "really", "very", "the"};
            if (std::any_of(std::begin(kAfter), std::end(kAfter), [&](const char* item) { return following == item; }))
                pieces[index].text = with_case_of(pieces[index].text, "it's");
        } else if (word == "alot") {
            pieces[index].text = with_case_of(pieces[index].text, "a lot");
        }
        // A short word typed twice (the the).
        static const char* const kDoubled[] = {"the", "a", "an", "to", "of", "and", "in", "is", "for", "on", "with", "as", "at", "by", "from"};
        if (word == following && std::any_of(std::begin(kDoubled), std::end(kDoubled), [&](const char* item) { return word == item; })) {
            pieces[index + 1].text.clear();
            pieces[next].text.clear();
            pieces[next].word = false;
            continue;
        }
        // a / an by the sound the next word starts with; u and h words and
        // capitals (an FBI agent) are left as typed.
        if ((word == "a" || word == "an") && !following.empty() && ascii_letter(following[0]) && all_lower(pieces[next].text.substr(1))) {
            static const char* const kSilentH[] = {"hour", "honest", "honor", "honour", "heir"};
            static const char* const kConsonantSound[] = {"one", "once", "eu", "ewe"};
            const char first = following[0];
            const auto starts = [&](const char* const* list, size_t count) {
                return std::any_of(list, list + count, [&](const char* item) { return following.rfind(item, 0) == 0; });
            };
            const bool vowel = (std::string_view("aeio").find(first) != std::string_view::npos &&
                                !starts(kConsonantSound, std::size(kConsonantSound))) ||
                               starts(kSilentH, std::size(kSilentH));
            const bool consonant = std::string_view("bcdfgjklmnpqrstvwxyz").find(first) != std::string_view::npos;
            if (word == "a" && vowel) pieces[index].text = with_case_of(pieces[index].text, "an");
            else if (word == "an" && consonant) pieces[index].text = with_case_of(pieces[index].text, "a");
        }
    }
    // Capitals where a sentence starts.
    for (size_t index = 0; index < pieces.size(); ++index) {
        if (!pieces[index].word || pieces[index].text.empty() || !all_lower(pieces[index].text)) continue;
        const bool starts = index == 0 ? sentence_start && !joined : ends_sentence(pieces, index);
        if (starts) pieces[index].text[0] = static_cast<char>(pieces[index].text[0] - 'a' + 'A');
    }
    std::string result;
    for (const auto& piece : pieces) result += piece.text;
    return result;
}

}  // namespace localpinyin
