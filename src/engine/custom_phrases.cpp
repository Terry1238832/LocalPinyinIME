#include "custom_phrases.h"

#include "../common/utf_utils.h"

#include <windows.h>

#include <algorithm>
#include <charconv>
#include <ctime>
#include <fstream>

namespace localpinyin {
namespace {

// Several applications load the input method, and the settings app edits
// the file too: changes are serialized per file.
class PhraseFileLock {
public:
    explicit PhraseFileLock(const std::wstring& path) {
        unsigned long long hash = 1469598103934665603ULL;
        for (wchar_t ch : path) {
            if (ch >= L'A' && ch <= L'Z') ch = ch - L'A' + L'a';
            hash = (hash ^ static_cast<unsigned long long>(ch)) * 1099511628211ULL;
        }
        handle_ = CreateMutexW(nullptr, FALSE, (L"Local\\LocalPinyinIME.Phrases." + std::to_wstring(hash)).c_str());
        if (handle_) {
            const DWORD result = WaitForSingleObject(handle_, 500);
            acquired_ = result == WAIT_OBJECT_0 || result == WAIT_ABANDONED;
        }
    }
    ~PhraseFileLock() {
        if (acquired_) ReleaseMutex(handle_);
        if (handle_) CloseHandle(handle_);
    }
    [[nodiscard]] bool acquired() const noexcept { return acquired_; }
private:
    HANDLE handle_ = nullptr;
    bool acquired_ = false;
};

bool write_all(const std::wstring& path, const std::vector<CustomPhrase>& entries) {
    const std::filesystem::path target(path);
    std::error_code error;
    if (!target.parent_path().empty()) std::filesystem::create_directories(target.parent_path(), error);
    const std::wstring temporary = path + L".tmp";
    {
        std::ofstream file(std::filesystem::path(temporary), std::ios::binary | std::ios::trunc);
        if (!file) return false;
        for (const auto& entry : entries) {
            file << wide_to_utf8(entry.pinyin) << '\t' << wide_to_utf8(entry.text) << '\t' << wide_to_utf8(entry.label)
                 << '\t' << entry.created << '\n';
        }
        file.flush();
        if (!file) return false;
    }
    for (int attempt = 0; attempt < 10; ++attempt) {
        if (MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
        Sleep(15);
    }
    DeleteFileW(temporary.c_str());
    return false;
}

}  // namespace

bool CustomPhrases::valid(const CustomPhrase& phrase) {
    const auto clean = [](const std::wstring& text, size_t limit) {
        return !text.empty() && text.size() <= limit && text.find_first_of(L"\t\r\n") == std::wstring::npos;
    };
    return clean(phrase.pinyin, 64) && clean(phrase.text, 64) && phrase.label.size() <= 32 &&
           phrase.label.find_first_of(L"\t\r\n") == std::wstring::npos &&
           phrase.pinyin.find_first_not_of(L"abcdefghijklmnopqrstuvwxyz") == std::wstring::npos;
}

std::vector<CustomPhrase> CustomPhrases::read(const std::wstring& path) {
    std::vector<CustomPhrase> entries;
    std::ifstream file(std::filesystem::path(path), std::ios::binary);
    if (!file) return entries;
    std::string line;
    while (entries.size() < kCapacity && std::getline(file, line)) {
        if (line.size() > 1024) continue;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::wstring wide = utf8_to_wide(line);
        std::vector<std::wstring> fields;
        size_t start = 0;
        for (size_t tab; (tab = wide.find(L'\t', start)) != std::wstring::npos; start = tab + 1)
            fields.push_back(wide.substr(start, tab - start));
        fields.push_back(wide.substr(start));
        if (fields.size() < 2) continue;
        CustomPhrase phrase{fields[0], fields[1], fields.size() > 2 ? fields[2] : L"", 0};
        if (fields.size() > 3) {
            const auto narrow = wide_to_utf8(fields[3]);
            std::from_chars(narrow.data(), narrow.data() + narrow.size(), phrase.created);
        }
        if (valid(phrase)) entries.push_back(std::move(phrase));
    }
    return entries;
}

bool CustomPhrases::add(const std::wstring& path, CustomPhrase phrase) {
    if (phrase.created <= 0) phrase.created = static_cast<long long>(std::time(nullptr));
    if (!valid(phrase)) return false;
    PhraseFileLock lock(path);
    if (!lock.acquired()) return false;
    auto entries = read(path);
    const auto same = std::find_if(entries.begin(), entries.end(), [&](const CustomPhrase& entry) {
        return entry.pinyin == phrase.pinyin && entry.text == phrase.text;
    });
    if (same != entries.end()) {
        same->label = phrase.label;
        return write_all(path, entries);
    }
    if (entries.size() >= kCapacity) return false;
    entries.push_back(std::move(phrase));
    return write_all(path, entries);
}

bool CustomPhrases::remove(const std::wstring& path, const std::wstring& pinyin, const std::wstring& text) {
    PhraseFileLock lock(path);
    if (!lock.acquired()) return false;
    auto entries = read(path);
    const auto before = entries.size();
    entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const CustomPhrase& entry) {
        return entry.pinyin == pinyin && entry.text == text;
    }), entries.end());
    return entries.size() == before || write_all(path, entries);
}

bool CustomPhrases::remember(const std::wstring& path, CustomPhrase phrase, size_t capacity) {
    phrase.created = static_cast<long long>(std::time(nullptr));
    if (!valid(phrase)) return false;
    PhraseFileLock lock(path);
    if (!lock.acquired()) return false;
    auto entries = read(path);
    entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const CustomPhrase& entry) {
        return entry.pinyin == phrase.pinyin && entry.text == phrase.text;
    }), entries.end());
    entries.push_back(std::move(phrase));
    if (entries.size() > capacity) {
        std::stable_sort(entries.begin(), entries.end(), [](const CustomPhrase& left, const CustomPhrase& right) {
            return left.created > right.created;
        });
        entries.resize(capacity);
    }
    return write_all(path, entries);
}

void CustomPhrases::open(const std::wstring& path) {
    path_ = path;
    has_write_time_ = false;
    load();
}

void CustomPhrases::load() {
    entries_.clear();
    by_pinyin_.clear();
    if (path_.empty()) return;
    {
        PhraseFileLock lock(path_);
        entries_ = read(path_);
    }
    for (const auto& entry : entries_) by_pinyin_[entry.pinyin].push_back(entry);
    std::error_code error;
    write_time_ = std::filesystem::last_write_time(std::filesystem::path(path_), error);
    has_write_time_ = !error;
}

void CustomPhrases::refresh_if_changed() {
    if (path_.empty()) return;
    std::error_code error;
    const auto current = std::filesystem::last_write_time(std::filesystem::path(path_), error);
    if ((error && has_write_time_) || (!error && (!has_write_time_ || current != write_time_))) load();
}

const std::vector<CustomPhrase>& CustomPhrases::for_pinyin(const std::wstring& pinyin) const {
    static const std::vector<CustomPhrase> none;
    const auto found = by_pinyin_.find(pinyin);
    return found == by_pinyin_.end() ? none : found->second;
}

}  // namespace localpinyin
