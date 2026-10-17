#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace localpinyin {

// English words, programming terms and abbreviations that may appear inside
// a pinyin stream ("woyongpythonxiedaima" -> 我用 Python 写代码), from
// resources/dictionary/english_terms.tsv: key, written form, weight.
//
// A term whose key also reads entirely as pinyin (change = chang'e, line =
// li'ne) would take over Chinese input, so it is only offered when the whole
// input is that term, never inside a sentence; so are two-letter keys (ok,
// ai), which hide inside too many syllables.
class EnglishTerms {
public:
    struct Term {
        std::wstring text;
        int weight = 0;
        // The key also reads as pinyin syllables.
        bool pinyin_like = false;
    };

    bool load(const std::wstring& path);
    void clear() noexcept;
    [[nodiscard]] bool empty() const noexcept { return terms_.empty(); }
    [[nodiscard]] const Term* find(const std::wstring& key) const;
    // Terms whose key starts at `offset` of the lowercase input, as (key
    // length, term); pinyin-like terms are left out.
    void matches_at(const std::wstring& input, size_t offset, std::vector<std::pair<size_t, const Term*>>& out) const;
    // Terms being typed: keys that start with `prefix` (pyth -> Python),
    // strongest first, at most three.
    [[nodiscard]] std::vector<const Term*> completions(const std::wstring& prefix) const;
    // Whether any term (for sentences) occurs anywhere in the input.
    [[nodiscard]] bool occurs_in(const std::wstring& input) const;

    // True when `letters` splits completely into legal pinyin syllables.
    [[nodiscard]] static bool reads_as_pinyin(const std::wstring& letters);

private:
    std::unordered_map<std::wstring, Term> terms_;
    size_t max_key_length_ = 0;
};

}  // namespace localpinyin
