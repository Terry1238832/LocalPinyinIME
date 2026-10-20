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

    // A chosen word fades the way a memory does. It ranks first for one
    // stability period after it was chosen, then one place lower for each
    // further period, and after kFadeSteps periods as if never chosen.
    // Choosing it again puts it first again; choosing it again after a
    // break (kSpacedGapSeconds) also makes it fade more slowly: the first
    // choice lasts 3 days, each spaced one multiplies that by 2.5, up to
    // half a year.
    static constexpr long long kFirstStabilitySeconds = 3LL * 24 * 3600;
    static constexpr long long kSpacedGapSeconds = 12LL * 3600;
    static constexpr long long kMaxStabilitySeconds = 180LL * 24 * 3600;
    static constexpr double kSpacedGrowth = 2.5;
    static constexpr int kFadeSteps = 4;

    bool open(const std::wstring& storage_path);
    int frequency(const std::wstring& pinyin, const std::wstring& word) const;
    // Seconds since the epoch of the latest selection, or 0.
    long long last_selected(const std::wstring& pinyin, const std::wstring& word) const;
    // The current stability period in seconds, or 0 when never chosen.
    long long stability(const std::wstring& pinyin, const std::wstring& word) const;
    // How many places the word has faded at `now` (0: still first), or -1
    // when it was never chosen or has faded out.
    int faded_steps(const std::wstring& pinyin, const std::wstring& word, long long now) const;
    // `now` (seconds since the epoch) is for tests; 0 is the current time.
    bool record_selection(const std::wstring& pinyin, const std::wstring& word, long long now = 0);
    bool clear();
    // One learned choice, for the settings app's list.
    struct Entry {
        std::wstring pinyin;
        std::wstring word;
        int count = 0;
        long long last_selected = 0;
    };
    // Every learned choice, the latest first.
    [[nodiscard]] std::vector<Entry> entries() const;
    // Forgets `word` as chosen for `pinyin`, or for any pinyin when `pinyin`
    // is empty. Returns how many choices were forgotten.
    size_t forget(const std::wstring& word, const std::wstring& pinyin = {});
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
        long long stability_seconds = 0;
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
