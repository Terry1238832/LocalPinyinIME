#include "language_bar.h"

#include "guids.h"
#include "resource.h"
#include "../common/logging.h"
#include "../common/win32_utils.h"

#include <ctffunc.h>
#include <oleauto.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cwchar>
#include <sstream>
#include <utility>

namespace localpinyin {
namespace {

const wchar_t* icon_variant(InputMode mode) noexcept {
    return mode == InputMode::Chinese ? L"zh" : L"en";
}

const wchar_t* input_mode_log_text(InputMode mode) noexcept {
    return mode == InputMode::Chinese ? L"Chinese" : L"English";
}

const wchar_t* click_text(TfLBIClick click) noexcept {
    switch (click) {
    case TF_LBI_CLK_LEFT:
        return L"left";
    case TF_LBI_CLK_RIGHT:
        return L"right";
    default:
        return L"unknown";
    }
}

int mode_icon_resource_id(InputMode mode) noexcept {
    return mode == InputMode::Chinese ? IDI_MODE_ZH : IDI_MODE_EN;
}

HMODULE language_bar_module() noexcept {
    HMODULE module = nullptr;
    const BOOL ok = GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                       reinterpret_cast<LPCWSTR>(&language_bar_module),
                                       &module);
    return ok ? module : nullptr;
}

HICON load_mode_icon(InputMode mode, int* width, int* height, DWORD* last_error) noexcept {
    if (last_error) {
        *last_error = ERROR_SUCCESS;
    }
    const int cx = mode_icon_pixel_size();
    const int cy = cx;
    if (width) {
        *width = cx;
    }
    if (height) {
        *height = cy;
    }
    if (HICON rendered = render_mode_icon(mode, cx, mode_icon_color())) return rendered;

    HMODULE module = language_bar_module();
    if (!module) {
        if (last_error) {
            *last_error = GetLastError();
        }
        return nullptr;
    }

    const int resource_id = mode_icon_resource_id(mode);
    auto* resource_name = MAKEINTRESOURCEW(resource_id);
    auto icon = reinterpret_cast<HICON>(
        LoadImageW(module, resource_name, IMAGE_ICON, cx, cy, 0));
    if (!icon && last_error) {
        *last_error = GetLastError();
    }
    return icon;
}

bool taskbar_uses_light_theme() noexcept {
    DWORD value = 0;
    DWORD size = sizeof(value);
    return RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                        L"SystemUsesLightTheme", RRF_RT_REG_DWORD, nullptr, &value, &size) == ERROR_SUCCESS &&
           value != 0;
}

std::wstring update_flags_text(DWORD flags) {
    std::wstringstream stream;
    stream << L"on_update_flags=0x" << std::hex << flags;
    return stream.str();
}

enum LangBarMenuId : UINT {
    kMenuToggleInputMode = 1001,
    kMenuOpenSettings = 1002,
};

constexpr DWORD kModeUpdateFlags = TF_LBI_ICON | TF_LBI_TEXT | TF_LBI_STATUS | TF_LBI_TOOLTIP;
constexpr unsigned kMaxInitialUpdateRetries = 3;
constexpr wchar_t kPopupOwnerClassName[] = L"LocalPinyinIME_ModeIconPopupOwner";

LRESULT CALLBACK popup_owner_wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

bool register_popup_owner_window_class() {
    static bool attempted = false;
    static bool registered = false;
    if (attempted) {
        return registered;
    }
    attempted = true;
    WNDCLASSW window_class{};
    window_class.lpfnWndProc = popup_owner_wnd_proc;
    window_class.hInstance = language_bar_module();
    window_class.lpszClassName = kPopupOwnerClassName;
    if (!window_class.hInstance) {
        return false;
    }
    if (RegisterClassW(&window_class) || GetLastError() == ERROR_CLASS_ALREADY_EXISTS) {
        registered = true;
    }
    return registered;
}

const wchar_t* popup_command_text(ModeIconPopupCommand command) noexcept {
    switch (command) {
    case ModeIconPopupCommand::Toggle:
        return L"toggle";
    case ModeIconPopupCommand::Settings:
        return L"settings";
    case ModeIconPopupCommand::Error:
        return L"error";
    case ModeIconPopupCommand::Cancel:
    default:
        return L"cancel";
    }
}

}  // namespace

int mode_icon_pixel_size() noexcept {
    // Query metrics as a per-monitor aware thread so a DPI virtualized host
    // still receives an icon at the taskbar's real pixel size.
    using SetContextFn = DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT);
    using DpiForSystemFn = UINT(WINAPI*)();
    using MetricsForDpiFn = int(WINAPI*)(int, UINT);
    static const HMODULE user32 = GetModuleHandleW(L"user32.dll");
    static const auto set_context = user32 ? reinterpret_cast<SetContextFn>(GetProcAddress(user32, "SetThreadDpiAwarenessContext")) : nullptr;
    static const auto dpi_for_system = user32 ? reinterpret_cast<DpiForSystemFn>(GetProcAddress(user32, "GetDpiForSystem")) : nullptr;
    static const auto metrics_for_dpi = user32 ? reinterpret_cast<MetricsForDpiFn>(GetProcAddress(user32, "GetSystemMetricsForDpi")) : nullptr;
    int size = GetSystemMetrics(SM_CXSMICON);
    if (set_context && dpi_for_system && metrics_for_dpi) {
        const auto previous = set_context(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        if (previous) {
            size = metrics_for_dpi(SM_CXSMICON, dpi_for_system());
            set_context(previous);
        }
    }
    return std::clamp(size, 16, 64);
}

COLORREF mode_icon_color() noexcept {
    HIGHCONTRASTW contrast{sizeof(contrast)};
    if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0) &&
        (contrast.dwFlags & HCF_HIGHCONTRASTON)) return GetSysColor(COLOR_BTNTEXT);
    return taskbar_uses_light_theme() ? RGB(26, 26, 26) : RGB(255, 255, 255);
}

HICON render_mode_icon(InputMode mode, int size, COLORREF color) noexcept {
    if (size < 8 || size > 256) return nullptr;
    const wchar_t glyph = mode == InputMode::Chinese ? L'\u4E2D' : L'\u82F1';
    // Draw white on black with grayscale antialiasing into an oversized
    // canvas, then center the glyph's ink so it sits optically centered.
    const int canvas = size * 2;
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = canvas;
    info.bmiHeader.biHeight = -canvas;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    HDC dc = CreateCompatibleDC(nullptr);
    if (!dc) return nullptr;
    void* raw = nullptr;
    HBITMAP scratch = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &raw, nullptr, 0);
    if (!scratch || !raw) {
        if (scratch) DeleteObject(scratch);
        DeleteDC(dc);
        return nullptr;
    }
    const HGDIOBJ previous_bitmap = SelectObject(dc, scratch);
    std::fill_n(static_cast<std::uint32_t*>(raw), static_cast<size_t>(canvas) * canvas, 0u);
    // The em box of a CJK face is a little larger than its ink. At taskbar
    // sizes the face's hinted outlines without antialiasing put every stroke
    // on whole pixels (as the system's own indicator does); larger icons use
    // smooth grayscale strokes.
    const bool hinted = size <= 20;
    const int em = std::max(8, static_cast<int>(std::lround(size * 1.02)));
    HFONT font = CreateFontW(-em, 0, 0, 0, hinted ? FW_NORMAL : FW_SEMIBOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                             hinted ? NONANTIALIASED_QUALITY : ANTIALIASED_QUALITY,
                             DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    const HGDIOBJ previous_font = font ? SelectObject(dc, font) : nullptr;
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    RECT area{0, 0, canvas, canvas};
    DrawTextW(dc, &glyph, 1, &area, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    GdiFlush();
    if (previous_font) SelectObject(dc, previous_font);
    if (font) DeleteObject(font);
    SelectObject(dc, previous_bitmap);
    DeleteDC(dc);

    const auto* source = static_cast<const std::uint8_t*>(raw);
    int left = canvas, top = canvas, right = -1, bottom = -1;
    for (int y = 0; y < canvas; ++y) {
        for (int x = 0; x < canvas; ++x) {
            if (source[(static_cast<size_t>(y) * canvas + x) * 4 + 1] > 8) {
                left = std::min(left, x); right = std::max(right, x);
                top = std::min(top, y); bottom = std::max(bottom, y);
            }
        }
    }
    HICON icon = nullptr;
    if (right >= left && bottom >= top) {
        const int offset_x = (size - (right - left + 1)) / 2 - left;
        const int offset_y = (size - (bottom - top + 1)) / 2 - top;
        info.bmiHeader.biWidth = size;
        info.bmiHeader.biHeight = -size;
        void* pixels = nullptr;
        HBITMAP color_bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
        HBITMAP mask = CreateBitmap(size, size, 1, 1, nullptr);
        if (color_bitmap && pixels && mask) {
            auto* target = static_cast<std::uint8_t*>(pixels);
            for (int y = 0; y < size; ++y) {
                for (int x = 0; x < size; ++x) {
                    const int sx = x - offset_x, sy = y - offset_y;
                    const std::uint8_t coverage = sx >= 0 && sy >= 0 && sx < canvas && sy < canvas
                        ? source[(static_cast<size_t>(sy) * canvas + sx) * 4 + 1] : 0;
                    std::uint8_t* pixel = target + (static_cast<size_t>(y) * size + x) * 4;
                    // Icons carry straight (not premultiplied) alpha.
                    pixel[0] = GetBValue(color);
                    pixel[1] = GetGValue(color);
                    pixel[2] = GetRValue(color);
                    pixel[3] = coverage;
                }
            }
            HDC mask_dc = CreateCompatibleDC(nullptr);
            if (mask_dc) {
                const HGDIOBJ previous_mask = SelectObject(mask_dc, mask);
                PatBlt(mask_dc, 0, 0, size, size, BLACKNESS);
                SelectObject(mask_dc, previous_mask);
                DeleteDC(mask_dc);
            }
            ICONINFO icon_info{TRUE, 0, 0, mask, color_bitmap};
            icon = CreateIconIndirect(&icon_info);
        }
        if (color_bitmap) DeleteObject(color_bitmap);
        if (mask) DeleteObject(mask);
    }
    DeleteObject(scratch);
    return icon;
}

const wchar_t* input_mode_label(InputMode mode) noexcept {
    return mode == InputMode::Chinese ? L"\u4E2D" : L"\u82F1";
}

const wchar_t* input_mode_tooltip(InputMode mode) noexcept {
    return mode == InputMode::Chinese
        ? L"LocalPinyinIME \u4E2D\u6587\u6A21\u5F0F"
        : L"LocalPinyinIME \u82F1\u6587\u76F4\u8F93\u6A21\u5F0F";
}

LanguageBarState::~LanguageBarState() {
    unregister_item();
    destroy_popup_owner_window();
}

void LanguageBarState::set_mode(InputMode mode) noexcept {
    if (mode_ == mode) {
        return;
    }
    const InputMode old_mode = mode_;
    mode_ = mode;
    logged_get_icon_chinese_ = false;
    logged_get_icon_english_ = false;
    log_status(L"language-bar",
               std::wstring(L"event=set_mode old=") + input_mode_log_text(old_mode) +
                   L" new=" + input_mode_log_text(mode_));
    notify_mode_changed();
}

void LanguageBarState::set_toggle_callback(std::function<HRESULT(const wchar_t*)> callback) {
    toggle_callback_ = std::move(callback);
}

void LanguageBarState::set_settings_callback(std::function<HRESULT(const wchar_t*)> callback) {
    settings_callback_ = std::move(callback);
}

void LanguageBarState::set_popup_menu_callback(std::function<ModeIconPopupCommand()> callback) {
    popup_menu_callback_ = std::move(callback);
}

void LanguageBarState::clear_callbacks() {
    toggle_callback_ = nullptr;
    settings_callback_ = nullptr;
    popup_menu_callback_ = nullptr;
}

HRESULT LanguageBarState::register_item(ITfThreadMgr* thread_mgr) {
    if (item_added_) {
        return S_OK;
    }
    if (!thread_mgr) {
        return E_POINTER;
    }
    ITfLangBarItemMgr* mgr = nullptr;
    HRESULT hr = thread_mgr->QueryInterface(IID_PPV_ARGS(&mgr));
    if (FAILED(hr)) {
        return hr;
    }
    hr = mgr->AddItem(static_cast<ITfLangBarItem*>(this));
    if (SUCCEEDED(hr)) {
        item_mgr_ = mgr;
        item_added_ = true;
        log_status(L"language-bar", L"event=language_bar_item_add_result hr=0x00000000");
        return S_OK;
    }
    log_status(L"language-bar", L"event=language_bar_item_add_result hr=" + hresult_hex(hr));
    mgr->Release();
    return hr;
}

HRESULT LanguageBarState::unregister_item() {
    HRESULT hr = S_OK;
    destroy_popup_owner_window();
    if (sink_) {
        sink_->Release();
        sink_ = nullptr;
        sink_cookie_ = TF_INVALID_COOKIE;
        pending_initial_update_ = false;
        initial_update_retry_count_ = 0;
    }
    if (item_mgr_) {
        if (item_added_) {
            hr = item_mgr_->RemoveItem(static_cast<ITfLangBarItem*>(this));
        }
        item_mgr_->Release();
        item_mgr_ = nullptr;
        item_added_ = false;
    }
    return hr;
}

void LanguageBarState::notify_mode_changed() {
    send_update(kModeUpdateFlags, L"mode_changed", true);
}

void LanguageBarState::refresh_icon() {
    logged_get_icon_chinese_ = false;
    logged_get_icon_english_ = false;
    send_update(TF_LBI_ICON, L"theme_changed", false);
}

void LanguageBarState::publish_initial_mode() {
    log_status(L"language-bar", L"event=language_bar_publish_initial_mode");
    if (pending_initial_update_) {
        retry_pending_initial_update(L"publish_initial_mode");
        return;
    }
    send_update(kModeUpdateFlags, L"publish_initial_mode", true);
}

HRESULT LanguageBarState::send_update(DWORD flags, const wchar_t* reason, bool defer_on_failure) {
    log_status(L"language-bar",
               std::wstring(L"event=notify_mode_changed reason=") +
                   (reason ? reason : L"unknown") + L" " + update_flags_text(flags) +
                   L" sink_available=" + (sink_ ? L"true" : L"false"));
    if (!sink_) {
        return S_FALSE;
    }
    const HRESULT hr = sink_->OnUpdate(flags);
    if (SUCCEEDED(hr)) {
        if (pending_initial_update_) {
            pending_initial_update_ = false;
            initial_update_retry_count_ = 0;
            log_status(L"language-bar", L"event=language_bar_initial_update_retry result=S_OK reason=notify_success");
        }
        return hr;
    }
    if (defer_on_failure && !pending_initial_update_) {
        pending_initial_update_ = true;
        initial_update_retry_count_ = 0;
        log_status(L"language-bar",
                   L"event=language_bar_initial_update_deferred reason=on_update_failed hr=" +
                       hresult_hex(hr));
    }
    return hr;
}

HRESULT LanguageBarState::retry_pending_initial_update(const wchar_t* reason) {
    if (!pending_initial_update_) {
        return S_FALSE;
    }
    if (!sink_) {
        log_status(L"language-bar",
                   std::wstring(L"event=language_bar_initial_update_retry result=no_sink reason=") +
                       (reason ? reason : L"unknown"));
        return S_FALSE;
    }
    if (initial_update_retry_count_ >= kMaxInitialUpdateRetries) {
        log_status(L"language-bar",
                   std::wstring(L"event=language_bar_initial_update_retry result=skipped max_retries=") +
                       std::to_wstring(kMaxInitialUpdateRetries) + L" reason=" +
                       (reason ? reason : L"unknown"));
        return HRESULT_FROM_WIN32(ERROR_RETRY);
    }
    ++initial_update_retry_count_;
    const HRESULT hr = sink_->OnUpdate(kModeUpdateFlags);
    log_status(L"language-bar",
               std::wstring(L"event=language_bar_initial_update_retry attempt=") +
                   std::to_wstring(initial_update_retry_count_) + L" " +
                   update_flags_text(kModeUpdateFlags) + L" reason=" +
                   (reason ? reason : L"unknown") + L" result=" +
                   (SUCCEEDED(hr) ? std::wstring(L"S_OK") : hresult_hex(hr)));
    if (SUCCEEDED(hr)) {
        pending_initial_update_ = false;
        initial_update_retry_count_ = 0;
    }
    return hr;
}

HRESULT LanguageBarState::QueryInterface(REFIID riid, void** object) {
    if (!object) {
        return E_POINTER;
    }
    *object = nullptr;
    if (riid == IID_IUnknown || riid == __uuidof(ITfLangBarItem) || riid == __uuidof(ITfLangBarItemButton)) {
        *object = static_cast<ITfLangBarItemButton*>(this);
    } else if (riid == __uuidof(ITfSource)) {
        *object = static_cast<ITfSource*>(this);
    } else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG LanguageBarState::AddRef() {
    return static_cast<ULONG>(InterlockedIncrement(&ref_count_));
}

ULONG LanguageBarState::Release() {
    const long count = InterlockedDecrement(&ref_count_);
    return static_cast<ULONG>(count);
}

HRESULT LanguageBarState::GetInfo(TF_LANGBARITEMINFO* info) {
    if (!info) {
        return E_POINTER;
    }
    *info = {};
    info->clsidService = CLSID_LocalPinyinTextService;
    info->guidItem = GUID_LBI_INPUTMODE;
    info->dwStyle = TF_LBI_STYLE_SHOWNINTRAY |
                    TF_LBI_STYLE_TEXTCOLORICON |
                    TF_LBI_STYLE_BTN_BUTTON |
                    TF_LBI_STYLE_BTN_MENU |
                    TF_LBI_STYLE_BTN_TOGGLE;
    info->ulSort = 0;
    constexpr wchar_t description[] = L"LocalPinyinIME";
    wcsncpy_s(info->szDescription, description, _TRUNCATE);
    return S_OK;
}

HRESULT LanguageBarState::GetStatus(DWORD* status) {
    if (!status) {
        return E_POINTER;
    }
    *status = visible_ ? 0 : TF_LBI_STATUS_HIDDEN;
    if (mode_ == InputMode::English) {
        *status |= TF_LBI_STATUS_BTN_TOGGLED;
    }
    return S_OK;
}

HRESULT LanguageBarState::Show(BOOL show) {
    visible_ = show != FALSE;
    notify_mode_changed();
    return S_OK;
}

HRESULT LanguageBarState::GetTooltipString(BSTR* tooltip) {
    if (!tooltip) {
        return E_POINTER;
    }
    *tooltip = SysAllocString(input_mode_tooltip(mode_));
    return *tooltip ? S_OK : E_OUTOFMEMORY;
}

HRESULT LanguageBarState::OnClick(TfLBIClick click, POINT, const RECT*) {
    log_status(L"language-bar",
               std::wstring(L"event=language_bar_on_click click=") + click_text(click));
    if (click == TF_LBI_CLK_RIGHT) {
        log_status(L"language-bar", L"event=mode_icon_popup_menu_show requested=true");
        const ModeIconPopupCommand command =
            popup_menu_callback_ ? popup_menu_callback_() : show_mode_icon_context_menu();
        log_status(L"language-bar",
                   std::wstring(L"event=mode_icon_popup_menu_result command=") +
                       popup_command_text(command));
        if (command == ModeIconPopupCommand::Toggle) {
            return handle_toggle_request(L"popup_menu");
        }
        if (command == ModeIconPopupCommand::Settings) {
            return handle_settings_request(L"popup_menu");
        }
        return command == ModeIconPopupCommand::Error ? E_FAIL : S_OK;
    }
    if (click != TF_LBI_CLK_LEFT) {
        return S_OK;
    }
    return handle_toggle_request(L"click");
}

HRESULT LanguageBarState::InitMenu(ITfMenu* menu) {
    log_status(L"language-bar", L"event=language_bar_init_menu called=true");
    if (!menu) {
        return E_POINTER;
    }
    const wchar_t toggle_text[] = L"Switch Chinese/English";
    HRESULT hr = menu->AddMenuItem(kMenuToggleInputMode,
                                   mode_ == InputMode::English ? TF_LBMENUF_CHECKED : 0,
                                   nullptr,
                                   nullptr,
                                   toggle_text,
                                   ARRAYSIZE(toggle_text) - 1,
                                   nullptr);
    if (FAILED(hr)) {
        log_status(L"language-bar", L"event=language_bar_menu_add item=toggle result=" + hresult_hex(hr));
        return hr;
    }
    log_status(L"language-bar", L"event=language_bar_menu_add item=toggle result=S_OK");
    const wchar_t settings_text[] = L"Open Settings";
    hr = menu->AddMenuItem(kMenuOpenSettings,
                           0,
                           nullptr,
                           nullptr,
                           settings_text,
                           ARRAYSIZE(settings_text) - 1,
                           nullptr);
    log_status(L"language-bar",
               L"event=language_bar_menu_add item=settings result=" +
                   (SUCCEEDED(hr) ? std::wstring(L"S_OK") : hresult_hex(hr)));
    return hr;
}

HRESULT LanguageBarState::OnMenuSelect(UINT id) {
    log_status(L"language-bar", L"event=language_bar_menu_select id=" + std::to_wstring(id));
    if (id == kMenuToggleInputMode) {
        return handle_toggle_request(L"menu");
    }
    if (id == kMenuOpenSettings) {
        return handle_settings_request(L"menu");
    }
    return S_OK;
}

HRESULT LanguageBarState::GetIcon(HICON* icon) {
    if (!icon) {
        return E_POINTER;
    }
    // LoadImage without LR_SHARED returns an HICON owned by the caller, matching
    // ITfLangBarItemButton::GetIcon's handoff semantics.
    int width = 0;
    int height = 0;
    DWORD last_error = ERROR_SUCCESS;
    *icon = load_mode_icon(mode_, &width, &height, &last_error);
    const bool succeeded = *icon != nullptr;
    bool& logged = mode_ == InputMode::Chinese ? logged_get_icon_chinese_ : logged_get_icon_english_;
    if (!logged || !succeeded) {
        log_status(L"language-bar",
                   std::wstring(L"event=language_bar_get_icon mode=") +
                       input_mode_log_text(mode_) +
                       L" icon_variant=" + icon_variant(mode_) +
                       L" source=rendered_or_resource resource_id=" +
                       std::to_wstring(mode_icon_resource_id(mode_)) +
                       L" size=" + std::to_wstring(width) + L"x" + std::to_wstring(height) +
                       L" result=" + (succeeded ? L"S_OK" : L"E_FAIL") +
                       (succeeded ? std::wstring() : L" error=" + std::to_wstring(last_error)));
        logged = succeeded;
    }
    if (succeeded && pending_initial_update_) {
        retry_pending_initial_update(L"get_icon_success");
    }
    return succeeded ? S_OK : E_FAIL;
}

HRESULT LanguageBarState::GetText(BSTR* text) {
    if (!text) {
        return E_POINTER;
    }
    *text = SysAllocString(input_mode_label(mode_));
    return *text ? S_OK : E_OUTOFMEMORY;
}

HRESULT LanguageBarState::AdviseSink(REFIID riid, IUnknown* unknown, DWORD* cookie) {
    if (!unknown || !cookie) {
        return E_POINTER;
    }
    if (riid != __uuidof(ITfLangBarItemSink)) {
        return E_NOINTERFACE;
    }
    ITfLangBarItemSink* sink = nullptr;
    HRESULT hr = unknown->QueryInterface(IID_PPV_ARGS(&sink));
    if (FAILED(hr)) {
        return hr;
    }
    if (sink_) {
        sink_->Release();
    }
    sink_ = sink;
    sink_cookie_ = 1;
    *cookie = sink_cookie_;
    const HRESULT update_hr = sink_->OnUpdate(kModeUpdateFlags);
    if (FAILED(update_hr)) {
        pending_initial_update_ = true;
        initial_update_retry_count_ = 0;
    }
    log_status(L"language-bar",
               std::wstring(L"event=language_bar_advise_sink sink_available=true initial_update_sent=") +
                   (SUCCEEDED(update_hr) ? L"true" : L"false") +
                   L" " + update_flags_text(kModeUpdateFlags) +
                   L" result=" + (SUCCEEDED(update_hr) ? std::wstring(L"S_OK") : hresult_hex(update_hr)));
    if (FAILED(update_hr)) {
        log_status(L"language-bar",
                   L"event=language_bar_initial_update_deferred reason=on_update_failed hr=" +
                       hresult_hex(update_hr));
    }
    return S_OK;
}

HRESULT LanguageBarState::UnadviseSink(DWORD cookie) {
    if (cookie != sink_cookie_ || !sink_) {
        return E_INVALIDARG;
    }
    sink_->Release();
    sink_ = nullptr;
    sink_cookie_ = TF_INVALID_COOKIE;
    pending_initial_update_ = false;
    initial_update_retry_count_ = 0;
    return S_OK;
}

HRESULT LanguageBarState::handle_toggle_request(const wchar_t* source) {
    if (!toggle_callback_) {
        log_status(L"language-bar",
                   std::wstring(L"event=mode_icon_toggle_requested source=") +
                       (source ? source : L"unknown") + L" result=callback_missing");
        return E_NOTIMPL;
    }
    log_status(L"language-bar",
               std::wstring(L"event=mode_icon_toggle_requested source=") +
                   (source ? source : L"unknown"));
    const HRESULT hr = toggle_callback_(source);
    log_status(L"language-bar",
               std::wstring(L"event=mode_icon_toggle_result source=") +
                   (source ? source : L"unknown") + L" result=" + hresult_hex(hr));
    return hr;
}

HRESULT LanguageBarState::handle_settings_request(const wchar_t* source) {
    if (!settings_callback_) {
        log_status(L"language-bar",
                   std::wstring(L"event=mode_icon_settings_requested source=") +
                       (source ? source : L"unknown") + L" result=callback_missing");
        return E_NOTIMPL;
    }
    log_status(L"language-bar",
               std::wstring(L"event=mode_icon_settings_requested source=") +
                   (source ? source : L"unknown"));
    const HRESULT hr = settings_callback_(source);
    log_status(L"language-bar",
               std::wstring(L"event=mode_icon_settings_result source=") +
                   (source ? source : L"unknown") + L" result=" + hresult_hex(hr));
    return hr;
}

ModeIconPopupCommand LanguageBarState::show_mode_icon_context_menu() {
    POINT cursor{};
    if (!GetCursorPos(&cursor)) {
        log_status(L"language-bar",
                   L"event=mode_icon_popup_menu_show result=get_cursor_pos_failed error=" +
                       std::to_wstring(GetLastError()));
        return ModeIconPopupCommand::Error;
    }
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        log_status(L"language-bar",
                   L"event=mode_icon_popup_menu_show result=create_menu_failed error=" +
                       std::to_wstring(GetLastError()));
        return ModeIconPopupCommand::Error;
    }

    const BOOL toggle_added = AppendMenuW(menu, MF_STRING, kMenuToggleInputMode, L"Switch Chinese/English");
    const BOOL settings_added = AppendMenuW(menu, MF_STRING, kMenuOpenSettings, L"Open Settings");
    if (!toggle_added || !settings_added) {
        const DWORD error = GetLastError();
        DestroyMenu(menu);
        log_status(L"language-bar",
                   L"event=mode_icon_popup_menu_show result=append_failed error=" +
                       std::to_wstring(error));
        return ModeIconPopupCommand::Error;
    }

    HWND owner = ensure_popup_owner_window();
    if (!owner) {
        const DWORD error = GetLastError();
        DestroyMenu(menu);
        log_status(L"language-bar",
                   L"event=mode_icon_popup_menu_show result=owner_failed error=" +
                       std::to_wstring(error));
        return ModeIconPopupCommand::Error;
    }

    SetForegroundWindow(owner);
    SetLastError(ERROR_SUCCESS);
    const UINT command = TrackPopupMenuEx(menu,
                                          TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                                          cursor.x,
                                          cursor.y,
                                          owner,
                                          nullptr);
    const DWORD error = command == 0 ? GetLastError() : ERROR_SUCCESS;
    PostMessageW(owner, WM_NULL, 0, 0);
    DestroyMenu(menu);

    if (command == kMenuToggleInputMode) {
        return ModeIconPopupCommand::Toggle;
    }
    if (command == kMenuOpenSettings) {
        return ModeIconPopupCommand::Settings;
    }
    if (command == 0 && error == ERROR_SUCCESS) {
        return ModeIconPopupCommand::Cancel;
    }
    log_status(L"language-bar",
               L"event=mode_icon_popup_menu_show result=track_failed command=" +
                   std::to_wstring(command) + L" error=" + std::to_wstring(error));
    return ModeIconPopupCommand::Error;
}

HWND LanguageBarState::ensure_popup_owner_window() {
    if (popup_owner_hwnd_) {
        return popup_owner_hwnd_;
    }
    if (!register_popup_owner_window_class()) {
        return nullptr;
    }
    popup_owner_hwnd_ = CreateWindowExW(0,
                                        kPopupOwnerClassName,
                                        L"",
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        HWND_MESSAGE,
                                        nullptr,
                                        language_bar_module(),
                                        nullptr);
    return popup_owner_hwnd_;
}

void LanguageBarState::destroy_popup_owner_window() {
    if (popup_owner_hwnd_) {
        DestroyWindow(popup_owner_hwnd_);
        popup_owner_hwnd_ = nullptr;
    }
}

}  // namespace localpinyin
