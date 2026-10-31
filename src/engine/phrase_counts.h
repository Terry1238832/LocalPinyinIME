#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace localpinyin {

// How often short phrases are sent as a whole clause in real chat
// (phrase_zh.lm, tools/build_phrase_counts.py): 你猜 far more than 你才, though
// the lexicon ranks 你才 first. The file is memory mapped and shared between
// processes.
class PhraseCounts {
public:
    static constexpr double kLevelsPerDecade = 32.0;

    bool load(const std::wstring& path);
    void reset() noexcept;
    [[nodiscard]] bool empty() const noexcept { return count_ == 0; }
    // log10(occurrences) in units of 1/32; zero for an unstored phrase.
    [[nodiscard]] int level(std::wstring_view phrase) const noexcept;

private:
    struct Storage;
    const std::uint32_t* keys_ = nullptr;
    const std::uint8_t* levels_ = nullptr;
    size_t count_ = 0;
    std::shared_ptr<const Storage> storage_;
};

}  // namespace localpinyin
