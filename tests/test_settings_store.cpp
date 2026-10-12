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
    REQUIRE_TRUE(!store.load_input_mode_options().shift_toggle_enabled);
    CandidateWindowOptions options;
    options.theme_mode = CandidateThemeMode::Dark;
    options.show_key_hints = false;
    options.text_size = CandidateTextSize::Level6;
    InputModeOptions input_options;
    input_options.shift_toggle_enabled = false;
    REQUIRE_TRUE(store.save_options(options, input_options));

    const CandidateWindowOptions loaded = store.load_candidate_options();
    REQUIRE_EQ(loaded.theme_mode, CandidateThemeMode::Dark);
    REQUIRE_TRUE(!loaded.show_key_hints);
    REQUIRE_EQ(loaded.text_size, CandidateTextSize::Level6);
    REQUIRE_TRUE(!store.load_input_mode_options().shift_toggle_enabled);

    const auto dictionary = root / L"core_zh_pinyin.tsv";
    const auto learning = std::filesystem::path(store.learning_data_path());
    write_text_file(dictionary, "nihao\tplaceholder\t100\n");
    write_text_file(learning, "nihao\tplaceholder\t1\t1\n");

    options.theme_mode = CandidateThemeMode::Light;
    options.show_key_hints = true;
    options.text_size = CandidateTextSize::Level2;
    REQUIRE_TRUE(store.save_candidate_options(options));
    REQUIRE_TRUE(std::filesystem::exists(dictionary));
    REQUIRE_TRUE(std::filesystem::exists(learning));
    REQUIRE_TRUE(!store.load_input_mode_options().shift_toggle_enabled);

    input_options.shift_toggle_enabled = true;
    REQUIRE_TRUE(store.save_input_mode_options(input_options));
    const CandidateWindowOptions preserved = store.load_candidate_options();
    REQUIRE_EQ(preserved.theme_mode, CandidateThemeMode::Light);
    REQUIRE_TRUE(preserved.show_key_hints);
    REQUIRE_EQ(preserved.text_size, CandidateTextSize::Level2);
    REQUIRE_TRUE(store.load_input_mode_options().shift_toggle_enabled);
    REQUIRE_EQ(store.load_candidate_options().theme_mode, CandidateThemeMode::Light);
    REQUIRE_TRUE(store.load_input_mode_options().shift_toggle_enabled);

    RuntimeInputModeStatus status;
    status.schema_version = 1;
    status.active = true;
    status.mode = InputMode::English;
    status.source = L"ctrl_space_thread_keyboard_hook";
    status.process_id = GetCurrentProcessId();
    status.thread_id = 5678;
    status.sequence = 42;
    status.updated_at_utc_ms = runtime_status_current_utc_ms();
    status.available = true;
    REQUIRE_TRUE(store.save_runtime_input_mode_status(status));
    const RuntimeInputModeStatus loaded_status = store.load_runtime_input_mode_status();
    REQUIRE_TRUE(loaded_status.available);
    REQUIRE_TRUE(loaded_status.active);
    REQUIRE_EQ(loaded_status.schema_version, static_cast<unsigned int>(1));
    REQUIRE_EQ(loaded_status.mode, InputMode::English);
    REQUIRE_EQ(loaded_status.source, std::wstring(L"ctrl_space_thread_keyboard_hook"));
    REQUIRE_EQ(loaded_status.process_id, GetCurrentProcessId());
    REQUIRE_EQ(loaded_status.thread_id, static_cast<unsigned long>(5678));
    REQUIRE_EQ(loaded_status.sequence, static_cast<unsigned long long>(42));
    REQUIRE_TRUE(loaded_status.updated_at_utc_ms > 0);

    status.active = false;
    status.source = L"deactivate";
    status.sequence = 43;
    status.updated_at_utc_ms = runtime_status_current_utc_ms();
    REQUIRE_TRUE(store.save_runtime_input_mode_status(status));
    const RuntimeInputModeStatus inactive_status = store.load_runtime_input_mode_status();
    REQUIRE_TRUE(!inactive_status.available);
    REQUIRE_TRUE(!inactive_status.active);

    status.active = true;
    status.source = L"activate";
    status.sequence = 44;
    status.process_id = 0;
    status.updated_at_utc_ms = runtime_status_current_utc_ms();
    REQUIRE_TRUE(store.save_runtime_input_mode_status(status));
    REQUIRE_TRUE(!store.load_runtime_input_mode_status().available);

    status.process_id = GetCurrentProcessId();
    status.sequence = 45;
    status.updated_at_utc_ms = runtime_status_current_utc_ms() - runtime_status_stale_after_ms() - 1;
    REQUIRE_TRUE(store.save_runtime_input_mode_status(status));
    REQUIRE_TRUE(!store.load_runtime_input_mode_status().available);

    write_text_file(root / L"runtime_status.json",
                    "{\n"
                    "  \"mode\": \"Chinese\",\n"
                    "  \"source\": \"legacy\"\n"
                    "}\n");
    REQUIRE_TRUE(!store.load_runtime_input_mode_status().available);

    write_text_file(root / L"settings.json",
                    "{\n"
                    "  \"candidateWindow\": {\"theme\": \"dark\", \"showKeyHints\": true, \"textSize\": \"large\"},\n"
                    "  \"inputMode\": {\"shiftToggleEnabled\": true}\n"
                    "}\n");
    REQUIRE_EQ(store.load_candidate_options().text_size, CandidateTextSize::Large);

    write_text_file(root / L"settings.json",
                    "{\n"
                    "  \"candidateWindow\": {\"theme\": \"dark\", \"showKeyHints\": true, \"textSize\": \"large\"},\n"
                    "  \"inputMode\": {\"shiftToggleEnabled\": true},\n"
                    "  \"updates\": {\"promptShown\": \"bad\", \"autoCheckEnabled\": 1, \"lastAutoCheckUnixSeconds\": -5, \"lastCheckUnixSeconds\": \"x\"}\n"
                    "}\n");

    const auto blocked_parent = root / L"not-a-directory";
    write_text_file(blocked_parent, "blocked");
    SettingsStore failing_store((blocked_parent / L"child").wstring());
    REQUIRE_TRUE(!failing_store.save_options(options, input_options));

    REQUIRE_TRUE(store.clear_learning_data());
    REQUIRE_TRUE(std::filesystem::exists(dictionary));
    REQUIRE_TRUE(!std::filesystem::exists(learning));

    std::filesystem::remove_all(root);
    return 0;
}
