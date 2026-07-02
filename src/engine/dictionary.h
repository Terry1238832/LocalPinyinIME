#pragma once

#include "candidate.h"
#include "user_lexicon.h"

#include <cstddef>
#include <filesystem>
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

struct DictionaryLayerStats {
    std::wstring layer_name;
    DictionaryStats stats;
    bool loaded = false;
    bool missing = false;
    bool created = false;
};

struct UserLexiconRefreshResult {
    bool checked = false;
    bool changed = false;
    bool skipped_active_composition = false;
    bool reloaded = false;
    bool failed = false;
    UserLexiconStats stats;
};

enum class UserLexiconCreateMode {
    CreateIfMissing,
    DoNotCreate
};

class Dictionary {
public:
    Dictionary();
    explicit Dictionary(DictionaryLoadMode mode);

    void clear();
    bool load_from_default_resource();
    bool load_from_resource_directory(const std::wstring& directory);
    bool load_from_resource_directory(const std::wstring& directory, const std::wstring& user_lexicon_path);
    bool load_from_layered_files(const std::wstring& core_path,
                                 const std::wstring& local_core_path,
                                 const std::wstring& user_lexicon_path,
                                 UserLexiconCreateMode create_mode = UserLexiconCreateMode::CreateIfMissing);
    bool load_from_json_file(const std::wstring& path);
    bool load_from_tsv_file(const std::wstring& path);
    [[nodiscard]] std::vector<Candidate> lookup(const std::wstring& pinyin) const;
    [[nodiscard]] std::vector<std::wstring> matching_pinyins_at(const std::wstring& input, size_t offset) const;
    UserLexiconRefreshResult refresh_user_lexicon_if_changed(bool composition_active);
    [[nodiscard]] size_t entry_count() const noexcept;
    [[nodiscard]] const DictionaryStats& stats() const noexcept { return stats_; }
    [[nodiscard]] const std::vector<DictionaryLayerStats>& layer_stats() const noexcept { return layer_stats_; }
    [[nodiscard]] const std::vector<std::wstring>& layer_log_messages() const noexcept { return layer_log_messages_; }
    [[nodiscard]] bool using_fallback() const noexcept { return using_fallback_; }
    [[nodiscard]] const std::wstring& loaded_resource_path() const noexcept { return loaded_resource_path_; }
    [[nodiscard]] static std::wstring default_resource_path();
    [[nodiscard]] static std::wstring default_local_core_resource_path();
    [[nodiscard]] static std::wstring default_user_lexicon_path();

private:
    enum class EntryAddMode {
        KeepExisting,
        ReplaceExisting
    };

    void load_minimal_fallback();
    bool user_lexicon_file_changed() const;
    void remember_user_lexicon_write_time();
    void revert_user_lexicon_entries();
    bool add_user_lexicon_entry(const UserLexiconEntry& entry, DictionaryStats& layer_stats);
    bool load_user_lexicon_layer(const std::wstring& path,
                                 DictionaryLayerStats& layer,
                                 bool create_if_missing);
    void recompute_stats_from_layers();
    bool load_tsv_file_into(const std::wstring& path,
                            DictionaryStats& layer_stats,
                            EntryAddMode mode);
    DictionaryLayerStats load_layer(const std::wstring& layer_name,
                                    const std::wstring& path,
                                    EntryAddMode mode,
                                    bool required,
                                    bool create_if_missing);
    bool add_entry(const std::wstring& pinyin,
                   const std::wstring& word,
                   int base_frequency,
                   EntryAddMode mode = EntryAddMode::KeepExisting);
    void add_entry(const std::wstring& pinyin, const std::vector<std::wstring>& words);

    std::unordered_map<std::wstring, std::vector<Candidate>> entries_;
    size_t entry_count_ = 0;
    DictionaryStats stats_;
    std::vector<DictionaryLayerStats> layer_stats_;
    std::vector<std::wstring> layer_log_messages_;
    bool using_fallback_ = false;
    std::wstring loaded_resource_path_;
    struct UserLexiconOverlay {
        std::wstring pinyin;
        std::wstring word;
        bool had_previous = false;
        int previous_score = 0;
    };
    std::wstring user_lexicon_path_;
    std::vector<UserLexiconOverlay> user_lexicon_overlays_;
    std::filesystem::file_time_type user_lexicon_write_time_{};
    bool has_user_lexicon_write_time_ = false;
};

}  // namespace localpinyin
