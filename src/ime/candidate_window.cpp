#include "candidate_window.h"

#include "globals.h"
#include "../common/logging.h"
#include "../settings/settings_store.h"

#include <algorithm>
#include <cwchar>
#include <sstream>
#include <string>
#include <utility>

namespace localpinyin {
namespace {

constexpr wchar_t kWindowClassName[] = L"LocalPinyinCandidateWindow";
constexpr wchar_t kSettingsWindowClassName[] = L"LocalPinyinSettingsWindow";
constexpr wchar_t kSettingsExecutableName[] = L"LocalPinyinSettings.exe";
constexpr wchar_t kSettingsButtonText[] = L"\u2699";
constexpr wchar_t kHintText[] = L"Space \u9009\u9996\u9879 \u00B7 1-9 \u9009\u8BCD \u00B7 Esc \u53D6\u6D88";

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

RECT monitor_work_area_for(const RECT& rect) {
    HMONITOR monitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    if (monitor && GetMonitorInfoW(monitor, &info)) {
        return info.rcWork;
    }

    RECT work{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    return work;
}

std::wstring placement_text(CandidatePlacement placement) {
    switch (placement) {
        case CandidatePlacement::Above:
            return L"above";
        case CandidatePlacement::Below:
            return L"below";
        case CandidatePlacement::Hidden:
        default:
            return L"hidden";
    }
}

std::wstring theme_text(const CandidateThemePalette& palette) {
    return palette.dark ? L"dark" : L"light";
}

const wchar_t* bool_text(bool value) noexcept {
    return value ? L"true" : L"false";
}

void fill_round_rect(HDC hdc, const RECT& rect, int radius, HBRUSH brush, HPEN pen) {
    HGDIOBJ old_brush = SelectObject(hdc, brush);
    HGDIOBJ old_pen = SelectObject(hdc, pen);
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(hdc, old_pen);
    SelectObject(hdc, old_brush);
}

COLORREF blend_color(COLORREF from, COLORREF to, int percent_to) {
    const int keep = 100 - percent_to;
    return RGB((GetRValue(from) * keep + GetRValue(to) * percent_to) / 100,
               (GetGValue(from) * keep + GetGValue(to) * percent_to) / 100,
               (GetBValue(from) * keep + GetBValue(to) * percent_to) / 100);
}

HFONT create_candidate_font(int height_px, LONG weight) {
    return CreateFontW(-height_px, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Variable");
}

int measure_text_width_px(HWND hwnd, HFONT font, const std::wstring& text, int fallback_font_px) {
    HDC hdc = GetDC(hwnd);
    if (!hdc) {
        return estimate_candidate_text_width_px(text, fallback_font_px);
    }

    HGDIOBJ old_font = nullptr;
    if (font) {
        old_font = SelectObject(hdc, font);
    }

    RECT rect{0, 0, 0, 0};
    const int measured_height = DrawTextW(hdc,
                                          text.c_str(),
                                          static_cast<int>(text.size()),
                                          &rect,
                                          DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
    const int width = measured_height ? rect.right - rect.left : 0;

    if (old_font) {
        SelectObject(hdc, old_font);
    }
    ReleaseDC(hwnd, hdc);

    if (width > 0) {
        return width;
    }
    return estimate_candidate_text_width_px(text, fallback_font_px);
}

std::wstring settings_executable_path_from_module() {
    wchar_t module_path[MAX_PATH]{};
    constexpr DWORD buffer_size = static_cast<DWORD>(sizeof(module_path) / sizeof(module_path[0]));
    const DWORD size = GetModuleFileNameW(g_instance, module_path, buffer_size);
    if (size == 0 || size >= buffer_size) {
        return {};
    }

    std::wstring path(module_path, size);
    const size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return kSettingsExecutableName;
    }
    path.resize(slash + 1);
    path += kSettingsExecutableName;
    return path;
}

std::wstring parent_directory_of(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return {};
    }
    return path.substr(0, slash);
}

}  // namespace

CandidateWindow::CandidateWindow() = default;

CandidateWindow::~CandidateWindow() {
    release_fonts();
    if (hwnd_) {
        DestroyWindow(hwnd_);
    }
}

bool CandidateWindow::create() {
    if (hwnd_) {
        return true;
    }

    const CandidateWindowCreateConfig create_config = make_candidate_window_create_config(nullptr, false);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_DROPSHADOW;
    wc.lpfnWndProc = CandidateWindow::window_proc;
    wc.hInstance = g_instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kWindowClassName;
    const ATOM atom = RegisterClassExW(&wc);
    const DWORD register_error = atom ? ERROR_SUCCESS : GetLastError();
    if (!atom && register_error != ERROR_CLASS_ALREADY_EXISTS) {
        std::wstringstream stream;
        stream << L"window_class_registered=false last_error=" << register_error;
        log_status(L"candidate-ui", stream.str());
        return false;
    }

    log_status(L"candidate-ui", make_candidate_window_create_attempt_log(create_config));

    SetLastError(ERROR_SUCCESS);
    hwnd_ = CreateWindowExW(create_config.ex_style,
                            wc.lpszClassName, L"", create_config.style,
                            create_config.x, create_config.y, create_config.width, create_config.height,
                            create_config.parent, create_config.menu, g_instance, this);
    const DWORD create_error = hwnd_ ? ERROR_SUCCESS : GetLastError();
    if (hwnd_) {
        SetLayeredWindowAttributes(hwnd_, 0, 248, LWA_ALPHA);
    }
    log_status(L"candidate-ui", make_candidate_window_create_result_log(hwnd_ != nullptr, create_error));
    return hwnd_ != nullptr;
}

void CandidateWindow::show(const std::wstring& composition_text,
                           const std::vector<Candidate>& candidates,
                           size_t selected_index,
                           const RECT& caret_rect) {
    std::wstringstream request_stream;
    request_stream << L"request_show=true composition_active=" << bool_text(!composition_text.empty())
                   << L" candidate_count=" << candidates.size();
    log_status(L"candidate-ui", request_stream.str());

    if (composition_text.empty()) {
        hide(L"empty_composition");
        return;
    }
    if (candidates.empty()) {
        hide(L"empty_candidates");
        return;
    }
    if (!create()) {
        hide(L"create_failed");
        return;
    }

    std::vector<std::wstring> candidate_texts;
    candidate_texts.reserve(std::min(candidates.size(), kMaxCandidateCount));
    for (const auto& candidate : candidates) {
        candidate_texts.push_back(candidate.text);
    }

    reload_options();
    const int dpi = GetDpiForWindow(hwnd_);
    update_fonts(dpi);
    const int fallback_font_px = candidate_font_px_for_size(options_.text_size, dpi);
    std::vector<int> candidate_text_widths;
    std::vector<int> number_text_widths;
    candidate_text_widths.reserve(candidate_texts.size());
    number_text_widths.reserve(candidate_texts.size());
    for (size_t i = 0; i < candidate_texts.size(); ++i) {
        candidate_text_widths.push_back(measure_text_width_px(hwnd_, candidate_font_, candidate_texts[i], fallback_font_px));
        const std::wstring number = std::to_wstring(i + 1);
        number_text_widths.push_back(measure_text_width_px(hwnd_, candidate_font_, number, fallback_font_px));
    }

    CandidateLayoutInput input;
    input.composition_text = composition_text;
    input.candidate_texts = std::move(candidate_texts);
    input.candidate_text_widths_px = std::move(candidate_text_widths);
    input.number_text_widths_px = std::move(number_text_widths);
    input.selected_index = selected_index;
    input.caret_rect = caret_rect;
    input.work_area = monitor_work_area_for(caret_rect);
    input.dpi = dpi;
    input.options = options_;
    layout_ = make_candidate_layout(input);
    if (!layout_.visible) {
        hide(L"layout_hidden");
        return;
    }

    composition_text_ = composition_text;
    update_fonts(dpi);

    const int width = layout_.window_rect.right - layout_.window_rect.left;
    const int height = layout_.window_rect.bottom - layout_.window_rect.top;
    HRGN region = CreateRoundRectRgn(0, 0, width + 1, height + 1,
                                     layout_.corner_radius * 2, layout_.corner_radius * 2);
    if (region) {
        if (!SetWindowRgn(hwnd_, region, FALSE)) {
            DeleteObject(region);
        }
    }

    SetLastError(ERROR_SUCCESS);
    const BOOL set_window_pos_ok = SetWindowPos(hwnd_, HWND_TOPMOST,
                                                layout_.window_rect.left,
                                                layout_.window_rect.top,
                                                width,
                                                height,
                                                SWP_NOACTIVATE | SWP_SHOWWINDOW);
    const DWORD set_window_pos_error = set_window_pos_ok ? ERROR_SUCCESS : GetLastError();
    if (!set_window_pos_ok) {
        log_show_event(false, set_window_pos_error);
        hide(L"set_window_pos_failed");
        return;
    }

    InvalidateRect(hwnd_, nullptr, FALSE);
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    UpdateWindow(hwnd_);
    log_show_event(true, ERROR_SUCCESS);
}

void CandidateWindow::hide(const wchar_t* reason) {
    if (hwnd_) {
        settings_button_hot_ = false;
        settings_button_pressed_ = false;
        tracking_mouse_leave_ = false;
        ShowWindow(hwnd_, SW_HIDE);
        std::wstringstream stream;
        stream << L"hidden=true reason=" << (reason ? reason : L"unspecified");
        log_status(L"candidate-ui", stream.str());
    }
}

bool CandidateWindow::is_visible() const noexcept {
    return hwnd_ && IsWindowVisible(hwnd_);
}

LRESULT CALLBACK CandidateWindow::window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* self = reinterpret_cast<CandidateWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
        self = static_cast<CandidateWindow*>(create->lpCreateParams);
        if (self) {
            self->hwnd_ = hwnd;
        }
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    return self ? self->handle_message(hwnd, message, wparam, lparam) : DefWindowProcW(hwnd, message, wparam, lparam);
}

LRESULT CandidateWindow::handle_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case WM_MOUSEMOVE: {
            const POINTS points = MAKEPOINTS(lparam);
            const POINT point{points.x, points.y};
            update_settings_button_hot(hit_test_settings_button(point));
            track_mouse_leave();
            return 0;
        }
        case WM_MOUSELEAVE:
            tracking_mouse_leave_ = false;
            update_settings_button_hot(false);
            if (settings_button_pressed_) {
                settings_button_pressed_ = false;
                InvalidateRect(hwnd_, &layout_.action_button_rect, FALSE);
            }
            return 0;
        case WM_LBUTTONDOWN: {
            const POINTS points = MAKEPOINTS(lparam);
            const POINT point{points.x, points.y};
            if (hit_test_settings_button(point)) {
                settings_button_pressed_ = true;
                SetCapture(hwnd_);
                InvalidateRect(hwnd_, &layout_.action_button_rect, FALSE);
                return 0;
            }
            break;
        }
        case WM_LBUTTONUP: {
            const POINTS points = MAKEPOINTS(lparam);
            const POINT point{points.x, points.y};
            const bool activate_settings = settings_button_pressed_ && hit_test_settings_button(point);
            if (settings_button_pressed_) {
                settings_button_pressed_ = false;
                if (GetCapture() == hwnd_) {
                    ReleaseCapture();
                }
                InvalidateRect(hwnd_, &layout_.action_button_rect, FALSE);
            }
            if (activate_settings) {
                open_settings_window();
                return 0;
            }
            break;
        }
        case WM_SETCURSOR:
            if (settings_button_hot_) {
                SetCursor(LoadCursorW(nullptr, IDC_HAND));
                return TRUE;
            }
            break;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT:
            paint();
            return 0;
        default:
            return DefWindowProcW(hwnd, message, wparam, lparam);
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

void CandidateWindow::reload_options() {
    SettingsStore store;
    options_ = store.load_candidate_options();
    palette_ = make_candidate_palette(options_.theme_mode, windows_apps_use_dark_theme());
}

void CandidateWindow::update_fonts(int dpi) {
    if (composition_font_ && candidate_font_ && hint_font_ && font_dpi_ == dpi && font_text_size_ == options_.text_size) {
        return;
    }
    release_fonts();
    font_dpi_ = dpi;
    font_text_size_ = options_.text_size;

    composition_font_ = create_candidate_font(scale_dip(15, dpi), FW_NORMAL);
    candidate_font_ = create_candidate_font(candidate_font_px_for_size(options_.text_size, dpi), FW_MEDIUM);
    hint_font_ = create_candidate_font(scale_dip(13, dpi), FW_NORMAL);
}

bool CandidateWindow::hit_test_settings_button(POINT point) const noexcept {
    const RECT& rect = layout_.action_button_rect;
    return layout_.action_button_visible &&
           point.x >= rect.left && point.x < rect.right &&
           point.y >= rect.top && point.y < rect.bottom;
}

void CandidateWindow::update_settings_button_hot(bool hot) {
    if (settings_button_hot_ == hot) {
        return;
    }
    settings_button_hot_ = hot;
    InvalidateRect(hwnd_, &layout_.action_button_rect, FALSE);
}

void CandidateWindow::track_mouse_leave() {
    if (tracking_mouse_leave_ || !hwnd_) {
        return;
    }
    TRACKMOUSEEVENT event{};
    event.cbSize = sizeof(event);
    event.dwFlags = TME_LEAVE;
    event.hwndTrack = hwnd_;
    if (TrackMouseEvent(&event)) {
        tracking_mouse_leave_ = true;
    }
}

void CandidateWindow::open_settings_window() {
    HWND existing = FindWindowW(kSettingsWindowClassName, nullptr);
    if (existing) {
        ShowWindow(existing, SW_SHOWNORMAL);
        SetForegroundWindow(existing);
        return;
    }

    const std::wstring executable_path = settings_executable_path_from_module();
    if (executable_path.empty() || GetFileAttributesW(executable_path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        log_status(L"candidate-ui", L"settings_open=false reason=settings_exe_not_found");
        return;
    }

    std::wstring command_line = L"\"" + executable_path + L"\"";
    const std::wstring working_directory = parent_directory_of(executable_path);
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    const BOOL created = CreateProcessW(executable_path.c_str(),
                                        command_line.data(),
                                        nullptr,
                                        nullptr,
                                        FALSE,
                                        0,
                                        nullptr,
                                        working_directory.empty() ? nullptr : working_directory.c_str(),
                                        &startup,
                                        &process);
    if (!created) {
        std::wstringstream stream;
        stream << L"settings_open=false create_process_last_error=" << GetLastError();
        log_status(L"candidate-ui", stream.str());
        return;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
}

void CandidateWindow::draw_settings_button(HDC hdc) {
    if (!layout_.action_button_visible) {
        return;
    }

    const RECT rect = layout_.action_button_rect;
    COLORREF fill = palette_.dark ? RGB(0x2E, 0x31, 0x38) : RGB(0xF4, 0xF7, 0xFB);
    if (settings_button_hot_) {
        fill = blend_color(fill, palette_.selected_background, palette_.dark ? 28 : 16);
    }
    if (settings_button_pressed_) {
        fill = blend_color(fill, RGB(0, 0, 0), palette_.dark ? 18 : 8);
    }
    const COLORREF border_color = settings_button_hot_ ? palette_.selected_border : palette_.border;
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, std::max(1, scale_dip(1, layout_.dpi)), border_color);
    fill_round_rect(hdc, rect, scale_dip(8, layout_.dpi) * 2, brush, pen);
    DeleteObject(pen);
    DeleteObject(brush);

    HGDIOBJ old_font = nullptr;
    if (candidate_font_) {
        old_font = SelectObject(hdc, candidate_font_);
    }
    RECT text_rect = rect;
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, settings_button_hot_ ? palette_.number_text : palette_.muted_text);
    DrawTextW(hdc,
              kSettingsButtonText,
              static_cast<int>(wcslen(kSettingsButtonText)),
              &text_rect,
              DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
    if (old_font) {
        SelectObject(hdc, old_font);
    }
}

void CandidateWindow::paint() {
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hwnd_, &ps);
    RECT rect{};
    GetClientRect(hwnd_, &rect);

    HDC memory_dc = CreateCompatibleDC(hdc);
    HBITMAP bitmap = CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top);
    HGDIOBJ old_bitmap = SelectObject(memory_dc, bitmap);
    SetBkMode(memory_dc, TRANSPARENT);

    HBRUSH background = CreateSolidBrush(palette_.background);
    HPEN border = CreatePen(PS_SOLID, std::max(1, scale_dip(1, layout_.dpi)), palette_.border);
    fill_round_rect(memory_dc, rect, layout_.corner_radius * 2, background, border);

    if (composition_font_) {
        SelectObject(memory_dc, composition_font_);
    }
    SetTextColor(memory_dc, palette_.muted_text);
    if (!composition_text_.empty() && layout_.composition_rect.bottom > layout_.composition_rect.top) {
        DrawTextW(memory_dc, composition_text_.c_str(), static_cast<int>(composition_text_.size()),
                  &layout_.composition_rect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

    if (candidate_font_) {
        SelectObject(memory_dc, candidate_font_);
    }
    for (const auto& item : layout_.items) {
        if (item.selected) {
            HBRUSH selected_brush = CreateSolidBrush(palette_.selected_background);
            HPEN selected_pen = CreatePen(PS_SOLID, std::max(1, scale_dip(1, layout_.dpi)), palette_.selected_border);
            fill_round_rect(memory_dc, item.bounds, layout_.selected_corner_radius * 2, selected_brush, selected_pen);
            DeleteObject(selected_pen);
            DeleteObject(selected_brush);
        }

        const std::wstring number = std::to_wstring(item.candidate_index + 1);
        RECT number_rect = item.number_bounds;
        SetTextColor(memory_dc, palette_.number_text);
        DrawTextW(memory_dc, number.c_str(), static_cast<int>(number.size()), &number_rect,
                  DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);

        RECT text_rect = item.text_bounds;
        SetTextColor(memory_dc, palette_.text);
        DrawTextW(memory_dc, item.display_text.c_str(), static_cast<int>(item.display_text.size()), &text_rect,
                  candidate_text_draw_flags(item.truncated));
    }

    draw_settings_button(memory_dc);

    if (layout_.show_hint) {
        if (hint_font_) {
            SelectObject(memory_dc, hint_font_);
        }
        RECT hint_rect = layout_.hint_rect;
        SetTextColor(memory_dc, palette_.hint_text);
        DrawTextW(memory_dc, kHintText, static_cast<int>(wcslen(kHintText)), &hint_rect,
                  DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

    BitBlt(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top, memory_dc, 0, 0, SRCCOPY);
    SelectObject(memory_dc, old_bitmap);
    DeleteObject(border);
    DeleteObject(background);
    DeleteObject(bitmap);
    DeleteDC(memory_dc);

    EndPaint(hwnd_, &ps);
}

void CandidateWindow::log_show_event(bool set_window_pos_ok, DWORD set_window_pos_error) const {
    const CandidateWindowShowStatus show_status = make_candidate_window_show_status(set_window_pos_ok);
    std::wstringstream stream;
    stream << L"theme=" << theme_text(palette_)
           << L" dpi=" << layout_.dpi
           << L" candidate_count=" << layout_.items.size()
           << L" placement=" << placement_text(layout_.placement)
           << L" rect_clamped=" << bool_text(layout_.rect_clamped)
           << L" set_window_pos_ok=" << bool_text(set_window_pos_ok)
           << L" set_window_pos_last_error=" << set_window_pos_error
           << L" show_window_called=" << bool_text(show_status.show_window_called)
           << L" shown=" << bool_text(show_status.shown);
    log_status(L"candidate-ui", stream.str());
}

void CandidateWindow::release_fonts() {
    if (composition_font_) {
        DeleteObject(composition_font_);
        composition_font_ = nullptr;
    }
    if (candidate_font_) {
        DeleteObject(candidate_font_);
        candidate_font_ = nullptr;
    }
    if (hint_font_) {
        DeleteObject(hint_font_);
        hint_font_ = nullptr;
    }
}

}  // namespace localpinyin
