#include "test_common.h"
#include "../src/clipboard/clipboard_store.h"

#include <windows.h>

#include <filesystem>
#include <string>

int main() {
    using localpinyin::ClipboardItem;
    using localpinyin::ClipboardStore;

    // Index lines round-trip, escapes and characters outside the BMP included.
    ClipboardItem item;
    item.id = "0123456789abcdef";
    item.time = 1700000000123;
    item.text = L"第一行\n\"引号\"\t\\ 😀";
    item.hash = "abc";
    ClipboardItem parsed;
    REQUIRE_TRUE(localpinyin::parse_clipboard_item(localpinyin::clipboard_item_json(item), &parsed));
    REQUIRE_EQ(parsed.id, item.id);
    REQUIRE_EQ(parsed.time, item.time);
    REQUIRE_EQ(parsed.text, item.text);
    REQUIRE_TRUE(!parsed.image);
    ClipboardItem picture;
    picture.id = "fedcba9876543210";
    picture.image = true;
    picture.time = 5;
    picture.width = 640;
    picture.height = 480;
    picture.ocr = L"图中文字";
    picture.ocr_done = true;
    REQUIRE_TRUE(localpinyin::parse_clipboard_item(localpinyin::clipboard_item_json(picture), &parsed));
    REQUIRE_TRUE(parsed.image && parsed.ocr_done);
    REQUIRE_EQ(parsed.width, 640);
    REQUIRE_EQ(parsed.ocr, std::wstring(L"图中文字"));
    REQUIRE_TRUE(!localpinyin::parse_clipboard_item("{\"id\":\"short\",\"time\":1}", &parsed));
    REQUIRE_TRUE(!localpinyin::parse_clipboard_item("{\"id\":\"0123456789abcdef\",\"time\":", &parsed));

    wchar_t temp[MAX_PATH]{};
    GetTempPathW(MAX_PATH, temp);
    const auto folder = std::filesystem::path(temp) / (L"lp-clipboard-" + std::to_wstring(GetCurrentProcessId()));
    std::filesystem::remove_all(folder);
    const ClipboardStore store(folder.wstring());

    // A copy made again moves to the top instead of repeating.
    std::string first, again, second;
    REQUIRE_TRUE(store.add_text(L"你好", 1000, &first));
    REQUIRE_TRUE(store.add_text(L"世界", 2000, &second));
    REQUIRE_TRUE(store.add_text(L"你好", 3000, &again));
    REQUIRE_EQ(first, again);
    auto items = store.read();
    REQUIRE_EQ(items.size(), size_t{2});
    REQUIRE_EQ(items[0].id, first);
    REQUIRE_EQ(items[0].time, 3000LL);

    // Long text keeps a preview in the index and the whole text beside it.
    const std::wstring long_text(ClipboardStore::kPreviewLength + 50, L'长');
    std::string long_id;
    REQUIRE_TRUE(store.add_text(long_text, 4000, &long_id));
    items = store.read();
    REQUIRE_TRUE(items[0].long_text);
    REQUIRE_EQ(items[0].text.size(), ClipboardStore::kPreviewLength);
    std::wstring whole;
    REQUIRE_TRUE(store.full_text(long_id, &whole));
    REQUIRE_EQ(whole, long_text);

    // Pictures: same pixels are one copy; OCR text is added later.
    std::string image_id, image_again;
    REQUIRE_TRUE(store.add_picture({1, 2, 3}, 2, 2, "pixels", 5000, &image_id));
    REQUIRE_TRUE(store.add_picture({1, 2, 3}, 2, 2, "pixels", 6000, &image_again));
    REQUIRE_EQ(image_id, image_again);
    REQUIRE_TRUE(std::filesystem::exists(store.picture_path(image_id)));
    REQUIRE_TRUE(store.set_ocr(image_id, L"截图里的字"));
    items = store.read();
    REQUIRE_EQ(items.size(), size_t{4});
    REQUIRE_EQ(items[0].ocr, std::wstring(L"截图里的字"));

    // Retention: copies older than the days kept go, with their files.
    const long long day = 24LL * 3600 * 1000;
    REQUIRE_EQ(store.prune(0, 100 * day), size_t{0});
    REQUIRE_EQ(store.prune(1, 5000 + day + 1), size_t{3});
    items = store.read();
    REQUIRE_EQ(items.size(), size_t{1});
    REQUIRE_EQ(items[0].id, image_id);
    REQUIRE_TRUE(!std::filesystem::exists(store.text_path(long_id)));

    REQUIRE_TRUE(store.remove(image_id));
    REQUIRE_TRUE(!std::filesystem::exists(store.picture_path(image_id)));
    REQUIRE_TRUE(store.read().empty());
    REQUIRE_TRUE(store.add_text(L"再来", 7000));
    REQUIRE_TRUE(store.clear());
    REQUIRE_TRUE(store.read().empty());
    std::filesystem::remove_all(folder);
    return 0;
}
