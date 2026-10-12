#include "test_common.h"
#include "../src/settings/dictionary_catalog.h"

#include <windows.h>

#include <filesystem>
#include <fstream>

int main() {
    using namespace localpinyin;
    wchar_t temp[MAX_PATH]{}; GetTempPathW(MAX_PATH, temp);
    wchar_t unique[MAX_PATH]{}; GetTempFileNameW(temp, L"lpc", 0, unique);
    DeleteFileW(unique); std::filesystem::create_directories(unique);
    const std::filesystem::path root(unique);
    { std::ofstream out(root / L"core_zh_pinyin.tsv", std::ios::binary);
      out << "# core\nnihao\t\xE4\xBD\xA0\xE5\xA5\xBD\t9000\ninvalid\n"; }
    { std::ofstream out(root / L"local_core_zh_pinyin.tsv", std::ios::binary);
      out << "\nnihao\t\xE4\xBD\xA0\xE5\xA5\xBD\t7000\nxiexie\t\xE8\xB0\xA2\xE8\xB0\xA2\t8000\nzaijian\t\xE5\x86\x8D\xE8\xA7\x81\t6000\n"; }
    DictionaryCatalog catalog;
    REQUIRE_TRUE(catalog.load(root.wstring()));
    REQUIRE_EQ(catalog.stats().core_entries, static_cast<size_t>(1));
    REQUIRE_EQ(catalog.stats().local_core_entries, static_cast<size_t>(3));
    REQUIRE_EQ(catalog.stats().total_unique_entries, static_cast<size_t>(3));
    REQUIRE_EQ(catalog.stats().invalid_rows, static_cast<size_t>(1));
    const auto defaults = catalog.preview(L"", 200);
    REQUIRE_EQ(defaults.size(), static_cast<size_t>(3));
    REQUIRE_EQ(defaults.front().pinyin, std::wstring(L"xiexie"));
    REQUIRE_EQ(defaults.front().source, DictionaryCatalogSource::LocalCore);
    REQUIRE_EQ(catalog.preview(L"xie", 500).size(), static_cast<size_t>(1));
    REQUIRE_EQ(catalog.preview(L"\u4F60\u597D", 500).size(), static_cast<size_t>(1));
    std::filesystem::remove_all(root);
    return 0;
}
