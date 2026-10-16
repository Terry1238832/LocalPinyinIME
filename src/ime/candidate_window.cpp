#include "candidate_window.h"

#include "globals.h"
#include "../common/logging.h"
#include "../settings/settings_store.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <sstream>
#include <string>
#include <utility>

namespace localpinyin {
namespace {

constexpr wchar_t kWindowClassName[] = L"LocalPinyinCandidateWindow";
constexpr wchar_t kCandidateOptionsChangedMessageName[] = L"LocalPinyinIME.CandidateOptionsChanged";
constexpr wchar_t kEllipsis[] = L"…";
constexpr wchar_t kSettingsExecutableName[] = L"LocalPinyinSettings.exe";
constexpr UINT_PTR kLayoutRetryTimerId = 1;
constexpr UINT kDwmAttributeCloaked = 14;
constexpr DWORD kDwmCloakedShell = 0x00000002;

using DwmGetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, PVOID, DWORD);

struct DwmSnapshot {
    HRESULT cloaked_hr = E_NOTIMPL;
    DWORD cloaked = 0;
};

// Windows Store (immersive) processes: the shell cloaks unowned top-level
// windows, so the popup must be owned by the text input's window there.
bool immersive_process() noexcept {
    using IsImmersiveProcessFn = BOOL(WINAPI*)(HANDLE);
    static const auto function = reinterpret_cast<IsImmersiveProcessFn>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "IsImmersiveProcess"));
    static const bool immersive = function && function(GetCurrentProcess());
    return immersive;
}

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

// The settings app sits beside the input method DLL. It is single-instance,
// so a second launch only brings the open window forward.
void open_settings_app() {
    wchar_t module_path[MAX_PATH]{};
    const DWORD size = GetModuleFileNameW(g_instance, module_path, MAX_PATH);
    if (size == 0 || size >= MAX_PATH) return;
    std::wstring path(module_path, size);
    const size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return;
    const std::wstring directory = path.substr(0, slash);
    path = directory + L"\\" + kSettingsExecutableName;
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        log_status(L"candidate-ui", L"settings_open=false reason=settings_exe_not_found");
        return;
    }
    std::wstring command_line = L"\"" + path + L"\"";
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(path.c_str(), command_line.data(), nullptr, nullptr, FALSE, 0, nullptr,
                        directory.c_str(), &startup, &process)) {
        log_status(L"candidate-ui", L"settings_open=false create_process_last_error=" + std::to_wstring(GetLastError()));
        return;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
}

void notify_candidate_ime_win_event(DWORD event,
                                    HWND hwnd,
                                    LONG object_id,
                                    LONG child_id,
                                    void*) noexcept {
    NotifyWinEvent(event, hwnd, object_id, child_id);
}

// Coverage (0..1) of a rounded rectangle at a pixel center, antialiased over
// one pixel. Used only for the shadow mask, which is blurred afterwards.
float rounded_rect_coverage(float x, float y, const RECT& rect, float radius) noexcept {
    const float cx = std::clamp(x, rect.left + radius, rect.right - radius);
    const float cy = std::clamp(y, rect.top + radius, rect.bottom - radius);
    const float distance = std::hypot(x - cx, y - cy) - radius;
    return std::clamp(0.5f - distance, 0.0f, 1.0f);
}

// Three box blurs approximate a Gaussian. Operates in place on one channel.
void box_blur(std::vector<float>& values, int width, int height, int radius) {
    if (radius <= 0) return;
    std::vector<float> scratch(values.size());
    const float scale = 1.0f / static_cast<float>(radius * 2 + 1);
    for (int pass = 0; pass < 3; ++pass) {
        for (int y = 0; y < height; ++y) {
            float sum = 0;
            const float* row = values.data() + static_cast<size_t>(y) * width;
            for (int x = -radius; x <= radius; ++x) sum += row[std::clamp(x, 0, width - 1)];
            for (int x = 0; x < width; ++x) {
                scratch[static_cast<size_t>(y) * width + x] = sum * scale;
                sum += row[std::min(x + radius + 1, width - 1)] - row[std::max(x - radius, 0)];
            }
        }
        for (int x = 0; x < width; ++x) {
            float sum = 0;
            for (int y = -radius; y <= radius; ++y) sum += scratch[static_cast<size_t>(std::clamp(y, 0, height - 1)) * width + x];
            for (int y = 0; y < height; ++y) {
                values[static_cast<size_t>(y) * width + x] = sum * scale;
                sum += scratch[static_cast<size_t>(std::min(y + radius + 1, height - 1)) * width + x] -
                       scratch[static_cast<size_t>(std::max(y - radius, 0)) * width + x];
            }
        }
    }
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

    // The popup draws its own antialiased corners and soft shadow through
    // per-pixel alpha, so the class needs no legacy CS_DROPSHADOW.
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
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
        owner_hwnd_ = create_config.parent;
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

bool CandidateWindow::position_and_show_current_layout() {
    if (!present()) {
        log_status(L"candidate-ui", L"UpdateLayeredWindow failed last_error=" + std::to_wstring(GetLastError()));
        return false;
    }
    SetLastError(ERROR_SUCCESS);
    const BOOL set_window_pos_ok = SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0,
                                                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    if (!set_window_pos_ok) {
        log_status(L"candidate-ui",
                   L"SetWindowPos failed last_error=" + std::to_wstring(GetLastError()));
        return false;
    }
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
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
    const HWND owner = immersive_process() && owner_info.context_valid ? owner_info.context_hwnd : nullptr;
    if (hwnd_ && owner && owner != owner_hwnd_) {
        // The input moved to another window: recreate under the new owner.
        destroy_hwnd_for_recreate();
        reset_window_lifecycle_state();
    }
    if (!create_with_owner(owner)) {
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
    redraw();
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
        case WM_SETTINGCHANGE:
        case WM_THEMECHANGED:
        case WM_SYSCOLORCHANGE:
            if (message != WM_SETTINGCHANGE ||
                (lparam && std::wcscmp(reinterpret_cast<const wchar_t*>(lparam), L"ImmersiveColorSet") == 0) ||
                wparam == SPI_SETHIGHCONTRAST) {
                apply_options_changed();
                const auto callback = theme_changed_callback_;
                if (callback) callback();
            }
            break;
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case WM_MOUSEMOVE: {
            const POINTS points = MAKEPOINTS(lparam);
            const POINT point{points.x, points.y};
            update_pointer_hot(point);
            track_mouse_leave();
            return 0;
        }
        case WM_MOUSELEAVE:
            tracking_mouse_leave_ = false;
            cancel_pointer_interaction();
            redraw();
            return 0;
        case WM_LBUTTONDOWN: {
            const POINTS points = MAKEPOINTS(lparam);
            const POINT point{points.x, points.y};
            const int page_direction = page_direction_at_point(point);
            if (page_direction && page_callback_) {
                candidate_pointer_.cancel();
                pressed_page_direction_ = page_direction;
                SetCapture(hwnd_);
                redraw();
                return 0;
            }
            if (settings_button_at_point(point)) {
                candidate_pointer_.cancel();
                settings_pressed_ = true;
                SetCapture(hwnd_);
                redraw();
                return 0;
            }
            if (selection_callback_) {
                const auto index = candidate_index_at_point(layout_, point);
                if (index) {
                    candidate_pointer_.press(index);
                    SetCapture(hwnd_);
                    track_mouse_leave();
                    redraw();
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
            const bool open_settings = settings_pressed_ && settings_button_at_point(point);
            settings_pressed_ = false;
            const auto display_index = candidate_pointer_.release(
                selection_callback_ ? candidate_index_at_point(layout_, point) : std::nullopt);
            const auto model_index = display_index ? candidate_model_index(layout_, *display_index, candidate_texts_)
                                                    : std::nullopt;
            if (GetCapture() == hwnd_) ReleaseCapture();
            update_pointer_hot(point);
            track_mouse_leave();
            redraw();
            if (page_direction && page_callback_) {
                const auto callback = page_callback_;
                callback(page_direction);
                return 0;
            }
            if (open_settings) {
                open_settings_app();
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
            redraw();
            return 0;
        case WM_SETCURSOR:
            if (hot_page_direction_ || settings_hot_ || (selection_callback_ && candidate_pointer_.hovered())) {
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
        case WM_PAINT: {
            // Content is pushed with UpdateLayeredWindow; only validate here.
            PAINTSTRUCT ps{};
            BeginPaint(hwnd_, &ps);
            EndPaint(hwnd_, &ps);
            return 0;
        }
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
    if (composition_font_ && candidate_font_ && font_dpi_ == dpi && font_text_size_ == options_.text_size) {
        return;
    }
    release_fonts();
    font_dpi_ = dpi;
    font_text_size_ = options_.text_size;

    composition_font_ = create_candidate_font(scale_dip(13, dpi), FW_NORMAL);
    candidate_font_ = create_candidate_font(candidate_font_px_for_size(options_.text_size, dpi), FW_NORMAL);
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
        const int measured = renderer_.measure_text(candidate_texts_[i], static_cast<float>(fallback_font_px), FW_NORMAL);
        candidate_text_widths.push_back(measured > 0 ? measured : measure_text_width_px(hwnd_, candidate_font_, candidate_texts_[i], fallback_font_px));
        // Numbers are drawn smaller than the candidate text.
        const std::wstring number = std::to_wstring(i % kMaxCandidateCount + 1);
        const float number_px = static_cast<float>(fallback_font_px) * 0.78f;
        const int number_width = renderer_.measure_text(number, number_px);
        number_text_widths.push_back(number_width > 0 ? number_width : measure_text_width_px(hwnd_, candidate_font_, number, fallback_font_px));
    }

    CandidateLayoutInput input;
    input.composition_text = composition_text_;
    input.candidate_texts = candidate_texts_;
    input.candidate_text_widths_px = std::move(candidate_text_widths);
    input.number_text_widths_px = std::move(number_text_widths);
    input.selected_index = selected_index_;
    input.page_indices = page_indices_;
    input.composition_text_width_px = renderer_.measure_text(composition_text_, static_cast<float>(scale_dip(13, dpi)));
    input.caret_rect = caret_rect_;
    input.work_area = monitor_work_area_for(caret_rect_);
    input.dpi = dpi;
    input.options = options_;
    // A Store app sandbox cannot start the settings app.
    input.settings_button = !immersive_process();
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
    if (!present()) {
        log_status(L"candidate-ui", L"candidate_options_changed_present_success=false last_error=" +
                   std::to_wstring(GetLastError()));
    }
}

void CandidateWindow::update_pointer_hot(POINT point) {
    const int page = page_direction_at_point(point);
    const bool settings = settings_button_at_point(point);
    const auto index = selection_callback_ && !page && !settings ? candidate_index_at_point(layout_, point) : std::nullopt;
    if (candidate_pointer_.hovered() == index && hot_page_direction_ == page && settings_hot_ == settings) return;
    candidate_pointer_.move(index);
    hot_page_direction_ = page;
    settings_hot_ = settings;
    redraw();
}

bool CandidateWindow::settings_button_at_point(POINT point) const noexcept {
    return layout_.visible && layout_.action_button_visible && PtInRect(&layout_.action_button_rect, point);
}

void CandidateWindow::draw_settings_button() {
    auto* canvas = active_modern_renderer();
    if (!canvas || !layout_.action_button_visible) return;
    const RECT rect = layout_.action_button_rect;
    // A hairline separates the button from the candidates.
    const float separator_x = rect.left - layout_.item_gap_px * 1.5f;
    const float inset = (rect.bottom - rect.top) * 0.2f;
    canvas->line(separator_x, rect.top + inset, separator_x, rect.bottom - inset, palette_.border, 1.0f);
    COLORREF fill = palette_.background;
    if (settings_hot_ || settings_pressed_) {
        fill = settings_pressed_ ? blend_color(palette_.hover_background, palette_.text, palette_.dark ? 10 : 6)
                                 : palette_.hover_background;
        canvas->rounded(rect, static_cast<float>(layout_.selected_corner_radius), fill, fill);
    }
    // Gear: eight rounded teeth around a ring with a hollow centre.
    const COLORREF color = settings_hot_ ? palette_.text : palette_.muted_text;
    const float cx = (rect.left + rect.right) / 2.0f;
    const float cy = (rect.top + rect.bottom) / 2.0f;
    const float unit = (rect.right - rect.left) / 24.0f;
    const float stroke = (std::max)(1.2f, unit * 1.5f);
    for (int tooth = 0; tooth < 8; ++tooth) {
        const float angle = tooth * 3.14159265f / 4.0f;
        const float c = std::cos(angle), s = std::sin(angle);
        canvas->line(cx + c * unit * 5.0f, cy + s * unit * 5.0f, cx + c * unit * 7.4f, cy + s * unit * 7.4f, color, stroke * 1.35f);
    }
    canvas->ellipse(cx, cy, unit * 5.8f, unit * 5.8f, color);
    canvas->ellipse(cx, cy, unit * 5.8f - stroke, unit * 5.8f - stroke, fill);
    canvas->ellipse(cx, cy, unit * 2.3f, unit * 2.3f, color);
    canvas->ellipse(cx, cy, unit * 2.3f - stroke, unit * 2.3f - stroke, fill);
}

void CandidateWindow::cancel_pointer_interaction() noexcept {
    candidate_pointer_.cancel();
    hot_page_direction_ = 0;
    pressed_page_direction_ = 0;
    settings_hot_ = false;
    settings_pressed_ = false;
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

int CandidateWindow::page_direction_at_point(POINT point) const noexcept {
    if (!layout_.visible || !layout_.paging_visible) return 0;
    if (PtInRect(&layout_.previous_page_rect, point)) return -1;
    if (PtInRect(&layout_.next_page_rect, point)) return 1;
    return 0;
}

std::wstring CandidateWindow::fitted_composition_text(int available_px) {
    // Keep the end of long pinyin visible: it is where typing happens.
    const float size = static_cast<float>(layout_.composition_font_px);
    std::wstring text = composition_text_;
    if (available_px <= 0 || renderer_.measure_text(text, size) <= available_px) return text;
    size_t drop = 1;
    while (drop < composition_text_.size()) {
        text = kEllipsis + composition_text_.substr(drop);
        if (renderer_.measure_text(text, size) <= available_px) break;
        drop += std::max<size_t>(1, (composition_text_.size() - drop) / 8);
    }
    return text;
}

void CandidateWindow::draw_page_button(int direction) {
    auto* canvas = active_modern_renderer();
    if (!canvas || !layout_.paging_visible) return;
    const RECT rect = direction < 0 ? layout_.previous_page_rect : layout_.next_page_rect;
    const bool enabled = direction < 0 ? layout_.has_previous_page : layout_.has_next_page;
    if (enabled && (hot_page_direction_ == direction || pressed_page_direction_ == direction)) {
        const COLORREF fill = pressed_page_direction_ == direction
            ? blend_color(palette_.hover_background, palette_.text, palette_.dark ? 10 : 6)
            : palette_.hover_background;
        canvas->rounded(rect, static_cast<float>(layout_.selected_corner_radius), fill, fill);
    }
    const COLORREF color = enabled ? palette_.muted_text : blend_color(palette_.background, palette_.muted_text, 35);
    const float cx = (rect.left + rect.right) / 2.0f;
    const float cy = (rect.top + rect.bottom) / 2.0f;
    const float arm = static_cast<float>(std::max(3, scale_dip(4, layout_.dpi)));
    const float stroke = std::max(1.25f, static_cast<float>(layout_.dpi) / 96.0f * 1.6f);
    const float tip = cx + (direction < 0 ? -arm / 2 : arm / 2);
    const float back = cx + (direction < 0 ? arm / 2 : -arm / 2);
    canvas->line(back, cy - arm, tip, cy, color, stroke);
    canvas->line(tip, cy, back, cy + arm, color, stroke);
}

void CandidateWindow::draw_content() {
    auto* canvas = active_modern_renderer();
    if (!canvas) return;
    const float radius = static_cast<float>(layout_.corner_radius);
    canvas->rounded(layout_.panel_rect, radius, palette_.background, palette_.border);
    const RECT& label = layout_.composition_rect;
    canvas->text(fitted_composition_text(label.right - label.left), label, palette_.composition_text,
                 static_cast<float>(layout_.composition_font_px), FW_NORMAL,
                 DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
    const float candidate_px = static_cast<float>(layout_.candidate_font_px);
    for (const auto& item : layout_.items) {
        // Inset the highlight vertically so it reads as a pill inside the row.
        RECT pill = item.bounds;
        const int inset = std::max(1, scale_dip(2, layout_.dpi));
        pill.top += inset;
        pill.bottom -= inset;
        if (item.selected) {
            canvas->rounded(pill, static_cast<float>(layout_.selected_corner_radius),
                            palette_.selected_background, palette_.selected_background);
        } else if (candidate_pointer_.hovered() == item.candidate_index) {
            const COLORREF hover = candidate_pointer_.pressed() == item.candidate_index
                ? blend_color(palette_.hover_background, palette_.selected_background, 14)
                : palette_.hover_background;
            canvas->rounded(pill, static_cast<float>(layout_.selected_corner_radius), hover, hover);
        }
        canvas->text(std::to_wstring(item.page_number), item.number_bounds,
                     item.selected ? palette_.selected_number_text : palette_.number_text,
                     candidate_px * 0.78f, FW_NORMAL, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
        canvas->text(item.display_text, item.text_bounds,
                     item.selected ? palette_.selected_text : palette_.text,
                     candidate_px, FW_NORMAL, candidate_text_draw_flags(item.truncated));
    }
    draw_page_button(-1);
    draw_page_button(1);
    draw_settings_button();
}

void CandidateWindow::composite_shadow(std::uint8_t* pixels, int width, int height) const {
    if (palette_.shadow_alpha <= 0 || layout_.shadow_margin_px <= 0) return;
    // Blurred panel silhouette, slightly offset downward, placed beneath the
    // already drawn premultiplied panel ("destination over").
    const int offset = std::max(1, scale_dip(2, layout_.dpi));
    RECT casting = layout_.panel_rect;
    OffsetRect(&casting, 0, offset);
    std::vector<float> mask(static_cast<size_t>(width) * height);
    const float radius = static_cast<float>(layout_.corner_radius);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            mask[static_cast<size_t>(y) * width + x] = rounded_rect_coverage(x + 0.5f, y + 0.5f, casting, radius);
    box_blur(mask, width, height, std::max(1, layout_.shadow_margin_px / 3));
    const float peak = static_cast<float>(palette_.shadow_alpha);
    for (size_t index = 0; index < mask.size(); ++index) {
        std::uint8_t* pixel = pixels + index * 4;
        const float shadow = mask[index] * peak;
        const float remaining = 1.0f - pixel[3] / 255.0f;
        // Black shadow: premultiplied color stays, only alpha accumulates.
        pixel[3] = static_cast<std::uint8_t>(std::clamp(pixel[3] + shadow * remaining + 0.5f, 0.0f, 255.0f));
    }
}

void CandidateWindow::render_layered(HDC dc, std::uint8_t* pixels, int width, int height) {
    std::memset(pixels, 0, static_cast<size_t>(width) * height * 4);
    const RECT bounds{0, 0, width, height};
    {
        ModernRenderScope modern(renderer_, dc, bounds, true);
        if (modern.active()) {
            draw_content();
            modern.finish();
            GdiFlush();
            composite_shadow(pixels, width, height);
            return;
        }
    }
    // GDI fallback: an opaque, square-cornered panel without shadow.
    HBRUSH background = CreateSolidBrush(palette_.background);
    FillRect(dc, &layout_.panel_rect, background);
    DeleteObject(background);
    HBRUSH border = CreateSolidBrush(palette_.border);
    FrameRect(dc, &layout_.panel_rect, border);
    DeleteObject(border);
    SetBkMode(dc, TRANSPARENT);
    if (composition_font_) SelectObject(dc, composition_font_);
    SetTextColor(dc, palette_.composition_text);
    RECT label = layout_.composition_rect;
    DrawTextW(dc, composition_text_.c_str(), static_cast<int>(composition_text_.size()), &label,
              DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    if (candidate_font_) SelectObject(dc, candidate_font_);
    for (const auto& item : layout_.items) {
        if (item.selected) {
            HBRUSH selected = CreateSolidBrush(palette_.selected_background);
            FillRect(dc, &item.bounds, selected);
            DeleteObject(selected);
        }
        const std::wstring number = std::to_wstring(item.page_number);
        RECT number_rect = item.number_bounds;
        SetTextColor(dc, item.selected ? palette_.selected_number_text : palette_.number_text);
        DrawTextW(dc, number.c_str(), static_cast<int>(number.size()), &number_rect, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
        RECT text_rect = item.text_bounds;
        SetTextColor(dc, item.selected ? palette_.selected_text : palette_.text);
        DrawTextW(dc, item.display_text.c_str(), static_cast<int>(item.display_text.size()), &text_rect,
                  candidate_text_draw_flags(item.truncated));
    }
    GdiFlush();
    for (int y = layout_.panel_rect.top; y < layout_.panel_rect.bottom; ++y)
        for (int x = layout_.panel_rect.left; x < layout_.panel_rect.right; ++x)
            pixels[(static_cast<size_t>(y) * width + x) * 4 + 3] = 255;
}

bool CandidateWindow::present() {
    if (!hwnd_ || !layout_.visible || layout_.width <= 0 || layout_.height <= 0) return false;
    const int width = layout_.width;
    const int height = layout_.height;
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    HDC screen = GetDC(nullptr);
    HDC dc = screen ? CreateCompatibleDC(screen) : nullptr;
    void* bits = nullptr;
    HBITMAP dib = dc ? CreateDIBSection(screen, &info, DIB_RGB_COLORS, &bits, nullptr, 0) : nullptr;
    bool presented = false;
    if (dib && bits) {
        const HGDIOBJ previous = SelectObject(dc, dib);
        render_layered(dc, static_cast<std::uint8_t*>(bits), width, height);
        POINT source{0, 0};
        POINT destination{layout_.window_rect.left, layout_.window_rect.top};
        SIZE size{width, height};
        BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
        presented = UpdateLayeredWindow(hwnd_, screen, &destination, &size, dc, &source, 0, &blend, ULW_ALPHA) != FALSE;
        SelectObject(dc, previous);
    }
    if (dib) DeleteObject(dib);
    if (dc) DeleteDC(dc);
    if (screen) ReleaseDC(nullptr, screen);
    if (presented) {
        paint_seen_ = true;
        if (IsWindowVisible(hwnd_)) {
            RECT rect{};
            if (GetWindowRect(hwnd_, &rect)) ime_win_event_state_.notify_rect_changed(hwnd_, rect);
        }
    }
    return presented;
}

void CandidateWindow::redraw() {
    if (hwnd_ && has_cached_show_state_ && IsWindowVisible(hwnd_)) present();
}

void CandidateWindow::render(HDC hdc) {
    // WM_PRINTCLIENT and offline capture: compose the layered image over the
    // destination with its alpha.
    if (!hdc || !layout_.visible || layout_.width <= 0 || layout_.height <= 0) return;
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = layout_.width;
    info.bmiHeader.biHeight = -layout_.height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    HDC dc = CreateCompatibleDC(hdc);
    void* bits = nullptr;
    HBITMAP dib = dc ? CreateDIBSection(hdc, &info, DIB_RGB_COLORS, &bits, nullptr, 0) : nullptr;
    if (dib && bits) {
        const HGDIOBJ previous = SelectObject(dc, dib);
        render_layered(dc, static_cast<std::uint8_t*>(bits), layout_.width, layout_.height);
        BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
        GdiAlphaBlend(hdc, 0, 0, layout_.width, layout_.height, dc, 0, 0, layout_.width, layout_.height, blend);
        SelectObject(dc, previous);
    }
    if (dib) DeleteObject(dib);
    if (dc) DeleteDC(dc);
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
}

}  // namespace localpinyin
