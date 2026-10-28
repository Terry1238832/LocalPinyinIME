#include "user_learning.h"

#include "../common/utf_utils.h"

#include <windows.h>

#include <algorithm>
#include <charconv>
#include <ctime>
#include <fstream>
#include <sstream>
#include <vector>

namespace localpinyin {
namespace {

constexpr size_t kMaxLearningRecords = UserLearning::kCapacity;
constexpr int kMaxSelectionCount = 100000;

std::wstring key_for(const std::wstring& pinyin, const std::wstring& word) {
    return pinyin + L"\t" + word;
}

constexpr wchar_t kContextMark = L'~';

// Plain pinyin, or contextual: pinyin, the mark, then one character.
bool valid_selection(const std::wstring& pinyin, const std::wstring& word) {
    const auto mark = pinyin.find(kContextMark);
    const auto keys = pinyin.substr(0, mark);
    const bool context_ok = mark == std::wstring::npos ||
                            (mark + 2 == pinyin.size() && pinyin[mark + 1] >= 0x3400 && pinyin[mark + 1] <= 0x9FFF);
    return !keys.empty() && keys.size() <= 256 && context_ok && !word.empty() && word.size() <= 128 &&
           keys.find_first_not_of(L"abcdefghijklmnopqrstuvwxyz0123456789") == std::wstring::npos &&
           word.find_first_of(L"\t\r\n") == std::wstring::npos;
}

bool contextual(const std::wstring& key, size_t tab) noexcept {
    return key.find(kContextMark) < tab;
}

template<typename T>
bool parse_number(const std::wstring& text, T& value) {
    const auto narrow = wide_to_utf8(text);
    const auto [end, error] = std::from_chars(narrow.data(), narrow.data() + narrow.size(), value);
    return error == std::errc{} && end == narrow.data() + narrow.size();
}

// TSF loads the engine into multiple applications. Serialize updates per file
// so selecting a word in one application does not overwrite another's history.
class LearningFileLock {
public:
    explicit LearningFileLock(const std::wstring& path) {
        unsigned long long hash = 1469598103934665603ULL;
        for (wchar_t ch : path) {
            if (ch >= L'A' && ch <= L'Z') ch = ch - L'A' + L'a';
            hash = (hash ^ static_cast<unsigned long long>(ch)) * 1099511628211ULL;
        }
        const std::wstring name = L"Local\\LocalPinyinIME.Learning." + std::to_wstring(hash);
        handle_ = CreateMutexW(nullptr, FALSE, name.c_str());
        if (handle_) {
            const DWORD result = WaitForSingleObject(handle_, 200);
            acquired_ = result == WAIT_OBJECT_0 || result == WAIT_ABANDONED;
        }
    }
    ~LearningFileLock() {
        if (acquired_) ReleaseMutex(handle_);
        if (handle_) CloseHandle(handle_);
    }
    bool acquired() const { return acquired_; }
private:
    HANDLE handle_ = nullptr;
    bool acquired_ = false;
};

}  // namespace

bool UserLearning::open(const std::wstring& storage_path) {
    storage_path_ = storage_path;
    has_write_time_ = false;
    if (storage_path_.empty()) { records_.clear(); return false; }
    LearningFileLock lock(storage_path_);
    if (!lock.acquired()) return false;
    return load();
}

int UserLearning::frequency(const std::wstring& pinyin, const std::wstring& word) const {
    const auto it = records_.find(key_for(pinyin, word));
    return it == records_.end() ? 0 : it->second.selection_count;
}

long long UserLearning::last_selected(const std::wstring& pinyin, const std::wstring& word) const {
    const auto it = records_.find(key_for(pinyin, word));
    return it == records_.end() ? 0 : it->second.last_selected_time;
}

long long UserLearning::stability(const std::wstring& pinyin, const std::wstring& word) const {
    const auto it = records_.find(key_for(pinyin, word));
    return it == records_.end() ? 0 : it->second.stability_seconds;
}

int UserLearning::faded_steps(const std::wstring& pinyin, const std::wstring& word, long long now) const {
    const auto it = records_.find(key_for(pinyin, word));
    if (it == records_.end() || it->second.selection_count <= 0) return -1;
    const long long period = std::max(1LL, it->second.stability_seconds);
    const long long steps = std::max(0LL, now - it->second.last_selected_time) / period;
    return steps >= kFadeSteps ? -1 : static_cast<int>(steps);
}

std::wstring UserLearning::contextual_key(const std::wstring& pinyin, wchar_t before) {
    return pinyin + kContextMark + before;
}

std::vector<std::pair<std::wstring, int>> UserLearning::selected_texts() const {
    std::vector<std::pair<std::wstring, int>> result;
    result.reserve(records_.size());
    for (const auto& [key, record] : records_) {
        const auto tab = key.find(L'\t');
        if (tab != std::wstring::npos && !contextual(key, tab)) result.emplace_back(key.substr(tab + 1), record.selection_count);
    }
    return result;
}

const std::vector<std::wstring>& UserLearning::words_for(const std::wstring& pinyin) const {
    static const std::vector<std::wstring> none;
    const auto found = words_by_pinyin_.find(pinyin);
    return found == words_by_pinyin_.end() ? none : found->second;
}

std::vector<std::wstring> UserLearning::phrase_keys_at(const std::wstring& input, size_t offset) const {
    std::vector<std::wstring> keys;
    if (phrase_keys_.empty() || offset >= input.size()) return keys;
    for (size_t length = std::min(longest_phrase_key_, input.size() - offset); length > 0; --length) {
        std::wstring key = input.substr(offset, length);
        if (phrase_keys_.contains(key)) keys.push_back(std::move(key));
    }
    return keys;
}

void UserLearning::reindex() {
    words_by_pinyin_.clear();
    phrase_keys_.clear();
    longest_phrase_key_ = 0;
    for (const auto& [key, record] : records_) {
        const auto tab = key.find(L'\t');
        if (tab == std::wstring::npos || contextual(key, tab)) continue;
        auto pinyin = key.substr(0, tab);
        const auto word = key.substr(tab + 1);
        // Two or more characters (a surrogate pair is one).
        const auto characters = std::count_if(word.begin(), word.end(), [](wchar_t ch) { return ch < 0xDC00 || ch > 0xDFFF; });
        if (characters >= 2) {
            longest_phrase_key_ = std::max(longest_phrase_key_, pinyin.size());
            phrase_keys_[pinyin] = true;
        }
        words_by_pinyin_[std::move(pinyin)].push_back(word);
    }
}

bool UserLearning::record_selection(const std::wstring& pinyin, const std::wstring& word, long long now) {
    if (storage_path_.empty() || !valid_selection(pinyin, word)) return false;
    LearningFileLock lock(storage_path_);
    if (!lock.acquired() || !load()) return false;
    if (now <= 0) now = static_cast<long long>(std::time(nullptr));
    auto& record = records_[key_for(pinyin, word)];
    if (record.selection_count <= 0 || record.stability_seconds <= 0) {
        record.stability_seconds = kFirstStabilitySeconds;
    } else if (now - record.last_selected_time >= kSpacedGapSeconds) {
        // Chosen again after a break: remembered longer from now on. Picks
        // in one sitting only renew it.
        record.stability_seconds = std::min(kMaxStabilitySeconds,
            static_cast<long long>(static_cast<double>(record.stability_seconds) * kSpacedGrowth));
    }
    record.selection_count = std::min(kMaxSelectionCount, record.selection_count + 1);
    record.last_selected_time = now;
    prune();
    reindex();
    const bool saved = flush();
    if (saved) remember_write_time();
    return saved;
}

bool UserLearning::clear() {
    if (storage_path_.empty()) return false;
    LearningFileLock lock(storage_path_);
    if (!lock.acquired()) return false;
    records_.clear();
    reindex();
    ++revision_;
    const bool saved = flush();
    if (saved) remember_write_time();
    return saved;
}

std::vector<UserLearning::Entry> UserLearning::entries() const {
    std::vector<Entry> list;
    list.reserve(records_.size());
    for (const auto& [key, record] : records_) {
        const auto tab = key.find(L'\t');
        if (tab != std::wstring::npos && !contextual(key, tab)) list.push_back(Entry{key.substr(0, tab), key.substr(tab + 1), record.selection_count, record.last_selected_time});
    }
    std::sort(list.begin(), list.end(), [](const Entry& left, const Entry& right) {
        return left.last_selected != right.last_selected ? left.last_selected > right.last_selected : left.pinyin < right.pinyin;
    });
    return list;
}

size_t UserLearning::forget(const std::wstring& word, const std::wstring& pinyin) {
    if (storage_path_.empty() || word.empty()) return 0;
    LearningFileLock lock(storage_path_);
    if (!lock.acquired() || !load()) return 0;
    size_t forgotten = 0;
    for (auto it = records_.begin(); it != records_.end();) {
        const auto tab = it->first.find(L'\t');
        // A contextual record goes with its plain pinyin.
        const auto keys_end = std::min(tab, it->first.find(kContextMark));
        const bool match = tab != std::wstring::npos && it->first.compare(tab + 1, std::wstring::npos, word) == 0 &&
                           (pinyin.empty() || it->first.compare(0, keys_end, pinyin) == 0 && keys_end == pinyin.size());
        if (match) { it = records_.erase(it); ++forgotten; }
        else ++it;
    }
    if (!forgotten) return 0;
    reindex();
    ++revision_;
    if (flush()) remember_write_time();
    return forgotten;
}

void UserLearning::remember_write_time() {
    std::error_code error;
    write_time_ = std::filesystem::last_write_time(std::filesystem::path(storage_path_), error);
    has_write_time_ = !error;
}

void UserLearning::refresh_if_changed() {
    if (storage_path_.empty()) return;
    std::error_code error;
    const auto current = std::filesystem::last_write_time(std::filesystem::path(storage_path_), error);
    if ((error && has_write_time_) || (!error && (!has_write_time_ || current != write_time_))) {
        LearningFileLock lock(storage_path_);
        if (lock.acquired()) load();
    }
}

void UserLearning::prune() {
    // Plain and contextual records each keep up to the capacity, so the
    // settings app's count (plain choices) is measured against its own limit.
    if (records_.size() <= kMaxLearningRecords) return;
    std::vector<std::pair<long long, std::wstring>> plain, in_context;
    for (const auto& [key, record] : records_)
        (contextual(key, key.find(L'\t')) ? in_context : plain).emplace_back(record.last_selected_time, key);
    for (auto* by_age : {&plain, &in_context}) {
        if (by_age->size() <= kMaxLearningRecords) continue;
        std::sort(by_age->begin(), by_age->end());
        for (size_t i = 0; i < by_age->size() - kMaxLearningRecords; ++i) records_.erase((*by_age)[i].second);
    }
}

bool UserLearning::load() {
    records_.clear();
    ++revision_;
    std::error_code size_error;
    const auto size = std::filesystem::file_size(std::filesystem::path(storage_path_), size_error);
    if (!size_error && size > 2 * 1024 * 1024) { remember_write_time(); return true; }
    std::ifstream file(std::filesystem::path(storage_path_), std::ios::binary);
    if (!file) {
        remember_write_time();
        std::error_code error;
        return !std::filesystem::exists(std::filesystem::path(storage_path_), error) && !error;
    }

    std::string line;
    size_t rows = 0;
    while (rows++ < kMaxLearningRecords * 4 && std::getline(file, line)) {
        if (line.size() > 2048) continue;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::wstring wide = utf8_to_wide(line);
        const auto first = wide.find(L'\t');
        if (first == std::wstring::npos) continue;
        const auto second = wide.find(L'\t', first + 1);
        if (second == std::wstring::npos) continue;
        const auto pinyin = wide.substr(0, first);
        const auto word = wide.substr(first + 1, second - first - 1);
        if (!valid_selection(pinyin, word)) continue;
        const auto third = wide.find(L'\t', second + 1);
        const auto fourth = third == std::wstring::npos ? std::wstring::npos : wide.find(L'\t', third + 1);
        int count = 0;
        long long time = 0;
        long long stability = 0;
        if (!parse_number(wide.substr(second + 1, third == std::wstring::npos ? std::wstring::npos : third - second - 1), count) || count <= 0) continue;
        if (third != std::wstring::npos &&
            (!parse_number(wide.substr(third + 1, fourth == std::wstring::npos ? std::wstring::npos : fourth - third - 1), time) ||
             time < 0)) continue;
        if (fourth != std::wstring::npos && (!parse_number(wide.substr(fourth + 1), stability) || stability < 0)) stability = 0;
        count = std::min(count, kMaxSelectionCount);
        if (stability <= 0) {
            // Records from before 0.18.2: as if each of up to two repeat
            // choices came after a break.
            stability = kFirstStabilitySeconds;
            for (int repeat = 1; repeat < std::min(count, 3); ++repeat)
                stability = static_cast<long long>(static_cast<double>(stability) * kSpacedGrowth);
        }
        records_[key_for(pinyin, word)] = Record{count, time, std::min(stability, kMaxStabilitySeconds)};
    }
    prune();
    reindex();
    remember_write_time();
    return true;
}

bool UserLearning::flush() const {
    const std::filesystem::path path(storage_path_);
    std::error_code error;
    if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path(), error);
    if (error) return false;
    const std::wstring temporary = storage_path_ + L".tmp";
    {
        std::ofstream file(std::filesystem::path(temporary), std::ios::binary | std::ios::trunc);
        if (!file) return false;
        for (const auto& [key, record] : records_) {
            file << wide_to_utf8(key) << '\t' << record.selection_count << '\t' << record.last_selected_time << '\t'
                 << record.stability_seconds << '\n';
        }
        file.flush();
        if (!file) {
            file.close();
            DeleteFileW(temporary.c_str());
            return false;
        }
    }
    // Antivirus and the search indexer briefly open freshly written files;
    // replacing the target then fails with a sharing error, so retry shortly.
    for (int attempt = 0; attempt < 10; ++attempt) {
        if (MoveFileExW(temporary.c_str(), storage_path_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
        const DWORD move_error = GetLastError();
        if (move_error != ERROR_ACCESS_DENIED && move_error != ERROR_SHARING_VIOLATION &&
            move_error != ERROR_LOCK_VIOLATION) break;
        Sleep(15);
    }
    DeleteFileW(temporary.c_str());
    return false;
}

}  // namespace localpinyin
