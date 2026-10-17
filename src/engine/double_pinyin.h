#pragma once

#include "input_options.h"

#include <string>
#include <vector>

namespace localpinyin {

// Typed double-pinyin keys as full pinyin. Each key pair becomes one syllable
// and syllables are separated by apostrophes, so the engine segments exactly
// where the user's pairs are; a trailing single key becomes its initial (v ->
// zh in both schemes). Typed apostrophes are kept.
struct DoublePinyinConversion {
    std::wstring pinyin;
    // raw_end[i]: how many typed characters the first i characters of
    // `pinyin` stand for, so a partial commit removes the right keys.
    std::vector<size_t> raw_end;
};

DoublePinyinConversion convert_double_pinyin(const std::wstring& typed, DoublePinyinScheme scheme);

// The full-pinyin syllable for one key pair, or empty when the pair forms no
// syllable in the scheme.
std::wstring double_pinyin_syllable(wchar_t first, wchar_t second, DoublePinyinScheme scheme);

}  // namespace localpinyin
