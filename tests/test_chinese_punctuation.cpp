#include "test_common.h"
#include "../src/ime/chinese_punctuation.h"

int main() {
    using localpinyin::chinese_punctuation_for_key;
    REQUIRE_EQ(chinese_punctuation_for_key(VK_OEM_COMMA, false, true, true, true), L'，');
    REQUIRE_EQ(chinese_punctuation_for_key(VK_OEM_PERIOD, false, false, true, true), L'。');
    REQUIRE_EQ(chinese_punctuation_for_key(VK_OEM_2, true, true, true, true), L'？');
    REQUIRE_EQ(chinese_punctuation_for_key(L'1', true, true, true, true), L'！');
    REQUIRE_EQ(chinese_punctuation_for_key(L'1', false, true, true, true), wchar_t(0));
    REQUIRE_EQ(chinese_punctuation_for_key(L'9', true, false, true, true), L'（');
    REQUIRE_EQ(chinese_punctuation_for_key(L'0', true, false, true, true), L'）');
    REQUIRE_EQ(chinese_punctuation_for_key(VK_OEM_7, false, true, true, true), wchar_t(0));
    REQUIRE_EQ(chinese_punctuation_for_key(VK_OEM_7, false, false, true, true), L'‘');
    REQUIRE_EQ(chinese_punctuation_for_key(VK_OEM_7, false, false, true, false), L'’');
    REQUIRE_EQ(chinese_punctuation_for_key(VK_OEM_7, true, true, true, true), L'“');
    REQUIRE_EQ(chinese_punctuation_for_key(VK_OEM_7, true, false, false, true), L'”');
    REQUIRE_EQ(chinese_punctuation_for_key(L'A', false, false, true, true), wchar_t(0));

    // Code editors type ASCII punctuation: <({[!?, and the rest.
    using localpinyin::ascii_punctuation_for_key;
    REQUIRE_EQ(ascii_punctuation_for_key(VK_OEM_COMMA, false), L',');
    REQUIRE_EQ(ascii_punctuation_for_key(VK_OEM_COMMA, true), L'<');
    REQUIRE_EQ(ascii_punctuation_for_key(VK_OEM_PERIOD, false), L'.');
    REQUIRE_EQ(ascii_punctuation_for_key(VK_OEM_2, true), L'?');
    REQUIRE_EQ(ascii_punctuation_for_key(VK_OEM_4, false), L'[');
    REQUIRE_EQ(ascii_punctuation_for_key(VK_OEM_4, true), L'{');
    REQUIRE_EQ(ascii_punctuation_for_key(L'1', true), L'!');
    REQUIRE_EQ(ascii_punctuation_for_key(L'9', true), L'(');
    REQUIRE_EQ(ascii_punctuation_for_key(L'9', false), wchar_t(0));
    REQUIRE_EQ(ascii_punctuation_for_key(L'A', true), wchar_t(0));
    using localpinyin::is_digit_punctuation;
    // The period has a switch of its own; , / : share the other one.
    REQUIRE_TRUE(is_digit_punctuation(L'，') && is_digit_punctuation(L'、') && is_digit_punctuation(L'：'));
    REQUIRE_TRUE(!is_digit_punctuation(L'。') && !is_digit_punctuation(L'？') && !is_digit_punctuation(L'《'));
    using localpinyin::is_code_application;
    REQUIRE_TRUE(is_code_application(L"Code.exe"));
    REQUIRE_TRUE(is_code_application(L"devenv.exe"));
    REQUIRE_TRUE(is_code_application(L"idea64.exe"));
    REQUIRE_TRUE(is_code_application(L"WindowsTerminal.exe"));
    REQUIRE_TRUE(!is_code_application(L"notepad.exe"));
    REQUIRE_TRUE(!is_code_application(L"QQMusic.exe"));
    REQUIRE_TRUE(!is_code_application(L"code.exe.bak"));
    return 0;
}
