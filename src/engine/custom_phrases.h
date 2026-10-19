#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace localpinyin {

// 输入习惯: text the user tied to words they type, from the tool panel's
// "添加短语" (美元 and 美刀 for $). Typing the word offers the text first;
// choosing another candidate there pushes it to second, never further,
// until the habit is deleted. Kept in custom_phrases.tsv beside the
// learning records: pinyin, text, the word as the user wrote it, and when
// it was added (seconds since the epoch).
struct CustomPhrase {
    std::wstring pinyin;
    std::wstring text;
    std::wstring label;
    long long created = 0;
};

class CustomPhrases {
public:
    static constexpr size_t kCapacity = 5000;

    void open(const std::wstring& path);
    // Reads the file again when another process changed it.
    void refresh_if_changed();
    [[nodiscard]] const std::vector<CustomPhrase>& for_pinyin(const std::wstring& pinyin) const;
    [[nodiscard]] const std::vector<CustomPhrase>& all() const noexcept { return entries_; }

    // Editing, for the settings bridge: each reads the file, changes it and
    // writes it back under a lock shared with other processes.
    static bool valid(const CustomPhrase& phrase);
    static std::vector<CustomPhrase> read(const std::wstring& path);
    static bool add(const std::wstring& path, CustomPhrase phrase);
    static bool remove(const std::wstring& path, const std::wstring& pinyin, const std::wstring& text);
    // Adds the phrase, or marks it added now when it is there; beyond
    // `capacity` the longest unused go. For emoji used from the panel.
    static bool remember(const std::wstring& path, CustomPhrase phrase, size_t capacity);

private:
    void load();

    std::wstring path_;
    std::vector<CustomPhrase> entries_;
    std::unordered_map<std::wstring, std::vector<CustomPhrase>> by_pinyin_;
    std::filesystem::file_time_type write_time_{};
    bool has_write_time_ = false;
};

}  // namespace localpinyin
