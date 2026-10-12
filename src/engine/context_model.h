#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace localpinyin {

class Dictionary;

// A small 1-4gram character language model. Transitions make the score
// independent of dictionary word boundaries and generalize to unseen sentences.
class ContextModel {
public:
    void reset(const Dictionary& dictionary);
    bool load(const std::wstring& path);
    void set_learning(const std::vector<std::pair<std::wstring, int>>& selections);
    [[nodiscard]] int extend_score(const std::wstring& previous, const std::wstring& word) const;
    [[nodiscard]] int finish_score(const std::wstring& text) const;
    [[nodiscard]] size_t entry_count() const noexcept { return counts_.size(); }

private:
    [[nodiscard]] int count(const std::wstring& sequence) const;
    [[nodiscard]] int transition_score(const std::wstring& sequence) const;
    void rebuild_statistics();
    std::unordered_map<std::wstring, int> counts_;
    std::unordered_map<std::wstring, int> dictionary_counts_;
    std::unordered_map<std::wstring, int> learned_counts_;
    std::unordered_map<std::wstring, double> continuation_totals_;
    double unigram_total_ = 0;
    mutable std::unordered_map<std::wstring, int> transition_scores_;
    mutable std::unordered_map<std::wstring, int> word_scores_;
};

}  // namespace localpinyin
