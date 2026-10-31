#include "pinyin_engine.h"

#include "double_pinyin.h"
#include "input_tools.h"
#include "pinyin_spelling.h"
#include "typo_correction.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <iterator>
#include <filesystem>
#include <cmath>
#include <ctime>
#include <unordered_map>
#include <unordered_set>

namespace localpinyin {
namespace {

constexpr int kExactMatchBonus = 100000;
constexpr int kCompleteSegmentationBonus = 2000;
constexpr int kFallbackScore = 1;
constexpr size_t kSentenceBeamWidth = 32;
constexpr size_t kLongestInitialsInput = 12;
constexpr size_t kLongestInitialsWord = 6;
constexpr size_t kInitialsWordsPerSpan = 12;
constexpr size_t kCharactersPerInitial = 32;
// What a composed phrase counts for beside lexicon frequencies (the lowest band).
constexpr int kComposedPhraseFrequency = 5500;
constexpr size_t kMaxInputLength = 256;
constexpr size_t kLongestSyllable = 6;
constexpr size_t kNeuralCandidates = 5;
// Learned single characters inside a sentence: see decode().
constexpr int kOutOfContextLearningDivisor = 4;
// A fuzzy reading is a little less likely than the spelling as typed.
constexpr int kFuzzyPenalty = 600;
// A word of a chosen usage scene as a sentence token. Enough to lift an
// everyday word the news-heavy context model underrates (其实 after 题), not
// enough to override clear context (直到 stays 直到 with 知道 chosen).
constexpr int kDomainBonus = 1000;
// A phrase the user built from separately chosen pieces reads as a common
// word wherever its pinyin appears.
constexpr int kLearnedPhraseFrequency = 9000;
// English terms inside pinyin: a modest prior, so Chinese wins any span both
// can read (they are only decoded when Chinese cannot read the whole input).
constexpr int kEnglishBase = -2600;
constexpr int kEnglishWeightScale = 12;
constexpr int kEnglishLengthBonus = 150;
// A correction is offered beside valid readings only when its word is common
// and clearly more common than the typed reading's best word.
constexpr int kStrongCorrectionFrequency = 9500;
// When no dictionary word spells the whole input, its reading is a string of
// single characters and a common word one slip away is worth offering.
constexpr int kUnreadCorrectionFrequency = 7000;
constexpr int kStrongCorrectionMargin = 2500;
// Margin at which a correction is offered second; smaller ones go fifth.
constexpr int kLeadingCorrectionMargin = 4000;
// A typed reading that is already a common word is never second-guessed.
constexpr int kCommonTypedFrequency = 8000;
constexpr int kCorrectionMaxCost = 450;
constexpr size_t kSentenceCorrectionDecodes = 6;

// Relative weights of the character language model (milli-nats), the lexicon
// prior and the clause-final probability. Calibrated on held-out development
// cases only (0.13: the even lines of the held-out sets); see
// docs/CONTEXT_MODEL.md for the measurements.
struct ScoreWeights {
    double language_model = 0.7;
    double lexical = 3.0;
    double finish = 0.5;
    // The clause-final probability for sentences of four or more characters,
    // where a sentence-final particle (吧, 吗) is a strong cue.
    double finish_sentence = 1.0;
    int split_penalty = 1800;
    int boundary_penalty = -1000;
    // Lexicon bonus for a 2-4 character word, and for a frequent 5-8
    // character expression.
    int word_prior = 350;
    int long_word_prior = 2000;
    // Cost of each token a sentence is cut into.
    int segment_penalty = 600;
    size_t beam = kSentenceBeamWidth;
    // Readings of one span scored by the language model, best priors first;
    // 48 reaches rarer homophones (哈佛, 嬉水) for about 0.7 ms a keystroke (1.8.3).
    size_t span_candidates = 48;
    // Word association (word_zh.lm) bonus for each pair of adjacent words;
    // chosen on the development half like the weights above (1.4.0), and for
    // each run of three (1.5.0).
    double word_model = 0.35;
    double word_triples = 0.4;
    // Neural rescoring of whole-input sentences (1.6.0): nats of the
    // character GRU per milli-nat of the decoder, in thousandths.
    double neural = 0.4;
    // Clause-start language model share of a word typed on its own.
    double standalone_language_model = 0.1;
    // A short phrase typed on its own (phrase_zh.lm): frequency points per
    // tenfold of its chat count, what it must win by, and its longest length.
    bool initials = true;
    double chat_phrase = 800.0;
    double chat_phrase_margin = 500.0;
    size_t chat_phrase_length = 4;
};

// Fixed in the IME; the evaluation tool may replace them to calibrate on the
// development half (PinyinEngine::set_evaluation_weights).
ScoreWeights g_weights{};
const ScoreWeights& weights() { return g_weights; }

struct ParsedInput {
    std::wstring pinyin;
    std::vector<size_t> raw_ends;
    std::vector<size_t> boundaries;
};

ParsedInput parse_input(const std::wstring& raw) {
    ParsedInput result;
    for (size_t index = 0; index < raw.size(); ++index) {
        wchar_t ch = raw[index];
        if (ch >= L'A' && ch <= L'Z') ch = ch - L'A' + L'a';
        if ((ch >= L'a' && ch <= L'z') || (ch >= L'0' && ch <= L'9')) {
            result.pinyin.push_back(ch);
            result.raw_ends.push_back(index + 1);
        } else if (ch == L'\'' && !result.pinyin.empty()) {
            result.boundaries.push_back(result.pinyin.size());
        }
    }
    // One letter for one letter, so raw positions stay valid.
    spell_ue(result.pinyin);
    return result;
}

size_t raw_prefix_end(const ParsedInput& input, const std::wstring& raw, size_t normalized_end) {
    size_t end = input.raw_ends[normalized_end - 1];
    while (end < raw.size() && raw[end] == L'\'') ++end;
    return end;
}

// Characters, not UTF-16 units: a CJK Extension B+ character is one character
// written as a surrogate pair, and must not count as a two-character word.
size_t character_count(const std::wstring& text) {
    return static_cast<size_t>(std::count_if(text.begin(), text.end(),
                                             [](wchar_t ch) { return ch < 0xDC00 || ch > 0xDFFF; }));
}

int lexical_score(const Candidate& candidate) {
    const size_t characters = character_count(candidate.text);
    int word_prior = characters >= 2 && characters <= 4 ? weights().word_prior : 0;
    // A deliberately frequent project/user expression is useful evidence, even
    // when it spans more than a conventional word. Keep the preference modest:
    // every character still receives the same context score as any other path.
    if (characters >= 5 && characters <= 8 && candidate.base_score >= 8000) word_prior = weights().long_word_prior;
    const int frequency = std::clamp(candidate.decoding_frequency > 0 ? candidate.decoding_frequency : candidate.base_score, 0, 20000);
    // Project frequencies encode a deliberate common-word preference. Keep
    // that evidence meaningful beside ngram scores while limiting each token.
    const int frequency_prior = (frequency - 9000) / 4;
    return static_cast<int>(std::log1p(static_cast<double>(frequency)) * 150) - 1450 + word_prior + frequency_prior;
}

bool can_compose_token(const Candidate& candidate, bool whole_key_match) {
    // Imported Latin names have spelling keys, which can also be valid pinyin
    // syllables. Keep explicit whole-name lookup and personal choices, while
    // avoiding accidental Latin fragments in automatically composed Chinese.
    return whole_key_match || candidate.is_user_lexicon_entry || candidate.user_frequency > 0 ||
           std::none_of(candidate.text.begin(), candidate.text.end(), [](wchar_t ch) {
               return (ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z');
           });
}

// Language-model history: the last three characters before the next token.
// An English term is set apart by a space, which starts a new run of Chinese.
std::wstring transition_history(const std::wstring& preceding, const std::wstring& text) {
    std::wstring joined = text.size() >= 3 ? text.substr(text.size() - 3)
                                           : preceding.substr(preceding.size() > 3 ? preceding.size() - 3 : 0) + text;
    if (joined.size() > 3) joined = joined.substr(joined.size() - 3);
    const auto space = joined.find_last_of(L' ');
    return space == std::wstring::npos ? joined : joined.substr(space + 1);
}

// An English term of mixed input stands apart: 我用 Python 写代码. (Latin
// dictionary entries such as names join as before, without a space.)
std::wstring join_text(const std::wstring& left, const std::wstring& right, bool left_english, bool right_english) {
    if (left.empty() || right.empty() || !(left_english || right_english)) return left + right;
    return left + L' ' + right;
}

std::wstring normalize_input(const std::wstring& input) {
    std::wstring normalized;
    normalized.reserve(input.size());
    for (wchar_t ch : input) {
        if (ch >= L'A' && ch <= L'Z') {
            normalized.push_back(static_cast<wchar_t>(ch - L'A' + L'a'));
        } else if (ch >= L'a' && ch <= L'z') {
            normalized.push_back(ch);
        } else if (ch >= L'0' && ch <= L'9') {
            normalized.push_back(ch);
        }
    }
    spell_ue(normalized);
    return normalized;
}

struct SegmentPath {
    int score = 0;
    int segment_count = 0;
    std::wstring text;
    // The last token is an English term (mixed input), not Chinese.
    bool english_end = false;
    // The last two Chinese words, for the word association model.
    std::wstring last_word;
    std::wstring word_before_last;
};

void prune_paths(std::vector<SegmentPath>& paths) {
    std::stable_sort(paths.begin(), paths.end(), [](const auto& left, const auto& right) {
        return left.score != right.score ? left.score > right.score : left.segment_count < right.segment_count;
    });
    std::vector<SegmentPath> kept;
    std::unordered_map<std::wstring, size_t> state_counts;
    for (auto& path : paths) {
        const auto tail = path.text.substr(path.text.size() > 3 ? path.text.size() - 3 : 0);
        // Paths with the same three final characters have identical future
        // 4gram transitions; preserve different states before widening ties.
        if (state_counts[tail]++ >= 2) continue;
        kept.push_back(std::move(path));
        if (kept.size() == weights().beam) break;
    }
    paths = std::move(kept);
}

void add_path(std::vector<SegmentPath>& paths, std::unordered_map<std::wstring, size_t>& indices,
              SegmentPath proposed) {
    const auto existing = indices.find(proposed.text);
    if (existing == indices.end()) {
        indices.emplace(proposed.text, paths.size());
        paths.push_back(std::move(proposed));
    } else if (proposed.score > paths[existing->second].score) {
        paths[existing->second] = std::move(proposed);
    }
}

struct DecodeOptions {
    unsigned fuzzy = 0;
    // Non-null: English terms may be tokens of the sentence.
    const EnglishTerms* english = nullptr;
    // Non-null: words of the chosen usage scenes rank higher.
    const DomainLexicon* domains = nullptr;
    // Non-null: adjacent word pairs seen together often score higher.
    const WordModel* words = nullptr;
};

int word_bonus(const DecodeOptions& options, const SegmentPath& path, const std::wstring& word, bool ends_input) {
    if (!options.words) return 0;
    int bonus = 0;
    if (weights().word_model != 0.0) bonus += static_cast<int>(weights().word_model * options.words->pair_score(path.last_word, word));
    // A run of three ending the input in a single character is mostly the
    // start of a longer word (但是 是 自 from 但是是自己), which would favour
    // loose characters over the word being typed (式子).
    if (weights().word_triples != 0.0 && !(ends_input && character_count(word) == 1))
        bonus += static_cast<int>(weights().word_triples * options.words->triple_score(path.word_before_last, path.last_word, word));
    return bonus;
}

int english_prior(const EnglishTerms::Term& term, size_t length) {
    return kEnglishBase + term.weight * kEnglishWeightScale + kEnglishLengthBonus * static_cast<int>(std::min<size_t>(length, 8));
}

// The lattice of an earlier input sharing this one's first `stable` letters:
// its positions up to `stable` hold their final, pruned paths and are copied;
// only words ending after `stable` are scored again.
struct DecodeReuse {
    const std::vector<std::vector<SegmentPath>>* lattice = nullptr;
    size_t stable = 0;
};

std::vector<std::vector<SegmentPath>> decode(const Dictionary& dictionary, const CandidateRanker& ranker,
                                           const UserLearning& learning, const ContextModel& context,
                                           const ParsedInput& input, const std::wstring& preceding,
                                           const DecodeOptions& options = {}, const DecodeReuse& reuse = {}) {
    const auto& pinyin = input.pinyin;
    std::vector<std::vector<SegmentPath>> dp(pinyin.size() + 1);
    const size_t stable = reuse.lattice && !options.english ? std::min(reuse.stable, pinyin.size()) : 0;
    for (size_t position = 0; position <= stable && reuse.lattice && position < reuse.lattice->size(); ++position) dp[position] = (*reuse.lattice)[position];
    if (stable == 0) dp[0].assign(1, SegmentPath{});
    if (options.words && stable == 0) {
        auto& start = dp[0].back();
        start.last_word = options.words->previous_word(preceding);
        if (!start.last_word.empty())
            start.word_before_last = options.words->previous_word(
                std::wstring_view(preceding).substr(0, preceding.size() - start.last_word.size()));
    }
    std::vector<int> split_penalties(pinyin.size() + 1);
    for (size_t start = 0; start < pinyin.size(); ++start) {
        for (size_t length = 2; length <= 6 && start + length <= pinyin.size(); ++length) {
            if (!Dictionary::is_syllable(pinyin.substr(start, length))) continue;
            for (size_t boundary = start + 1; boundary < start + length; ++boundary) split_penalties[boundary] = weights().split_penalty;
        }
    }
    for (size_t boundary : input.boundaries) split_penalties[boundary] = 0;

    for (size_t offset = 0; offset < pinyin.size(); ++offset) {
        if (dp[offset].empty()) {
            continue;
        }
        // Copied positions are already pruned; from them, only words that
        // end after the copied part add anything.
        const size_t first_new_end = offset < stable ? stable + 1 : 0;
        if (offset >= stable) prune_paths(dp[offset]);

        // Literal keys, then keys that use fuzzy partners (a small penalty).
        std::vector<std::pair<std::wstring, size_t>> parts;
        for (auto& key : dictionary.matching_pinyins_at(pinyin, offset)) {
            const size_t length = key.size();
            parts.emplace_back(std::move(key), length);
        }
        // Learned phrases (造词) whose pinyin is not a dictionary key.
        for (auto& key : learning.phrase_keys_at(pinyin, offset)) {
            if (std::none_of(parts.begin(), parts.end(), [&](const auto& part) { return part.first == key; })) {
                const size_t length = key.size();
                parts.emplace_back(std::move(key), length);
            }
        }
        const size_t literal_parts = parts.size();
        for (auto& match : dictionary.fuzzy_matches_at(pinyin, offset, input.boundaries, options.fuzzy))
            parts.push_back(std::move(match));
        for (size_t part = 0; part < parts.size(); ++part) {
            if (offset + parts[part].second < first_new_end) continue;
            const auto& part_pinyin = parts[part].first;
            const bool fuzzy = part >= literal_parts;
            auto part_candidates = dictionary.lookup(part_pinyin);
            if (options.domains) options.domains->apply(part_pinyin, part_candidates);
            if (!fuzzy) {
                for (const auto& phrase : learning.words_for(part_pinyin)) {
                    if (character_count(phrase) < 2 || std::any_of(part_candidates.begin(), part_candidates.end(),
                            [&](const Candidate& candidate) { return candidate.text == phrase; })) continue;
                    Candidate learned;
                    learned.text = phrase;
                    learned.base_score = kLearnedPhraseFrequency;
                    part_candidates.push_back(std::move(learned));
                }
            }
            ranker.apply_learning(part_pinyin, part_candidates, &learning);
            if (part_candidates.empty()) {
                continue;
            }

            const size_t next = offset + parts[part].second;
            auto& paths = dp[next];
            std::vector<size_t> boundaries;
            for (size_t boundary : input.boundaries) {
                if (boundary > offset && boundary < next) boundaries.push_back(boundary - offset);
            }
            // A single character chosen elsewhere keeps a quarter of its
            // learned bonus inside a sentence (把 for 我爸爸, 的 for 得奖), the
            // rest only right after the character it was chosen after.
            struct PreparedCandidate { Candidate candidate; int prior; int contextual_bonus; };
            std::vector<PreparedCandidate> prepared;
            prepared.reserve(part_candidates.size());
            for (auto& candidate : part_candidates) {
                if (!can_compose_token(candidate, offset == 0 && next == pinyin.size())) continue;
                // Fuzzy keys follow the typed syllables, so typed boundaries hold.
                if (!fuzzy && !dictionary.reading_matches_boundaries(part_pinyin, candidate.text, boundaries)) continue;
                const bool single = character_count(candidate.text) == 1;
                const int learned = candidate.user_frequency / 3;
                const int kept = single ? learned / kOutOfContextLearningDivisor : learned;
                const int prior = static_cast<int>(weights().lexical * lexical_score(candidate)) + kept -
                                  (fuzzy ? kFuzzyPenalty : 0) + (candidate.domain ? kDomainBonus : 0);
                prepared.push_back({std::move(candidate), prior, learned - kept});
            }
            std::stable_sort(prepared.begin(), prepared.end(), [](const auto& left, const auto& right) {
                return left.prior > right.prior;
            });
            if (prepared.size() > weights().span_candidates) prepared.resize(weights().span_candidates);
            std::unordered_map<std::wstring, size_t> indices;
            indices.reserve(paths.size() + dp[offset].size() * prepared.size());
            for (size_t index = 0; index < paths.size(); ++index) indices.emplace(paths[index].text, index);
            const long long now = static_cast<long long>(std::time(nullptr));
            for (const auto& path : dp[offset]) {
                const auto history = transition_history(preceding, path.text);
                const wchar_t before = history.empty() ? 0 : history.back();
                const bool han_before = before >= 0x3400 && before <= 0x9FFF;
                for (const auto& item : prepared) {
                    const int here = item.contextual_bonus > 0 && han_before &&
                                     learning.faded_steps(UserLearning::contextual_key(part_pinyin, before), item.candidate.text, now) >= 0
                                         ? item.contextual_bonus : 0;
                    const int score = path.score + item.prior + here - split_penalties[next] - weights().segment_penalty +
                                      static_cast<int>(weights().language_model * context.extend_score(history, item.candidate.text)) +
                                      word_bonus(options, path, item.candidate.text, next == pinyin.size());
                    add_path(paths, indices, SegmentPath{score, path.segment_count + 1,
                                                         join_text(path.text, item.candidate.text, path.english_end, false),
                                                         false, item.candidate.text, path.last_word});
                }
            }
            if (paths.size() > weights().beam * 2) prune_paths(paths);
        }
        // English terms (Python, GitHub, PPT) as tokens of the sentence.
        if (options.english) {
            std::vector<std::pair<size_t, const EnglishTerms::Term*>> terms;
            options.english->matches_at(pinyin, offset, terms);
            for (const auto& [length, term] : terms) {
                const size_t next = offset + length;
                if (std::any_of(input.boundaries.begin(), input.boundaries.end(),
                                [&](size_t boundary) { return boundary > offset && boundary < next; })) continue;
                auto& paths = dp[next];
                std::unordered_map<std::wstring, size_t> indices;
                for (size_t index = 0; index < paths.size(); ++index) indices.emplace(paths[index].text, index);
                for (const auto& path : dp[offset]) {
                    add_path(paths, indices, SegmentPath{path.score + english_prior(*term, length) - split_penalties[next],
                                                         path.segment_count + 1, join_text(path.text, term->text, path.english_end, true), true});
                }
                if (paths.size() > weights().beam * 2) prune_paths(paths);
            }
        }
    }
    prune_paths(dp.back());
    return dp;
}

bool all_han(const std::wstring& text) {
    return !text.empty() && std::all_of(text.begin(), text.end(), [](wchar_t ch) { return ch >= 0x3400 && ch <= 0x9FFF; });
}

// Sentences typed as initials (wkdlxl, 我快到楼下了). Each letter may begin one
// character, and a run of letters may spell a word's initials; the language
// models then read it like any other sentence.
std::vector<std::vector<SegmentPath>> decode_initials(const Dictionary& dictionary, const ContextModel& context, const std::wstring& input,
                                                      const std::wstring& preceding, const DecodeOptions& options,
                                                      const std::function<const std::vector<Candidate>&(const std::wstring&)>& characters) {
    std::vector<std::vector<SegmentPath>> dp(input.size() + 1);
    dp[0].assign(1, SegmentPath{});
    if (options.words) {
        auto& start = dp[0].back();
        start.last_word = options.words->previous_word(preceding);
        if (!start.last_word.empty())
            start.word_before_last = options.words->previous_word(std::wstring_view(preceding).substr(0, preceding.size() - start.last_word.size()));
    }
    for (size_t offset = 0; offset < input.size(); ++offset) {
        if (dp[offset].empty()) continue;
        prune_paths(dp[offset]);
        const auto extend = [&](const Candidate& word, size_t next) {
            auto& paths = dp[next];
            std::unordered_map<std::wstring, size_t> indices;
            indices.reserve(paths.size() + dp[offset].size());
            for (size_t index = 0; index < paths.size(); ++index) indices.emplace(paths[index].text, index);
            for (const auto& path : dp[offset]) {
                const auto history = transition_history(preceding, path.text);
                const int score = path.score + static_cast<int>(weights().lexical * lexical_score(word)) - weights().segment_penalty +
                                  static_cast<int>(weights().language_model * context.extend_score(history, word.text)) +
                                  word_bonus(options, path, word.text, next == input.size());
                add_path(paths, indices, SegmentPath{score, path.segment_count + 1, join_text(path.text, word.text, path.english_end, false),
                                                     false, word.text, path.last_word});
            }
            if (paths.size() > weights().beam * 2) prune_paths(paths);
        };
        for (const auto& word : characters(input.substr(offset, 1))) extend(word, offset + 1);
        if (offset + 1 < input.size() && input[offset + 1] == L'h' && std::wstring_view(L"zcs").find(input[offset]) != std::wstring_view::npos)
            for (const auto& word : characters(input.substr(offset, 2))) extend(word, offset + 2);
        for (size_t length = 2; length <= kLongestInitialsWord && offset + length <= input.size(); ++length) {
            size_t added = 0;
            for (const auto& word : dictionary.lookup_abbreviation(input.substr(offset, length))) {
                if (!all_han(word.text)) continue;
                extend(word, offset + length);
                if (++added == kInitialsWordsPerSpan) break;
            }
        }
    }
    prune_paths(dp.back());
    return dp;
}

// Letters that no reading explains and that look like initials: consonants
// only, since with a vowel it is more likely a syllable being typed (wok, for
// 我看) or a slip in one (hsao), which completion and correction handle. So is
// a lone zh, ch or sh, which lists the common characters (这, 吃, 是).
bool looks_like_initials(const std::wstring& input) {
    return input != L"zh" && input != L"ch" && input != L"sh" && input.size() >= 2 && input.size() <= kLongestInitialsInput && std::none_of(input.begin(), input.end(), [](wchar_t letter) {
        return letter < L'a' || letter > L'z' || std::wstring_view(L"aeiouv").find(letter) != std::wstring_view::npos;
    });
}

// Reorders the candidates that read the whole input by their decoder score
// plus the neural model's probability of the sentence after the preceding
// clause. A first choice the decoder did not rank first (placed there by
// learning or kept as a whole word) keeps its place, as does every other kind
// of candidate.
void rescore_with_neural(const NeuralModel& model, std::vector<Candidate>& ranked, const std::wstring& preceding,
                         bool first_is_word) {
    if (model.empty() || weights().neural == 0.0 || ranked.size() < 2 || ranked.front().consumed_pinyin != 0) return;
    // A word typed on its own keeps the lexicon order: a short one, or one
    // the lexicon has for exactly this input (二项式定理, not 而像是定理).
    if (preceding.empty() && (character_count(ranked.front().text) <= 2 || first_is_word)) return;
    std::vector<size_t> slots;
    // The first five sentences: further down the model rarely changes the
    // first choice, and each one costs scoring on every keystroke.
    for (size_t index = 0; index < ranked.size() && slots.size() < kNeuralCandidates; ++index)
        if (ranked[index].consumed_pinyin == 0 && all_han(ranked[index].text)) slots.push_back(index);
    if (slots.size() < 2 || slots.front() != 0) return;
    int highest = std::numeric_limits<int>::min();
    for (size_t slot : slots) highest = std::max(highest, ranked[slot].total_score());
    if (ranked.front().total_score() < highest) return;
    size_t start = preceding.size();
    while (start > 0 && preceding.size() - start < 24 && preceding[start - 1] >= 0x3400 && preceding[start - 1] <= 0x9FFF) --start;
    std::vector<std::wstring> texts;
    for (size_t slot : slots) texts.push_back(ranked[slot].text);
    const auto probabilities = model.score(std::wstring_view(preceding).substr(start), texts);
    std::vector<std::pair<double, Candidate>> scored;
    for (size_t i = 0; i < slots.size(); ++i)
        scored.emplace_back(ranked[slots[i]].total_score() + weights().neural * 1000.0 * probabilities[i], ranked[slots[i]]);
    std::stable_sort(scored.begin(), scored.end(), [](const auto& left, const auto& right) { return left.first > right.first; });
    for (size_t i = 0; i < slots.size(); ++i) ranked[slots[i]] = std::move(scored[i].second);
}

// Whether every character of `text` has a lexicon reading that together spell
// `pinyin` exactly: fuzzy readings and corrections are not what was typed.
template <typename Words>
bool reads_exactly(const std::wstring& pinyin, const std::wstring& text, const Words& words_for) {
    std::vector<std::vector<bool>> reachable(text.size() + 1, std::vector<bool>(pinyin.size() + 1, false));
    reachable[0][0] = true;
    for (size_t chars = 0; chars < text.size(); ++chars) {
        for (size_t letters = 0; letters < pinyin.size(); ++letters) {
            if (!reachable[chars][letters]) continue;
            for (size_t length = 1; length <= kLongestSyllable && letters + length <= pinyin.size(); ++length) {
                const auto syllable = pinyin.substr(letters, length);
                if (!Dictionary::is_syllable(syllable)) continue;
                const auto found = words_for(syllable);
                if (std::any_of(found.begin(), found.end(), [&](const auto& word) { return word.text.size() == 1 && word.text[0] == text[chars]; }))
                    reachable[chars + 1][letters + length] = true;
            }
        }
    }
    return reachable[text.size()][pinyin.size()];
}

// A short phrase typed on its own: the reading people actually send moves to
// first when its chat count outweighs the lexicon's preference. Anything the
// user has chosen stays where it is, and single characters keep their order
// (a character sent alone says little about the character in general).
template <typename Words>
void promote_chat_phrase(std::vector<Candidate>& ranked, const std::vector<Candidate>& exact, const PhraseCounts& counts,
                         const Words& words_for, const std::wstring& pinyin, bool initials) {
    if (counts.empty() || weights().chat_phrase == 0.0 || ranked.size() < 2) return;
    const auto& first = ranked.front();
    const size_t characters = character_count(first.text);
    if (first.consumed_pinyin != 0 || characters < 2 || characters > weights().chat_phrase_length || first.user_frequency > 0 || first.is_user_lexicon_entry ||
        first.correction || !all_han(first.text)) return;
    // A sentence the decoder composed and the table has never seen is no
    // evidence against it.
    if (!initials && counts.level(first.text) == 0 && std::none_of(exact.begin(), exact.end(), [&](const auto& word) { return word.text == first.text; })) return;
    const auto score = [&](const std::wstring& text) {
        int frequency = kComposedPhraseFrequency;
        for (const auto& word : exact) if (word.text == text) frequency = std::max(word.is_user_lexicon_entry ? 0 : word.base_score, frequency);
        return frequency + weights().chat_phrase * counts.level(text) / PhraseCounts::kLevelsPerDecade;
    };
    size_t best = 0;
    double highest = score(first.text) + weights().chat_phrase_margin;
    for (size_t index = 1; index < ranked.size(); ++index) {
        const auto& candidate = ranked[index];
        if (candidate.consumed_pinyin != 0 || candidate.correction || candidate.user_frequency > 0 || character_count(candidate.text) != characters ||
            !all_han(candidate.text) || counts.level(candidate.text) == 0) continue;
        const double value = score(candidate.text);
        if (value > highest) {
            highest = value;
            best = index;
        }
    }
    if (best == 0 || (!initials && !reads_exactly(pinyin, ranked[best].text, words_for))) return;
    std::rotate(ranked.begin(), ranked.begin() + static_cast<std::ptrdiff_t>(best), ranked.begin() + static_cast<std::ptrdiff_t>(best) + 1);
}

Candidate candidate_from_path(const SegmentPath& path, const ContextModel& context) {
    Candidate candidate;
    // A sentence ending in an English term has no Chinese clause ending to score.
    const double finish = character_count(path.text) >= 4 ? weights().finish_sentence : weights().finish;
    candidate.base_score = path.score +
        (path.english_end ? 0 : static_cast<int>(finish * weights().language_model * context.finish_score(path.text)));
    candidate.complete_segmentation_bonus = kCompleteSegmentationBonus;
    candidate.text = path.text;
    return candidate;
}

// Words for single-keystroke corrections of the whole input, cheapest first.
std::vector<Candidate> correction_candidates(const Dictionary& dictionary, const std::wstring& input, bool broad,
                                             int max_cost) {
    if (input.size() < 2 || input.size() > 32) return {};
    std::vector<Candidate> result;
    size_t probes = 0;
    for (const auto& correction : spelling_corrections(input, broad)) {
        if (correction.cost > max_cost || result.size() >= 24 || ++probes > 1200) break;
        if (!dictionary.has_key(correction.pinyin)) continue;
        auto matches = dictionary.lookup(correction.pinyin);
        for (size_t index = 0; index < std::min<size_t>(3, matches.size()); ++index) {
            matches[index].segmentation_penalty = 4000 + correction.cost;
            matches[index].correction = true;
            result.push_back(std::move(matches[index]));
        }
    }
    return result;
}

int top_frequency(const std::vector<Candidate>& words) {
    int best = 0;
    for (const auto& word : words) best = std::max(best, word.base_score);
    return best;
}

std::vector<Candidate> deduplicate_and_limit(std::vector<Candidate> ranked) {
    std::vector<Candidate> result;
    std::unordered_set<std::wstring> seen;
    for (const auto& candidate : ranked) {
        if (candidate.text.empty()) {
            continue;
        }
        if (!seen.insert(candidate.text).second) {
            continue;
        }
        result.push_back(candidate);
        if (result.size() >= kMaxCandidateCount) {
            break;
        }
    }
    return result;
}

}  // namespace

PinyinEngine::PinyinEngine() {
    open_learning(Dictionary::default_user_lexicon_path());
    const auto resources = std::filesystem::path(Dictionary::default_resource_path()).parent_path();
    rebuild_context_model((resources / L"context_zh.tsv").wstring());
    english_.load((resources / L"english_terms.tsv").wstring());
    domain_directory_ = (resources / L"domains").wstring();
}

PinyinEngine::PinyinEngine(DictionaryLoadMode dictionary_load_mode)
    : dictionary_(dictionary_load_mode) {
    open_learning(Dictionary::default_user_lexicon_path());
    rebuild_context_model(dictionary_load_mode == DictionaryLoadMode::Empty ? L"" :
                          (std::filesystem::path(Dictionary::default_resource_path()).parent_path() / L"context_zh.tsv").wstring());
}

void PinyinEngine::rebuild_context_model(const std::wstring& context_path) {
    ++decode_epoch_;
    context_path_ = context_path;
    context_model_.reset(dictionary_);
    context_model_.set_boundary_penalty(weights().boundary_penalty);
    if (!context_path.empty()) {
        // The packaged binary model supersedes the authored count table.
        const auto binary = std::filesystem::path(context_path).replace_extension(L".lm");
        std::error_code error;
        if (!std::filesystem::is_regular_file(binary, error) || !context_model_.load(binary.wstring()))
            context_model_.load(context_path);
    }
    word_model_.reset();
    if (!context_path.empty()) word_model_.load((std::filesystem::path(context_path).parent_path() / L"word_zh.lm").wstring());
    neural_model_.reset();
    if (!context_path.empty()) neural_model_.load((std::filesystem::path(context_path).parent_path() / L"neural_zh.lm").wstring());
    initial_characters_.clear();
    phrase_counts_.reset();
    if (!context_path.empty()) phrase_counts_.load((std::filesystem::path(context_path).parent_path() / L"phrase_zh.lm").wstring());
    continuation_model_.reset(dictionary_);
    if (!context_path.empty()) continuation_model_.load((std::filesystem::path(context_path).parent_path() / L"continuations_zh.tsv").wstring());
    learning_revision_ = static_cast<size_t>(-1);
}

bool PinyinEngine::load_context_model(const std::wstring& path) {
    ++decode_epoch_;
    context_path_ = path;
    context_model_.reset(dictionary_);
    learning_revision_ = static_cast<size_t>(-1);
    return context_model_.load(path);
}

bool PinyinEngine::load_continuation_model(const std::wstring& path) {
    return continuation_model_.load(path);
}

std::vector<Candidate> PinyinEngine::predict(const std::wstring& preceding_text, size_t limit) const {
    learning_.refresh_if_changed();
    if (learning_revision_ != learning_.revision()) {
        const auto selections = learning_.selected_texts();
        context_model_.set_learning(selections);
        continuation_model_.set_learning(selections);
        learning_revision_ = learning_.revision();
    }
    return continuation_model_.predict(preceding_text, limit);
}

void PinyinEngine::open_learning(const std::wstring& user_lexicon_path) {
    if (user_lexicon_path.empty()) return;
    const auto parent = std::filesystem::path(user_lexicon_path).parent_path();
    learning_.open((parent / L"user_learning.tsv").wstring());
    phrases_.open((parent / L"custom_phrases.tsv").wstring());
    emoji_.open((parent / L"emoji_candidates.tsv").wstring());
}

bool PinyinEngine::is_tool_input(const std::wstring& typed) {
    return typed.size() >= 2 && (typed[0] == L'v' || typed[0] == L'V') && is_tool_text(typed.substr(1));
}

void PinyinEngine::place_phrases(const std::wstring& pinyin, std::vector<Candidate>& ranked) const {
    phrases_.refresh_if_changed();
    const auto& phrases = phrases_.for_pinyin(pinyin);
    if (phrases.empty()) return;
    const long long now = static_cast<long long>(std::time(nullptr));
    // When each phrase was last chosen (or added), latest first.
    std::vector<std::pair<long long, std::wstring>> latest;
    for (const auto& phrase : phrases)
        latest.emplace_back(std::max(phrase.created, learning_.last_selected(pinyin, phrase.text)), phrase.text);
    std::stable_sort(latest.begin(), latest.end(), [](const auto& left, const auto& right) { return left.first > right.first; });
    ranked.erase(std::remove_if(ranked.begin(), ranked.end(), [&](const Candidate& candidate) {
        return std::any_of(latest.begin(), latest.end(), [&](const auto& phrase) { return phrase.second == candidate.text; });
    }), ranked.end());
    // The leading candidate holds first place only if chosen after the phrase.
    const long long leading = !ranked.empty() && learning_.faded_steps(pinyin, ranked.front().text, now) >= 0
                                  ? learning_.last_selected(pinyin, ranked.front().text) : 0;
    size_t slot = 0;
    for (const auto& [time, text] : latest) {
        const size_t at = std::min(slot == 0 && leading > time ? size_t{1} : slot, ranked.size());
        Candidate candidate{text, kLearnedPhraseFrequency, 0};
        ranked.insert(ranked.begin() + static_cast<std::ptrdiff_t>(at), std::move(candidate));
        slot = at + 1;
    }
    if (ranked.size() > kMaxCandidateCount) ranked.resize(kMaxCandidateCount);
}

void PinyinEngine::place_emoji(const std::wstring& pinyin, std::vector<Candidate>& ranked) const {
    if (!options_.emoji_candidates || ranked.empty()) return;
    emoji_.refresh_if_changed();
    auto emoji = emoji_.for_pinyin(pinyin);
    if (emoji.empty()) return;
    std::stable_sort(emoji.begin(), emoji.end(), [](const CustomPhrase& left, const CustomPhrase& right) {
        return left.created > right.created;
    });
    size_t at = std::min<size_t>(2, ranked.size());
    for (size_t index = 0; index < emoji.size() && index < 2; ++index) {
        if (std::any_of(ranked.begin(), ranked.end(), [&](const Candidate& candidate) { return candidate.text == emoji[index].text; }))
            continue;
        ranked.insert(ranked.begin() + static_cast<std::ptrdiff_t>(at), Candidate{emoji[index].text, kFallbackScore + 1, 0});
        ++at;
    }
    if (ranked.size() > kMaxCandidateCount) ranked.resize(kMaxCandidateCount);
}

bool PinyinEngine::load_dictionary(const std::wstring& path) {
    dictionary_.clear();
    const auto extension = path.substr(path.find_last_of(L'.') == std::wstring::npos ? path.size() : path.find_last_of(L'.'));
    const bool loaded = extension == L".tsv" || extension == L".TSV" ?
                        dictionary_.load_from_tsv_file(path) : dictionary_.load_from_json_file(path);
    rebuild_context_model((std::filesystem::path(path).parent_path() / L"context_zh.tsv").wstring());
    return loaded;
}

bool PinyinEngine::load_dictionary_resource_directory(const std::wstring& directory,
                                                      const std::wstring& user_lexicon_path,
                                                      UserLexiconCreateMode create_mode) {
    dictionary_.clear();
    const auto user_path = user_lexicon_path.empty() ? Dictionary::default_user_lexicon_path() : user_lexicon_path;
    open_learning(user_path);
    const auto resources = std::filesystem::path(directory) / L"dictionary";
    const bool loaded = dictionary_.load_from_layered_files(
        (resources / L"core_zh_pinyin.tsv").wstring(),
        (resources / L"local_core_zh_pinyin.tsv").wstring(), user_path, create_mode,
        (resources / L"standard_zh_pinyin.tsv").wstring());
    rebuild_context_model((std::filesystem::path(directory) / L"dictionary" / L"context_zh.tsv").wstring());
    english_.load((resources / L"english_terms.tsv").wstring());
    domain_directory_ = (resources / L"domains").wstring();
    if (options_.domains) domains_.load(domain_directory_, options_.domains);
    return loaded;
}

const std::vector<Candidate>& PinyinEngine::characters_for_initial(const std::wstring& initial) const {
    const auto found = initial_characters_.find(initial);
    if (found != initial_characters_.end()) return found->second;
    static constexpr const wchar_t* kFinals[] = {L"", L"a", L"o", L"e", L"i", L"u", L"v", L"ai", L"ei", L"ao", L"ou", L"an", L"en", L"ang", L"eng", L"ong", L"ia", L"ie", L"iu", L"in", L"ing", L"ian",
                                                 L"iao", L"iang", L"iong", L"ua", L"uo", L"ui", L"un", L"uai", L"uan", L"uang", L"ue", L"ve", L"er", L"r"};
    std::vector<Candidate> characters;
    const auto add = [&](const std::wstring& start) {
        for (const wchar_t* final_part : kFinals) {
            const std::wstring syllable = start + final_part;
            if (!Dictionary::is_syllable(syllable)) continue;
            for (const auto& word : dictionary_.lookup(syllable)) {
                if (word.text.size() != 1 || !all_han(word.text)) continue;
                const auto same = std::find_if(characters.begin(), characters.end(), [&](const auto& item) { return item.text == word.text; });
                if (same == characters.end()) characters.push_back(word);
                else if (same->base_score < word.base_score) *same = word;
            }
        }
    };
    add(initial);
    // z, c and s also begin zh, ch and sh syllables.
    if (initial == L"z" || initial == L"c" || initial == L"s") add(initial + L"h");
    std::stable_sort(characters.begin(), characters.end(), [](const auto& left, const auto& right) { return left.base_score > right.base_score; });
    if (characters.size() > kCharactersPerInitial) characters.resize(kCharactersPerInitial);
    return initial_characters_.emplace(initial, std::move(characters)).first->second;
}

std::vector<Candidate> PinyinEngine::words_for(const std::wstring& pinyin) const {
    auto words = dictionary_.lookup(pinyin);
    domains_.apply(pinyin, words);
    return words;
}

void PinyinEngine::set_input_options(const EngineInputOptions& options) noexcept {
    // The text service passes the saved options on every key; only a change
    // discards the cached lattice.
    if (!(options == options_)) ++decode_epoch_;
    options_ = options;
    if (options.domains == domains_.mask()) return;
    try {
        domains_.load(domain_directory_, options.domains);
    } catch (...) {
        // An unreadable pack leaves the scenes unapplied; typing goes on.
        domains_ = DomainLexicon{};
    }
}

bool PinyinEngine::set_evaluation_weights(const std::wstring& spec) {
    // "lm=0.7,lex=3,finish=0.5,split=1800,boundary=-1000,word=350,long=2000,segment=0,beam=32"
    ScoreWeights next = g_weights;
    size_t start = 0;
    while (start < spec.size()) {
        const size_t comma = spec.find(L',', start);
        const std::wstring item = spec.substr(start, comma == std::wstring::npos ? std::wstring::npos : comma - start);
        const size_t equals = item.find(L'=');
        if (equals == std::wstring::npos) return false;
        const std::wstring key = item.substr(0, equals);
        double value = 0;
        try { value = std::stod(item.substr(equals + 1)); } catch (...) { return false; }
        if (key == L"lm") next.language_model = value;
        else if (key == L"lex") next.lexical = value;
        else if (key == L"finish") next.finish = value;
        else if (key == L"finish4") next.finish_sentence = value;
        else if (key == L"split") next.split_penalty = static_cast<int>(value);
        else if (key == L"boundary") next.boundary_penalty = static_cast<int>(value);
        else if (key == L"word") next.word_prior = static_cast<int>(value);
        else if (key == L"long") next.long_word_prior = static_cast<int>(value);
        else if (key == L"segment") next.segment_penalty = static_cast<int>(value);
        else if (key == L"beam") next.beam = static_cast<size_t>(std::max(4.0, value));
        else if (key == L"span") next.span_candidates = static_cast<size_t>(std::max(4.0, value));
        else if (key == L"wm") next.word_model = value;
        else if (key == L"wm3") next.word_triples = value;
        else if (key == L"nn") next.neural = value;
        else if (key == L"stand") next.standalone_language_model = value;
        else if (key == L"phrase") next.chat_phrase = value;
        else if (key == L"initials") next.initials = value != 0.0;
        else if (key == L"pmargin") next.chat_phrase_margin = value;
        else if (key == L"plen") next.chat_phrase_length = static_cast<size_t>(value);
        else return false;
        if (comma == std::wstring::npos) break;
        start = comma + 1;
    }
    g_weights = next;
    return true;
}

bool PinyinEngine::load_english_terms(const std::wstring& path) {
    return english_.load(path);
}

void PinyinEngine::refresh_learning() const {
    learning_.refresh_if_changed();
    if (learning_revision_ != learning_.revision()) {
        const auto selections = learning_.selected_texts();
        context_model_.set_learning(selections);
        continuation_model_.set_learning(selections);
        learning_revision_ = learning_.revision();
    }
}

std::wstring PinyinEngine::full_pinyin(const std::wstring& typed) const {
    if (options_.double_pinyin == DoublePinyinScheme::None) return typed;
    return convert_double_pinyin(typed, options_.double_pinyin).pinyin;
}

std::vector<Candidate> PinyinEngine::lookup(const std::wstring& typed, const std::wstring& preceding_text) const {
    std::tm now{};
    const std::time_t clock = std::time(nullptr);
    localtime_s(&now, &clock);
    if (options_.double_pinyin == DoublePinyinScheme::None) {
        if (is_tool_input(typed)) {
            std::vector<Candidate> results;
            for (const auto& text : tool_results(typed.substr(1), now)) results.push_back(Candidate{text, kFallbackScore + 1, 0});
            if (results.empty()) results.push_back(Candidate{typed, kFallbackScore, 0});
            if (results.size() > kMaxCandidateCount) results.resize(kMaxCandidateCount);
            return results;
        }
        auto candidates = lookup_pinyin(typed, preceding_text, options_);
        const auto key = normalize_input(typed);
        place_phrases(key, candidates);
        place_emoji(key, candidates);
        // sj, rq, xq: the time, date or weekday second, after the usual word.
        const auto shortcuts = time_shortcuts(key, now);
        for (size_t index = 0; index < shortcuts.size(); ++index) {
            const size_t at = std::min(index + 1, candidates.size());
            candidates.insert(candidates.begin() + static_cast<std::ptrdiff_t>(at), Candidate{shortcuts[index], kFallbackScore + 1, 0});
        }
        if (candidates.size() > kMaxCandidateCount) candidates.resize(kMaxCandidateCount);
        return candidates;
    }
    // Double pinyin: each key pair is one syllable. Letters mean other things
    // there, so English terms and keyboard corrections stay off; fuzzy pinyin
    // applies to the converted syllables.
    const auto conversion = convert_double_pinyin(typed, options_.double_pinyin);
    auto options = options_;
    options.mixed_english = false;
    options.typo_correction = false;
    auto candidates = lookup_pinyin(conversion.pinyin, preceding_text, options);
    for (auto& candidate : candidates) {
        if (candidate.consumed_pinyin)
            candidate.consumed_pinyin = conversion.raw_end[std::min(candidate.consumed_pinyin, conversion.raw_end.size() - 1)];
    }
    place_phrases(normalize_input(conversion.pinyin), candidates);
    place_emoji(normalize_input(conversion.pinyin), candidates);
    return candidates;
}

struct PinyinEngine::DecodeCache {
    std::wstring pinyin;
    std::vector<size_t> boundaries;
    std::wstring context;
    unsigned fuzzy = 0;
    size_t learning_revision = 0;
    size_t epoch = 0;
    std::vector<std::vector<SegmentPath>> lattice;
};

std::vector<Candidate> PinyinEngine::lookup_pinyin(const std::wstring& pinyin, const std::wstring& preceding_text,
                                                   const EngineInputOptions& options) const {
    const auto context = ContinuationModel::context_suffix(preceding_text);
    const auto parsed = parse_input(pinyin);
    const auto& normalized = parsed.pinyin;
    if (normalized.empty()) {
        return {};
    }
    if (normalized.size() > kMaxInputLength) return {Candidate{normalized, kFallbackScore, 0}};
    refresh_learning();

    const DomainLexicon* scenes = domains_.empty() ? nullptr : &domains_;
    DecodeOptions decoding{options.fuzzy, nullptr, scenes, word_model_.empty() ? nullptr : &word_model_};
    DecodeReuse reuse;
    if (decode_cache_ && decode_cache_->context == context && decode_cache_->fuzzy == options.fuzzy &&
        decode_cache_->learning_revision == learning_.revision() && decode_cache_->epoch == decode_epoch_) {
        size_t common = 0;
        const auto& previous = decode_cache_->pinyin;
        while (common < previous.size() && common < normalized.size() && previous[common] == normalized[common]) ++common;
        // A syllable spans at most six letters: positions further back keep
        // their split penalties and their words.
        size_t stable = common > kLongestSyllable ? common - kLongestSyllable : 0;
        const auto boundaries_before = [&](const std::vector<size_t>& all) {
            std::vector<size_t> kept;
            for (size_t boundary : all) if (boundary <= stable + kLongestSyllable) kept.push_back(boundary);
            return kept;
        };
        if (boundaries_before(decode_cache_->boundaries) != boundaries_before(parsed.boundaries)) stable = 0;
        if (stable > 0) reuse = DecodeReuse{&decode_cache_->lattice, stable};
    }
    auto dp = decode(dictionary_, ranker_, learning_, context_model_, parsed, context, decoding, reuse);
    if (!decode_cache_) decode_cache_ = std::make_shared<DecodeCache>();
    *decode_cache_ = DecodeCache{normalized, parsed.boundaries, context, options.fuzzy, learning_.revision(), decode_epoch_, dp};
    // Chinese first; English terms join only when Chinese cannot read the
    // whole input (Python, GitHub and most English words are not pinyin).
    bool mixed = false;
    if (dp.back().empty() && options.mixed_english && english_.occurs_in(normalized)) {
        decoding.english = &english_;
        auto with_english = decode(dictionary_, ranker_, learning_, context_model_, parsed, context, decoding);
        if (!with_english.back().empty()) {
            dp = std::move(with_english);
            mixed = true;
        } else {
            decoding.english = nullptr;
        }
    }
    bool initials = false;
    if (weights().initials && dp.back().empty() && !mixed && parsed.boundaries.empty() && looks_like_initials(normalized)) {
        auto read = decode_initials(dictionary_, context_model_, normalized, context, decoding,
                                    [this](const std::wstring& initial) -> const std::vector<Candidate>& { return characters_for_initial(initial); });
        if (!read.back().empty()) {
            dp = std::move(read);
            initials = true;
        }
    }
    // The furthest point the input reads to, for corrections inside a sentence.
    size_t reach = 0;
    for (size_t offset = 0; offset < dp.size(); ++offset) if (!dp[offset].empty()) reach = offset;
    std::vector<Candidate> candidates;
    for (const auto& path : dp.back()) candidates.push_back(candidate_from_path(path, context_model_));
    // A standalone short word keeps its familiar frequency order. An explicit
    // personal phrase at the recommended common-word frequency also preserves
    // a complete key match, even if sentence decoding pruned that whole phrase.
    // Lower-frequency personal phrases and system sentences keep their normal
    // contextual ranking.
    for (auto candidate : words_for(normalized)) {
        const bool standalone_word = context.empty() && candidate.text.size() <= 4;
        const bool preferred_personal_phrase = candidate.is_user_lexicon_entry && candidate.text.size() > 4 &&
                                               candidate.base_score >= kPreferredUserLexiconFrequency;
        if ((!standalone_word && !preferred_personal_phrase) ||
            !dictionary_.reading_matches_boundaries(normalized, candidate.text, parsed.boundaries)) continue;
        candidate.exact_match_bonus = kExactMatchBonus;
        candidate.complete_segmentation_bonus = kCompleteSegmentationBonus;
        // Lexicon frequencies are coarse and often tied (请 and 晴 share one
        // value). A tenth of the clause-start language model score breaks such
        // near ties without overriding deliberate frequency priorities.
        if (standalone_word && !candidate.is_user_lexicon_entry)
            candidate.base_score += static_cast<int>(weights().standalone_language_model * context_model_.extend_score(L"", candidate.text));
        candidates.push_back(std::move(candidate));
    }

    // Complete an unfinished final syllable in its sentence context. This also
    // preserves a fully spelled sentence: xi is not extended to xiao and ji
    // is not extended to jian when the complete input already has a reading.
    for (size_t offset = normalized.size() > 12 ? normalized.size() - 12 : 0;
         dp.back().empty() && offset < normalized.size(); ++offset) {
        if (dp[offset].empty()) continue;
        const auto suffix = normalized.substr(offset);
        auto completions = ranker_.rank(suffix, dictionary_.lookup_completion(suffix, true), &learning_);
        completions.erase(std::remove_if(completions.begin(), completions.end(), [&](const auto& word) {
            return !can_compose_token(word, offset == 0);
        }), completions.end());
        // An English term being typed: pyt -> Python, from three letters on
        // (shorter tails are too often a pinyin slip).
        if (options.mixed_english && !english_.empty() && suffix.size() >= 3) {
            for (const auto& term : english_.completions(suffix)) {
                for (const auto& path : dp[offset]) {
                    SegmentPath completed{path.score + english_prior(*term, suffix.size()) - 600, path.segment_count + 1,
                                          join_text(path.text, term->text, path.english_end, true), true};
                    candidates.push_back(candidate_from_path(completed, context_model_));
                }
            }
        }
        if (completions.empty()) continue;
        for (const auto& path : dp[offset]) {
            for (size_t index = 0; index < std::min<size_t>(12, completions.size()); ++index) {
                const auto& word = completions[index];
                SegmentPath completed{path.score + static_cast<int>(weights().lexical * lexical_score(word)) + word.user_frequency / 3 +
                                      static_cast<int>(weights().language_model *
                                          context_model_.extend_score(transition_history(context, path.text), word.text)) - 600 +
                                      word_bonus(decoding, path, word.text, true),
                                      path.segment_count + 1, join_text(path.text, word.text, path.english_end, false)};
                candidates.push_back(candidate_from_path(completed, context_model_));
            }
        }
        if (candidates.size() > 256) candidates = deduplicate_and_limit(ranker_.rank(normalized, std::move(candidates), &learning_));
    }

    if (candidates.empty()) candidates = dictionary_.lookup_abbreviation(normalized);
    if (candidates.empty() && options.typo_correction) {
        // Nothing reads: a keystroke slip. Whole words first (nihso -> 你好),
        // then sentences re-read with the slip near where reading stopped.
        candidates = correction_candidates(dictionary_, normalized, true, 900);
        if (candidates.empty() && normalized.size() >= 6) {
            const size_t first = reach > 0 ? reach - 1 : 0;
            size_t decoded = 0;
            for (const auto& correction : spelling_corrections_near(normalized, first, std::min(normalized.size(), reach + 5), false)) {
                if (decoded >= kSentenceCorrectionDecodes) break;
                if (!EnglishTerms::reads_as_pinyin(correction.pinyin)) continue;
                ++decoded;
                const auto corrected = parse_input(correction.pinyin);
                auto corrected_dp = decode(dictionary_, ranker_, learning_, context_model_, corrected, context, {options.fuzzy, nullptr, scenes, decoding.words});
                for (size_t index = 0; index < std::min<size_t>(2, corrected_dp.back().size()); ++index) {
                    auto candidate = candidate_from_path(corrected_dp.back()[index], context_model_);
                    candidate.segmentation_penalty = correction.cost;
                    candidate.correction = true;
                    candidates.push_back(std::move(candidate));
                }
            }
        }
    }
    const wchar_t before = !context.empty() && context.back() >= 0x3400 && context.back() <= 0x9FFF ? context.back() : 0;
    auto ranked = deduplicate_and_limit(ranker_.rank(normalized, std::move(candidates), &learning_,
                                                     before ? before : (context.empty() ? 0 : L' ')));
    if (!mixed && !ranked.empty()) {
        const auto exact = words_for(normalized);
        const bool first_is_word = std::any_of(exact.begin(), exact.end(), [&](const auto& word) { return word.text == ranked.front().text; });
        rescore_with_neural(neural_model_, ranked, context, first_is_word);
        if (context.empty()) promote_chat_phrase(ranked, exact, phrase_counts_, [&](const std::wstring& key) { return words_for(key); }, normalized, initials);
    }

    // Keep complete long-word alternatives beside sentence decoding. A rescue
    // must not evict another complete word already in the last two slots.
    // Preserve contextual leading choices and a slot for a shorter prefix.
    auto complete_words = words_for(normalized);
    complete_words.erase(std::remove_if(complete_words.begin(), complete_words.end(), [&](const auto& word) {
        return word.text.size() <= 4 || word.text.size() > 16 || word.base_score < kPreferredUserLexiconFrequency ||
               !dictionary_.reading_matches_boundaries(normalized, word.text, parsed.boundaries);
    }), complete_words.end());
    complete_words = ranker_.rank(normalized, std::move(complete_words), &learning_);
    auto missing_word = std::find_if(complete_words.begin(), complete_words.end(), [&](const auto& word) {
        return std::none_of(ranked.begin(), ranked.end(), [&](const auto& item) { return item.text == word.text; });
    });
    if (missing_word != complete_words.end()) {
        std::vector<Candidate> rescued;
        // Retain existing whole words in their contextual order, including
        // their original score and commit metadata, before adding a missing one.
        for (size_t index = std::min(ranked.size(), kMaxCandidateCount - 3); index < ranked.size(); ++index) {
            const auto& item = ranked[index];
            if (std::any_of(complete_words.begin(), complete_words.end(), [&](const auto& word) {
                    return item.text == word.text;
                })) {
                rescued.push_back(item);
                break;
            }
        }
        rescued.push_back(*missing_word);
        if (rescued.size() < 2) {
            const auto second = std::find_if(std::next(missing_word), complete_words.end(), [&](const auto& word) {
                return std::none_of(ranked.begin(), ranked.end(), [&](const auto& item) { return item.text == word.text; });
            });
            if (second != complete_words.end()) rescued.push_back(*second);
        }
        const size_t leading_slots = kMaxCandidateCount - 1 - rescued.size();
        if (ranked.size() > leading_slots) ranked.resize(leading_slots);
        for (auto& word : rescued) {
            if (std::none_of(ranked.begin(), ranked.end(), [&](const auto& item) { return item.text == word.text; }))
                ranked.push_back(std::move(word));
        }
    }

    // With text before the caret, an input that spells whole words (shizi)
    // lists them right after the leading choice, ahead of loose character
    // pairs (是自, 是紫), as it does without context. The leading choice keeps
    // its contextual rank, and so does anything the user has chosen before.
    if (!context.empty() && !mixed && ranked.size() > 1) {
        auto whole = words_for(normalized);
        whole.erase(std::remove_if(whole.begin(), whole.end(), [&](const auto& word) {
            const size_t characters = character_count(word.text);
            return characters < 2 || characters > 4 ||
                   !dictionary_.reading_matches_boundaries(normalized, word.text, parsed.boundaries);
        }), whole.end());
        if (!whole.empty()) {
            whole = ranker_.rank(normalized, std::move(whole), &learning_);
            std::unordered_set<std::wstring> word_texts;
            for (const auto& word : whole) word_texts.insert(word.text);
            const auto stays = [&](const Candidate& item) {
                return word_texts.count(item.text) > 0 || item.user_frequency > 0 || item.is_user_lexicon_entry;
            };
            std::vector<Candidate> reordered{ranked.front()};
            std::unordered_set<std::wstring> listed{ranked.front().text};
            for (size_t index = 1; index < ranked.size(); ++index)
                if (stays(ranked[index]) && listed.insert(ranked[index].text).second) reordered.push_back(ranked[index]);
            for (auto& word : whole)
                if (reordered.size() < kMaxCandidateCount && listed.insert(word.text).second) reordered.push_back(std::move(word));
            for (size_t index = 1; index < ranked.size() && reordered.size() < kMaxCandidateCount; ++index)
                if (listed.insert(ranked[index].text).second) reordered.push_back(ranked[index]);
            if (reordered.size() > kMaxCandidateCount) reordered.resize(kMaxCandidateCount);
            ranked = std::move(reordered);
        }
    }

    std::vector<Candidate> prefixes;
    if (ranked.empty()) {
        // A missing dictionary item must not discard an otherwise useful
        // converted prefix. The user can commit it and continue with the tail.
        for (size_t end = normalized.size() - 1; end > 0; --end) {
            if (dp[end].empty()) continue;
            for (const auto& path : dp[end]) {
                auto candidate = candidate_from_path(path, context_model_);
                candidate.consumed_pinyin = raw_prefix_end(parsed, pinyin, end);
                prefixes.push_back(std::move(candidate));
            }
            prefixes = ranker_.rank(normalized.substr(0, end), std::move(prefixes), &learning_);
            break;
        }
    } else {
        // Reserve a choice for a first word/phrase even while a full sentence
        // is available, enabling the normal select-prefix-and-continue workflow.
        for (const auto& key : dictionary_.matching_pinyins_at(normalized, 0)) {
            if (key.size() >= normalized.size() || key.size() > 12) continue;
            auto words = ranker_.rank(key, words_for(key), &learning_);
            std::vector<size_t> boundaries;
            for (size_t boundary : parsed.boundaries) {
                if (boundary < key.size()) boundaries.push_back(boundary);
            }
            for (auto& word : words) {
                if (word.text.size() > 4 || !dictionary_.reading_matches_boundaries(key, word.text, boundaries)) continue;
                word.consumed_pinyin = raw_prefix_end(parsed, pinyin, key.size());
                prefixes.push_back(std::move(word));
            }
            if (!prefixes.empty()) break;
        }
    }
    if (!prefixes.empty()) {
        if (ranked.size() >= kMaxCandidateCount) {
            // An eligible complete word may already be ranked ninth. The
            // earlier retention pass then leaves it in place; protect it here
            // before its slot is reassigned to a shorter prefix. Preserve its
            // original score/commit metadata and the first seven choices.
            if (std::any_of(complete_words.begin(), complete_words.end(), [&](const auto& word) {
                    return word.text == ranked[kMaxCandidateCount - 1].text;
                })) {
                ranked[kMaxCandidateCount - 2] = ranked[kMaxCandidateCount - 1];
            }
            ranked.resize(kMaxCandidateCount - 1);
        }
        std::unordered_set<std::wstring> seen;
        for (const auto& candidate : ranked) seen.insert(candidate.text);
        for (auto& candidate : prefixes) {
            if (!seen.insert(candidate.text).second) continue;
            ranked.push_back(std::move(candidate));
            if (ranked.size() >= kMaxCandidateCount) break;
        }
    }
    // The whole input is an English term or abbreviation (ok, ppt, python):
    // first when no Chinese reads it, otherwise after the leading readings.
    if (options.mixed_english) {
        if (const auto* term = english_.find(normalized); term && parsed.boundaries.empty() &&
            std::none_of(ranked.begin(), ranked.end(), [&](const auto& item) { return item.text == term->text; })) {
            Candidate english{term->text, kFallbackScore, 0};
            const bool chinese_reads = !dp.back().empty() && !mixed;
            const size_t slot = !chinese_reads ? 0 : term->pinyin_like ? std::min<size_t>(2, ranked.size()) : 1;
            if (ranked.size() >= kMaxCandidateCount) ranked.pop_back();
            ranked.insert(ranked.begin() + static_cast<std::ptrdiff_t>(std::min(slot, ranked.size())), std::move(english));
        }
    }
    // A likely slip beside valid readings: zhida reads as 直达, but 知道 is
    // far more common one keystroke away. Offered second, never first.
    const int typed_frequency = top_frequency(words_for(normalized));
    if (options.typo_correction && !ranked.empty() && normalized.size() >= 4 && normalized.size() <= 12 &&
        parsed.boundaries.empty() && !mixed && typed_frequency < kCommonTypedFrequency) {
        const Candidate* best = nullptr;
        int best_margin = std::numeric_limits<int>::min();
        std::vector<Candidate> pool;
        for (const auto& correction : spelling_corrections(normalized, false)) {
            if (correction.cost > kCorrectionMaxCost) break;
            if (!dictionary_.has_key(correction.pinyin)) continue;
            auto matches = dictionary_.lookup(correction.pinyin);
            if (matches.empty()) continue;
            const auto top = std::max_element(matches.begin(), matches.end(), [](const auto& left, const auto& right) {
                return left.base_score < right.base_score;
            });
            const size_t characters = character_count(top->text);
            if (characters < 2 || characters > 4 ||
                top->base_score < (typed_frequency ? kStrongCorrectionFrequency : kUnreadCorrectionFrequency)) continue;
            const int margin = top->base_score - correction.cost - typed_frequency;
            if ((typed_frequency && margin < kStrongCorrectionMargin) || margin <= best_margin) continue;
            best_margin = margin;
            pool.clear();
            pool.push_back(*top);
            pool.back().correction = true;
            pool.back().segmentation_penalty = correction.cost;
            best = &pool.back();
        }
        if (best && std::none_of(ranked.begin(), ranked.end(), [&](const auto& item) { return item.text == best->text; })) {
            if (ranked.size() >= kMaxCandidateCount) ranked.pop_back();
            const size_t slot = !typed_frequency || best_margin >= kLeadingCorrectionMargin ? 1 : 4;
            ranked.insert(ranked.begin() + static_cast<std::ptrdiff_t>(std::min(slot, ranked.size())), *best);
        }
    }
    if (ranked.empty()) ranked.push_back(Candidate{normalized, kFallbackScore, 0});
    return ranked;
}

std::wstring PinyinEngine::display_pinyin(const std::wstring& typed) {
    const auto segment = [&](const std::wstring& chunk) {
        const size_t n = chunk.size();
        if (n == 0 || n > 64) return chunk;
        // Fewest pieces; on a tie the longest first syllable (fang'an, xian).
        std::vector<double> cost(n + 1, 1e9);
        std::vector<size_t> next(n + 1, 0);
        cost[n] = 0;
        for (size_t start = n; start-- > 0;) {
            for (size_t length = std::min<size_t>(6, n - start); length > 0; --length) {
                const auto piece = chunk.substr(start, length);
                double piece_cost = 0;
                if (Dictionary::is_syllable(piece)) piece_cost = 1;
                else if (start + length == n && Dictionary::is_syllable_prefix(piece)) piece_cost = 1.4;
                else continue;
                if (piece_cost + cost[start + length] < cost[start]) {
                    cost[start] = piece_cost + cost[start + length];
                    next[start] = start + length;
                }
            }
        }
        if (cost[0] >= 1e9) return chunk;
        std::wstring result;
        for (size_t position = 0; position < n; position = next[position]) {
            if (!result.empty()) result.push_back(L'\'');
            result += chunk.substr(position, next[position] - position);
        }
        return result;
    };
    std::wstring result;
    std::wstring chunk;
    for (wchar_t ch : typed) {
        if (ch == L'\'') {
            result += segment(chunk);
            result.push_back(L'\'');
            chunk.clear();
        } else {
            chunk.push_back(ch >= L'A' && ch <= L'Z' ? static_cast<wchar_t>(ch - L'A' + L'a') : ch);
        }
    }
    return result + segment(chunk);
}

std::wstring PinyinEngine::display_input(const std::wstring& typed) const {
    if (options_.double_pinyin != DoublePinyinScheme::None) return convert_double_pinyin(typed, options_.double_pinyin).pinyin;
    if (is_tool_input(typed) || (typed == L"v" || typed == L"V")) return typed;
    const std::wstring plain = display_pinyin(typed);
    if (!options_.mixed_english || english_.empty() || plain.find(L'\'') != std::wstring::npos ||
        std::any_of(typed.begin(), typed.end(), [](wchar_t ch) { return ch >= L'A' && ch <= L'Z'; })) return plain;
    // Not pinyin as a whole: set English terms apart (wo'yong python xie'dai'ma).
    const auto normalized = normalize_input(typed);
    const size_t n = normalized.size();
    if (n == 0 || n > 64) return plain;
    std::vector<double> cost(n + 1, 1e9);
    std::vector<size_t> next(n + 1, 0);
    std::vector<bool> english(n + 1, false);
    cost[n] = 0;
    std::vector<std::pair<size_t, const EnglishTerms::Term*>> terms;
    for (size_t start = n; start-- > 0;) {
        for (size_t length = std::min<size_t>(6, n - start); length > 0; --length) {
            const auto piece = normalized.substr(start, length);
            double piece_cost = 0;
            if (Dictionary::is_syllable(piece)) piece_cost = 1;
            else if (start + length == n && Dictionary::is_syllable_prefix(piece)) piece_cost = 1.4;
            else continue;
            if (piece_cost + cost[start + length] < cost[start]) {
                cost[start] = piece_cost + cost[start + length];
                next[start] = start + length;
                english[start] = false;
            }
        }
        english_.matches_at(normalized, start, terms);
        for (const auto& [length, term] : terms) {
            if (1.0 + cost[start + length] < cost[start]) {
                cost[start] = 1.0 + cost[start + length];
                next[start] = start + length;
                english[start] = true;
            }
        }
    }
    if (cost[0] >= 1e9) return plain;
    std::wstring result;
    bool previous_english = false;
    for (size_t position = 0; position < n; position = next[position]) {
        if (!result.empty()) result.push_back(english[position] || previous_english ? L' ' : L'\'');
        result += normalized.substr(position, next[position] - position);
        previous_english = english[position];
    }
    return result;
}

bool PinyinEngine::warm_up_step() const {
    return dictionary_.warm_prediction_index_step();
}

UserLexiconRefreshResult PinyinEngine::refresh_user_lexicon_if_needed(bool composition_active) {
    auto result = dictionary_.refresh_user_lexicon_if_changed(composition_active);
    if (result.reloaded) rebuild_context_model(context_path_);  // also discards the cached lattice
    return result;
}

bool PinyinEngine::learn_phrase(const std::wstring& typed, const std::wstring& phrase, const std::wstring& preceding_text) {
    const auto offered = lookup(typed, preceding_text);
    if (!offered.empty() && offered.front().text == phrase) return false;
    learn(typed, phrase);
    return true;
}

size_t PinyinEngine::forget(const std::wstring& text) {
    if (text.empty()) return 0;
    refresh_learning();
    size_t forgotten = learning_.forget(text);
    if (!forgotten) {
        for (const auto& entry : learning_.entries())
            if (entry.word.size() >= 2 && entry.word.size() * 2 > text.size() && text.find(entry.word) != std::wstring::npos)
                forgotten += learning_.forget(entry.word, entry.pinyin);
    }
    refresh_learning();
    return forgotten;
}

void PinyinEngine::learn(const std::wstring& typed, const std::wstring& selected_word, bool offered_first,
                         const std::wstring& preceding_text) {
    const auto normalized = normalize_input(full_pinyin(typed));
    if (normalized.empty() || normalized == selected_word) return;
    const wchar_t before = !preceding_text.empty() && preceding_text.back() >= 0x3400 && preceding_text.back() <= 0x9FFF
                               ? preceding_text.back() : 0;
    const auto in_context = before ? UserLearning::contextual_key(normalized, before) : std::wstring();
    refresh_learning();
    const long long now = static_cast<long long>(std::time(nullptr));
    // The first choice only refreshes what was learned before.
    if (!offered_first || learning_.faded_steps(normalized, selected_word, now) >= 0) learn(typed, selected_word);
    if (before && (!offered_first || learning_.faded_steps(in_context, selected_word, now) >= 0))
        learning_.record_selection(in_context, selected_word);
}

void PinyinEngine::learn(const std::wstring& typed, const std::wstring& selected_word) {
    ++decode_epoch_;
    // Learning is keyed by full pinyin, whatever the typing scheme.
    const auto normalized = normalize_input(full_pinyin(typed));
    if (normalized.empty() || normalized == selected_word) return;
    if (!learning_.record_selection(normalized, selected_word)) ranker_.increment_frequency(normalized, selected_word);
}

}  // namespace localpinyin
