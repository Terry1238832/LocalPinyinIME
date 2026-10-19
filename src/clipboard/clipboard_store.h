#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace localpinyin {

// The clipboard history the tool panel shows, in <data>\clipboard:
// history.jsonl (one JSON object per copy, newest first), <id>.png for each
// picture and <id>.txt for texts longer than the preview kept in the index.
// LocalPinyinClipboard.exe adds copies; the settings bridge pastes and
// deletes them. Every change reads the index, changes it and writes it back
// under a lock the two share, so neither keeps a stale copy.
struct ClipboardItem {
    std::string id;             // 16 hex digits
    bool image = false;
    long long time = 0;         // milliseconds since the epoch
    std::wstring text;          // a text copy, up to kPreviewLength
    bool long_text = false;     // the whole text is in <id>.txt
    std::wstring ocr;           // words read from a picture
    bool ocr_done = false;
    int width = 0;
    int height = 0;
    std::string hash;           // of the text or the picture's pixels
};

class ClipboardStore {
public:
    static constexpr size_t kPreviewLength = 1000;
    static constexpr size_t kMaxTextLength = 1024 * 1024;

    explicit ClipboardStore(std::wstring directory);
    [[nodiscard]] const std::wstring& directory() const noexcept { return directory_; }
    [[nodiscard]] std::wstring index_path() const { return directory_ + L"\\history.jsonl"; }
    [[nodiscard]] std::wstring picture_path(const std::string& id) const;
    [[nodiscard]] std::wstring text_path(const std::string& id) const;

    [[nodiscard]] std::vector<ClipboardItem> read() const;
    // The whole text of a text copy.
    [[nodiscard]] bool full_text(const std::string& id, std::wstring* text) const;
    // A copy already there (same hash) moves to the top instead.
    bool add_text(const std::wstring& text, long long now_ms, std::string* id = nullptr) const;
    bool add_picture(const std::vector<std::uint8_t>& png, int width, int height, const std::string& hash,
                     long long now_ms, std::string* id = nullptr) const;
    bool set_ocr(const std::string& id, const std::wstring& text) const;
    bool remove(const std::string& id) const;
    bool clear() const;
    // Removes copies older than `days` (0 keeps everything); returns how many.
    size_t prune(unsigned long days, long long now_ms) const;
    // Puts `source` in place of the index (an import), under the lock.
    bool replace_index(const std::wstring& source) const;

    static std::string hash_bytes(const void* data, size_t size);
    static bool valid_id(const std::string& id);
    static long long now_ms();

private:
    bool write(const std::vector<ClipboardItem>& items) const;
    std::wstring directory_;
};

// One index line, and back; exposed for tests.
std::string clipboard_item_json(const ClipboardItem& item);
bool parse_clipboard_item(const std::string& line, ClipboardItem* item);

}  // namespace localpinyin
