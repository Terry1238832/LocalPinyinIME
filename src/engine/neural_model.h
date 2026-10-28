#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace localpinyin {

// A small character GRU language model (neural_zh.lm, tools/export_neural_lm.py)
// that rescores whole-sentence candidates. It sees the whole clause instead of
// three characters, so 这家店是专门卖 and 这家电视专门买 differ by everything
// before the deciding character. Weights are half floats in a read-only file
// mapping shared by every process; inference is single threaded.
class NeuralModel {
public:
    bool load(const std::wstring& path);
    void reset() noexcept;
    [[nodiscard]] bool empty() const noexcept { return vocab_ == 0; }
    // Natural-log probability of each text after `context` (the clause before
    // it, possibly empty). Texts sharing a beginning reuse its computation.
    [[nodiscard]] std::vector<double> score(std::wstring_view context, const std::vector<std::wstring>& texts) const;
    // Results for one beginning (context, then the first characters of a
    // text) are kept for the next keystroke, which mostly extends them.
    static constexpr size_t kCacheEntries = 512;

private:
    struct Storage;
    using State = std::vector<float>;
    struct Cached { State state; double sum = 0; };
    // `scored`: add log P(ch) to the running sum (not for the context).
    [[nodiscard]] Cached advance(const std::wstring& key, const Cached& from, wchar_t ch, bool scored) const;
    [[nodiscard]] int token(wchar_t ch) const noexcept;
    void step(int token, const State& before, State& after) const;
    [[nodiscard]] double log_probability(const State& state, int token) const;

    size_t vocab_ = 0, embed_ = 0, hidden_ = 0;
    const std::uint16_t* embedding_ = nullptr;
    const std::uint16_t* input_weights_ = nullptr;
    const std::uint16_t* hidden_weights_ = nullptr;
    const std::uint16_t* input_bias_ = nullptr;
    const std::uint16_t* hidden_bias_ = nullptr;
    const std::uint16_t* projection_ = nullptr;
    const std::uint16_t* projection_bias_ = nullptr;
    const std::uint16_t* output_bias_ = nullptr;
    std::unordered_map<wchar_t, int> tokens_;
    bool vector_math_ = false;
    mutable std::unordered_map<std::wstring, Cached> cache_;
    std::shared_ptr<const Storage> storage_;
};

}  // namespace localpinyin
