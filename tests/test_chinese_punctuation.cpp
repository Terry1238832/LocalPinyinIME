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
    return 0;
}
