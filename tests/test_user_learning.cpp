#include "test_common.h"
#include "../src/engine/user_learning.h"

#include <windows.h>
#include <fstream>
#include <thread>
#include <atomic>

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

    // A truncated or manually edited record must never crash an IME process.
    {
        std::ofstream file(std::filesystem::path(file_path), std::ios::binary | std::ios::trunc);
        file << "nihao\tvalid\t3\t42\n"
             << "nihao\tbad\tnot-a-number\t42\n"
             << "nihao\tnegative\t-7\t42\n"
             << "nihao\toverflow\t99999999999999999\t42\n"
             << "nihao\tbadtime\t7\tnonsense\n"
             << "malformed\n";
    }
    localpinyin::UserLearning first;
    localpinyin::UserLearning second;
    REQUIRE_TRUE(first.open(file_path));
    REQUIRE_TRUE(second.open(file_path));
    REQUIRE_EQ(first.frequency(L"nihao", L"valid"), 3);
    REQUIRE_EQ(first.frequency(L"nihao", L"bad"), 0);
    REQUIRE_EQ(first.frequency(L"nihao", L"negative"), 0);
    REQUIRE_EQ(first.frequency(L"nihao", L"badtime"), 0);
    REQUIRE_TRUE(!first.record_selection(L"nihao", L"bad\trow"));
    REQUIRE_TRUE(first.record_selection(L"nihao", L"valid"));
    REQUIRE_TRUE(second.record_selection(L"nihao", L"valid"));
    first.refresh_if_changed();
    REQUIRE_EQ(first.frequency(L"nihao", L"valid"), 5);

    // Concurrent IME instances merge under the same per-file lock.
    std::atomic<bool> writes_ok{true};
    const auto record_many = [&](localpinyin::UserLearning& learning) {
        for (int index = 0; index < 20; ++index) {
            if (!learning.record_selection(L"nihao", L"valid")) writes_ok = false;
        }
    };
    std::thread first_thread([&] { record_many(first); });
    std::thread second_thread([&] { record_many(second); });
    first_thread.join();
    second_thread.join();
    REQUIRE_TRUE(writes_ok);
    first.refresh_if_changed();
    REQUIRE_EQ(first.frequency(L"nihao", L"valid"), 45);
    REQUIRE_TRUE(second.clear());
    first.refresh_if_changed();
    REQUIRE_EQ(first.frequency(L"nihao", L"valid"), 0);

    DeleteFileW(file_path);
    return 0;
}
