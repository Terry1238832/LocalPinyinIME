#include "dictionary_preview_window.h"

#include "settings_window.h"

#include <commctrl.h>

#include <algorithm>
#include <string>

namespace localpinyin {
namespace {
constexpr int kSearchId = 1;
constexpr int kListId = 2;
constexpr int kStatusId = 3;
constexpr UINT_PTR kListHeaderSubclassId = 1;

void fill_solid_rect(HDC hdc, const RECT& rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);
}

void draw_text_in_rect(HDC hdc, const std::wstring& text, const RECT& rect, COLORREF color, HFONT font, UINT format) {
    HGDIOBJ old_font = font ? SelectObject(hdc, font) : nullptr;
    const int old_mode = SetBkMode(hdc, TRANSPARENT);
    const COLORREF old_text = SetTextColor(hdc, color);
    RECT copy = rect;
    DrawTextW(hdc, text.c_str(), static_cast<int>(text.size()), &copy, format);
    SetTextColor(hdc, old_text);
    SetBkMode(hdc, old_mode);
    if (old_font) {
        SelectObject(hdc, old_font);
    }
}

std::wstring window_text(HWND hwnd) {
    const int length = GetWindowTextLengthW(hwnd);
    std::wstring value(static_cast<size_t>(std::max(0, length)) + 1, L'\0');
    if (length > 0) GetWindowTextW(hwnd, value.data(), length + 1);
    value.resize(static_cast<size_t>(std::max(0, length)));
    return value;
}
}  // namespace

DictionaryPreviewWindow::DictionaryPreviewWindow(const DictionaryCatalog& catalog, bool dark_theme)
    : catalog_(catalog), dark_theme_(dark_theme) {}

HWND DictionaryPreviewWindow::show(HWND owner, HINSTANCE instance,
                                   const DictionaryCatalog& catalog, bool dark_theme) {
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.hInstance = instance;
        wc.lpfnWndProc = window_proc;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.lpszClassName = L"LocalPinyinDictionaryPreviewWindow";
        wc.hbrBackground = nullptr;
        registered = RegisterClassExW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    }
    if (!registered) return nullptr;
    auto* self = new DictionaryPreviewWindow(catalog, dark_theme);
    HWND hwnd = CreateWindowExW(WS_EX_APPWINDOW, L"LocalPinyinDictionaryPreviewWindow", L"词库预览",
                                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                CW_USEDEFAULT, CW_USEDEFAULT, 820, 620, owner, nullptr, instance, self);
    if (!hwnd) { delete self; return nullptr; }
    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);
    return hwnd;
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
        delete self;
    }
    return result;
}

LRESULT DictionaryPreviewWindow::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
        case WM_CREATE: apply_theme(); create_controls(); layout(); populate(); return 0;
        case WM_SIZE: layout(); return 0;
        case WM_COMMAND:
            if (LOWORD(wparam) == kSearchId && HIWORD(wparam) == EN_CHANGE) populate();
            return 0;
        case WM_NOTIFY: {
            const auto* header = reinterpret_cast<NMHDR*>(lparam);
            const HWND list_header = list_ ? ListView_GetHeader(list_) : nullptr;
            if (header && list_header && header->hwndFrom == list_header && header->code == NM_CUSTOMDRAW) {
                auto* custom = reinterpret_cast<NMCUSTOMDRAW*>(lparam);
                if (custom->dwDrawStage == CDDS_PREPAINT) {
                    const SettingsHeaderDrawStyle style =
                        settings_header_draw_style(settings_theme_palette(dark_theme_ ? SettingsResolvedTheme::Dark
                                                                                      : SettingsResolvedTheme::Light));
                    fill_solid_rect(custom->hdc, custom->rc, style.background);
                    return CDRF_NOTIFYITEMDRAW;
                }
                if (custom->dwDrawStage == CDDS_ITEMPREPAINT) {
                    return draw_list_header_item(*custom) ? CDRF_SKIPDEFAULT : CDRF_DODEFAULT;
                }
            }
            break;
        }
        case WM_ERASEBKGND: {
            RECT rect{}; GetClientRect(hwnd_, &rect);
            FillRect(reinterpret_cast<HDC>(wparam), &rect, background_);
            return 1;
        }
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT: {
            HDC dc = reinterpret_cast<HDC>(wparam);
            const SettingsThemePalette palette =
                settings_theme_palette(dark_theme_ ? SettingsResolvedTheme::Dark : SettingsResolvedTheme::Light);
            SetTextColor(dc, palette.text);
            SetBkColor(dc, palette.window_background);
            return reinterpret_cast<LRESULT>(background_);
        }
        case WM_DESTROY:
            if (font_) DeleteObject(font_);
            if (background_) DeleteObject(background_);
            font_ = nullptr; background_ = nullptr;
            return 0;
        default: return DefWindowProcW(hwnd_, message, wparam, lparam);
    }
    return DefWindowProcW(hwnd_, message, wparam, lparam);
}

void DictionaryPreviewWindow::create_controls() {
    const int dpi = GetDpiForWindow(hwnd_);
    const SettingsThemePalette palette =
        settings_theme_palette(dark_theme_ ? SettingsResolvedTheme::Dark : SettingsResolvedTheme::Light);
    LOGFONTW lf{};
    lf.lfHeight = -MulDiv(10, dpi, 72);
    wcscpy_s(lf.lfFaceName, L"Segoe UI");
    font_ = CreateFontIndirectW(&lf);
    background_ = CreateSolidBrush(palette.window_background);
    search_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                              0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSearchId)), nullptr, nullptr);
    SendMessageW(search_, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(kDictionaryPreviewSearchPlaceholder));
    list_ = CreateWindowExW(0, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL,
                            0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kListId)), nullptr, nullptr);
    ListView_SetExtendedListViewStyle(list_, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    status_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd_,
                              reinterpret_cast<HMENU>(static_cast<INT_PTR>(kStatusId)), nullptr, nullptr);
    for (HWND child : {search_, list_, status_}) SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
    const wchar_t* names[] = {L"拼音", L"词语", L"权重", L"来源"};
    for (int i = 0; i < 4; ++i) {
        LVCOLUMNW column{}; column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        column.pszText = const_cast<wchar_t*>(names[i]); column.iSubItem = i;
        column.cx = i == 1 ? 280 : (i == 0 ? 180 : 110);
        ListView_InsertColumn(list_, i, &column);
    }
    apply_theme();
    install_list_header_theme_subclass();
}

void DictionaryPreviewWindow::layout() {
    if (!hwnd_) return;
    RECT client{}; GetClientRect(hwnd_, &client);
    const int dpi = GetDpiForWindow(hwnd_); const int m = MulDiv(20, dpi, 96); const int h = MulDiv(36, dpi, 96);
    const int width = std::max<int>(100, static_cast<int>(client.right) - m * 2);
    const int list_height = std::max<int>(100, static_cast<int>(client.bottom) -
                                               (m * 3 + h + MulDiv(28, dpi, 96)));
    MoveWindow(search_, m, m, width, h, TRUE);
    MoveWindow(list_, m, m + h + MulDiv(12, dpi, 96), width, list_height, TRUE);
    MoveWindow(status_, m, static_cast<int>(client.bottom) - m - MulDiv(24, dpi, 96),
               width, MulDiv(24, dpi, 96), TRUE);
}

void DictionaryPreviewWindow::populate() {
    const std::wstring query = window_text(search_);
    const auto rows = catalog_.preview(query, query.empty() ? 200 : 500);
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
    const std::wstring status = L"显示 " + std::to_wstring(rows.size()) + L" 条 / 总计 " +
                                std::to_wstring(catalog_.stats().total_unique_entries) + L" 条（只读）";
    SetWindowTextW(status_, status.c_str());
}

void DictionaryPreviewWindow::apply_theme() {
    if (!hwnd_) {
        return;
    }
    const SettingsThemePalette palette =
        settings_theme_palette(dark_theme_ ? SettingsResolvedTheme::Dark : SettingsResolvedTheme::Light);
    apply_settings_dark_title_bar(hwnd_, palette.dark);
    if (list_) {
        ListView_SetBkColor(list_, palette.window_background);
        ListView_SetTextBkColor(list_, palette.window_background);
        ListView_SetTextColor(list_, palette.text);
        const HWND header = ListView_GetHeader(list_);
        if (header) {
            InvalidateRect(header, nullptr, TRUE);
        }
    }
}

void DictionaryPreviewWindow::install_list_header_theme_subclass() {
    const HWND header = list_ ? ListView_GetHeader(list_) : nullptr;
    if (!header) {
        return;
    }
    SetWindowSubclass(header,
                      DictionaryPreviewWindow::list_header_subclass_proc,
                      kListHeaderSubclassId,
                      reinterpret_cast<DWORD_PTR>(this));
    InvalidateRect(header, nullptr, TRUE);
}

bool DictionaryPreviewWindow::draw_list_header_item(const NMCUSTOMDRAW& custom_draw) {
    if (!custom_draw.hdc || !custom_draw.hdr.hwndFrom) {
        return false;
    }
    wchar_t text[64]{};
    HDITEMW item{};
    item.mask = HDI_TEXT;
    item.pszText = text;
    item.cchTextMax = static_cast<int>(sizeof(text) / sizeof(text[0]));
    if (!Header_GetItem(custom_draw.hdr.hwndFrom, static_cast<int>(custom_draw.dwItemSpec), &item)) {
        return false;
    }

    const int dpi = GetDpiForWindow(hwnd_);
    const SettingsHeaderDrawStyle style =
        settings_header_draw_style(settings_theme_palette(dark_theme_ ? SettingsResolvedTheme::Dark
                                                                      : SettingsResolvedTheme::Light));
    RECT rect = custom_draw.rc;
    fill_solid_rect(custom_draw.hdc, rect, style.background);
    RECT text_rect = rect;
    text_rect.left += scale_settings_value(12, dpi);
    text_rect.right -= scale_settings_value(8, dpi);
    draw_text_in_rect(custom_draw.hdc,
                      text,
                      text_rect,
                      style.text,
                      font_,
                      DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);

    HPEN pen = CreatePen(PS_SOLID, 1, style.separator);
    HGDIOBJ old_pen = SelectObject(custom_draw.hdc, pen);
    MoveToEx(custom_draw.hdc, rect.right - 1, rect.top + scale_settings_value(5, dpi), nullptr);
    LineTo(custom_draw.hdc, rect.right - 1, rect.bottom - scale_settings_value(5, dpi));
    MoveToEx(custom_draw.hdc, rect.left, rect.bottom - 1, nullptr);
    LineTo(custom_draw.hdc, rect.right, rect.bottom - 1);
    SelectObject(custom_draw.hdc, old_pen);
    DeleteObject(pen);
    return true;
}

void DictionaryPreviewWindow::paint_list_header(HWND header, HDC hdc) {
    if (!header || !hdc) {
        return;
    }
    const int dpi = GetDpiForWindow(hwnd_);
    const SettingsHeaderDrawStyle style =
        settings_header_draw_style(settings_theme_palette(dark_theme_ ? SettingsResolvedTheme::Dark
                                                                      : SettingsResolvedTheme::Light));
    RECT client{};
    GetClientRect(header, &client);
    fill_solid_rect(hdc, client, style.background);

    const int item_count = Header_GetItemCount(header);
    for (int column = 0; column < item_count; ++column) {
        RECT item_rect{};
        if (!Header_GetItemRect(header, column, &item_rect)) {
            continue;
        }

        wchar_t text[64]{};
        HDITEMW item{};
        item.mask = HDI_TEXT;
        item.pszText = text;
        item.cchTextMax = static_cast<int>(sizeof(text) / sizeof(text[0]));
        Header_GetItem(header, column, &item);

        fill_solid_rect(hdc, item_rect, style.background);
        RECT text_rect = item_rect;
        text_rect.left += scale_settings_value(12, dpi);
        text_rect.right -= scale_settings_value(8, dpi);
        draw_text_in_rect(hdc,
                          text,
                          text_rect,
                          style.text,
                          font_,
                          DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);

        HPEN pen = CreatePen(PS_SOLID, 1, style.separator);
        HGDIOBJ old_pen = SelectObject(hdc, pen);
        MoveToEx(hdc, item_rect.right - 1, item_rect.top + scale_settings_value(5, dpi), nullptr);
        LineTo(hdc, item_rect.right - 1, item_rect.bottom - scale_settings_value(5, dpi));
        SelectObject(hdc, old_pen);
        DeleteObject(pen);
    }

    HPEN border_pen = CreatePen(PS_SOLID, 1, style.border);
    HGDIOBJ old_pen = SelectObject(hdc, border_pen);
    MoveToEx(hdc, client.left, client.bottom - 1, nullptr);
    LineTo(hdc, client.right, client.bottom - 1);
    SelectObject(hdc, old_pen);
    DeleteObject(border_pen);
}

LRESULT CALLBACK DictionaryPreviewWindow::list_header_subclass_proc(
    HWND hwnd,
    UINT message,
    WPARAM wparam,
    LPARAM lparam,
    UINT_PTR subclass_id,
    DWORD_PTR ref_data) {
    (void)subclass_id;
    auto* self = reinterpret_cast<DictionaryPreviewWindow*>(ref_data);
    switch (message) {
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC hdc = BeginPaint(hwnd, &ps);
            if (self) {
                self->paint_list_header(hwnd, hdc);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_THEMECHANGED:
        case WM_SETTINGCHANGE:
            InvalidateRect(hwnd, nullptr, TRUE);
            break;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, DictionaryPreviewWindow::list_header_subclass_proc, kListHeaderSubclassId);
            break;
        default:
            break;
    }
    return DefSubclassProc(hwnd, message, wparam, lparam);
}

}  // namespace localpinyin
