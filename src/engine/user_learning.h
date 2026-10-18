#pragma once

#include <string>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <utility>

namespace localpinyin {

class UserLearning {
public:
    // The oldest selections are dropped beyond this many records.
    static constexpr size_t kCapacity = 2048;

    bool open(const std::wstring& storage_path);
    int frequency(const std::wstring& pinyin, const std::wstring& word) const;
    // Seconds since the epoch of the latest selection, or 0.
    long long last_selected(const std::wstring& pinyin, const std::wstring& word) const;
    bool record_selection(const std::wstring& pinyin, const std::wstring& word);
    bool clear();
    void refresh_if_changed();
    [[nodiscard]] size_t revision() const noexcept { return revision_; }
    [[nodiscard]] std::vector<std::pair<std::wstring, int>> selected_texts() const;
    // Every word learned for exactly this pinyin, including phrases built from
    // separately chosen pieces (造词) that the dictionary does not have.
    [[nodiscard]] const std::vector<std::wstring>& words_for(const std::wstring& pinyin) const;
    // Pinyin of learned phrases (two or more characters) that `input` spells
    // from `offset`, so a sentence can use them as tokens.
    [[nodiscard]] std::vector<std::wstring> phrase_keys_at(const std::wstring& input, size_t offset) const;

private:
    struct Record {
        int selection_count = 0;
        long long last_selected_time = 0;
    };

    bool flush() const;
    bool load();
    void prune();
    void remember_write_time();
    void reindex();

    std::wstring storage_path_;
    std::unordered_map<std::wstring, Record> records_;
    std::unordered_map<std::wstring, std::vector<std::wstring>> words_by_pinyin_;
    std::unordered_map<std::wstring, bool> phrase_keys_;
    size_t longest_phrase_key_ = 0;
    std::filesystem::file_time_type write_time_{};
    bool has_write_time_ = false;
    size_t revision_ = 0;
};

}  // namespace localpinyin
