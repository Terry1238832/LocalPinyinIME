#pragma once

#include "candidate.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace localpinyin {

enum class DictionaryLoadMode {
    LoadDefaultResource,
    Empty
};

struct DictionaryStats {
    size_t source_rows = 0;
    size_t comment_rows = 0;
    size_t blank_rows = 0;
    size_t duplicate_rows = 0;
    size_t invalid_rows = 0;
    size_t valid_entries = 0;
};

class Dictionary {
public:
    Dictionary();
    explicit Dictionary(DictionaryLoadMode mode);

    void clear();
    bool load_from_default_resource();
    bool load_from_resource_directory(const std::wstring& directory);
    bool load_from_json_file(const std::wstring& path);
    bool load_from_tsv_file(const std::wstring& path);
    [[nodiscard]] std::vector<Candidate> lookup(const std::wstring& pinyin) const;
    [[nodiscard]] std::vector<std::wstring> matching_pinyins_at(const std::wstring& input, size_t offset) const;
    [[nodiscard]] size_t entry_count() const noexcept;
    [[nodiscard]] const DictionaryStats& stats() const noexcept { return stats_; }
    [[nodiscard]] bool using_fallback() const noexcept { return using_fallback_; }
    [[nodiscard]] const std::wstring& loaded_resource_path() const noexcept { return loaded_resource_path_; }
    [[nodiscard]] static std::wstring default_resource_path();

private:
    void load_minimal_fallback();
    void add_entry(const std::wstring& pinyin, const std::wstring& word, int base_frequency);
    void add_entry(const std::wstring& pinyin, const std::vector<std::wstring>& words);

    std::unordered_map<std::wstring, std::vector<Candidate>> entries_;
    size_t entry_count_ = 0;
    DictionaryStats stats_;
    bool using_fallback_ = false;
    std::wstring loaded_resource_path_;
};

}  // namespace localpinyin
