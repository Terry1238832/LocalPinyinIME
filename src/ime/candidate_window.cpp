#include "candidate_window.h"

#include "globals.h"
#include "../common/logging.h"
#include "../common/win32_utils.h"
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
constexpr UINT_PTR kMotionTimerId = 2;
constexpr UINT_PTR kAnchorCheckTimerId = 3;
constexpr UINT_PTR kNoticeTimerId = 4;
constexpr UINT_PTR kFirstTaskTimerId = 100;
// A frame every 15 ms while the focus moves (for as long as the style's
// candidate_style_motion_ms) or new content animates in.
constexpr UINT kMotionFrameMs = 15;
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

// Windows' "Animation effects" setting (also off for Remote Desktop and
// some accessibility setups): when off, the popup changes without motion.
bool client_animations_enabled() noexcept {
    BOOL enabled = TRUE;
    return !SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &enabled, 0) || enabled != FALSE;
}

bool rect_empty(const RECT& rect) noexcept {
    return rect.right <= rect.left || rect.bottom <= rect.top;
}

// The settings app sits beside the input method DLL. It is single-instance,
// so a second launch only brings the open window forward.
void open_settings_app() {
    launch_settings_app();
}

}  // namespace

// Windows Store (immersive) processes: the shell cloaks unowned top-level
// windows, so the popup must be owned by the text input's window there.
bool immersive_process() noexcept {
    using IsImmersiveProcessFn = BOOL(WINAPI*)(HANDLE);
    static const auto function = reinterpret_cast<IsImmersiveProcessFn>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "IsImmersiveProcess"));
    static const bool immersive = function && function(GetCurrentProcess());
    return immersive;
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

bool high_contrast_enabled() noexcept {
    HIGHCONTRASTW contrast{};
    contrast.cbSize = sizeof(contrast);
    return SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0) &&
           (contrast.dwFlags & HCF_HIGHCONTRASTON);
}

bool launch_settings_app(const std::wstring& arguments) {
    const std::wstring path = install_file_path(g_instance, kSettingsExecutableName);
    const std::wstring directory = path.substr(0, path.find_last_of(L'\\') + 1);
    if (path.empty()) {
        log_status(L"candidate-ui", L"settings_open=false reason=settings_exe_not_found");
        return false;
    }
    std::wstring command_line = L"\"" + path + L"\"" + (arguments.empty() ? L"" : L" " + arguments);
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(path.c_str(), command_line.data(), nullptr, nullptr, FALSE, 0, nullptr,
                        directory.c_str(), &startup, &process)) {
        log_status(L"candidate-ui", L"settings_open=false create_process_last_error=" + std::to_wstring(GetLastError()));
        return false;
    }
    // The new process may bring its window to the front.
    AllowSetForegroundWindow(process.dwProcessId);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}

namespace {

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
        // The tool panel (an ordinary process) hands text over with
        // WM_COPYDATA, also into applications running as administrator;
        // the message carries the token of the panel opened from here.
        ChangeWindowMessageFilterEx(created_hwnd, WM_COPYDATA, MSGFLT_ALLOW, nullptr);
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
    motion_timer_ = 0;
    motion_active_ = false;
    snap_motion_ = true;
    anchor_check_timer_ = 0;
    anchor_check_callback_ = {};
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
    notice_mode_ = notice_request_;
    notice_request_ = false;
    if (!notice_mode_ && hwnd_) KillTimer(hwnd_, kNoticeTimerId);
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

void CandidateWindow::show_notice(const std::wstring& title, const std::wstring& body, const RECT& caret_rect,
                                  const CandidateWindowOwnerInfo& owner_info, UINT hide_after_ms) {
    prediction_mode_ = false;
    notice_request_ = true;
    show(title, {Candidate{body, 1, 0}}, 0, caret_rect, owner_info);
    if (hwnd_ && notice_mode_ && hide_after_ms) SetTimer(hwnd_, kNoticeTimerId, hide_after_ms, nullptr);
}

bool CandidateWindow::schedule_task(int slot, UINT delay_ms, std::function<void()> callback) {
    if (slot < 0 || slot >= kTaskSlots || !callback || !create()) return false;
    task_callbacks_[slot] = std::move(callback);
    if (!SetTimer(hwnd_, kFirstTaskTimerId + slot, delay_ms, nullptr)) {
        task_callbacks_[slot] = {};
        return false;
    }
    return true;
}

void CandidateWindow::cancel_task(int slot) {
    if (slot < 0 || slot >= kTaskSlots) return;
    if (hwnd_) KillTimer(hwnd_, kFirstTaskTimerId + slot);
    task_callbacks_[slot] = {};
}

HWND CandidateWindow::async_target() {
    create();
    return hwnd_;
}

void CandidateWindow::hide(const wchar_t*) {
    cancel_pointer_interaction();
    stop_motion();
    cancel_anchor_check();
    notice_mode_ = false;
    if (hwnd_) KillTimer(hwnd_, kNoticeTimerId);
    // The next popup starts fresh: no motion from stale positions, and
    // Adaptive Flow chooses its form anew.
    snap_motion_ = true;
    flow_form_memory_.reset();
    // The next popup turns in from blank.
    shown_texts_.clear();
    previous_texts_.clear();
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

bool CandidateWindow::schedule_anchor_check(std::function<void()> callback, UINT delay_ms) {
    if (!callback || !hwnd_) return false;
    anchor_check_callback_ = std::move(callback);
    anchor_check_timer_ = SetTimer(hwnd_, kAnchorCheckTimerId, delay_ms, nullptr);
    if (!anchor_check_timer_) anchor_check_callback_ = {};
    return anchor_check_timer_ != 0;
}

void CandidateWindow::cancel_anchor_check() {
    if (hwnd_ && anchor_check_timer_) KillTimer(hwnd_, kAnchorCheckTimerId);
    anchor_check_timer_ = 0;
    anchor_check_callback_ = {};
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
    if (message == async_message()) {
        if (async_callback_) async_callback_(wparam);
        return 0;
    }
    if (message == WM_COPYDATA) {
        const auto* data = reinterpret_cast<const COPYDATASTRUCT*>(lparam);
        return data && copydata_callback_ && copydata_callback_(*data) ? TRUE : FALSE;
    }
    if (message == candidate_options_changed_message()) {
        apply_options_changed();
        return 0;
    }
    switch (message) {
        case WM_TIMER:
            if (wparam == kMotionTimerId) {
                if (has_cached_show_state_ && IsWindowVisible(hwnd_)) present();
                else stop_motion();
                return 0;
            }
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
            if (wparam >= kFirstTaskTimerId && wparam < kFirstTaskTimerId + kTaskSlots) {
                KillTimer(hwnd_, wparam);
                auto callback = std::move(task_callbacks_[wparam - kFirstTaskTimerId]);
                task_callbacks_[wparam - kFirstTaskTimerId] = {};
                if (callback) callback();
                return 0;
            }
            if (wparam == kNoticeTimerId) {
                KillTimer(hwnd_, kNoticeTimerId);
                if (notice_mode_) hide(L"notice_done");
                return 0;
            }
            if (wparam == kAnchorCheckTimerId && anchor_check_timer_) {
                KillTimer(hwnd_, kAnchorCheckTimerId);
                anchor_check_timer_ = 0;
                auto callback = std::move(anchor_check_callback_);
                anchor_check_callback_ = {};
                if (callback) callback();
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
            stop_motion();
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
    palette_ = high_contrast_enabled()
        ? make_high_contrast_candidate_palette()
        : make_candidate_palette(options_.theme_mode, windows_apps_use_dark_theme(), options_.style);
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

CandidateLayoutInput CandidateWindow::layout_input(int dpi, const RECT& work_area) {
    CandidateLayoutInput input;
    input.composition_text = query_text();
    input.candidate_texts = candidate_texts_;
    input.selected_index = selected_index_;
    input.page_indices = page_indices_;
    input.caret_rect = caret_rect_;
    input.work_area = work_area;
    input.dpi = dpi;
    input.options = options_;
    // Space commits the current choice; Tab accepts a suggestion.
    focus_hint_ = notice_mode_ ? L"" : prediction_mode_ ? L"Tab" : L"\u7A7A\u683C";
    input.focus_hint = focus_hint_;
    // Each style measures in the sizes, weights and faces it draws with.
    input.measure = [this](const std::wstring& text, int font_px, int weight, CandidateFontFace face) {
        return renderer_.measure_text(text, static_cast<float>(font_px), weight, candidate_font_family(face));
    };
    input.previous_flow_form = flow_form_memory_;
    // A Store app sandbox cannot start the settings app.
    input.settings_button = !immersive_process();
    return input;
}

bool CandidateWindow::rebuild_layout(int dpi) {
    cancel_pointer_interaction();
    update_fonts(dpi);
    layout_ = make_candidate_layout(layout_input(dpi, monitor_work_area_for(caret_rect_)));
    if (layout_.visible && layout_.style == CandidateStyle::AdaptiveFlow) flow_form_memory_ = layout_.flow_form;
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
    const std::wstring query = query_text();
    std::wstring text = query;
    if (available_px <= 0 || renderer_.measure_text(text, size) <= available_px) return text;
    size_t drop = 1;
    while (drop < query.size()) {
        text = kEllipsis + query.substr(drop);
        if (renderer_.measure_text(text, size) <= available_px) break;
        drop += std::max<size_t>(1, (query.size() - drop) / 8);
    }
    return text;
}

void CandidateWindow::paint_content() {
    auto* canvas = active_modern_renderer();
    if (!canvas) return;
    CandidatePaintState state;
    state.hovered = candidate_pointer_.hovered();
    state.pressed = candidate_pointer_.pressed();
    state.hot_page_direction = hot_page_direction_;
    state.pressed_page_direction = pressed_page_direction_;
    state.settings_hot = settings_hot_;
    state.settings_pressed = settings_pressed_;
    const RECT& label = layout_.composition_rect;
    const int caret_space = (std::max)(4, scale_dip(4, layout_.dpi));
    state.query = fitted_composition_text(label.right - label.left - caret_space);
    state.show_caret = !prediction_mode_ && !notice_mode_;
    state.focus_hint = focus_hint_;
    state.marker = marker_motion_.current;
    state.marker_from = marker_motion_.from;
    state.marker_to = marker_motion_.to;
    state.marker_progress = marker_progress_;
    if (motion_frozen_) {
        state.marker_from = motion_frozen_->first;
        state.marker_progress = motion_frozen_->second;
        state.marker = candidate_marker_frame(layout_.style, state.marker_from, state.marker_to, state.marker_progress);
    }
    state.track_marker = track_motion_.current;
    if (reveal_frozen_ms_) {
        state.reveal_ms = *reveal_frozen_ms_;
        state.previous_texts = previous_texts_;
    } else if (reveal_active_) {
        state.reveal_ms = GetTickCount64() - reveal_start_;
        state.previous_texts = previous_texts_;
    }
    paint_candidate_window(CandidatePaintContext{*canvas, layout_, palette_, state});
}

void CandidateWindow::update_motion(bool snap) {
    const RECT marker = candidate_marker_target(layout_, candidate_pointer_.hovered());
    const RECT track = candidate_track_target(layout_);
    const bool instant = snap || snap_motion_ || palette_.high_contrast || !client_animations_enabled();
    snap_motion_ = false;
    const auto retarget = [instant](Motion& motion, const RECT& target) {
        if (instant || !motion.valid || rect_empty(motion.current) || rect_empty(target)) {
            motion.from = motion.to = motion.current = target;
            motion.valid = true;
            return false;
        }
        if (EqualRect(&motion.to, &target)) return false;
        motion.from = motion.current;
        motion.to = target;
        return true;
    };
    const bool marker_moved = retarget(marker_motion_, marker);
    const bool track_moved = retarget(track_motion_, track);
    const ULONGLONG now = GetTickCount64();
    // New content starts the reveal clock for the styles that animate it,
    // also on the popup's first frame (everything turns in from blank).
    // Offline capture (snap), high contrast and "animations off" stay at rest.
    std::vector<std::wstring> texts;
    texts.reserve(layout_.items.size());
    for (const auto& item : layout_.items) texts.push_back(item.display_text);
    if (texts != shown_texts_) {
        previous_texts_ = std::move(shown_texts_);
        shown_texts_ = std::move(texts);
        const bool reveal = candidate_style_reveals(layout_.style) && !snap && !palette_.high_contrast &&
                            client_animations_enabled();
        reveal_active_ = reveal;
        reveal_start_ = now;
    }
    if (reveal_active_ && (snap || now - reveal_start_ >= kRevealSpanMs)) reveal_active_ = false;
    if (marker_moved || track_moved) {
        motion_start_ = now;
        motion_active_ = true;
    }
    if (motion_active_ && !instant) {
        const float t = (std::min)(1.0f, static_cast<float>(now - motion_start_) /
                                             static_cast<float>(candidate_style_motion_ms(layout_.style)));
        marker_motion_.current = candidate_marker_frame(layout_.style, marker_motion_.from, marker_motion_.to, t);
        track_motion_.current = candidate_marker_frame(CandidateStyle::CursorRail, track_motion_.from, track_motion_.to, t);
        marker_progress_ = t;
        if (t >= 1.0f) motion_active_ = false;
    } else {
        motion_active_ = false;
        marker_motion_.current = marker_motion_.to;
        track_motion_.current = track_motion_.to;
        marker_progress_ = 1.0f;
    }
    if ((motion_active_ || reveal_active_) && hwnd_ && !motion_timer_)
        motion_timer_ = SetTimer(hwnd_, kMotionTimerId, kMotionFrameMs, nullptr);
    if (!motion_active_ && !reveal_active_) stop_motion();
}

void CandidateWindow::stop_motion() noexcept {
    if (motion_timer_ && hwnd_) KillTimer(hwnd_, kMotionTimerId);
    motion_timer_ = 0;
    motion_active_ = false;
    reveal_active_ = false;
}

void CandidateWindow::composite_shadow(std::uint8_t* pixels, int width, int height) {
    if (layout_.shadow_margin_px <= 0 || (palette_.shadow_key_alpha <= 0 && palette_.shadow_ambient_alpha <= 0)) return;
    // Two blurred panel silhouettes beneath the drawn panel ("destination
    // over"): a tight key shadow that defines the edge and a wide, lower
    // ambient one that lifts the popup off the page. Floating Glass floats
    // higher: its ambient shadow drops further and spreads wider.
    const bool floating = layout_.style == CandidateStyle::FloatingGlass;
    const RECT& panel = layout_.panel_rect;
    const std::array<int, 11> key{width, height, static_cast<int>(panel.left), static_cast<int>(panel.top),
                                  static_cast<int>(panel.right), static_cast<int>(panel.bottom), layout_.corner_radius,
                                  palette_.shadow_key_alpha, palette_.shadow_ambient_alpha, layout_.dpi, floating ? 1 : 0};
    if (key != shadow_key_ || shadow_cache_.size() != static_cast<size_t>(width) * height) {
        const float radius = static_cast<float>(layout_.corner_radius);
        const auto silhouette = [&](int offset, int blur) {
            RECT casting = panel;
            OffsetRect(&casting, 0, offset);
            std::vector<float> mask(static_cast<size_t>(width) * height);
            for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x)
                    mask[static_cast<size_t>(y) * width + x] = rounded_rect_coverage(x + 0.5f, y + 0.5f, casting, radius);
            box_blur(mask, width, height, blur);
            return mask;
        };
        const int unit = std::max(1, scale_dip(1, layout_.dpi));
        const auto key_shadow = silhouette(unit, unit);
        const auto ambient = silhouette(std::max(1, scale_dip(floating ? 6 : 4, layout_.dpi)),
                                        std::max(1, layout_.shadow_margin_px / (floating ? 3 : 4)));
        shadow_cache_.resize(key_shadow.size());
        for (size_t index = 0; index < key_shadow.size(); ++index) {
            const float k = key_shadow[index] * palette_.shadow_key_alpha / 255.0f;
            const float a = ambient[index] * palette_.shadow_ambient_alpha / 255.0f;
            shadow_cache_[index] = 1.0f - (1.0f - k) * (1.0f - a);
        }
        shadow_key_ = key;
    }
    for (size_t index = 0; index < shadow_cache_.size(); ++index) {
        std::uint8_t* pixel = pixels + index * 4;
        const float remaining = 1.0f - pixel[3] / 255.0f;
        // Black shadow: premultiplied color stays, only alpha accumulates.
        pixel[3] = static_cast<std::uint8_t>(std::clamp(pixel[3] + shadow_cache_[index] * 255.0f * remaining + 0.5f,
                                                        0.0f, 255.0f));
    }
}

void CandidateWindow::render_layered(HDC dc, std::uint8_t* pixels, int width, int height) {
    std::memset(pixels, 0, static_cast<size_t>(width) * height * 4);
    const RECT bounds{0, 0, width, height};
    {
        ModernRenderScope modern(renderer_, dc, bounds, true);
        if (modern.active()) {
            paint_content();
            modern.finish();
            GdiFlush();
            composite_shadow(pixels, width, height);
            return;
        }
    }
    // GDI fallback: an opaque, square-cornered panel without shadow.
    const auto fill_rect = [dc](const RECT& rect, COLORREF color) {
        HBRUSH brush = CreateSolidBrush(color);
        FillRect(dc, &rect, brush);
        DeleteObject(brush);
    };
    fill_rect(layout_.panel_rect, palette_.background);
    HBRUSH border = CreateSolidBrush(palette_.border);
    FrameRect(dc, &layout_.panel_rect, border);
    DeleteObject(border);
    SetBkMode(dc, TRANSPARENT);
    if (composition_font_) SelectObject(dc, composition_font_);
    SetTextColor(dc, palette_.composition_text);
    RECT label = layout_.composition_rect;
    const std::wstring query = query_text();
    DrawTextW(dc, query.c_str(), static_cast<int>(query.size()), &label,
              DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    if (candidate_font_) SelectObject(dc, candidate_font_);
    for (const auto& item : layout_.items) {
        fill_rect(item.bounds, item.featured ? palette_.focus_background : palette_.chip_background);
        fill_rect(item.number_bounds, item.featured ? palette_.focus_keycap : palette_.keycap);
        const std::wstring number = std::to_wstring(item.page_number);
        RECT number_rect = item.number_bounds;
        SetTextColor(dc, item.featured ? palette_.focus_number_text : palette_.number_text);
        DrawTextW(dc, number.c_str(), static_cast<int>(number.size()), &number_rect,
                  DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
        RECT text_rect = item.text_bounds;
        SetTextColor(dc, palette_.text);
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
    update_motion(false);
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
    // destination with its alpha, focus marker at rest.
    if (!hdc || !layout_.visible || layout_.width <= 0 || layout_.height <= 0) return;
    update_motion(true);
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
