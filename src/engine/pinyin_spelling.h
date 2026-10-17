#pragma once

#include <string>

namespace localpinyin {

// ü before e is written ue (lue, nue: 略, 虐) but is also typed ve (lve,
// nve). Dictionary keys and typed input are both brought to ue, so either
// spelling finds the same words. A following r keeps v: nver is nü'er (女儿).
inline void spell_ue(std::wstring& pinyin) {
    for (size_t index = 1; index + 1 < pinyin.size(); ++index) {
        if (pinyin[index] == L'v' && (pinyin[index - 1] == L'l' || pinyin[index - 1] == L'n') &&
            pinyin[index + 1] == L'e' && (index + 2 >= pinyin.size() || pinyin[index + 2] != L'r')) {
            pinyin[index] = L'u';
        }
    }
}

}  // namespace localpinyin
