#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace localpinyin {

// Associations of adjacent lexicon words (word_zh.lm, tools/build_word_model.py).
// The character model sees three characters of history; this one rewards a
// word that often follows the previous word, wherever the deciding characters
// are. A stored pair scores ln(observed / expected) in milli-nats; any other
// pair scores zero. The file is memory mapped and shared between processes.
class WordModel {
public:
    bool load(const std::wstring& path);
    void reset() noexcept;
    [[nodiscard]] bool empty() const noexcept { return pair_count_ == 0; }
    [[nodiscard]] int pair_score(std::wstring_view previous, std::wstring_view word) const noexcept;
    // The longest suffix (up to four characters) of committed text that begins
    // a stored pair: the previous word for the first word of new input.
    [[nodiscard]] std::wstring previous_word(std::wstring_view preceding) const;

private:
    struct Storage;
    const std::uint32_t* word_keys_ = nullptr;
    const std::uint32_t* pair_keys_ = nullptr;
    const std::int16_t* pair_scores_ = nullptr;
    size_t word_count_ = 0;
    size_t pair_count_ = 0;
    std::shared_ptr<const Storage> storage_;
};

}  // namespace localpinyin
