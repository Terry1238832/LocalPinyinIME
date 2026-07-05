#include "settings_window.h"

#include "../common/logging.h"
#include "localpinyin_version.h"
#include "local_pinyin_blue_theme.h"
#include "resource.h"

#include <commctrl.h>
#include <shellapi.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <sstream>
#include <vector>

namespace localpinyin {
namespace {

constexpr int kBaseDpi = 96;
constexpr int kIdOpenDataFolder = 1001;
constexpr int kIdCopyDataFolder = 1002;
constexpr int kIdOpenGithubProject = 1003;
constexpr int kIdCopyGithubUrl = 1004;
constexpr int kIdThemeSystem = 1101;
constexpr int kIdThemeLight = 1102;
constexpr int kIdThemeDark = 1103;
constexpr int kIdShowKeyHints = 1201;
constexpr int kIdShiftToggle = 1202;
constexpr int kIdTextSmall = 1301;
constexpr int kIdTextStandard = 1302;
constexpr int kIdTextLarge = 1303;
constexpr int kIdClearLearning = 1401;
constexpr int kIdLexiconSearch = 1501;
constexpr int kIdLexiconList = 1502;
constexpr int kIdLexiconPinyin = 1503;
constexpr int kIdLexiconWord = 1504;
constexpr int kIdLexiconFrequency = 1505;
constexpr int kIdLexiconSave = 1506;
constexpr int kIdLexiconDelete = 1507;
constexpr int kIdLexiconRefresh = 1508;
constexpr int kIdLexiconStatus = 1509;
constexpr int kIdLexiconEmptyState = 1510;
constexpr int kIdLexiconNew = 1511;

constexpr int kIdNavAppearance = 2001;
constexpr int kIdNavCandidate = 2002;
constexpr int kIdNavUserLexicon = 2003;
constexpr int kIdNavLearning = 2004;
constexpr int kIdNavAbout = 2005;
constexpr int kIdAppTitle = 2010;
constexpr int kIdSidebarDivider = 2011;

constexpr int kIdAppearanceTitle = 3001;
constexpr int kIdAppearanceDescription = 3002;
constexpr int kIdAppearanceCard = 3003;
constexpr int kIdAppearanceThemeLabel = 3004;

constexpr int kIdCandidateTitle = 3101;
constexpr int kIdCandidateDescription = 3102;
constexpr int kIdCandidateCard = 3103;
constexpr int kIdCandidateHintLabel = 3104;
constexpr int kIdCandidateTextSizeLabel = 3105;

constexpr int kIdUserLexiconTitle = 3201;
constexpr int kIdUserLexiconDescription = 3202;
constexpr int kIdUserLexiconTableCard = 3203;
constexpr int kIdUserLexiconEditorCard = 3204;
constexpr int kIdLexiconPinyinLabel = 3205;
constexpr int kIdLexiconWordLabel = 3206;
constexpr int kIdLexiconFrequencyLabel = 3207;
constexpr int kIdUserLexiconEditorTitle = 3208;

constexpr int kIdLearningTitle = 3301;
constexpr int kIdLearningDescription = 3302;
constexpr int kIdLearningCard = 3303;
constexpr int kIdLearningCardText = 3304;

constexpr int kIdAboutTitle = 3401;
constexpr int kIdAboutDescription = 3402;
constexpr int kIdAboutCard = 3403;
constexpr int kIdAboutProductKey = 3404;
constexpr int kIdAboutProductValue = 3405;
constexpr int kIdAboutVersionKey = 3406;
constexpr int kIdAboutVersionValue = 3407;
constexpr int kIdAboutArchKey = 3408;
constexpr int kIdAboutArchValue = 3409;
constexpr int kIdAboutOfflineKey = 3410;
constexpr int kIdAboutOfflineValue = 3411;
constexpr int kIdAboutStatusKey = 3412;
constexpr int kIdAboutStatusValue = 3413;
constexpr int kIdAboutDataDirKey = 3414;
constexpr int kIdAboutDataDirValue = 3415;
constexpr int kIdAboutGithubKey = 3416;
constexpr int kIdAboutGithubValue = 3417;

constexpr int kSettingsPageCount = 5;
constexpr UINT_PTR kUserLexiconHeaderSubclassId = 1;
constexpr wchar_t kGithubProjectUrl[] = L"https://github.com/Terry1238832/LocalPinyinIME";

COLORREF rgb_hex(int value) {
    return RGB((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
}

int page_index(SettingsPage page) {
    return static_cast<int>(page);
}

int nav_id_for_page(SettingsPage page) {
    switch (page) {
        case SettingsPage::Appearance:
            return kIdNavAppearance;
        case SettingsPage::Candidate:
            return kIdNavCandidate;
        case SettingsPage::UserLexicon:
            return kIdNavUserLexicon;
        case SettingsPage::Learning:
            return kIdNavLearning;
        case SettingsPage::About:
            return kIdNavAbout;
    }
    return kIdNavUserLexicon;
}

SettingsPage page_for_nav_id(int id) {
    switch (id) {
        case kIdNavAppearance:
            return SettingsPage::Appearance;
        case kIdNavCandidate:
            return SettingsPage::Candidate;
        case kIdNavUserLexicon:
            return SettingsPage::UserLexicon;
        case kIdNavLearning:
            return SettingsPage::Learning;
        case kIdNavAbout:
            return SettingsPage::About;
        default:
            return SettingsPage::UserLexicon;
    }
}

int rect_right(const SettingsRect& rect) {
    return rect.x + rect.width;
}

int rect_bottom(const SettingsRect& rect) {
    return rect.y + rect.height;
}

RECT to_win_rect(const SettingsRect& rect) {
    return RECT{rect.x, rect.y, rect.x + rect.width, rect.y + rect.height};
}

int rect_width(const RECT& rect) {
    return std::max<LONG>(0, rect.right - rect.left);
}

int rect_height(const RECT& rect) {
    return std::max<LONG>(0, rect.bottom - rect.top);
}

int size_width(const SettingsSize& size) {
    return std::max(0, size.width);
}

int size_height(const SettingsSize& size) {
    return std::max(0, size.height);
}

COLORREF blend(COLORREF from, COLORREF to, int percent_to) {
    const int keep = 100 - percent_to;
    return RGB((GetRValue(from) * keep + GetRValue(to) * percent_to) / 100,
               (GetGValue(from) * keep + GetGValue(to) * percent_to) / 100,
               (GetBValue(from) * keep + GetBValue(to) * percent_to) / 100);
}

void fill_rect(HDC hdc, const RECT& rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);
}

void draw_round_rect(HDC hdc, const RECT& rect, int radius, COLORREF fill, COLORREF border) {
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ old_brush = SelectObject(hdc, brush);
    HGDIOBJ old_pen = SelectObject(hdc, pen);
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius * 2, radius * 2);
    SelectObject(hdc, old_pen);
    SelectObject(hdc, old_brush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void draw_text_in_rect(HDC hdc, const std::wstring& text, const RECT& rect, COLORREF color, HFONT font, UINT flags) {
    HGDIOBJ old_font = nullptr;
    if (font) {
        old_font = SelectObject(hdc, font);
    }
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, color);
    RECT draw_rect = rect;
    DrawTextW(hdc, text.c_str(), static_cast<int>(text.size()), &draw_rect, flags | DT_NOPREFIX);
    if (old_font) {
        SelectObject(hdc, old_font);
    }
}

void move_window(HWND hwnd, const SettingsRect& rect) {
    if (hwnd) {
        MoveWindow(hwnd, rect.x, rect.y, rect.width, rect.height, TRUE);
    }
}

void move_child(HWND parent, int id, const SettingsRect& rect) {
    move_window(GetDlgItem(parent, id), rect);
}

void move_child(HWND parent, int id, int x, int y, int width, int height) {
    move_child(parent, id, SettingsRect{x, y, width, height});
}

void set_control_font(HWND hwnd, HFONT font) {
    if (hwnd && font) {
        SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }
}

HFONT create_ui_font(int dpi, int point_size, LONG weight) {
    LOGFONTW font{};
    font.lfHeight = -MulDiv(point_size, dpi, 72);
    font.lfWeight = weight;
    wcscpy_s(font.lfFaceName, L"Segoe UI Variable");
    return CreateFontIndirectW(&font);
}

HWND make_control(HWND parent,
                  const wchar_t* class_name,
                  const wchar_t* text,
                  int id,
                  DWORD style,
                  DWORD ex_style,
                  HFONT font) {
    const auto control_id = reinterpret_cast<HMENU>(static_cast<INT_PTR>(id));
    HWND hwnd = CreateWindowExW(ex_style,
                                class_name,
                                text,
                                WS_CHILD | WS_VISIBLE | style,
                                0,
                                0,
                                10,
                                10,
                                parent,
                                control_id,
                                nullptr,
                                nullptr);
    set_control_font(hwnd, font);
    return hwnd;
}

HWND make_static(HWND parent, const wchar_t* text, int id, HFONT font) {
    return make_control(parent, L"STATIC", text, id, 0, 0, font);
}

HWND make_group(HWND parent, const wchar_t* text, int id, HFONT font) {
    (void)text;
    return make_control(parent, L"STATIC", L"", id, SS_LEFT, 0, font);
}

HWND make_button(HWND parent, const wchar_t* text, int id, DWORD style, HFONT font) {
    // For child controls, CreateWindowExW's hMenu parameter carries the integer control ID.
    return make_control(parent, L"BUTTON", text, id, (style & ~0x0000000FL) | BS_OWNERDRAW | WS_TABSTOP, 0, font);
}

HWND make_edit(HWND parent, int id, HFONT font) {
    return make_control(parent,
                        L"EDIT",
                        L"",
                        id,
                        ES_AUTOHSCROLL,
                        0,
                        font);
}

HWND make_list_view(HWND parent, int id, HFONT font) {
    HWND list = make_control(parent,
                             WC_LISTVIEWW,
                             L"",
                             id,
                             LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                             0,
                             font);
    const SettingsThemePalette palette = settings_theme_palette(SettingsResolvedTheme::Light);
    ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    ListView_SetBkColor(list, palette.card_background);
    ListView_SetTextBkColor(list, palette.card_background);
    ListView_SetTextColor(list, palette.text);
    return list;
}

bool is_checked(HWND parent, int id) {
    return SendMessageW(GetDlgItem(parent, id), BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void set_checked(HWND parent, int id, bool checked) {
    SendMessageW(GetDlgItem(parent, id), BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

std::wstring control_text(HWND hwnd) {
    const int length = GetWindowTextLengthW(hwnd);
    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    if (length > 0) {
        GetWindowTextW(hwnd, text.data(), length + 1);
    }
    text.resize(static_cast<size_t>(length));
    return text;
}

std::wstring control_text(HWND parent, int id) {
    return control_text(GetDlgItem(parent, id));
}

void set_control_text(HWND parent, int id, const std::wstring& text) {
    SetWindowTextW(GetDlgItem(parent, id), text.c_str());
}

void set_cue_banner(HWND edit, const wchar_t* text) {
    SendMessageW(edit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(text));
}

bool windows_apps_use_dark_theme() {
    DWORD value = 1;
    DWORD size = sizeof(value);
    const LSTATUS status = RegGetValueW(HKEY_CURRENT_USER,
                                        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                                        L"AppsUseLightTheme",
                                        RRF_RT_REG_DWORD,
                                        nullptr,
                                        &value,
                                        &size);
    return status == ERROR_SUCCESS && value == 0;
}

RECT primary_work_area_near_cursor() {
    POINT cursor{};
    if (!GetCursorPos(&cursor)) {
        cursor = POINT{0, 0};
    }
    HMONITOR monitor = MonitorFromPoint(cursor, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    if (monitor && GetMonitorInfoW(monitor, &info)) {
        return info.rcWork;
    }
    RECT work_area{};
    if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_area, 0)) {
        return work_area;
    }
    return RECT{0, 0, 1280, 720};
}

void apply_dark_title_bar(HWND hwnd, bool dark) {
    using DwmSetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
    HMODULE dwm = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!dwm) {
        return;
    }
    auto* set_window_attribute = reinterpret_cast<DwmSetWindowAttributeFn>(
        GetProcAddress(dwm, "DwmSetWindowAttribute"));
    if (set_window_attribute) {
        const BOOL enabled = dark ? TRUE : FALSE;
        constexpr DWORD kDwmUseImmersiveDarkMode = 20;
        constexpr DWORD kDwmUseImmersiveDarkModeBefore20H1 = 19;
        if (FAILED(set_window_attribute(hwnd,
                                        kDwmUseImmersiveDarkMode,
                                        &enabled,
                                        sizeof(enabled)))) {
            (void)set_window_attribute(hwnd,
                                      kDwmUseImmersiveDarkModeBefore20H1,
                                      &enabled,
                                      sizeof(enabled));
        }
    }
    FreeLibrary(dwm);
}

std::wstring compact_path_for_width(HWND hwnd, HFONT font, const std::wstring& path, int width_px) {
    if (path.empty() || width_px <= 0) {
        return path;
    }

    using PathCompactPathWFn = BOOL(WINAPI*)(HDC, LPWSTR, UINT);
    HMODULE shlwapi = LoadLibraryExW(L"shlwapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!shlwapi) {
        return path;
    }
    auto* compact_path = reinterpret_cast<PathCompactPathWFn>(GetProcAddress(shlwapi, "PathCompactPathW"));
    if (!compact_path) {
        FreeLibrary(shlwapi);
        return path;
    }

    HDC hdc = GetDC(hwnd);
    if (!hdc) {
        FreeLibrary(shlwapi);
        return path;
    }
    HGDIOBJ old_font = nullptr;
    if (font) {
        old_font = SelectObject(hdc, font);
    }

    std::vector<wchar_t> buffer(path.begin(), path.end());
    buffer.push_back(L'\0');
    const BOOL compacted = compact_path(hdc, buffer.data(), static_cast<UINT>(width_px));

    if (old_font) {
        SelectObject(hdc, old_font);
    }
    ReleaseDC(hwnd, hdc);
    FreeLibrary(shlwapi);

    if (!compacted) {
        return path;
    }
    return std::wstring(buffer.data());
}

}  // namespace

int scale_settings_value(int value, int dpi) {
    return MulDiv(value, dpi <= 0 ? kBaseDpi : dpi, kBaseDpi);
}

SettingsSize settings_minimum_client_size(int dpi) {
    const int safe_dpi = dpi <= 0 ? kBaseDpi : dpi;
    return SettingsSize{scale_settings_value(980, safe_dpi),
                        scale_settings_value(700, safe_dpi)};
}

SettingsSize settings_initial_client_size(int dpi) {
    const int safe_dpi = dpi <= 0 ? kBaseDpi : dpi;
    const SettingsSize minimum = settings_minimum_client_size(safe_dpi);
    return SettingsSize{std::max(minimum.width, scale_settings_value(1040, safe_dpi)),
                        std::max(minimum.height, scale_settings_value(740, safe_dpi))};
}

SettingsSize settings_outer_size_for_client(
    SettingsSize client_size,
    int dpi,
    DWORD style,
    DWORD ex_style) {
    const int safe_dpi = dpi <= 0 ? kBaseDpi : dpi;
    RECT rect{0, 0, size_width(client_size), size_height(client_size)};
    if (!AdjustWindowRectExForDpi(&rect, style, FALSE, ex_style, static_cast<UINT>(safe_dpi))) {
        AdjustWindowRectEx(&rect, style, FALSE, ex_style);
    }
    return SettingsSize{rect_width(rect), rect_height(rect)};
}

SettingsRect calculate_settings_initial_window_rect(
    const RECT& work_area,
    int dpi,
    DWORD style,
    DWORD ex_style) {
    const SettingsSize client = settings_initial_client_size(dpi);
    const SettingsSize outer = settings_outer_size_for_client(client, dpi, style, ex_style);
    const int work_width = rect_width(work_area);
    const int work_height = rect_height(work_area);
    const int width = std::min(size_width(outer), work_width > 0 ? work_width : size_width(outer));
    const int height = std::min(size_height(outer), work_height > 0 ? work_height : size_height(outer));
    const int x = work_area.left + std::max(0, (work_width - width) / 2);
    const int y = work_area.top + std::max(0, (work_height - height) / 2);
    return SettingsRect{x, y, width, height};
}

SettingsResolvedTheme resolve_settings_theme(CandidateThemeMode mode, bool system_uses_dark_theme) {
    if (mode == CandidateThemeMode::Dark) {
        return SettingsResolvedTheme::Dark;
    }
    if (mode == CandidateThemeMode::Light) {
        return SettingsResolvedTheme::Light;
    }
    return system_uses_dark_theme ? SettingsResolvedTheme::Dark : SettingsResolvedTheme::Light;
}

SettingsThemePalette settings_theme_palette(SettingsResolvedTheme theme) {
    if (theme == SettingsResolvedTheme::Dark) {
        return SettingsThemePalette{
            true,
            brand::kLocalPinyinBlueDarkWindow_background,
            brand::kLocalPinyinBlueDarkSidebar_background,
            brand::kLocalPinyinBlueDarkCard_background,
            brand::kLocalPinyinBlueDarkCard_border,
            brand::kLocalPinyinBlueDarkPrimary,
            brand::kLocalPinyinBlueDarkMuted,
            brand::kLocalPinyinBlueDarkWeak,
            brand::kLocalPinyinBlueDarkAccent,
            brand::kLocalPinyinBlueDarkAccent_soft,
            brand::kLocalPinyinBlueDarkAccent_border,
            brand::kLocalPinyinBlueDarkInput_background,
            brand::kLocalPinyinBlueDarkInput_border,
            brand::kLocalPinyinBlueDarkSecondary_button,
            brand::kLocalPinyinBlueDarkSecondary_button_pressed,
            brand::kLocalPinyinBlueDarkPrimary_button_pressed,
            brand::kLocalPinyinBlueDarkDanger,
            brand::kLocalPinyinBlueDarkDanger_soft,
            brand::kLocalPinyinBlueDarkDanger_border,
            brand::kLocalPinyinBlueDarkDisabled_background,
            brand::kLocalPinyinBlueDarkDisabled_text,
            brand::kLocalPinyinBlueDarkList_header_background,
            brand::kLocalPinyinBlueDarkList_header,
            brand::kLocalPinyinBlueDarkInactive_selection
        };
    }
    return SettingsThemePalette{
        false,
        brand::kLocalPinyinBlueLightWindow_background,
        brand::kLocalPinyinBlueLightSidebar_background,
        brand::kLocalPinyinBlueLightCard_background,
        brand::kLocalPinyinBlueLightCard_border,
        brand::kLocalPinyinBlueLightPrimary,
        brand::kLocalPinyinBlueLightMuted,
        brand::kLocalPinyinBlueLightWeak,
        brand::kLocalPinyinBlueLightAccent,
        brand::kLocalPinyinBlueLightAccent_soft,
        brand::kLocalPinyinBlueLightAccent_border,
        brand::kLocalPinyinBlueLightInput_background,
        brand::kLocalPinyinBlueLightInput_border,
        brand::kLocalPinyinBlueLightSecondary_button,
        brand::kLocalPinyinBlueLightSecondary_button_pressed,
        brand::kLocalPinyinBlueLightPrimary_button_pressed,
        brand::kLocalPinyinBlueLightDanger,
        brand::kLocalPinyinBlueLightDanger_soft,
        brand::kLocalPinyinBlueLightDanger_border,
        brand::kLocalPinyinBlueLightDisabled_background,
        brand::kLocalPinyinBlueLightDisabled_text,
        brand::kLocalPinyinBlueLightList_header_background,
        brand::kLocalPinyinBlueLightList_header,
        brand::kLocalPinyinBlueLightInactive_selection
    };
}

SettingsControlDrawStyle settings_control_draw_style(
    const SettingsThemePalette& palette,
    SettingsControlRole role,
    SettingsControlSurface surface,
    const SettingsControlDrawState& state) {
    SettingsControlDrawStyle style;
    switch (surface) {
        case SettingsControlSurface::Sidebar:
            style.background = palette.sidebar_background;
            break;
        case SettingsControlSurface::Card:
            style.background = palette.card_background;
            break;
        case SettingsControlSurface::Window:
        default:
            style.background = palette.window_background;
            break;
    }

    style.fill = palette.secondary_button;
    style.border = palette.card_border;
    style.text = state.disabled ? palette.disabled_text : palette.text;
    style.mark_fill = palette.card_background;
    style.mark_border = palette.card_border;
    style.fills_entire_client_rect = true;
    style.uses_focus_border = state.focused;

    switch (role) {
        case SettingsControlRole::Navigation:
            style.fill = state.checked ? palette.accent_soft : palette.sidebar_background;
            if (state.hot && !state.checked && !state.disabled) {
                style.fill = blend(style.fill, palette.text, palette.dark ? 7 : 4);
            }
            style.border = style.fill;
            break;
        case SettingsControlRole::Option:
            style.fill = state.checked ? palette.accent_soft : palette.card_background;
            if (state.hot && !state.checked && !state.disabled) {
                style.fill = blend(style.fill, palette.accent, palette.dark ? 10 : 6);
            }
            style.border = state.checked ? palette.accent_border : palette.card_border;
            style.uses_option_mark = true;
            style.mark_fill = state.checked ? palette.accent : style.fill;
            style.mark_border = state.checked ? palette.accent : palette.card_border;
            break;
        case SettingsControlRole::PrimaryButton:
            style.fill = state.disabled ? palette.disabled_background :
                         state.pressed ? palette.primary_button_pressed :
                         state.hot ? blend(palette.accent, RGB(255, 255, 255), palette.dark ? 8 : 5) :
                         palette.accent;
            style.border = style.fill;
            style.text = state.disabled ? palette.disabled_text : RGB(255, 255, 255);
            break;
        case SettingsControlRole::DangerButton:
            style.fill = state.disabled ? palette.disabled_background :
                         state.hot ? blend(palette.danger_soft, palette.danger, palette.dark ? 10 : 6) :
                         palette.danger_soft;
            if (state.pressed && !state.disabled) {
                style.fill = blend(style.fill, RGB(0, 0, 0), 8);
            }
            style.border = state.disabled ? palette.card_border : palette.danger_border;
            style.text = state.disabled ? palette.disabled_text : palette.danger;
            break;
        case SettingsControlRole::SecondaryButton:
        default:
            style.fill = state.disabled ? palette.disabled_background :
                         state.pressed ? palette.secondary_button_pressed :
                         state.hot ? blend(palette.secondary_button, palette.accent, palette.dark ? 9 : 5) :
                         palette.secondary_button;
            style.border = state.disabled ? palette.card_border : palette.card_border;
            break;
    }

    if (state.pressed && role != SettingsControlRole::PrimaryButton &&
        role != SettingsControlRole::DangerButton && !state.disabled) {
        style.fill = blend(style.fill, RGB(0, 0, 0), palette.dark ? 10 : 6);
    }
    if (state.focused && !state.disabled) {
        style.border = palette.accent;
    }
    return style;
}

SettingsHeaderDrawStyle settings_header_draw_style(const SettingsThemePalette& palette) {
    SettingsHeaderDrawStyle style;
    style.background = palette.list_header_background;
    style.text = palette.list_header_text;
    style.border = palette.card_border;
    style.separator = palette.card_border;
    style.fills_entire_client_rect = true;
    style.uses_default_system_background = false;
    return style;
}

SettingsThemeRefreshPlan settings_theme_refresh_plan() {
    return SettingsThemeRefreshPlan{};
}

SettingsAboutPathPolicy settings_about_path_policy() {
    return SettingsAboutPathPolicy{};
}

SettingsVisualSpec settings_visual_spec(int dpi) {
    const int safe_dpi = dpi <= 0 ? kBaseDpi : dpi;
    SettingsVisualSpec spec;
    spec.body_font_px = scale_settings_value(14, safe_dpi);
    spec.title_font_px = scale_settings_value(24, safe_dpi);
    spec.section_font_px = scale_settings_value(15, safe_dpi);
    spec.card_corner_radius = scale_settings_value(12, safe_dpi);
    spec.control_corner_radius = scale_settings_value(8, safe_dpi);
    spec.nav_corner_radius = scale_settings_value(8, safe_dpi);
    spec.parent_paints_cards = true;
    spec.parent_uses_double_buffering = true;
    spec.owner_draw_buttons = true;
    spec.owner_draw_fills_client_rect = true;
    spec.static_controls_use_theme_brushes = true;
    spec.uses_legacy_group_boxes = false;
    spec.edit_uses_client_edge = false;
    spec.list_view_uses_grid_lines = false;
    spec.user_lexicon_status_inside_editor = true;
    spec.user_lexicon_status_region_count = 1;
    return spec;
}

UserLexiconColumnWidths calculate_user_lexicon_column_widths(int table_width, int dpi) {
    const int safe_dpi = dpi <= 0 ? kBaseDpi : dpi;
    const int available = std::max(1, table_width - scale_settings_value(18, safe_dpi));
    UserLexiconColumnWidths widths;
    widths.pinyin = std::max(scale_settings_value(120, safe_dpi), available * 25 / 100);
    widths.word = std::max(scale_settings_value(240, safe_dpi), available * 50 / 100);
    widths.frequency = std::max(scale_settings_value(80, safe_dpi),
                                available - widths.pinyin - widths.word);
    return widths;
}

UserLexiconRowVisualStyle user_lexicon_row_visual_style(bool selected, bool list_has_focus) {
    return user_lexicon_row_visual_style(settings_theme_palette(SettingsResolvedTheme::Light), selected, list_has_focus);
}

UserLexiconRowVisualStyle user_lexicon_row_visual_style(
    const SettingsThemePalette& palette,
    bool selected,
    bool list_has_focus) {
    UserLexiconRowVisualStyle style;
    style.text = palette.text;
    style.background = palette.card_background;
    style.selected_background = false;
    if (selected) {
        style.background = list_has_focus ? palette.accent_soft : palette.inactive_selection;
        style.selected_background = true;
    }
    return style;
}

bool settings_rects_overlap(const SettingsRect& left, const SettingsRect& right) {
    return left.x < rect_right(right) &&
           rect_right(left) > right.x &&
           left.y < rect_bottom(right) &&
           rect_bottom(left) > right.y;
}

SettingsWindowLayout calculate_settings_window_layout(int client_width, int client_height, int dpi) {
    const int safe_dpi = dpi <= 0 ? kBaseDpi : dpi;
    const SettingsSize minimum = settings_minimum_client_size(safe_dpi);
    const int width = std::max(client_width, minimum.width);
    const int height = std::max(client_height, minimum.height);
    const int sidebar_width = scale_settings_value(220, safe_dpi);
    const int margin = scale_settings_value(32, safe_dpi);
    const int gap = scale_settings_value(20, safe_dpi);

    SettingsWindowLayout layout;
    layout.dpi = safe_dpi;
    layout.sidebar = SettingsRect{0, 0, sidebar_width, height};
    layout.content = SettingsRect{sidebar_width, 0, width - sidebar_width, height};
    layout.page_title = SettingsRect{sidebar_width + margin,
                                     margin,
                                     width - sidebar_width - margin * 2,
                                     scale_settings_value(34, safe_dpi)};
    layout.page_description = SettingsRect{layout.page_title.x,
                                           rect_bottom(layout.page_title) + scale_settings_value(4, safe_dpi),
                                           layout.page_title.width,
                                           scale_settings_value(42, safe_dpi)};

    const int nav_x = scale_settings_value(16, safe_dpi);
    int nav_y = scale_settings_value(92, safe_dpi);
    const int nav_width = sidebar_width - nav_x * 2;
    const int nav_height = scale_settings_value(40, safe_dpi);
    const int nav_gap = scale_settings_value(6, safe_dpi);
    for (int i = 0; i < kSettingsPageCount; ++i) {
        layout.nav_items[static_cast<size_t>(i)] = SettingsRect{nav_x, nav_y, nav_width, nav_height};
        nav_y += nav_height + nav_gap;
    }

    const int content_x = layout.page_title.x;
    const int content_width = layout.page_title.width;
    const int content_top = rect_bottom(layout.page_description) + scale_settings_value(18, safe_dpi);
    layout.primary_card = SettingsRect{content_x,
                                       content_top,
                                       content_width,
                                       scale_settings_value(150, safe_dpi)};
    layout.secondary_card = SettingsRect{content_x,
                                         rect_bottom(layout.primary_card) + gap,
                                         content_width,
                                         height - rect_bottom(layout.primary_card) - margin - gap};
    layout.search_box = SettingsRect{content_x,
                                     content_top,
                                     content_width - scale_settings_value(108, safe_dpi),
                                     scale_settings_value(38, safe_dpi)};
    layout.refresh_button = SettingsRect{rect_right(layout.search_box) + scale_settings_value(12, safe_dpi),
                                         content_top,
                                         scale_settings_value(96, safe_dpi),
                                         scale_settings_value(38, safe_dpi)};

    const int editor_height = scale_settings_value(230, safe_dpi);
    layout.user_editor_card = SettingsRect{content_x,
                                           height - margin - editor_height,
                                           content_width,
                                           editor_height};
    layout.user_editor_title = SettingsRect{layout.user_editor_card.x + scale_settings_value(24, safe_dpi),
                                            layout.user_editor_card.y + scale_settings_value(18, safe_dpi),
                                            layout.user_editor_card.width - scale_settings_value(48, safe_dpi),
                                            scale_settings_value(24, safe_dpi)};
    layout.user_status = SettingsRect{layout.user_editor_card.x + scale_settings_value(24, safe_dpi),
                                      rect_bottom(layout.user_editor_card) - scale_settings_value(54, safe_dpi),
                                      layout.user_editor_card.width - scale_settings_value(48, safe_dpi),
                                      scale_settings_value(36, safe_dpi)};
    const int table_y = rect_bottom(layout.search_box) + scale_settings_value(12, safe_dpi);
    const int table_height = std::max(scale_settings_value(140, safe_dpi),
                                      layout.user_editor_card.y - gap - table_y);
    layout.user_table = SettingsRect{content_x, table_y, content_width, table_height};
    layout.user_empty_state = layout.user_table;

    const int about_card_height = scale_settings_value(400, safe_dpi);
    layout.about_card = SettingsRect{content_x,
                                     content_top,
                                     content_width,
                                     std::min(about_card_height, height - content_top - margin)};
    const int about_padding = scale_settings_value(24, safe_dpi);
    const int about_label_width = scale_settings_value(112, safe_dpi);
    const int about_row_height = scale_settings_value(30, safe_dpi);
    const int about_action_height = scale_settings_value(34, safe_dpi);
    const int about_action_gap = scale_settings_value(12, safe_dpi);
    const int about_value_x = layout.about_card.x + about_padding + about_label_width + scale_settings_value(12, safe_dpi);
    const int about_value_width = std::max(scale_settings_value(120, safe_dpi),
                                           rect_right(layout.about_card) - about_value_x - about_padding);
    int about_row_y = layout.about_card.y + about_padding;
    for (size_t i = 0; i < layout.about_labels.size(); ++i) {
        layout.about_labels[i] = SettingsRect{layout.about_card.x + about_padding,
                                              about_row_y,
                                              about_label_width,
                                              about_row_height};
        layout.about_values[i] = SettingsRect{about_value_x,
                                              about_row_y,
                                              about_value_width,
                                              about_row_height};
        about_row_y += about_row_height;
    }
    const int about_button_y = std::min(rect_bottom(layout.about_card) - about_padding - about_action_height * 2 - about_action_gap,
                                        about_row_y + scale_settings_value(18, safe_dpi));
    layout.about_open_data_button = SettingsRect{about_value_x,
                                                 about_button_y,
                                                 scale_settings_value(152, safe_dpi),
                                                 about_action_height};
    layout.about_copy_data_button = SettingsRect{rect_right(layout.about_open_data_button) + about_action_gap,
                                                 about_button_y,
                                                 scale_settings_value(112, safe_dpi),
                                                 about_action_height};
    const int github_button_y = rect_bottom(layout.about_open_data_button) + about_action_gap;
    layout.about_open_github_button = SettingsRect{about_value_x,
                                                   github_button_y,
                                                   scale_settings_value(152, safe_dpi),
                                                   about_action_height};
    layout.about_copy_github_button = SettingsRect{rect_right(layout.about_open_github_button) + about_action_gap,
                                                   github_button_y,
                                                   scale_settings_value(136, safe_dpi),
                                                   about_action_height};
    return layout;
}

UserLexiconTableRow make_user_lexicon_table_row(const UserLexiconEntry& entry) {
    return UserLexiconTableRow{entry.pinyin, entry.word, std::to_wstring(entry.frequency)};
}

UserLexiconEditorState make_new_user_lexicon_editor_state(bool focus_pinyin) {
    UserLexiconEditorState state;
    state.focus_pinyin = focus_pinyin;
    state.mode = UserLexiconEditorMode::NewEntry;
    state.primary_action_text = L"\u65B0\u589E\u8BCD\u6761";
    return state;
}

UserLexiconEditorState user_lexicon_editor_state_for_selection(
    const std::vector<UserLexiconEntry>& entries,
    const std::vector<size_t>& filtered_indexes,
    int selected_row) {
    if (selected_row < 0 || static_cast<size_t>(selected_row) >= filtered_indexes.size()) {
        return make_new_user_lexicon_editor_state(false);
    }
    const size_t entry_index = filtered_indexes[static_cast<size_t>(selected_row)];
    if (entry_index >= entries.size()) {
        return make_new_user_lexicon_editor_state(false);
    }
    const auto row = make_user_lexicon_table_row(entries[entry_index]);
    UserLexiconEditorState state;
    state.pinyin = row.pinyin;
    state.word = row.word;
    state.frequency = row.frequency;
    state.delete_enabled = true;
    state.mode = UserLexiconEditorMode::EditExisting;
    state.primary_action_text = L"\u4FDD\u5B58\u4FEE\u6539";
    return state;
}

int SettingsWindow::run(HINSTANCE instance, int show_command) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    INITCOMMONCONTROLSEX controls{};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&controls);

    store_.ensure_data_dir();
    options_ = store_.load_candidate_options();
    input_mode_options_ = store_.load_input_mode_options();
    palette_ = settings_theme_palette(resolve_settings_theme(options_.theme_mode, windows_apps_use_dark_theme()));

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = SettingsWindow::window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"LocalPinyinSettingsWindow";
    app_icon_big_ = static_cast<HICON>(LoadImageW(instance,
                                                  MAKEINTRESOURCEW(IDI_LOCALPINYINIME_APP),
                                                  IMAGE_ICON,
                                                  GetSystemMetrics(SM_CXICON),
                                                  GetSystemMetrics(SM_CYICON),
                                                  LR_DEFAULTCOLOR));
    app_icon_small_ = static_cast<HICON>(LoadImageW(instance,
                                                    MAKEINTRESOURCEW(IDI_LOCALPINYINIME_APP),
                                                    IMAGE_ICON,
                                                    GetSystemMetrics(SM_CXSMICON),
                                                    GetSystemMetrics(SM_CYSMICON),
                                                    LR_DEFAULTCOLOR));
    wc.hIcon = app_icon_big_;
    wc.hIconSm = app_icon_small_;
    RegisterClassExW(&wc);

    constexpr DWORD kSettingsWindowStyle = WS_OVERLAPPEDWINDOW;
    constexpr DWORD kSettingsWindowExStyle = 0;
    const UINT initial_dpi = GetDpiForSystem();
    const RECT work_area = primary_work_area_near_cursor();
    const SettingsRect initial_rect = calculate_settings_initial_window_rect(
        work_area,
        static_cast<int>(initial_dpi),
        kSettingsWindowStyle,
        kSettingsWindowExStyle);

    hwnd_ = CreateWindowExW(kSettingsWindowExStyle,
                            wc.lpszClassName,
                            L"LocalPinyinIME Settings",
                            kSettingsWindowStyle,
                            initial_rect.x,
                            initial_rect.y,
                            initial_rect.width,
                            initial_rect.height,
                            nullptr,
                            nullptr,
                            instance,
                            this);
    if (!hwnd_) {
        return 1;
    }
    if (app_icon_big_) {
        SendMessageW(hwnd_, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(app_icon_big_));
    }
    if (app_icon_small_) {
        SendMessageW(hwnd_, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(app_icon_small_));
    }
    ShowWindow(hwnd_, show_command);
    UpdateWindow(hwnd_);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}

LRESULT CALLBACK SettingsWindow::window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* self = reinterpret_cast<SettingsWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
        self = static_cast<SettingsWindow*>(create->lpCreateParams);
        self->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    return self ? self->handle_message(message, wparam, lparam) : DefWindowProcW(hwnd, message, wparam, lparam);
}

LRESULT SettingsWindow::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
        case WM_CREATE:
            dpi_ = GetDpiForWindow(hwnd_);
            create_fonts();
            create_controls();
            apply_options_to_controls();
            refresh_theme();
            load_user_lexicon();
            show_page(active_page_);
            layout_controls();
            return 0;
        case WM_SIZE:
            layout_controls();
            InvalidateRect(hwnd_, nullptr, TRUE);
            return 0;
        case WM_DPICHANGED: {
            dpi_ = HIWORD(wparam);
            destroy_fonts();
            create_fonts();
            apply_fonts();
            refresh_theme();
            const RECT* suggested = reinterpret_cast<const RECT*>(lparam);
            SetWindowPos(hwnd_,
                         nullptr,
                         suggested->left,
                         suggested->top,
                         suggested->right - suggested->left,
                         suggested->bottom - suggested->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            layout_controls();
            return 0;
        }
        case WM_GETMINMAXINFO: {
            auto* minmax = reinterpret_cast<MINMAXINFO*>(lparam);
            const int dpi = dpi_ > 0 ? dpi_ : static_cast<int>(GetDpiForSystem());
            constexpr DWORD kSettingsWindowStyle = WS_OVERLAPPEDWINDOW;
            constexpr DWORD kSettingsWindowExStyle = 0;
            const SettingsSize min_outer = settings_outer_size_for_client(
                settings_minimum_client_size(dpi),
                dpi,
                kSettingsWindowStyle,
                kSettingsWindowExStyle);
            minmax->ptMinTrackSize.x = min_outer.width;
            minmax->ptMinTrackSize.y = min_outer.height;
            return 0;
        }
        case WM_SHOWWINDOW:
            if (wparam) {
                layout_controls();
                RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
            }
            return 0;
        case WM_COMMAND:
            switch (LOWORD(wparam)) {
                case kIdNavAppearance:
                case kIdNavCandidate:
                case kIdNavUserLexicon:
                case kIdNavLearning:
                case kIdNavAbout:
                    show_page(page_for_nav_id(LOWORD(wparam)));
                    return 0;
                case kIdOpenDataFolder:
                    open_data_dir();
                    return 0;
                case kIdCopyDataFolder:
                    copy_data_dir_to_clipboard();
                    return 0;
                case kIdOpenGithubProject:
                    open_github_project();
                    return 0;
                case kIdCopyGithubUrl:
                    copy_github_url_to_clipboard();
                    return 0;
                case kIdThemeSystem:
                case kIdThemeLight:
                case kIdThemeDark:
                    options_.theme_mode = LOWORD(wparam) == kIdThemeLight ? CandidateThemeMode::Light :
                                          LOWORD(wparam) == kIdThemeDark ? CandidateThemeMode::Dark :
                                          CandidateThemeMode::System;
                    store_.save_options(options_, input_mode_options_);
                    refresh_theme();
                    return 0;
                case kIdShowKeyHints:
                    options_.show_key_hints = !options_.show_key_hints;
                    store_.save_options(options_, input_mode_options_);
                    InvalidateRect(GetDlgItem(hwnd_, kIdShowKeyHints), nullptr, TRUE);
                    return 0;
                case kIdShiftToggle:
                    input_mode_options_.shift_toggle_enabled = !input_mode_options_.shift_toggle_enabled;
                    store_.save_options(options_, input_mode_options_);
                    InvalidateRect(GetDlgItem(hwnd_, kIdShiftToggle), nullptr, TRUE);
                    return 0;
                case kIdTextSmall:
                case kIdTextStandard:
                case kIdTextLarge:
                    options_.text_size = LOWORD(wparam) == kIdTextSmall ? CandidateTextSize::Small :
                                         LOWORD(wparam) == kIdTextLarge ? CandidateTextSize::Large :
                                         CandidateTextSize::Standard;
                    store_.save_options(options_, input_mode_options_);
                    InvalidateRect(hwnd_, nullptr, FALSE);
                    return 0;
                case kIdClearLearning:
                    clear_learning_data();
                    return 0;
                case kIdLexiconRefresh:
                    refresh_user_lexicon();
                    return 0;
                case kIdLexiconNew:
                    enter_new_user_lexicon_state(true, L"\u6B63\u5728\u65B0\u5EFA\u8BCD\u6761\u3002");
                    return 0;
                case kIdLexiconSave:
                    save_user_lexicon_entry();
                    return 0;
                case kIdLexiconDelete:
                    delete_user_lexicon_entry();
                    return 0;
                case kIdLexiconSearch:
                    if (HIWORD(wparam) == EN_CHANGE) {
                        populate_user_lexicon_list();
                    }
                    if (HIWORD(wparam) == EN_SETFOCUS || HIWORD(wparam) == EN_KILLFOCUS) {
                        InvalidateRect(hwnd_, nullptr, FALSE);
                    }
                    return 0;
                case kIdLexiconPinyin:
                case kIdLexiconWord:
                case kIdLexiconFrequency:
                    if (HIWORD(wparam) == EN_SETFOCUS || HIWORD(wparam) == EN_KILLFOCUS) {
                        InvalidateRect(hwnd_, nullptr, FALSE);
                    }
                    return 0;
                default:
                    return 0;
        }
        case WM_NOTIFY: {
            const auto* header = reinterpret_cast<NMHDR*>(lparam);
            const HWND list_header = lexicon_list_ ? ListView_GetHeader(lexicon_list_) : nullptr;
            if (header && list_header && header->hwndFrom == list_header && header->code == NM_CUSTOMDRAW) {
                auto* custom = reinterpret_cast<NMCUSTOMDRAW*>(lparam);
                if (custom->dwDrawStage == CDDS_PREPAINT) {
                    fill_rect(custom->hdc, custom->rc, palette_.list_header_background);
                    return CDRF_NOTIFYITEMDRAW;
                }
                if (custom->dwDrawStage == CDDS_ITEMPREPAINT) {
                    return draw_list_header_item(*custom) ? CDRF_SKIPDEFAULT : CDRF_DODEFAULT;
                }
            }
            if (header && header->idFrom == kIdLexiconList && header->code == NM_CUSTOMDRAW) {
                auto* custom = reinterpret_cast<NMLVCUSTOMDRAW*>(lparam);
                if (custom->nmcd.dwDrawStage == CDDS_PREPAINT) {
                    return CDRF_NOTIFYITEMDRAW;
                }
                if (custom->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                    const int row = static_cast<int>(custom->nmcd.dwItemSpec);
                    const bool selected = (ListView_GetItemState(lexicon_list_, row, LVIS_SELECTED) & LVIS_SELECTED) != 0;
                    const bool list_has_focus = GetFocus() == lexicon_list_;
                    const auto style = user_lexicon_row_visual_style(palette_, selected, list_has_focus);
                    custom->clrText = style.text;
                    custom->clrTextBk = style.background;
                    return CDRF_NEWFONT;
                }
            }
            if (header && header->idFrom == kIdLexiconList && header->code == LVN_ITEMCHANGED) {
                const int selected = selected_user_lexicon_row();
                const auto state = user_lexicon_editor_state_for_selection(user_lexicon_entries_,
                                                                           filtered_user_lexicon_indexes_,
                                                                           selected);
                if (state.delete_enabled) {
                    populate_user_lexicon_editor(state);
                } else {
                    enter_new_user_lexicon_state(false);
                }
                update_user_lexicon_action_controls();
                return 0;
            }
            break;
        }
        case WM_DRAWITEM:
            draw_button(*reinterpret_cast<DRAWITEMSTRUCT*>(lparam));
            return TRUE;
        case WM_CTLCOLORSTATIC: {
            HDC hdc = reinterpret_cast<HDC>(wparam);
            const int id = GetDlgCtrlID(reinterpret_cast<HWND>(lparam));
            const bool title = id == kIdAppTitle ||
                               id == kIdAppearanceTitle || id == kIdCandidateTitle ||
                               id == kIdUserLexiconTitle || id == kIdLearningTitle ||
                               id == kIdAboutTitle;
            const bool section = id == kIdAppearanceThemeLabel || id == kIdCandidateHintLabel ||
                                  id == kIdCandidateTextSizeLabel || id == kIdLexiconPinyinLabel ||
                                  id == kIdLexiconWordLabel || id == kIdLexiconFrequencyLabel ||
                                   id == kIdUserLexiconEditorTitle ||
                                   id == kIdAboutProductKey || id == kIdAboutVersionKey ||
                                   id == kIdAboutArchKey || id == kIdAboutOfflineKey ||
                                   id == kIdAboutStatusKey || id == kIdAboutDataDirKey ||
                                   id == kIdAboutGithubKey;
            const HBRUSH background = brush_for_static_control(id);
            SetBkMode(hdc, TRANSPARENT);
            SetBkColor(hdc,
                       background == sidebar_background_brush_ ? palette_.sidebar_background :
                       background == card_background_brush_ ? palette_.card_background :
                       background == border_background_brush_ ? palette_.card_border :
                       palette_.window_background);
            SetTextColor(hdc, title || section ? palette_.text : palette_.muted_text);
            return reinterpret_cast<LRESULT>(background ? background : GetStockObject(NULL_BRUSH));
        }
        case WM_CTLCOLOREDIT: {
            HDC hdc = reinterpret_cast<HDC>(wparam);
            SetBkColor(hdc, palette_.input_background);
            SetTextColor(hdc, palette_.text);
            return reinterpret_cast<LRESULT>(edit_background_brush_ ? edit_background_brush_ : GetStockObject(WHITE_BRUSH));
        }
        case WM_CTLCOLORBTN: {
            HDC hdc = reinterpret_cast<HDC>(wparam);
            const int id = GetDlgCtrlID(reinterpret_cast<HWND>(lparam));
            const SettingsControlSurface surface = control_surface_for_id(id);
            HBRUSH brush = brush_for_control_surface(surface);
            SetBkMode(hdc, TRANSPARENT);
            SetBkColor(hdc,
                       surface == SettingsControlSurface::Sidebar ? palette_.sidebar_background :
                       surface == SettingsControlSurface::Card ? palette_.card_background :
                       palette_.window_background);
            return reinterpret_cast<LRESULT>(brush ? brush : GetStockObject(NULL_BRUSH));
        }
        case WM_SETTINGCHANGE:
            if (options_.theme_mode == CandidateThemeMode::System) {
                refresh_theme();
            }
            break;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT:
            paint();
            return 0;
        case WM_DESTROY:
            if (lexicon_list_) {
                const HWND header = ListView_GetHeader(lexicon_list_);
                if (header) {
                    RemoveWindowSubclass(header, SettingsWindow::list_header_subclass_proc, kUserLexiconHeaderSubclassId);
                }
            }
            destroy_theme_brushes();
            destroy_fonts();
            if (app_icon_big_) {
                DestroyIcon(app_icon_big_);
                app_icon_big_ = nullptr;
            }
            if (app_icon_small_) {
                DestroyIcon(app_icon_small_);
                app_icon_small_ = nullptr;
            }
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd_, message, wparam, lparam);
    }
    return DefWindowProcW(hwnd_, message, wparam, lparam);
}

void SettingsWindow::create_fonts() {
    body_font_ = create_ui_font(dpi_, 10, FW_NORMAL);
    title_font_ = create_ui_font(dpi_, 18, FW_SEMIBOLD);
    section_font_ = create_ui_font(dpi_, 11, FW_SEMIBOLD);
}

void SettingsWindow::destroy_fonts() {
    if (body_font_) {
        DeleteObject(body_font_);
        body_font_ = nullptr;
    }
    if (title_font_) {
        DeleteObject(title_font_);
        title_font_ = nullptr;
    }
    if (section_font_) {
        DeleteObject(section_font_);
        section_font_ = nullptr;
    }
}

void SettingsWindow::apply_fonts() {
    set_control_font(GetDlgItem(hwnd_, kIdAppTitle), section_font_);
    for (HWND nav : nav_buttons_) {
        set_control_font(nav, body_font_);
    }
    const int title_ids[] = {
        kIdAppearanceTitle,
        kIdCandidateTitle,
        kIdUserLexiconTitle,
        kIdLearningTitle,
        kIdAboutTitle,
    };
    for (int id : title_ids) {
        set_control_font(GetDlgItem(hwnd_, id), title_font_);
    }
    const int section_ids[] = {
        kIdAppearanceThemeLabel,
        kIdCandidateHintLabel,
        kIdCandidateTextSizeLabel,
        kIdUserLexiconEditorTitle,
        kIdLexiconPinyinLabel,
        kIdLexiconWordLabel,
        kIdLexiconFrequencyLabel,
        kIdAboutProductKey,
        kIdAboutVersionKey,
        kIdAboutArchKey,
        kIdAboutOfflineKey,
        kIdAboutStatusKey,
        kIdAboutDataDirKey,
        kIdAboutGithubKey,
    };
    for (int id : section_ids) {
        set_control_font(GetDlgItem(hwnd_, id), section_font_);
    }
    for (const auto& controls : page_controls_) {
        for (HWND control : controls) {
            set_control_font(control, body_font_);
        }
    }
    for (int id : title_ids) {
        set_control_font(GetDlgItem(hwnd_, id), title_font_);
    }
    for (int id : section_ids) {
        set_control_font(GetDlgItem(hwnd_, id), section_font_);
    }
}

void SettingsWindow::destroy_theme_brushes() {
    if (window_background_brush_) {
        DeleteObject(window_background_brush_);
        window_background_brush_ = nullptr;
    }
    if (sidebar_background_brush_) {
        DeleteObject(sidebar_background_brush_);
        sidebar_background_brush_ = nullptr;
    }
    if (card_background_brush_) {
        DeleteObject(card_background_brush_);
        card_background_brush_ = nullptr;
    }
    if (border_background_brush_) {
        DeleteObject(border_background_brush_);
        border_background_brush_ = nullptr;
    }
    if (edit_background_brush_) {
        DeleteObject(edit_background_brush_);
        edit_background_brush_ = nullptr;
    }
}

void SettingsWindow::refresh_theme() {
    palette_ = settings_theme_palette(resolve_settings_theme(options_.theme_mode, windows_apps_use_dark_theme()));
    apply_theme_to_controls();
    apply_title_bar_theme();

    const SettingsThemeRefreshPlan plan = settings_theme_refresh_plan();
    if (plan.redraw_list_view && lexicon_list_) {
        InvalidateRect(lexicon_list_, nullptr, TRUE);
        const HWND header = ListView_GetHeader(lexicon_list_);
        if (header) {
            InvalidateRect(header, nullptr, TRUE);
        }
    }
    if (plan.redraw_child_controls) {
        redraw_theme_children();
    }
    if (plan.invalidate_window && hwnd_) {
        RedrawWindow(hwnd_,
                     nullptr,
                     nullptr,
                     RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }
}

void SettingsWindow::apply_theme_to_controls() {
    destroy_theme_brushes();
    window_background_brush_ = CreateSolidBrush(palette_.window_background);
    sidebar_background_brush_ = CreateSolidBrush(palette_.sidebar_background);
    card_background_brush_ = CreateSolidBrush(palette_.card_background);
    border_background_brush_ = CreateSolidBrush(palette_.card_border);
    edit_background_brush_ = CreateSolidBrush(palette_.input_background);

    if (lexicon_list_) {
        ListView_SetBkColor(lexicon_list_, palette_.card_background);
        ListView_SetTextBkColor(lexicon_list_, palette_.card_background);
        ListView_SetTextColor(lexicon_list_, palette_.text);
        install_list_header_theme_subclass();
    }
}

void SettingsWindow::redraw_theme_children() {
    for (HWND nav : nav_buttons_) {
        if (nav) {
            RedrawWindow(nav, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
        }
    }
    for (const auto& controls : page_controls_) {
        for (HWND control : controls) {
            if (control) {
                RedrawWindow(control, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
            }
        }
    }
    if (lexicon_list_) {
        const HWND header = ListView_GetHeader(lexicon_list_);
        if (header) {
            RedrawWindow(header, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
        }
    }
}

void SettingsWindow::install_list_header_theme_subclass() {
    if (!lexicon_list_) {
        return;
    }
    const HWND header = ListView_GetHeader(lexicon_list_);
    if (!header) {
        return;
    }
    SetWindowSubclass(header,
                      SettingsWindow::list_header_subclass_proc,
                      kUserLexiconHeaderSubclassId,
                      reinterpret_cast<DWORD_PTR>(this));
    RedrawWindow(header, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
}

HBRUSH SettingsWindow::brush_for_control_surface(SettingsControlSurface surface) const {
    switch (surface) {
        case SettingsControlSurface::Sidebar:
            return sidebar_background_brush_;
        case SettingsControlSurface::Card:
            return card_background_brush_;
        case SettingsControlSurface::Window:
        default:
            return window_background_brush_;
    }
}

HBRUSH SettingsWindow::brush_for_static_control(int id) const {
    if (id == kIdAppearanceCard || id == kIdCandidateCard ||
        id == kIdUserLexiconTableCard || id == kIdUserLexiconEditorCard ||
        id == kIdLearningCard || id == kIdAboutCard) {
        return nullptr;
    }
    if (id == kIdSidebarDivider) {
        return border_background_brush_;
    }
    if (id == kIdAppTitle) {
        return sidebar_background_brush_;
    }

    switch (id) {
        case kIdAppearanceTitle:
        case kIdAppearanceDescription:
        case kIdCandidateTitle:
        case kIdCandidateDescription:
        case kIdUserLexiconTitle:
        case kIdUserLexiconDescription:
        case kIdLearningTitle:
        case kIdLearningDescription:
        case kIdAboutTitle:
        case kIdAboutDescription:
            return window_background_brush_;
        default:
            return card_background_brush_;
    }
}

SettingsControlRole SettingsWindow::control_role_for_id(int id) const {
    if (id == kIdNavAppearance || id == kIdNavCandidate ||
        id == kIdNavUserLexicon || id == kIdNavLearning || id == kIdNavAbout) {
        return SettingsControlRole::Navigation;
    }
    if (id == kIdThemeSystem || id == kIdThemeLight || id == kIdThemeDark ||
        id == kIdShowKeyHints || id == kIdShiftToggle || id == kIdTextSmall ||
        id == kIdTextStandard || id == kIdTextLarge) {
        return SettingsControlRole::Option;
    }
    if (id == kIdLexiconSave) {
        return SettingsControlRole::PrimaryButton;
    }
    if (id == kIdLexiconDelete || id == kIdClearLearning) {
        return SettingsControlRole::DangerButton;
    }
    return SettingsControlRole::SecondaryButton;
}

SettingsControlSurface SettingsWindow::control_surface_for_id(int id) const {
    if (id == kIdNavAppearance || id == kIdNavCandidate ||
        id == kIdNavUserLexicon || id == kIdNavLearning || id == kIdNavAbout) {
        return SettingsControlSurface::Sidebar;
    }
    if (id == kIdLexiconRefresh) {
        return SettingsControlSurface::Window;
    }
    return SettingsControlSurface::Card;
}

void SettingsWindow::apply_title_bar_theme() {
    if (hwnd_) {
        apply_dark_title_bar(hwnd_, palette_.dark);
    }
}

bool SettingsWindow::draw_list_header_item(const NMCUSTOMDRAW& custom_draw) {
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

    const SettingsHeaderDrawStyle style = settings_header_draw_style(palette_);
    RECT rect = custom_draw.rc;
    fill_rect(custom_draw.hdc, rect, style.background);
    RECT text_rect = rect;
    text_rect.left += scale_settings_value(12, dpi_);
    text_rect.right -= scale_settings_value(8, dpi_);
    draw_text_in_rect(custom_draw.hdc,
                      text,
                      text_rect,
                      style.text,
                      body_font_,
                      DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);

    HPEN pen = CreatePen(PS_SOLID, 1, style.separator);
    HGDIOBJ old_pen = SelectObject(custom_draw.hdc, pen);
    MoveToEx(custom_draw.hdc, rect.right - 1, rect.top + scale_settings_value(5, dpi_), nullptr);
    LineTo(custom_draw.hdc, rect.right - 1, rect.bottom - scale_settings_value(5, dpi_));
    MoveToEx(custom_draw.hdc, rect.left, rect.bottom - 1, nullptr);
    LineTo(custom_draw.hdc, rect.right, rect.bottom - 1);
    SelectObject(custom_draw.hdc, old_pen);
    DeleteObject(pen);
    return true;
}

void SettingsWindow::paint_list_header(HWND header, HDC hdc) {
    if (!header || !hdc) {
        return;
    }

    const SettingsHeaderDrawStyle style = settings_header_draw_style(palette_);
    RECT client{};
    GetClientRect(header, &client);
    fill_rect(hdc, client, style.background);

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

        fill_rect(hdc, item_rect, style.background);
        RECT text_rect = item_rect;
        text_rect.left += scale_settings_value(12, dpi_);
        text_rect.right -= scale_settings_value(8, dpi_);
        draw_text_in_rect(hdc,
                          text,
                          text_rect,
                          style.text,
                          body_font_,
                          DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);

        HPEN pen = CreatePen(PS_SOLID, 1, style.separator);
        HGDIOBJ old_pen = SelectObject(hdc, pen);
        MoveToEx(hdc, item_rect.right - 1, item_rect.top + scale_settings_value(5, dpi_), nullptr);
        LineTo(hdc, item_rect.right - 1, item_rect.bottom - scale_settings_value(5, dpi_));
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

LRESULT CALLBACK SettingsWindow::list_header_subclass_proc(
    HWND hwnd,
    UINT message,
    WPARAM wparam,
    LPARAM lparam,
    UINT_PTR subclass_id,
    DWORD_PTR ref_data) {
    (void)subclass_id;
    auto* self = reinterpret_cast<SettingsWindow*>(ref_data);
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
            RemoveWindowSubclass(hwnd, SettingsWindow::list_header_subclass_proc, kUserLexiconHeaderSubclassId);
            break;
        default:
            break;
    }
    return DefSubclassProc(hwnd, message, wparam, lparam);
}

void SettingsWindow::paint() {
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hwnd_, &ps);
    RECT client{};
    GetClientRect(hwnd_, &client);
    const int client_width = rect_width(client);
    const int client_height = rect_height(client);

    HDC paint_dc = hdc;
    HDC memory_dc = nullptr;
    HBITMAP bitmap = nullptr;
    HGDIOBJ old_bitmap = nullptr;
    if (client_width > 0 && client_height > 0) {
        memory_dc = CreateCompatibleDC(hdc);
        bitmap = memory_dc ? CreateCompatibleBitmap(hdc, client_width, client_height) : nullptr;
        if (memory_dc && bitmap) {
            old_bitmap = SelectObject(memory_dc, bitmap);
            paint_dc = memory_dc;
        } else {
            if (bitmap) {
                DeleteObject(bitmap);
                bitmap = nullptr;
            }
            if (memory_dc) {
                DeleteDC(memory_dc);
                memory_dc = nullptr;
            }
        }
    }

    const auto layout = calculate_settings_window_layout(client_width, client_height, dpi_);
    const auto spec = settings_visual_spec(dpi_);
    fill_rect(paint_dc, client, palette_.window_background);
    fill_rect(paint_dc, to_win_rect(layout.sidebar), palette_.sidebar_background);

    RECT divider{layout.sidebar.x + layout.sidebar.width - 1, 0, layout.sidebar.x + layout.sidebar.width, layout.sidebar.height};
    fill_rect(paint_dc, divider, palette_.card_border);

    const auto draw_card = [&](const SettingsRect& settings_rect) {
        draw_round_rect(paint_dc, to_win_rect(settings_rect), spec.card_corner_radius, palette_.card_background, palette_.card_border);
    };

    const auto draw_edit_frame = [&](const SettingsRect& settings_rect, HWND edit) {
        const bool focused = GetFocus() == edit;
        draw_round_rect(paint_dc,
                        to_win_rect(settings_rect),
                        spec.control_corner_radius,
                        palette_.input_background,
                        focused ? palette_.accent : palette_.input_border);
    };

    if (active_page_ == SettingsPage::UserLexicon) {
        draw_edit_frame(layout.search_box, lexicon_search_);
        draw_card(layout.user_table);
        draw_card(layout.user_editor_card);
    } else if (active_page_ == SettingsPage::About) {
        draw_card(layout.about_card);
    } else {
        draw_card(layout.primary_card);
    }

    if (active_page_ == SettingsPage::UserLexicon) {
        const int s24 = scale_settings_value(24, dpi_);
        const int s18 = scale_settings_value(18, dpi_);
        const int s12 = scale_settings_value(12, dpi_);
        const int s24h = scale_settings_value(24, dpi_);
        const int s32 = scale_settings_value(32, dpi_);
        const int editor_x = layout.user_editor_card.x + s24;
        const int editor_y = rect_bottom(layout.user_editor_title) + s18;
        const int editor_width = layout.user_editor_card.width - s24 * 2;
        const int frequency_width = scale_settings_value(100, dpi_);
        const int pinyin_width = scale_settings_value(160, dpi_);
        const int word_width = std::max(scale_settings_value(220, dpi_), editor_width - pinyin_width - frequency_width - s12 * 2);
        draw_edit_frame(SettingsRect{editor_x, editor_y + s24h, pinyin_width, s32}, lexicon_pinyin_);
        draw_edit_frame(SettingsRect{editor_x + pinyin_width + s12, editor_y + s24h, word_width, s32}, lexicon_word_);
        draw_edit_frame(SettingsRect{editor_x + pinyin_width + s12 + word_width + s12, editor_y + s24h, frequency_width, s32}, lexicon_frequency_);
    }

    if (memory_dc && bitmap) {
        BitBlt(hdc, 0, 0, client_width, client_height, memory_dc, 0, 0, SRCCOPY);
        SelectObject(memory_dc, old_bitmap);
        DeleteObject(bitmap);
        DeleteDC(memory_dc);
    }
    EndPaint(hwnd_, &ps);
}

void SettingsWindow::draw_button(const DRAWITEMSTRUCT& item) {
    if (!item.hwndItem || !item.hDC) {
        return;
    }

    const int id = static_cast<int>(item.CtlID);
    const bool disabled = (item.itemState & ODS_DISABLED) != 0 || !IsWindowEnabled(item.hwndItem);
    const bool pressed = (item.itemState & ODS_SELECTED) != 0;
    const bool hot = (item.itemState & ODS_HOTLIGHT) != 0;
    bool checked = id == nav_id_for_page(active_page_);
    if (id == kIdThemeSystem) {
        checked = options_.theme_mode == CandidateThemeMode::System;
    } else if (id == kIdThemeLight) {
        checked = options_.theme_mode == CandidateThemeMode::Light;
    } else if (id == kIdThemeDark) {
        checked = options_.theme_mode == CandidateThemeMode::Dark;
    } else if (id == kIdShowKeyHints) {
        checked = options_.show_key_hints;
    } else if (id == kIdShiftToggle) {
        checked = input_mode_options_.shift_toggle_enabled;
    } else if (id == kIdTextSmall) {
        checked = options_.text_size == CandidateTextSize::Small;
    } else if (id == kIdTextStandard) {
        checked = options_.text_size == CandidateTextSize::Standard;
    } else if (id == kIdTextLarge) {
        checked = options_.text_size == CandidateTextSize::Large;
    }
    const bool focused = (item.itemState & ODS_FOCUS) != 0;
    const std::wstring text = control_text(item.hwndItem);
    RECT rect = item.rcItem;
    const auto spec = settings_visual_spec(dpi_);

    const bool is_nav = id == kIdNavAppearance || id == kIdNavCandidate ||
                        id == kIdNavUserLexicon || id == kIdNavLearning || id == kIdNavAbout;
    const bool is_option = id == kIdThemeSystem || id == kIdThemeLight || id == kIdThemeDark ||
                           id == kIdShowKeyHints || id == kIdShiftToggle || id == kIdTextSmall ||
                           id == kIdTextStandard || id == kIdTextLarge;
    SettingsControlDrawState state;
    state.checked = checked;
    state.hot = hot;
    state.pressed = pressed;
    state.disabled = disabled;
    state.focused = focused;
    const SettingsControlRole role = control_role_for_id(id);
    const SettingsControlSurface surface = control_surface_for_id(id);
    const SettingsControlDrawStyle style = settings_control_draw_style(palette_, role, surface, state);
    const int radius = is_nav ? spec.nav_corner_radius : spec.control_corner_radius;
    UINT text_flags = is_nav ? (DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS)
                             : (DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_END_ELLIPSIS);

    fill_rect(item.hDC, rect, style.background);
    draw_round_rect(item.hDC, rect, radius, style.fill, style.border);

    RECT text_rect = rect;
    if (is_nav) {
        text_rect.left += scale_settings_value(18, dpi_);
        if (checked) {
            RECT accent{rect.left + scale_settings_value(5, dpi_),
                        rect.top + scale_settings_value(9, dpi_),
                        rect.left + scale_settings_value(9, dpi_),
                        rect.bottom - scale_settings_value(9, dpi_)};
            draw_round_rect(item.hDC, accent, scale_settings_value(2, dpi_), palette_.accent, palette_.accent);
        }
    } else if (is_option) {
        const int mark = scale_settings_value(13, dpi_);
        RECT mark_rect{rect.left + scale_settings_value(12, dpi_),
                       rect.top + (rect_height(rect) - mark) / 2,
                       rect.left + scale_settings_value(12, dpi_) + mark,
                       rect.top + (rect_height(rect) - mark) / 2 + mark};
        fill_rect(item.hDC, mark_rect, style.fill);
        draw_round_rect(item.hDC,
                        mark_rect,
                        mark / 2,
                        style.mark_fill,
                        style.mark_border);
        text_rect.left += scale_settings_value(32, dpi_);
        text_flags = DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS;
    }

    draw_text_in_rect(item.hDC, text, text_rect, style.text, body_font_, text_flags);
}

void SettingsWindow::create_controls() {
    make_static(hwnd_, L"LocalPinyinIME", kIdAppTitle, section_font_);
    make_static(hwnd_, L"", kIdSidebarDivider, body_font_);

    nav_buttons_[page_index(SettingsPage::Appearance)] =
        make_button(hwnd_, L"\u5916\u89C2", kIdNavAppearance, BS_AUTORADIOBUTTON | BS_PUSHLIKE | BS_LEFT, body_font_);
    nav_buttons_[page_index(SettingsPage::Candidate)] =
        make_button(hwnd_, L"\u5019\u9009\u7A97", kIdNavCandidate, BS_AUTORADIOBUTTON | BS_PUSHLIKE | BS_LEFT, body_font_);
    nav_buttons_[page_index(SettingsPage::UserLexicon)] =
        make_button(hwnd_, L"\u7528\u6237\u8BCD\u5E93", kIdNavUserLexicon, BS_AUTORADIOBUTTON | BS_PUSHLIKE | BS_LEFT, body_font_);
    nav_buttons_[page_index(SettingsPage::Learning)] =
        make_button(hwnd_, L"\u5B66\u4E60\u6570\u636E", kIdNavLearning, BS_AUTORADIOBUTTON | BS_PUSHLIKE | BS_LEFT, body_font_);
    nav_buttons_[page_index(SettingsPage::About)] =
        make_button(hwnd_, L"\u5173\u4E8E", kIdNavAbout, BS_AUTORADIOBUTTON | BS_PUSHLIKE | BS_LEFT, body_font_);

    auto track = [&](SettingsPage page, HWND control) {
        page_controls_[page_index(page)].push_back(control);
        return control;
    };

    track(SettingsPage::Appearance, make_static(hwnd_, L"\u5916\u89C2", kIdAppearanceTitle, title_font_));
    track(SettingsPage::Appearance,
          make_static(hwnd_,
                      L"\u8BBE\u7F6E\u5019\u9009\u7A97\u7684\u57FA\u672C\u663E\u793A\u98CE\u683C\u3002",
                      kIdAppearanceDescription,
                      body_font_));
    track(SettingsPage::Appearance, make_group(hwnd_, L"\u4E3B\u9898", kIdAppearanceCard, section_font_));
    track(SettingsPage::Appearance, make_static(hwnd_, L"\u5916\u89C2\u6A21\u5F0F", kIdAppearanceThemeLabel, section_font_));
    track(SettingsPage::Appearance, make_button(hwnd_, L"\u8DDF\u968F\u7CFB\u7EDF", kIdThemeSystem, BS_AUTORADIOBUTTON, body_font_));
    track(SettingsPage::Appearance, make_button(hwnd_, L"\u6D45\u8272", kIdThemeLight, BS_AUTORADIOBUTTON, body_font_));
    track(SettingsPage::Appearance, make_button(hwnd_, L"\u6DF1\u8272", kIdThemeDark, BS_AUTORADIOBUTTON, body_font_));

    track(SettingsPage::Candidate, make_static(hwnd_, L"\u5019\u9009\u7A97", kIdCandidateTitle, title_font_));
    track(SettingsPage::Candidate,
          make_static(hwnd_,
                      L"\u8C03\u6574\u5019\u9009\u9879\u7684\u8F85\u52A9\u63D0\u793A\u548C\u6587\u5B57\u5C3A\u5BF8\u3002",
                      kIdCandidateDescription,
                      body_font_));
    track(SettingsPage::Candidate, make_group(hwnd_, L"\u663E\u793A", kIdCandidateCard, section_font_));
    track(SettingsPage::Candidate, make_static(hwnd_, L"\u6309\u952E\u63D0\u793A", kIdCandidateHintLabel, section_font_));
    track(SettingsPage::Candidate, make_button(hwnd_, L"\u663E\u793A\u6309\u952E\u63D0\u793A", kIdShowKeyHints, BS_AUTOCHECKBOX, body_font_));
    track(SettingsPage::Candidate, make_button(hwnd_, L"\u5355\u72EC\u6309 Shift \u5207\u6362\u4E2D\u82F1\u6587", kIdShiftToggle, BS_AUTOCHECKBOX, body_font_));
    track(SettingsPage::Candidate, make_static(hwnd_, L"\u5019\u9009\u6587\u5B57\u5927\u5C0F", kIdCandidateTextSizeLabel, section_font_));
    track(SettingsPage::Candidate, make_button(hwnd_, L"\u5C0F", kIdTextSmall, BS_AUTORADIOBUTTON, body_font_));
    track(SettingsPage::Candidate, make_button(hwnd_, L"\u6807\u51C6", kIdTextStandard, BS_AUTORADIOBUTTON, body_font_));
    track(SettingsPage::Candidate, make_button(hwnd_, L"\u5927", kIdTextLarge, BS_AUTORADIOBUTTON, body_font_));

    track(SettingsPage::UserLexicon, make_static(hwnd_, L"\u7528\u6237\u8BCD\u5E93", kIdUserLexiconTitle, title_font_));
    track(SettingsPage::UserLexicon,
          make_static(hwnd_,
                      L"\u4EC5\u7F16\u8F91\u5F53\u524D Windows \u7528\u6237\u7684\u79C1\u6709\u8BCD\u5E93\uFF0C\u4E0D\u4F1A\u4FEE\u6539\u5185\u7F6E\u8BCD\u5E93\u6216 Program Files\u3002",
                      kIdUserLexiconDescription,
                      body_font_));
    lexicon_search_ = track(SettingsPage::UserLexicon, make_edit(hwnd_, kIdLexiconSearch, body_font_));
    set_cue_banner(lexicon_search_, L"\u641C\u7D22\u62FC\u97F3\u6216\u8BCD\u8BED");
    track(SettingsPage::UserLexicon, make_button(hwnd_, L"\u5237\u65B0", kIdLexiconRefresh, BS_PUSHBUTTON, body_font_));
    lexicon_list_ = track(SettingsPage::UserLexicon, make_list_view(hwnd_, kIdLexiconList, body_font_));
    lexicon_empty_state_ = track(SettingsPage::UserLexicon,
                                 make_static(hwnd_,
                                             L"\u8FD8\u6CA1\u6709\u7528\u6237\u8BCD\u6761\u3002\u53EF\u4EE5\u5728\u4E0B\u65B9\u6DFB\u52A0\u7B2C\u4E00\u6761\u3002",
                                             kIdLexiconEmptyState,
                                             body_font_));
    track(SettingsPage::UserLexicon, make_group(hwnd_, L"\u65B0\u5EFA\u6216\u7F16\u8F91\u8BCD\u6761", kIdUserLexiconEditorCard, section_font_));
    track(SettingsPage::UserLexicon, make_static(hwnd_, L"\u65B0\u5EFA\u6216\u7F16\u8F91\u8BCD\u6761", kIdUserLexiconEditorTitle, section_font_));
    track(SettingsPage::UserLexicon, make_static(hwnd_, L"\u62FC\u97F3", kIdLexiconPinyinLabel, section_font_));
    track(SettingsPage::UserLexicon, make_static(hwnd_, L"\u8BCD\u8BED", kIdLexiconWordLabel, section_font_));
    track(SettingsPage::UserLexicon, make_static(hwnd_, L"\u9891\u7387", kIdLexiconFrequencyLabel, section_font_));
    lexicon_pinyin_ = track(SettingsPage::UserLexicon, make_edit(hwnd_, kIdLexiconPinyin, body_font_));
    set_cue_banner(lexicon_pinyin_, L"nihao");
    lexicon_word_ = track(SettingsPage::UserLexicon, make_edit(hwnd_, kIdLexiconWord, body_font_));
    set_cue_banner(lexicon_word_, L"\u4F60\u597D");
    lexicon_frequency_ = track(SettingsPage::UserLexicon, make_edit(hwnd_, kIdLexiconFrequency, body_font_));
    set_cue_banner(lexicon_frequency_, L"5000");
    lexicon_new_button_ = track(SettingsPage::UserLexicon,
                                make_button(hwnd_, L"\u65B0\u5EFA\u8BCD\u6761", kIdLexiconNew, BS_PUSHBUTTON, body_font_));
    lexicon_save_button_ = track(SettingsPage::UserLexicon,
                                 make_button(hwnd_, L"\u65B0\u589E\u8BCD\u6761", kIdLexiconSave, BS_DEFPUSHBUTTON, body_font_));
    lexicon_delete_button_ = track(SettingsPage::UserLexicon,
                                   make_button(hwnd_, L"\u5220\u9664\u9009\u4E2D\u8BCD\u6761", kIdLexiconDelete, BS_PUSHBUTTON, body_font_));
    lexicon_status_ = track(SettingsPage::UserLexicon, make_static(hwnd_, L"", kIdLexiconStatus, body_font_));

    LVCOLUMNW column{};
    column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    column.pszText = const_cast<wchar_t*>(L"\u62FC\u97F3");
    column.cx = 160;
    ListView_InsertColumn(lexicon_list_, 0, &column);
    column.pszText = const_cast<wchar_t*>(L"\u8BCD\u8BED");
    column.cx = 320;
    column.iSubItem = 1;
    ListView_InsertColumn(lexicon_list_, 1, &column);
    column.pszText = const_cast<wchar_t*>(L"\u9891\u7387");
    column.cx = 100;
    column.iSubItem = 2;
    ListView_InsertColumn(lexicon_list_, 2, &column);
    install_list_header_theme_subclass();

    track(SettingsPage::Learning, make_static(hwnd_, L"\u5B66\u4E60\u6570\u636E", kIdLearningTitle, title_font_));
    track(SettingsPage::Learning,
          make_static(hwnd_,
                      L"\u672C\u5730\u5B66\u4E60\u53EA\u7528\u4E8E\u6539\u5584\u5019\u9009\u6392\u5E8F\uFF0C\u4E0D\u4F1A\u4E0A\u4F20\u3002",
                      kIdLearningDescription,
                      body_font_));
    track(SettingsPage::Learning, make_group(hwnd_, L"\u5371\u9669\u64CD\u4F5C", kIdLearningCard, section_font_));
    track(SettingsPage::Learning,
          make_static(hwnd_,
                      L"\u6E05\u7A7A\u540E\uFF0CLocalPinyinIME \u4F1A\u5FD8\u8BB0\u4F60\u4E4B\u524D\u9009\u8FC7\u7684\u5019\u9009\u6392\u5E8F\u3002\u8BE5\u64CD\u4F5C\u4E0D\u4F1A\u5220\u9664\u7528\u6237\u8BCD\u5E93\u3002",
                      kIdLearningCardText,
                      body_font_));
    track(SettingsPage::Learning, make_button(hwnd_, L"\u6E05\u7A7A\u5B66\u4E60\u6570\u636E", kIdClearLearning, BS_PUSHBUTTON, body_font_));

    track(SettingsPage::About, make_static(hwnd_, L"\u5173\u4E8E", kIdAboutTitle, title_font_));
    track(SettingsPage::About,
          make_static(hwnd_,
                      L"LocalPinyinIME \u662F\u79BB\u7EBF\u8FD0\u884C\u7684 Windows x64 TSF \u62FC\u97F3\u8F93\u5165\u6CD5\u5B9E\u9A8C\u9879\u76EE\u3002",
                      kIdAboutDescription,
                      body_font_));
    track(SettingsPage::About, make_group(hwnd_, L"\u4FE1\u606F", kIdAboutCard, section_font_));
    track(SettingsPage::About, make_static(hwnd_, L"\u7248\u672C", kIdAboutVersionKey, section_font_));
    track(SettingsPage::About, make_static(hwnd_, LOCALPINYINIME_VERSION_STRING, kIdAboutVersionValue, body_font_));
    track(SettingsPage::About, make_static(hwnd_, L"\u4EA7\u54C1", kIdAboutProductKey, section_font_));
    track(SettingsPage::About, make_static(hwnd_, L"LocalPinyinIME", kIdAboutProductValue, body_font_));
    track(SettingsPage::About, make_static(hwnd_, L"\u67B6\u6784", kIdAboutArchKey, section_font_));
    track(SettingsPage::About, make_static(hwnd_, L"x64", kIdAboutArchValue, body_font_));
    track(SettingsPage::About, make_static(hwnd_, L"\u8FD0\u884C", kIdAboutOfflineKey, section_font_));
    track(SettingsPage::About, make_static(hwnd_, L"\u79BB\u7EBF", kIdAboutOfflineValue, body_font_));
    track(SettingsPage::About, make_static(hwnd_, L"\u72B6\u6001", kIdAboutStatusKey, section_font_));
    track(SettingsPage::About, make_static(hwnd_, L"\u672A\u7B7E\u540D\u5F00\u53D1\u7248", kIdAboutStatusValue, body_font_));
    track(SettingsPage::About, make_static(hwnd_, L"\u6570\u636E\u76EE\u5F55", kIdAboutDataDirKey, section_font_));
    track(SettingsPage::About, make_static(hwnd_, store_.data_dir().c_str(), kIdAboutDataDirValue, body_font_));
    track(SettingsPage::About, make_static(hwnd_, L"GitHub", kIdAboutGithubKey, section_font_));
    track(SettingsPage::About, make_static(hwnd_, kGithubProjectUrl, kIdAboutGithubValue, body_font_));
    track(SettingsPage::About, make_button(hwnd_, L"\u6253\u5F00\u6570\u636E\u76EE\u5F55", kIdOpenDataFolder, BS_PUSHBUTTON, body_font_));
    track(SettingsPage::About, make_button(hwnd_, L"\u590D\u5236\u8DEF\u5F84", kIdCopyDataFolder, BS_PUSHBUTTON, body_font_));
    track(SettingsPage::About, make_button(hwnd_, L"\u6253\u5F00 GitHub", kIdOpenGithubProject, BS_PUSHBUTTON, body_font_));
    track(SettingsPage::About, make_button(hwnd_, L"\u590D\u5236 GitHub \u94FE\u63A5", kIdCopyGithubUrl, BS_PUSHBUTTON, body_font_));

    update_user_lexicon_action_controls();
}

void SettingsWindow::layout_controls() {
    if (!hwnd_) {
        return;
    }

    RECT client{};
    GetClientRect(hwnd_, &client);
    const auto layout = calculate_settings_window_layout(client.right - client.left,
                                                         client.bottom - client.top,
                                                         dpi_);
    const int s6 = scale_settings_value(6, dpi_);
    const int s10 = scale_settings_value(10, dpi_);
    const int s12 = scale_settings_value(12, dpi_);
    const int s20 = scale_settings_value(20, dpi_);
    const int s22 = scale_settings_value(22, dpi_);
    const int s24 = scale_settings_value(24, dpi_);
    const int s28 = scale_settings_value(28, dpi_);
    const int s30 = scale_settings_value(30, dpi_);
    const int s32 = scale_settings_value(32, dpi_);
    const int s34 = scale_settings_value(34, dpi_);
    const int s36 = scale_settings_value(36, dpi_);
    const int s40 = scale_settings_value(40, dpi_);
    const int s44 = scale_settings_value(44, dpi_);
    const int s48 = scale_settings_value(48, dpi_);
    const int s68 = scale_settings_value(68, dpi_);
    const int s72 = scale_settings_value(72, dpi_);
    const int s74 = scale_settings_value(74, dpi_);
    const int s76 = scale_settings_value(76, dpi_);
    const int s84 = scale_settings_value(84, dpi_);
    const int s86 = scale_settings_value(86, dpi_);
    const int s100 = scale_settings_value(100, dpi_);

    move_child(hwnd_, kIdAppTitle, s20, s24, layout.sidebar.width - s40, s30);
    move_child(hwnd_, kIdSidebarDivider, layout.sidebar.width - 1, 0, 1, layout.sidebar.height);
    for (int i = 0; i < kSettingsPageCount; ++i) {
        move_window(nav_buttons_[static_cast<size_t>(i)], layout.nav_items[static_cast<size_t>(i)]);
    }

    const auto place_page_header = [&](int title_id, int description_id) {
        move_child(hwnd_, title_id, layout.page_title);
        move_child(hwnd_, description_id, layout.page_description);
    };

    place_page_header(kIdAppearanceTitle, kIdAppearanceDescription);
    move_child(hwnd_, kIdAppearanceCard, layout.primary_card);
    move_child(hwnd_, kIdAppearanceThemeLabel,
               layout.primary_card.x + s24,
               layout.primary_card.y + s36,
               scale_settings_value(180, dpi_),
               s24);
    move_child(hwnd_, kIdThemeSystem, layout.primary_card.x + s24, layout.primary_card.y + s72, scale_settings_value(150, dpi_), s28);
    move_child(hwnd_, kIdThemeLight, layout.primary_card.x + scale_settings_value(190, dpi_), layout.primary_card.y + s72, scale_settings_value(90, dpi_), s28);
    move_child(hwnd_, kIdThemeDark, layout.primary_card.x + scale_settings_value(300, dpi_), layout.primary_card.y + s72, scale_settings_value(90, dpi_), s28);

    place_page_header(kIdCandidateTitle, kIdCandidateDescription);
    move_child(hwnd_, kIdCandidateCard, layout.primary_card);
    move_child(hwnd_, kIdCandidateHintLabel, layout.primary_card.x + s24, layout.primary_card.y + s36, scale_settings_value(180, dpi_), s24);
    move_child(hwnd_, kIdShowKeyHints, layout.primary_card.x + scale_settings_value(220, dpi_), layout.primary_card.y + s34, scale_settings_value(200, dpi_), s28);
    move_child(hwnd_, kIdCandidateTextSizeLabel, layout.primary_card.x + s24, layout.primary_card.y + s76, scale_settings_value(180, dpi_), s24);
    move_child(hwnd_, kIdTextSmall, layout.primary_card.x + scale_settings_value(220, dpi_), layout.primary_card.y + s74, scale_settings_value(70, dpi_), s28);
    move_child(hwnd_, kIdTextStandard, layout.primary_card.x + scale_settings_value(300, dpi_), layout.primary_card.y + s74, scale_settings_value(90, dpi_), s28);
    move_child(hwnd_, kIdTextLarge, layout.primary_card.x + scale_settings_value(400, dpi_), layout.primary_card.y + s74, scale_settings_value(70, dpi_), s28);
    move_child(hwnd_, kIdShiftToggle, layout.primary_card.x + s24, layout.primary_card.y + scale_settings_value(116, dpi_), scale_settings_value(310, dpi_), s28);

    place_page_header(kIdUserLexiconTitle, kIdUserLexiconDescription);
    move_window(lexicon_search_, SettingsRect{layout.search_box.x + s12,
                                              layout.search_box.y + s6,
                                              layout.search_box.width - s24,
                                              layout.search_box.height - s6 * 2});
    move_child(hwnd_, kIdLexiconRefresh, layout.refresh_button);
    move_window(lexicon_list_, SettingsRect{layout.user_table.x + 1,
                                            layout.user_table.y + 1,
                                            layout.user_table.width - 2,
                                            layout.user_table.height - 2});
    move_window(lexicon_empty_state_, SettingsRect{layout.user_empty_state.x + s24,
                                                   layout.user_empty_state.y + s40,
                                                   layout.user_empty_state.width - s48,
                                                   s44});
    move_child(hwnd_, kIdUserLexiconEditorCard, layout.user_editor_card);
    move_child(hwnd_, kIdUserLexiconEditorTitle, layout.user_editor_title);

    const int editor_x = layout.user_editor_card.x + s24;
    const int editor_y = rect_bottom(layout.user_editor_title) + scale_settings_value(18, dpi_);
    const int editor_width = layout.user_editor_card.width - s48;
    const int frequency_width = scale_settings_value(100, dpi_);
    const int pinyin_width = scale_settings_value(160, dpi_);
    const int word_width = std::max(scale_settings_value(220, dpi_), editor_width - pinyin_width - frequency_width - s24);
    move_child(hwnd_, kIdLexiconPinyinLabel, editor_x, editor_y, pinyin_width, s22);
    move_child(hwnd_, kIdLexiconWordLabel, editor_x + pinyin_width + s12, editor_y, word_width, s22);
    move_child(hwnd_, kIdLexiconFrequencyLabel, editor_x + pinyin_width + s12 + word_width + s12, editor_y, frequency_width, s22);
    move_window(lexicon_pinyin_, SettingsRect{editor_x + s10, editor_y + s24 + s6, pinyin_width - s20, s32 - s6 * 2});
    move_window(lexicon_word_, SettingsRect{editor_x + pinyin_width + s12 + s10, editor_y + s24 + s6, word_width - s20, s32 - s6 * 2});
    move_window(lexicon_frequency_, SettingsRect{editor_x + pinyin_width + s12 + word_width + s12 + s10, editor_y + s24 + s6, frequency_width - s20, s32 - s6 * 2});
    move_window(lexicon_new_button_, SettingsRect{editor_x, editor_y + s68, scale_settings_value(112, dpi_), s32});
    move_window(lexicon_save_button_, SettingsRect{editor_x + scale_settings_value(124, dpi_), editor_y + s68, scale_settings_value(128, dpi_), s32});
    move_window(lexicon_delete_button_, SettingsRect{editor_x + scale_settings_value(264, dpi_), editor_y + s68, scale_settings_value(160, dpi_), s32});
    move_window(lexicon_status_, layout.user_status);

    const auto columns = calculate_user_lexicon_column_widths(layout.user_table.width, dpi_);
    ListView_SetColumnWidth(lexicon_list_, 0, columns.pinyin);
    ListView_SetColumnWidth(lexicon_list_, 1, columns.word);
    ListView_SetColumnWidth(lexicon_list_, 2, columns.frequency);

    place_page_header(kIdLearningTitle, kIdLearningDescription);
    move_child(hwnd_, kIdLearningCard, layout.primary_card);
    move_child(hwnd_, kIdLearningCardText,
               layout.primary_card.x + s24,
               layout.primary_card.y + s40,
               layout.primary_card.width - s48,
               s44);
    move_child(hwnd_, kIdClearLearning,
               layout.primary_card.x + s24,
               layout.primary_card.y + s100,
               scale_settings_value(180, dpi_),
               s32);

    place_page_header(kIdAboutTitle, kIdAboutDescription);
    move_child(hwnd_, kIdAboutCard, layout.about_card);
    const int key_ids[] = {kIdAboutProductKey, kIdAboutVersionKey, kIdAboutArchKey, kIdAboutOfflineKey, kIdAboutStatusKey, kIdAboutDataDirKey, kIdAboutGithubKey};
    const int value_ids[] = {kIdAboutProductValue, kIdAboutVersionValue, kIdAboutArchValue, kIdAboutOfflineValue, kIdAboutStatusValue, kIdAboutDataDirValue, kIdAboutGithubValue};
    for (int i = 0; i < 7; ++i) {
        move_child(hwnd_, key_ids[i], layout.about_labels[static_cast<size_t>(i)]);
        move_child(hwnd_, value_ids[i], layout.about_values[static_cast<size_t>(i)]);
    }
    const std::wstring data_dir = store_.data_dir().empty()
                                      ? L"\u6570\u636E\u76EE\u5F55\u5C1A\u672A\u53EF\u7528"
                                      : compact_path_for_width(hwnd_, body_font_, store_.data_dir(), layout.about_values[5].width);
    set_control_text(hwnd_, kIdAboutDataDirValue, data_dir);
    set_control_text(hwnd_, kIdAboutGithubValue, kGithubProjectUrl);
    move_child(hwnd_, kIdOpenDataFolder, layout.about_open_data_button);
    move_child(hwnd_, kIdCopyDataFolder, layout.about_copy_data_button);
    move_child(hwnd_, kIdOpenGithubProject, layout.about_open_github_button);
    move_child(hwnd_, kIdCopyGithubUrl, layout.about_copy_github_button);
}

void SettingsWindow::show_page(SettingsPage page) {
    active_page_ = page;
    CheckRadioButton(hwnd_, kIdNavAppearance, kIdNavAbout, nav_id_for_page(page));
    for (int i = 0; i < kSettingsPageCount; ++i) {
        const bool visible = i == page_index(page);
        for (HWND control : page_controls_[static_cast<size_t>(i)]) {
            ShowWindow(control, visible ? SW_SHOW : SW_HIDE);
        }
    }
    if (page == SettingsPage::UserLexicon) {
        populate_user_lexicon_list();
    }
    layout_controls();
    InvalidateRect(hwnd_, nullptr, TRUE);
}

void SettingsWindow::apply_options_to_controls() {
    CheckRadioButton(hwnd_, kIdThemeSystem, kIdThemeDark,
                     options_.theme_mode == CandidateThemeMode::Light ? kIdThemeLight :
                     options_.theme_mode == CandidateThemeMode::Dark ? kIdThemeDark : kIdThemeSystem);
    set_checked(hwnd_, kIdShowKeyHints, options_.show_key_hints);
    set_checked(hwnd_, kIdShiftToggle, input_mode_options_.shift_toggle_enabled);
    CheckRadioButton(hwnd_, kIdTextSmall, kIdTextLarge,
                     options_.text_size == CandidateTextSize::Small ? kIdTextSmall :
                     options_.text_size == CandidateTextSize::Large ? kIdTextLarge : kIdTextStandard);
}

void SettingsWindow::save_options_from_controls() {
    if (is_checked(hwnd_, kIdThemeLight)) {
        options_.theme_mode = CandidateThemeMode::Light;
    } else if (is_checked(hwnd_, kIdThemeDark)) {
        options_.theme_mode = CandidateThemeMode::Dark;
    } else {
        options_.theme_mode = CandidateThemeMode::System;
    }

    options_.show_key_hints = is_checked(hwnd_, kIdShowKeyHints);
    input_mode_options_.shift_toggle_enabled = is_checked(hwnd_, kIdShiftToggle);
    if (is_checked(hwnd_, kIdTextSmall)) {
        options_.text_size = CandidateTextSize::Small;
    } else if (is_checked(hwnd_, kIdTextLarge)) {
        options_.text_size = CandidateTextSize::Large;
    } else {
        options_.text_size = CandidateTextSize::Standard;
    }

    store_.save_options(options_, input_mode_options_);
}

void SettingsWindow::open_data_dir() {
    store_.ensure_data_dir();
    ShellExecuteW(hwnd_, L"open", store_.data_dir().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void SettingsWindow::copy_data_dir_to_clipboard() {
    const std::wstring path = store_.data_dir();
    if (path.empty()) {
        MessageBoxW(hwnd_, L"\u6570\u636E\u76EE\u5F55\u5C1A\u672A\u53EF\u7528\u3002", L"LocalPinyinIME", MB_OK | MB_ICONINFORMATION);
        return;
    }
    copy_text_to_clipboard(path,
                           L"\u6570\u636E\u76EE\u5F55\u8DEF\u5F84\u5DF2\u590D\u5236\u3002",
                           L"\u590D\u5236\u8DEF\u5F84\u5931\u8D25\u3002");
}

void SettingsWindow::open_github_project() {
    ShellExecuteW(hwnd_, L"open", kGithubProjectUrl, nullptr, nullptr, SW_SHOWNORMAL);
}

void SettingsWindow::copy_github_url_to_clipboard() {
    copy_text_to_clipboard(kGithubProjectUrl,
                           L"GitHub \u94FE\u63A5\u5DF2\u590D\u5236\u3002",
                           L"\u590D\u5236 GitHub \u94FE\u63A5\u5931\u8D25\u3002");
}

bool SettingsWindow::copy_text_to_clipboard(const std::wstring& text,
                                            const wchar_t* success_message,
                                            const wchar_t* failure_message) {
    if (!OpenClipboard(hwnd_)) {
        MessageBoxW(hwnd_, L"\u65E0\u6CD5\u6253\u5F00\u526A\u8D34\u677F\u3002", L"LocalPinyinIME", MB_OK | MB_ICONERROR);
        return false;
    }

    EmptyClipboard();
    const SIZE_T byte_count = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, byte_count);
    if (!memory) {
        CloseClipboard();
        MessageBoxW(hwnd_, failure_message, L"LocalPinyinIME", MB_OK | MB_ICONERROR);
        return false;
    }

    void* data = GlobalLock(memory);
    if (!data) {
        GlobalFree(memory);
        CloseClipboard();
        MessageBoxW(hwnd_, failure_message, L"LocalPinyinIME", MB_OK | MB_ICONERROR);
        return false;
    }
    std::memcpy(data, text.c_str(), byte_count);
    GlobalUnlock(memory);

    if (!SetClipboardData(CF_UNICODETEXT, memory)) {
        GlobalFree(memory);
        CloseClipboard();
        MessageBoxW(hwnd_, failure_message, L"LocalPinyinIME", MB_OK | MB_ICONERROR);
        return false;
    }
    CloseClipboard();
    MessageBoxW(hwnd_, success_message, L"LocalPinyinIME", MB_OK | MB_ICONINFORMATION);
    return true;
}

void SettingsWindow::clear_learning_data() {
    const int answer = MessageBoxW(hwnd_,
                                   L"\u786E\u8BA4\u6E05\u7A7A LocalPinyinIME \u672C\u5730\u5B66\u4E60\u6570\u636E\uFF1F\n\u8BE5\u64CD\u4F5C\u4E0D\u4F1A\u5220\u9664\u8BCD\u5178 TSV\u3002",
                                   L"LocalPinyinIME",
                                   MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
    if (answer != IDYES) {
        return;
    }

    const int second = MessageBoxW(hwnd_,
                                   L"\u8BF7\u518D\u6B21\u786E\u8BA4\u6E05\u7A7A\u5B66\u4E60\u6570\u636E\u3002",
                                   L"LocalPinyinIME",
                                   MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
    if (second != IDYES) {
        return;
    }

    if (store_.clear_learning_data()) {
        MessageBoxW(hwnd_, L"\u5B66\u4E60\u6570\u636E\u5DF2\u6E05\u7A7A\u3002", L"LocalPinyinIME", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxW(hwnd_, L"\u6E05\u7A7A\u5B66\u4E60\u6570\u636E\u5931\u8D25\u3002", L"LocalPinyinIME", MB_OK | MB_ICONERROR);
    }
}

std::wstring SettingsWindow::user_lexicon_path() const {
    return default_user_lexicon_path();
}

void SettingsWindow::load_user_lexicon() {
    const UserLexiconLoadResult result = load_user_lexicon_file(user_lexicon_path(), true);
    if (result.failed) {
        SetWindowTextW(lexicon_status_, L"\u7528\u6237\u8BCD\u5E93\u8BFB\u53D6\u5931\u8D25\uFF0C\u8BF7\u68C0\u67E5 %LOCALAPPDATA% \u5199\u5165\u6743\u9650\u3002");
        return;
    }

    user_lexicon_entries_ = result.entries;
    populate_user_lexicon_list();

    std::wstringstream stream;
    stream << L"\u5DF2\u8F7D\u5165 " << user_lexicon_entries_.size()
           << L" \u6761\u7528\u6237\u8BCD\u6761";
    if (result.created) {
        stream << L"\uFF0C\u5DF2\u521B\u5EFA\u65B0\u6587\u4EF6";
    }
    SetWindowTextW(lexicon_status_, stream.str().c_str());
}

void SettingsWindow::populate_user_lexicon_list() {
    if (!lexicon_list_) {
        return;
    }
    ListView_DeleteAllItems(lexicon_list_);
    filtered_user_lexicon_indexes_ = filter_user_lexicon_entries(user_lexicon_entries_,
                                                                 control_text(hwnd_, kIdLexiconSearch));
    for (size_t row_index = 0; row_index < filtered_user_lexicon_indexes_.size(); ++row_index) {
        const size_t entry_index = filtered_user_lexicon_indexes_[row_index];
        const auto row = make_user_lexicon_table_row(user_lexicon_entries_[entry_index]);
        LVITEMW item{};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = static_cast<int>(row_index);
        item.iSubItem = 0;
        item.pszText = const_cast<wchar_t*>(row.pinyin.c_str());
        item.lParam = static_cast<LPARAM>(entry_index);
        ListView_InsertItem(lexicon_list_, &item);
        ListView_SetItemText(lexicon_list_, static_cast<int>(row_index), 1, const_cast<wchar_t*>(row.word.c_str()));
        ListView_SetItemText(lexicon_list_, static_cast<int>(row_index), 2, const_cast<wchar_t*>(row.frequency.c_str()));
    }
    const bool empty = filtered_user_lexicon_indexes_.empty();
    const bool query_empty = control_text(hwnd_, kIdLexiconSearch).empty();
    SetWindowTextW(lexicon_empty_state_,
                   query_empty
                       ? L"\u8FD8\u6CA1\u6709\u7528\u6237\u8BCD\u6761\u3002\u53EF\u4EE5\u5728\u4E0B\u65B9\u6DFB\u52A0\u7B2C\u4E00\u6761\u3002"
                       : L"\u6CA1\u6709\u5339\u914D\u7684\u8BCD\u6761\u3002\u8BF7\u5C1D\u8BD5\u5176\u4ED6\u641C\u7D22\u8BCD\u3002");
    ShowWindow(lexicon_empty_state_, empty && active_page_ == SettingsPage::UserLexicon ? SW_SHOW : SW_HIDE);
    ShowWindow(lexicon_list_, empty ? SW_HIDE : SW_SHOW);
    if (selected_user_lexicon_row() < 0) {
        populate_user_lexicon_editor(make_new_user_lexicon_editor_state(false));
    }
    update_user_lexicon_action_controls();
    InvalidateRect(lexicon_list_, nullptr, TRUE);
}

void SettingsWindow::populate_user_lexicon_editor(size_t entry_index) {
    if (entry_index >= user_lexicon_entries_.size()) {
        return;
    }
    const auto row = make_user_lexicon_table_row(user_lexicon_entries_[entry_index]);
    UserLexiconEditorState state;
    state.pinyin = row.pinyin;
    state.word = row.word;
    state.frequency = row.frequency;
    state.delete_enabled = true;
    state.mode = UserLexiconEditorMode::EditExisting;
    state.primary_action_text = L"\u4FDD\u5B58\u4FEE\u6539";
    populate_user_lexicon_editor(state);
}

void SettingsWindow::populate_user_lexicon_editor(const UserLexiconEditorState& state) {
    set_control_text(hwnd_, kIdLexiconPinyin, state.pinyin);
    set_control_text(hwnd_, kIdLexiconWord, state.word);
    set_control_text(hwnd_, kIdLexiconFrequency, state.frequency);
    if (lexicon_save_button_) {
        SetWindowTextW(lexicon_save_button_, state.primary_action_text.c_str());
    }
    EnableWindow(lexicon_delete_button_, state.delete_enabled);
    if (state.focus_pinyin && lexicon_pinyin_) {
        SetFocus(lexicon_pinyin_);
    }
}

void SettingsWindow::enter_new_user_lexicon_state(bool focus_pinyin, const wchar_t* status_text) {
    if (lexicon_list_) {
        ListView_SetItemState(lexicon_list_, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
        InvalidateRect(lexicon_list_, nullptr, TRUE);
    }
    populate_user_lexicon_editor(make_new_user_lexicon_editor_state(focus_pinyin));
    if (status_text && lexicon_status_) {
        SetWindowTextW(lexicon_status_, status_text);
    }
}

void SettingsWindow::enter_edit_user_lexicon_state(size_t entry_index) {
    populate_user_lexicon_editor(entry_index);
}

bool SettingsWindow::select_user_lexicon_entry(const UserLexiconEntry& entry) {
    for (size_t row = 0; row < filtered_user_lexicon_indexes_.size(); ++row) {
        const size_t entry_index = filtered_user_lexicon_indexes_[row];
        if (entry_index >= user_lexicon_entries_.size()) {
            continue;
        }
        const UserLexiconEntry& candidate = user_lexicon_entries_[entry_index];
        if (candidate.pinyin == entry.pinyin && candidate.word == entry.word) {
            const int row_index = static_cast<int>(row);
            ListView_SetItemState(lexicon_list_, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_SetItemState(lexicon_list_, row_index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(lexicon_list_, row_index, FALSE);
            enter_edit_user_lexicon_state(entry_index);
            InvalidateRect(lexicon_list_, nullptr, TRUE);
            return true;
        }
    }
    return false;
}

void SettingsWindow::save_user_lexicon_entry() {
    int frequency = 0;
    if (!parse_user_lexicon_frequency(control_text(hwnd_, kIdLexiconFrequency), &frequency)) {
        SetWindowTextW(lexicon_status_, L"\u6821\u9A8C\u5931\u8D25\uFF1A\u9891\u7387\u5FC5\u987B\u662F\u975E\u8D1F\u6574\u6570\u3002");
        MessageBoxW(hwnd_, L"\u9891\u7387\u5FC5\u987B\u662F\u975E\u8D1F\u6574\u6570\u3002", L"LocalPinyinIME", MB_OK | MB_ICONWARNING);
        return;
    }

    UserLexiconEntry entry;
    const auto validation = validate_user_lexicon_entry(control_text(hwnd_, kIdLexiconPinyin),
                                                       control_text(hwnd_, kIdLexiconWord),
                                                       frequency,
                                                       &entry);
    if (!validation.valid) {
        const std::wstring status = L"\u6821\u9A8C\u5931\u8D25\uFF1A" + validation.message;
        SetWindowTextW(lexicon_status_, status.c_str());
        MessageBoxW(hwnd_, validation.message.c_str(), L"LocalPinyinIME", MB_OK | MB_ICONWARNING);
        return;
    }

    bool updated = false;
    if (!upsert_user_lexicon_entry(user_lexicon_entries_, entry, &updated)) {
        SetWindowTextW(lexicon_status_, L"\u6821\u9A8C\u5931\u8D25\uFF1A\u8BCD\u6761\u65E0\u6CD5\u5199\u5165\u5217\u8868\u3002");
        MessageBoxW(hwnd_, L"\u8BCD\u6761\u6821\u9A8C\u5931\u8D25\u3002", L"LocalPinyinIME", MB_OK | MB_ICONERROR);
        return;
    }

    const UserLexiconSaveResult saved = save_user_lexicon_file_atomic(user_lexicon_path(), user_lexicon_entries_);
    if (!saved.saved) {
        std::wstringstream stream;
        stream << L"\u4FDD\u5B58\u5931\u8D25\uFF0CWin32 error=" << saved.win32_error;
        SetWindowTextW(lexicon_status_, stream.str().c_str());
        MessageBoxW(hwnd_, stream.str().c_str(), L"LocalPinyinIME", MB_OK | MB_ICONERROR);
        return;
    }

    populate_user_lexicon_list();
    if (!select_user_lexicon_entry(entry)) {
        SetWindowTextW(lexicon_search_, L"");
        populate_user_lexicon_list();
        select_user_lexicon_entry(entry);
    }
    log_status(L"settings", user_lexicon_save_log_message(user_lexicon_entries_.size(), updated ? 1 : 0, 0));
    SetWindowTextW(lexicon_status_, user_lexicon_reload_hint().c_str());
    InvalidateRect(hwnd_, nullptr, FALSE);
}

int SettingsWindow::selected_user_lexicon_row() const {
    if (!lexicon_list_) {
        return -1;
    }
    return ListView_GetNextItem(lexicon_list_, -1, LVNI_SELECTED);
}

void SettingsWindow::update_user_lexicon_action_controls() {
    const bool has_selection = selected_user_lexicon_row() >= 0;
    EnableWindow(lexicon_delete_button_, has_selection);
    if (lexicon_save_button_) {
        SetWindowTextW(lexicon_save_button_,
                       has_selection ? L"\u4FDD\u5B58\u4FEE\u6539" : L"\u65B0\u589E\u8BCD\u6761");
    }
}

void SettingsWindow::delete_user_lexicon_entry() {
    const int selected = selected_user_lexicon_row();
    if (selected < 0 || static_cast<size_t>(selected) >= filtered_user_lexicon_indexes_.size()) {
        MessageBoxW(hwnd_, L"\u8BF7\u5148\u9009\u4E2D\u8981\u5220\u9664\u7684\u8BCD\u6761\u3002", L"LocalPinyinIME", MB_OK | MB_ICONINFORMATION);
        return;
    }

    const int answer = MessageBoxW(hwnd_,
                                   L"\u786E\u8BA4\u5220\u9664\u9009\u4E2D\u7684\u7528\u6237\u8BCD\u6761\uFF1F",
                                   L"LocalPinyinIME",
                                   MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
    if (answer != IDYES) {
        return;
    }

    const auto entry = user_lexicon_entries_[filtered_user_lexicon_indexes_[static_cast<size_t>(selected)]];
    if (!remove_user_lexicon_entry(user_lexicon_entries_, entry.pinyin, entry.word)) {
        return;
    }

    const UserLexiconSaveResult saved = save_user_lexicon_file_atomic(user_lexicon_path(), user_lexicon_entries_);
    if (!saved.saved) {
        std::wstringstream stream;
        stream << L"\u5220\u9664\u540E\u4FDD\u5B58\u5931\u8D25\uFF0CWin32 error=" << saved.win32_error;
        SetWindowTextW(lexicon_status_, stream.str().c_str());
        MessageBoxW(hwnd_, stream.str().c_str(), L"LocalPinyinIME", MB_OK | MB_ICONERROR);
        return;
    }

    populate_user_lexicon_list();
    enter_new_user_lexicon_state(false);
    log_status(L"settings", user_lexicon_save_log_message(user_lexicon_entries_.size(), 0, 1));
    SetWindowTextW(lexicon_status_,
                   L"\u5DF2\u5220\u9664\u3002\u8BF7\u5728\u6CA1\u6709\u6B63\u5728\u8F93\u5165\u7684\u62FC\u97F3\u65F6\u7EE7\u7EED\u4F7F\u7528\uFF1BLocalPinyinIME \u4F1A\u5728\u5B89\u5168\u65F6\u673A\u5237\u65B0\u7528\u6237\u8BCD\u5E93\u3002");
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void SettingsWindow::refresh_user_lexicon() {
    load_user_lexicon();
}

}  // namespace localpinyin
