#pragma once

#include "../clipboard/clipboard_store.h"
#include "../panel/panel_data.h"
#include "../vault/vault.h"

#include <windows.h>

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace localpinyin {

// The small tool panel Tab twice opens at the caret, drawn by the input
// method like its candidate window. The document keeps the focus: while the
// panel is up the input method takes the keys, so everything works from the
// keyboard (and the mouse). This class is the panel's state and key logic;
// ToolPanelWindow draws what view() describes.
enum class PanelPage { Tools = 0, Clipboard, Symbol, Kaomoji, Emoji, Stickers, Records };
constexpr int kPanelPageCount = 7;
const wchar_t* panel_page_label(PanelPage page) noexcept;
// The settings app's id for the page (its large panel opens on it).
const wchar_t* panel_page_id(PanelPage page) noexcept;

enum class PanelFont { Text, Symbol, Emoji, Kaomoji };

struct PanelCell {
    std::wstring text;    // the item, or a list row's title
    std::wstring detail;  // a list row's second part
    std::wstring tag;     // a list row's label: 密码, 地址, 图片
    std::wstring image;   // a picture file to draw (stickers, copied pictures)
    bool favorite = false;
};

struct ToolPanelView {
    PanelPage page = PanelPage::Symbol;
    std::vector<std::wstring> categories;
    int category = -1;
    std::wstring info;  // right of the category bar (a count, a lock state)
    bool grid = true;
    int columns = 10;
    int rows = 4;
    PanelFont font = PanelFont::Symbol;
    std::vector<PanelCell> cells;  // the visible rows, row by row
    int highlight = -1;            // in cells
    int numbered_row = -1;         // grid: the row whose items have number keys
    bool numbered_list = false;    // list: every row has one
    bool more_above = false;
    bool more_below = false;
    std::wstring prompt;  // before the typed text: 搜索, 算式, 输入密码…
    std::wstring input;   // what was typed (dots for a password)
    bool input_active = false;
    std::vector<std::wstring> tray;
    std::wstring message;  // shown when there are no cells, or above them
    std::wstring hint;     // keys at the bottom right
};

// What the input method does after a key or click.
struct PanelAction {
    enum class Kind { None, Redraw, Close, Insert, PasteSticker, PasteClip, CopySecret, Expand };
    Kind kind = Kind::None;
    std::wstring text;     // Insert, CopySecret
    std::wstring file;     // PasteSticker
    std::string clip_id;   // PasteClip
    // Emoji typed from the panel and the names they come up for among candidates.
    std::vector<std::pair<std::wstring, std::vector<std::wstring>>> learn_emoji;
};

struct PanelKey {
    UINT vk = 0;
    wchar_t ch = 0;  // the character the key types, 0 for none
    bool shift = false;
    bool ctrl = false;
};

struct PanelHit {
    enum class Kind { None, Page, Category, Cell, Tray, Expand, Close };
    Kind kind = Kind::None;
    int index = 0;
};

class ToolPanel {
public:
    struct Services {
        std::wstring data_dir;       // %LOCALAPPDATA%\LocalPinyinIME
        std::wstring resource_dir;   // the dictionary folder (panel\*.tsv inside)
        // Chinese words for typed pinyin, best first (the engine's candidates).
        std::function<std::vector<std::wstring>(const std::wstring&)> words;
        // Whether clipboard history is on.
        std::function<bool()> clipboard_on;
        // Seconds since the epoch, for tests.
        std::function<long long()> now;
    };

    explicit ToolPanel(Services services);

    // Shows `page` (or the last one) with fresh lists.
    void open(std::optional<PanelPage> page = std::nullopt);
    void close();
    [[nodiscard]] bool is_open() const noexcept { return open_; }
    [[nodiscard]] PanelPage page() const noexcept { return page_; }
    [[nodiscard]] const ToolPanelView& view();

    PanelAction key(const PanelKey& key);
    PanelAction click(const PanelHit& hit);
    // Whether the panel takes this key while open (others go to the application).
    static bool takes_key(UINT vk, bool ctrl, bool alt) noexcept;

private:
    struct Row {
        int group = 0;
        std::vector<int> items;  // indexes into entries_
    };
    struct Entry {
        std::wstring text;
        std::wstring name;
        PanelKind kind = PanelKind::Symbol;
        bool favorite = false;
        const PanelItem* item = nullptr;
        size_t sticker = 0;
    };
    enum class RecordsState { Missing, SetPassword, ConfirmPassword, Locked, Items, Fields };

    void set_page(PanelPage page);
    void load_catalogs();
    void rebuild();
    void build_grid();
    void build_list();
    PanelAction grid_key(const PanelKey& key);
    PanelAction list_key(const PanelKey& key);
    PanelAction tools_key(const PanelKey& key);
    PanelAction records_key(const PanelKey& key);
    PanelAction pick(int row, int column);
    PanelAction activate(int index);
    PanelAction insert_tray();
    void move(int rows, int columns);
    void keep_cursor_visible();
    void jump_category(int direction);
    [[nodiscard]] std::vector<std::wstring> search_words() const;
    [[nodiscard]] bool matches(const std::wstring& text) const;
    [[nodiscard]] int columns() const noexcept;
    [[nodiscard]] int visible_rows() const noexcept;
    [[nodiscard]] bool grid_page() const noexcept;
    [[nodiscard]] PanelKind kind() const noexcept;
    void unlock(const std::wstring& password);
    void lock();
    // The password, encrypted for this process while records stay unlocked,
    // so changes made in the settings app show without asking again.
    void remember_password(const std::wstring& password);
    [[nodiscard]] std::wstring recall_password() const;
    [[nodiscard]] long long vault_stamp() const;
    void fill_view();

    Services services_;
    bool open_ = false;
    PanelPage page_ = PanelPage::Symbol;
    std::wstring input_;
    std::wstring message_;
    // Grid pages: groups (最近使用, 收藏, then the catalog's), entries and rows.
    std::vector<std::wstring> group_names_;
    std::vector<Entry> entries_;
    std::vector<Row> rows_;
    int cursor_row_ = 0;
    int cursor_column_ = 0;
    int scroll_ = 0;
    std::vector<std::pair<PanelKind, std::wstring>> tray_;
    // List pages: the rows and which one is current.
    std::vector<PanelCell> list_;
    std::vector<std::string> list_ids_;  // clipboard ids
    std::vector<int> list_refs_;         // records, tool results, choices
    int list_cursor_ = 0;
    int list_scroll_ = 0;
    // Data.
    PanelCatalog catalogs_[3];
    bool catalogs_loaded_ = false;
    PanelUserState user_state_;
    std::vector<Sticker> stickers_;
    std::vector<ClipboardItem> clips_;
    std::vector<std::wstring> tool_results_;
    // 记录: decrypted while unlocked, for five minutes after the panel was last used.
    RecordsState records_state_ = RecordsState::Missing;
    std::vector<VaultItem> records_;
    bool records_unlocked_ = false;
    long long records_used_ = 0;
    int record_category_ = 0;
    int record_open_ = -1;
    std::wstring new_password_;
    std::vector<unsigned char> sealed_password_;
    long long records_stamp_ = 0;
    ToolPanelView view_;
};

}  // namespace localpinyin
