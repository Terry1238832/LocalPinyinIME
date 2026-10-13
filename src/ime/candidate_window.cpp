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
constexpr wchar_t kHintText[] = L"\u7A7A\u683C \u786E\u8BA4  \u00B7  \u6570\u5B57 \u9009\u8BCD  \u00B7  Esc \u53D6\u6D88";
constexpr wchar_t kPredictionHintText[] = L"Tab 续写  ·  点击选词  ·  Esc 关闭";
constexpr wchar_t kCandidateOptionsChangedMessageName[] = L"LocalPinyinIME.CandidateOptionsChanged";
constexpr UINT_PTR kLayoutRetryTimerId = 1;
constexpr UINT kDwmAttributeCloaked = 14;
constexpr DWORD kDwmCloakedShell = 0x00000002;

using DwmGetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, PVOID, DWORD);

struct DwmSnapshot {
    HRESULT cloaked_hr = E_NOTIMPL;
    DWORD cloaked = 0;
};

bool rect_valid(const RECT& rect) noexcept {
    return rect.right > rect.left && rect.bottom > rect.top;
}

DwmGetWindowAttributeFn dwm_get_window_attribute() {
    static const HMODULE module = LoadLibraryW(L"dwmapi.dll");
    if (!module) {
        return nullptr;
    }
    return reinterpret_cast<DwmGetWindowAttributeFn>(GetProcAddress(module, "DwmGetWindowAttribute"));
}

DwmSnapshot query_dwm_snapshot(HWND hwnd) {
    DwmSnapshot snapshot;
    const auto function = dwm_get_window_attribute();
    if (!function || !hwnd) {
        return snapshot;
    }
    snapshot.cloaked_hr = function(hwnd, kDwmAttributeCloaked, &snapshot.cloaked, sizeof(snapshot.cloaked));
    return snapshot;
}

UINT candidate_options_changed_message() {
    static const UINT message = RegisterWindowMessageW(kCandidateOptionsChangedMessageName);
    return message;
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
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
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

void notify_candidate_ime_win_event(DWORD event,
                                    HWND hwnd,
                                    LONG object_id,
                                    LONG child_id,
                                    void*) noexcept {
    NotifyWinEvent(event, hwnd, object_id, child_id);
}

}  // namespace

CandidateWindow::CandidateWindow()
    : ime_win_event_state_(notify_candidate_ime_win_event, nullptr) {}

CandidateWindow::~CandidateWindow() {
    cancel_layout_retry();
    release_fonts();
    if (hwnd_) {
        notify_ime_window_hidden(hwnd_);
        DestroyWindow(hwnd_);
    }
}

bool CandidateWindow::create() {
    return create_with_owner(nullptr);
}

bool CandidateWindow::create_with_owner(HWND requested_owner) {
    if (hwnd_) {
        return true;
    }
    return create_replacement_hwnd(requested_owner, nullptr) != nullptr;
}

HWND CandidateWindow::create_replacement_hwnd(HWND requested_owner, HWND original_hwnd) {
    const bool requested_owner_is_window = requested_owner && IsWindow(requested_owner);
    const CandidateWindowCreateConfig create_config =
        make_candidate_window_create_config(requested_owner, requested_owner_is_window);

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
        return nullptr;
    }

    SetLastError(ERROR_SUCCESS);
    HWND created_hwnd = CreateWindowExW(create_config.ex_style,
                                        wc.lpszClassName, L"", create_config.style,
                                        create_config.x, create_config.y, create_config.width, create_config.height,
                                        create_config.parent, create_config.menu, g_instance, this);
    const DWORD create_error = created_hwnd ? ERROR_SUCCESS : GetLastError();
    if (!created_hwnd && original_hwnd) {
        hwnd_ = original_hwnd;
    }
    if (created_hwnd) {
        hwnd_ = created_hwnd;
        SetLastError(ERROR_SUCCESS);
        const BOOL layered_ok = SetLayeredWindowAttributes(created_hwnd, 0, 248, LWA_ALPHA);
        if (!layered_ok) {
            log_status(L"candidate-ui",
                       L"SetLayeredWindowAttributes failed last_error=" +
                           std::to_wstring(GetLastError()));
        }
    } else {
        log_status(L"candidate-ui",
                   L"CreateWindowEx failed last_error=" + std::to_wstring(create_error));
    }
    return created_hwnd;
}

void CandidateWindow::destroy_hwnd_for_recreate() {
    if (!hwnd_) {
        return;
    }
    cancel_pointer_interaction();
    const HWND old_hwnd = hwnd_;
    notify_ime_window_hidden(old_hwnd);
    DestroyWindow(old_hwnd);
    if (hwnd_ == old_hwnd) {
        hwnd_ = nullptr;
    }
    release_fonts();
    font_dpi_ = 0;
}

void CandidateWindow::reset_window_lifecycle_state() noexcept {
    paint_seen_ = false;
}

bool CandidateWindow::apply_window_region(int width, int height) {
    HRGN region = CreateRoundRectRgn(0, 0, width + 1, height + 1,
                                     layout_.corner_radius * 2, layout_.corner_radius * 2);
    if (!region) {
        return false;
    }
    if (!SetWindowRgn(hwnd_, region, FALSE)) {
        DeleteObject(region);
        return false;
    }
    return true;
}

bool CandidateWindow::position_and_show_current_layout() {
    const int width = layout_.window_rect.right - layout_.window_rect.left;
    const int height = layout_.window_rect.bottom - layout_.window_rect.top;
    apply_window_region(width, height);

    SetLastError(ERROR_SUCCESS);
    const BOOL set_window_pos_ok = SetWindowPos(hwnd_, HWND_TOPMOST,
                                                layout_.window_rect.left,
                                                layout_.window_rect.top,
                                                width,
                                                height,
                                                SWP_NOACTIVATE | SWP_SHOWWINDOW);
    const DWORD set_window_pos_error = set_window_pos_ok ? ERROR_SUCCESS : GetLastError();
    if (!set_window_pos_ok) {
        log_status(L"candidate-ui",
                   L"SetWindowPos failed last_error=" + std::to_wstring(set_window_pos_error));
        return false;
    }

    InvalidateRect(hwnd_, nullptr, FALSE);
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    UpdateWindow(hwnd_);
    return true;
}

void CandidateWindow::show(const std::wstring& composition_text,
                           const std::vector<Candidate>& candidates,
                           size_t selected_index,
                           const RECT& caret_rect,
                           const CandidateWindowOwnerInfo& owner_info,
                           const std::vector<UINT>& page_indices) {
    const bool continuing_session = has_cached_show_state_;
    cancel_pointer_interaction();
    reset_presentation_state();
    if (!continuing_session) {
        shell_cloak_recovery_attempts_ = 0;
    }
    show_requested_ = true;
    owner_info_ = owner_info;

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
    cancel_layout_retry();

    if (!prepare_layout(composition_text, candidates, selected_index, caret_rect, owner_info, page_indices).visible) {
        hide(L"layout_hidden");
        return;
    }
    has_cached_show_state_ = true;

    if (!position_and_show_current_layout()) {
        hide(L"set_window_pos_failed");
        return;
    }

    notify_ime_window_shown(hwnd_);

    maybe_recover_from_shell_cloak();
}

CandidateLayoutResult CandidateWindow::prepare_layout(const std::wstring& composition_text,
    const std::vector<Candidate>& candidates, size_t selected_index, const RECT& caret_rect,
    const CandidateWindowOwnerInfo& owner_info, const std::vector<UINT>& page_indices) {
    candidate_texts_.clear();
    for (const auto& candidate : candidates) candidate_texts_.push_back(candidate.text);
    composition_text_ = composition_text;
    selected_index_ = selected_index;
    page_indices_ = page_indices;
    caret_rect_ = caret_rect;
    owner_info_ = owner_info;
    reload_options();
    const HWND dpi_window = owner_info.context_valid ? owner_info.context_hwnd : nullptr;
    int dpi = dpi_window ? static_cast<int>(GetDpiForWindow(dpi_window)) : static_cast<int>(GetDpiForSystem());
    // A hidden candidate HWND can remain on the previous monitor. Measure using
    // the target monitor so host-managed UI and first presentation agree.
    using MonitorDpiFn = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);
    static const HMODULE shcore = LoadLibraryW(L"shcore.dll");
    static const auto monitor_dpi = shcore ? reinterpret_cast<MonitorDpiFn>(GetProcAddress(shcore, "GetDpiForMonitor")) : nullptr;
    UINT dpi_x = 0, dpi_y = 0;
    if (monitor_dpi && SUCCEEDED(monitor_dpi(MonitorFromRect(&caret_rect_, MONITOR_DEFAULTTONEAREST), 0, &dpi_x, &dpi_y)))
        dpi = static_cast<int>(dpi_x);
    rebuild_layout((std::max)(1, dpi));
    return layout_;
}

void CandidateWindow::hide(const wchar_t*) {
    cancel_pointer_interaction();
    if (hwnd_) {
        ShowWindow(hwnd_, SW_HIDE);
        notify_ime_window_hidden(hwnd_);
    }
    has_cached_show_state_ = false;
    shell_cloak_recovery_attempts_ = 0;
}

bool CandidateWindow::schedule_layout_retry(std::function<void()> callback,
                                            unsigned int attempt,
                                            UINT delay_ms) {
    if (!callback || !create()) {
        return false;
    }
    cancel_layout_retry();
    layout_retry_callback_ = std::move(callback);
    layout_retry_attempt_ = attempt;
    layout_retry_timer_ = SetTimer(hwnd_, kLayoutRetryTimerId, delay_ms, nullptr);
    if (!layout_retry_timer_) {
        log_status(L"candidate-ui", L"candidate layout retry SetTimer failed");
        layout_retry_callback_ = {};
        layout_retry_attempt_ = 0;
        return false;
    }
    return true;
}

void CandidateWindow::cancel_layout_retry() {
    if (hwnd_ && layout_retry_timer_) {
        KillTimer(hwnd_, layout_retry_timer_);
    }
    layout_retry_timer_ = 0;
    layout_retry_attempt_ = 0;
    layout_retry_callback_ = {};
}

bool CandidateWindow::is_visible() const noexcept {
    return hwnd_ && IsWindowVisible(hwnd_);
}

void CandidateWindow::set_options_changed_callback(std::function<void()> callback) {
    create();
    options_changed_callback_ = std::move(callback);
}

void CandidateWindow::set_selection_callback(std::function<void(size_t)> callback) {
    cancel_pointer_interaction();
    selection_callback_ = std::move(callback);
    if (hwnd_) InvalidateRect(hwnd_, nullptr, FALSE);
}

void CandidateWindow::reset_presentation_state() noexcept {
    show_requested_ = false;
    paint_seen_ = false;
}

void CandidateWindow::maybe_recover_from_shell_cloak() {
    const HWND original_hwnd = hwnd_;
    const bool candidate_valid = original_hwnd && IsWindow(original_hwnd);
    const bool candidate_visible = candidate_valid && IsWindowVisible(original_hwnd);
    DWORD candidate_process_id = 0;
    if (candidate_valid) {
        GetWindowThreadProcessId(original_hwnd, &candidate_process_id);
    }
    DWORD owner_root_process_id = 0;
    const bool owner_root_valid = owner_info_.root_valid && owner_info_.root_hwnd && IsWindow(owner_info_.root_hwnd);
    if (owner_root_valid) {
        GetWindowThreadProcessId(owner_info_.root_hwnd, &owner_root_process_id);
    }
    const DwmSnapshot candidate_dwm = query_dwm_snapshot(original_hwnd);
    const DwmSnapshot owner_dwm = query_dwm_snapshot(owner_info_.root_hwnd);

    CandidateShellCloakRecoveryInput input;
    input.candidate_model_valid = has_cached_show_state_ && !composition_text_.empty() && !candidate_texts_.empty();
    input.candidate_show_requested = show_requested_;
    input.candidate_hwnd_valid = candidate_valid;
    input.candidate_hwnd_visible = candidate_visible;
    input.current_process_id = GetCurrentProcessId();
    input.candidate_process_id = candidate_process_id;
    input.candidate_cloak_hr = candidate_dwm.cloaked_hr;
    input.candidate_cloak_value = candidate_dwm.cloaked;
    input.owner_root_valid = owner_root_valid;
    input.owner_root_process_id = owner_root_process_id;
    input.owner_root_cloak_hr = owner_dwm.cloaked_hr;
    input.owner_root_cloak_value = owner_dwm.cloaked;
    input.recovery_attempts = shell_cloak_recovery_attempts_;

    const CandidateShellCloakRecoveryDecision decision = decide_candidate_shell_cloak_recovery(input);
    if (!decision.recovery_eligible) {
        return;
    }

    ++shell_cloak_recovery_attempts_;
    log_status(L"candidate-ui",
               L"candidate_shell_cloak_recovery_attempted=true strategy=" +
                   candidate_shell_cloak_recovery_strategy_to_string(decision.strategy));

    destroy_hwnd_for_recreate();
    reset_window_lifecycle_state();

    bool recreated = create_with_owner(owner_info_.root_hwnd);
    bool position_ok = false;
    if (recreated) {
        const int dpi = GetDpiForWindow(hwnd_);
        recreated = rebuild_layout(dpi);
    }
    if (recreated) {
        position_ok = position_and_show_current_layout();
    }

    const DwmSnapshot after_dwm = query_dwm_snapshot(hwnd_);
    RECT window_rect{};
    const bool window_rect_ok = hwnd_ && GetWindowRect(hwnd_, &window_rect);
    const bool rect_on_monitor =
        window_rect_ok && rect_valid(window_rect) && MonitorFromRect(&window_rect, MONITOR_DEFAULTTONULL) != nullptr;
    const bool after_shell_cloaked = SUCCEEDED(after_dwm.cloaked_hr) &&
                                     (after_dwm.cloaked & kCandidateDwmCloakedShell) != 0;
    const bool success = recreated && position_ok && hwnd_ && IsWindow(hwnd_) &&
                          IsWindowVisible(hwnd_) && rect_on_monitor &&
                          paint_seen_ && !after_shell_cloaked;
    if (success) {
        notify_ime_window_shown(hwnd_);
    }

    if (success) {
        log_status(L"candidate-ui", L"candidate_shell_cloak_recovery_succeeded=true");
        return;
    }

    const wchar_t* reason = L"unknown";
    if (!recreated) {
        reason = L"recreate_failed";
    } else if (!position_ok) {
        reason = L"recreate_show_failed";
    } else if (after_shell_cloaked) {
        reason = L"still_shell_cloaked";
    } else if (!paint_seen_) {
        reason = L"paint_not_seen";
    } else if (!rect_on_monitor) {
        reason = L"rect_not_on_monitor";
    }
    log_status(L"candidate-ui",
               L"candidate_shell_cloak_recovery_failed=true reason=" + std::wstring(reason));
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
    if (message == candidate_options_changed_message()) {
        apply_options_changed();
        return 0;
    }
    switch (message) {
        case WM_TIMER:
            if (wparam == kLayoutRetryTimerId && layout_retry_timer_) {
                KillTimer(hwnd_, layout_retry_timer_);
                layout_retry_timer_ = 0;
                layout_retry_attempt_ = 0;
                auto callback = std::move(layout_retry_callback_);
                layout_retry_callback_ = {};
                if (callback) {
                    callback();
                }
                return 0;
            }
            break;
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case WM_MOUSEMOVE: {
            const POINTS points = MAKEPOINTS(lparam);
            const POINT point{points.x, points.y};
            update_settings_button_hot(hit_test_settings_button(point));
            update_candidate_hot(point);
            track_mouse_leave();
            return 0;
        }
        case WM_MOUSELEAVE:
            tracking_mouse_leave_ = false;
            update_settings_button_hot(false);
            cancel_pointer_interaction();
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        case WM_LBUTTONDOWN: {
            const POINTS points = MAKEPOINTS(lparam);
            const POINT point{points.x, points.y};
            const int page_direction = page_direction_at_point(point);
            if (page_direction && page_callback_) {
                candidate_pointer_.cancel();
                pressed_page_direction_ = page_direction;
                SetCapture(hwnd_);
                return 0;
            }
            if (hit_test_settings_button(point)) {
                candidate_pointer_.cancel();
                settings_button_pressed_ = true;
                SetCapture(hwnd_);
                InvalidateRect(hwnd_, &layout_.action_button_rect, FALSE);
                return 0;
            }
            if (selection_callback_) {
                const auto index = candidate_index_at_point(layout_, point);
                if (index) {
                    candidate_pointer_.press(index);
                    SetCapture(hwnd_);
                    track_mouse_leave();
                    InvalidateRect(hwnd_, nullptr, FALSE);
                    return 0;
                }
            }
            break;
        }
        case WM_LBUTTONUP: {
            const POINTS points = MAKEPOINTS(lparam);
            const POINT point{points.x, points.y};
            const int page_direction = pressed_page_direction_ == page_direction_at_point(point) ? pressed_page_direction_ : 0;
            pressed_page_direction_ = 0;
            const bool activate_settings = settings_button_pressed_ && hit_test_settings_button(point);
            const auto display_index = candidate_pointer_.release(
                selection_callback_ ? candidate_index_at_point(layout_, point) : std::nullopt);
            const auto model_index = display_index ? candidate_model_index(layout_, *display_index, candidate_texts_)
                                                    : std::nullopt;
            settings_button_pressed_ = false;
            if (GetCapture() == hwnd_) ReleaseCapture();
            update_candidate_hot(point);
            track_mouse_leave();
            InvalidateRect(hwnd_, nullptr, FALSE);
            if (page_direction && page_callback_) {
                const auto callback = page_callback_;
                callback(page_direction);
                return 0;
            }
            if (activate_settings) {
                open_settings_window();
                return 0;
            }
            if (model_index && selection_callback_) {
                // Committing may hide the popup or destroy the service. Do not
                // access window members after invoking the copied callback.
                const auto callback = selection_callback_;
                callback(*model_index);
                return 0;
            }
            break;
        }
        case WM_CAPTURECHANGED:
        case WM_CANCELMODE:
            cancel_pointer_interaction();
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        case WM_SETCURSOR:
            if (settings_button_hot_ || (selection_callback_ && candidate_pointer_.hovered())) {
                SetCursor(LoadCursorW(nullptr, IDC_HAND));
                return TRUE;
            }
            break;
        case WM_DESTROY:
            cancel_pointer_interaction();
            notify_ime_window_hidden(hwnd);
            break;
        case WM_NCDESTROY:
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            if (hwnd_ == hwnd) hwnd_ = nullptr;
            return DefWindowProcW(hwnd, message, wparam, lparam);
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT:
            paint_seen_ = true;
            paint();
            return 0;
        case WM_PRINTCLIENT:
            render(reinterpret_cast<HDC>(wparam));
            return 0;
        default:
            return DefWindowProcW(hwnd, message, wparam, lparam);
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

bool CandidateWindow::notify_ime_window_shown(HWND hwnd) {
    RECT rect{};
    if (!hwnd || !IsWindow(hwnd) || !IsWindowVisible(hwnd) || !GetWindowRect(hwnd, &rect)) {
        return false;
    }
    return ime_win_event_state_.notify_visible(hwnd, rect);
}

void CandidateWindow::notify_ime_window_hidden(HWND hwnd) noexcept {
    ime_win_event_state_.notify_hidden(hwnd);
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

bool CandidateWindow::rebuild_layout(int dpi) {
    cancel_pointer_interaction();
    update_fonts(dpi);
    const int fallback_font_px = candidate_font_px_for_size(options_.text_size, dpi);
    std::vector<int> candidate_text_widths;
    std::vector<int> number_text_widths;
    candidate_text_widths.reserve(candidate_texts_.size());
    number_text_widths.reserve(candidate_texts_.size());
    for (size_t i = 0; i < candidate_texts_.size(); ++i) {
        const int measured = renderer_.measure_text(candidate_texts_[i], static_cast<float>(fallback_font_px), FW_MEDIUM);
        candidate_text_widths.push_back(measured > 0 ? measured : measure_text_width_px(hwnd_, candidate_font_, candidate_texts_[i], fallback_font_px));
        const std::wstring number = std::to_wstring(i + 1);
        const int number_width = renderer_.measure_text(number, static_cast<float>(fallback_font_px));
        number_text_widths.push_back(number_width > 0 ? number_width : measure_text_width_px(hwnd_, candidate_font_, number, fallback_font_px));
    }

    CandidateLayoutInput input;
    input.composition_text = composition_text_;
    input.candidate_texts = candidate_texts_;
    input.candidate_text_widths_px = std::move(candidate_text_widths);
    input.number_text_widths_px = std::move(number_text_widths);
    input.selected_index = selected_index_;
    input.page_indices = page_indices_;
    input.composition_text_width_px = renderer_.measure_text(composition_text_, static_cast<float>(scale_dip(15, dpi)));
    input.caret_rect = caret_rect_;
    input.work_area = monitor_work_area_for(caret_rect_);
    input.dpi = dpi;
    input.options = options_;
    layout_ = make_candidate_layout(input);
    return layout_.visible;
}

void CandidateWindow::apply_options_changed() {
    reload_options();
    const auto callback = options_changed_callback_;
    if (callback) callback();
    if (!hwnd_ || !IsWindowVisible(hwnd_) || !has_cached_show_state_) {
        return;
    }
    const int dpi = GetDpiForWindow(hwnd_);
    if (!rebuild_layout(dpi)) {
        hide(L"options_changed_layout_hidden");
        return;
    }
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
    const BOOL set_window_pos_ok = SetWindowPos(hwnd_,
                                                HWND_TOPMOST,
                                                layout_.window_rect.left,
                                                layout_.window_rect.top,
                                                width,
                                                height,
                                                SWP_NOACTIVATE | SWP_SHOWWINDOW);
    const DWORD set_window_pos_error = set_window_pos_ok ? ERROR_SUCCESS : GetLastError();
    if (!set_window_pos_ok) {
        std::wstringstream stream;
        stream << L"candidate_options_changed_setwindowpos_success=false last_error=" << set_window_pos_error;
        log_status(L"candidate-ui", stream.str());
    }
    RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
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

void CandidateWindow::update_candidate_hot(POINT point) {
    const auto index = selection_callback_ ? candidate_index_at_point(layout_, point) : std::nullopt;
    if (candidate_pointer_.hovered() == index) return;
    candidate_pointer_.move(index);
    if (hwnd_) InvalidateRect(hwnd_, nullptr, FALSE);
}

void CandidateWindow::cancel_pointer_interaction() noexcept {
    candidate_pointer_.cancel();
    settings_button_hot_ = false;
    settings_button_pressed_ = false;
    pressed_page_direction_ = 0;
    if (tracking_mouse_leave_ && hwnd_) {
        TRACKMOUSEEVENT event{};
        event.cbSize = sizeof(event);
        event.dwFlags = TME_LEAVE | TME_CANCEL;
        event.hwndTrack = hwnd_;
        TrackMouseEvent(&event);
    }
    tracking_mouse_leave_ = false;
    if (hwnd_ && GetCapture() == hwnd_) ReleaseCapture();
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

int CandidateWindow::page_direction_at_point(POINT point) const noexcept {
    if (!layout_.visible) return 0;
    if (PtInRect(&layout_.previous_page_rect, point)) return -1;
    if (PtInRect(&layout_.next_page_rect, point)) return 1;
    return 0;
}

void CandidateWindow::draw_page_controls(HDC hdc) {
    const std::wstring page = std::to_wstring(layout_.current_page + 1) + L"/" + std::to_wstring(layout_.page_indices.size());
    auto draw = [&](const std::wstring& text, RECT rect, bool enabled) {
        const COLORREF color = enabled ? palette_.muted_text : blend_color(palette_.background, palette_.muted_text, 40);
        const UINT flags = DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX;
        if (auto* canvas = active_modern_renderer()) {
            canvas->text(text, rect, color, static_cast<float>(layout_.hint_font_px), FW_NORMAL, flags);
        } else {
            if (hint_font_) SelectObject(hdc, hint_font_);
            SetTextColor(hdc, color);
            DrawTextW(hdc, text.c_str(), static_cast<int>(text.size()), &rect, flags);
        }
    };
    draw(L"‹", layout_.previous_page_rect, layout_.has_previous_page);
    draw(page, layout_.page_indicator_rect, true);
    draw(L"›", layout_.next_page_rect, layout_.has_next_page);
}

void CandidateWindow::draw_settings_button(HDC hdc) {
    if (!layout_.action_button_visible) {
        return;
    }

    const RECT rect = layout_.action_button_rect;
    const int icon_extent = (std::min)(rect.right - rect.left, rect.bottom - rect.top);
    COLORREF fill = blend_color(palette_.background, palette_.border, 30);
    if (settings_button_hot_) {
        fill = blend_color(fill, palette_.selected_background, palette_.dark ? 28 : 16);
    }
    if (settings_button_pressed_) {
        fill = blend_color(fill, RGB(0, 0, 0), palette_.dark ? 18 : 8);
    }
    const COLORREF border_color = settings_button_hot_ ? palette_.selected_border : palette_.border;
    if (auto* canvas = active_modern_renderer()) {
        canvas->rounded(rect, static_cast<float>(scale_dip(7, layout_.dpi)), fill, border_color);
        RECT icon = rect;
        const int inset = (std::min)(scale_dip(6, layout_.dpi), icon_extent / 4);
        InflateRect(&icon, -inset, -inset);
        canvas->icon(8, icon, settings_button_hot_ ? palette_.number_text : palette_.muted_text,
                       scale_dip(1, layout_.dpi) * 1.5f);
        return;
    }
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, std::max(1, scale_dip(1, layout_.dpi)), border_color);
    fill_round_rect(hdc, rect, scale_dip(8, layout_.dpi) * 2, brush, pen);
    DeleteObject(pen);
    DeleteObject(brush);

    // Draw the settings affordance with GDI, so it stays crisp at every DPI
    // without a symbol-font dependency or a bitmap resource.
    const int center_x = (rect.left + rect.right) / 2;
    const int center_y = (rect.top + rect.bottom) / 2;
    HPEN icon_pen = CreatePen(PS_SOLID, std::max(1, scale_dip(2, layout_.dpi)),
                              settings_button_hot_ ? palette_.number_text : palette_.muted_text);
    HGDIOBJ old_pen = SelectObject(hdc, icon_pen);
    HGDIOBJ old_brush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    const int radius = (std::min)(scale_dip(5, layout_.dpi), icon_extent / 6);
    Ellipse(hdc, center_x - radius, center_y - radius, center_x + radius, center_y + radius);
    const int inner = (std::min)(scale_dip(2, layout_.dpi), icon_extent / 12);
    Ellipse(hdc, center_x - inner, center_y - inner, center_x + inner, center_y + inner);
    const int outer = (std::min)(scale_dip(8, layout_.dpi), icon_extent / 3);
    const POINT spokes[][2] = {
        {{center_x - outer, center_y}, {center_x - radius, center_y}},
        {{center_x + radius, center_y}, {center_x + outer, center_y}},
        {{center_x, center_y - outer}, {center_x, center_y - radius}},
        {{center_x, center_y + radius}, {center_x, center_y + outer}}
    };
    for (const auto& spoke : spokes) {
        MoveToEx(hdc, spoke[0].x, spoke[0].y, nullptr);
        LineTo(hdc, spoke[1].x, spoke[1].y);
    }
    SelectObject(hdc, old_brush);
    SelectObject(hdc, old_pen);
    DeleteObject(icon_pen);
}

void CandidateWindow::paint() {
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hwnd_, &ps);
    render(hdc);
    EndPaint(hwnd_, &ps);
}

void CandidateWindow::render(HDC hdc) {
    if (!hdc || !hwnd_) return;
    RECT rect{};
    GetClientRect(hwnd_, &rect);
    if (rect.right <= rect.left || rect.bottom <= rect.top) return;

    HDC memory_dc = CreateCompatibleDC(hdc);
    HBITMAP bitmap = memory_dc ? CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top) : nullptr;
    if (!bitmap) {
        if (memory_dc) DeleteDC(memory_dc);
        return;
    }
    HGDIOBJ old_bitmap = SelectObject(memory_dc, bitmap);
    SetBkMode(memory_dc, TRANSPARENT);

    ModernRenderScope modern(renderer_, memory_dc, rect);
    if (modern.active()) {
        renderer_.fill(rect, palette_.background);
        renderer_.rounded(rect, static_cast<float>(layout_.corner_radius), palette_.background, palette_.border);
        renderer_.text(composition_text_, layout_.composition_rect, palette_.muted_text,
                         static_cast<float>(layout_.composition_font_px), FW_NORMAL, candidate_text_draw_flags(true));
        for (const auto& item : layout_.items) {
            if (item.selected) renderer_.rounded(item.bounds, static_cast<float>(layout_.selected_corner_radius),
                                                 palette_.selected_background, palette_.selected_border);
            else if (candidate_pointer_.hovered() == item.candidate_index) {
                const int tint = candidate_pointer_.pressed() == item.candidate_index ? 22 : 12;
                const COLORREF hover = blend_color(palette_.background, palette_.selected_background, tint);
                renderer_.rounded(item.bounds, static_cast<float>(layout_.selected_corner_radius), hover, hover);
            }
            renderer_.text(std::to_wstring(item.page_number), item.number_bounds,
                             item.selected ? palette_.selected_text : palette_.number_text,
                             static_cast<float>(layout_.candidate_font_px));
            renderer_.text(item.display_text, item.text_bounds,
                             item.selected ? palette_.selected_text : palette_.text,
                             static_cast<float>(layout_.candidate_font_px), FW_MEDIUM, candidate_text_draw_flags(item.truncated));
        }
        draw_page_controls(memory_dc);
        draw_settings_button(memory_dc);
        if (layout_.show_hint) {
            const float y = static_cast<float>(layout_.hint_rect.top - scale_dip(4, layout_.dpi));
            renderer_.line(static_cast<float>(layout_.hint_rect.left), y,
                             static_cast<float>(layout_.hint_rect.right), y, palette_.border, 1);
            renderer_.text(prediction_mode_ ? kPredictionHintText : kHintText, layout_.hint_rect, palette_.hint_text,
                             static_cast<float>(layout_.hint_font_px));
        }
        modern.finish();
        BitBlt(hdc, 0, 0, rect.right, rect.bottom, memory_dc, 0, 0, SRCCOPY);
        SelectObject(memory_dc, old_bitmap);
        DeleteObject(bitmap);
        DeleteDC(memory_dc);
        return;
    }

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
        } else if (candidate_pointer_.hovered() == item.candidate_index) {
            const int tint = candidate_pointer_.pressed() == item.candidate_index ? 22 : 12;
            const COLORREF hover = blend_color(palette_.background, palette_.selected_background, tint);
            HBRUSH hover_brush = CreateSolidBrush(hover);
            HPEN hover_pen = CreatePen(PS_SOLID, 1, hover);
            fill_round_rect(memory_dc, item.bounds, layout_.selected_corner_radius * 2, hover_brush, hover_pen);
            DeleteObject(hover_pen);
            DeleteObject(hover_brush);
        }

        const std::wstring number = std::to_wstring(item.page_number);
        RECT number_rect = item.number_bounds;
        SetTextColor(memory_dc, item.selected ? palette_.selected_text : palette_.number_text);
        DrawTextW(memory_dc, number.c_str(), static_cast<int>(number.size()), &number_rect,
                  DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);

        RECT text_rect = item.text_bounds;
        SetTextColor(memory_dc, item.selected ? palette_.selected_text : palette_.text);
        DrawTextW(memory_dc, item.display_text.c_str(), static_cast<int>(item.display_text.size()), &text_rect,
                  candidate_text_draw_flags(item.truncated));
    }

    draw_page_controls(memory_dc);
    draw_settings_button(memory_dc);

    if (layout_.show_hint) {
        HPEN separator = CreatePen(PS_SOLID, 1, palette_.border);
        HGDIOBJ old_pen = SelectObject(memory_dc, separator);
        const int separator_y = layout_.hint_rect.top - scale_dip(4, layout_.dpi);
        MoveToEx(memory_dc, layout_.hint_rect.left, separator_y, nullptr);
        LineTo(memory_dc, layout_.hint_rect.right, separator_y);
        SelectObject(memory_dc, old_pen);
        DeleteObject(separator);
        if (hint_font_) {
            SelectObject(memory_dc, hint_font_);
        }
        RECT hint_rect = layout_.hint_rect;
        SetTextColor(memory_dc, palette_.hint_text);
        const auto hint = prediction_mode_ ? kPredictionHintText : kHintText;
        DrawTextW(memory_dc, hint, static_cast<int>(wcslen(hint)), &hint_rect,
                  DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

    BitBlt(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top, memory_dc, 0, 0, SRCCOPY);
    SelectObject(memory_dc, old_bitmap);
    DeleteObject(border);
    DeleteObject(background);
    DeleteObject(bitmap);
    DeleteDC(memory_dc);

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
