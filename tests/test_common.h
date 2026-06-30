#pragma once

#include <windows.h>

#include <filesystem>
#include <iostream>
#include <string>

#define REQUIRE_TRUE(expr) do { if (!(expr)) { std::wcerr << L"Requirement failed: " << L#expr << L"\n"; return 1; } } while (false)
#define REQUIRE_EQ(left, right) do { if (!((left) == (right))) { std::wcerr << L"Requirement failed: " << L#left << L" == " << L#right << L"\n"; return 1; } } while (false)

inline std::filesystem::path make_test_temp_directory() {
    wchar_t temp_path[MAX_PATH]{};
    wchar_t unique[MAX_PATH]{};
    GetTempPathW(MAX_PATH, temp_path);
    GetTempFileNameW(temp_path, L"lpi", 0, unique);
    DeleteFileW(unique);
    std::filesystem::create_directories(unique);
    return unique;
}

inline std::filesystem::path use_temp_user_lexicon_override() {
    const auto root = make_test_temp_directory();
    const auto lexicon = root / L"user_lexicon.tsv";
    SetEnvironmentVariableW(L"LOCALPINYINIME_USER_LEXICON_PATH", lexicon.c_str());
    return root;
}
