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

constexpr size_t kMaxLearningRecords = 2048;
constexpr int kMaxSelectionCount = 100000;

std::wstring key_for(const std::wstring& pinyin, const std::wstring& word) {
    return pinyin + L"\t" + word;
}

bool valid_selection(const std::wstring& pinyin, const std::wstring& word) {
    return !pinyin.empty() && pinyin.size() <= 128 && !word.empty() && word.size() <= 128 &&
           pinyin.find_first_not_of(L"abcdefghijklmnopqrstuvwxyz0123456789") == std::wstring::npos &&
           word.find_first_of(L"\t\r\n") == std::wstring::npos;
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

bool UserLearning::record_selection(const std::wstring& pinyin, const std::wstring& word) {
    if (storage_path_.empty() || !valid_selection(pinyin, word)) return false;
    LearningFileLock lock(storage_path_);
    if (!lock.acquired() || !load()) return false;
    auto& record = records_[key_for(pinyin, word)];
    record.selection_count = std::min(kMaxSelectionCount, record.selection_count + 1);
    record.last_selected_time = static_cast<long long>(std::time(nullptr));
    prune();
    const bool saved = flush();
    if (saved) remember_write_time();
    return saved;
}

bool UserLearning::clear() {
    if (storage_path_.empty()) return false;
    LearningFileLock lock(storage_path_);
    if (!lock.acquired()) return false;
    records_.clear();
    const bool saved = flush();
    if (saved) remember_write_time();
    return saved;
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
    if (records_.size() <= kMaxLearningRecords) return;
    std::vector<std::pair<long long, std::wstring>> by_age;
    by_age.reserve(records_.size());
    for (const auto& [key, record] : records_) by_age.emplace_back(record.last_selected_time, key);
    std::sort(by_age.begin(), by_age.end());
    for (size_t i = 0; i < by_age.size() - kMaxLearningRecords; ++i) records_.erase(by_age[i].second);
}

bool UserLearning::load() {
    records_.clear();
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
        int count = 0;
        long long time = 0;
        if (!parse_number(wide.substr(second + 1, third == std::wstring::npos ? std::wstring::npos : third - second - 1), count) || count <= 0) continue;
        if (third != std::wstring::npos && (!parse_number(wide.substr(third + 1), time) || time < 0)) continue;
        records_[key_for(pinyin, word)] = Record{std::min(count, kMaxSelectionCount), time};
    }
    prune();
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
            file << wide_to_utf8(key) << '\t' << record.selection_count << '\t' << record.last_selected_time << '\n';
        }
        file.flush();
        if (!file) {
            file.close();
            DeleteFileW(temporary.c_str());
            return false;
        }
    }
    if (MoveFileExW(temporary.c_str(), storage_path_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
    DeleteFileW(temporary.c_str());
    return false;
}

}  // namespace localpinyin
