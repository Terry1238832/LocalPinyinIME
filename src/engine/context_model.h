#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace localpinyin {

class Dictionary;

// A smoothed 1-4gram character language model in backoff form. Scores are
// natural log probabilities in thousandths (milli-nats), so a transition that
// the model considers ten times likelier scores about 2303 points higher.
//
// The packaged model (context_zh.lm) is memory mapped and shared by every
// process that loads the input method. A UTF-8 TSV of raw counts is also
// accepted; it is estimated in memory with interpolated absolute discounting.
class ContextModel {
public:
    void reset(const Dictionary& dictionary);
    bool load(const std::wstring& path);
    void set_learning(const std::vector<std::pair<std::wstring, int>>& selections);
    // Score of appending `word` after `previous` (an empty history is the
    // start of a clause). Scores are additive across any word segmentation.
    [[nodiscard]] int extend_score(const std::wstring& previous, const std::wstring& word) const;
    // Score of ending a clause after `text`.
    [[nodiscard]] int finish_score(const std::wstring& text) const;
    // log P(next | history) in milli-nats; only the last three history
    // characters are used. L'^' marks the start of a clause, L'$' its end.
    [[nodiscard]] int log_probability(std::wstring_view history, wchar_t next) const;
    [[nodiscard]] size_t entry_count() const noexcept;
    // Cost (milli-nats) of an implicit clause boundary: typed text often omits
    // the punctuation between clauses, so a character may also start a new one.
    void set_boundary_penalty(int penalty) noexcept { boundary_penalty_ = penalty; }
    [[nodiscard]] bool empty() const noexcept { return entry_count() == 0; }

private:
    struct Table {
        const std::uint32_t* keys = nullptr;
        const std::int16_t* logprob = nullptr;
        const std::int16_t* backoff = nullptr;
        size_t size = 0;
    };
    struct Storage;
    [[nodiscard]] bool load_binary(const std::wstring& path);
    [[nodiscard]] bool load_counts(const std::wstring& path);
    [[nodiscard]] int learned_bonus(std::wstring_view history, wchar_t next) const;
    [[nodiscard]] static size_t find(const Table& table, std::uint64_t packed, size_t order) noexcept;

    std::array<Table, 4> tables_{};
    int unknown_logprob_ = -14000;
    int boundary_penalty_ = -1500;
    std::shared_ptr<const Storage> storage_;
    // n-gram (2-4 characters) -> number of times the user selected text
    // containing it. Learned evidence is added on top of the shipped model.
    std::unordered_map<std::uint64_t, int> learned_;
};

}  // namespace localpinyin
