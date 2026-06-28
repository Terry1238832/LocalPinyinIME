#include "../engine/dictionary.h"
#include "../engine/pinyin_engine.h"
#include "../common/utf_utils.h"

#include <windows.h>

#include <filesystem>
#include <iostream>
#include <string>

namespace {

const char* bool_text(bool value) {
    return value ? "TRUE" : "FALSE";
}

bool is_absolute_path(const std::wstring& path) {
    return std::filesystem::path(path).is_absolute();
}

std::wstring first_candidate(localpinyin::PinyinEngine& engine, const std::wstring& pinyin) {
    const auto candidates = engine.lookup(pinyin);
    return candidates.empty() ? L"<none>" : candidates.front().text;
}

void print_usage() {
    std::cout << "Usage:\n"
              << "  LocalPinyinImeDictionarySmoke.exe --dictionary <absolute path>\n";
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    SetConsoleOutputCP(CP_UTF8);

    std::wstring dictionary_path;
    for (int i = 1; i < argc; ++i) {
        const std::wstring arg = argv[i];
        if (arg == L"--dictionary") {
            if (i + 1 >= argc) {
                std::cerr << "error: --dictionary requires a value\n";
                return 2;
            }
            dictionary_path = argv[++i];
        } else {
            std::cerr << "error: unknown argument: " << localpinyin::wide_to_utf8(arg) << "\n";
            print_usage();
            return 2;
        }
    }

    if (dictionary_path.empty()) {
        print_usage();
        return 2;
    }
    if (!is_absolute_path(dictionary_path)) {
        std::cerr << "error: --dictionary must be an absolute path\n";
        return 2;
    }

    const bool exists = std::filesystem::exists(std::filesystem::path(dictionary_path));
    localpinyin::Dictionary dictionary(localpinyin::DictionaryLoadMode::Empty);
    const bool loaded = dictionary.load_from_tsv_file(dictionary_path);

    std::cout << "Dictionary path: " << localpinyin::wide_to_utf8(dictionary_path) << "\n"
              << "Dictionary exists: " << bool_text(exists) << "\n"
              << "Dictionary loaded: " << bool_text(loaded) << "\n"
              << "Source rows: " << dictionary.stats().source_rows << "\n"
              << "Comment rows: " << dictionary.stats().comment_rows << "\n"
              << "Blank rows: " << dictionary.stats().blank_rows << "\n"
              << "Duplicate rows: " << dictionary.stats().duplicate_rows << "\n"
              << "Invalid rows: " << dictionary.stats().invalid_rows << "\n"
              << "Valid entries: " << dictionary.stats().valid_entries << "\n";

    localpinyin::PinyinEngine engine(localpinyin::DictionaryLoadMode::Empty);
    const bool engine_loaded = engine.load_dictionary(dictionary_path);
    const std::wstring nihao = first_candidate(engine, L"nihao");
    const std::wstring nihaoshijie = first_candidate(engine, L"nihaoshijie");
    const std::wstring woxiangqubeijing = first_candidate(engine, L"woxiangqubeijing");
    const bool nihao_ok = nihao == L"\u4F60\u597D";
    const bool nihaoshijie_ok = nihaoshijie == L"\u4F60\u597D\u4E16\u754C";
    const bool woxiangqubeijing_ok = woxiangqubeijing == L"\u6211\u60F3\u53BB\u5317\u4EAC";
    std::cout << "nihao first candidate: " << localpinyin::wide_to_utf8(nihao) << "\n"
              << "nihaoshijie first candidate: " << localpinyin::wide_to_utf8(nihaoshijie) << "\n"
              << "woxiangqubeijing first candidate: " << localpinyin::wide_to_utf8(woxiangqubeijing) << "\n"
              << "nihao expected match: " << bool_text(nihao_ok) << "\n"
              << "nihaoshijie expected match: " << bool_text(nihaoshijie_ok) << "\n"
              << "woxiangqubeijing expected match: " << bool_text(woxiangqubeijing_ok) << "\n";

    const bool candidates_ok = nihao_ok && nihaoshijie_ok && woxiangqubeijing_ok;
    return exists && loaded && engine_loaded && dictionary.stats().valid_entries >= 300 && candidates_ok ? 0 : 1;
}
