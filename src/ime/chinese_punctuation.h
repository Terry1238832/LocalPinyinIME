#pragma once
#include <Windows.h>

#include <algorithm>
#include <cwctype>
#include <string_view>

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

// What a punctuation key types on a US keyboard, for code editors where
// Chinese punctuation is never wanted: <({[!?, and the rest. Zero otherwise.
inline wchar_t ascii_punctuation_for_key(WPARAM key, bool shift) noexcept {
    switch (key) {
        case VK_OEM_COMMA: return shift ? L'<' : L',';
        case VK_OEM_PERIOD: return shift ? L'>' : L'.';
        case VK_OEM_1: return shift ? L':' : L';';
        case VK_OEM_2: return shift ? L'?' : L'/';
        case VK_OEM_4: return shift ? L'{' : L'[';
        case VK_OEM_6: return shift ? L'}' : L']';
        case VK_OEM_7: return shift ? L'"' : L'\'';
        case L'1': return shift ? L'!' : 0;
        case L'9': return shift ? L'(' : 0;
        case L'0': return shift ? L')' : 0;
        default: return 0;
    }
}

// Chinese punctuation that follows a digit as ASCII: 。，、： become . , / :
// (3.14, 1,000, 1/2, 12:30).
inline bool is_digit_punctuation(wchar_t punctuation) noexcept {
    return punctuation == L'。' || punctuation == L'，' || punctuation == L'、' || punctuation == L'：';
}

// Code editors, IDEs and terminals, by executable file name (any case).
inline bool is_code_application(std::wstring_view executable) noexcept {
    static constexpr std::wstring_view kNames[] = {
        L"code.exe", L"code - insiders.exe", L"cursor.exe", L"windsurf.exe", L"trae.exe", L"zed.exe",
        L"devenv.exe", L"idea.exe", L"idea64.exe", L"pycharm.exe", L"pycharm64.exe", L"webstorm64.exe",
        L"clion64.exe", L"goland64.exe", L"rider64.exe", L"phpstorm64.exe", L"rubymine64.exe",
        L"datagrip64.exe", L"rustrover64.exe", L"studio64.exe", L"fleet.exe", L"sublime_text.exe",
        L"notepad++.exe", L"hbuilderx.exe", L"eclipse.exe", L"gvim.exe", L"nvim-qt.exe", L"emacs.exe",
        L"windowsterminal.exe", L"openconsole.exe", L"conhost.exe", L"powershell_ise.exe",
    };
    return std::any_of(std::begin(kNames), std::end(kNames), [&](std::wstring_view name) {
        return name.size() == executable.size() &&
               std::equal(name.begin(), name.end(), executable.begin(), [](wchar_t left, wchar_t right) {
                   return left == static_cast<wchar_t>(std::towlower(static_cast<wint_t>(right)));
               });
    });
}

} // namespace localpinyin
