#include "settings_window.h"

#include "../common/logging.h"
#include "../common/utf_utils.h"
#include "../common/win32_utils.h"
#include "dictionary_preview_window.h"
#include "localpinyin_version.h"
#include "local_pinyin_blue_theme.h"
#include "resource.h"

#include <commctrl.h>
#include <shellapi.h>
#include <windowsx.h>

#include <algorithm>
#include <array>
#include <cwchar>
#include <cstring>
#include <cmath>
#include <filesystem>
#include <sstream>
#include <vector>

namespace localpinyin {
namespace {

constexpr int kBaseDpi = 96;
constexpr int kIdOpenDataFolder = 1001;
constexpr int kIdCopyDataFolder = 1002;
constexpr int kIdThemeSystem = 1101;
constexpr int kIdThemeLight = 1102;
constexpr int kIdThemeDark = 1103;
constexpr int kIdShowKeyHints = 1201;
constexpr int kIdShiftToggle = 1202;
constexpr int kIdCandidateHintDescription = 1203;
constexpr int kIdShiftToggleLabel = 1204;
constexpr int kIdShiftToggleDescription = 1205;
constexpr int kIdShiftDefaultNote = 1206;
constexpr int kIdShortcutTestCard = 1207;
constexpr int kIdShortcutTestTitle = 1208;
constexpr int kIdShortcutTestDescription = 1209;
constexpr int kIdShortcutModeLabel = 1210;
constexpr int kIdShortcutModeValue = 1211;
constexpr int kIdShortcutInputLabel = 1212;
constexpr int kIdShortcutInput = 1213;
constexpr int kIdShortcutBoundaryNote = 1214;
constexpr int kIdOpenInputTest = 1215;
constexpr int kIdCandidateSizeSlider = 1301;
constexpr int kIdCandidateSizeValue = 1302;
constexpr int kIdCandidateSizeSmallLabel = 1303;
constexpr int kIdCandidateSizeStandardLabel = 1304;
constexpr int kIdCandidateSizeLargeLabel = 1305;
constexpr int kIdCandidatePreviewInput = 1306;
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
constexpr int kIdBuiltinDictionary = 1512;

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
constexpr int kIdLexiconFrequencyHelp = 3209;
constexpr int kIdPersonalDictionaryTitle = 3210;

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
constexpr int kIdAboutShortcutKey = 3416;
constexpr int kIdAboutShortcutValue = 3417;
constexpr int kIdAboutDictionaryTitle = 3420;
constexpr int kIdAboutDictionaryCount = 3421;
constexpr int kIdAboutDictionaryPreview = 3422;

constexpr int kSettingsPageCount = 5;
constexpr UINT_PTR kUserLexiconHeaderSubclassId = 1;
constexpr wchar_t kCandidateOptionsChangedMessageName[] = L"LocalPinyinIME.CandidateOptionsChanged";
constexpr UINT_PTR kShortcutTestTimerId = 4101;
constexpr UINT kShortcutTestTimerMs = 45;
constexpr wchar_t kUserLexiconFrequencyHelpText[] =
    L"\u9891\u7387\u8D8A\u9AD8\uFF0C\u521D\u59CB\u5019\u9009\u4F4D\u7F6E\u8D8A\u9760\u524D\u3002\u5EFA\u8BAE\u5E38\u7528\u8BCD\u586B 5000\uFF1B\u8F93\u5165\u6CD5\u4F1A\u7ED3\u5408\u4F7F\u7528\u4E60\u60EF\u7EE7\u7EED\u8C03\u6574\u6392\u5E8F\u3002";

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

SettingsKeyboardState current_settings_keyboard_state() {
    const bool left = (GetKeyState(VK_LCONTROL) & 0x8000) != 0;
    const bool right = (GetKeyState(VK_RCONTROL) & 0x8000) != 0;
    const bool generic = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    return SettingsKeyboardState{left || (generic && !right), right};
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
    if (auto* renderer = active_modern_renderer()) {
        renderer->fill(rect, color);
        return;
    }
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);
}

void draw_round_rect(HDC hdc, const RECT& rect, int radius, COLORREF fill, COLORREF border) {
    if (auto* renderer = active_modern_renderer()) {
        renderer->rounded(rect, static_cast<float>(radius), fill, border);
        return;
    }
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
    if (auto* renderer = active_modern_renderer()) {
        LOGFONTW metrics{};
        const bool has_font = font && GetObjectW(font, sizeof(metrics), &metrics) != 0;
        renderer->text(text, rect, color,
                         has_font ? static_cast<float>(std::abs(metrics.lfHeight)) : 14.0f,
                         has_font ? metrics.lfWeight : FW_NORMAL, flags);
        return;
    }
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

void draw_navigation_icon(HDC hdc, int id, const RECT& rect, COLORREF color, int dpi) {
    if (auto* renderer = active_modern_renderer()) {
        renderer->icon(id - kIdNavAppearance, rect, color, scale_settings_value(1, dpi) * 1.5f);
        return;
    }
    const int unit = std::max(1, scale_settings_value(1, dpi));
    HPEN pen = CreatePen(PS_SOLID, unit * 2, color);
    HGDIOBJ old_pen = SelectObject(hdc, pen);
    HGDIOBJ old_brush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    const int x = rect.left;
    const int y = rect.top;
    const auto s = [dpi](int value) { return scale_settings_value(value, dpi); };
    const auto line = [&](int x1, int y1, int x2, int y2) {
        MoveToEx(hdc, x + s(x1), y + s(y1), nullptr);
        LineTo(hdc, x + s(x2), y + s(y2));
    };
    if (id == kIdNavAppearance) {
        RoundRect(hdc, x + s(2), y + s(2), x + s(20), y + s(20), s(6), s(6));
        line(3, 8, 19, 8);
        line(9, 9, 9, 19);
    } else if (id == kIdNavCandidate) {
        RoundRect(hdc, x + s(1), y + s(3), x + s(21), y + s(18), s(5), s(5));
        line(5, 8, 17, 8);
        line(5, 13, 12, 13);
        line(5, 18, 5, 21);
    } else if (id == kIdNavUserLexicon) {
        RoundRect(hdc, x + s(3), y + s(2), x + s(20), y + s(20), s(3), s(3));
        line(8, 3, 8, 19);
        line(11, 8, 16, 8);
        line(11, 13, 16, 13);
    } else if (id == kIdNavLearning) {
        const POINT points[] = {{x + s(11), y + s(1)}, {x + s(14), y + s(8)},
                                {x + s(21), y + s(11)}, {x + s(14), y + s(14)},
                                {x + s(11), y + s(21)}, {x + s(8), y + s(14)},
                                {x + s(1), y + s(11)}, {x + s(8), y + s(8)}};
        Polygon(hdc, points, static_cast<int>(std::size(points)));
    } else {
        Ellipse(hdc, x + s(2), y + s(2), x + s(21), y + s(21));
        line(11, 10, 11, 17);
        line(11, 6, 11, 7);
    }
    SelectObject(hdc, old_brush);
    SelectObject(hdc, old_pen);
    DeleteObject(pen);
}

HFONT create_candidate_preview_font(int height_px, LONG weight) {
    return CreateFontW(-height_px, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
}

CandidateLayoutResult draw_candidate_preview(HDC hdc,
                            const SettingsWindowLayout& settings_layout,
                            const CandidateWindowOptions& options,
                            bool system_dark,
                            int dpi,
                            const std::wstring& pinyin,
                            const std::vector<std::wstring>& candidates) {
    const CandidateThemePalette candidate_palette = make_candidate_palette(options.theme_mode, system_dark);
    const RECT preview_outer = to_win_rect(settings_layout.appearance_preview_window);
    // Use enough virtual screen height to keep the real candidate-window hint
    // policy enabled; the preview itself is still centered inside its card.
    const RECT work_area{preview_outer.left, preview_outer.top, preview_outer.right,
                          preview_outer.bottom + rect_height(preview_outer)};
    const RECT caret_rect{preview_outer.left + scale_settings_value(28, dpi),
                          preview_outer.top + scale_settings_value(24, dpi),
                          preview_outer.left + scale_settings_value(29, dpi),
                          preview_outer.top + scale_settings_value(46, dpi)};

    CandidateLayoutInput input;
    input.composition_text = pinyin;
    // The settings card shows a compact sample of the actual lookup. Keep the
    // first four real results within the two-row space used by this preview.
    input.candidate_texts.assign(candidates.begin(), candidates.begin() + std::min<size_t>(4, candidates.size()));
    input.selected_index = 0;
    input.caret_rect = caret_rect;
    input.work_area = work_area;
    input.dpi = dpi;
    input.options = options;
    if (auto* canvas = active_modern_renderer()) {
        const int font_size = candidate_font_px_for_size(options.text_size, dpi);
        for (size_t i = 0; i < input.candidate_texts.size(); ++i) {
            input.candidate_text_widths_px.push_back(canvas->measure_text(input.candidate_texts[i], static_cast<float>(font_size), FW_MEDIUM));
            input.number_text_widths_px.push_back(canvas->measure_text(std::to_wstring(i + 1), static_cast<float>(font_size)));
        }
    }
    CandidateLayoutResult layout = make_candidate_layout(input);
    // Long sentence candidates can occupy a complete row. Reduce only the
    // displayed tail so every font size stays inside the fixed preview card.
    while (layout.height > rect_height(preview_outer) && input.candidate_texts.size() > 1) {
        input.candidate_texts.pop_back();
        layout = make_candidate_layout(input);
    }
    if (!layout.visible) {
        return layout;
    }

    const int origin_x = preview_outer.left + std::max(0, (rect_width(preview_outer) - layout.width) / 2);
    const int origin_y = preview_outer.top + std::max(0, (rect_height(preview_outer) - layout.height) / 2);
    layout.window_rect = RECT{origin_x, origin_y, origin_x + layout.width, origin_y + layout.height};
    OffsetRect(&layout.composition_rect, origin_x, origin_y);
    OffsetRect(&layout.candidate_area_rect, origin_x, origin_y);
    OffsetRect(&layout.action_button_rect, origin_x, origin_y);
    OffsetRect(&layout.hint_rect, origin_x, origin_y);
    for (auto& item : layout.items) {
        OffsetRect(&item.bounds, origin_x, origin_y);
        OffsetRect(&item.number_bounds, origin_x, origin_y);
        OffsetRect(&item.text_bounds, origin_x, origin_y);
    }

    draw_round_rect(hdc, layout.window_rect, layout.corner_radius, candidate_palette.background, candidate_palette.border);

    HFONT composition_font = create_candidate_preview_font(layout.composition_font_px, FW_NORMAL);
    HFONT candidate_font = create_candidate_preview_font(layout.candidate_font_px, FW_MEDIUM);
    HFONT hint_font = create_candidate_preview_font(layout.hint_font_px, FW_NORMAL);

    draw_text_in_rect(hdc, input.composition_text, layout.composition_rect, candidate_palette.muted_text, composition_font,
                      DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    for (const auto& item : layout.items) {
        if (item.selected) {
            draw_round_rect(hdc,
                            item.bounds,
                            layout.selected_corner_radius,
                            candidate_palette.selected_background,
                            candidate_palette.selected_border);
        }
        draw_text_in_rect(hdc,
                          std::to_wstring(item.candidate_index + 1),
                          item.number_bounds,
                          item.selected ? candidate_palette.selected_text : candidate_palette.number_text,
                          candidate_font,
                          DT_SINGLELINE | DT_VCENTER);
        draw_text_in_rect(hdc,
                          item.display_text,
                          item.text_bounds,
                          item.selected ? candidate_palette.selected_text : candidate_palette.text,
                          candidate_font,
                          candidate_text_draw_flags(item.truncated));
    }
    if (layout.action_button_visible) {
        const RECT& icon_rect = layout.action_button_rect;
        const COLORREF button_fill = blend(candidate_palette.background, candidate_palette.border, 30);
        draw_round_rect(hdc, icon_rect, scale_settings_value(8, dpi), button_fill, candidate_palette.border);
        if (auto* canvas = active_modern_renderer()) {
            RECT icon = icon_rect;
            InflateRect(&icon, -scale_settings_value(6, dpi), -scale_settings_value(6, dpi));
            canvas->icon(8, icon, candidate_palette.muted_text, scale_settings_value(1, dpi) * 1.5f);
        }
    }
    if (layout.show_hint) {
        if (auto* canvas = active_modern_renderer()) {
            const float y = static_cast<float>(layout.hint_rect.top - scale_settings_value(4, dpi));
            canvas->line(static_cast<float>(layout.hint_rect.left), y, static_cast<float>(layout.hint_rect.right), y,
                         candidate_palette.border, 1);
        }
        draw_text_in_rect(hdc,
                          L"\u7A7A\u683C \u786E\u8BA4  \u00B7  \u6570\u5B57 \u9009\u8BCD  \u00B7  Esc \u53D6\u6D88",
                          layout.hint_rect,
                          candidate_palette.hint_text,
                          hint_font,
                          DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    }

    DeleteObject(hint_font);
    DeleteObject(candidate_font);
    DeleteObject(composition_font);
    return layout;
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
    wcscpy_s(font.lfFaceName, L"Microsoft YaHei UI");
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
    (void)parent;
    (void)text;
    (void)id;
    (void)font;
    // Cards are painted by the parent backbuffer. A full-card child HWND would
    // clip that paint pass and can leave stale pixels when pages are switched.
    return nullptr;
}

HWND make_button(HWND parent, const wchar_t* text, int id, DWORD style, HFONT font) {
    // For child controls, CreateWindowExW's hMenu parameter carries the integer control ID.
    // Preserve the native role, selection state and keyboard behavior. The
    // subclass paints the control without replacing checkbox/radio semantics.
    const DWORD group = id == kIdNavAppearance || id == kIdThemeSystem || id == kIdShowKeyHints ||
                        id == kIdOpenInputTest ? WS_GROUP : 0;
    return make_control(parent, L"BUTTON", text, id, style | WS_TABSTOP | group, 0, font);
}

HWND make_edit(HWND parent, int id, HFONT font) {
    return make_control(parent,
                        L"EDIT",
                        L"",
                        id,
                        ES_AUTOHSCROLL | WS_TABSTOP,
                        0,
                        font);
}

HWND make_list_view(HWND parent, int id, HFONT font) {
    HWND list = make_control(parent,
                             WC_LISTVIEWW,
                             L"",
                             id,
                             LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | WS_TABSTOP,
                             0,
                             font);
    const SettingsThemePalette palette = settings_theme_palette(SettingsResolvedTheme::Light);
    ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    ListView_SetBkColor(list, palette.card_background);
    ListView_SetTextBkColor(list, palette.card_background);
    ListView_SetTextColor(list, palette.text);
    return list;
}

HWND make_trackbar(HWND parent, int id, HFONT font) {
    HWND slider = make_control(parent,
                               L"STATIC",
                               L"",
                               id,
                               SS_NOTIFY | WS_TABSTOP | WS_GROUP,
                               0,
                               font);
    return slider;
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
    SetPropW(edit, L"LocalPinyin.ModernCue", reinterpret_cast<HANDLE>(const_cast<wchar_t*>(text)));
    SendMessageW(edit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(text));
}

void notify_candidate_options_changed() {
    const UINT message = RegisterWindowMessageW(kCandidateOptionsChangedMessageName);
    if (message != 0) {
        PostMessageW(HWND_BROADCAST, message, 0, 0);
    }
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
                        scale_settings_value(720, safe_dpi)};
}

SettingsSize settings_initial_client_size(int dpi) {
    const int safe_dpi = dpi <= 0 ? kBaseDpi : dpi;
    const SettingsSize minimum = settings_minimum_client_size(safe_dpi);
    return SettingsSize{std::max(minimum.width, scale_settings_value(1080, safe_dpi)),
                        std::max(minimum.height, scale_settings_value(760, safe_dpi))};
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
    const bool dark = theme == SettingsResolvedTheme::Dark;
    SettingsThemePalette p;
    p.dark = dark;
    p.window_background = dark ? RGB(29, 29, 32) : RGB(247, 248, 250);
    p.sidebar_background = dark ? RGB(24, 24, 27) : RGB(242, 243, 246);
    p.card_background = dark ? RGB(38, 38, 42) : RGB(255, 255, 255);
    p.card_border = dark ? RGB(57, 57, 63) : RGB(231, 233, 237);
    p.text = dark ? RGB(246, 247, 250) : RGB(26, 28, 34);
    p.muted_text = dark ? RGB(176, 178, 187) : RGB(100, 104, 116);
    p.weak_text = dark ? RGB(140, 143, 156) : RGB(138, 143, 154);
    p.accent = dark ? RGB(96, 175, 255) : RGB(0, 103, 192);
    p.accent_soft = dark ? RGB(38, 55, 75) : RGB(227, 239, 252);
    p.accent_border = dark ? RGB(81, 135, 190) : RGB(107, 165, 219);
    p.input_background = dark ? RGB(29, 29, 32) : RGB(250, 251, 253);
    p.input_border = dark ? RGB(68, 68, 76) : RGB(215, 218, 225);
    p.secondary_button = p.card_background;
    p.secondary_button_pressed = dark ? RGB(48, 49, 55) : RGB(239, 241, 245);
    p.primary_button_pressed = dark ? RGB(79, 151, 222) : RGB(0, 82, 156);
    p.danger = dark ? RGB(255, 152, 161) : RGB(183, 43, 57);
    p.danger_soft = dark ? RGB(50, 36, 40) : RGB(255, 248, 248);
    p.danger_border = dark ? RGB(103, 61, 67) : RGB(238, 211, 215);
    p.disabled_background = dark ? RGB(43, 43, 47) : RGB(243, 244, 247);
    p.disabled_text = dark ? RGB(106, 109, 118) : RGB(170, 174, 184);
    p.list_header_background = dark ? RGB(32, 32, 36) : RGB(247, 248, 250);
    p.list_header_text = p.muted_text;
    p.inactive_selection = dark ? RGB(47, 47, 53) : RGB(241, 244, 248);
    return p;
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
            style.text = state.checked ? palette.accent : palette.muted_text;
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
            style.text = state.disabled ? palette.disabled_text :
                         palette.dark ? RGB(17, 35, 56) : RGB(255, 255, 255);
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

SettingsSwitchDrawStyle settings_switch_draw_style(
    const SettingsThemePalette& palette,
    bool checked,
    bool focused) {
    SettingsSwitchDrawStyle style;
    style.thumb_on_right = checked;
    style.uses_dotted_focus_frame = false;
    style.uses_heavy_focus_ring = false;
    if (checked) {
        style.track = palette.accent;
        style.track_border = focused ? blend(palette.accent, RGB(255, 255, 255), palette.dark ? 16 : 10)
                                     : palette.accent;
        style.thumb = RGB(255, 255, 255);
        style.thumb_border = blend(RGB(255, 255, 255), palette.accent, focused ? 18 : 10);
        return style;
    }

    style.track = palette.dark
        ? blend(palette.window_background, palette.card_border, 18)
        : blend(palette.card_border, palette.card_background, 46);
    style.track_border = focused ? blend(palette.card_border, palette.accent, palette.dark ? 18 : 12)
                                 : blend(palette.card_border, style.track, 42);
    style.thumb = palette.dark
        ? blend(RGB(255, 255, 255), palette.card_background, 14)
        : RGB(255, 255, 255);
    style.thumb_border = blend(palette.muted_text, palette.card_background, palette.dark ? 45 : 65);
    return style;
}

SettingsSwitchMetrics settings_switch_metrics(
    const SettingsRect& bounds,
    bool checked,
    int dpi) {
    const int safe_dpi = dpi <= 0 ? kBaseDpi : dpi;
    const int track_inset_x = scale_settings_value(1, safe_dpi);
    const int track_inset_y = scale_settings_value(1, safe_dpi);
    const int thumb_inset = scale_settings_value(4, safe_dpi);
    SettingsSwitchMetrics metrics;
    metrics.track = SettingsRect{bounds.x + track_inset_x,
                                 bounds.y + track_inset_y,
                                 std::max(1, bounds.width - track_inset_x * 2),
                                 std::max(1, bounds.height - track_inset_y * 2)};
    const int thumb_size = std::max(1, metrics.track.height - thumb_inset * 2);
    const int thumb_left = checked
        ? metrics.track.x + metrics.track.width - thumb_inset - thumb_size
        : metrics.track.x + thumb_inset;
    metrics.thumb = SettingsRect{thumb_left,
                                 metrics.track.y + thumb_inset,
                                 thumb_size,
                                 thumb_size};
    metrics.track_corner_radius = metrics.track.height / 2;
    metrics.thumb_corner_radius = thumb_size / 2;
    return metrics;
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

SettingsPageVisibilityPolicy settings_page_visibility_policy() {
    return SettingsPageVisibilityPolicy{};
}

SettingsAboutPathPolicy settings_about_path_policy() {
    return SettingsAboutPathPolicy{};
}

void apply_settings_dark_title_bar(HWND hwnd, bool dark) {
    apply_dark_title_bar(hwnd, dark);
}

void apply_settings_control_theme(HWND hwnd, bool dark) {
    if (!hwnd) return;
    using SetTheme = HRESULT(WINAPI*)(HWND, LPCWSTR, LPCWSTR);
    static const HMODULE module = LoadLibraryW(L"uxtheme.dll");
    static const auto set_theme = module ? reinterpret_cast<SetTheme>(GetProcAddress(module, "SetWindowTheme")) : nullptr;
    if (set_theme) set_theme(hwnd, dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
}

CandidateSizeSliderMetrics calculate_candidate_size_slider_metrics(
    const SettingsRect& bounds,
    int level,
    int dpi) {
    const int safe_dpi = dpi <= 0 ? kBaseDpi : dpi;
    const int clamped_level = std::max(candidate_text_size_min_level(),
                                       std::min(level, candidate_text_size_max_level()));
    const int thumb_size = scale_settings_value(20, safe_dpi);
    const int stop_size = scale_settings_value(8, safe_dpi);
    const int track_height = scale_settings_value(6, safe_dpi);
    const int left = bounds.x + thumb_size / 2;
    const int right = bounds.x + std::max(thumb_size, bounds.width - thumb_size / 2);
    const int center_y = bounds.y + bounds.height / 2;
    CandidateSizeSliderMetrics metrics;
    metrics.track = SettingsRect{left,
                                 center_y - track_height / 2,
                                 std::max(1, right - left),
                                 track_height};
    for (int i = 0; i < 7; ++i) {
        const int stop_x = metrics.track.x + MulDiv(metrics.track.width, i, 6);
        metrics.stops[static_cast<size_t>(i)] = SettingsRect{stop_x - stop_size / 2,
                                                             center_y - stop_size / 2,
                                                             stop_size,
                                                             stop_size};
    }
    const SettingsRect& selected_stop = metrics.stops[static_cast<size_t>(clamped_level - 1)];
    const int selected_x = selected_stop.x + selected_stop.width / 2;
    metrics.thumb = SettingsRect{selected_x - thumb_size / 2,
                                 center_y - thumb_size / 2,
                                 thumb_size,
                                 thumb_size};
    return metrics;
}

int candidate_size_slider_level_from_x(const SettingsRect& bounds, int x, int dpi) {
    const CandidateSizeSliderMetrics metrics = calculate_candidate_size_slider_metrics(
        bounds,
        candidate_text_size_default_level(),
        dpi);
    if (metrics.track.width <= 0) {
        return candidate_text_size_default_level();
    }
    const int relative = std::max(0, std::min(x - metrics.track.x, metrics.track.width));
    const int index = static_cast<int>((static_cast<long long>(relative) * 6 + metrics.track.width / 2) /
                                       metrics.track.width) + 1;
    return std::max(candidate_text_size_min_level(), std::min(index, candidate_text_size_max_level()));
}

CandidateSizeSliderDrawStyle candidate_size_slider_draw_style(
    const SettingsThemePalette& palette,
    const CandidateSizeSliderDrawState& state) {
    CandidateSizeSliderDrawStyle style;
    style.background = palette.card_background;
    style.track = palette.dark ? blend(palette.card_border, palette.text, 12)
                               : blend(palette.card_border, palette.accent, 12);
    style.track_active = state.disabled ? palette.disabled_background : palette.accent_soft;
    style.stop = state.disabled ? palette.disabled_background : palette.card_border;
    style.stop_active = state.disabled ? palette.disabled_text : palette.accent;
    style.thumb = state.disabled ? palette.disabled_background :
                  state.pressed ? palette.primary_button_pressed :
                  state.hot ? blend(palette.accent, palette.accent_soft, 22) :
                  palette.accent;
    style.thumb_border = state.disabled ? palette.disabled_text : palette.accent_border;
    style.focus = palette.accent;
    style.focus_uses_dotted_frame = false;
    style.focus_covers_entire_client_rect = false;
    return style;
}

SettingsVisualSpec settings_visual_spec(int dpi) {
    const int safe_dpi = dpi <= 0 ? kBaseDpi : dpi;
    SettingsVisualSpec spec;
    spec.body_font_px = scale_settings_value(14, safe_dpi);
    spec.title_font_px = scale_settings_value(32, safe_dpi);
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
    spec.uses_full_card_child_windows = false;
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
    const int sidebar_width = scale_settings_value(184, safe_dpi);
    const int margin = scale_settings_value(32, safe_dpi);
    const int gap = scale_settings_value(20, safe_dpi);

    SettingsWindowLayout layout;
    layout.dpi = safe_dpi;
    layout.sidebar = SettingsRect{0, 0, sidebar_width, height};
    layout.content = SettingsRect{sidebar_width, 0, width - sidebar_width, height};
    layout.page_title = SettingsRect{sidebar_width + margin,
                                     margin,
                                     width - sidebar_width - margin * 2,
                                     scale_settings_value(48, safe_dpi)};
    layout.page_description = SettingsRect{layout.page_title.x,
                                           rect_bottom(layout.page_title) + scale_settings_value(4, safe_dpi),
                                           layout.page_title.width,
                                           scale_settings_value(28, safe_dpi)};

    const int nav_x = scale_settings_value(12, safe_dpi);
    int nav_y = scale_settings_value(124, safe_dpi);
    const int nav_width = sidebar_width - nav_x * 2;
    const int nav_height = scale_settings_value(42, safe_dpi);
    const int nav_gap = scale_settings_value(8, safe_dpi);
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
    const int candidate_content_width = std::min(content_width, scale_settings_value(760, safe_dpi));
    const int candidate_card_padding = scale_settings_value(24, safe_dpi);
    const int candidate_toggle_width = scale_settings_value(44, safe_dpi);
    const int candidate_toggle_height = scale_settings_value(24, safe_dpi);
    layout.candidate_options_card = SettingsRect{content_x,
                                                 content_top,
                                                 candidate_content_width,
                                                 scale_settings_value(190, safe_dpi)};
    const int candidate_text_width = std::max(scale_settings_value(220, safe_dpi),
                                              candidate_content_width - candidate_card_padding * 3 - candidate_toggle_width);
    const int candidate_row_one_y = layout.candidate_options_card.y + scale_settings_value(30, safe_dpi);
    const int candidate_row_two_y = layout.candidate_options_card.y + scale_settings_value(100, safe_dpi);
    layout.candidate_hint_title = SettingsRect{layout.candidate_options_card.x + candidate_card_padding,
                                               candidate_row_one_y,
                                               candidate_text_width,
                                               scale_settings_value(24, safe_dpi)};
    layout.candidate_hint_description = SettingsRect{layout.candidate_hint_title.x,
                                                     rect_bottom(layout.candidate_hint_title) + scale_settings_value(2, safe_dpi),
                                                     candidate_text_width,
                                                     scale_settings_value(34, safe_dpi)};
    layout.candidate_hint_toggle = SettingsRect{rect_right(layout.candidate_options_card) - candidate_card_padding - candidate_toggle_width,
                                                candidate_row_one_y + scale_settings_value(8, safe_dpi),
                                                candidate_toggle_width,
                                                candidate_toggle_height};
    layout.candidate_shift_title = SettingsRect{layout.candidate_hint_title.x,
                                                candidate_row_two_y,
                                                candidate_text_width,
                                                scale_settings_value(24, safe_dpi)};
    layout.candidate_shift_description = SettingsRect{layout.candidate_shift_title.x,
                                                      rect_bottom(layout.candidate_shift_title) + scale_settings_value(2, safe_dpi),
                                                      candidate_text_width,
                                                      scale_settings_value(28, safe_dpi)};
    layout.candidate_shift_toggle = SettingsRect{layout.candidate_hint_toggle.x,
                                                 candidate_row_two_y + scale_settings_value(8, safe_dpi),
                                                 candidate_toggle_width,
                                                 candidate_toggle_height};
    layout.candidate_shift_note = SettingsRect{layout.candidate_shift_title.x,
                                               rect_bottom(layout.candidate_shift_description) + scale_settings_value(4, safe_dpi),
                                               candidate_text_width,
                                               scale_settings_value(24, safe_dpi)};
    layout.shortcut_test_card = SettingsRect{content_x,
                                             rect_bottom(layout.candidate_options_card) + gap,
                                             candidate_content_width,
                                             scale_settings_value(326, safe_dpi)};
    layout.shortcut_test_title = SettingsRect{layout.shortcut_test_card.x + candidate_card_padding,
                                              layout.shortcut_test_card.y + scale_settings_value(22, safe_dpi),
                                              candidate_content_width - candidate_card_padding * 2,
                                              scale_settings_value(42, safe_dpi)};
    layout.shortcut_test_description = SettingsRect{layout.shortcut_test_title.x,
                                                    rect_bottom(layout.shortcut_test_title) + scale_settings_value(8, safe_dpi),
                                                    candidate_content_width - candidate_card_padding * 2,
                                                    scale_settings_value(42, safe_dpi)};
    const int key_width = scale_settings_value(132, safe_dpi);
    const int key_height = scale_settings_value(46, safe_dpi);
    const int key_gap = scale_settings_value(14, safe_dpi);
    layout.shortcut_test_ctrl_key = SettingsRect{layout.shortcut_test_title.x,
                                                 rect_bottom(layout.shortcut_test_description) + scale_settings_value(16, safe_dpi),
                                                 key_width,
                                                 key_height};
    layout.shortcut_test_space_key = SettingsRect{rect_right(layout.shortcut_test_ctrl_key) + key_gap,
                                                  layout.shortcut_test_ctrl_key.y,
                                                  key_width,
                                                  key_height};
    const int mode_label_width = scale_settings_value(56, safe_dpi);
    const int mode_gap = scale_settings_value(8, safe_dpi);
    const int mode_left = rect_right(layout.shortcut_test_space_key) + scale_settings_value(26, safe_dpi);
    const int mode_value_width = std::max(scale_settings_value(240, safe_dpi),
                                          rect_right(layout.shortcut_test_card) - candidate_card_padding - mode_left - mode_label_width - mode_gap);
    layout.shortcut_test_mode_label = SettingsRect{rect_right(layout.shortcut_test_space_key) + scale_settings_value(26, safe_dpi),
                                                   layout.shortcut_test_ctrl_key.y,
                                                   mode_label_width,
                                                   key_height};
    layout.shortcut_test_mode_value = SettingsRect{rect_right(layout.shortcut_test_mode_label) + mode_gap,
                                                   layout.shortcut_test_ctrl_key.y,
                                                   mode_value_width,
                                                   key_height};
    layout.shortcut_test_input_label = SettingsRect{layout.shortcut_test_title.x,
                                                    rect_bottom(layout.shortcut_test_ctrl_key) + scale_settings_value(22, safe_dpi),
                                                    candidate_content_width - candidate_card_padding * 2,
                                                    scale_settings_value(22, safe_dpi)};
    layout.shortcut_test_input_box = SettingsRect{layout.shortcut_test_title.x,
                                                  rect_bottom(layout.shortcut_test_input_label) + scale_settings_value(8, safe_dpi),
                                                  candidate_content_width - candidate_card_padding * 2,
                                                  scale_settings_value(36, safe_dpi)};
    layout.shortcut_test_boundary_note = SettingsRect{layout.shortcut_test_title.x,
                                                      rect_bottom(layout.shortcut_test_input_box) + scale_settings_value(10, safe_dpi),
                                                      candidate_content_width - candidate_card_padding * 2,
                                                      scale_settings_value(28, safe_dpi)};
    const int appearance_settings_width = std::min(scale_settings_value(360, safe_dpi), content_width * 47 / 100);
    const int appearance_height = scale_settings_value(510, safe_dpi);
    layout.appearance_theme_card = SettingsRect{content_x, content_top, appearance_settings_width, appearance_height};
    layout.appearance_preview_card = SettingsRect{content_x + appearance_settings_width + gap, content_top,
                                                  content_width - appearance_settings_width - gap, appearance_height};
    layout.appearance_preview_window = SettingsRect{layout.appearance_preview_card.x + scale_settings_value(20, safe_dpi),
                                                    layout.appearance_preview_card.y + scale_settings_value(172, safe_dpi),
                                                    layout.appearance_preview_card.width - scale_settings_value(40, safe_dpi),
                                                    scale_settings_value(176, safe_dpi)};
    layout.candidate_size_slider = SettingsRect{layout.appearance_theme_card.x + scale_settings_value(24, safe_dpi),
                                                layout.appearance_theme_card.y + scale_settings_value(258, safe_dpi),
                                                appearance_settings_width - scale_settings_value(48, safe_dpi),
                                                scale_settings_value(40, safe_dpi)};
    layout.candidate_size_label_left = SettingsRect{layout.candidate_size_slider.x,
                                                    rect_bottom(layout.candidate_size_slider) + scale_settings_value(8, safe_dpi),
                                                    scale_settings_value(72, safe_dpi), scale_settings_value(24, safe_dpi)};
    layout.candidate_size_label_middle = SettingsRect{layout.candidate_size_slider.x + layout.candidate_size_slider.width / 2 - scale_settings_value(36, safe_dpi),
                                                      layout.candidate_size_label_left.y,
                                                      scale_settings_value(72, safe_dpi), scale_settings_value(24, safe_dpi)};
    layout.candidate_size_label_right = SettingsRect{rect_right(layout.candidate_size_slider) - scale_settings_value(72, safe_dpi),
                                                     layout.candidate_size_label_left.y,
                                                     scale_settings_value(72, safe_dpi), scale_settings_value(24, safe_dpi)};
    layout.candidate_size_value = SettingsRect{layout.candidate_size_slider.x,
                                               rect_bottom(layout.candidate_size_label_left) + scale_settings_value(12, safe_dpi),
                                               appearance_settings_width - scale_settings_value(48, safe_dpi), scale_settings_value(28, safe_dpi)};
    layout.search_box = SettingsRect{content_x,
                                     content_top,
                                     content_width - scale_settings_value(244, safe_dpi),
                                     scale_settings_value(44, safe_dpi)};
    layout.refresh_button = SettingsRect{rect_right(layout.search_box) + scale_settings_value(12, safe_dpi),
                                         content_top,
                                         scale_settings_value(72, safe_dpi),
                                         scale_settings_value(44, safe_dpi)};
    layout.builtin_dictionary_button = SettingsRect{rect_right(layout.refresh_button) + scale_settings_value(12, safe_dpi),
                                                      content_top, scale_settings_value(148, safe_dpi),
                                                      scale_settings_value(44, safe_dpi)};
    layout.personal_dictionary_title = SettingsRect{content_x, rect_bottom(layout.search_box) + scale_settings_value(8, safe_dpi),
                                                      content_width, scale_settings_value(24, safe_dpi)};

    const int editor_height = scale_settings_value(278, safe_dpi);
    layout.user_editor_card = SettingsRect{content_x,
                                           height - margin - editor_height,
                                           content_width,
                                           editor_height};
    layout.user_editor_title = SettingsRect{layout.user_editor_card.x + scale_settings_value(24, safe_dpi),
                                            layout.user_editor_card.y + scale_settings_value(18, safe_dpi),
                                            layout.user_editor_card.width - scale_settings_value(48, safe_dpi),
                                            scale_settings_value(24, safe_dpi)};
    layout.user_frequency_help = SettingsRect{layout.user_editor_card.x + scale_settings_value(24, safe_dpi),
                                              rect_bottom(layout.user_editor_title) + scale_settings_value(82, safe_dpi),
                                              layout.user_editor_card.width - scale_settings_value(48, safe_dpi),
                                              scale_settings_value(38, safe_dpi)};
    const int user_action_y = rect_bottom(layout.user_frequency_help) + scale_settings_value(14, safe_dpi);
    layout.user_new_button = SettingsRect{layout.user_frequency_help.x,
                                          user_action_y,
                                          scale_settings_value(112, safe_dpi),
                                          scale_settings_value(36, safe_dpi)};
    layout.user_save_button = SettingsRect{rect_right(layout.user_new_button) + scale_settings_value(12, safe_dpi),
                                           user_action_y,
                                           scale_settings_value(128, safe_dpi),
                                           layout.user_new_button.height};
    layout.user_delete_button = SettingsRect{rect_right(layout.user_save_button) + scale_settings_value(12, safe_dpi),
                                             user_action_y,
                                             scale_settings_value(160, safe_dpi),
                                             layout.user_new_button.height};
    layout.user_status = SettingsRect{layout.user_frequency_help.x,
                                      rect_bottom(layout.user_new_button) + scale_settings_value(16, safe_dpi),
                                      layout.user_frequency_help.width,
                                      scale_settings_value(36, safe_dpi)};
    const int table_y = rect_bottom(layout.personal_dictionary_title) + scale_settings_value(4, safe_dpi);
    const int table_height = std::max(scale_settings_value(140, safe_dpi),
                                      layout.user_editor_card.y - gap - table_y);
    layout.user_table = SettingsRect{content_x, table_y, content_width, table_height};
    layout.user_empty_state = layout.user_table;

    const int about_card_height = scale_settings_value(300, safe_dpi);
    layout.about_card = SettingsRect{content_x,
                                     content_top,
                                     content_width,
                                     std::min(about_card_height, height - content_top - margin)};
    const int about_padding = scale_settings_value(24, safe_dpi);
    const int about_label_width = scale_settings_value(112, safe_dpi);
    const int about_row_height = scale_settings_value(26, safe_dpi);
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
    const int about_button_y = std::min(rect_bottom(layout.about_card) - about_padding - about_action_height,
                                        about_row_y + scale_settings_value(18, safe_dpi));
    layout.about_open_data_button = SettingsRect{about_value_x,
                                                 about_button_y,
                                                 scale_settings_value(152, safe_dpi),
                                                 about_action_height};
    layout.about_copy_data_button = SettingsRect{rect_right(layout.about_open_data_button) + about_action_gap,
                                                 about_button_y,
                                                 scale_settings_value(112, safe_dpi),
                                                 about_action_height};
    layout.about_dictionary_card = SettingsRect{content_x,
                                                rect_bottom(layout.about_card) + gap,
                                                content_width,
                                                scale_settings_value(88, safe_dpi)};
    layout.about_dictionary_title = SettingsRect{layout.about_dictionary_card.x + about_padding,
                                                 layout.about_dictionary_card.y + scale_settings_value(18, safe_dpi),
                                                 scale_settings_value(190, safe_dpi),
                                                 scale_settings_value(26, safe_dpi)};
    layout.about_dictionary_count = SettingsRect{layout.about_dictionary_title.x,
                                                 rect_bottom(layout.about_dictionary_title) + scale_settings_value(6, safe_dpi),
                                                 layout.about_dictionary_card.width - scale_settings_value(230, safe_dpi),
                                                 scale_settings_value(28, safe_dpi)};
    layout.about_dictionary_preview_button = SettingsRect{rect_right(layout.about_dictionary_card) - about_padding - scale_settings_value(120, safe_dpi),
                                                          layout.about_dictionary_card.y + scale_settings_value(32, safe_dpi),
                                                          scale_settings_value(120, safe_dpi),
                                                          about_action_height};
    layout.about_offline_card = SettingsRect{content_x,
                                             rect_bottom(layout.about_dictionary_card) + gap,
                                             content_width,
                                             scale_settings_value(110, safe_dpi)};
    return layout;
}

bool settings_page_control_visible(SettingsPage active_page, SettingsPage owner_page) {
    return active_page == owner_page;
}

SettingsRect settings_page_switch_dirty_rect(const SettingsWindowLayout& layout) {
    return layout.content;
}

std::array<SettingsRect, 2> shortcut_test_key_dirty_rects(
    const SettingsWindowLayout& layout,
    int dpi) {
    const int safe_dpi = dpi <= 0 ? kBaseDpi : dpi;
    std::array<SettingsRect, 2> dirty{layout.shortcut_test_ctrl_key, layout.shortcut_test_space_key};
    const int inflate = scale_settings_value(2, safe_dpi);
    for (SettingsRect& rect : dirty) {
        rect.x -= inflate;
        rect.y -= inflate;
        rect.width += inflate * 2;
        rect.height += inflate * 2;
    }
    return dirty;
}

SettingsRect user_lexicon_input_frame_rect(
    const SettingsWindowLayout& layout,
    UserLexiconInputField field,
    int dpi) {
    const int safe_dpi = dpi <= 0 ? kBaseDpi : dpi;
    const int s24 = scale_settings_value(24, safe_dpi);
    const int s18 = scale_settings_value(18, safe_dpi);
    const int s12 = scale_settings_value(12, safe_dpi);
    const int s24h = scale_settings_value(24, safe_dpi);
    const int s32 = scale_settings_value(32, safe_dpi);
    const int editor_x = layout.user_editor_card.x + s24;
    const int editor_y = rect_bottom(layout.user_editor_title) + s18;
    const int editor_width = layout.user_editor_card.width - s24 * 2;
    const int frequency_width = scale_settings_value(100, safe_dpi);
    const int pinyin_width = scale_settings_value(160, safe_dpi);
    const int word_width = std::max(scale_settings_value(220, safe_dpi),
                                    editor_width - pinyin_width - frequency_width - s12 * 2);
    if (field == UserLexiconInputField::Word) {
        return SettingsRect{editor_x + pinyin_width + s12,
                            editor_y + s24h,
                            word_width,
                            s32};
    }
    if (field == UserLexiconInputField::Frequency) {
        return SettingsRect{editor_x + pinyin_width + s12 + word_width + s12,
                            editor_y + s24h,
                            frequency_width,
                            s32};
    }
    return SettingsRect{editor_x, editor_y + s24h, pinyin_width, s32};
}

bool settings_keyboard_suppresses_control_activation(
    UINT message,
    WPARAM key,
    SettingsKeyboardState state) noexcept {
    if (message != WM_KEYDOWN && message != WM_SYSKEYDOWN) {
        return false;
    }
    return key == VK_SPACE && (state.left_ctrl_down || state.right_ctrl_down);
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

int SettingsWindow::run(HINSTANCE instance, int show_command, std::function<void(HWND)> ready) {
    instance_ = instance;
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    INITCOMMONCONTROLSEX controls{};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&controls);

    store_.ensure_data_dir();
    options_ = store_.load_candidate_options();
    input_mode_options_ = store_.load_input_mode_options();
    load_dictionary_catalog();
    candidate_preview_ready_ = candidate_preview_engine_.load_dictionary_resource_directory(
        std::filesystem::path(module_path(instance)).parent_path().wstring());
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

    constexpr DWORD kSettingsWindowStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
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
                            L"\u672C\u5730\u62FC\u97F3 \u00B7 \u8BBE\u7F6E",
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
    if (ready) ready(hwnd_);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (settings_keyboard_suppresses_control_activation(message.message,
                                                            message.wParam,
                                                            current_settings_keyboard_state())) {
            continue;
        }
        if (dictionary_preview_hwnd_ && IsWindow(dictionary_preview_hwnd_) &&
            (message.hwnd == dictionary_preview_hwnd_ || IsChild(dictionary_preview_hwnd_, message.hwnd)) &&
            IsDialogMessageW(dictionary_preview_hwnd_, &message)) continue;
        if (IsDialogMessageW(hwnd_, &message)) continue;
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
            if (wparam == SIZE_MINIMIZED) {
                set_shortcut_test_timer_enabled(false);
                clear_shortcut_test_key_state();
            } else {
                set_shortcut_test_timer_enabled(shortcut_test_should_sample());
            }
            layout_controls();
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        case WM_DPICHANGED: {
            dpi_ = HIWORD(wparam);
            destroy_fonts();
            create_fonts();
            apply_fonts();
            install_modern_controls();
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
                set_shortcut_test_timer_enabled(shortcut_test_should_sample());
                InvalidateRect(hwnd_, nullptr, FALSE);
            } else {
                set_shortcut_test_timer_enabled(false);
                clear_shortcut_test_key_state();
            }
            return 0;
        case WM_ACTIVATE:
        case WM_ACTIVATEAPP:
            set_shortcut_test_timer_enabled(shortcut_test_should_sample());
            if (wparam) {
                update_runtime_input_mode_status();
            }
            return 0;
        case WM_TIMER:
            if (wparam == kShortcutTestTimerId) {
                if (shortcut_test_should_sample()) {
                    update_shortcut_test_key_state();
                    update_runtime_input_mode_status();
                } else {
                    set_shortcut_test_timer_enabled(false);
                    clear_shortcut_test_key_state();
                    update_runtime_input_mode_status();
                }
                return 0;
            }
            break;
        case WM_COMMAND:
            switch (LOWORD(wparam)) {
                case kIdCandidatePreviewInput:
                    if (HIWORD(wparam) == EN_CHANGE) update_candidate_preview();
                    if (HIWORD(wparam) == EN_SETFOCUS || HIWORD(wparam) == EN_KILLFOCUS)
                        invalidate_candidate_size_preview();
                    return 0;
                case kIdNavAppearance:
                case kIdNavCandidate:
                case kIdNavUserLexicon:
                case kIdNavLearning:
                case kIdNavAbout:
                    if (HIWORD(wparam) != BN_CLICKED) return 0;
                    show_page(page_for_nav_id(LOWORD(wparam)));
                    return 0;
                case kIdOpenDataFolder:
                    open_data_dir();
                    return 0;
                case kIdCopyDataFolder:
                    copy_data_dir_to_clipboard();
                    return 0;
                case kIdAboutDictionaryPreview:
                case kIdBuiltinDictionary:
                    open_dictionary_preview();
                    return 0;
                case kIdOpenInputTest:
                    show_page(SettingsPage::Candidate);
                    SetFocus(shortcut_test_input_);
                    return 0;
                case kIdThemeSystem:
                case kIdThemeLight:
                case kIdThemeDark:
                    if (HIWORD(wparam) != BN_CLICKED) return 0;
                    options_.theme_mode = LOWORD(wparam) == kIdThemeLight ? CandidateThemeMode::Light :
                                          LOWORD(wparam) == kIdThemeDark ? CandidateThemeMode::Dark :
                                          CandidateThemeMode::System;
                    persist_candidate_display_options();
                    refresh_theme();
                    return 0;
                case kIdShowKeyHints:
                    if (HIWORD(wparam) != BN_CLICKED) return 0;
                    options_.show_key_hints = is_checked(hwnd_, kIdShowKeyHints);
                    persist_candidate_display_options();
                    InvalidateRect(GetDlgItem(hwnd_, kIdShowKeyHints), nullptr, FALSE);
                    return 0;
                case kIdShiftToggle:
                    if (HIWORD(wparam) != BN_CLICKED) return 0;
                    input_mode_options_.shift_toggle_enabled = is_checked(hwnd_, kIdShiftToggle);
                    store_.save_options(options_, input_mode_options_);
                    InvalidateRect(GetDlgItem(hwnd_, kIdShiftToggle), nullptr, FALSE);
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
                        RECT client{};
                        GetClientRect(hwnd_, &client);
                        const auto layout = calculate_settings_window_layout(client.right - client.left,
                                                                             client.bottom - client.top,
                                                                             dpi_);
                        RECT dirty = to_win_rect(layout.search_box);
                        InvalidateRect(hwnd_, &dirty, FALSE);
                    }
                    return 0;
                case kIdLexiconPinyin:
                case kIdLexiconWord:
                case kIdLexiconFrequency:
                    if (HIWORD(wparam) == EN_SETFOCUS || HIWORD(wparam) == EN_KILLFOCUS) {
                        invalidate_user_lexicon_input_frame(LOWORD(wparam));
                    }
                    return 0;
                case kIdShortcutInput:
                    if (HIWORD(wparam) == EN_SETFOCUS || HIWORD(wparam) == EN_KILLFOCUS) {
                        RECT client{};
                        GetClientRect(hwnd_, &client);
                        const auto layout = calculate_settings_window_layout(client.right - client.left,
                                                                             client.bottom - client.top,
                                                                             dpi_);
                        RECT dirty = to_win_rect(layout.shortcut_test_input_box);
                        InvalidateRect(hwnd_, &dirty, FALSE);
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
                    RECT bounds{};
                    if (!ListView_GetItemRect(lexicon_list_, row, &bounds, LVIR_BOUNDS)) return CDRF_NEWFONT;
                    ModernRenderScope modern(renderer_, custom->nmcd.hdc, bounds);
                    if (!modern.active()) return CDRF_NEWFONT;
                    fill_rect(custom->nmcd.hdc, bounds, style.background);
                    int column_left = bounds.left;
                    for (int column = 0; column < 3; ++column) {
                        const int column_width = ListView_GetColumnWidth(lexicon_list_, column);
                        RECT cell{column_left, bounds.top, column_left + column_width, bounds.bottom};
                        column_left += column_width;
                        wchar_t text[2048]{};
                        ListView_GetItemText(lexicon_list_, row, column, text, static_cast<int>(std::size(text)));
                        cell.left += scale_settings_value(12, dpi_);
                        cell.right -= scale_settings_value(12, dpi_);
                        draw_text_in_rect(custom->nmcd.hdc, text, cell, style.text, body_font_,
                            DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | (column == 2 ? DT_RIGHT : DT_LEFT));
                    }
                    if (selected) {
                        renderer_.line(static_cast<float>(bounds.left + scale_settings_value(2, dpi_)),
                            static_cast<float>(bounds.top + scale_settings_value(8, dpi_)),
                            static_cast<float>(bounds.left + scale_settings_value(2, dpi_)),
                            static_cast<float>(bounds.bottom - scale_settings_value(8, dpi_)), palette_.accent, static_cast<float>(scale_settings_value(2, dpi_)));
                    }
                    return CDRF_SKIPDEFAULT;
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
                                  id == kIdShiftToggleLabel || id == kIdShortcutTestTitle ||
                                  id == kIdShortcutModeValue || id == kIdShortcutInputLabel ||
                                  id == kIdCandidateTextSizeLabel || id == kIdLexiconPinyinLabel ||
                                  id == kIdLexiconWordLabel || id == kIdLexiconFrequencyLabel ||
                                   id == kIdUserLexiconEditorTitle ||
                                   id == kIdPersonalDictionaryTitle ||
                                   id == kIdAboutProductKey || id == kIdAboutVersionKey ||
                                   id == kIdAboutArchKey || id == kIdAboutOfflineKey ||
                                   id == kIdAboutStatusKey || id == kIdAboutDataDirKey ||
                                   id == kIdAboutShortcutKey || id == kIdAboutDictionaryTitle;
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
        case WM_PRINTCLIENT:
            render(reinterpret_cast<HDC>(wparam));
            return 0;
        case WM_DESTROY:
            if (lexicon_row_metrics_) {
                ImageList_Destroy(lexicon_row_metrics_);
                lexicon_row_metrics_ = nullptr;
            }
            if (dictionary_preview_hwnd_ && IsWindow(dictionary_preview_hwnd_)) {
                DestroyWindow(dictionary_preview_hwnd_);
            }
            dictionary_preview_hwnd_ = nullptr;
            set_shortcut_test_timer_enabled(false);
            clear_shortcut_test_key_state();
            if (lexicon_list_) {
                const HWND header = ListView_GetHeader(lexicon_list_);
                if (header) {
                    RemoveWindowSubclass(header, SettingsWindow::list_header_subclass_proc, kUserLexiconHeaderSubclassId);
                }
            }
            if (candidate_size_slider_) {
                RemoveWindowSubclass(candidate_size_slider_,
                                     SettingsWindow::candidate_size_slider_subclass_proc,
                                     static_cast<UINT_PTR>(kIdCandidateSizeSlider));
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
    body_font_ = create_ui_font(dpi_, 11, FW_NORMAL);
    title_font_ = create_ui_font(dpi_, 23, FW_NORMAL);
    section_font_ = create_ui_font(dpi_, 11, FW_MEDIUM);
}

void SettingsWindow::install_modern_controls() {
    for (const auto& controls : page_controls_) {
        for (HWND control : controls) {
            wchar_t type[32]{};
            GetClassNameW(control, type, static_cast<int>(std::size(type)));
            if (_wcsicmp(type, L"Static") == 0 && GetDlgCtrlID(control) != kIdCandidateSizeSlider) {
                SetWindowSubclass(control, modern_static_subclass_proc, 21, reinterpret_cast<DWORD_PTR>(this));
            } else if (_wcsicmp(type, L"Edit") == 0) {
                SetWindowSubclass(control, modern_edit_subclass_proc, 22, reinterpret_cast<DWORD_PTR>(this));
            } else if (_wcsicmp(type, L"Button") == 0) {
                SetWindowSubclass(control, modern_button_subclass_proc, 23, reinterpret_cast<DWORD_PTR>(this));
            }
        }
    }
    // A one-pixel image supplies a comfortable native list-row height. Row
    // selection and accessibility stay with ListView; DirectWrite paints it.
    for (HWND nav : nav_buttons_) SetWindowSubclass(nav, modern_button_subclass_proc, 23, reinterpret_cast<DWORD_PTR>(this));
    if (lexicon_row_metrics_) {
        ListView_SetImageList(lexicon_list_, nullptr, LVSIL_SMALL);
        ImageList_Destroy(lexicon_row_metrics_);
    }
    lexicon_row_metrics_ = ImageList_Create(1, scale_settings_value(36, dpi_), ILC_COLOR32, 1, 0);
    if (lexicon_row_metrics_) {
        HBITMAP bitmap = CreateBitmap(1, scale_settings_value(36, dpi_), 1, 32, nullptr);
        if (bitmap) {
            ImageList_Add(lexicon_row_metrics_, bitmap, nullptr);
            DeleteObject(bitmap);
        }
        ListView_SetImageList(lexicon_list_, lexicon_row_metrics_, LVSIL_SMALL);
    }
}

void SettingsWindow::render_static_control(HWND control, HDC hdc) {
    RECT bounds{};
    GetClientRect(control, &bounds);
    ModernRenderScope modern(renderer_, hdc, bounds);
    const int id = GetDlgCtrlID(control);
    const bool title = id == kIdAppearanceTitle || id == kIdCandidateTitle ||
                       id == kIdUserLexiconTitle || id == kIdLearningTitle || id == kIdAboutTitle;
    const bool description = id == kIdAppearanceDescription || id == kIdCandidateDescription ||
                             id == kIdUserLexiconDescription || id == kIdLearningDescription || id == kIdAboutDescription;
    const HBRUSH background = brush_for_static_control(id);
    fill_rect(hdc, bounds, background == window_background_brush_ ? palette_.window_background : palette_.card_background);
    HFONT font = reinterpret_cast<HFONT>(SendMessageW(control, WM_GETFONT, 0, 0));
    UINT flags = DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS;
    if (id == kIdLexiconEmptyState) flags |= DT_CENTER;
    if (id == kIdCandidateSizeStandardLabel) flags |= DT_CENTER;
    if (id == kIdCandidateSizeLargeLabel) flags |= DT_RIGHT;
    if (id == kIdLexiconFrequencyHelp || id == kIdShortcutTestDescription || id == kIdLearningCardText) {
        flags = DT_WORDBREAK;
    }
    LOGFONTW metrics{};
    const bool strong = font && GetObjectW(font, sizeof(metrics), &metrics) && metrics.lfWeight >= FW_MEDIUM;
    draw_text_in_rect(hdc, control_text(control), bounds,
                       title || strong ? palette_.text : description ? palette_.muted_text : palette_.muted_text,
                       font, flags);
}

void SettingsWindow::render_edit_hint(HWND control, HDC hdc, bool printing) {
    const std::wstring value = control_text(control);
    if (!printing && (!value.empty() || GetFocus() == control)) return;
    const auto* cue = reinterpret_cast<const wchar_t*>(GetPropW(control, L"LocalPinyin.ModernCue"));
    if (value.empty() && (!cue || !*cue)) return;
    RECT bounds{};
    GetClientRect(control, &bounds);
    ModernRenderScope modern(renderer_, hdc, bounds);
    fill_rect(hdc, bounds, palette_.input_background);
    HFONT font = reinterpret_cast<HFONT>(SendMessageW(control, WM_GETFONT, 0, 0));
    bounds.left += scale_settings_value(2, dpi_);
    draw_text_in_rect(hdc, value.empty() ? std::wstring(cue) : value, bounds,
                       value.empty() ? palette_.weak_text : palette_.text, font,
                       DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
}

LRESULT CALLBACK SettingsWindow::modern_static_subclass_proc(
    HWND control, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR data) {
    auto* self = reinterpret_cast<SettingsWindow*>(data);
    if (message == WM_PAINT && self) {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(control, &paint);
        self->render_static_control(control, dc);
        EndPaint(control, &paint);
        return 0;
    }
    if (message == WM_PRINTCLIENT && self) {
        self->render_static_control(control, reinterpret_cast<HDC>(wparam));
        return 0;
    }
    if (message == WM_ERASEBKGND) return 1;
    if (message == WM_NCDESTROY) RemoveWindowSubclass(control, modern_static_subclass_proc, id);
    return DefSubclassProc(control, message, wparam, lparam);
}

LRESULT CALLBACK SettingsWindow::modern_edit_subclass_proc(
    HWND control, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR data) {
    auto* self = reinterpret_cast<SettingsWindow*>(data);
    const LRESULT result = DefSubclassProc(control, message, wparam, lparam);
    if (message == WM_PAINT && self) {
        HDC dc = GetDC(control);
        self->render_edit_hint(control, dc);
        ReleaseDC(control, dc);
    } else if (message == WM_PRINTCLIENT && self) {
        self->render_edit_hint(control, reinterpret_cast<HDC>(wparam), true);
    } else if (message == WM_SETFOCUS || message == WM_KILLFOCUS) {
        InvalidateRect(control, nullptr, FALSE);
    } else if (message == WM_NCDESTROY) {
        RemovePropW(control, L"LocalPinyin.ModernCue");
        RemoveWindowSubclass(control, modern_edit_subclass_proc, id);
    }
    return result;
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

LRESULT CALLBACK SettingsWindow::modern_button_subclass_proc(
    HWND control, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR data) {
    auto* self = reinterpret_cast<SettingsWindow*>(data);
    const auto render = [&](HDC dc) {
        DRAWITEMSTRUCT item{};
        item.CtlType = ODT_BUTTON;
        item.CtlID = static_cast<UINT>(GetDlgCtrlID(control));
        item.itemAction = ODA_DRAWENTIRE;
        item.hwndItem = control;
        item.hDC = dc;
        GetClientRect(control, &item.rcItem);
        const LRESULT state = SendMessageW(control, BM_GETSTATE, 0, 0);
        if (state & BST_PUSHED) item.itemState |= ODS_SELECTED;
        if (SendMessageW(control, BM_GETCHECK, 0, 0) == BST_CHECKED) item.itemState |= ODS_CHECKED;
        if (!IsWindowEnabled(control)) item.itemState |= ODS_DISABLED;
        if (GetFocus() == control && (SendMessageW(control, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS) == 0)
            item.itemState |= ODS_FOCUS;
        if (GetPropW(control, L"LocalPinyin.Hot")) item.itemState |= ODS_HOTLIGHT;
        self->draw_button(item);
    };
    if (self && message == WM_PAINT) {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(control, &paint);
        render(dc);
        EndPaint(control, &paint);
        return 0;
    }
    if (self && (message == WM_PRINTCLIENT || (message == WM_PRINT && (lparam & PRF_CLIENT)))) {
        render(reinterpret_cast<HDC>(wparam));
        return 0;
    }
    if (message == WM_ERASEBKGND) return 1;
    if (message == WM_MOUSEMOVE && !GetPropW(control, L"LocalPinyin.Hot")) {
        SetPropW(control, L"LocalPinyin.Hot", reinterpret_cast<HANDLE>(1));
        TRACKMOUSEEVENT tracking{sizeof(tracking), TME_LEAVE, control, 0};
        TrackMouseEvent(&tracking);
        InvalidateRect(control, nullptr, FALSE);
    } else if (message == WM_MOUSELEAVE) {
        RemovePropW(control, L"LocalPinyin.Hot");
        InvalidateRect(control, nullptr, FALSE);
    } else if (message == WM_NCDESTROY) {
        RemovePropW(control, L"LocalPinyin.Hot");
        RemoveWindowSubclass(control, modern_button_subclass_proc, id);
    }
    const LRESULT result = DefSubclassProc(control, message, wparam, lparam);
    if (message == BM_SETCHECK || message == BM_SETSTATE || message == WM_SETFOCUS || message == WM_KILLFOCUS ||
        message == WM_ENABLE || message == WM_UPDATEUISTATE || message == WM_THEMECHANGED) {
        InvalidateRect(control, nullptr, FALSE);
    }
    return result;
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
        kIdShiftToggleLabel,
        kIdShortcutTestTitle,
        kIdShortcutModeValue,
        kIdShortcutInputLabel,
        kIdCandidateTextSizeLabel,
        kIdUserLexiconEditorTitle,
        kIdPersonalDictionaryTitle,
        kIdLexiconPinyinLabel,
        kIdLexiconWordLabel,
        kIdLexiconFrequencyLabel,
        kIdAboutProductKey,
        kIdAboutVersionKey,
        kIdAboutArchKey,
        kIdAboutOfflineKey,
        kIdAboutStatusKey,
        kIdAboutDataDirKey,
        kIdAboutShortcutKey,
        kIdAboutDictionaryTitle,
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
    DictionaryPreviewWindow::set_theme(dictionary_preview_hwnd_, palette_.dark);

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
        apply_settings_control_theme(lexicon_list_, palette_.dark);
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
    if (id == kIdAppearanceCard || id == kIdCandidateCard || id == kIdShortcutTestCard ||
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
        case kIdPersonalDictionaryTitle:
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
        id == kIdShowKeyHints || id == kIdShiftToggle) {
        return SettingsControlRole::Option;
    }
    if (id == kIdLexiconSave || id == kIdOpenInputTest) {
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
    if (id == kIdLexiconRefresh || id == kIdBuiltinDictionary) {
        return SettingsControlSurface::Window;
    }
    return SettingsControlSurface::Card;
}

void SettingsWindow::apply_title_bar_theme() {
    if (hwnd_) {
        apply_dark_title_bar(hwnd_, palette_.dark);
        using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        static HMODULE module = LoadLibraryW(L"dwmapi.dll");
        const auto set_attribute = module ? reinterpret_cast<SetAttribute>(GetProcAddress(module, "DwmSetWindowAttribute")) : nullptr;
        if (set_attribute) {
            const DWORD rounded = 2;
            const DWORD backdrop = 2;
            set_attribute(hwnd_, 33, &rounded, sizeof(rounded));
            set_attribute(hwnd_, 35, &palette_.window_background, sizeof(palette_.window_background));
            set_attribute(hwnd_, 36, &palette_.text, sizeof(palette_.text));
            set_attribute(hwnd_, 38, &backdrop, sizeof(backdrop));
        }
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
    ModernRenderScope modern(renderer_, custom_draw.hdc, rect);
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

    if (auto* canvas = active_modern_renderer()) {
        canvas->line(static_cast<float>(rect.left), static_cast<float>(rect.bottom - 1),
                     static_cast<float>(rect.right), static_cast<float>(rect.bottom - 1), style.separator, 1);
        return true;
    }
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
    ModernRenderScope modern(renderer_, hdc, client);
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
                          DT_SINGLELINE | DT_VCENTER | (column == 2 ? DT_RIGHT : DT_LEFT) | DT_END_ELLIPSIS);

        if (active_modern_renderer()) continue;
        HPEN pen = CreatePen(PS_SOLID, 1, style.separator);
        HGDIOBJ old_pen = SelectObject(hdc, pen);
        MoveToEx(hdc, item_rect.right - 1, item_rect.top + scale_settings_value(5, dpi_), nullptr);
        LineTo(hdc, item_rect.right - 1, item_rect.bottom - scale_settings_value(5, dpi_));
        SelectObject(hdc, old_pen);
        DeleteObject(pen);
    }

    if (auto* canvas = active_modern_renderer()) {
        canvas->line(static_cast<float>(client.left), static_cast<float>(client.bottom - 1),
                     static_cast<float>(client.right), static_cast<float>(client.bottom - 1), style.border, 1);
        return;
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
        case WM_PRINTCLIENT:
            if (self) self->paint_list_header(hwnd, reinterpret_cast<HDC>(wparam));
            return 0;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, SettingsWindow::list_header_subclass_proc, kUserLexiconHeaderSubclassId);
            break;
        default:
            break;
    }
    return DefSubclassProc(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK SettingsWindow::candidate_size_slider_subclass_proc(
    HWND hwnd,
    UINT message,
    WPARAM wparam,
    LPARAM lparam,
    UINT_PTR subclass_id,
    DWORD_PTR ref_data) {
    (void)subclass_id;
    auto* self = reinterpret_cast<SettingsWindow*>(ref_data);
    if (!self) {
        return DefSubclassProc(hwnd, message, wparam, lparam);
    }

    const auto update_from_x = [&](int x) {
        RECT client{};
        GetClientRect(hwnd, &client);
        const int level = candidate_size_slider_level_from_x(
            SettingsRect{0, 0, rect_width(client), rect_height(client)},
            x,
            self->dpi_);
        self->update_candidate_text_size_from_level(level);
    };

    switch (message) {
        case WM_ERASEBKGND:
            return 1;
        case WM_LBUTTONDOWN:
            SetFocus(hwnd);
            SetCapture(hwnd);
            self->candidate_size_slider_dragging_ = true;
            update_from_x(GET_X_LPARAM(lparam));
            return 0;
        case WM_MOUSEMOVE:
            if (self->candidate_size_slider_dragging_ && GetCapture() == hwnd) {
                update_from_x(GET_X_LPARAM(lparam));
                return 0;
            }
            break;
        case WM_LBUTTONUP:
            if (self->candidate_size_slider_dragging_) {
                update_from_x(GET_X_LPARAM(lparam));
                self->candidate_size_slider_dragging_ = false;
                if (GetCapture() == hwnd) {
                    ReleaseCapture();
                }
                return 0;
            }
            break;
        case WM_CANCELMODE:
        case WM_CAPTURECHANGED:
            self->candidate_size_slider_dragging_ = false;
            break;
        case WM_KEYDOWN: {
            int level = candidate_text_size_level(self->options_.text_size);
            if (wparam == VK_LEFT) {
                --level;
            } else if (wparam == VK_RIGHT) {
                ++level;
            } else if (wparam == VK_HOME) {
                level = candidate_text_size_min_level();
            } else if (wparam == VK_END) {
                level = candidate_text_size_max_level();
            } else {
                break;
            }
            self->update_candidate_text_size_from_level(level);
            return 0;
        }
        case WM_GETDLGCODE:
            return DLGC_WANTARROWS;
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC hdc = BeginPaint(hwnd, &ps);
            CandidateSizeSliderDrawState state;
            state.focused = GetFocus() == hwnd;
            state.pressed = self->candidate_size_slider_dragging_;
            state.disabled = !IsWindowEnabled(hwnd);
            self->draw_candidate_size_slider(hdc, ps.rcPaint, state);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_PRINTCLIENT: {
            RECT bounds{};
            GetClientRect(hwnd, &bounds);
            CandidateSizeSliderDrawState state;
            state.focused = GetFocus() == hwnd;
            state.pressed = self->candidate_size_slider_dragging_;
            state.disabled = !IsWindowEnabled(hwnd);
            self->draw_candidate_size_slider(reinterpret_cast<HDC>(wparam), bounds, state);
            return 0;
        }
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd,
                                 SettingsWindow::candidate_size_slider_subclass_proc,
                                 static_cast<UINT_PTR>(kIdCandidateSizeSlider));
            break;
        default:
            break;
    }
    return DefSubclassProc(hwnd, message, wparam, lparam);
}

void SettingsWindow::draw_candidate_size_slider(HDC hdc,
                                                const RECT&,
                                                const CandidateSizeSliderDrawState& state) {
    if (!candidate_size_slider_) {
        return;
    }
    RECT client{};
    GetClientRect(candidate_size_slider_, &client);
    const SettingsRect bounds{0, 0, rect_width(client), rect_height(client)};
    const CandidateSizeSliderMetrics metrics = calculate_candidate_size_slider_metrics(
        bounds,
        candidate_text_size_level(options_.text_size),
        dpi_);
    const CandidateSizeSliderDrawStyle style = candidate_size_slider_draw_style(palette_, state);

    ModernRenderScope modern(renderer_, hdc, client);
    if (auto* canvas = active_modern_renderer()) {
        canvas->fill(client, palette_.card_background);
        const float center_y = static_cast<float>(client.bottom) / 2;
        const float left = static_cast<float>(metrics.track.x);
        const float right = static_cast<float>(metrics.track.x + metrics.track.width);
        const float thumb_x = static_cast<float>(metrics.thumb.x + metrics.thumb.width / 2);
        canvas->line(left, center_y, right, center_y, palette_.card_border, static_cast<float>(scale_settings_value(4, dpi_)));
        canvas->line(left, center_y, thumb_x, center_y, palette_.accent, static_cast<float>(scale_settings_value(4, dpi_)));
        const float thumb_radius = static_cast<float>(scale_settings_value(10, dpi_));
        canvas->ellipse(thumb_x, center_y, thumb_radius, thumb_radius, palette_.card_background);
        const float accent_radius = static_cast<float>(scale_settings_value(state.pressed ? 4 : 5, dpi_));
        canvas->ellipse(thumb_x, center_y, accent_radius, accent_radius, palette_.accent);
        const RECT outline{static_cast<int>(thumb_x - thumb_radius), static_cast<int>(center_y - thumb_radius),
                             static_cast<int>(thumb_x + thumb_radius), static_cast<int>(center_y + thumb_radius)};
        canvas->outline(outline, thumb_radius, state.focused ? palette_.accent : palette_.input_border,
                           state.focused ? scale_settings_value(2, dpi_) : 1.0f);
        return;
    }

    fill_rect(hdc, client, style.background);
    draw_round_rect(hdc,
                    to_win_rect(metrics.track),
                    std::max(1, metrics.track.height / 2),
                    style.track,
                    style.track);

    RECT active_track = to_win_rect(metrics.track);
    active_track.right = metrics.thumb.x + metrics.thumb.width / 2;
    if (active_track.right > active_track.left) {
        draw_round_rect(hdc,
                        active_track,
                        std::max(1, metrics.track.height / 2),
                        style.track_active,
                        style.track_active);
    }

    const int current_level = candidate_text_size_level(options_.text_size);
    for (int i = 0; i < 7; ++i) {
        const bool active = i + 1 <= current_level;
        draw_round_rect(hdc,
                        to_win_rect(metrics.stops[static_cast<size_t>(i)]),
                        std::max(1, metrics.stops[static_cast<size_t>(i)].height / 2),
                        active ? style.stop_active : style.stop,
                        active ? style.stop_active : style.stop);
    }

    draw_round_rect(hdc,
                    to_win_rect(metrics.thumb),
                    std::max(1, metrics.thumb.height / 2),
                    style.thumb,
                    style.thumb_border);

    if (state.focused) {
        RECT focus_rect = to_win_rect(metrics.thumb);
        InflateRect(&focus_rect, scale_settings_value(5, dpi_), scale_settings_value(5, dpi_));
        HPEN pen = CreatePen(PS_SOLID, std::max(1, scale_settings_value(2, dpi_)), style.focus);
        HGDIOBJ old_pen = SelectObject(hdc, pen);
        HGDIOBJ old_brush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        RoundRect(hdc,
                  focus_rect.left,
                  focus_rect.top,
                  focus_rect.right,
                  focus_rect.bottom,
                  scale_settings_value(14, dpi_),
                  scale_settings_value(14, dpi_));
        SelectObject(hdc, old_brush);
        SelectObject(hdc, old_pen);
        DeleteObject(pen);
    }
}

void SettingsWindow::draw_shortcut_test_keys(HDC hdc, const SettingsWindowLayout& layout) {
    const auto draw_key = [&](const SettingsRect& settings_rect, const wchar_t* label, bool pressed) {
        const RECT rect = to_win_rect(settings_rect);
        const COLORREF fill = pressed ? palette_.accent_soft : blend(palette_.card_background, palette_.card_border, 35);
        const COLORREF border = pressed ? palette_.accent : palette_.card_border;
        const COLORREF text = pressed ? palette_.text : palette_.muted_text;
        draw_round_rect(hdc, rect, scale_settings_value(8, dpi_), fill, border);
        draw_text_in_rect(hdc,
                          label,
                          rect,
                          text,
                          section_font_,
                          DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS);
    };
    draw_key(layout.shortcut_test_ctrl_key, L"Ctrl", shortcut_test_key_state_.ctrl_down);
    draw_key(layout.shortcut_test_space_key, L"Space", shortcut_test_key_state_.space_down);

    const RECT mode_rect = to_win_rect(layout.shortcut_test_mode_value);
    const bool english = runtime_input_mode_status_.available &&
                         runtime_input_mode_status_.mode == InputMode::English;
    const bool available = runtime_input_mode_status_.available;
    const COLORREF mode_fill = available
        ? (english ? blend(palette_.accent_soft, palette_.card_background, 45)
                   : blend(palette_.accent_soft, palette_.card_background, 20))
        : blend(palette_.card_background, palette_.card_border, 30);
    const COLORREF mode_border = available ? palette_.accent_border : palette_.card_border;
    draw_round_rect(hdc, mode_rect, scale_settings_value(12, dpi_), mode_fill, mode_border);
}

bool SettingsWindow::shortcut_test_should_sample() const {
    if (!hwnd_ || active_page_ != SettingsPage::Candidate || !IsWindowVisible(hwnd_) || IsIconic(hwnd_)) {
        return false;
    }
    return true;
}

void SettingsWindow::set_shortcut_test_timer_enabled(bool enabled) {
    if (!hwnd_) {
        return;
    }
    if (enabled && !shortcut_test_timer_active_) {
        SetTimer(hwnd_, kShortcutTestTimerId, kShortcutTestTimerMs, nullptr);
        shortcut_test_timer_active_ = true;
    } else if (!enabled && shortcut_test_timer_active_) {
        KillTimer(hwnd_, kShortcutTestTimerId);
        shortcut_test_timer_active_ = false;
    }
}

void SettingsWindow::clear_shortcut_test_key_state() {
    if (!shortcut_test_key_state_.ctrl_down && !shortcut_test_key_state_.space_down) {
        return;
    }
    shortcut_test_key_state_ = shortcut_test_key_state_after_sampling_stop();
    invalidate_shortcut_test_key_state_rects();
}

void SettingsWindow::update_shortcut_test_key_state() {
    const ShortcutTestKeyState next = shortcut_test_key_state_from_readings(ShortcutTestKeyReadings{
        (GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0,
        (GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0,
        (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0
    });
    if (next.ctrl_down == shortcut_test_key_state_.ctrl_down &&
        next.space_down == shortcut_test_key_state_.space_down) {
        return;
    }
    shortcut_test_key_state_ = next;
    invalidate_shortcut_test_key_state_rects();
}

void SettingsWindow::invalidate_shortcut_test_key_state_rects() {
    if (!hwnd_) {
        return;
    }
    RECT client{};
    GetClientRect(hwnd_, &client);
    const SettingsWindowLayout layout = calculate_settings_window_layout(client.right - client.left,
                                                                         client.bottom - client.top,
                                                                         dpi_);
    const auto dirty_rects = shortcut_test_key_dirty_rects(layout, dpi_);
    RECT ctrl = to_win_rect(dirty_rects[0]);
    RECT space = to_win_rect(dirty_rects[1]);
    InvalidateRect(hwnd_, &ctrl, FALSE);
    InvalidateRect(hwnd_, &space, FALSE);
}

void SettingsWindow::invalidate_user_lexicon_input_frame(int control_id) {
    if (!hwnd_) {
        return;
    }
    RECT client{};
    GetClientRect(hwnd_, &client);
    const SettingsWindowLayout layout = calculate_settings_window_layout(client.right - client.left,
                                                                         client.bottom - client.top,
                                                                         dpi_);
    UserLexiconInputField field = UserLexiconInputField::Pinyin;
    if (control_id == kIdLexiconPinyin) {
        field = UserLexiconInputField::Pinyin;
    } else if (control_id == kIdLexiconWord) {
        field = UserLexiconInputField::Word;
    } else if (control_id == kIdLexiconFrequency) {
        field = UserLexiconInputField::Frequency;
    } else {
        return;
    }
    RECT dirty = to_win_rect(user_lexicon_input_frame_rect(layout, field, dpi_));
    InflateRect(&dirty, scale_settings_value(2, dpi_), scale_settings_value(2, dpi_));
    InvalidateRect(hwnd_, &dirty, FALSE);
}

void SettingsWindow::update_runtime_input_mode_status() {
    const RuntimeInputModeStatus next = store_.load_runtime_input_mode_status();
    if (next.available == runtime_input_mode_status_.available &&
        next.active == runtime_input_mode_status_.active &&
        next.mode == runtime_input_mode_status_.mode &&
        next.sequence == runtime_input_mode_status_.sequence &&
        next.source == runtime_input_mode_status_.source &&
        next.process_id == runtime_input_mode_status_.process_id &&
        next.updated_at_utc_ms == runtime_input_mode_status_.updated_at_utc_ms) {
        return;
    }
    runtime_input_mode_status_ = next;
    if (shortcut_mode_value_) {
        std::wstring text;
        if (!runtime_input_mode_status_.available) {
            text = L"\u7B49\u5F85\u8F93\u5165";
        } else if (runtime_input_mode_status_.mode == InputMode::Chinese) {
            text = L"\u4E2D\u6587\u8F93\u5165";
        } else {
            text = L"\u82F1\u6587\u8F93\u5165";
        }
        SetWindowTextW(shortcut_mode_value_, text.c_str());
    }
    RECT client{};
    GetClientRect(hwnd_, &client);
    const SettingsWindowLayout layout = calculate_settings_window_layout(client.right - client.left,
                                                                         client.bottom - client.top,
                                                                         dpi_);
    RECT dirty = to_win_rect(layout.shortcut_test_mode_value);
    InvalidateRect(hwnd_, &dirty, FALSE);
}

void SettingsWindow::paint() {
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hwnd_, &ps);
    render(hdc);
    EndPaint(hwnd_, &ps);
}

void SettingsWindow::render(HDC hdc) {
    if (!hdc) return;
    RECT client{};
    GetClientRect(hwnd_, &client);
    if (client.right <= 0 || client.bottom <= 0) return;
    HDC memory = CreateCompatibleDC(hdc);
    HBITMAP bitmap = memory ? CreateCompatibleBitmap(hdc, client.right, client.bottom) : nullptr;
    HGDIOBJ previous_bitmap = bitmap ? SelectObject(memory, bitmap) : nullptr;
    HDC dc = bitmap ? memory : hdc;
    ModernRenderScope modern(renderer_, dc, client);
    const auto layout = calculate_settings_window_layout(client.right, client.bottom, dpi_);
    const auto spec = settings_visual_spec(dpi_);
    const auto s = [this](int value) { return scale_settings_value(value, dpi_); };
    const auto label = [&](const wchar_t* value, const RECT& bounds, COLORREF color, HFONT font,
                           UINT flags = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS) {
        draw_text_in_rect(dc, value, bounds, color, font, flags);
    };
    const auto card = [&](const SettingsRect& r) {
        draw_round_rect(dc, to_win_rect(r), spec.card_corner_radius, palette_.card_background, palette_.card_border);
    };
    const auto input = [&](const SettingsRect& r, HWND edit) {
        draw_round_rect(dc, to_win_rect(r), s(6), palette_.input_background,
                          GetFocus() == edit ? palette_.accent : palette_.input_border);
        if (GetFocus() == edit && active_modern_renderer()) {
            active_modern_renderer()->line(static_cast<float>(r.x + s(6)), static_cast<float>(r.y + r.height - 1),
                static_cast<float>(r.x + r.width - s(6)), static_cast<float>(r.y + r.height - 1), palette_.accent, static_cast<float>(s(2)));
        }
    };
    fill_rect(dc, client, palette_.window_background);
    fill_rect(dc, to_win_rect(layout.sidebar), palette_.sidebar_background);
    draw_round_rect(dc, RECT{s(20), s(28), s(52), s(60)}, s(9), palette_.accent, palette_.accent);
    label(L"P", RECT{s(20), s(28), s(52), s(60)}, palette_.dark ? RGB(17,35,56) : RGB(255,255,255),
          section_font_, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
    label(L"本地拼音", RECT{s(64), s(24), layout.sidebar.width - s(12), s(49)}, palette_.text, section_font_);
    label(L"离线智能输入", RECT{s(64), s(47), layout.sidebar.width - s(8), s(72)}, palette_.muted_text, body_font_);
    if (auto* canvas = active_modern_renderer()) {
        canvas->text(L"设置", RECT{s(22), s(94), layout.sidebar.width - s(12), s(116)}, palette_.weak_text, static_cast<float>(s(12)));
    }
    const RECT status{s(16), client.bottom - s(156), layout.sidebar.width - s(16), client.bottom - s(60)};
    draw_round_rect(dc, status, s(10), palette_.accent_soft, palette_.accent_soft);
    label(L"完全本地", RECT{status.left + s(14), status.top + s(11), status.right - s(10), status.top + s(33)}, palette_.text, section_font_);
    label(L"输入和学习记录\n只保存在你的电脑", RECT{status.left + s(14), status.top + s(37), status.right - s(10), status.bottom - s(8)},
          palette_.muted_text, body_font_, DT_WORDBREAK);
    if (auto* canvas = active_modern_renderer()) {
        canvas->text(L"LocalPinyinIME  " LOCALPINYINIME_VERSION_STRING,
                     RECT{s(22), client.bottom - s(42), layout.sidebar.width - s(10), client.bottom - s(18)},
                     palette_.weak_text, static_cast<float>(s(11)));
    }

    if (active_page_ == SettingsPage::Appearance) {
        card(layout.appearance_theme_card);
        card(layout.appearance_preview_card);
        const auto& theme = layout.appearance_theme_card;
        const auto& preview = layout.appearance_preview_card;
        if (auto* canvas = active_modern_renderer()) {
            canvas->line(static_cast<float>(theme.x + s(24)), static_cast<float>(theme.y + s(198)),
                         static_cast<float>(rect_right(theme) - s(24)), static_cast<float>(theme.y + s(198)), palette_.card_border, 1);
        }
        label(L"候选窗预览", RECT{preview.x + s(24), preview.y + s(23), rect_right(preview) - s(24), preview.y + s(51)}, palette_.text, section_font_);
        label(L"与真实候选窗使用相同的显示设置", RECT{preview.x + s(24), preview.y + s(55), rect_right(preview) - s(24), preview.y + s(83)}, palette_.muted_text, body_font_);
        label(L"试填拼音", RECT{preview.x + s(24), preview.y + s(101), rect_right(preview) - s(24), preview.y + s(123)}, palette_.weak_text, body_font_);
        const RECT sample{preview.x + s(20), preview.y + s(133), rect_right(preview) - s(20), preview.y + s(178)};
        draw_round_rect(dc, sample, s(7), palette_.input_background,
                        GetFocus() == candidate_preview_input_ ? palette_.accent : palette_.input_border);
        candidate_preview_layout_ = draw_candidate_preview(dc, layout, options_, windows_apps_use_dark_theme(), dpi_,
                                                          candidate_preview_pinyin_, candidate_preview_texts_);
        if (candidate_preview_texts_.empty()) {
            label(candidate_preview_ready_ ? L"输入拼音查看真实候选" : L"词库未能加载，请重新打开设置",
                  to_win_rect(layout.appearance_preview_window), palette_.muted_text, body_font_,
                  DT_SINGLELINE | DT_VCENTER | DT_CENTER);
        }
        label(L"空格确认首选词 · 数字键选词", RECT{preview.x + s(24), preview.y + s(369), rect_right(preview) - s(24), preview.y + s(393)}, palette_.muted_text, body_font_);
        label(L"点击下方按钮，试试实际中文输入", RECT{preview.x + s(24), preview.y + s(397), rect_right(preview) - s(24), preview.y + s(421)}, palette_.weak_text, body_font_);
        const RECT note{theme.x + s(24), theme.y + s(407), rect_right(theme) - s(24), theme.y + s(457)};
        label(L"主题会同时应用到设置和候选窗。\n字号调整即时生效。", note, palette_.muted_text, body_font_, DT_WORDBREAK);
        label(L"设置会自动保存", RECT{theme.x + s(24), theme.y + s(465), rect_right(theme) - s(24), theme.y + s(487)}, palette_.weak_text, body_font_);
    } else if (active_page_ == SettingsPage::Candidate) {
        card(layout.candidate_options_card);
        card(layout.shortcut_test_card);
        input(layout.shortcut_test_input_box, shortcut_test_input_);
        draw_shortcut_test_keys(dc, layout);
        if (auto* canvas = active_modern_renderer()) {
            canvas->line(static_cast<float>(layout.candidate_options_card.x + s(24)), static_cast<float>(layout.candidate_options_card.y + s(92)),
                         static_cast<float>(rect_right(layout.candidate_options_card) - s(24)), static_cast<float>(layout.candidate_options_card.y + s(92)), palette_.card_border, 1);
        }
    } else if (active_page_ == SettingsPage::UserLexicon) {
        input(layout.search_box, lexicon_search_);
        card(layout.user_table);
        card(layout.user_editor_card);
        input(user_lexicon_input_frame_rect(layout, UserLexiconInputField::Pinyin, dpi_), lexicon_pinyin_);
        input(user_lexicon_input_frame_rect(layout, UserLexiconInputField::Word, dpi_), lexicon_word_);
        input(user_lexicon_input_frame_rect(layout, UserLexiconInputField::Frequency, dpi_), lexicon_frequency_);
        if (filtered_user_lexicon_indexes_.empty() && active_modern_renderer()) {
            const int cx = layout.user_table.x + layout.user_table.width / 2;
            const int cy = layout.user_table.y + s(46);
            draw_round_rect(dc, RECT{cx - s(24), cy - s(24), cx + s(24), cy + s(24)}, s(14), palette_.input_background, palette_.input_background);
            active_modern_renderer()->icon(2, RECT{cx - s(12), cy - s(12), cx + s(12), cy + s(12)}, palette_.weak_text, s(1) * 1.5f);
        }
    } else if (active_page_ == SettingsPage::Learning) {
        card(layout.primary_card);
        label(L"重置个人排序", RECT{layout.primary_card.x + s(24), layout.primary_card.y + s(25), rect_right(layout.primary_card) - s(220), layout.primary_card.y + s(53)}, palette_.text, section_font_);
        const SettingsRect features{layout.secondary_card.x, layout.secondary_card.y, layout.secondary_card.width, s(258)};
        card(features);
        const wchar_t* titles[] = {L"候选学习", L"整句输入", L"专属词库"};
        const wchar_t* descriptions[] = {L"根据选词习惯调整排序，让常用的表达更容易找到。", L"连续输入拼音，组合词语生成整句候选。", L"添加人名、专业术语和常用语，保存后即可使用。"};
        const int column = (features.width - s(48) - s(32)) / 3;
        for (int i = 0; i < 3; ++i) {
            const int x = features.x + s(24) + i * (column + s(16));
            const int y = features.y + s(24);
            draw_round_rect(dc, RECT{x, y, x + s(42), y + s(42)}, s(12), palette_.accent_soft, palette_.accent_soft);
            draw_navigation_icon(dc, i == 2 ? kIdNavUserLexicon : kIdNavLearning, RECT{x + s(10), y + s(10), x + s(32), y + s(32)}, palette_.accent, dpi_);
            label(titles[i], RECT{x, y + s(61), x + column, y + s(87)}, palette_.text, section_font_);
            label(descriptions[i], RECT{x, y + s(101), x + column, y + s(183)}, palette_.muted_text, body_font_, DT_WORDBREAK);
        }
    } else if (active_page_ == SettingsPage::About) {
        card(layout.about_card);
        card(layout.about_dictionary_card);
        card(layout.about_offline_card);
        const int x = layout.about_offline_card.x + s(24);
        const int y = layout.about_offline_card.y + s(15);
        label(L"开始输入", RECT{x, y, rect_right(layout.about_offline_card) - s(24), y + s(25)}, palette_.text, section_font_);
        label(L"Win + 空格选择本地拼音，Ctrl + Space 切换中英文。\n输入拼音，空格确认；数字 1–9 选词，Esc 取消当前输入。",
              RECT{x, y + s(35), rect_right(layout.about_offline_card) - s(24), rect_bottom(layout.about_offline_card) - s(10)},
              palette_.muted_text, body_font_, DT_WORDBREAK);
    }
    modern.finish();
    if (bitmap) {
        BitBlt(hdc, 0, 0, client.right, client.bottom, memory, 0, 0, SRCCOPY);
        SelectObject(memory, previous_bitmap);
        DeleteObject(bitmap);
    }
    if (memory) DeleteDC(memory);
}

void SettingsWindow::draw_button(const DRAWITEMSTRUCT& item) {
    if (!item.hwndItem || !item.hDC) {
        return;
    }

    const int id = static_cast<int>(item.CtlID);
    ModernRenderScope modern(renderer_, item.hDC, item.rcItem);
    const bool disabled = (item.itemState & ODS_DISABLED) != 0 || !IsWindowEnabled(item.hwndItem);
    const bool pressed = (item.itemState & ODS_SELECTED) != 0;
    const bool hot = (item.itemState & ODS_HOTLIGHT) != 0 || GetPropW(item.hwndItem, L"LocalPinyin.Hot") != nullptr;
    const bool checked = SendMessageW(item.hwndItem, BM_GETCHECK, 0, 0) == BST_CHECKED;
    const bool focused = (item.itemState & ODS_FOCUS) != 0;
    const std::wstring text = control_text(item.hwndItem);
    RECT rect = item.rcItem;
    const auto spec = settings_visual_spec(dpi_);

    const bool is_nav = id == kIdNavAppearance || id == kIdNavCandidate ||
                        id == kIdNavUserLexicon || id == kIdNavLearning || id == kIdNavAbout;
    const bool is_option = id == kIdThemeSystem || id == kIdThemeLight || id == kIdThemeDark ||
                           id == kIdShowKeyHints || id == kIdShiftToggle;
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
    if (id == kIdShowKeyHints || id == kIdShiftToggle) {
        const SettingsSwitchDrawStyle switch_style = settings_switch_draw_style(palette_, checked, focused);
        const SettingsSwitchMetrics metrics = settings_switch_metrics(
            SettingsRect{rect.left, rect.top, rect_width(rect), rect_height(rect)},
            switch_style.thumb_on_right,
            dpi_);
        const RECT track = to_win_rect(metrics.track);
        const RECT thumb = to_win_rect(metrics.thumb);
        draw_round_rect(item.hDC,
                        track,
                        metrics.track_corner_radius,
                        switch_style.track,
                        switch_style.track_border);
        draw_round_rect(item.hDC,
                        thumb,
                        metrics.thumb_corner_radius,
                        switch_style.thumb,
                        switch_style.thumb_border);
        return;
    }
    if (id == kIdThemeSystem || id == kIdThemeLight || id == kIdThemeDark) {
        draw_round_rect(item.hDC, rect, scale_settings_value(9, dpi_),
                          hot ? palette_.input_background : palette_.card_background,
                          checked ? palette_.accent : palette_.card_border);
        const int inset = scale_settings_value(12, dpi_);
        RECT miniature{rect.left + inset, rect.top + inset, rect.right - inset,
                         rect.top + scale_settings_value(56, dpi_)};
        const bool dark_miniature = id == kIdThemeDark || (id == kIdThemeSystem && windows_apps_use_dark_theme());
        const COLORREF miniature_bg = dark_miniature ? RGB(39, 41, 47) : RGB(245, 247, 250);
        const COLORREF miniature_line = dark_miniature ? RGB(65, 69, 77) : RGB(222, 226, 233);
        draw_round_rect(item.hDC, miniature, scale_settings_value(5, dpi_), miniature_bg, miniature_line);
        if (auto* canvas = active_modern_renderer()) {
            canvas->line(static_cast<float>(miniature.left + scale_settings_value(16, dpi_)),
                         static_cast<float>(miniature.top + scale_settings_value(8, dpi_)),
                         static_cast<float>(miniature.left + scale_settings_value(16, dpi_)),
                         static_cast<float>(miniature.bottom - scale_settings_value(8, dpi_)), miniature_line, 1);
            const int x = miniature.left + scale_settings_value(24, dpi_);
            const COLORREF miniature_accent = dark_miniature ? RGB(96, 175, 255) : RGB(0, 103, 192);
            draw_round_rect(item.hDC,
                RECT{x, miniature.top + scale_settings_value(10, dpi_), miniature.right - scale_settings_value(9, dpi_), miniature.top + scale_settings_value(17, dpi_)},
                scale_settings_value(3, dpi_), miniature_accent, miniature_accent);
            canvas->line(static_cast<float>(x), static_cast<float>(miniature.top + scale_settings_value(25, dpi_)),
                         static_cast<float>(miniature.right - scale_settings_value(10, dpi_)),
                         static_cast<float>(miniature.top + scale_settings_value(25, dpi_)), miniature_line, 2);
        }
        RECT caption{rect.left + scale_settings_value(4, dpi_), rect.top + scale_settings_value(68, dpi_),
                       rect.right - scale_settings_value(4, dpi_), rect.bottom - scale_settings_value(8, dpi_)};
        draw_text_in_rect(item.hDC, text, caption, palette_.text, body_font_, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        return;
    }
    draw_round_rect(item.hDC, rect, radius, style.fill, style.border);

    RECT text_rect = rect;
    if (is_nav) {
        const int icon_size = scale_settings_value(22, dpi_);
        const int icon_top = rect.top + (rect_height(rect) - icon_size) / 2;
        draw_navigation_icon(item.hDC, id,
                               RECT{rect.left + scale_settings_value(14, dpi_), icon_top,
                                    rect.left + scale_settings_value(14, dpi_) + icon_size, icon_top + icon_size},
                               style.text, dpi_);
        text_rect.left += scale_settings_value(48, dpi_);
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

    nav_buttons_[page_index(SettingsPage::Appearance)] =
        make_button(hwnd_, L"\u5916\u89C2", kIdNavAppearance, BS_AUTORADIOBUTTON | BS_PUSHLIKE | BS_LEFT, body_font_);
    nav_buttons_[page_index(SettingsPage::Candidate)] =
        make_button(hwnd_, L"输入", kIdNavCandidate, BS_AUTORADIOBUTTON | BS_PUSHLIKE | BS_LEFT, body_font_);
    nav_buttons_[page_index(SettingsPage::UserLexicon)] =
        make_button(hwnd_, L"词库", kIdNavUserLexicon, BS_AUTORADIOBUTTON | BS_PUSHLIKE | BS_LEFT, body_font_);
    nav_buttons_[page_index(SettingsPage::Learning)] =
        make_button(hwnd_, L"学习", kIdNavLearning, BS_AUTORADIOBUTTON | BS_PUSHLIKE | BS_LEFT, body_font_);
    nav_buttons_[page_index(SettingsPage::About)] =
        make_button(hwnd_, L"关于", kIdNavAbout, BS_AUTORADIOBUTTON | BS_PUSHLIKE | BS_LEFT, body_font_);

    auto track = [&](SettingsPage page, HWND control) {
        if (control) {
            page_controls_[page_index(page)].push_back(control);
        }
        return control;
    };

    track(SettingsPage::Appearance, make_static(hwnd_, L"外观", kIdAppearanceTitle, title_font_));
    track(SettingsPage::Appearance,
          make_static(hwnd_,
                      L"选一个舒服的主题，调整候选字号。",
                      kIdAppearanceDescription,
                      body_font_));
    track(SettingsPage::Appearance, make_group(hwnd_, L"\u5019\u9009\u7A97\u9884\u89C8", kIdAppearanceCard, section_font_));
    track(SettingsPage::Appearance, make_static(hwnd_, L"主题", kIdAppearanceThemeLabel, section_font_));
    track(SettingsPage::Appearance, make_button(hwnd_, L"\u8DDF\u968F\u7CFB\u7EDF", kIdThemeSystem, BS_AUTORADIOBUTTON, body_font_));
    track(SettingsPage::Appearance, make_button(hwnd_, L"\u6D45\u8272", kIdThemeLight, BS_AUTORADIOBUTTON, body_font_));
    track(SettingsPage::Appearance, make_button(hwnd_, L"\u6DF1\u8272", kIdThemeDark, BS_AUTORADIOBUTTON, body_font_));
    track(SettingsPage::Appearance, make_static(hwnd_, L"候选字号", kIdCandidateTextSizeLabel, section_font_));
    candidate_size_slider_ = track(SettingsPage::Appearance, make_trackbar(hwnd_, kIdCandidateSizeSlider, body_font_));
    SetWindowSubclass(candidate_size_slider_,
                      SettingsWindow::candidate_size_slider_subclass_proc,
                      static_cast<UINT_PTR>(kIdCandidateSizeSlider),
                      reinterpret_cast<DWORD_PTR>(this));
    candidate_size_value_ = track(SettingsPage::Appearance, make_static(hwnd_, L"", kIdCandidateSizeValue, body_font_));
    track(SettingsPage::Appearance, make_static(hwnd_, L"\u5C0F", kIdCandidateSizeSmallLabel, body_font_));
    track(SettingsPage::Appearance, make_static(hwnd_, L"\u6807\u51C6", kIdCandidateSizeStandardLabel, body_font_));
    track(SettingsPage::Appearance, make_static(hwnd_, L"\u5927", kIdCandidateSizeLargeLabel, body_font_));
    candidate_preview_input_ = track(SettingsPage::Appearance,
        make_edit(hwnd_, kIdCandidatePreviewInput, body_font_));
    SendMessageW(candidate_preview_input_, EM_SETLIMITTEXT, 256, 0);
    set_cue_banner(candidate_preview_input_, L"输入拼音，如 suibian");
    SetWindowTextW(candidate_preview_input_, L"nihao");
    track(SettingsPage::Appearance, make_button(hwnd_, L"打开输入体验", kIdOpenInputTest, BS_PUSHBUTTON, body_font_));

    track(SettingsPage::Candidate, make_static(hwnd_, L"输入", kIdCandidateTitle, title_font_));
    track(SettingsPage::Candidate,
          make_static(hwnd_,
                      L"设置切换方式，直接体验中文输入。",
                      kIdCandidateDescription,
                      body_font_));
    track(SettingsPage::Candidate, make_group(hwnd_, L"\u6309\u952E\u63D0\u793A / \u5207\u6362\u65B9\u5F0F", kIdCandidateCard, section_font_));
    track(SettingsPage::Candidate, make_static(hwnd_, L"\u6309\u952E\u63D0\u793A", kIdCandidateHintLabel, section_font_));
    track(SettingsPage::Candidate,
          make_static(hwnd_,
                      L"\u5728\u5019\u9009\u7A97\u5E95\u90E8\u663E\u793A\u9009\u8BCD\u4E0E\u786E\u8BA4\u7684\u5FEB\u6377\u952E\u3002",
                      kIdCandidateHintDescription,
                      body_font_));
    track(SettingsPage::Candidate, make_button(hwnd_, L"按键提示", kIdShowKeyHints, BS_AUTOCHECKBOX, body_font_));
    track(SettingsPage::Candidate, make_static(hwnd_, L"Shift \u5207\u6362\u4E2D\u82F1\u6587", kIdShiftToggleLabel, section_font_));
    track(SettingsPage::Candidate,
          make_static(hwnd_,
                      L"\u5F00\u542F\u540E\uFF0C\u8F7B\u6309\u4E00\u6B21 Shift \u5373\u53EF\u5207\u6362\u3002",
                      kIdShiftToggleDescription,
                      body_font_));
    track(SettingsPage::Candidate, make_button(hwnd_, L"Shift 切换中英文", kIdShiftToggle, BS_AUTOCHECKBOX, body_font_));
    track(SettingsPage::Candidate,
          make_static(hwnd_,
                      L"\u968F\u65F6\u53EF\u4EE5\u4F7F\u7528 Ctrl + Space \u5207\u6362\u3002",
                      kIdShiftDefaultNote,
                      body_font_));
    track(SettingsPage::Candidate, make_group(hwnd_, L"\u7269\u7406\u6309\u952E\u89C2\u5BDF", kIdShortcutTestCard, section_font_));
    track(SettingsPage::Candidate, make_static(hwnd_, L"\u8BD5\u8BD5\u4F60\u7684\u8F93\u5165\u6CD5", kIdShortcutTestTitle, section_font_));
    track(SettingsPage::Candidate,
          make_static(hwnd_,
                      L"\u5148\u7528 Win + \u7A7A\u683C\u9009\u62E9\u672C\u5730\u62FC\u97F3\uFF0C\u518D\u5728\u4E0B\u65B9\u8F93\u5165\u3002\u6309\u952E\u6309\u4E0B\u65F6\u4F1A\u4EAE\u8D77\u3002",
                      kIdShortcutTestDescription,
                      body_font_));
    track(SettingsPage::Candidate, make_static(hwnd_, L"\u72B6\u6001", kIdShortcutModeLabel, body_font_));
    shortcut_mode_value_ = track(SettingsPage::Candidate, make_static(hwnd_, L"\u7B49\u5F85\u8F93\u5165", kIdShortcutModeValue, section_font_));
    track(SettingsPage::Candidate, make_static(hwnd_, L"\u8F93\u5165\u533A", kIdShortcutInputLabel, section_font_));
    shortcut_test_input_ = track(SettingsPage::Candidate, make_edit(hwnd_, kIdShortcutInput, body_font_));
    set_cue_banner(shortcut_test_input_, L"\u8F93\u5165 nihao\uFF0C\u6309\u7A7A\u683C\u5F97\u5230\u300C\u4F60\u597D\u300D");
    track(SettingsPage::Candidate,
          make_static(hwnd_,
                      L"\u7A7A\u683C\u786E\u8BA4\u9996\u9009\u9879 \u00B7 \u6570\u5B57 1\u20139 \u9009\u8BCD \u00B7 Esc \u53D6\u6D88\u5F53\u524D\u8F93\u5165",
                      kIdShortcutBoundaryNote,
                      body_font_));

    track(SettingsPage::UserLexicon, make_static(hwnd_, L"词库", kIdUserLexiconTitle, title_font_));
    const std::wstring lexicon_description = L"内置 " + std::to_wstring(dictionary_catalog_.stats().total_unique_entries) +
                                             L" 条词语；下方管理你的个人词条。";
    track(SettingsPage::UserLexicon,
          make_static(hwnd_,
                      lexicon_description.c_str(),
                      kIdUserLexiconDescription,
                      body_font_));
    lexicon_search_ = track(SettingsPage::UserLexicon, make_edit(hwnd_, kIdLexiconSearch, body_font_));
    set_cue_banner(lexicon_search_, L"搜索个人词条（拼音或中文）");
    track(SettingsPage::UserLexicon, make_button(hwnd_, L"\u5237\u65B0", kIdLexiconRefresh, BS_PUSHBUTTON, body_font_));
    track(SettingsPage::UserLexicon, make_button(hwnd_, L"浏览内置词库", kIdBuiltinDictionary, BS_PUSHBUTTON, body_font_));
    track(SettingsPage::UserLexicon, make_static(hwnd_, L"个人词库", kIdPersonalDictionaryTitle, section_font_));
    lexicon_list_ = track(SettingsPage::UserLexicon, make_list_view(hwnd_, kIdLexiconList, body_font_));
    lexicon_empty_state_ = track(SettingsPage::UserLexicon,
                                 make_static(hwnd_,
                                             L"还没有个人词条，可以在下方添加。",
                                             kIdLexiconEmptyState,
                                             body_font_));
    track(SettingsPage::UserLexicon, make_group(hwnd_, L"\u65B0\u5EFA\u6216\u7F16\u8F91\u8BCD\u6761", kIdUserLexiconEditorCard, section_font_));
    track(SettingsPage::UserLexicon, make_static(hwnd_, L"\u65B0\u5EFA\u6216\u7F16\u8F91\u8BCD\u6761", kIdUserLexiconEditorTitle, section_font_));
    track(SettingsPage::UserLexicon, make_static(hwnd_, L"\u62FC\u97F3", kIdLexiconPinyinLabel, section_font_));
    track(SettingsPage::UserLexicon, make_static(hwnd_, L"\u8BCD\u8BED", kIdLexiconWordLabel, section_font_));
    track(SettingsPage::UserLexicon, make_static(hwnd_, L"\u9891\u7387", kIdLexiconFrequencyLabel, section_font_));
    track(SettingsPage::UserLexicon, make_static(hwnd_, kUserLexiconFrequencyHelpText, kIdLexiconFrequencyHelp, body_font_));
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

    track(SettingsPage::Learning, make_static(hwnd_, L"学习", kIdLearningTitle, title_font_));
    track(SettingsPage::Learning,
          make_static(hwnd_,
                      L"了解本地输入法怎样记住你的用词习惯。",
                      kIdLearningDescription,
                      body_font_));
    track(SettingsPage::Learning, make_group(hwnd_, L"\u5371\u9669\u64CD\u4F5C", kIdLearningCard, section_font_));
    track(SettingsPage::Learning,
          make_static(hwnd_,
                      L"\u91CD\u65B0\u5F00\u59CB\u5B66\u4E60\u4F60\u7684\u7528\u8BCD\u4E60\u60EF\u3002\u81EA\u5B9A\u4E49\u8BCD\u5E93\u4F1A\u4FDD\u7559\u3002",
                      kIdLearningCardText,
                      body_font_));
    track(SettingsPage::Learning, make_button(hwnd_, L"重置学习记录", kIdClearLearning, BS_PUSHBUTTON, body_font_));

    track(SettingsPage::About, make_static(hwnd_, L"关于", kIdAboutTitle, title_font_));
    track(SettingsPage::About,
          make_static(hwnd_,
                      L"一个轻巧、完全本地的中文输入法。",
                      kIdAboutDescription,
                      body_font_));
    track(SettingsPage::About, make_group(hwnd_, L"\u4FE1\u606F", kIdAboutCard, section_font_));
    track(SettingsPage::About, make_static(hwnd_, L"\u7248\u672C", kIdAboutVersionKey, section_font_));
    track(SettingsPage::About, make_static(hwnd_, LOCALPINYINIME_VERSION_STRING, kIdAboutVersionValue, body_font_));
    track(SettingsPage::About, make_static(hwnd_, L"\u4EA7\u54C1", kIdAboutProductKey, section_font_));
    track(SettingsPage::About, make_static(hwnd_, L"\u672C\u5730\u62FC\u97F3 \u00B7 LocalPinyinIME", kIdAboutProductValue, body_font_));
    track(SettingsPage::About, make_static(hwnd_, L"\u67B6\u6784", kIdAboutArchKey, section_font_));
    track(SettingsPage::About, make_static(hwnd_, L"x64", kIdAboutArchValue, body_font_));
    track(SettingsPage::About, make_static(hwnd_, L"\u8FD0\u884C", kIdAboutOfflineKey, section_font_));
    track(SettingsPage::About, make_static(hwnd_, L"\u5B8C\u5168\u672C\u5730 \u00B7 \u65E0\u9700\u7F51\u7EDC", kIdAboutOfflineValue, body_font_));
    track(SettingsPage::About, make_static(hwnd_, L"\u667A\u80FD\u8F93\u5165", kIdAboutStatusKey, section_font_));
    track(SettingsPage::About, make_static(hwnd_, L"\u6574\u53E5\u8054\u60F3 \u00B7 \u4E2A\u4EBA\u7528\u8BCD\u5B66\u4E60", kIdAboutStatusValue, body_font_));
    track(SettingsPage::About, make_static(hwnd_, L"\u6570\u636E\u76EE\u5F55", kIdAboutDataDirKey, section_font_));
    track(SettingsPage::About, make_static(hwnd_, store_.data_dir().c_str(), kIdAboutDataDirValue, body_font_));
    track(SettingsPage::About, make_static(hwnd_, L"\u6A21\u5F0F\u5207\u6362", kIdAboutShortcutKey, section_font_));
    track(SettingsPage::About, make_static(hwnd_, L"Ctrl + Space \u5207\u6362\u4E2D\u82F1\u6587", kIdAboutShortcutValue, body_font_));
    track(SettingsPage::About, make_button(hwnd_, L"\u6253\u5F00\u6570\u636E\u76EE\u5F55", kIdOpenDataFolder, BS_PUSHBUTTON, body_font_));
    track(SettingsPage::About, make_button(hwnd_, L"\u590D\u5236\u8DEF\u5F84", kIdCopyDataFolder, BS_PUSHBUTTON, body_font_));
    track(SettingsPage::About, make_static(hwnd_, L"\u5185\u7F6E\u8BCD\u5E93", kIdAboutDictionaryTitle, section_font_));
    const std::wstring dictionary_count = std::to_wstring(dictionary_catalog_.stats().total_unique_entries) +
                                           L" 条本地词条 · 常用基础 " +
                                           std::to_wstring(dictionary_catalog_.stats().standard_entries) + L" 条";
    track(SettingsPage::About, make_static(hwnd_, dictionary_count.c_str(), kIdAboutDictionaryCount, body_font_));
    track(SettingsPage::About, make_button(hwnd_, L"\u9884\u89C8\u8BCD\u5E93", kIdAboutDictionaryPreview, BS_PUSHBUTTON, body_font_));
    install_modern_controls();
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
    move_child(hwnd_, kIdAppearanceCard, layout.appearance_preview_card);
    move_child(hwnd_, kIdAppearanceThemeLabel, layout.appearance_theme_card.x + s24,
               layout.appearance_theme_card.y + s24, layout.appearance_theme_card.width - s48, s28);
    const int theme_width = (layout.appearance_theme_card.width - s48 - s12 * 2) / 3;
    const int theme_y = layout.appearance_theme_card.y + scale_settings_value(74, dpi_);
    move_child(hwnd_, kIdThemeSystem, layout.appearance_theme_card.x + s24, theme_y, theme_width, scale_settings_value(106, dpi_));
    move_child(hwnd_, kIdThemeLight, layout.appearance_theme_card.x + s24 + theme_width + s12, theme_y, theme_width, scale_settings_value(106, dpi_));
    move_child(hwnd_, kIdThemeDark, layout.appearance_theme_card.x + s24 + (theme_width + s12) * 2, theme_y, theme_width, scale_settings_value(106, dpi_));
    move_child(hwnd_, kIdCandidateTextSizeLabel, layout.appearance_theme_card.x + s24,
               layout.appearance_theme_card.y + scale_settings_value(222, dpi_), layout.appearance_theme_card.width - s48, s28);
    move_child(hwnd_, kIdOpenInputTest, layout.appearance_preview_card.x + s24,
               layout.appearance_preview_card.y + scale_settings_value(446, dpi_), layout.appearance_preview_card.width - s48, scale_settings_value(40, dpi_));
    move_window(candidate_preview_input_, SettingsRect{
        layout.appearance_preview_card.x + scale_settings_value(35, dpi_),
        layout.appearance_preview_card.y + scale_settings_value(145, dpi_),
        layout.appearance_preview_card.width - scale_settings_value(67, dpi_),
        scale_settings_value(23, dpi_)});
    move_window(candidate_size_slider_, layout.candidate_size_slider);
    move_window(candidate_size_value_, layout.candidate_size_value);
    move_child(hwnd_, kIdCandidateSizeSmallLabel, layout.candidate_size_label_left);
    move_child(hwnd_, kIdCandidateSizeStandardLabel, layout.candidate_size_label_middle);
    move_child(hwnd_, kIdCandidateSizeLargeLabel, layout.candidate_size_label_right);

    place_page_header(kIdCandidateTitle, kIdCandidateDescription);
    move_child(hwnd_, kIdCandidateCard, layout.candidate_options_card);
    move_child(hwnd_, kIdCandidateHintLabel, layout.candidate_hint_title);
    move_child(hwnd_, kIdCandidateHintDescription, layout.candidate_hint_description);
    move_child(hwnd_, kIdShowKeyHints, layout.candidate_hint_toggle);
    move_child(hwnd_, kIdShiftToggleLabel, layout.candidate_shift_title);
    move_child(hwnd_, kIdShiftToggleDescription, layout.candidate_shift_description);
    move_child(hwnd_, kIdShiftToggle, layout.candidate_shift_toggle);
    move_child(hwnd_, kIdShiftDefaultNote, layout.candidate_shift_note);
    move_child(hwnd_, kIdShortcutTestCard, layout.shortcut_test_card);
    move_child(hwnd_, kIdShortcutTestTitle, layout.shortcut_test_title);
    move_child(hwnd_, kIdShortcutTestDescription, layout.shortcut_test_description);
    move_child(hwnd_, kIdShortcutModeLabel, layout.shortcut_test_mode_label);
    move_child(hwnd_, kIdShortcutModeValue, layout.shortcut_test_mode_value);
    move_child(hwnd_, kIdShortcutInputLabel, layout.shortcut_test_input_label);
    move_window(shortcut_test_input_, SettingsRect{layout.shortcut_test_input_box.x + s10,
                                                   layout.shortcut_test_input_box.y + s6,
                                                   layout.shortcut_test_input_box.width - s20,
                                                   layout.shortcut_test_input_box.height - s6 * 2});
    move_child(hwnd_, kIdShortcutBoundaryNote, layout.shortcut_test_boundary_note);

    place_page_header(kIdUserLexiconTitle, kIdUserLexiconDescription);
    move_window(lexicon_search_, SettingsRect{layout.search_box.x + s12,
                                              layout.search_box.y + s6,
                                              layout.search_box.width - s24,
                                              layout.search_box.height - s6 * 2});
    move_child(hwnd_, kIdLexiconRefresh, layout.refresh_button);
    move_child(hwnd_, kIdBuiltinDictionary, layout.builtin_dictionary_button);
    move_child(hwnd_, kIdPersonalDictionaryTitle, layout.personal_dictionary_title);
    move_window(lexicon_list_, SettingsRect{layout.user_table.x + 1,
                                            layout.user_table.y + 1,
                                            layout.user_table.width - 2,
                                            layout.user_table.height - 2});
    move_window(lexicon_empty_state_, SettingsRect{layout.user_empty_state.x + s24,
                                                   layout.user_empty_state.y + scale_settings_value(82, dpi_),
                                                   layout.user_empty_state.width - s48,
                                                   scale_settings_value(58, dpi_)});
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
    move_child(hwnd_, kIdLexiconFrequencyHelp, layout.user_frequency_help);
    move_window(lexicon_new_button_, layout.user_new_button);
    move_window(lexicon_save_button_, layout.user_save_button);
    move_window(lexicon_delete_button_, layout.user_delete_button);
    move_window(lexicon_status_, layout.user_status);

    const auto columns = calculate_user_lexicon_column_widths(layout.user_table.width, dpi_);
    ListView_SetColumnWidth(lexicon_list_, 0, columns.pinyin);
    ListView_SetColumnWidth(lexicon_list_, 1, columns.word);
    ListView_SetColumnWidth(lexicon_list_, 2, columns.frequency);

    place_page_header(kIdLearningTitle, kIdLearningDescription);
    move_child(hwnd_, kIdLearningCard, layout.primary_card);
    move_child(hwnd_, kIdLearningCardText,
               layout.primary_card.x + s24,
               layout.primary_card.y + scale_settings_value(62, dpi_),
               layout.primary_card.width - scale_settings_value(260, dpi_),
               scale_settings_value(56, dpi_));
    move_child(hwnd_, kIdClearLearning,
               rect_right(layout.primary_card) - s24 - scale_settings_value(170, dpi_),
               layout.primary_card.y + scale_settings_value(55, dpi_),
               scale_settings_value(170, dpi_),
               scale_settings_value(40, dpi_));

    place_page_header(kIdAboutTitle, kIdAboutDescription);
    move_child(hwnd_, kIdAboutCard, layout.about_card);
    const int key_ids[] = {kIdAboutProductKey, kIdAboutVersionKey, kIdAboutArchKey, kIdAboutOfflineKey, kIdAboutStatusKey, kIdAboutDataDirKey, kIdAboutShortcutKey};
    const int value_ids[] = {kIdAboutProductValue, kIdAboutVersionValue, kIdAboutArchValue, kIdAboutOfflineValue, kIdAboutStatusValue, kIdAboutDataDirValue, kIdAboutShortcutValue};
    for (int i = 0; i < 7; ++i) {
        move_child(hwnd_, key_ids[i], layout.about_labels[static_cast<size_t>(i)]);
        move_child(hwnd_, value_ids[i], layout.about_values[static_cast<size_t>(i)]);
    }
    const std::wstring data_dir = store_.data_dir().empty()
                                      ? L"\u6570\u636E\u76EE\u5F55\u5C1A\u672A\u53EF\u7528"
                                      : compact_path_for_width(hwnd_, body_font_, store_.data_dir(), layout.about_values[5].width);
    set_control_text(hwnd_, kIdAboutDataDirValue, data_dir);
    move_child(hwnd_, kIdOpenDataFolder, layout.about_open_data_button);
    move_child(hwnd_, kIdCopyDataFolder, layout.about_copy_data_button);
    move_child(hwnd_, kIdAboutDictionaryTitle, layout.about_dictionary_title);
    move_child(hwnd_, kIdAboutDictionaryCount, layout.about_dictionary_count);
    move_child(hwnd_, kIdAboutDictionaryPreview, layout.about_dictionary_preview_button);
}

void SettingsWindow::show_page(SettingsPage page) {
    const SettingsPage previous_page = active_page_;
    active_page_ = page;
    CheckRadioButton(hwnd_, kIdNavAppearance, kIdNavAbout, nav_id_for_page(page));
    InvalidateRect(GetDlgItem(hwnd_, nav_id_for_page(previous_page)), nullptr, FALSE);
    InvalidateRect(GetDlgItem(hwnd_, nav_id_for_page(page)), nullptr, FALSE);
    const SettingsPageVisibilityPolicy visibility_policy = settings_page_visibility_policy();
    for (int i = 0; i < kSettingsPageCount; ++i) {
        const SettingsPage owner_page = static_cast<SettingsPage>(i);
        const bool visible = !visibility_policy.show_only_active_page_controls ||
                             settings_page_control_visible(page, owner_page);
        for (HWND control : page_controls_[static_cast<size_t>(i)]) {
            ShowWindow(control, visible ? SW_SHOW : SW_HIDE);
        }
    }
    if (page == SettingsPage::UserLexicon) {
        populate_user_lexicon_list();
    }
    if (previous_page == SettingsPage::Candidate && page != SettingsPage::Candidate) {
        set_shortcut_test_timer_enabled(false);
        clear_shortcut_test_key_state();
    } else if (page == SettingsPage::Candidate) {
        set_shortcut_test_timer_enabled(shortcut_test_should_sample());
        update_runtime_input_mode_status();
    }
    layout_controls();
    RECT client{};
    GetClientRect(hwnd_, &client);
    const SettingsWindowLayout layout = calculate_settings_window_layout(client.right - client.left,
                                                                         client.bottom - client.top,
                                                                         dpi_);
    RECT dirty = to_win_rect(settings_page_switch_dirty_rect(layout));
    InvalidateRect(hwnd_, &dirty, FALSE);
}

void SettingsWindow::apply_options_to_controls() {
    CheckRadioButton(hwnd_, kIdThemeSystem, kIdThemeDark,
                     options_.theme_mode == CandidateThemeMode::Light ? kIdThemeLight :
                     options_.theme_mode == CandidateThemeMode::Dark ? kIdThemeDark : kIdThemeSystem);
    set_checked(hwnd_, kIdShowKeyHints, options_.show_key_hints);
    set_checked(hwnd_, kIdShiftToggle, input_mode_options_.shift_toggle_enabled);
    sync_candidate_text_size_controls();
}

void SettingsWindow::update_candidate_preview() {
    candidate_preview_pinyin_ = control_text(candidate_preview_input_);
    candidate_preview_texts_.clear();
    if (candidate_preview_ready_) {
        for (const auto& candidate : candidate_preview_engine_.lookup(candidate_preview_pinyin_))
            candidate_preview_texts_.push_back(candidate.text);
    }
    // Look up only when the edit changes. Font and theme repaints reuse this
    // result and never write learning records or commit text to an application.
    invalidate_candidate_size_preview();
}

void SettingsWindow::sync_candidate_text_size_controls() {
    if (candidate_size_value_) {
        SetWindowTextW(candidate_size_value_, candidate_text_size_display_text(options_.text_size).c_str());
    }
    if (candidate_size_slider_) {
        InvalidateRect(candidate_size_slider_, nullptr, FALSE);
    }
}

void SettingsWindow::persist_candidate_display_options() {
    store_.save_options(options_, input_mode_options_);
    notify_candidate_options_changed();
}

void SettingsWindow::update_candidate_text_size_from_level(int level) {
    const CandidateTextSize text_size = candidate_text_size_from_level(level);
    if (text_size == options_.text_size) {
        return;
    }
    options_.text_size = text_size;
    sync_candidate_text_size_controls();
    persist_candidate_display_options();
    invalidate_candidate_size_preview();
}

void SettingsWindow::invalidate_candidate_size_preview() {
    if (!hwnd_) {
        return;
    }
    RECT client{};
    GetClientRect(hwnd_, &client);
    const SettingsWindowLayout layout = calculate_settings_window_layout(client.right - client.left,
                                                                         client.bottom - client.top,
                                                                         dpi_);
    RECT preview = to_win_rect(layout.appearance_preview_card);
    InvalidateRect(hwnd_, &preview, FALSE);
    RECT value = to_win_rect(layout.candidate_size_value);
    InvalidateRect(hwnd_, &value, FALSE);
    if (candidate_size_slider_) {
        InvalidateRect(candidate_size_slider_, nullptr, FALSE);
    }
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

void SettingsWindow::load_dictionary_catalog() {
    const std::filesystem::path executable(module_path(instance_));
    dictionary_catalog_.load((executable.parent_path() / L"dictionary").wstring());
}

void SettingsWindow::open_dictionary_preview() {
    if (dictionary_preview_hwnd_ && IsWindow(dictionary_preview_hwnd_)) {
        ShowWindow(dictionary_preview_hwnd_, SW_RESTORE);
        SetForegroundWindow(dictionary_preview_hwnd_);
        return;
    }
    dictionary_preview_hwnd_ =
        DictionaryPreviewWindow::show(hwnd_, instance_, dictionary_catalog_, palette_.dark);
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
    InvalidateRect(lexicon_list_, nullptr, FALSE);
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
        InvalidateRect(lexicon_list_, nullptr, FALSE);
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
            InvalidateRect(lexicon_list_, nullptr, FALSE);
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
    RECT client{};
    GetClientRect(hwnd_, &client);
    const auto layout = calculate_settings_window_layout(client.right - client.left,
                                                         client.bottom - client.top,
                                                         dpi_);
    RECT dirty = to_win_rect(layout.user_editor_card);
    InvalidateRect(hwnd_, &dirty, FALSE);
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
    RECT client{};
    GetClientRect(hwnd_, &client);
    const auto layout = calculate_settings_window_layout(client.right - client.left,
                                                         client.bottom - client.top,
                                                         dpi_);
    RECT dirty = to_win_rect(layout.user_editor_card);
    InvalidateRect(hwnd_, &dirty, FALSE);
}

void SettingsWindow::refresh_user_lexicon() {
    load_user_lexicon();
}

}  // namespace localpinyin
