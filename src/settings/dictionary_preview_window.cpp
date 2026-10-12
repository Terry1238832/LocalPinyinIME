#include "dictionary_preview_window.h"

#include "settings_window.h"

#include <algorithm>
#include <iterator>
#include <string>

namespace localpinyin {
namespace {
constexpr int kSearchId = 1;
constexpr int kListId = 2;
constexpr int kStatusId = 3;
constexpr UINT_PTR kListHeaderSubclassId = 1;
constexpr UINT_PTR kSearchTimer = 20;
constexpr UINT kSearchDelayMs = 120;

SettingsThemePalette palette_for(bool dark) {
    return settings_theme_palette(dark ? SettingsResolvedTheme::Dark : SettingsResolvedTheme::Light);
}

void fill_solid_rect(HDC hdc, const RECT& rect, COLORREF color) {
    if (auto* renderer = active_modern_renderer()) { renderer->fill(rect, color); return; }
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);
}

void draw_text(HDC hdc, const std::wstring& text, const RECT& rect, COLORREF color,
               HFONT font, float size, UINT flags = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS) {
    if (auto* renderer = active_modern_renderer()) { renderer->text(text, rect, color, size, FW_NORMAL, flags); return; }
    const HGDIOBJ previous = font ? SelectObject(hdc, font) : nullptr;
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, color);
    RECT copy = rect;
    DrawTextW(hdc, text.c_str(), static_cast<int>(text.size()), &copy, flags | DT_NOPREFIX);
    if (previous) SelectObject(hdc, previous);
}

std::wstring window_text(HWND hwnd) {
    const int length = GetWindowTextLengthW(hwnd);
    std::wstring value(static_cast<size_t>(std::max(0, length)) + 1, L'\0');
    if (length > 0) GetWindowTextW(hwnd, value.data(), length + 1);
    value.resize(static_cast<size_t>(std::max(0, length)));
    return value;
}

void move_control(HWND hwnd, const RECT& rect) {
    if (hwnd) MoveWindow(hwnd, rect.left, rect.top, std::max<LONG>(0, rect.right - rect.left),
                         std::max<LONG>(0, rect.bottom - rect.top), TRUE);
}
}  // namespace

DictionaryPreviewLayout calculate_dictionary_preview_layout(int width, int height, int dpi) noexcept {
    const auto s = [dpi](int value) { return scale_settings_value(value, dpi); };
    const int margin = std::min(s(28), std::max(0, width / 12));
    const int right = std::max(margin, width - margin);
    const int search_top = s(108);
    const int table_top = search_top + s(60);
    const int status_top = std::max(table_top + s(80), height - s(44));
    DictionaryPreviewLayout layout;
    layout.title = RECT{margin, s(22), std::max(margin, right - s(130)), s(58)};
    layout.subtitle = RECT{margin, s(62), right, s(88)};
    layout.badge = RECT{std::max(margin, right - s(96)), s(28), right, s(58)};
    layout.search_frame = RECT{margin, search_top, right, search_top + s(44)};
    layout.search_edit = RECT{margin + s(44), search_top + s(8), std::max(margin + s(44), right - s(14)), search_top + s(36)};
    layout.table_frame = RECT{margin, table_top, right, std::max(table_top, status_top - s(12))};
    layout.list = layout.table_frame;
    InflateRect(&layout.list, -s(1), -s(1));
    layout.status = RECT{margin, status_top, right, status_top + s(24)};
    return layout;
}

DictionaryPreviewWindow::DictionaryPreviewWindow(const DictionaryCatalog& catalog, bool dark_theme)
    : catalog_(catalog), dark_theme_(dark_theme) {}

HWND DictionaryPreviewWindow::show(HWND owner, HINSTANCE instance,
                                   const DictionaryCatalog& catalog, bool dark_theme, int show_command) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.hInstance = instance;
    wc.lpfnWndProc = window_proc;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"LocalPinyinDictionaryPreviewWindow";
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return nullptr;
    const int dpi = owner ? static_cast<int>(GetDpiForWindow(owner)) : 96;
    const int width = scale_settings_value(920, dpi);
    const int height = scale_settings_value(680, dpi);
    RECT area{};
    if (owner) GetWindowRect(owner, &area);
    const int x = owner ? area.left + (area.right - area.left - width) / 2 : CW_USEDEFAULT;
    const int y = owner ? area.top + (area.bottom - area.top - height) / 2 : CW_USEDEFAULT;
    auto* self = new DictionaryPreviewWindow(catalog, dark_theme);
    HWND hwnd = CreateWindowExW(WS_EX_APPWINDOW | WS_EX_CONTROLPARENT, wc.lpszClassName, L"本地拼音 · 词库浏览",
                                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                x, y, width, height, owner, nullptr, instance, self);
    if (!hwnd) { delete self; return nullptr; }
    self->creation_finished_ = true;
    if (owner) {
        for (WPARAM kind : {static_cast<WPARAM>(ICON_BIG), static_cast<WPARAM>(ICON_SMALL)}) {
            const LRESULT icon = SendMessageW(owner, WM_GETICON, kind, 0);
            if (icon) SendMessageW(hwnd, WM_SETICON, kind, icon);
        }
    }
    ShowWindow(hwnd, show_command);
    UpdateWindow(hwnd);
    if (show_command == SW_SHOWNORMAL || show_command == SW_SHOW) SetFocus(self->search_);
    return hwnd;
}

void DictionaryPreviewWindow::set_theme(HWND hwnd, bool dark_theme) {
    if (!hwnd || !IsWindow(hwnd)) return;
    wchar_t class_name[64]{};
    if (!GetClassNameW(hwnd, class_name, static_cast<int>(std::size(class_name))) ||
        wcscmp(class_name, L"LocalPinyinDictionaryPreviewWindow") != 0) return;
    auto* self = reinterpret_cast<DictionaryPreviewWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!self || self->dark_theme_ == dark_theme) return;
    self->dark_theme_ = dark_theme;
    self->apply_theme();
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

LRESULT CALLBACK DictionaryPreviewWindow::window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* self = reinterpret_cast<DictionaryPreviewWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        self = static_cast<DictionaryPreviewWindow*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
        self->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (!self) return DefWindowProcW(hwnd, message, wparam, lparam);
    const LRESULT result = self->handle_message(message, wparam, lparam);
    if (message == WM_NCDESTROY) {
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        if (self->creation_finished_) delete self;
    }
    return result;
}

LRESULT DictionaryPreviewWindow::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
        case WM_CREATE: create_controls(); refresh_metrics(); apply_theme(); layout(); populate(); return 0;
        case WM_SIZE: layout(); return 0;
        case WM_GETMINMAXINFO: {
            auto* info = reinterpret_cast<MINMAXINFO*>(lparam);
            info->ptMinTrackSize = POINT{scale_settings_value(760, dpi_), scale_settings_value(540, dpi_)};
            return 0;
        }
        case WM_DPICHANGED: {
            dpi_ = HIWORD(wparam);
            const auto* rect = reinterpret_cast<const RECT*>(lparam);
            SetWindowPos(hwnd_, nullptr, rect->left, rect->top, rect->right - rect->left,
                         rect->bottom - rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
            refresh_metrics(); layout(); return 0;
        }
        case WM_COMMAND:
            if (LOWORD(wparam) == kSearchId) {
                if (HIWORD(wparam) == EN_CHANGE) {
                    if (!SetTimer(hwnd_, kSearchTimer, kSearchDelayMs, nullptr)) populate();
                } else if (HIWORD(wparam) == EN_SETFOCUS || HIWORD(wparam) == EN_KILLFOCUS) {
                    InvalidateRect(hwnd_, nullptr, FALSE);
                }
            } else if (LOWORD(wparam) == IDCANCEL) DestroyWindow(hwnd_);
            return 0;
        case WM_TIMER:
            if (wparam == kSearchTimer) { KillTimer(hwnd_, kSearchTimer); populate(); return 0; }
            break;
        case WM_NOTIFY: {
            const auto* header = reinterpret_cast<NMHDR*>(lparam);
            if (!header || header->code != NM_CUSTOMDRAW) break;
            if (header->hwndFrom == list_) {
                auto* custom = reinterpret_cast<NMLVCUSTOMDRAW*>(lparam);
                if (custom->nmcd.dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
                if (custom->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                    const auto palette = palette_for(dark_theme_);
                    custom->clrText = palette.text;
                    custom->clrTextBk = palette.card_background;
                    return draw_list_row(*custom) ? CDRF_SKIPDEFAULT : CDRF_NEWFONT;
                }
            }
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(hwnd_, &paint);
            render(dc);
            EndPaint(hwnd_, &paint);
            return 0;
        }
        case WM_PRINTCLIENT: render(reinterpret_cast<HDC>(wparam)); return 0;
        case WM_ERASEBKGND: return 1;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT: {
            const auto palette = palette_for(dark_theme_);
            const bool edit = message == WM_CTLCOLOREDIT;
            SetTextColor(reinterpret_cast<HDC>(wparam), palette.text);
            SetBkColor(reinterpret_cast<HDC>(wparam), edit ? palette.input_background : palette.window_background);
            return reinterpret_cast<LRESULT>(edit ? input_background_ : background_);
        }
        case WM_DESTROY:
            KillTimer(hwnd_, kSearchTimer);
            if (font_) DeleteObject(font_);
            if (background_) DeleteObject(background_);
            if (input_background_) DeleteObject(input_background_);
            if (row_metrics_) {
                if (list_ && IsWindow(list_)) ListView_SetImageList(list_, nullptr, LVSIL_SMALL);
                ImageList_Destroy(row_metrics_);
            }
            font_ = nullptr; background_ = nullptr; input_background_ = nullptr; row_metrics_ = nullptr;
            return 0;
        default: return DefWindowProcW(hwnd_, message, wparam, lparam);
    }
    return DefWindowProcW(hwnd_, message, wparam, lparam);
}

void DictionaryPreviewWindow::create_controls() {
    dpi_ = static_cast<int>(GetDpiForWindow(hwnd_));
    search_ = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                              0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSearchId)), nullptr, nullptr);
    SendMessageW(search_, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(kDictionaryPreviewSearchPlaceholder));
    SendMessageW(search_, EM_SETLIMITTEXT, 128, 0);
    list_ = CreateWindowExW(0, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHAREIMAGELISTS,
                            0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kListId)), nullptr, nullptr);
    ListView_SetExtendedListViewStyle(list_, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    status_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd_,
                              reinterpret_cast<HMENU>(static_cast<INT_PTR>(kStatusId)), nullptr, nullptr);
    for (HWND child : {search_, list_, status_}) {
        SetWindowSubclass(child, control_subclass_proc, 1, reinterpret_cast<DWORD_PTR>(this));
    }
    const wchar_t* names[] = {L"拼音", L"词语", L"权重", L"来源"};
    for (int i = 0; i < 4; ++i) {
        LVCOLUMNW column{};
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
        column.pszText = const_cast<wchar_t*>(names[i]); column.iSubItem = i;
        column.cx = scale_settings_value(160, dpi_);
        column.fmt = i == 2 ? LVCFMT_RIGHT : LVCFMT_LEFT;
        ListView_InsertColumn(list_, i, &column);
    }
    install_list_header_theme_subclass();
}

void DictionaryPreviewWindow::refresh_metrics() {
    LOGFONTW font{};
    font.lfHeight = -MulDiv(11, dpi_, 72);
    wcscpy_s(font.lfFaceName, L"Microsoft YaHei UI");
    HFONT replacement = CreateFontIndirectW(&font);
    if (replacement) {
        for (HWND child : {search_, list_, status_}) SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(replacement), TRUE);
        if (font_) DeleteObject(font_);
        font_ = replacement;
    }
    if (row_metrics_) {
        ListView_SetImageList(list_, nullptr, LVSIL_SMALL);
        ImageList_Destroy(row_metrics_);
    }
    row_metrics_ = ImageList_Create(1, scale_settings_value(36, dpi_), ILC_COLOR32, 1, 1);
    if (row_metrics_) ListView_SetImageList(list_, row_metrics_, LVSIL_SMALL);
}

void DictionaryPreviewWindow::layout() {
    if (!hwnd_ || !list_) return;
    RECT client{}; GetClientRect(hwnd_, &client);
    const auto geometry = calculate_dictionary_preview_layout(client.right, client.bottom, dpi_);
    move_control(search_, geometry.search_edit);
    move_control(list_, geometry.list);
    move_control(status_, geometry.status);
    const int available = std::max<int>(0, geometry.list.right - geometry.list.left - GetSystemMetricsForDpi(SM_CXVSCROLL, dpi_));
    const int pinyin_width = std::min(scale_settings_value(236, dpi_), available / 3);
    const int weight_width = scale_settings_value(96, dpi_);
    const int source_width = scale_settings_value(138, dpi_);
    const int word_width = std::max(0, available - pinyin_width - weight_width - source_width);
    const int widths[] = {pinyin_width, word_width, weight_width, source_width};
    for (int column = 0; column < 4; ++column) ListView_SetColumnWidth(list_, column, widths[column]);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void DictionaryPreviewWindow::populate() {
    const std::wstring query = window_text(search_);
    const auto rows = catalog_.preview(query, query.empty() ? 200 : 500);
    SendMessageW(list_, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(list_);
    for (size_t i = 0; i < rows.size(); ++i) {
        const auto& row = rows[i];
        LVITEMW item{}; item.mask = LVIF_TEXT; item.iItem = static_cast<int>(i);
        item.pszText = const_cast<wchar_t*>(row.pinyin.c_str());
        ListView_InsertItem(list_, &item);
        ListView_SetItemText(list_, static_cast<int>(i), 1, const_cast<wchar_t*>(row.word.c_str()));
        std::wstring weight = std::to_wstring(row.weight);
        ListView_SetItemText(list_, static_cast<int>(i), 2, weight.data());
        std::wstring source = dictionary_catalog_source_text(row.source);
        ListView_SetItemText(list_, static_cast<int>(i), 3, source.data());
    }
    SendMessageW(list_, WM_SETREDRAW, TRUE, 0);
    const std::wstring status = (query.empty() ? L"当前预览 " : L"匹配结果 ") + std::to_wstring(rows.size()) +
                                L" 条 · 内置词库共 " + std::to_wstring(catalog_.stats().total_unique_entries) + L" 条" +
                                (rows.size() == (query.empty() ? 200u : 500u) ? L" · 输入关键词缩小范围" : L"");
    SetWindowTextW(status_, status.c_str());
    RedrawWindow(list_, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void DictionaryPreviewWindow::apply_theme() {
    const auto palette = palette_for(dark_theme_);
    apply_settings_dark_title_bar(hwnd_, palette.dark);
    apply_settings_control_theme(list_, dark_theme_);
    if (background_) DeleteObject(background_);
    if (input_background_) DeleteObject(input_background_);
    background_ = CreateSolidBrush(palette.window_background);
    input_background_ = CreateSolidBrush(palette.input_background);
    ListView_SetBkColor(list_, palette.card_background);
    ListView_SetTextBkColor(list_, palette.card_background);
    ListView_SetTextColor(list_, palette.text);
}

void DictionaryPreviewWindow::render(HDC hdc) {
    if (!hdc) return;
    RECT client{}; GetClientRect(hwnd_, &client);
    const auto geometry = calculate_dictionary_preview_layout(client.right, client.bottom, dpi_);
    const auto palette = palette_for(dark_theme_);
    const auto s = [this](int value) { return scale_settings_value(value, dpi_); };
    ModernRenderScope modern(renderer_, hdc, client);
    fill_solid_rect(hdc, client, palette.window_background);
    draw_text(hdc, L"词库浏览", geometry.title, palette.text, font_, static_cast<float>(s(26)));
    draw_text(hdc, L"查看随输入法安装的词语，按拼音或中文搜索。", geometry.subtitle,
                palette.muted_text, font_, static_cast<float>(s(14)));
    if (modern.active()) {
        renderer_.rounded(geometry.badge, static_cast<float>(s(15)), palette.accent_soft, palette.accent_soft);
        renderer_.rounded(geometry.search_frame, static_cast<float>(s(7)), palette.input_background,
                            GetFocus() == search_ ? palette.accent : palette.input_border);
        renderer_.rounded(geometry.table_frame, static_cast<float>(s(10)), palette.card_background, palette.card_border);
        RECT icon{geometry.search_frame.left + s(14), geometry.search_frame.top + s(12),
                    geometry.search_frame.left + s(34), geometry.search_frame.top + s(32)};
        renderer_.icon(5, icon, palette.muted_text, s(1) * 1.4f);
        if (GetFocus() == search_) {
            renderer_.line(static_cast<float>(geometry.search_frame.left + s(7)), static_cast<float>(geometry.search_frame.bottom - 1),
                             static_cast<float>(geometry.search_frame.right - s(7)), static_cast<float>(geometry.search_frame.bottom - 1),
                             palette.accent, static_cast<float>(s(2)));
        }
    } else {
        fill_solid_rect(hdc, geometry.search_frame, palette.input_background);
        fill_solid_rect(hdc, geometry.table_frame, palette.card_background);
    }
    draw_text(hdc, L"只读词库", geometry.badge, palette.accent, font_, static_cast<float>(s(12)), DT_SINGLELINE | DT_VCENTER | DT_CENTER);
}

bool DictionaryPreviewWindow::draw_list_row(const NMLVCUSTOMDRAW& custom) {
    const int row = static_cast<int>(custom.nmcd.dwItemSpec);
    RECT bounds{};
    if (!ListView_GetItemRect(list_, row, &bounds, LVIR_BOUNDS)) return false;
    ModernRenderScope modern(renderer_, custom.nmcd.hdc, bounds);
    if (!modern.active()) return false;
    const auto palette = palette_for(dark_theme_);
    const bool selected = (ListView_GetItemState(list_, row, LVIS_SELECTED) & LVIS_SELECTED) != 0;
    const auto style = user_lexicon_row_visual_style(palette, selected, GetFocus() == list_);
    renderer_.fill(bounds, style.background);
    int left = bounds.left;
    for (int column = 0; column < 4; ++column) {
        const int width = ListView_GetColumnWidth(list_, column);
        RECT cell{left + scale_settings_value(12, dpi_), bounds.top,
                     left + width - scale_settings_value(12, dpi_), bounds.bottom};
        left += width;
        wchar_t text[2048]{};
        ListView_GetItemText(list_, row, column, text, static_cast<int>(std::size(text)));
        renderer_.text(text, cell, column == 3 && !selected ? palette.muted_text : style.text,
                          11.0f * dpi_ / 72, FW_NORMAL,
                          DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | (column == 2 ? DT_RIGHT : DT_LEFT));
    }
    if (selected) {
        const float x = static_cast<float>(bounds.left + scale_settings_value(2, dpi_));
        renderer_.line(x, static_cast<float>(bounds.top + scale_settings_value(8, dpi_)), x,
                         static_cast<float>(bounds.bottom - scale_settings_value(8, dpi_)), palette.accent,
                         static_cast<float>(scale_settings_value(2, dpi_)));
    }
    return true;
}

void DictionaryPreviewWindow::paint_empty_list(HDC hdc) {
    if (ListView_GetItemCount(list_) != 0 || !hdc) return;
    RECT client{}; GetClientRect(list_, &client);
    RECT header{}; GetClientRect(ListView_GetHeader(list_), &header);
    RECT area{client.left, header.bottom, client.right, client.bottom};
    const auto palette = palette_for(dark_theme_);
    ModernRenderScope modern(renderer_, hdc, client);
    fill_solid_rect(hdc, area, palette.card_background);
    const int center_y = (area.top + area.bottom) / 2;
    RECT icon{(client.right - scale_settings_value(36, dpi_)) / 2, center_y - scale_settings_value(58, dpi_),
                 (client.right + scale_settings_value(36, dpi_)) / 2, center_y - scale_settings_value(22, dpi_)};
    if (modern.active()) renderer_.icon(5, icon, palette.weak_text, static_cast<float>(scale_settings_value(2, dpi_)));
    draw_text(hdc, L"没有找到匹配的词语", RECT{area.left, center_y, area.right, center_y + scale_settings_value(28, dpi_)},
                palette.text, font_, 11.0f * dpi_ / 72, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
    draw_text(hdc, L"试试更短的拼音或其他关键词", RECT{area.left, center_y + scale_settings_value(32, dpi_),
                area.right, center_y + scale_settings_value(58, dpi_)}, palette.muted_text, font_,
                11.0f * dpi_ / 72, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
}

LRESULT CALLBACK DictionaryPreviewWindow::control_subclass_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
    UINT_PTR subclass_id, DWORD_PTR ref_data) {
    auto* self = reinterpret_cast<DictionaryPreviewWindow*>(ref_data);
    if (!self) return DefSubclassProc(hwnd, message, wparam, lparam);
    const auto overlay = [&](HDC dc, bool printing) {
        if (hwnd == self->list_) { self->paint_empty_list(dc); return; }
        const bool search = hwnd == self->search_;
        const std::wstring value = window_text(hwnd);
        if (search && !printing && (!value.empty() || GetFocus() == hwnd)) return;
        RECT client{}; GetClientRect(hwnd, &client);
        const auto palette = palette_for(self->dark_theme_);
        ModernRenderScope modern(self->renderer_, dc, client);
        fill_solid_rect(dc, client, search ? palette.input_background : palette.window_background);
        draw_text(dc, search && value.empty() ? std::wstring(kDictionaryPreviewSearchPlaceholder) : value, client,
                    search ? (value.empty() ? palette.weak_text : palette.text) : palette.muted_text,
                    self->font_, 11.0f * self->dpi_ / 72);
    };
    if (message == WM_PAINT) {
        const LRESULT result = DefSubclassProc(hwnd, message, wparam, lparam);
        HDC dc = GetDC(hwnd);
        if (dc) { overlay(dc, false); ReleaseDC(hwnd, dc); }
        return result;
    }
    if (message == WM_PRINTCLIENT) {
        const LRESULT result = DefSubclassProc(hwnd, message, wparam, lparam);
        // Hidden native EDIT printing omits its glyphs. Print the actual text
        // here; normal caret/selection/TSF painting remains owned by EDIT.
        overlay(reinterpret_cast<HDC>(wparam), true);
        return result;
    }
    if (message == WM_NCDESTROY) RemoveWindowSubclass(hwnd, control_subclass_proc, subclass_id);
    return DefSubclassProc(hwnd, message, wparam, lparam);
}

void DictionaryPreviewWindow::install_list_header_theme_subclass() {
    const HWND header = ListView_GetHeader(list_);
    if (header) SetWindowSubclass(header, list_header_subclass_proc, kListHeaderSubclassId, reinterpret_cast<DWORD_PTR>(this));
}

void DictionaryPreviewWindow::paint_list_header(HWND header, HDC hdc) {
    if (!header || !hdc) return;
    RECT client{}; GetClientRect(header, &client);
    const auto palette = palette_for(dark_theme_);
    ModernRenderScope modern(renderer_, hdc, client);
    fill_solid_rect(hdc, client, palette.list_header_background);
    for (int column = 0; column < Header_GetItemCount(header); ++column) {
        RECT rect{};
        if (!Header_GetItemRect(header, column, &rect)) continue;
        wchar_t text[64]{};
        HDITEMW item{}; item.mask = HDI_TEXT; item.pszText = text; item.cchTextMax = static_cast<int>(std::size(text));
        Header_GetItem(header, column, &item);
        rect.left += scale_settings_value(12, dpi_); rect.right -= scale_settings_value(12, dpi_);
        draw_text(hdc, text, rect, palette.muted_text, font_, 11.0f * dpi_ / 72,
                     DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | (column == 2 ? DT_RIGHT : DT_LEFT));
    }
    RECT separator{client.left, client.bottom - 1, client.right, client.bottom};
    fill_solid_rect(hdc, separator, palette.card_border);
}

LRESULT CALLBACK DictionaryPreviewWindow::list_header_subclass_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
    UINT_PTR subclass_id, DWORD_PTR ref_data) {
    auto* self = reinterpret_cast<DictionaryPreviewWindow*>(ref_data);
    switch (message) {
        case WM_ERASEBKGND: return 1;
        case WM_PAINT: {
            PAINTSTRUCT paint{}; HDC dc = BeginPaint(hwnd, &paint);
            if (self) self->paint_list_header(hwnd, dc);
            EndPaint(hwnd, &paint); return 0;
        }
        case WM_PRINTCLIENT:
            if (self) self->paint_list_header(hwnd, reinterpret_cast<HDC>(wparam));
            return 0;
        case WM_THEMECHANGED:
        case WM_SETTINGCHANGE: InvalidateRect(hwnd, nullptr, FALSE); break;
        case WM_NCDESTROY: RemoveWindowSubclass(hwnd, list_header_subclass_proc, subclass_id); break;
        default: break;
    }
    return DefSubclassProc(hwnd, message, wparam, lparam);
}

}  // namespace localpinyin
