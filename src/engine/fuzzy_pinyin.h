#pragma once

#include <string>
#include <vector>

namespace localpinyin {

// The other spellings a typed syllable stands for under the enabled fuzzy
// pairs (input_options.h), e.g. zong -> zhong with z=zh, lin -> ling and
// nin, ning with n=l and in=ing. Only legal syllables; never the syllable
// itself.
std::vector<std::wstring> fuzzy_syllable_variants(const std::wstring& syllable, unsigned mask);

}  // namespace localpinyin
