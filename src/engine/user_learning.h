#pragma once

#include <string>
#include <unordered_map>

namespace localpinyin {

class UserLearning {
public:
    bool open(const std::wstring& storage_path);
    int frequency(const std::wstring& pinyin, const std::wstring& word) const;
    bool record_selection(const std::wstring& pinyin, const std::wstring& word);
    bool clear();

private:
    struct Record {
        int selection_count = 0;
        long long last_selected_time = 0;
    };

    bool flush() const;
    bool load();

    std::wstring storage_path_;
    std::unordered_map<std::wstring, Record> records_;
};

}  // namespace localpinyin
