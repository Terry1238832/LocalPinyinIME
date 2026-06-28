#include "test_common.h"
#include "../src/engine/dictionary.h"
#include "../src/engine/pinyin_engine.h"

#include <windows.h>

#include <filesystem>
#include <string>

namespace {

std::filesystem::path executable_directory() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
}

std::filesystem::path make_temp_directory() {
    wchar_t temp_path[MAX_PATH]{};
    GetTempPathW(MAX_PATH, temp_path);
    wchar_t unique[MAX_PATH]{};
    GetTempFileNameW(temp_path, L"lpi", 0, unique);
    DeleteFileW(unique);
    std::filesystem::create_directories(unique);
    return unique;
}

int require_first_candidate(const std::wstring& pinyin, const std::wstring& expected) {
    localpinyin::PinyinEngine engine;
    const auto candidates = engine.lookup(pinyin);
    REQUIRE_TRUE(!candidates.empty());
    REQUIRE_EQ(candidates.front().text, expected);
    return 0;
}

}  // namespace

int main() {
    const auto source_dictionary = executable_directory() / L"dictionary" / L"core_zh_pinyin.tsv";
    REQUIRE_TRUE(std::filesystem::exists(source_dictionary));

    const auto layout_root = make_temp_directory();
    const auto unrelated_cwd = make_temp_directory();
    const auto fake_x64 = layout_root / L"x64";
    const auto fake_dictionary_dir = fake_x64 / L"dictionary";
    std::filesystem::create_directories(fake_dictionary_dir);
    std::filesystem::copy_file(source_dictionary,
                               fake_dictionary_dir / L"core_zh_pinyin.tsv",
                               std::filesystem::copy_options::overwrite_existing);

    SetCurrentDirectoryW(unrelated_cwd.c_str());

    localpinyin::Dictionary explicit_layout(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(explicit_layout.load_from_resource_directory(fake_x64.wstring()));
    REQUIRE_TRUE(explicit_layout.entry_count() >= 300);
    REQUIRE_EQ(explicit_layout.lookup(L"nihao").front().text, std::wstring(L"\u4F60\u597D"));

    REQUIRE_EQ(require_first_candidate(L"nihao", L"\u4F60\u597D"), 0);
    REQUIRE_EQ(require_first_candidate(L"nihaoshijie", L"\u4F60\u597D\u4E16\u754C"), 0);
    REQUIRE_EQ(require_first_candidate(L"woxiangqubeijing", L"\u6211\u60F3\u53BB\u5317\u4EAC"), 0);

    localpinyin::Dictionary missing(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(!missing.load_from_resource_directory((layout_root / L"missing").wstring()));
    REQUIRE_EQ(missing.entry_count(), static_cast<size_t>(0));

    SetCurrentDirectoryW(executable_directory().c_str());
    std::filesystem::remove_all(layout_root);
    std::filesystem::remove_all(unrelated_cwd);
    return 0;
}
