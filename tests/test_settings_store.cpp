#include "test_common.h"
#include "../src/settings/settings_store.h"

#include <windows.h>

#include <filesystem>
#include <fstream>

namespace {

std::filesystem::path make_temp_directory() {
    wchar_t temp_path[MAX_PATH]{};
    GetTempPathW(MAX_PATH, temp_path);
    wchar_t unique[MAX_PATH]{};
    GetTempFileNameW(temp_path, L"lps", 0, unique);
    DeleteFileW(unique);
    std::filesystem::create_directories(unique);
    return unique;
}

void write_text_file(const std::filesystem::path& path, const char* text) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file << text;
}

}  // namespace

int main() {
    using namespace localpinyin;

    const auto root = make_temp_directory();
    SettingsStore store(root.wstring());

    CandidateWindowOptions options;
    options.theme_mode = CandidateThemeMode::Dark;
    options.show_key_hints = false;
    options.text_size = CandidateTextSize::Large;
    REQUIRE_TRUE(store.save_candidate_options(options));

    const CandidateWindowOptions loaded = store.load_candidate_options();
    REQUIRE_EQ(loaded.theme_mode, CandidateThemeMode::Dark);
    REQUIRE_TRUE(!loaded.show_key_hints);
    REQUIRE_EQ(loaded.text_size, CandidateTextSize::Large);

    const auto dictionary = root / L"core_zh_pinyin.tsv";
    const auto learning = std::filesystem::path(store.learning_data_path());
    write_text_file(dictionary, "nihao\tplaceholder\t100\n");
    write_text_file(learning, "nihao\tplaceholder\t1\t1\n");

    options.theme_mode = CandidateThemeMode::Light;
    options.show_key_hints = true;
    options.text_size = CandidateTextSize::Small;
    REQUIRE_TRUE(store.save_candidate_options(options));
    REQUIRE_TRUE(std::filesystem::exists(dictionary));
    REQUIRE_TRUE(std::filesystem::exists(learning));

    REQUIRE_TRUE(store.clear_learning_data());
    REQUIRE_TRUE(std::filesystem::exists(dictionary));
    REQUIRE_TRUE(!std::filesystem::exists(learning));

    std::filesystem::remove_all(root);
    return 0;
}
