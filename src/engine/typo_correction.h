#pragma once

#include <string>
#include <vector>

namespace localpinyin {

// One-keystroke slips on a QWERTY keyboard, the way a typist makes them: an
// extra key (often doubled or a neighbor), a missing key, two keys swapped,
// or a neighboring key hit instead. Each correction has a cost (lower is more
// likely).
//
// Sound confusions are not typos: inserting or dropping the h of zh/ch/sh,
// the g of -ng, or swapping n/l, l/r and f/h belong to fuzzy pinyin, which the
// user enables per pair, so they are never proposed here.
struct SpellingCorrection {
    std::wstring pinyin;
    int cost = 0;
};

// `broad` also allows substituting a key that is not a neighbor (used when
// the input reads as nothing at all).
std::vector<SpellingCorrection> spelling_corrections(const std::wstring& input, bool broad);

// The same edits limited to positions [first, last) of `input`.
std::vector<SpellingCorrection> spelling_corrections_near(const std::wstring& input, size_t first, size_t last,
                                                          bool broad);

[[nodiscard]] bool keyboard_neighbors(wchar_t a, wchar_t b) noexcept;

}  // namespace localpinyin
