#include "clipboard_store.h"

#include "../common/utf_utils.h"

#include <windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>

namespace localpinyin {
namespace {

class ClipboardLock {
public:
    explicit ClipboardLock(const std::wstring& directory) {
        unsigned long long hash = 1469598103934665603ULL;
        for (wchar_t ch : directory) {
            if (ch >= L'A' && ch <= L'Z') ch = ch - L'A' + L'a';
            hash = (hash ^ static_cast<unsigned long long>(ch)) * 1099511628211ULL;
        }
        handle_ = CreateMutexW(nullptr, FALSE, (L"Local\\LocalPinyinIME.Clipboard." + std::to_wstring(hash)).c_str());
        if (handle_) {
            const DWORD result = WaitForSingleObject(handle_, 3000);
            acquired_ = result == WAIT_OBJECT_0 || result == WAIT_ABANDONED;
        }
    }
    ~ClipboardLock() {
        if (acquired_) ReleaseMutex(handle_);
        if (handle_) CloseHandle(handle_);
    }
    ClipboardLock(const ClipboardLock&) = delete;
    ClipboardLock& operator=(const ClipboardLock&) = delete;
    [[nodiscard]] bool acquired() const { return acquired_; }

private:
    HANDLE handle_ = nullptr;
    bool acquired_ = false;
};

// Lone surrogates (possible in copied text) become U+FFFD so the text is
// valid UTF-8 on disk.
std::string to_utf8(const std::wstring& text) {
    std::wstring clean;
    clean.reserve(text.size());
    for (size_t index = 0; index < text.size(); ++index) {
        const wchar_t ch = text[index];
        if (ch >= 0xD800 && ch <= 0xDBFF && index + 1 < text.size() && text[index + 1] >= 0xDC00 && text[index + 1] <= 0xDFFF) {
            clean += ch;
            clean += text[++index];
        } else if (ch >= 0xD800 && ch <= 0xDFFF) clean += L'\xFFFD';
        else clean += ch;
    }
    return wide_to_utf8(clean);
}

void put_string(std::string& out, const std::wstring& value) {
    static constexpr char hex[] = "0123456789abcdef";
    out += '"';
    for (unsigned char ch : to_utf8(value)) {
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (ch < 0x20) { out += "\\u00"; out += hex[ch >> 4]; out += hex[ch & 15]; }
                else out += static_cast<char>(ch);
        }
    }
    out += '"';
}

// A flat JSON object of strings, numbers and booleans.
class Reader {
public:
    explicit Reader(const std::string& text) : text_(text) {}
    bool object(ClipboardItem* item) {
        skip();
        if (!eat('{')) return false;
        skip();
        if (eat('}')) return true;
        while (true) {
            std::string key;
            if (!string(&key)) return false;
            skip();
            if (!eat(':')) return false;
            skip();
            if (!value(key, item)) return false;
            skip();
            if (eat(',')) { skip(); continue; }
            return eat('}');
        }
    }

private:
    bool value(const std::string& key, ClipboardItem* item) {
        if (peek() == '"') {
            std::string value;
            if (!string(&value)) return false;
            if (key == "id") item->id = value;
            else if (key == "kind") item->image = value == "image";
            else if (key == "text") item->text = utf8_to_wide(value);
            else if (key == "ocr") item->ocr = utf8_to_wide(value);
            else if (key == "hash") item->hash = value;
            return true;
        }
        if (text_.compare(at_, 4, "true") == 0 || text_.compare(at_, 5, "false") == 0) {
            const bool on = text_[at_] == 't';
            at_ += on ? 4 : 5;
            if (key == "long") item->long_text = on;
            else if (key == "ocrDone") item->ocr_done = on;
            return true;
        }
        const size_t start = at_;
        while (at_ < text_.size() && (std::isdigit(static_cast<unsigned char>(text_[at_])) || text_[at_] == '-')) ++at_;
        if (start == at_) return false;
        const long long number = std::stoll(text_.substr(start, at_ - start));
        if (key == "time") item->time = number;
        else if (key == "w") item->width = static_cast<int>(number);
        else if (key == "h") item->height = static_cast<int>(number);
        return true;
    }
    bool string(std::string* out) {
        if (!eat('"')) return false;
        while (at_ < text_.size()) {
            const char ch = text_[at_++];
            if (ch == '"') return true;
            if (ch != '\\') { *out += ch; continue; }
            if (at_ >= text_.size()) return false;
            const char escape = text_[at_++];
            switch (escape) {
                case 'n': *out += '\n'; break;
                case 'r': *out += '\r'; break;
                case 't': *out += '\t'; break;
                case 'b': *out += '\b'; break;
                case 'f': *out += '\f'; break;
                case 'u': {
                    if (at_ + 4 > text_.size()) return false;
                    unsigned code = std::stoul(text_.substr(at_, 4), nullptr, 16);
                    at_ += 4;
                    if (code >= 0xD800 && code <= 0xDBFF && text_.compare(at_, 2, "\\u") == 0 && at_ + 6 <= text_.size()) {
                        const unsigned low = std::stoul(text_.substr(at_ + 2, 4), nullptr, 16);
                        if (low >= 0xDC00 && low <= 0xDFFF) { code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00); at_ += 6; }
                    }
                    const std::wstring wide = code > 0xFFFF
                        ? std::wstring{static_cast<wchar_t>(0xD800 + ((code - 0x10000) >> 10)), static_cast<wchar_t>(0xDC00 + ((code - 0x10000) & 0x3FF))}
                        : std::wstring(1, static_cast<wchar_t>(code));
                    *out += to_utf8(wide);
                    break;
                }
                default: *out += escape;
            }
        }
        return false;
    }
    void skip() { while (at_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[at_]))) ++at_; }
    bool eat(char ch) { if (at_ < text_.size() && text_[at_] == ch) { ++at_; return true; } return false; }
    char peek() const { return at_ < text_.size() ? text_[at_] : '\0'; }

    const std::string& text_;
    size_t at_ = 0;
};

std::string random_id() {
    std::uint8_t bytes[8]{};
    if (!BCRYPT_SUCCESS(BCryptGenRandom(nullptr, bytes, sizeof(bytes), BCRYPT_USE_SYSTEM_PREFERRED_RNG))) {
        const auto tick = static_cast<unsigned long long>(std::chrono::steady_clock::now().time_since_epoch().count());
        std::memcpy(bytes, &tick, sizeof(bytes));
    }
    static constexpr char hex[] = "0123456789abcdef";
    std::string id;
    for (auto byte : bytes) { id += hex[byte >> 4]; id += hex[byte & 15]; }
    return id;
}

bool write_file(const std::wstring& path, const std::string& data) {
    const std::wstring temporary = path + L".tmp";
    {
        std::ofstream file(std::filesystem::path(temporary), std::ios::binary | std::ios::trunc);
        if (!file) return false;
        file.write(data.data(), static_cast<std::streamsize>(data.size()));
        if (!file) return false;
    }
    return MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
}

void remove_files(const ClipboardStore& store, const ClipboardItem& item) {
    std::error_code error;
    if (item.image) std::filesystem::remove(std::filesystem::path(store.picture_path(item.id)), error);
    if (item.long_text) std::filesystem::remove(std::filesystem::path(store.text_path(item.id)), error);
}

}  // namespace

std::string clipboard_item_json(const ClipboardItem& item) {
    std::string out = "{\"id\":\"" + item.id + "\",\"kind\":\"" + (item.image ? "image" : "text") + "\",\"time\":" +
                      std::to_string(item.time);
    if (!item.image) {
        out += ",\"text\":";
        put_string(out, item.text);
        if (item.long_text) out += ",\"long\":true";
    } else {
        out += ",\"w\":" + std::to_string(item.width) + ",\"h\":" + std::to_string(item.height);
        if (!item.ocr.empty()) { out += ",\"ocr\":"; put_string(out, item.ocr); }
        if (item.ocr_done) out += ",\"ocrDone\":true";
    }
    out += ",\"hash\":\"" + item.hash + "\"}";
    return out;
}

bool parse_clipboard_item(const std::string& line, ClipboardItem* item) {
    *item = {};
    try {
        Reader reader(line);
        return reader.object(item) && ClipboardStore::valid_id(item->id) && item->time > 0;
    } catch (...) {
        return false;
    }
}

ClipboardStore::ClipboardStore(std::wstring directory) : directory_(std::move(directory)) {}

std::wstring ClipboardStore::picture_path(const std::string& id) const { return directory_ + L"\\" + utf8_to_wide(id) + L".png"; }
std::wstring ClipboardStore::text_path(const std::string& id) const { return directory_ + L"\\" + utf8_to_wide(id) + L".txt"; }

bool ClipboardStore::valid_id(const std::string& id) {
    return id.size() == 16 && std::all_of(id.begin(), id.end(), [](char ch) { return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'); });
}

long long ClipboardStore::now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string ClipboardStore::hash_bytes(const void* data, size_t size) {
    unsigned long long hash = 1469598103934665603ULL;
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (size_t index = 0; index < size; ++index) hash = (hash ^ bytes[index]) * 1099511628211ULL;
    char text[17]{};
    std::snprintf(text, sizeof(text), "%016llx", hash);
    return text;
}

std::vector<ClipboardItem> ClipboardStore::read() const {
    std::vector<ClipboardItem> items;
    std::ifstream file(std::filesystem::path(index_path()), std::ios::binary);
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        ClipboardItem item;
        if (!line.empty() && parse_clipboard_item(line, &item)) items.push_back(std::move(item));
    }
    std::stable_sort(items.begin(), items.end(), [](const ClipboardItem& left, const ClipboardItem& right) { return left.time > right.time; });
    return items;
}

bool ClipboardStore::write(const std::vector<ClipboardItem>& items) const {
    std::error_code error;
    std::filesystem::create_directories(std::filesystem::path(directory_), error);
    std::string data;
    for (const auto& item : items) { data += clipboard_item_json(item); data += '\n'; }
    return write_file(index_path(), data);
}

bool ClipboardStore::full_text(const std::string& id, std::wstring* text) const {
    for (const auto& item : read()) {
        if (item.id != id || item.image) continue;
        if (!item.long_text) { *text = item.text; return true; }
        std::ifstream file(std::filesystem::path(text_path(id)), std::ios::binary);
        const std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        *text = utf8_to_wide(data);
        return !text->empty();
    }
    return false;
}

bool ClipboardStore::add_text(const std::wstring& text, long long now, std::string* id) const {
    if (text.empty() || text.size() > kMaxTextLength) return false;
    const std::string utf8 = to_utf8(text);
    const std::string hash = hash_bytes(utf8.data(), utf8.size());
    ClipboardLock lock(directory_);
    if (!lock.acquired()) return false;
    auto items = read();
    const auto same = std::find_if(items.begin(), items.end(), [&](const ClipboardItem& item) { return !item.image && item.hash == hash; });
    if (same != items.end()) {
        same->time = now;
        if (id) *id = same->id;
        std::rotate(items.begin(), same, same + 1);
        return write(items);
    }
    ClipboardItem item;
    item.id = random_id();
    item.time = now;
    item.hash = hash;
    item.long_text = text.size() > kPreviewLength;
    item.text = item.long_text ? text.substr(0, kPreviewLength) : text;
    std::error_code error;
    std::filesystem::create_directories(std::filesystem::path(directory_), error);
    if (item.long_text && !write_file(text_path(item.id), utf8)) return false;
    if (id) *id = item.id;
    items.insert(items.begin(), std::move(item));
    return write(items);
}

bool ClipboardStore::add_picture(const std::vector<std::uint8_t>& png, int width, int height, const std::string& hash,
                                 long long now, std::string* id) const {
    if (png.empty()) return false;
    ClipboardLock lock(directory_);
    if (!lock.acquired()) return false;
    auto items = read();
    const auto same = std::find_if(items.begin(), items.end(), [&](const ClipboardItem& item) { return item.image && item.hash == hash; });
    if (same != items.end()) {
        same->time = now;
        if (id) *id = same->id;
        std::rotate(items.begin(), same, same + 1);
        return write(items);
    }
    ClipboardItem item;
    item.id = random_id();
    item.image = true;
    item.time = now;
    item.hash = hash;
    item.width = width;
    item.height = height;
    std::error_code error;
    std::filesystem::create_directories(std::filesystem::path(directory_), error);
    if (!write_file(picture_path(item.id), std::string(png.begin(), png.end()))) return false;
    if (id) *id = item.id;
    items.insert(items.begin(), std::move(item));
    return write(items);
}

bool ClipboardStore::set_ocr(const std::string& id, const std::wstring& text) const {
    ClipboardLock lock(directory_);
    if (!lock.acquired()) return false;
    auto items = read();
    const auto item = std::find_if(items.begin(), items.end(), [&](const ClipboardItem& entry) { return entry.id == id; });
    if (item == items.end()) return true;
    item->ocr = text.substr(0, 4000);
    item->ocr_done = true;
    return write(items);
}

bool ClipboardStore::remove(const std::string& id) const {
    ClipboardLock lock(directory_);
    if (!lock.acquired()) return false;
    auto items = read();
    const auto item = std::find_if(items.begin(), items.end(), [&](const ClipboardItem& entry) { return entry.id == id; });
    if (item == items.end()) return true;
    remove_files(*this, *item);
    items.erase(item);
    return write(items);
}

bool ClipboardStore::clear() const {
    ClipboardLock lock(directory_);
    if (!lock.acquired()) return false;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(directory_), error)) {
        const auto name = entry.path().filename().wstring();
        const auto extension = entry.path().extension().wstring();
        if (name == L"history.jsonl" || extension == L".png" || extension == L".txt" || extension == L".tmp")
            std::filesystem::remove(entry.path(), error);
    }
    return true;
}

size_t ClipboardStore::prune(unsigned long days, long long now) const {
    if (days == 0) return 0;
    ClipboardLock lock(directory_);
    if (!lock.acquired()) return 0;
    auto items = read();
    const long long oldest = now - static_cast<long long>(days) * 24 * 3600 * 1000;
    const auto before = items.size();
    items.erase(std::remove_if(items.begin(), items.end(), [&](const ClipboardItem& item) {
        if (item.time >= oldest) return false;
        remove_files(*this, item);
        return true;
    }), items.end());
    const size_t removed = before - items.size();
    if (removed) write(items);
    return removed;
}

bool ClipboardStore::replace_index(const std::wstring& source) const {
    ClipboardLock lock(directory_);
    if (!lock.acquired()) return false;
    std::error_code error;
    std::filesystem::create_directories(std::filesystem::path(directory_), error);
    return MoveFileExW(source.c_str(), index_path().c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) != FALSE;
}

}  // namespace localpinyin
