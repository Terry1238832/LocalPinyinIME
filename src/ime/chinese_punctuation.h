#pragma once
#include <Windows.h>

namespace localpinyin {

// Keep an unshifted apostrophe available as a pinyin syllable separator.
inline wchar_t chinese_punctuation_for_key(WPARAM key, bool shift, bool composing,
                                           bool opening_double_quote, bool opening_single_quote) noexcept {
    switch (key) {
        case VK_OEM_COMMA: return shift ? L'《' : L'，';
        case VK_OEM_PERIOD: return shift ? L'》' : L'。';
        case VK_OEM_1: return shift ? L'：' : L'；';
        case VK_OEM_2: return shift ? L'？' : L'、';
        case VK_OEM_4: return shift ? L'｛' : L'【';
        case VK_OEM_6: return shift ? L'｝' : L'】';
        case VK_OEM_7:
            if (shift) return opening_double_quote ? L'“' : L'”';
            return composing ? 0 : (opening_single_quote ? L'‘' : L'’');
        case L'1': return shift ? L'！' : 0;
        case L'9': return shift ? L'（' : 0;
        case L'0': return shift ? L'）' : 0;
        default: return 0;
    }
}

} // namespace localpinyin
