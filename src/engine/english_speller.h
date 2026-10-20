#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace localpinyin {

// The word being typed in 本地英文: what it is likely meant to be (a
// keyboard slip or misspelling corrected, a missing apostrophe or capital
// put in) and the words it may be the start of.
//
// Platform independent and UTF-8 throughout, so it can go to other systems
// unchanged: words are ASCII letters and apostrophes, and nothing here calls
// the operating system beyond the standard library.
//
// The words come from resources/dictionary/english_words.tsv (written form,
// count, S = offered / V = accepted as typed only), properly written terms
// from english_terms.tsv (JavaScript) and the user's own words.
class EnglishSpeller {
public:
    struct Candidate {
        enum class Kind : std::uint8_t {
            AsTyped,     // exactly what was typed
            Correction,  // a different spelling of it (or its capitals put right)
            Completion   // a longer word starting with it
        };
        std::string text;
        Kind kind = Kind::AsTyped;
    };
    struct Result {
        // The first is what Space puts in.
        std::vector<Candidate> candidates;
        // The first replaces what was typed: Space corrects it.
        bool corrects = false;
    };

    // english_words.tsv, and optionally english_terms.tsv: a term whose key
    // is its own spelling in lowercase (javascript → JavaScript) and is not
    // already a word is added.
    bool load(const std::filesystem::path& words_path, const std::filesystem::path& terms_path = {});
    // The user's own words, one per line; words learned are added to it.
    void open_user_words(const std::filesystem::path& path);
    // Picks up words another program added since the file was read.
    void refresh_user_words_if_changed();
    // Remembers `word` as typed (it is then never corrected). False when it
    // is not a word (letters and apostrophes) or is already known as typed.
    bool learn(std::string_view word);
    // Forgets a learned word. True when there was one.
    bool forget(std::string_view word);

    [[nodiscard]] bool empty() const noexcept { return words_.empty(); }
    [[nodiscard]] size_t size() const noexcept { return words_.size(); }
    // A word as typed, ignoring capitals (a possessive counts as its word).
    [[nodiscard]] bool knows(std::string_view typed) const;
    // Candidates for the letters typed, best first.
    [[nodiscard]] Result lookup(std::string_view typed) const;

private:
    struct Word {
        std::uint32_t offset = 0;  // into keys_
        std::uint8_t length = 0;
        bool offered = true;
        std::uint32_t count = 0;
    };
    struct Scored {
        size_t index = 0;
        int cost = 0;
        int score = 0;
    };
    static constexpr size_t npos = static_cast<size_t>(-1);

    [[nodiscard]] std::string_view key(size_t index) const noexcept {
        return std::string_view(keys_).substr(words_[index].offset, words_[index].length);
    }
    [[nodiscard]] size_t find(std::string_view key) const noexcept;
    [[nodiscard]] std::string form(size_t index) const;
    [[nodiscard]] int score(size_t index, int cost) const noexcept;
    // Words within `budget` edits of `typed` (lowercase), cheapest first.
    [[nodiscard]] std::vector<Scored> corrections(std::string_view typed, int budget) const;
    [[nodiscard]] std::vector<size_t> completions(std::string_view prefix, size_t limit) const;
    // The likeliest word that is `key` with one apostrophe put in (dont →
    // don't), or npos.
    [[nodiscard]] size_t apostrophe_variant(std::string_view key) const;
    void read_user_words();

    std::string keys_;
    std::vector<Word> words_;
    // Written forms that are not simply the key (English, I'm, iPhone).
    std::unordered_map<std::uint32_t, std::string> forms_;
    // Lowercase key → the user's spelling.
    std::unordered_map<std::string, std::string> user_words_;
    std::filesystem::path user_path_;
    std::filesystem::file_time_type user_time_{};
    std::uintmax_t user_size_ = 0;
};

}  // namespace localpinyin
