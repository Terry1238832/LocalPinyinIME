#pragma once

#include "candidate.h"
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace localpinyin {
class Dictionary;

// Explicit first-party next-word associations plus completions of dictionary
// and already-learned phrases. No host document text is stored by this model.
class ContinuationModel {
public:
    void reset(const Dictionary& dictionary);
    bool load(const std::wstring& path);
    void set_learning(const std::vector<std::pair<std::wstring, int>>& selections);
    [[nodiscard]] std::vector<Candidate> predict(const std::wstring& preceding_text,
                                                size_t limit = kMaxCandidateCount) const;
    [[nodiscard]] size_t rule_count() const noexcept { return rule_count_; }
    [[nodiscard]] static std::wstring context_suffix(const std::wstring& text);

private:
    struct Entry { std::wstring text; int weight = 0; bool personal = false; };
    using Index = std::unordered_map<std::wstring, std::vector<Entry>>;
    static void add(Index& index, const std::wstring& prefix, Entry entry);
    static void compact(Index& index);
    static void add_phrase(Index& index, const std::wstring& text, int weight, bool personal);
    Index dictionary_;
    Index rules_;
    Index learned_;
    size_t rule_count_ = 0;
};
}  // namespace localpinyin
