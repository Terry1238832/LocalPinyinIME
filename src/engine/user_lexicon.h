#pragma once

#include <windows.h>

#include <cstddef>
#include <string>
#include <vector>

namespace localpinyin {

struct UserLexiconEntry {
    std::wstring pinyin;
    std::wstring word;
    int frequency = 0;
};

struct UserLexiconStats {
    size_t source_rows = 0;
    size_t comment_rows = 0;
    size_t blank_rows = 0;
    size_t duplicate_rows = 0;
    size_t invalid_rows = 0;
    size_t valid_entries = 0;
};

struct UserLexiconLoadResult {
    bool loaded = false;
    bool missing = false;
    bool created = false;
    bool failed = false;
    UserLexiconStats stats;
    std::vector<UserLexiconEntry> entries;
};

struct UserLexiconSaveOptions {
    bool simulate_replace_failure = false;
};

struct UserLexiconSaveResult {
    bool saved = false;
    DWORD win32_error = ERROR_SUCCESS;
};

struct UserLexiconValidationResult {
    bool valid = false;
    std::wstring message;
};

[[nodiscard]] std::wstring default_user_lexicon_path();
[[nodiscard]] std::wstring user_lexicon_reload_hint();
[[nodiscard]] std::wstring user_lexicon_save_log_message(size_t valid_entries,
                                                         size_t updated_entries,
                                                         size_t removed_entries);
[[nodiscard]] UserLexiconValidationResult validate_user_lexicon_entry(const std::wstring& pinyin,
                                                                      const std::wstring& word,
                                                                      int frequency,
                                                                      UserLexiconEntry* normalized_entry);
[[nodiscard]] bool parse_user_lexicon_frequency(const std::wstring& text, int* frequency);
[[nodiscard]] UserLexiconLoadResult load_user_lexicon_file(const std::wstring& path, bool create_if_missing);
[[nodiscard]] UserLexiconSaveResult save_user_lexicon_file_atomic(
    const std::wstring& path,
    const std::vector<UserLexiconEntry>& entries,
    const UserLexiconSaveOptions& options = {});
[[nodiscard]] bool upsert_user_lexicon_entry(std::vector<UserLexiconEntry>& entries,
                                             const UserLexiconEntry& entry,
                                             bool* updated_existing);
[[nodiscard]] bool remove_user_lexicon_entry(std::vector<UserLexiconEntry>& entries,
                                             const std::wstring& pinyin,
                                             const std::wstring& word);
[[nodiscard]] std::vector<size_t> filter_user_lexicon_entries(const std::vector<UserLexiconEntry>& entries,
                                                              const std::wstring& query);

}  // namespace localpinyin
