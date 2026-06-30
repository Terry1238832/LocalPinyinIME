#include "settings_store.h"

#include "../common/utf_utils.h"
#include "../common/win32_utils.h"

#include <windows.h>

#include <fstream>
#include <utility>

namespace localpinyin {
namespace {

std::wstring read_utf8_file(const std::wstring& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return L"";
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return utf8_to_wide(content);
}

std::wstring setting_value(const std::wstring& text, const std::wstring& key) {
    const std::wstring quoted_key = L"\"" + key + L"\"";
    const size_t key_pos = text.find(quoted_key);
    if (key_pos == std::wstring::npos) {
        return L"";
    }
    const size_t colon = text.find(L':', key_pos + quoted_key.size());
    if (colon == std::wstring::npos) {
        return L"";
    }
    size_t value_start = text.find_first_not_of(L" \t\r\n", colon + 1);
    if (value_start == std::wstring::npos) {
        return L"";
    }
    if (text[value_start] == L'"') {
        const size_t value_end = text.find(L'"', value_start + 1);
        if (value_end == std::wstring::npos) {
            return L"";
        }
        return text.substr(value_start + 1, value_end - value_start - 1);
    }
    const size_t value_end = text.find_first_of(L",}\r\n", value_start);
    return text.substr(value_start, value_end == std::wstring::npos ? std::wstring::npos : value_end - value_start);
}

bool parse_bool(const std::wstring& text, bool fallback) {
    if (text == L"true") {
        return true;
    }
    if (text == L"false") {
        return false;
    }
    return fallback;
}

}  // namespace

SettingsStore::SettingsStore(std::wstring data_dir_override)
    : data_dir_override_(std::move(data_dir_override)) {}

std::wstring SettingsStore::data_dir() const {
    if (!data_dir_override_.empty()) {
        return data_dir_override_;
    }
    return local_app_data_path() + L"\\LocalPinyinIME";
}

std::wstring SettingsStore::settings_path() const {
    return data_dir() + L"\\settings.json";
}

std::wstring SettingsStore::learning_data_path() const {
    return data_dir() + L"\\user_learning.tsv";
}

bool SettingsStore::ensure_data_dir() const {
    return ensure_directory(data_dir());
}

CandidateWindowOptions SettingsStore::load_candidate_options() const {
    CandidateWindowOptions options;
    const std::wstring text = read_utf8_file(settings_path());
    if (text.empty()) {
        return options;
    }

    const std::wstring theme = setting_value(text, L"theme");
    const std::wstring text_size = setting_value(text, L"textSize");
    const std::wstring show_key_hints = setting_value(text, L"showKeyHints");
    if (!theme.empty()) {
        options.theme_mode = parse_candidate_theme_mode(theme);
    }
    if (!text_size.empty()) {
        options.text_size = parse_candidate_text_size(text_size);
    }
    if (!show_key_hints.empty()) {
        options.show_key_hints = parse_bool(show_key_hints, options.show_key_hints);
    }
    return options;
}

bool SettingsStore::save_candidate_options(const CandidateWindowOptions& options) const {
    if (!ensure_data_dir()) {
        return false;
    }

    std::wstring text;
    text += L"{\n";
    text += L"  \"candidateWindow\": {\n";
    text += L"    \"theme\": \"" + candidate_theme_mode_to_string(options.theme_mode) + L"\",\n";
    text += L"    \"showKeyHints\": ";
    text += options.show_key_hints ? L"true" : L"false";
    text += L",\n";
    text += L"    \"textSize\": \"" + candidate_text_size_to_string(options.text_size) + L"\"\n";
    text += L"  }\n";
    text += L"}\n";

    std::ofstream file(settings_path(), std::ios::binary | std::ios::trunc);
    if (!file) {
        return false;
    }
    file << wide_to_utf8(text);
    return true;
}

bool SettingsStore::clear_learning_data() const {
    const std::wstring path = learning_data_path();
    if (DeleteFileW(path.c_str())) {
        return true;
    }
    const DWORD error = GetLastError();
    return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
}

}  // namespace localpinyin
