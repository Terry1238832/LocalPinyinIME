#include "test_common.h"
#include "../src/common/utf_utils.h"
#include "../src/engine/dictionary.h"
#include "../src/engine/pinyin_engine.h"
#include "../src/engine/user_lexicon.h"

#include <windows.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {

void write_utf8_text(const std::filesystem::path& path, const std::wstring& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file << localpinyin::wide_to_utf8(text);
    file.close();

    static int write_generation = 1;
    std::error_code ec;
    std::filesystem::last_write_time(
        path,
        std::filesystem::file_time_type::clock::now() + std::chrono::seconds(write_generation++),
        ec);
}

std::wstring read_utf8_text(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return localpinyin::utf8_to_wide(content);
}

bool contains_candidate_text(const std::vector<localpinyin::Candidate>& candidates, const std::wstring& text) {
    for (const auto& candidate : candidates) {
        if (candidate.text == text) {
            return true;
        }
    }
    return false;
}

}  // namespace

int main() {
    const auto root = make_test_temp_directory();
    const auto user_path = root / L"user_lexicon.tsv";

    auto missing = localpinyin::load_user_lexicon_file(user_path.wstring(), true);
    REQUIRE_TRUE(missing.created);
    REQUIRE_TRUE(std::filesystem::exists(user_path));
    REQUIRE_EQ(missing.entries.size(), static_cast<size_t>(0));

    localpinyin::UserLexiconEntry entry;
    REQUIRE_TRUE(localpinyin::validate_user_lexicon_entry(L"NIHAO", L"\u4F60\u597D", 5000, &entry).valid);
    REQUIRE_EQ(entry.pinyin, std::wstring(L"nihao"));
    REQUIRE_TRUE(!localpinyin::validate_user_lexicon_entry(L"csc108", L"CSC108", 5000, &entry).valid);
    REQUIRE_TRUE(!localpinyin::validate_user_lexicon_entry(L"ni hao", L"\u4F60\u597D", 5000, &entry).valid);
    REQUIRE_TRUE(!localpinyin::validate_user_lexicon_entry(L"bad", L"", 1, &entry).valid);
    REQUIRE_TRUE(!localpinyin::validate_user_lexicon_entry(L"bad", L"bad\tword", 1, &entry).valid);
    REQUIRE_TRUE(!localpinyin::validate_user_lexicon_entry(L"bad", L"bad\nword", 1, &entry).valid);
    REQUIRE_TRUE(!localpinyin::validate_user_lexicon_entry(L"bad", L"\u574F", -1, &entry).valid);

    int frequency = 0;
    REQUIRE_TRUE(localpinyin::parse_user_lexicon_frequency(L"123", &frequency));
    REQUIRE_EQ(frequency, 123);
    REQUIRE_TRUE(!localpinyin::parse_user_lexicon_frequency(L"-1", &frequency));
    REQUIRE_TRUE(!localpinyin::parse_user_lexicon_frequency(L"12.3", &frequency));

    std::vector<localpinyin::UserLexiconEntry> entries;
    bool updated = false;
    REQUIRE_TRUE(localpinyin::upsert_user_lexicon_entry(entries, {L"NIHAO", L"\u4F60\u597D", 5000}, &updated));
    REQUIRE_TRUE(!updated);
    REQUIRE_TRUE(localpinyin::upsert_user_lexicon_entry(entries, {L"nihao", L"\u4F60\u597D", 6000}, &updated));
    REQUIRE_TRUE(updated);
    REQUIRE_TRUE(localpinyin::upsert_user_lexicon_entry(entries, {L"hao", L"\u4F60\u597D", 3000}, &updated));
    REQUIRE_EQ(entries.size(), static_cast<size_t>(2));
    REQUIRE_TRUE(localpinyin::remove_user_lexicon_entry(entries, L"hao", L"\u4F60\u597D"));
    REQUIRE_EQ(entries.size(), static_cast<size_t>(1));

    const auto save_failed = localpinyin::save_user_lexicon_file_atomic(
        user_path.wstring(),
        std::vector<localpinyin::UserLexiconEntry>{{L"old", L"\u65E7", 1}});
    REQUIRE_TRUE(save_failed.saved);
    const std::wstring old_content = read_utf8_text(user_path);
    const auto simulated = localpinyin::save_user_lexicon_file_atomic(
        user_path.wstring(),
        std::vector<localpinyin::UserLexiconEntry>{{L"new", L"\u65B0", 2}},
        localpinyin::UserLexiconSaveOptions{true});
    REQUIRE_TRUE(!simulated.saved);
    REQUIRE_EQ(read_utf8_text(user_path), old_content);

    REQUIRE_TRUE(localpinyin::save_user_lexicon_file_atomic(user_path.wstring(), entries).saved);
    auto loaded = localpinyin::load_user_lexicon_file(user_path.wstring(), false);
    REQUIRE_EQ(loaded.entries.size(), static_cast<size_t>(1));
    REQUIRE_EQ(loaded.entries.front().frequency, 6000);

    const auto core_path = root / L"core.tsv";
    const auto local_core_path = root / L"local_core.tsv";
    write_utf8_text(core_path,
                    L"nihao\t\u4F60\u597D\t100\n"
                    L"custom\t\u57FA\u7840\t10\n");
    write_utf8_text(local_core_path, L"# local core\n");
    write_utf8_text(user_path, L"custom\t\u65E7\u8BCD\t1000\n");

    localpinyin::Dictionary dictionary(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(dictionary.load_from_layered_files(core_path.wstring(),
                                                    local_core_path.wstring(),
                                                    user_path.wstring(),
                                                    localpinyin::UserLexiconCreateMode::CreateIfMissing));
    REQUIRE_EQ(dictionary.lookup(L"custom").front().text, std::wstring(L"\u65E7\u8BCD"));

    Sleep(30);
    write_utf8_text(user_path, L"custom\t\u65B0\u8BCD\t2000\n");
    auto active_refresh = dictionary.refresh_user_lexicon_if_changed(true);
    REQUIRE_TRUE(active_refresh.changed);
    REQUIRE_TRUE(active_refresh.skipped_active_composition);
    REQUIRE_EQ(dictionary.lookup(L"custom").front().text, std::wstring(L"\u65E7\u8BCD"));

    auto idle_refresh = dictionary.refresh_user_lexicon_if_changed(false);
    REQUIRE_TRUE(idle_refresh.changed);
    REQUIRE_TRUE(idle_refresh.reloaded);
    REQUIRE_EQ(dictionary.lookup(L"custom").front().text, std::wstring(L"\u65B0\u8BCD"));

    const std::wstring log_message = localpinyin::user_lexicon_save_log_message(2, 1, 0);
    REQUIRE_TRUE(log_message.find(L"\u65B0\u8BCD") == std::wstring::npos);
    REQUIRE_TRUE(log_message.find(L"custom") == std::wstring::npos);

    const auto engine_root = root / L"engine";
    const auto dictionary_dir = engine_root / L"dictionary";
    const auto engine_user_path = root / L"engine_user_lexicon.tsv";
    write_utf8_text(dictionary_dir / L"core_zh_pinyin.tsv", L"nihao\t\u4F60\u597D\t100\n");
    write_utf8_text(dictionary_dir / L"local_core_zh_pinyin.tsv", L"# local core\n");
    write_utf8_text(engine_user_path, L"# test user lexicon\n");

    localpinyin::PinyinEngine engine(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(engine.load_dictionary_resource_directory(engine_root.wstring(), engine_user_path.wstring()));
    auto initial_settingscheck = engine.lookup(L"settingscheck");
    REQUIRE_TRUE(!contains_candidate_text(initial_settingscheck, L"\u8BBE\u7F6E\u754C\u9762\u6D4B\u8BD5"));

    write_utf8_text(engine_user_path, L"settingscheck\t\u8BBE\u7F6E\u754C\u9762\u6D4B\u8BD5\t9000\n");
    auto active_engine_refresh = engine.refresh_user_lexicon_if_needed(true);
    REQUIRE_TRUE(active_engine_refresh.changed);
    REQUIRE_TRUE(active_engine_refresh.skipped_active_composition);
    REQUIRE_TRUE(!contains_candidate_text(engine.lookup(L"settingscheck"), L"\u8BBE\u7F6E\u754C\u9762\u6D4B\u8BD5"));

    auto idle_engine_refresh = engine.refresh_user_lexicon_if_needed(false);
    REQUIRE_TRUE(idle_engine_refresh.changed);
    REQUIRE_TRUE(idle_engine_refresh.reloaded);
    auto refreshed_settingscheck = engine.lookup(L"settingscheck");
    REQUIRE_TRUE(!refreshed_settingscheck.empty());
    REQUIRE_EQ(refreshed_settingscheck.front().text, std::wstring(L"\u8BBE\u7F6E\u754C\u9762\u6D4B\u8BD5"));

    write_utf8_text(engine_user_path, L"# removed test user lexicon\n");
    auto remove_active_refresh = engine.refresh_user_lexicon_if_needed(true);
    REQUIRE_TRUE(remove_active_refresh.changed);
    REQUIRE_TRUE(remove_active_refresh.skipped_active_composition);
    REQUIRE_TRUE(contains_candidate_text(engine.lookup(L"settingscheck"), L"\u8BBE\u7F6E\u754C\u9762\u6D4B\u8BD5"));

    auto remove_idle_refresh = engine.refresh_user_lexicon_if_needed(false);
    REQUIRE_TRUE(remove_idle_refresh.changed);
    REQUIRE_TRUE(remove_idle_refresh.reloaded);
    REQUIRE_TRUE(!contains_candidate_text(engine.lookup(L"settingscheck"), L"\u8BBE\u7F6E\u754C\u9762\u6D4B\u8BD5"));

    const std::wstring runtime_log_message = localpinyin::user_lexicon_save_log_message(1, 1, 1);
    REQUIRE_TRUE(runtime_log_message.find(L"settingscheck") == std::wstring::npos);
    REQUIRE_TRUE(runtime_log_message.find(L"\u8BBE\u7F6E\u754C\u9762\u6D4B\u8BD5") == std::wstring::npos);

    std::filesystem::remove_all(root);
    return 0;
}
