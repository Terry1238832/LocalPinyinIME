#include "dictionary.h"

#include "../common/logging.h"
#include "../common/utf_utils.h"
#include "../common/win32_utils.h"

#include <windows.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace localpinyin {
namespace {

std::wstring trim_ascii(std::wstring value) {
    const auto first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) {
        return L"";
    }
    const auto last = value.find_last_not_of(L" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::wstring normalize_pinyin_key(const std::wstring& value) {
    std::wstring normalized;
    normalized.reserve(value.size());
    for (wchar_t ch : value) {
        if (ch >= L'A' && ch <= L'Z') {
            normalized.push_back(static_cast<wchar_t>(ch - L'A' + L'a'));
        } else if (ch >= L'a' && ch <= L'z') {
            normalized.push_back(ch);
        } else if (ch >= L'0' && ch <= L'9') {
            normalized.push_back(ch);
        }
    }
    return normalized;
}

std::wstring directory_name(std::wstring path) {
    const auto slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return L".";
    }
    return path.substr(0, slash);
}

void dictionary_module_anchor() {}

std::wstring module_directory() {
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(&dictionary_module_anchor),
                            &module)) {
        return L".";
    }

    std::array<wchar_t, MAX_PATH> path{};
    const DWORD size = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
    if (size == 0 || size >= path.size()) {
        return L".";
    }
    return directory_name(path.data());
}

std::wstring join_path(const std::wstring& directory, const std::wstring& leaf) {
    if (directory.empty()) {
        return leaf;
    }
    const wchar_t last = directory.back();
    if (last == L'\\' || last == L'/') {
        return directory + leaf;
    }
    return directory + L"\\" + leaf;
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

bool parse_int(const std::wstring& value, int& result) {
    const std::string narrow = wide_to_utf8(trim_ascii(value));
    const char* begin = narrow.data();
    const char* end = begin + narrow.size();
    const auto [ptr, ec] = std::from_chars(begin, end, result);
    return ec == std::errc{} && ptr == end;
}

std::wstring bool_text(bool value) {
    return value ? L"true" : L"false";
}

void merge_stats(DictionaryStats& target, const DictionaryStats& source) {
    target.source_rows += source.source_rows;
    target.comment_rows += source.comment_rows;
    target.blank_rows += source.blank_rows;
    target.duplicate_rows += source.duplicate_rows;
    target.invalid_rows += source.invalid_rows;
}

std::wstring make_layer_log_message(const DictionaryLayerStats& layer) {
    std::wstringstream stream;
    stream << L"dictionary_layer=" << layer.layer_name
           << L" loaded=" << bool_text(layer.loaded)
           << L" missing=" << bool_text(layer.missing)
           << L" created=" << bool_text(layer.created)
           << L" valid_entries=" << layer.stats.valid_entries
           << L" invalid_rows=" << layer.stats.invalid_rows
           << L" duplicate_rows=" << layer.stats.duplicate_rows
           << L" comment_rows=" << layer.stats.comment_rows
           << L" blank_rows=" << layer.stats.blank_rows;
    return stream.str();
}

bool write_empty_user_lexicon(const std::wstring& path) {
    const std::filesystem::path file_path(path);
    const auto parent = file_path.parent_path();
    if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            return false;
        }
    }

    std::ofstream file(file_path, std::ios::binary | std::ios::trunc);
    if (!file) {
        return false;
    }
    file << "# LocalPinyinIME user lexicon\n"
         << "# UTF-8 TSV format: pinyin<TAB>word<TAB>frequency\n"
         << "# Lines beginning with # are comments. Keep this file private to the current Windows user.\n";
    return static_cast<bool>(file);
}

}  // namespace

Dictionary::Dictionary() {
    load_from_default_resource();
}

Dictionary::Dictionary(DictionaryLoadMode mode) {
    if (mode == DictionaryLoadMode::LoadDefaultResource) {
        load_from_default_resource();
    }
}

void Dictionary::clear() {
    entries_.clear();
    entry_count_ = 0;
    stats_ = {};
    layer_stats_.clear();
    layer_log_messages_.clear();
    using_fallback_ = false;
    loaded_resource_path_.clear();
}

bool Dictionary::add_entry(const std::wstring& pinyin,
                           const std::wstring& word,
                           int base_frequency,
                           EntryAddMode mode) {
    const std::wstring key = normalize_pinyin_key(pinyin);
    if (key.empty() || word.empty() || base_frequency < 0) {
        return false;
    }

    auto& candidates = entries_[key];
    const auto it = std::find_if(candidates.begin(), candidates.end(), [&](const Candidate& candidate) {
        return candidate.text == word;
    });
    const bool added = it == candidates.end();
    if (!added) {
        if (mode == EntryAddMode::ReplaceExisting) {
            it->base_score = base_frequency;
        } else {
            it->base_score = std::max(it->base_score, base_frequency);
        }
    } else {
        candidates.push_back(Candidate{word, base_frequency, 0});
        ++entry_count_;
    }

    std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) {
        return left.base_score > right.base_score;
    });
    return added;
}

void Dictionary::add_entry(const std::wstring& pinyin, const std::vector<std::wstring>& words) {
    int score = static_cast<int>(words.size()) * 100;
    for (const auto& word : words) {
        add_entry(pinyin, word, score);
        score -= 10;
    }
}

void Dictionary::load_minimal_fallback() {
    add_entry(L"nihao", L"\u4F60\u597D", 100);
    add_entry(L"wo", L"\u6211", 100);
    using_fallback_ = true;
}

std::wstring Dictionary::default_resource_path() {
    return join_path(join_path(module_directory(), L"dictionary"), L"core_zh_pinyin.tsv");
}

std::wstring Dictionary::default_local_core_resource_path() {
    return join_path(join_path(module_directory(), L"dictionary"), L"local_core_zh_pinyin.tsv");
}

std::wstring Dictionary::default_user_lexicon_path() {
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
    return join_path(join_path(base, L"LocalPinyinIME"), L"user_lexicon.tsv");
}

bool Dictionary::load_from_default_resource() {
    const std::wstring core_path = default_resource_path();
    if (load_from_layered_files(core_path,
                                default_local_core_resource_path(),
                                default_user_lexicon_path(),
                                UserLexiconCreateMode::CreateIfMissing)) {
        return true;
    }

    const HRESULT hr = std::filesystem::exists(std::filesystem::path(core_path))
                           ? E_FAIL
                           : HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    log_status(L"dictionary", L"load failed hr=" + hresult_hex(hr));
    log_status(L"dictionary", L"loaded entries=0");
    load_minimal_fallback();
    log_status(L"dictionary", L"fallback dictionary active");
    return false;
}

bool Dictionary::load_from_resource_directory(const std::wstring& directory) {
    return load_from_resource_directory(directory, default_user_lexicon_path());
}

bool Dictionary::load_from_resource_directory(const std::wstring& directory,
                                              const std::wstring& user_lexicon_path) {
    const std::wstring dictionary_dir = join_path(directory, L"dictionary");
    return load_from_layered_files(join_path(dictionary_dir, L"core_zh_pinyin.tsv"),
                                   join_path(dictionary_dir, L"local_core_zh_pinyin.tsv"),
                                   user_lexicon_path,
                                   UserLexiconCreateMode::CreateIfMissing);
}

bool Dictionary::load_from_layered_files(const std::wstring& core_path,
                                         const std::wstring& local_core_path,
                                         const std::wstring& user_lexicon_path,
                                         UserLexiconCreateMode create_mode) {
    clear();

    const auto core = load_layer(L"core", core_path, EntryAddMode::KeepExisting, true, false);
    merge_stats(stats_, core.stats);
    layer_stats_.push_back(core);

    const auto local_core = load_layer(L"local_core", local_core_path, EntryAddMode::ReplaceExisting, false, false);
    merge_stats(stats_, local_core.stats);
    layer_stats_.push_back(local_core);

    const auto user = load_layer(L"local_user",
                                 user_lexicon_path,
                                 EntryAddMode::ReplaceExisting,
                                 false,
                                 create_mode == UserLexiconCreateMode::CreateIfMissing);
    merge_stats(stats_, user.stats);
    layer_stats_.push_back(user);

    stats_.valid_entries = entry_count_;
    if (entry_count_ > 0) {
        loaded_resource_path_ = core.loaded ? core_path : L"";
        return true;
    }
    return false;
}

DictionaryLayerStats Dictionary::load_layer(const std::wstring& layer_name,
                                            const std::wstring& path,
                                            EntryAddMode mode,
                                            bool /*required*/,
                                            bool create_if_missing) {
    DictionaryLayerStats layer;
    layer.layer_name = layer_name;
    if (path.empty()) {
        layer.missing = true;
        layer_log_messages_.push_back(make_layer_log_message(layer));
        log_status(L"dictionary", layer_log_messages_.back());
        return layer;
    }

    std::error_code exists_error;
    bool exists = std::filesystem::exists(std::filesystem::path(path), exists_error);
    if (!exists && !exists_error && create_if_missing) {
        layer.created = write_empty_user_lexicon(path);
        exists = layer.created;
    }
    if (!exists || exists_error) {
        layer.missing = true;
        layer_log_messages_.push_back(make_layer_log_message(layer));
        log_status(L"dictionary", layer_log_messages_.back());
        return layer;
    }

    layer.loaded = load_tsv_file_into(path, layer.stats, mode);
    layer_log_messages_.push_back(make_layer_log_message(layer));
    log_status(L"dictionary", layer_log_messages_.back());
    return layer;
}

bool Dictionary::load_tsv_file_into(const std::wstring& path,
                                    DictionaryStats& layer_stats,
                                    EntryAddMode mode) {
    std::ifstream file(std::filesystem::path(path), std::ios::binary);
    if (!file) {
        return false;
    }

    bool loaded_any = false;
    std::string line;
    while (std::getline(file, line)) {
        ++layer_stats.source_rows;

        const std::wstring decoded = utf8_to_wide(line);
        if (decoded.empty() && !line.empty()) {
            ++layer_stats.invalid_rows;
            continue;
        }

        const std::wstring wide = trim_ascii(decoded);
        if (wide.empty()) {
            ++layer_stats.blank_rows;
            continue;
        }
        if (wide.front() == L'#') {
            ++layer_stats.comment_rows;
            continue;
        }

        const auto fields = split_tsv_line(wide);
        if (fields.size() != 3) {
            ++layer_stats.invalid_rows;
            continue;
        }

        int frequency = 0;
        const std::wstring pinyin = trim_ascii(fields[0]);
        const std::wstring word = trim_ascii(fields[1]);
        if (normalize_pinyin_key(pinyin).empty() || word.empty() || !parse_int(fields[2], frequency) || frequency < 0) {
            ++layer_stats.invalid_rows;
            continue;
        }
        ++layer_stats.valid_entries;
        loaded_any = true;
        if (!add_entry(pinyin, word, frequency, mode)) {
            ++layer_stats.duplicate_rows;
        }
    }

    return loaded_any;
}

bool Dictionary::load_from_tsv_file(const std::wstring& path) {
    clear();
    DictionaryLayerStats layer;
    layer.layer_name = L"single";
    layer.loaded = load_tsv_file_into(path, layer.stats, EntryAddMode::KeepExisting);
    layer_stats_.push_back(layer);
    stats_ = layer.stats;
    stats_.valid_entries = entry_count_;
    if (layer.loaded) {
        loaded_resource_path_ = path;
    }
    return layer.loaded;
}

bool Dictionary::load_from_json_file(const std::wstring& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    const std::wstring content = utf8_to_wide(buffer.str());

    std::wstring key;
    std::vector<std::wstring> values;
    bool in_string = false;
    bool escape = false;
    std::wstring token;
    bool reading_key = true;

    for (wchar_t ch : content) {
        if (escape) {
            token.push_back(ch);
            escape = false;
            continue;
        }
        if (ch == L'\\' && in_string) {
            escape = true;
            continue;
        }
        if (ch == L'"') {
            if (in_string) {
                if (reading_key) {
                    key = token;
                    values.clear();
                    reading_key = false;
                } else {
                    values.push_back(token);
                }
                token.clear();
                in_string = false;
            } else {
                in_string = true;
            }
            continue;
        }
        if (in_string) {
            token.push_back(ch);
            continue;
        }
        if (ch == L']' && !key.empty()) {
            add_entry(trim_ascii(key), values);
            key.clear();
            values.clear();
            reading_key = true;
        }
    }

    return true;
}

std::vector<Candidate> Dictionary::lookup(const std::wstring& pinyin) const {
    const auto it = entries_.find(normalize_pinyin_key(pinyin));
    if (it == entries_.end()) {
        return {};
    }
    return it->second;
}

std::vector<std::wstring> Dictionary::matching_pinyins_at(const std::wstring& input, size_t offset) const {
    std::vector<std::wstring> matches;
    for (const auto& [pinyin, candidates] : entries_) {
        if (candidates.empty() || pinyin.empty() || offset + pinyin.size() > input.size()) {
            continue;
        }
        if (input.compare(offset, pinyin.size(), pinyin) == 0) {
            matches.push_back(pinyin);
        }
    }

    std::stable_sort(matches.begin(), matches.end(), [](const std::wstring& left, const std::wstring& right) {
        return left.size() > right.size();
    });
    return matches;
}

size_t Dictionary::entry_count() const noexcept {
    return entry_count_;
}

}  // namespace localpinyin
