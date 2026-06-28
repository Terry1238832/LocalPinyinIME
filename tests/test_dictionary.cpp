#include "test_common.h"
#include "../src/engine/dictionary.h"

int main() {
    localpinyin::Dictionary dictionary;
    REQUIRE_TRUE(dictionary.entry_count() >= 300);
    REQUIRE_EQ(dictionary.stats().source_rows, static_cast<size_t>(404));
    REQUIRE_EQ(dictionary.stats().duplicate_rows, static_cast<size_t>(1));
    REQUIRE_EQ(dictionary.stats().invalid_rows, static_cast<size_t>(0));
    REQUIRE_EQ(dictionary.stats().valid_entries, dictionary.entry_count());

    auto candidates = dictionary.lookup(L"nihao");
    REQUIRE_TRUE(!candidates.empty());
    REQUIRE_EQ(candidates.front().text, std::wstring(L"\u4F60\u597D"));
    REQUIRE_EQ(dictionary.lookup(L"zhongguo").front().text, std::wstring(L"\u4E2D\u56FD"));
    REQUIRE_EQ(dictionary.lookup(L"beijing").front().text, std::wstring(L"\u5317\u4EAC"));
    REQUIRE_TRUE(dictionary.lookup(L"missing").empty());
    return 0;
}
