// How well 本地英文 puts in what was meant when Space is pressed.
//
//   LocalPinyinEnglishEval <dictionary dir> <cases.tsv> <held-out sentences> [--list]
//
// cases.tsv: typed<TAB>what Space should put in (tests/data/english_misspellings.tsv).
// The held-out sentences (never counted in english_words.tsv) give three more
// measures: one keyboard slip typed into each known word of three or more
// letters (a neighbor key, two keys swapped, one left out, one doubled or a
// neighbor pressed too; slips that make another word are skipped), the words
// as written that are not in the word list (names, mostly), and the known
// words as written, which Space should keep apart from capitals and
// apostrophes. Platform independent, like the speller.
#include "../engine/english_speller.h"
#include "../engine/typo_correction.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

using localpinyin::EnglishSpeller;

bool letter(char ch) { return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'); }
bool lower_word(const std::string& word) {
    return std::all_of(word.begin(), word.end(), [](char ch) { return ch >= 'a' && ch <= 'z'; });
}

// A fixed generator, so every platform types the same slips.
struct Random {
    std::uint64_t state = 0x9E3779B97F4A7C15ull;
    std::uint32_t next(std::uint32_t bound) {
        state = state * 6364136223846793005ull + 1442695040888963407ull;
        return static_cast<std::uint32_t>((state >> 33) % bound);
    }
};

std::string slip(const std::string& word, Random& random) {
    std::string result = word;
    const auto neighbor_of = [&](char key) {
        std::string keys;
        for (char other = 'a'; other <= 'z'; ++other)
            if (localpinyin::keyboard_neighbors(static_cast<wchar_t>(key), static_cast<wchar_t>(other))) keys.push_back(other);
        return keys.empty() ? key : keys[random.next(static_cast<std::uint32_t>(keys.size()))];
    };
    const size_t at = random.next(static_cast<std::uint32_t>(word.size()));
    switch (random.next(5)) {
        case 0: result[at] = neighbor_of(word[at]); break;
        case 1:
            if (at + 1 < word.size()) std::swap(result[at], result[at + 1]);
            else std::swap(result[at - 1], result[at]);
            break;
        case 2: result.erase(at, 1); break;
        case 3: result.insert(at, 1, word[at]); break;
        default: result.insert(at + 1, 1, neighbor_of(word[at])); break;
    }
    return result;
}

double percent(size_t part, size_t whole) { return whole ? 100.0 * static_cast<double>(part) / static_cast<double>(whole) : 0.0; }

}  // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "usage: LocalPinyinEnglishEval <dictionary dir> <cases.tsv> <held-out sentences> [--list]\n";
        return 2;
    }
    const std::filesystem::path dictionary = argv[1];
    const bool list = argc > 4 && std::string(argv[4]) == "--list";
    EnglishSpeller speller;
    if (!speller.load(dictionary / "english_words.tsv", dictionary / "english_terms.tsv")) {
        std::cerr << "cannot load " << (dictionary / "english_words.tsv").string() << "\n";
        return 2;
    }
    double slowest_ms = 0;
    double total_ms = 0;
    size_t lookups = 0;
    const auto head = [&](const std::string& typed) {
        const auto start = std::chrono::steady_clock::now();
        const auto result = speller.lookup(typed);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        slowest_ms = std::max(slowest_ms, ms);
        total_ms += ms;
        ++lookups;
        return result;
    };

    size_t cases = 0, cases_right = 0;
    std::ifstream case_file(argv[2], std::ios::binary);
    std::string line;
    while (std::getline(case_file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        const size_t tab = line.find('\t');
        if (tab == std::string::npos) continue;
        const std::string typed = line.substr(0, tab);
        const std::string expected = line.substr(tab + 1);
        const auto result = head(typed);
        const std::string got = result.candidates.empty() ? typed : result.candidates.front().text;
        ++cases;
        if (got == expected) ++cases_right;
        else if (list) std::cout << "case " << typed << ": got " << got << ", expected " << expected << "\n";
    }

    size_t slips = 0, fixed = 0, miscorrected = 0, offered = 0;
    size_t unknown = 0, unknown_changed = 0;
    size_t known = 0, known_changed = 0;
    Random random;
    std::ifstream sentences(argv[3], std::ios::binary);
    while (std::getline(sentences, line)) {
        if (line.empty() || line[0] == '#') continue;
        size_t position = 0;
        size_t index = 0;
        while (position < line.size()) {
            while (position < line.size() && !letter(line[position])) ++position;
            size_t end = position;
            while (end < line.size() && (letter(line[end]) ||
                   (line[end] == '\'' && end + 1 < line.size() && letter(line[end + 1])))) ++end;
            if (end == position) break;
            const std::string word = line.substr(position, end - position);
            position = end;
            const bool first = index++ == 0;
            const auto as_written = head(word);
            const bool changes = as_written.corrects;
            if (!speller.knows(word)) {
                ++unknown;
                if (changes) {
                    ++unknown_changed;
                    if (list) std::cout << "unknown " << word << " -> " << as_written.candidates.front().text << "\n";
                }
                continue;
            }
            ++known;
            if (changes) {
                ++known_changed;
                if (list && !first) std::cout << "known " << word << " -> " << as_written.candidates.front().text << "\n";
            }
            if (word.size() < 3 || !lower_word(word)) continue;
            const std::string typed = slip(word, random);
            if (speller.knows(typed)) continue;
            ++slips;
            const auto result = head(typed);
            const std::string got = result.candidates.front().text;
            if (result.corrects && got == word) ++fixed;
            else if (result.corrects) {
                ++miscorrected;
                if (list) std::cout << "slip " << typed << " (" << word << ") -> " << got << "\n";
            }
            if (std::any_of(result.candidates.begin(), result.candidates.end(),
                            [&](const EnglishSpeller::Candidate& candidate) { return candidate.text == word; }))
                ++offered;
        }
    }

    std::printf("cases: %zu/%zu right (%.1f%%)\n", cases_right, cases, percent(cases_right, cases));
    std::printf("slips: %zu; Space fixes %.1f%%, puts in another word %.1f%%, leaves %.1f%%; meant word among candidates %.1f%%\n",
                slips, percent(fixed, slips), percent(miscorrected, slips),
                percent(slips - fixed - miscorrected, slips), percent(offered, slips));
    std::printf("words not in the list: %zu; Space changes %.2f%%\n", unknown, percent(unknown_changed, unknown));
    std::printf("known words: %zu; Space changes %.2f%% (capitals and apostrophes only)\n", known, percent(known_changed, known));
    std::printf("lookups: %zu, %.3f ms average, %.2f ms slowest\n", lookups, lookups ? total_ms / static_cast<double>(lookups) : 0.0, slowest_ms);
    return 0;
}
