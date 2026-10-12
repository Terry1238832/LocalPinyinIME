#pragma once

#include <string>
#include <filesystem>
#include <unordered_map>

namespace localpinyin {

class UserLearning {
public:
    bool open(const std::wstring& storage_path);
    int frequency(const std::wstring& pinyin, const std::wstring& word) const;
    bool record_selection(const std::wstring& pinyin, const std::wstring& word);
    bool clear();
    void refresh_if_changed();

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
};

}  // namespace localpinyin
