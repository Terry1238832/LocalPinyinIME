// 本地英文's speller against the shipped word list. Platform independent:
// run as test_english_speller <resources/dictionary>.
#include "../src/engine/english_speller.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>

#define REQUIRE_TRUE(expr) do { if (!(expr)) { std::cerr << "Requirement failed: " << #expr << "\n"; return 1; } } while (false)
#define REQUIRE_EQ(left, right) do { if (!((left) == (right))) { std::cerr << "Requirement failed: " << #left << " == " << #right \
    << " (" << (left) << ")\n"; return 1; } } while (false)

int main(int argc, char** argv) {
    using localpinyin::EnglishSpeller;
    using Kind = EnglishSpeller::Candidate::Kind;
    const std::filesystem::path dictionary = argc > 1 ? std::filesystem::path(argv[1])
                                                      : std::filesystem::path(argv[0]).parent_path() / "dictionary";
    EnglishSpeller speller;
    REQUIRE_TRUE(speller.load(dictionary / "english_words.tsv", dictionary / "english_terms.tsv"));
    REQUIRE_TRUE(speller.size() > 100000);

    const auto first = [&](const std::string& typed) { return speller.lookup(typed).candidates.front().text; };
    const auto has = [&](const std::string& typed, const std::string& text) {
        const auto result = speller.lookup(typed);
        return std::any_of(result.candidates.begin(), result.candidates.end(),
                           [&](const EnglishSpeller::Candidate& candidate) { return candidate.text == text; });
    };

    // A word as typed stays, first and unchanged; longer words follow.
    auto result = speller.lookup("form");
    REQUIRE_TRUE(!result.corrects);
    REQUIRE_EQ(result.candidates.front().text, std::string("form"));
    REQUIRE_TRUE(result.candidates.front().kind == Kind::AsTyped);
    REQUIRE_TRUE(std::any_of(result.candidates.begin(), result.candidates.end(),
                             [](const auto& candidate) { return candidate.kind == Kind::Completion; }));
    REQUIRE_EQ(first("its"), std::string("its"));
    REQUIRE_EQ(first("well"), std::string("well"));
    REQUIRE_EQ(first("colour"), std::string("colour"));  // British spellings are accepted
    REQUIRE_TRUE(!has("colo", "colour"));                 // but never offered

    // Slips and misspellings: Space corrects, and what was typed comes second.
    result = speller.lookup("teh");
    REQUIRE_TRUE(result.corrects);
    REQUIRE_EQ(result.candidates[0].text, std::string("the"));
    REQUIRE_TRUE(result.candidates[0].kind == Kind::Correction);
    REQUIRE_EQ(result.candidates[1].text, std::string("teh"));
    REQUIRE_TRUE(result.candidates[1].kind == Kind::AsTyped);
    REQUIRE_EQ(first("recieve"), std::string("receive"));
    REQUIRE_EQ(first("seperate"), std::string("separate"));
    REQUIRE_EQ(first("definately"), std::string("definitely"));
    REQUIRE_EQ(first("becuase"), std::string("because"));
    REQUIRE_EQ(first("untill"), std::string("until"));
    REQUIRE_EQ(first("occured"), std::string("occurred"));

    // Missing apostrophes, including words that exist without one when the
    // contraction is far more common (cant), but not otherwise (ill, its).
    REQUIRE_EQ(first("dont"), std::string("don't"));
    REQUIRE_EQ(first("im"), std::string("I'm"));
    REQUIRE_EQ(first("cant"), std::string("can't"));
    REQUIRE_EQ(first("ill"), std::string("ill"));

    // Capitals: I, names and terms keep theirs; typed capitals carry over.
    REQUIRE_EQ(first("i"), std::string("I"));
    REQUIRE_EQ(first("english"), std::string("English"));
    REQUIRE_EQ(first("javascript"), std::string("JavaScript"));
    REQUIRE_EQ(first("Teh"), std::string("The"));
    REQUIRE_EQ(first("Hell"), std::string("Hell"));
    REQUIRE_TRUE(has("Hell", "Hello"));
    REQUIRE_TRUE(has("HELL", "HELLO"));
    // Shouted letters and lowercase abbreviations are left as typed.
    REQUIRE_EQ(first("NASA"), std::string("NASA"));
    REQUIRE_EQ(first("ok"), std::string("ok"));
    REQUIRE_EQ(first("nad"), std::string("and"));

    // Too short to correct, or likely a name: left as typed.
    REQUIRE_EQ(first("mp"), std::string("mp"));
    REQUIRE_EQ(first("qianwen"), std::string("qianwen"));
    REQUIRE_EQ(first("Pantanal"), std::string("Pantanal"));
    // A possessive is its word.
    REQUIRE_TRUE(speller.knows("Pantanal's") == speller.knows("Pantanal"));
    REQUIRE_EQ(first("dog's"), std::string("dog's"));

    // Completions: the commonest longer words.
    REQUIRE_TRUE(has("beautif", "beautiful"));
    REQUIRE_TRUE(has("pyth", "Python") || has("pyth", "python"));

    // Learned words are never corrected, come back after reopening, and can be
    // forgotten.
    const auto root = std::filesystem::temp_directory_path() / "localpinyin_english_speller_test";
    std::filesystem::remove_all(root);
    const auto user_path = root / "english_user_words.txt";
    speller.open_user_words(user_path);
    REQUIRE_EQ(first("Qwen"), std::string("Qwen"));
    REQUIRE_TRUE(first("qwenx") != "qwenx" || !speller.lookup("qwenx").corrects);
    REQUIRE_TRUE(speller.learn("Tencent"));
    REQUIRE_TRUE(!speller.learn("Tencent"));
    REQUIRE_TRUE(!speller.learn("not a word"));
    REQUIRE_TRUE(speller.knows("tencent"));
    REQUIRE_TRUE(has("tenc", "Tencent"));
    EnglishSpeller reopened;
    REQUIRE_TRUE(reopened.load(dictionary / "english_words.tsv"));
    reopened.open_user_words(user_path);
    REQUIRE_TRUE(reopened.knows("Tencent"));
    REQUIRE_TRUE(reopened.forget("tencent"));
    REQUIRE_TRUE(!reopened.knows("Tencent"));
    speller.refresh_user_words_if_changed();
    REQUIRE_TRUE(!speller.knows("Tencent"));
    std::filesystem::remove_all(root);

    std::cout << "test_english_speller passed\n";
    return 0;
}
