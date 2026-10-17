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

private:
    struct Record {
        int selection_count = 0;
        long long last_selected_time = 0;
    };

    bool flush() const;
    bool load();
    void prune();
    void remember_write_time();

    std::wstring storage_path_;
    std::unordered_map<std::wstring, Record> records_;
    std::filesystem::file_time_type write_time_{};
    bool has_write_time_ = false;
    size_t revision_ = 0;
};

}  // namespace localpinyin
