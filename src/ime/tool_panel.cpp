#include "tool_panel.h"

#include "../engine/input_tools.h"

#include <dpapi.h>

#include "../common/win32_utils.h"

#include <algorithm>
#include <cstring>
#include <ctime>
#include <filesystem>

namespace localpinyin {
namespace {

constexpr const wchar_t* kPageLabels[] = {L"工具", L"剪贴板", L"符号", L"颜文字", L"表情", L"表情包", L"记录"};
constexpr const wchar_t* kPageIds[] = {L"tools", L"clipboard", L"symbol", L"kaomoji", L"emoji", L"stickers", L"records"};
constexpr const wchar_t* kRecordKinds[] = {L"", L"password", L"address", L"other"};
constexpr size_t kPreviewLimit = 20000;
constexpr const wchar_t* kRecordCategories[] = {L"全部", L"密码", L"地址", L"其他"};
constexpr int kListRows = 5;
constexpr size_t kTrayLimit = 64;
constexpr size_t kInputLimit = 64;
// Unlocked records stay readable this long after the panel was last used.
constexpr long long kRecordsUnlockSeconds = 5 * 60;

const wchar_t* record_kind_label(const std::wstring& kind) {
    if (kind == L"password") return L"密码";
    if (kind == L"address") return L"地址";
    return L"其他";
}

std::wstring one_line(const std::wstring& text, size_t limit = 160) {
    std::wstring line;
    for (wchar_t ch : text) {
        if (line.size() >= limit) break;
        line += (ch == L'\r' || ch == L'\n' || ch == L'\t') ? L' ' : ch;
    }
    const size_t start = line.find_first_not_of(L' ');
    return start == std::wstring::npos ? std::wstring() : line.substr(start);
}

std::wstring time_label(long long milliseconds, long long now_seconds) {
    const std::time_t when = static_cast<std::time_t>(milliseconds / 1000);
    const std::time_t now = static_cast<std::time_t>(now_seconds);
    std::tm then{}, today{};
    localtime_s(&then, &when);
    localtime_s(&today, &now);
    wchar_t text[32]{};
    if (then.tm_year == today.tm_year && then.tm_yday == today.tm_yday)
        swprintf_s(text, L"%02d:%02d", then.tm_hour, then.tm_min);
    else if (then.tm_year == today.tm_year && then.tm_yday + 1 == today.tm_yday)
        swprintf_s(text, L"昨天 %02d:%02d", then.tm_hour, then.tm_min);
    else
        swprintf_s(text, L"%d月%d日", then.tm_mon + 1, then.tm_mday);
    return text;
}

bool is_cjk_name(const std::wstring& text) {
    return text.size() >= 2 && text.size() <= 6 &&
           std::all_of(text.begin(), text.end(), [](wchar_t ch) { return ch >= 0x3400 && ch <= 0x9FFF; });
}

int digit_of(UINT vk) {
    if (vk >= L'0' && vk <= L'9') return static_cast<int>(vk - L'0');
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) return static_cast<int>(vk - VK_NUMPAD0);
    return -1;
}

void wipe(std::wstring& text) {
    if (!text.empty()) SecureZeroMemory(text.data(), text.size() * sizeof(wchar_t));
    text.clear();
}

struct Example {
    const wchar_t* title;
    const wchar_t* input;
};
constexpr Example kExamples[] = {
    {L"数字计算", L"55+15"}, {L"大小写转换", L"123"}, {L"日期格式", L"12.19"}, {L"当前时间", L"sj"}, {L"拼音声调", L"ai"}};
// 声调: tones one to four, then none.
constexpr int kToneOrder[] = {1, 2, 3, 4, 0};
constexpr const wchar_t* kToneNames[] = {L"不标声调", L"第一声", L"第二声", L"第三声", L"第四声"};

}  // namespace

const wchar_t* panel_page_label(PanelPage page) noexcept { return kPageLabels[static_cast<int>(page)]; }
const wchar_t* panel_page_id(PanelPage page) noexcept { return kPageIds[static_cast<int>(page)]; }

ToolPanel::ToolPanel(Services services) : services_(std::move(services)) {
    if (!services_.now) services_.now = [] { return static_cast<long long>(std::time(nullptr)); };
}

bool ToolPanel::takes_key(UINT vk, bool ctrl, bool alt) noexcept {
    if (alt) return false;
    switch (vk) {
        case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT: case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
        case VK_MENU: case VK_LMENU: case VK_RMENU: case VK_LWIN: case VK_RWIN: case VK_NUMLOCK:
        case VK_SCROLL: case VK_APPS:
            return false;
        default: break;
    }
    if (vk >= VK_F1 && vk <= VK_F24) return false;
    if (vk >= VK_BROWSER_BACK && vk <= VK_LAUNCH_APP2) return false;
    if (ctrl) return vk == L'C' || vk == L'D';
    return true;
}

void ToolPanel::load_catalogs() {
    if (catalogs_loaded_) return;
    catalogs_loaded_ = true;
    const std::wstring folder = services_.resource_dir + L"\\panel\\";
    catalogs_[0].load(folder + L"symbols.tsv");
    catalogs_[1].load(folder + L"kaomoji.tsv");
    catalogs_[2].load(folder + L"emoji.tsv");
}

void ToolPanel::open(std::optional<PanelPage> page) {
    open_ = true;
    load_catalogs();
    user_state_ = PanelUserState::load(services_.data_dir + L"\\desktop\\panel.json");
    // Shared with store apps, so the panel shows the stickers there too.
    static bool shared = false;
    if (!services_.sandboxed && !shared) shared = share_with_store_apps(services_.data_dir + L"\\stickers");
    stickers_ = load_stickers(services_.data_dir + L"\\stickers\\index.json");
    if (records_unlocked_ && services_.now() - records_used_ > kRecordsUnlockSeconds) lock();
    tray_.clear();
    set_page(page.value_or(page_));
}

void ToolPanel::close() {
    open_ = false;
    open_preview(-1);
    tray_.clear();
    wipe(input_);
    wipe(new_password_);
    message_.clear();
    if (records_unlocked_) records_used_ = services_.now();
    record_open_ = -1;
    if (records_state_ == RecordsState::Fields) records_state_ = RecordsState::Items;
}

void ToolPanel::lock() {
    for (auto& record : records_) {
        for (auto& field : record.fields) wipe(field.value);
    }
    records_.clear();
    records_unlocked_ = false;
    record_open_ = -1;
    if (!sealed_password_.empty()) SecureZeroMemory(sealed_password_.data(), sealed_password_.size());
    sealed_password_.clear();
}

void ToolPanel::remember_password(const std::wstring& password) {
    if (!sealed_password_.empty()) SecureZeroMemory(sealed_password_.data(), sealed_password_.size());
    // Whole blocks for CryptProtectMemory, the length first.
    const size_t bytes = sizeof(DWORD) + password.size() * sizeof(wchar_t);
    sealed_password_.assign((bytes + CRYPTPROTECTMEMORY_BLOCK_SIZE - 1) / CRYPTPROTECTMEMORY_BLOCK_SIZE * CRYPTPROTECTMEMORY_BLOCK_SIZE, 0);
    const DWORD length = static_cast<DWORD>(password.size());
    std::memcpy(sealed_password_.data(), &length, sizeof(length));
    if (!password.empty()) std::memcpy(sealed_password_.data() + sizeof(length), password.data(), password.size() * sizeof(wchar_t));
    if (!CryptProtectMemory(sealed_password_.data(), static_cast<DWORD>(sealed_password_.size()), CRYPTPROTECTMEMORY_SAME_PROCESS)) {
        SecureZeroMemory(sealed_password_.data(), sealed_password_.size());
        sealed_password_.clear();
    }
}

std::wstring ToolPanel::recall_password() const {
    if (sealed_password_.empty()) return {};
    std::vector<unsigned char> plain = sealed_password_;
    std::wstring password;
    if (CryptUnprotectMemory(plain.data(), static_cast<DWORD>(plain.size()), CRYPTPROTECTMEMORY_SAME_PROCESS)) {
        DWORD length = 0;
        std::memcpy(&length, plain.data(), sizeof(length));
        if (sizeof(length) + static_cast<size_t>(length) * sizeof(wchar_t) <= plain.size())
            password.assign(reinterpret_cast<const wchar_t*>(plain.data() + sizeof(length)), length);
    }
    SecureZeroMemory(plain.data(), plain.size());
    return password;
}

long long ToolPanel::vault_stamp() const {
    std::error_code error;
    const auto time = std::filesystem::last_write_time(std::filesystem::path(services_.data_dir + L"\\vault.json"), error);
    return error ? 0 : static_cast<long long>(time.time_since_epoch().count());
}

void ToolPanel::set_page(PanelPage page) {
    page_ = page;
    open_preview(-1);
    wipe(input_);
    message_.clear();
    cursor_row_ = cursor_column_ = scroll_ = 0;
    list_cursor_ = list_scroll_ = 0;
    if (page_ == PanelPage::Clipboard) {
        clips_.clear();
        if (!services_.sandboxed && (!services_.clipboard_on || services_.clipboard_on()))
            clips_ = ClipboardStore(services_.data_dir + L"\\clipboard").read();
    }
    if (page_ == PanelPage::Records && !services_.sandboxed) {
        record_open_ = -1;
        if (records_unlocked_ && vault_stamp() != records_stamp_) {
            // Changed in the settings app (or its password did): read it again.
            std::wstring password = recall_password();
            unlock(password);
            wipe(password);
        } else if (records_unlocked_) {
            records_state_ = RecordsState::Items;
        } else {
            const Vault vault(services_.data_dir + L"\\vault.json");
            if (!vault.exists()) records_state_ = RecordsState::Missing;
            else if (vault.has_password()) records_state_ = RecordsState::Locked;
            else unlock(L"");
        }
    }
    rebuild();
}

bool ToolPanel::grid_page() const noexcept {
    return page_ == PanelPage::Symbol || page_ == PanelPage::Kaomoji || page_ == PanelPage::Emoji ||
           page_ == PanelPage::Stickers;
}

PanelKind ToolPanel::kind() const noexcept {
    return page_ == PanelPage::Kaomoji ? PanelKind::Kaomoji : page_ == PanelPage::Emoji ? PanelKind::Emoji : PanelKind::Symbol;
}

int ToolPanel::columns() const noexcept {
    switch (page_) {
        case PanelPage::Kaomoji: return 3;
        case PanelPage::Stickers: return 6;
        case PanelPage::Symbol: case PanelPage::Emoji: return 10;
        default: return 1;
    }
}

int ToolPanel::visible_rows() const noexcept {
    if (page_ == PanelPage::Stickers) return 2;
    return grid_page() ? 4 : kListRows;
}

std::vector<std::wstring> ToolPanel::search_words() const {
    std::vector<std::wstring> words;
    if (input_.empty()) return words;
    words.push_back(input_);
    const bool letters = std::all_of(input_.begin(), input_.end(), [](wchar_t ch) { return ch >= L'a' && ch <= L'z'; });
    if (letters && services_.words) {
        for (const auto& word : services_.words(input_)) {
            if (word.empty() || word == input_) continue;
            words.push_back(word);
            if (words.size() >= 6) break;
        }
    }
    return words;
}

bool ToolPanel::matches(const std::wstring& text) const {
    const auto words = search_words();
    return words.empty() || std::any_of(words.begin(), words.end(), [&](const std::wstring& word) { return contains_folded(text, word); });
}

void ToolPanel::rebuild() {
    if (grid_page()) build_grid();
    else build_list();
}

void ToolPanel::build_grid() {
    entries_.clear();
    rows_.clear();
    group_names_.clear();
    const int width = columns();
    const auto add_group = [&](const std::wstring& name, std::vector<Entry> entries) {
        if (entries.empty()) return;
        const int group = static_cast<int>(group_names_.size());
        group_names_.push_back(name);
        for (size_t start = 0; start < entries.size(); start += static_cast<size_t>(width)) {
            Row row{group, {}};
            for (size_t index = start; index < std::min(entries.size(), start + static_cast<size_t>(width)); ++index) {
                row.items.push_back(static_cast<int>(entries_.size()));
                entries_.push_back(std::move(entries[index]));
            }
            rows_.push_back(std::move(row));
        }
    };
    if (page_ == PanelPage::Stickers) {
        std::vector<Entry> list;
        const auto words = search_words();
        for (size_t index = 0; index < stickers_.size(); ++index) {
            const auto& sticker = stickers_[index];
            if (!words.empty() && !std::any_of(words.begin(), words.end(), [&](const std::wstring& word) {
                    return contains_folded(sticker.name, word);
                })) continue;
            Entry entry;
            entry.text = sticker.name;
            entry.sticker = index;
            list.push_back(std::move(entry));
        }
        add_group(input_.empty() ? L"表情包" : L"搜索结果", std::move(list));
    } else {
        const PanelKind which = kind();
        const auto& catalog = catalogs_[static_cast<size_t>(which)];
        const auto& favorites = user_state_.favorites[static_cast<size_t>(which)];
        const auto make = [&](const std::wstring& text, const PanelItem* item) {
            Entry entry;
            entry.text = text;
            entry.kind = which;
            entry.item = item ? item : catalog.find(text);
            entry.name = entry.item ? entry.item->name : L"";
            entry.favorite = std::find(favorites.begin(), favorites.end(), text) != favorites.end();
            return entry;
        };
        if (!input_.empty()) {
            const auto words = search_words();
            std::vector<Entry> found;
            std::vector<std::wstring> seen;  // an item listed in two sections shows once
            for (const auto& group : catalog.groups()) {
                for (const auto& item : group.items) {
                    if (std::find(seen.begin(), seen.end(), item.text) != seen.end()) continue;
                    if (std::any_of(words.begin(), words.end(), [&](const std::wstring& word) { return panel_item_matches(item, word); })) {
                        found.push_back(make(item.text, &item));
                        seen.push_back(item.text);
                    }
                    if (found.size() >= 400) break;
                }
            }
            add_group(L"搜索结果", std::move(found));
        } else {
            std::vector<Entry> recent, favorite;
            for (const auto& text : user_state_.recents[static_cast<size_t>(which)]) recent.push_back(make(text, nullptr));
            for (const auto& text : favorites) favorite.push_back(make(text, nullptr));
            add_group(L"最近使用", std::move(recent));
            add_group(L"收藏", std::move(favorite));
            for (const auto& group : catalog.groups()) {
                std::vector<Entry> list;
                for (const auto& item : group.items) list.push_back(make(item.text, &item));
                add_group(group.name, std::move(list));
            }
        }
    }
    cursor_row_ = std::clamp(cursor_row_, 0, std::max(0, static_cast<int>(rows_.size()) - 1));
    if (!rows_.empty())
        cursor_column_ = std::clamp(cursor_column_, 0, static_cast<int>(rows_[static_cast<size_t>(cursor_row_)].items.size()) - 1);
    keep_cursor_visible();
}

void ToolPanel::build_list() {
    list_.clear();
    list_ids_.clear();
    list_refs_.clear();
    tool_results_.clear();
    const long long now = services_.now();
    if (page_ == PanelPage::Clipboard) {
        for (const auto& clip : clips_) {
            const std::wstring searchable = clip.image ? clip.ocr : clip.text;
            if (!input_.empty() && !matches(searchable)) continue;
            PanelCell cell;
            if (clip.image) {
                cell.text = L"图片 " + std::to_wstring(clip.width) + L"×" + std::to_wstring(clip.height);
                cell.tag = L"图片";
                cell.image = ClipboardStore(services_.data_dir + L"\\clipboard").picture_path(clip.id);
            } else {
                cell.text = one_line(clip.text);
            }
            cell.detail = time_label(clip.time, now);
            list_.push_back(std::move(cell));
            list_ids_.push_back(clip.id);
        }
    } else if (page_ == PanelPage::Tools) {
        if (!is_tone_text(input_)) tone_choices_.clear();
        else if (tone_choices_.size() >= tone_vowel_count(input_)) tone_choices_.resize(tone_vowel_count(input_) - 1);
        if (input_.empty()) {
            for (int index = 0; index < static_cast<int>(std::size(kExamples)); ++index) {
                list_.push_back(PanelCell{kExamples[index].title, kExamples[index].input, {}, {}, false});
                list_refs_.push_back(index);
            }
        } else if (is_tone_text(input_)) {
            // The next vowel in each tone, the ones before as chosen.
            for (int tone : kToneOrder) {
                auto tones = tone_choices_;
                tones.push_back(tone);
                list_.push_back(PanelCell{tone_text(input_, tones), kToneNames[tone], {}, {}, false});
                list_refs_.push_back(tone);
            }
        } else {
            std::tm local{};
            const std::time_t clock = static_cast<std::time_t>(now);
            localtime_s(&local, &clock);
            tool_results_ = is_tool_text(input_) ? tool_results(input_, local) : time_shortcuts(input_, local);
            for (const auto& result : tool_results_) list_.push_back(PanelCell{result, {}, {}, {}, false});
        }
    } else if (page_ == PanelPage::Records) {
        if (records_state_ == RecordsState::Missing) {
            list_.push_back(PanelCell{L"设置密码", L"以后打开记录要先输入它", {}, {}, false});
            list_.push_back(PanelCell{L"不设密码", L"只用这台电脑的 Windows 账户保护", {}, {}, false});
        } else if (records_state_ == RecordsState::Items) {
            for (size_t index = 0; index < records_.size(); ++index) {
                const auto& record = records_[index];
                if (record_category_ > 0 && record.kind != kRecordKinds[record_category_]) continue;
                std::wstring searchable = record.title;
                for (const auto& field : record.fields) if (!field.secret) searchable += L"\n" + field.value;
                if (!input_.empty() && !matches(searchable)) continue;
                PanelCell cell{record.title, {}, record_kind_label(record.kind), {}, false};
                for (const auto& field : record.fields) {
                    if (field.value.empty()) continue;
                    cell.detail = field.secret ? L"••••••" : one_line(field.value, 60);
                    break;
                }
                list_.push_back(std::move(cell));
                list_refs_.push_back(static_cast<int>(index));
            }
        } else if (records_state_ == RecordsState::Fields && record_open_ >= 0 &&
                   record_open_ < static_cast<int>(records_.size())) {
            const auto& record = records_[static_cast<size_t>(record_open_)];
            for (size_t index = 0; index < record.fields.size(); ++index) {
                const auto& field = record.fields[index];
                if (field.value.empty()) continue;
                list_.push_back(PanelCell{field.name, field.secret ? L"••••••" : one_line(field.value, 80), {}, {}, false});
                list_refs_.push_back(static_cast<int>(index));
            }
        }
    }
    list_cursor_ = std::clamp(list_cursor_, 0, std::max(0, static_cast<int>(list_.size()) - 1));
    if (list_cursor_ < list_scroll_) list_scroll_ = list_cursor_;
    if (list_cursor_ >= list_scroll_ + kListRows) list_scroll_ = list_cursor_ - kListRows + 1;
    list_scroll_ = std::clamp(list_scroll_, 0, std::max(0, static_cast<int>(list_.size()) - kListRows));
}

void ToolPanel::keep_cursor_visible() {
    const int rows = visible_rows();
    if (cursor_row_ < scroll_) scroll_ = cursor_row_;
    if (cursor_row_ >= scroll_ + rows) scroll_ = cursor_row_ - rows + 1;
    scroll_ = std::clamp(scroll_, 0, std::max(0, static_cast<int>(rows_.size()) - rows));
}

void ToolPanel::move(int rows, int columns) {
    if (rows_.empty()) return;
    const int count = static_cast<int>(rows_.size());
    if (columns != 0) {
        cursor_column_ += columns;
        if (cursor_column_ < 0) {
            if (cursor_row_ > 0) {
                --cursor_row_;
                cursor_column_ = static_cast<int>(rows_[static_cast<size_t>(cursor_row_)].items.size()) - 1;
            } else {
                cursor_column_ = 0;
            }
        } else if (cursor_column_ >= static_cast<int>(rows_[static_cast<size_t>(cursor_row_)].items.size())) {
            if (cursor_row_ + 1 < count) {
                ++cursor_row_;
                cursor_column_ = 0;
            } else {
                cursor_column_ = static_cast<int>(rows_[static_cast<size_t>(cursor_row_)].items.size()) - 1;
            }
        }
    }
    if (rows != 0) {
        cursor_row_ = std::clamp(cursor_row_ + rows, 0, count - 1);
        cursor_column_ = std::min(cursor_column_, static_cast<int>(rows_[static_cast<size_t>(cursor_row_)].items.size()) - 1);
    }
    keep_cursor_visible();
}

void ToolPanel::jump_category(int direction, bool wrap) {
    if (rows_.empty()) return;
    const int group = rows_[static_cast<size_t>(cursor_row_)].group;
    const int count = static_cast<int>(group_names_.size());
    const int target = wrap ? ((group + direction) % count + count) % count : std::clamp(group + direction, 0, count - 1);
    for (size_t row = 0; row < rows_.size(); ++row) {
        if (rows_[row].group == target) {
            cursor_row_ = static_cast<int>(row);
            cursor_column_ = 0;
            // The category's first row at the top.
            scroll_ = std::clamp(cursor_row_, 0, std::max(0, static_cast<int>(rows_.size()) - visible_rows()));
            return;
        }
    }
}

PanelAction ToolPanel::key(const PanelKey& key) {
    PanelAction redraw{PanelAction::Kind::Redraw};
    if (!open_) return {};
    if (key.vk == VK_TAB && !key.ctrl) {
        const int step = key.shift ? kPanelPageCount - 1 : 1;
        set_page(static_cast<PanelPage>((static_cast<int>(page_) + step) % kPanelPageCount));
        return redraw;
    }
    if (key.vk == VK_RETURN && key.shift) return PanelAction{PanelAction::Kind::Expand};
    if (preview_ >= 0) return preview_key(key);
    if (key.vk == VK_CAPITAL) {
        // CapsLock walks the categories (the second row), Shift+CapsLock back.
        const int step = key.shift ? -1 : 1;
        if (grid_page()) {
            jump_category(step, true);
            return redraw;
        }
        if (page_ == PanelPage::Records && records_state_ == RecordsState::Items) {
            record_category_ = (record_category_ + 4 + step) % 4;
            list_cursor_ = list_scroll_ = 0;
            rebuild();
            return redraw;
        }
        return {};
    }
    if (key.vk == VK_ESCAPE) {
        if (!input_.empty()) {
            wipe(input_);
            message_.clear();
            rebuild();
            return redraw;
        }
        if (page_ == PanelPage::Records && records_state_ == RecordsState::Fields) {
            records_state_ = RecordsState::Items;
            list_cursor_ = std::max(0, record_open_);
            record_open_ = -1;
            rebuild();
            return redraw;
        }
        if (page_ == PanelPage::Records &&
            (records_state_ == RecordsState::SetPassword || records_state_ == RecordsState::ConfirmPassword)) {
            records_state_ = RecordsState::Missing;
            wipe(new_password_);
            message_.clear();
            rebuild();
            return redraw;
        }
        return PanelAction{PanelAction::Kind::Close};
    }
    if (grid_page()) return grid_key(key);
    if (page_ == PanelPage::Tools) return tools_key(key);
    if (page_ == PanelPage::Records) return records_key(key);
    return list_key(key);
}

PanelAction ToolPanel::grid_key(const PanelKey& key) {
    PanelAction redraw{PanelAction::Kind::Redraw};
    const int digit = key.ctrl ? -1 : digit_of(key.vk);
    if (digit >= 0) return pick(cursor_row_, digit == 0 ? 9 : digit - 1);
    if (key.ctrl) {
        if (key.vk == L'D' && page_ != PanelPage::Stickers && !rows_.empty()) {
            const auto& row = rows_[static_cast<size_t>(cursor_row_)];
            if (cursor_column_ >= static_cast<int>(row.items.size())) return {};
            const std::wstring text = entries_[static_cast<size_t>(row.items[static_cast<size_t>(cursor_column_)])].text;
            auto& favorites = user_state_.favorites[static_cast<size_t>(kind())];
            const auto found = std::find(favorites.begin(), favorites.end(), text);
            if (found != favorites.end()) {
                favorites.erase(found);
                message_ = L"已取消收藏 " + text;
            } else {
                favorites.insert(favorites.begin(), text);
                message_ = L"已收藏 " + text;
            }
            user_state_.save(services_.data_dir + L"\\desktop\\panel.json");
            rebuild();
            return redraw;
        }
        return {};
    }
    message_.clear();
    switch (key.vk) {
        case VK_LEFT: move(0, -1); return redraw;
        case VK_RIGHT: move(0, 1); return redraw;
        case VK_UP: move(-1, 0); return redraw;
        case VK_DOWN: move(1, 0); return redraw;
        case VK_PRIOR: move(-visible_rows(), 0); return redraw;
        case VK_NEXT: move(visible_rows(), 0); return redraw;
        case VK_OEM_MINUS: if (!key.shift) { move(-visible_rows(), 0); return redraw; } break;
        case VK_OEM_PLUS: if (!key.shift) { move(visible_rows(), 0); return redraw; } break;
        case VK_OEM_4: jump_category(-1); return redraw;
        case VK_OEM_6: jump_category(1); return redraw;
        case VK_SPACE: return pick(cursor_row_, cursor_column_);
        case VK_RETURN:
            if (page_ == PanelPage::Stickers || tray_.empty()) {
                PanelAction action = pick(cursor_row_, cursor_column_);
                if (page_ == PanelPage::Stickers || tray_.empty()) return action;
            }
            return insert_tray();
        case VK_BACK:
            if (!input_.empty()) {
                input_.pop_back();
                cursor_row_ = cursor_column_ = scroll_ = 0;
                rebuild();
            } else if (!tray_.empty()) {
                tray_.pop_back();
            }
            return redraw;
        default: break;
    }
    const wchar_t ch = key.ch;
    if ((ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z')) {
        if (input_.size() < kInputLimit) input_ += static_cast<wchar_t>(ch >= L'A' && ch <= L'Z' ? ch - L'A' + L'a' : ch);
        cursor_row_ = cursor_column_ = scroll_ = 0;
        rebuild();
        return redraw;
    }
    return {};
}

PanelAction ToolPanel::pick(int row, int column) {
    if (row < 0 || row >= static_cast<int>(rows_.size())) return {};
    const auto& items = rows_[static_cast<size_t>(row)].items;
    if (column < 0 || column >= static_cast<int>(items.size())) return {};
    cursor_row_ = row;
    cursor_column_ = column;
    const Entry& entry = entries_[static_cast<size_t>(items[static_cast<size_t>(column)])];
    if (page_ == PanelPage::Stickers) {
        const auto& sticker = stickers_[entry.sticker];
        PanelAction action{PanelAction::Kind::PasteSticker};
        action.file = services_.data_dir + L"\\stickers\\" + sticker.id + sticker.ext;
        return action;
    }
    if (tray_.size() < kTrayLimit) tray_.emplace_back(entry.kind, entry.text);
    return PanelAction{PanelAction::Kind::Redraw};
}

PanelAction ToolPanel::insert_tray() {
    if (tray_.empty()) return {};
    PanelAction action{PanelAction::Kind::Insert};
    std::vector<std::wstring> picks[3];
    for (const auto& [which, text] : tray_) {
        action.text += text;
        picks[static_cast<size_t>(which)].push_back(text);
        if (which != PanelKind::Emoji) continue;
        const auto* item = catalogs_[static_cast<size_t>(PanelKind::Emoji)].find(text);
        if (!item) continue;
        std::vector<std::wstring> names;
        if (is_cjk_name(item->name)) names.push_back(item->name);
        for (const auto& word : item->keywords) {
            if (names.size() >= 3) break;
            if (is_cjk_name(word) && std::find(names.begin(), names.end(), word) == names.end()) names.push_back(word);
        }
        if (!names.empty()) action.learn_emoji.emplace_back(text, std::move(names));
    }
    for (size_t which = 0; which < 3; ++which)
        if (!picks[which].empty()) user_state_.remember(static_cast<PanelKind>(which), picks[which]);
    user_state_.save(services_.data_dir + L"\\desktop\\panel.json");
    tray_.clear();
    return action;
}

PanelAction ToolPanel::list_key(const PanelKey& key) {
    PanelAction redraw{PanelAction::Kind::Redraw};
    if (key.ctrl) return {};
    const int count = static_cast<int>(list_.size());
    const int digit = digit_of(key.vk);
    if (digit >= 1 && digit <= kListRows) return activate(list_scroll_ + digit - 1);
    switch (key.vk) {
        case VK_UP: list_cursor_ = std::max(0, list_cursor_ - 1); break;
        case VK_DOWN: list_cursor_ = std::min(std::max(0, count - 1), list_cursor_ + 1); break;
        case VK_PRIOR: list_cursor_ = std::max(0, list_cursor_ - kListRows); break;
        case VK_NEXT: list_cursor_ = std::min(std::max(0, count - 1), list_cursor_ + kListRows); break;
        case VK_OEM_MINUS: if (key.shift) return {}; list_cursor_ = std::max(0, list_cursor_ - kListRows); break;
        case VK_OEM_PLUS: if (key.shift) return {}; list_cursor_ = std::min(std::max(0, count - 1), list_cursor_ + kListRows); break;
        case VK_RETURN: case VK_SPACE: return activate(list_cursor_);
        case VK_RIGHT:
            if (list_.empty()) return {};
            open_preview(list_cursor_);
            return redraw;
        case VK_BACK:
            if (input_.empty()) return {};
            input_.pop_back();
            list_cursor_ = list_scroll_ = 0;
            rebuild();
            return redraw;
        default: {
            const wchar_t ch = key.ch;
            if (!((ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z'))) return {};
            if (input_.size() < kInputLimit) input_ += static_cast<wchar_t>(ch >= L'A' && ch <= L'Z' ? ch - L'A' + L'a' : ch);
            list_cursor_ = list_scroll_ = 0;
            rebuild();
            return redraw;
        }
    }
    rebuild();
    return redraw;
}

void ToolPanel::open_preview(int index) {
    if (!preview_text_.empty()) SecureZeroMemory(preview_text_.data(), preview_text_.size() * sizeof(wchar_t));
    preview_text_.clear();
    preview_ = -1;
    preview_scroll_ = 0;
    if (page_ != PanelPage::Clipboard || index < 0 || index >= static_cast<int>(list_ids_.size())) return;
    const std::string& id = list_ids_[static_cast<size_t>(index)];
    for (const auto& clip : clips_) {
        if (clip.id != id) continue;
        preview_ = index;
        list_cursor_ = index;
        preview_picture_ = clip.image;
        if (clip.image) preview_text_ = clip.ocr;
        else if (!clip.long_text || !ClipboardStore(services_.data_dir + L"\\clipboard").full_text(id, &preview_text_))
            preview_text_ = clip.text;
        preview_length_ = preview_text_.size();
        // Enough to read through; Enter still pastes all of it.
        if (preview_text_.size() > kPreviewLimit) {
            SecureZeroMemory(preview_text_.data() + kPreviewLimit, (preview_text_.size() - kPreviewLimit) * sizeof(wchar_t));
            preview_text_.resize(kPreviewLimit);
            preview_text_ += L"…\n（只显示前 2 万字，Enter 粘贴全部）";
        }
        return;
    }
}

PanelAction ToolPanel::preview_key(const PanelKey& key) {
    PanelAction redraw{PanelAction::Kind::Redraw};
    // About how many lines the text wraps to (a narrower column beside a
    // picture), so scrolling stops near its end.
    const size_t per_line = preview_picture_ ? 12 : 28;
    int lines = 1;
    size_t run = 0;
    for (wchar_t ch : preview_text_) {
        if (ch == L'\n' || ++run > per_line) { ++lines; run = ch == L'\n' ? 0 : 1; }
    }
    const int last = std::max(0, lines - 6);
    switch (key.vk) {
        case VK_ESCAPE: case VK_LEFT: case VK_BACK:
            open_preview(-1);
            return redraw;
        case VK_UP: preview_scroll_ = std::max(0, preview_scroll_ - 1); return redraw;
        case VK_DOWN: preview_scroll_ = std::min(last, preview_scroll_ + 1); return redraw;
        case VK_PRIOR: preview_scroll_ = std::max(0, preview_scroll_ - 5); return redraw;
        case VK_NEXT: preview_scroll_ = std::min(last, preview_scroll_ + 5); return redraw;
        case VK_RETURN: case VK_SPACE: {
            const int index = preview_;
            open_preview(-1);
            return activate(index);
        }
        default: return {};
    }
}

PanelAction ToolPanel::tools_key(const PanelKey& key) {
    PanelAction redraw{PanelAction::Kind::Redraw};
    if (key.ctrl) return {};
    const int count = static_cast<int>(list_.size());
    switch (key.vk) {
        case VK_UP: list_cursor_ = std::max(0, list_cursor_ - 1); rebuild(); return redraw;
        case VK_DOWN: list_cursor_ = std::min(std::max(0, count - 1), list_cursor_ + 1); rebuild(); return redraw;
        case VK_RETURN: return activate(list_cursor_);
        case VK_BACK:
            // Back one vowel first, then letters.
            if (!tone_choices_.empty()) tone_choices_.pop_back();
            else if (!input_.empty()) input_.pop_back();
            else return {};
            list_cursor_ = list_scroll_ = 0;
            rebuild();
            return redraw;
        default: break;
    }
    // 声调: 1-4 a tone, 5 none.
    const int digit = digit_of(key.vk);
    if (is_tone_text(input_) && digit >= 1 && digit <= 5) return activate(digit - 1);
    const wchar_t ch = key.ch;
    const bool allowed = (ch >= L'0' && ch <= L'9') || (ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z') ||
                         std::wstring(L"+-*/().%").find(ch) != std::wstring::npos;
    if (!ch || !allowed) return {};
    if (input_.size() < kInputLimit) input_ += static_cast<wchar_t>(ch >= L'A' && ch <= L'Z' ? ch - L'A' + L'a' : ch);
    list_cursor_ = list_scroll_ = 0;
    rebuild();
    return redraw;
}

void ToolPanel::unlock(const std::wstring& password) {
    std::vector<VaultItem> items;
    if (Vault(services_.data_dir + L"\\vault.json").open(password, &items)) {
        records_ = std::move(items);
        std::stable_sort(records_.begin(), records_.end(), [](const VaultItem& left, const VaultItem& right) {
            return left.updated > right.updated;
        });
        records_unlocked_ = true;
        records_used_ = services_.now();
        records_stamp_ = vault_stamp();
        records_state_ = RecordsState::Items;
        remember_password(password);
        message_.clear();
    } else {
        lock();
        records_state_ = RecordsState::Locked;
        message_ = password.empty() ? L"记录无法打开：只能在设置记录的这台电脑和账户上打开。" : L"密码不对，请再试一次。";
    }
}

PanelAction ToolPanel::records_key(const PanelKey& key) {
    PanelAction redraw{PanelAction::Kind::Redraw};
    if (services_.sandboxed) return {};
    const int count = static_cast<int>(list_.size());
    const bool typing_password = records_state_ == RecordsState::SetPassword ||
                                 records_state_ == RecordsState::ConfirmPassword || records_state_ == RecordsState::Locked;
    if (typing_password) {
        if (key.vk == VK_BACK) {
            if (!input_.empty()) input_.pop_back();
            return redraw;
        }
        if (key.vk == VK_RETURN) {
            if (input_.empty()) {
                message_ = records_state_ == RecordsState::SetPassword ? L"请输入一个密码；不想设密码就按 Esc 返回，选“不设密码”。"
                                                                       : L"请输入密码。";
                return redraw;
            }
            if (records_state_ == RecordsState::SetPassword) {
                new_password_ = input_;
                wipe(input_);
                records_state_ = RecordsState::ConfirmPassword;
                message_.clear();
            } else if (records_state_ == RecordsState::ConfirmPassword) {
                if (input_ == new_password_) {
                    const Vault vault(services_.data_dir + L"\\vault.json");
                    if (vault.create(new_password_)) unlock(new_password_);
                    else message_ = L"无法创建记录文件。";
                } else {
                    records_state_ = RecordsState::SetPassword;
                    message_ = L"两次输入的不一样，请重新设置。";
                }
                wipe(new_password_);
                wipe(input_);
            } else {
                std::wstring password = std::move(input_);
                input_.clear();
                unlock(password);
                wipe(password);
            }
            rebuild();
            return redraw;
        }
        if (!key.ctrl && key.ch >= 0x20 && input_.size() < 128) {
            input_ += key.ch;
            return redraw;
        }
        return {};
    }
    if (key.ctrl) {
        if (key.vk == L'C' && records_state_ == RecordsState::Fields && record_open_ >= 0 &&
            list_cursor_ < static_cast<int>(list_refs_.size())) {
            const auto& field = records_[static_cast<size_t>(record_open_)].fields[static_cast<size_t>(list_refs_[static_cast<size_t>(list_cursor_)])];
            PanelAction action{PanelAction::Kind::CopySecret};
            action.text = field.value;
            records_used_ = services_.now();
            return action;
        }
        return {};
    }
    const int digit = digit_of(key.vk);
    if (digit >= 1 && digit <= kListRows) return activate(list_scroll_ + digit - 1);
    switch (key.vk) {
        case VK_UP: list_cursor_ = std::max(0, list_cursor_ - 1); break;
        case VK_DOWN: list_cursor_ = std::min(std::max(0, count - 1), list_cursor_ + 1); break;
        case VK_PRIOR: case VK_OEM_MINUS: list_cursor_ = std::max(0, list_cursor_ - kListRows); break;
        case VK_NEXT: case VK_OEM_PLUS: list_cursor_ = std::min(std::max(0, count - 1), list_cursor_ + kListRows); break;
        case VK_RETURN: case VK_SPACE: return activate(list_cursor_);
        case VK_LEFT: case VK_OEM_4:
            if (records_state_ == RecordsState::Fields) {
                records_state_ = RecordsState::Items;
                list_cursor_ = 0;
                record_open_ = -1;
            } else if (records_state_ == RecordsState::Items) {
                record_category_ = (record_category_ + 3) % 4;
                list_cursor_ = list_scroll_ = 0;
            }
            break;
        case VK_RIGHT: case VK_OEM_6:
            if (records_state_ == RecordsState::Items) {
                record_category_ = (record_category_ + 1) % 4;
                list_cursor_ = list_scroll_ = 0;
            }
            break;
        case VK_BACK:
            if (!input_.empty()) {
                input_.pop_back();
                list_cursor_ = list_scroll_ = 0;
            } else if (records_state_ == RecordsState::Fields) {
                records_state_ = RecordsState::Items;
                list_cursor_ = 0;
                record_open_ = -1;
            }
            break;
        default: {
            const wchar_t ch = key.ch;
            if (records_state_ != RecordsState::Items || !((ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z'))) return {};
            if (input_.size() < kInputLimit) input_ += static_cast<wchar_t>(ch >= L'A' && ch <= L'Z' ? ch - L'A' + L'a' : ch);
            list_cursor_ = list_scroll_ = 0;
        }
    }
    rebuild();
    return redraw;
}

PanelAction ToolPanel::activate(int index) {
    PanelAction redraw{PanelAction::Kind::Redraw};
    if (index < 0 || index >= static_cast<int>(list_.size())) return {};
    list_cursor_ = index;
    if (page_ == PanelPage::Clipboard) {
        const std::string& id = list_ids_[static_cast<size_t>(index)];
        const ClipboardStore store(services_.data_dir + L"\\clipboard");
        for (const auto& clip : clips_) {
            if (clip.id != id) continue;
            if (clip.image) {
                PanelAction action{PanelAction::Kind::PasteClip};
                action.clip_id = id;
                return action;
            }
            PanelAction action{PanelAction::Kind::Insert};
            if (!clip.long_text || !store.full_text(id, &action.text)) action.text = clip.text;
            return action;
        }
        return {};
    }
    if (page_ == PanelPage::Tools) {
        if (input_.empty()) {
            input_ = kExamples[list_refs_[static_cast<size_t>(index)]].input;
            list_cursor_ = list_scroll_ = 0;
            rebuild();
            return redraw;
        }
        if (is_tone_text(input_)) {
            // A tone for this vowel; the pinyin goes in after the last one.
            tone_choices_.push_back(list_refs_[static_cast<size_t>(index)]);
            if (tone_choices_.size() < tone_vowel_count(input_)) {
                list_cursor_ = list_scroll_ = 0;
                rebuild();
                return redraw;
            }
            PanelAction action{PanelAction::Kind::Insert};
            action.text = tone_text(input_, tone_choices_);
            tone_choices_.clear();
            return action;
        }
        PanelAction action{PanelAction::Kind::Insert};
        action.text = list_[static_cast<size_t>(index)].text;
        return action;
    }
    if (page_ == PanelPage::Records) {
        if (records_state_ == RecordsState::Missing) {
            if (index == 0) {
                records_state_ = RecordsState::SetPassword;
                message_.clear();
            } else {
                const Vault vault(services_.data_dir + L"\\vault.json");
                if (vault.create(L"")) unlock(L"");
                else message_ = L"无法创建记录文件。";
            }
            rebuild();
            return redraw;
        }
        if (records_state_ == RecordsState::Items) {
            record_open_ = list_refs_[static_cast<size_t>(index)];
            records_state_ = RecordsState::Fields;
            wipe(input_);
            list_cursor_ = list_scroll_ = 0;
            records_used_ = services_.now();
            rebuild();
            return redraw;
        }
        if (records_state_ == RecordsState::Fields && record_open_ >= 0) {
            PanelAction action{PanelAction::Kind::Insert};
            action.text = records_[static_cast<size_t>(record_open_)].fields[static_cast<size_t>(list_refs_[static_cast<size_t>(index)])].value;
            records_used_ = services_.now();
            return action;
        }
    }
    return {};
}

PanelAction ToolPanel::click(const PanelHit& hit) {
    PanelAction redraw{PanelAction::Kind::Redraw};
    switch (hit.kind) {
        case PanelHit::Kind::Page:
            if (hit.index >= 0 && hit.index < kPanelPageCount) set_page(static_cast<PanelPage>(hit.index));
            return redraw;
        case PanelHit::Kind::Category:
            if (grid_page()) {
                for (size_t row = 0; row < rows_.size(); ++row) {
                    if (rows_[row].group != hit.index) continue;
                    cursor_row_ = static_cast<int>(row);
                    cursor_column_ = 0;
                    scroll_ = std::clamp(cursor_row_, 0, std::max(0, static_cast<int>(rows_.size()) - visible_rows()));
                    break;
                }
            } else if (page_ == PanelPage::Records && records_state_ == RecordsState::Items && hit.index >= 0 && hit.index < 4) {
                record_category_ = hit.index;
                list_cursor_ = list_scroll_ = 0;
                rebuild();
            }
            return redraw;
        case PanelHit::Kind::Cell:
            if (grid_page()) {
                const int width = columns();
                return pick(scroll_ + hit.index / width, hit.index % width);
            }
            return activate(list_scroll_ + hit.index);
        case PanelHit::Kind::Preview:
            open_preview(hit.index < 0 ? -1 : list_scroll_ + hit.index);
            return redraw;
        case PanelHit::Kind::Tray:
            if (hit.index >= 0 && hit.index < static_cast<int>(tray_.size())) tray_.erase(tray_.begin() + hit.index);
            return redraw;
        case PanelHit::Kind::Expand: return PanelAction{PanelAction::Kind::Expand};
        case PanelHit::Kind::Close: return PanelAction{PanelAction::Kind::Close};
        default: return {};
    }
}

const ToolPanelView& ToolPanel::view() {
    fill_view();
    return view_;
}

void ToolPanel::fill_view() {
    ToolPanelView view;
    view.page = page_;
    view.rows = visible_rows();
    for (const auto& [which, text] : tray_) view.tray.push_back(text);
    if (grid_page()) {
        view.grid = true;
        view.columns = columns();
        view.font = page_ == PanelPage::Emoji ? PanelFont::Emoji
                  : page_ == PanelPage::Kaomoji ? PanelFont::Kaomoji
                  : page_ == PanelPage::Stickers ? PanelFont::Text : PanelFont::Symbol;
        view.categories = group_names_;
        if (!rows_.empty()) view.category = rows_[static_cast<size_t>(cursor_row_)].group;
        if (!group_names_.empty() && group_names_.front() == L"最近使用") view.recent_category = 0;
        view.caps_hint = group_names_.size() > 1;
        for (int row = scroll_; row < std::min(static_cast<int>(rows_.size()), scroll_ + view.rows); ++row) {
            const auto& items = rows_[static_cast<size_t>(row)].items;
            for (int column = 0; column < view.columns; ++column) {
                PanelCell cell;
                if (column < static_cast<int>(items.size())) {
                    const Entry& entry = entries_[static_cast<size_t>(items[static_cast<size_t>(column)])];
                    cell.text = entry.text;
                    cell.detail = entry.name;
                    cell.favorite = entry.favorite;
                    if (page_ == PanelPage::Stickers) {
                        const auto& sticker = stickers_[entry.sticker];
                        cell.image = services_.data_dir + L"\\stickers\\" + sticker.id + sticker.ext;
                    }
                }
                view.cells.push_back(std::move(cell));
            }
        }
        if (!rows_.empty()) {
            view.highlight = (cursor_row_ - scroll_) * view.columns + cursor_column_;
            view.numbered_row = cursor_row_ - scroll_;
        }
        view.more_above = scroll_ > 0;
        view.more_below = scroll_ + view.rows < static_cast<int>(rows_.size());
        view.prompt = L"搜索";
        view.input = input_;
        view.input_active = true;
        if (page_ == PanelPage::Stickers) {
            view.hint = L"数字 发送 · Tab 换页";
            if (stickers_.empty())
                view.message = services_.sandboxed ? L"这里读不到表情包。在别的程序里打开一次小窗后再试，或按 Shift+Enter 在大窗口里发送。"
                                                   : L"还没有表情包。按 Shift+Enter 打开大窗口，把图片拖进去就能添加。";
        } else {
            view.hint = view.tray.empty() ? L"数字 选 · Enter 输入 · Ctrl+D 收藏" : L"Enter 输入 · Backspace 去掉";
        }
        if (view.cells.empty() && view.message.empty() && !input_.empty()) view.message = L"没有找到“" + input_ + L"”。";
    } else {
        view.grid = false;
        view.columns = 1;
        view.font = PanelFont::Text;
        for (int index = list_scroll_; index < std::min(static_cast<int>(list_.size()), list_scroll_ + view.rows); ++index)
            view.cells.push_back(list_[static_cast<size_t>(index)]);
        if (!list_.empty()) view.highlight = list_cursor_ - list_scroll_;
        view.more_above = list_scroll_ > 0;
        view.more_below = list_scroll_ + view.rows < static_cast<int>(list_.size());
        view.numbered_list = page_ != PanelPage::Tools;
        if (page_ == PanelPage::Clipboard) {
            view.prompt = L"搜索";
            view.input = input_;
            view.input_active = true;
            view.hint = L"数字/Enter 粘贴 · → 查看";
            view.magnifier = true;
            if (preview_ >= 0 && preview_ < static_cast<int>(list_ids_.size())) {
                for (const auto& clip : clips_) {
                    if (clip.id != list_ids_[static_cast<size_t>(preview_)]) continue;
                    view.preview = true;
                    view.preview_title = (clip.image ? L"图片 " + std::to_wstring(clip.width) + L"×" + std::to_wstring(clip.height)
                                                     : L"文字 " + std::to_wstring(preview_length_) + L" 字") +
                                         L" · " + time_label(clip.time, services_.now());
                    view.preview_text = clip.image && !preview_text_.empty() ? L"图中文字：" + preview_text_ : preview_text_;
                    if (clip.image) view.preview_image = ClipboardStore(services_.data_dir + L"\\clipboard").picture_path(clip.id);
                    view.preview_scroll = preview_scroll_;
                    view.hint = L"Enter 粘贴 · ↑↓ 滚动 · ← 返回";
                    view.prompt.clear();
                    view.input_active = false;
                }
            }
            if (services_.clipboard_on && !services_.clipboard_on())
                view.message = L"剪贴板记录没有开启。按 Shift+Enter 打开大窗口开启。";
            else if (list_.empty())
                view.message = input_.empty() ? L"还没有记录。复制一段文字或一张图片，它就会出现在这里。" : L"没有找到“" + input_ + L"”。";
            else
                view.info = L"共 " + std::to_wstring(list_.size()) + L" 条";
        } else if (page_ == PanelPage::Tools) {
            view.prompt = L"算式";
            view.input = input_;
            view.input_active = true;
            view.hint = input_.empty() ? L"↑↓ 选 · Enter 试一试" : L"↑↓ 选 · Enter 输入";
            if (is_tone_text(input_)) {
                // Which vowel is being marked, of how many.
                const size_t count = tone_vowel_count(input_);
                size_t seen = 0;
                wchar_t vowel = 0;
                for (wchar_t ch : input_) {
                    if (std::wstring(L"aeiouvü").find(ch) == std::wstring::npos) continue;
                    if (seen++ == tone_choices_.size()) { vowel = ch == L'v' ? L'ü' : ch; break; }
                }
                view.prompt = L"拼音";
                view.numbered_list = true;
                view.info = L"选 " + std::wstring(1, vowel) + L" 的声调 · " + std::to_wstring(tone_choices_.size() + 1) + L"/" +
                            std::to_wstring(count);
                view.hint = tone_choices_.empty() ? L"1–4 声调 · 5 不标" : L"1–4 声调 · 5 不标 · Backspace 上一个";
            }
            if (!input_.empty() && list_.empty()) view.message = L"这个还不会转换，试试 3*(4+5)、2024、2024.12.19、sj，或拼音 ai。";
        } else {
            switch (records_state_) {
                case RecordsState::Missing:
                    view.message = L"第一次使用记录：要不要设一个密码？";
                    view.hint = L"数字/Enter 选择";
                    break;
                case RecordsState::SetPassword:
                case RecordsState::ConfirmPassword:
                case RecordsState::Locked:
                    view.prompt = records_state_ == RecordsState::SetPassword ? L"设置密码"
                                : records_state_ == RecordsState::ConfirmPassword ? L"再输入一次" : L"输入密码";
                    view.input = std::wstring(input_.size(), L'•');
                    view.input_active = true;
                    view.hint = L"Enter 确定 · Esc 返回";
                    view.message = records_state_ == RecordsState::Locked ? L"记录已加密，输入密码后按 Enter。"
                                 : records_state_ == RecordsState::SetPassword ? L"设一个密码，以后打开记录时要先输入它。"
                                                                               : L"再输入一次刚才的密码。";
                    break;
                case RecordsState::Items:
                    view.categories.assign(std::begin(kRecordCategories), std::end(kRecordCategories));
                    view.category = record_category_;
                    view.caps_hint = true;
                    view.prompt = L"搜索";
                    view.input = input_;
                    view.input_active = true;
                    view.info = L"已加密";
                    view.hint = L"Enter 打开 · Caps 分类";
                    if (records_.empty())
                        view.message = L"还没有记录。按 Shift+Enter 打开大窗口，添加地址、账号密码、卡号等。";
                    else if (list_.empty())
                        view.message = input_.empty() ? L"这一类还没有记录。" : L"没有找到“" + input_ + L"”。";
                    break;
                case RecordsState::Fields:
                    if (record_open_ >= 0 && record_open_ < static_cast<int>(records_.size())) {
                        view.categories = {records_[static_cast<size_t>(record_open_)].title};
                        view.category = 0;
                    }
                    view.hint = L"Enter 输入 · Ctrl+C 复制 · ← 返回";
                    break;
            }
        }
    }
    if (services_.sandboxed && (page_ == PanelPage::Records || page_ == PanelPage::Clipboard)) {
        // A store app's sandbox: not read here, on purpose for the records.
        view.cells.clear();
        view.categories.clear();
        view.info.clear();
        view.prompt.clear();
        view.input_active = false;
        view.magnifier = view.preview = false;
        view.message = page_ == PanelPage::Records
            ? L"这是商店里的沙盒应用，为了安全，小窗在这里不打开记录。按 Shift+Enter 在大窗口里使用。"
            : L"这是商店里的沙盒应用，小窗在这里读不到剪贴板记录。按 Shift+Enter 在大窗口里使用。";
        view.hint = L"Shift+Enter 大窗口 · Tab 换页";
    }
    if (!message_.empty()) view.message = message_;
    view_ = std::move(view);
}

}  // namespace localpinyin
