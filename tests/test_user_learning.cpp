#include "test_common.h"
#include "../src/engine/user_learning.h"

#include <windows.h>

int main() {
    wchar_t temp_path[MAX_PATH]{};
    wchar_t file_path[MAX_PATH]{};
    GetTempPathW(MAX_PATH, temp_path);
    GetTempFileNameW(temp_path, L"lpi", 0, file_path);

    {
        localpinyin::UserLearning learning;
        REQUIRE_TRUE(learning.open(file_path));
        REQUIRE_TRUE(learning.record_selection(L"nihao", L"\u4F60\u597D"));
        REQUIRE_TRUE(learning.record_selection(L"nihao", L"\u4F60\u597D"));
        REQUIRE_EQ(learning.frequency(L"nihao", L"\u4F60\u597D"), 2);
        REQUIRE_TRUE(learning.clear());
        REQUIRE_EQ(learning.frequency(L"nihao", L"\u4F60\u597D"), 0);
        REQUIRE_TRUE(learning.record_selection(L"nihao", L"\u4F60\u597D"));
    }
    {
        localpinyin::UserLearning learning;
        REQUIRE_TRUE(learning.open(file_path));
        REQUIRE_EQ(learning.frequency(L"nihao", L"\u4F60\u597D"), 1);
    }

    DeleteFileW(file_path);
    return 0;
}
