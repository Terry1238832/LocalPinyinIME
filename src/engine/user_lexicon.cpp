#include "user_lexicon.h"

#include "../common/utf_utils.h"
#include "../common/win32_utils.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace localpinyin {
namespace {

constexpr const char* kUserLexiconHeader =
    "# LocalPinyinIME user lexicon\n"
    "# UTF-8 TSV format: pinyin<TAB>word<TAB>frequency\n"
    "# Lines beginning with # are comments. Keep this file private to the current Windows user.\n";

std::wstring trim_ascii(std::wstring value) {
    const auto first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) {
        return L"";
    }
    const auto last = value.find_last_not_of(L" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool contains_tab_or_newline(const std::wstring& value) {
    return value.find_first_of(L"\t\r\n") != std::wstring::npos;
}

std::wstring lowercase_ascii_letters(const std::wstring& value, bool* valid) {
    std::wstring normalized;
    normalized.reserve(value.size());
    *valid = true;
    for (wchar_t ch : trim_ascii(value)) {
        if (ch >= L'A' && ch <= L'Z') {
            normalized.push_back(static_cast<wchar_t>(ch - L'A' + L'a'));
        } else if (ch >= L'a' && ch <= L'z') {
            normalized.push_back(ch);
        } else {
            *valid = false;
        }
    }
    return normalized;
}

std::vector<std::wstring> split_tsv_line(const std::wstring& line) {
    std::vector<std::wstring> fields;
    size_t begin = 0;
    while (begin <= line.size()) {
        const size_t tab = line.find(L'\t', begin);
        if (tab == std::wstring::npos) {
            fields.push_back(line.substr(begin));
            break;
        }
        fields.push_back(line.substr(begin, tab - begin));
        begin = tab + 1;
    }
    return fields;
}

std::wstring entry_key(const std::wstring& pinyin, const std::wstring& word) {
    return pinyin + L"\t" + word;
}

void sort_entries(std::vector<UserLexiconEntry>& entries) {
    std::stable_sort(entries.begin(), entries.end(), [](const auto& left, const auto& right) {
        if (left.pinyin != right.pinyin) {
            return left.pinyin < right.pinyin;
        }
        return left.word < right.word;
    });
}

bool ensure_user_lexicon_file(const std::filesystem::path& path, bool* created) {
    *created = false;
    std::error_code exists_error;
    if (std::filesystem::exists(path, exists_error)) {
        return !exists_error;
    }
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::error_code create_error;
        std::filesystem::create_directories(parent, create_error);
        if (create_error) {
            return false;
        }
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        return false;
    }
    file << kUserLexiconHeader;
    *created = true;
    return static_cast<bool>(file);
}

void deduplicate_entries(std::vector<UserLexiconEntry>& entries) {
    std::vector<UserLexiconEntry> unique_entries;
    std::unordered_map<std::wstring, size_t> indexes;
    for (const auto& entry : entries) {
        const std::wstring key = entry_key(entry.pinyin, entry.word);
        const auto it = indexes.find(key);
        if (it == indexes.end()) {
            indexes.emplace(key, unique_entries.size());
            unique_entries.push_back(entry);
        } else {
            unique_entries[it->second].frequency = entry.frequency;
        }
    }
    sort_entries(unique_entries);
    entries = std::move(unique_entries);
}

}  // namespace

std::wstring default_user_lexicon_path() {
    std::array<wchar_t, MAX_PATH> override_path{};
    const DWORD override_size = GetEnvironmentVariableW(L"LOCALPINYINIME_USER_LEXICON_PATH",
                                                        override_path.data(),
                                                        static_cast<DWORD>(override_path.size()));
    if (override_size > 0 && override_size < override_path.size()) {
        return override_path.data();
    }

    const std::wstring base = local_app_data_path();
    if (base.empty()) {
        return L"";
    }
    return base + L"\\LocalPinyinIME\\user_lexicon.tsv";
}

std::wstring user_lexicon_reload_hint() {
    return L"\u5DF2\u4FDD\u5B58\u3002\u8BF7\u5728\u6CA1\u6709\u6B63\u5728\u8F93\u5165\u7684\u62FC\u97F3\u65F6\u7EE7\u7EED\u4F7F\u7528\uFF1BLocalPinyinIME \u4F1A\u5728\u5B89\u5168\u65F6\u673A\u5237\u65B0\u7528\u6237\u8BCD\u5E93\u3002";
}

std::wstring user_lexicon_save_log_message(size_t valid_entries,
                                           size_t updated_entries,
                                           size_t removed_entries) {
    std::wstringstream stream;
    stream << L"user_lexicon_ui_save valid_entries=" << valid_entries
           << L" updated_entries=" << updated_entries
           << L" removed_entries=" << removed_entries;
    return stream.str();
}

UserLexiconValidationResult validate_user_lexicon_entry(const std::wstring& pinyin,
                                                        const std::wstring& word,
                                                        int frequency,
                                                        UserLexiconEntry* normalized_entry) {
    bool pinyin_valid = false;
    const std::wstring normalized_pinyin = lowercase_ascii_letters(pinyin, &pinyin_valid);
    if (!pinyin_valid || normalized_pinyin.empty()) {
        return {false, L"\u62FC\u97F3\u53EA\u80FD\u4F7F\u7528 a-z \u5B57\u6BCD\uFF0C\u4E0D\u80FD\u5305\u542B\u7A7A\u683C\u3001\u6570\u5B57\u3001\u58F0\u8C03\u6216\u7279\u6B8A\u7B26\u53F7\u3002"};
    }

    const std::wstring normalized_word = trim_ascii(word);
    if (normalized_word.empty()) {
        return {false, L"\u8BCD\u8BED\u4E0D\u80FD\u4E3A\u7A7A\u3002"};
    }
    if (contains_tab_or_newline(normalized_word)) {
        return {false, L"\u8BCD\u8BED\u4E0D\u80FD\u5305\u542B Tab \u6216\u6362\u884C\u3002"};
    }
    if (frequency < 0) {
        return {false, L"\u9891\u7387\u5FC5\u987B\u662F\u975E\u8D1F\u6574\u6570\u3002"};
    }

    if (normalized_entry) {
        *normalized_entry = UserLexiconEntry{normalized_pinyin, normalized_word, frequency};
    }
    return {true, L""};
}

bool parse_user_lexicon_frequency(const std::wstring& text, int* frequency) {
    if (!frequency) {
        return false;
    }
    const std::string narrow = wide_to_utf8(trim_ascii(text));
    if (narrow.empty() || narrow.front() == '-') {
        return false;
    }
    int parsed = 0;
    const char* begin = narrow.data();
    const char* end = begin + narrow.size();
    const auto [ptr, ec] = std::from_chars(begin, end, parsed);
    if (ec != std::errc{} || ptr != end || parsed < 0) {
        return false;
    }
    *frequency = parsed;
    return true;
}

UserLexiconLoadResult load_user_lexicon_file(const std::wstring& path, bool create_if_missing) {
    UserLexiconLoadResult result;
    if (path.empty()) {
        result.missing = true;
        result.failed = true;
        return result;
    }

    const std::filesystem::path file_path(path);
    std::error_code exists_error;
    const bool exists = std::filesystem::exists(file_path, exists_error);
    if (exists_error) {
        result.failed = true;
        return result;
    }
    if (!exists) {
        if (!create_if_missing) {
            result.missing = true;
            return result;
        }
        bool created = false;
        if (!ensure_user_lexicon_file(file_path, &created)) {
            result.failed = true;
            result.missing = true;
            return result;
        }
        result.created = created;
    }

    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        result.failed = true;
        return result;
    }

    std::unordered_map<std::wstring, size_t> indexes;
    std::string line;
    while (std::getline(file, line)) {
        ++result.stats.source_rows;
        const std::wstring decoded = utf8_to_wide(line);
        if (decoded.empty() && !line.empty()) {
            ++result.stats.invalid_rows;
            continue;
        }

        const std::wstring wide = trim_ascii(decoded);
        if (wide.empty()) {
            ++result.stats.blank_rows;
            continue;
        }
        if (wide.front() == L'#') {
            ++result.stats.comment_rows;
            continue;
        }

        const auto fields = split_tsv_line(wide);
        if (fields.size() != 3) {
            ++result.stats.invalid_rows;
            continue;
        }

        int frequency = 0;
        if (!parse_user_lexicon_frequency(fields[2], &frequency)) {
            ++result.stats.invalid_rows;
            continue;
        }

        UserLexiconEntry entry;
        if (!validate_user_lexicon_entry(fields[0], fields[1], frequency, &entry).valid) {
            ++result.stats.invalid_rows;
            continue;
        }

        const std::wstring key = entry_key(entry.pinyin, entry.word);
        const auto it = indexes.find(key);
        if (it == indexes.end()) {
            indexes.emplace(key, result.entries.size());
            result.entries.push_back(entry);
        } else {
            result.entries[it->second].frequency = entry.frequency;
            ++result.stats.duplicate_rows;
        }
    }

    sort_entries(result.entries);
    result.stats.valid_entries = result.entries.size();
    result.loaded = !result.entries.empty();
    return result;
}

UserLexiconSaveResult save_user_lexicon_file_atomic(const std::wstring& path,
                                                    const std::vector<UserLexiconEntry>& entries,
                                                    const UserLexiconSaveOptions& options) {
    UserLexiconSaveResult result;
    if (path.empty()) {
        result.win32_error = ERROR_PATH_NOT_FOUND;
        return result;
    }

    const std::filesystem::path file_path(path);
    const auto parent = file_path.parent_path();
    if (!parent.empty()) {
        std::error_code create_error;
        std::filesystem::create_directories(parent, create_error);
        if (create_error) {
            result.win32_error = static_cast<DWORD>(create_error.value());
            return result;
        }
    }

    std::vector<UserLexiconEntry> canonical = entries;
    deduplicate_entries(canonical);

    const std::wstring temp_path = path + L".tmp";
    {
        std::ofstream file(std::filesystem::path(temp_path), std::ios::binary | std::ios::trunc);
        if (!file) {
            result.win32_error = ERROR_ACCESS_DENIED;
            return result;
        }
        file << kUserLexiconHeader;
        for (const auto& entry : canonical) {
            UserLexiconEntry normalized;
            if (!validate_user_lexicon_entry(entry.pinyin, entry.word, entry.frequency, &normalized).valid) {
                result.win32_error = ERROR_INVALID_DATA;
                DeleteFileW(temp_path.c_str());
                return result;
            }
            file << wide_to_utf8(normalized.pinyin) << '\t'
                 << wide_to_utf8(normalized.word) << '\t'
                 << normalized.frequency << '\n';
        }
        if (!file) {
            result.win32_error = ERROR_WRITE_FAULT;
            DeleteFileW(temp_path.c_str());
            return result;
        }
    }

    if (options.simulate_replace_failure) {
        result.win32_error = ERROR_CANCELLED;
        DeleteFileW(temp_path.c_str());
        return result;
    }

    if (!MoveFileExW(temp_path.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        result.win32_error = GetLastError();
        DeleteFileW(temp_path.c_str());
        return result;
    }

    result.saved = true;
    result.win32_error = ERROR_SUCCESS;
    return result;
}

bool upsert_user_lexicon_entry(std::vector<UserLexiconEntry>& entries,
                               const UserLexiconEntry& entry,
                               bool* updated_existing) {
    UserLexiconEntry normalized;
    if (!validate_user_lexicon_entry(entry.pinyin, entry.word, entry.frequency, &normalized).valid) {
        return false;
    }

    if (updated_existing) {
        *updated_existing = false;
    }
    for (auto& existing : entries) {
        if (existing.pinyin == normalized.pinyin && existing.word == normalized.word) {
            existing.frequency = normalized.frequency;
            if (updated_existing) {
                *updated_existing = true;
            }
            sort_entries(entries);
            return true;
        }
    }

    entries.push_back(normalized);
    sort_entries(entries);
    return true;
}

bool remove_user_lexicon_entry(std::vector<UserLexiconEntry>& entries,
                               const std::wstring& pinyin,
                               const std::wstring& word) {
    const auto before = entries.size();
    entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const auto& entry) {
        return entry.pinyin == pinyin && entry.word == word;
    }), entries.end());
    return entries.size() != before;
}

std::vector<size_t> filter_user_lexicon_entries(const std::vector<UserLexiconEntry>& entries,
                                                const std::wstring& query) {
    const std::wstring needle = trim_ascii(query);
    std::vector<size_t> indexes;
    for (size_t i = 0; i < entries.size(); ++i) {
        if (needle.empty() ||
            entries[i].pinyin.find(needle) != std::wstring::npos ||
            entries[i].word.find(needle) != std::wstring::npos) {
            indexes.push_back(i);
        }
    }
    return indexes;
}

}  // namespace localpinyin
