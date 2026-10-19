// A modern one-window installer front end. The proven Inno Setup package is
// embedded as a resource and run silently (it copies, registers and enables
// the input method); this window only presents the flow:
// welcome -> progress -> finished (or failed, with retry and log).
//
//   LocalPinyinInstaller.exe              install
//   LocalPinyinInstaller.exe --preview    simulate an installation (no changes)
//   LocalPinyinInstaller.exe --capture D  render every state to PNG files in D
//
// The manifest asks for the highest rights the user has (highestAvailable):
// an administrator confirms the system prompt as the installer opens, and
// setup then runs without asking again. A standard user opens it unelevated
// and setup asks for an administrator's password on install, so the input
// method is still enabled for the signed-in user. With the environment
// variable __COMPAT_LAYER=RunAsInvoker the developer modes run unelevated.
#include "../settings/modern_renderer.h"
#include "localpinyin_design_tokens.h"
#include "localpinyin_version.h"

#include <windows.h>
#include <dwmapi.h>
#include <exdisp.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <shldisp.h>
#include <shlobj.h>
#include <windowsx.h>
#include <wrl/client.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

namespace {

constexpr int kPayloadResource = 201;
constexpr int kIconResource = 1;
constexpr UINT_PTR kTimer = 1;
// Posted by the preparation thread once setup is unpacked and started.
constexpr UINT kPreparedMessage = WM_APP + 1;
constexpr wchar_t kWindowClass[] = L"LocalPinyinInstallerWindow";
#ifndef LOCALPINYIN_PAYLOAD_BYTES
#define LOCALPINYIN_PAYLOAD_BYTES 340000000ULL
#endif

enum class Stage { Welcome, Installing, Done, Failed };
enum class Hit { None, Primary, Secondary, Minimize, Close };

struct Palette {
    COLORREF background, text, muted, faint, accent, accent_hover, accent_pressed, on_accent, track, hover, success, danger;
};

bool system_dark() {
    DWORD value = 1, size = sizeof(value);
    return RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                        L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &value, &size) == ERROR_SUCCESS && value == 0;
}

// The installer shares the design tokens with the settings app and the
// candidate window (assets/branding/theme/design_tokens.json).
Palette palette_for(bool dark) {
    if (dark) {
        namespace ui = localpinyin::tokens::ui_dark;
        return {ui::kSurface, ui::kText, ui::kText2, ui::kText3, ui::kAccent, ui::kAccentHover, ui::kAccentPressed,
                ui::kOnAccent, ui::kTrack, localpinyin::tokens::candidate_flow_dark::kHover, ui::kSuccess, ui::kDanger};
    }
    namespace ui = localpinyin::tokens::ui_light;
    return {ui::kSurface, ui::kText, ui::kText2, ui::kText3, ui::kAccent, ui::kAccentHover, ui::kAccentPressed,
            ui::kOnAccent, ui::kTrack, localpinyin::tokens::candidate_flow_light::kHover, ui::kSuccess, ui::kDanger};
}

std::wstring install_root() {
    PWSTR path = nullptr;
    std::wstring result = L"C:\\Program Files";
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramFiles, 0, nullptr, &path))) result = path;
    CoTaskMemFree(path);
    return result + L"\\LocalPinyinIME";
}

std::wstring install_dir() {
    return install_root() + L"\\releases\\" + LOCALPINYINIME_VERSION_NUMERIC_STRING + L"\\x64";
}

bool process_elevated() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return false;
    TOKEN_ELEVATION elevation{};
    DWORD size = 0;
    const bool elevated = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size) &&
                          elevation.TokenIsElevated;
    CloseHandle(token);
    return elevated;
}

// Opens a file through the desktop (Explorer), so it runs as the signed-in
// user rather than inheriting the installer's elevation.
bool open_as_desktop_user(const std::wstring& file, const std::wstring& directory, const std::wstring& parameters = L"") {
    using Microsoft::WRL::ComPtr;
    ComPtr<IShellWindows> windows;
    if (FAILED(CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&windows)))) return false;
    VARIANT desktop{}, none{};
    desktop.vt = VT_I4;
    desktop.lVal = CSIDL_DESKTOP;
    long desktop_window = 0;
    ComPtr<IDispatch> found, background, application;
    ComPtr<IServiceProvider> provider;
    ComPtr<IShellBrowser> browser;
    ComPtr<IShellView> view;
    ComPtr<IShellFolderViewDual> folder_view;
    ComPtr<IShellDispatch2> shell;
    if (windows->FindWindowSW(&desktop, &none, SWC_DESKTOP, &desktop_window, SWFO_NEEDDISPATCH, &found) != S_OK ||
        !found || FAILED(found.As(&provider)) ||
        FAILED(provider->QueryService(SID_STopLevelBrowser, IID_PPV_ARGS(&browser))) ||
        FAILED(browser->QueryActiveShellView(&view)) ||
        FAILED(view->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARGS(&background))) ||
        FAILED(background.As(&folder_view)) || FAILED(folder_view->get_Application(&application)) ||
        FAILED(application.As(&shell))) {
        return false;
    }
    const auto text = [](const std::wstring& value) {
        VARIANT variant{};
        variant.vt = VT_BSTR;
        variant.bstrVal = SysAllocString(value.c_str());
        return variant;
    };
    BSTR path = SysAllocString(file.c_str());
    VARIANT arguments = text(parameters), folder = text(directory), verb = text(L""), show{};
    show.vt = VT_I4;
    show.lVal = SW_SHOWNORMAL;
    const HRESULT result = shell->ShellExecute(path, arguments, folder, verb, show);
    SysFreeString(path);
    VariantClear(&arguments);
    VariantClear(&folder);
    VariantClear(&verb);
    return SUCCEEDED(result);
}

std::uint64_t directory_bytes(const std::wstring& directory) {
    std::uint64_t total = 0;
    std::error_code error;
    for (std::filesystem::recursive_directory_iterator it(directory, error), end; !error && it != end; it.increment(error)) {
        if (it->is_regular_file(error)) total += it->file_size(error);
    }
    return total;
}

class InstallerWindow {
public:
    explicit InstallerWindow(bool preview)
        : preview_(preview), elevated_(!preview && process_elevated()), palette_(palette_for(system_dark())) {}

    int run(HINSTANCE instance, int show) {
        instance_ = instance;
        WNDCLASSEXW wc{sizeof(wc)};
        wc.lpfnWndProc = &InstallerWindow::proc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(kIconResource));
        wc.lpszClassName = kWindowClass;
        RegisterClassExW(&wc);
        dpi_ = static_cast<int>(GetDpiForSystem());
        RECT work{};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
        const int width = scale(kWidth), height = scale(kHeight);
        hwnd_ = CreateWindowExW(WS_EX_APPWINDOW, kWindowClass, L"安装本地拼音",
                                WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                (work.left + work.right - width) / 2, (work.top + work.bottom - height) / 2,
                                width, height, nullptr, nullptr, instance, this);
        if (!hwnd_) return 1;
        apply_frame();
        ShowWindow(hwnd_, show);
        MSG message;
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        return 0;
    }

    // Renders one state into a 32-bit DC of the given client size.
    void render(HDC dc, int width, int height) {
        const RECT client{0, 0, width, height};
        {
            localpinyin::ModernRenderScope scope(renderer_, dc, client);
            if (!scope.active()) return;
            draw_content(width);
        }
        draw_icon(dc, width);
        if (stage_ == Stage::Done || stage_ == Stage::Failed) {
            localpinyin::ModernRenderScope scope(renderer_, dc, client);
            if (scope.active()) draw_badge(width);
        }
    }

    void set_capture_state(Stage stage, double progress, int dpi) {
        stage_ = stage;
        shown_progress_ = target_progress_ = progress;
        dpi_ = dpi;
        if (stage == Stage::Failed) failure_ = L"安装已取消：没有获得管理员权限。点“重试”后，在弹出的窗口中选择“是”。";
    }

    void set_palette(bool dark) { palette_ = palette_for(dark); }
    int scale(int value) const { return MulDiv(value, dpi_, 96); }
    static constexpr int kWidth = 520;
    static constexpr int kHeight = 360;

private:
    static LRESULT CALLBACK proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
        auto* self = reinterpret_cast<InstallerWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            self = static_cast<InstallerWindow*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
            self->hwnd_ = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->handle(message, wparam, lparam) : DefWindowProcW(hwnd, message, wparam, lparam);
    }

    void apply_frame() {
        // No system frame: the client fills the window, while DWM keeps the
        // shadow and (on Windows 11) rounded corners.
        const MARGINS margins{0, 0, 0, 1};
        DwmExtendFrameIntoClientArea(hwnd_, &margins);
        const DWORD round = 2;  // DWMWCP_ROUND
        DwmSetWindowAttribute(hwnd_, 33, &round, sizeof(round));
        const BOOL dark = palette_.background != localpinyin::tokens::ui_light::kSurface;
        DwmSetWindowAttribute(hwnd_, 20, &dark, sizeof(dark));
        SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
    }

    RECT dip_rect(int left, int top, int right, int bottom) const {
        return RECT{scale(left), scale(top), scale(right), scale(bottom)};
    }
    RECT primary_rect() const { return dip_rect(kWidth / 2 - 100, 236, kWidth / 2 + 100, 280); }
    RECT secondary_rect() const { return dip_rect(kWidth / 2 - 70, 290, kWidth / 2 + 70, 318); }
    RECT close_rect() const { return dip_rect(kWidth - 46, 0, kWidth, 36); }
    RECT minimize_rect() const { return dip_rect(kWidth - 92, 0, kWidth - 46, 36); }

    // Only a standard user's setup still asks for an administrator here.
    bool waiting_for_elevation() const {
        return stage_ == Stage::Installing && !preview_ && !preparing_ && !elevated_at_ && !setup_finished_ && target_progress_ <= 0;
    }
    void open(const std::wstring& file, const std::wstring& directory, const std::wstring& parameters = L"") const {
        if (elevated_ && open_as_desktop_user(file, directory, parameters)) return;
        ShellExecuteW(nullptr, L"open", file.c_str(), parameters.empty() ? nullptr : parameters.c_str(),
                      directory.empty() ? nullptr : directory.c_str(), SW_SHOWNORMAL);
    }
    bool has_primary() const { return stage_ != Stage::Installing; }
    bool has_secondary() const {
        // The log link only helps when setup actually ran and wrote one.
        return stage_ == Stage::Done ||
               (stage_ == Stage::Failed && (log_path_.empty() || GetFileAttributesW(log_path_.c_str()) != INVALID_FILE_ATTRIBUTES));
    }

    Hit hit_test(POINT point) const {
        const auto inside = [&](RECT rect) { return PtInRect(&rect, point) != FALSE; };
        if (inside(close_rect())) return Hit::Close;
        if (inside(minimize_rect())) return Hit::Minimize;
        if (has_primary() && inside(primary_rect())) return Hit::Primary;
        if (has_secondary() && inside(secondary_rect())) return Hit::Secondary;
        return Hit::None;
    }

    void draw_content(int width) {
        auto& r = renderer_;
        const RECT all{0, 0, width, scale(kHeight)};
        r.fill(all, palette_.background);
        // Window buttons.
        const auto glyph_color = [&](Hit which) { return hot_ == which ? palette_.text : palette_.muted; };
        if (hot_ == Hit::Minimize) r.fill(minimize_rect(), palette_.hover);
        if (hot_ == Hit::Close) r.fill(close_rect(), localpinyin::tokens::ui_light::kWindowClose);
        const float unit = static_cast<float>(dpi_) / 96.0f;
        RECT minimize = minimize_rect(), close = close_rect();
        const float mx = (minimize.left + minimize.right) / 2.0f, my = (minimize.top + minimize.bottom) / 2.0f;
        r.line(mx - 5 * unit, my, mx + 5 * unit, my, glyph_color(Hit::Minimize), unit);
        const float cx = (close.left + close.right) / 2.0f, cy = (close.top + close.bottom) / 2.0f;
        const COLORREF close_color = hot_ == Hit::Close ? RGB(255, 255, 255) : palette_.muted;
        r.line(cx - 5 * unit, cy - 5 * unit, cx + 5 * unit, cy + 5 * unit, close_color, unit);
        r.line(cx - 5 * unit, cy + 5 * unit, cx + 5 * unit, cy - 5 * unit, close_color, unit);

        std::wstring title, subtitle, primary, secondary;
        switch (stage_) {
            case Stage::Welcome:
                title = L"本地拼音";
                subtitle = std::wstring(L"离线的中文拼音输入法 · 版本 ") + LOCALPINYINIME_VERSION_NUMERIC_STRING;
                primary = L"立即安装";
                break;
            case Stage::Installing:
                title = L"正在安装";
                subtitle = waiting_for_elevation() ? L"请在系统弹出的窗口中选择“是”，允许安装输入法。"
                                                   : L"请稍候，安装期间可以继续使用电脑。";
                break;
            case Stage::Done:
                title = L"安装完成";
                subtitle = L"按 Win + 空格 切换到本地拼音。已打开的程序需重新打开。";
                primary = L"开始使用";
                secondary = L"完成";
                break;
            case Stage::Failed:
                title = L"安装没有完成";
                subtitle = failure_;
                primary = L"重试";
                secondary = L"查看安装日志";
                break;
        }
        r.text(title, dip_rect(0, 146, kWidth, 182), palette_.text, 24.0f * unit, 600, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        r.text(subtitle, dip_rect(32, 184, kWidth - 32, 208), palette_.muted, 13.0f * unit, 400,
               DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS);

        if (has_primary()) {
            const RECT button = primary_rect();
            const COLORREF fill = pressed_ == Hit::Primary ? palette_.accent_pressed
                                : hot_ == Hit::Primary ? palette_.accent_hover : palette_.accent;
            r.rounded(button, (button.bottom - button.top) / 2.0f, fill, fill);
            r.text(primary, button, palette_.on_accent, 15.0f * unit, 600, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        }
        if (has_secondary()) {
            const RECT button = secondary_rect();
            if (hot_ == Hit::Secondary) r.rounded(button, static_cast<float>(scale(6)), palette_.hover, palette_.hover);
            r.text(secondary, button, hot_ == Hit::Secondary ? palette_.text : palette_.muted, 13.0f * unit, 400,
                   DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        }
        if (stage_ == Stage::Installing) {
            const RECT track = dip_rect(kWidth / 2 - 150, 248, kWidth / 2 + 150, 254);
            r.rounded(track, (track.bottom - track.top) / 2.0f, palette_.track, palette_.track);
            RECT fill = track;
            fill.right = fill.left + std::max<LONG>(fill.bottom - fill.top,
                static_cast<LONG>(std::lround((track.right - track.left) * std::clamp(shown_progress_, 0.0, 1.0))));
            r.rounded(fill, (fill.bottom - fill.top) / 2.0f, palette_.accent, palette_.accent);
            const int percent = static_cast<int>(std::lround(std::clamp(shown_progress_, 0.0, 1.0) * 100));
            const std::wstring status = preparing_ ? std::wstring(L"正在准备安装文件")
                : waiting_for_elevation() ? std::wstring(L"正在等待管理员权限")
                : (shown_progress_ < 0.92 ? L"正在复制文件 · " : L"正在注册输入法 · ") + std::to_wstring(percent) + L"%";
            r.text(status, dip_rect(0, 264, kWidth, 286), palette_.muted, 12.5f * unit, 400, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        }
        if (stage_ == Stage::Welcome) {
            r.text(L"安装到 " + install_root(), dip_rect(24, kHeight - 40, kWidth - 24, kHeight - 16), palette_.faint,
                   12.0f * unit, 400, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS);
        }
    }

    RECT icon_rect(int width) const {
        const int size = scale(76);
        const int left = (width - size) / 2, top = scale(56);
        return RECT{left, top, left + size, top + size};
    }

    void draw_icon(HDC dc, int width) {
        const RECT rect = icon_rect(width);
        const int size = rect.right - rect.left;
        HICON icon = static_cast<HICON>(LoadImageW(instance_ ? instance_ : GetModuleHandleW(nullptr),
                                                   MAKEINTRESOURCEW(kIconResource), IMAGE_ICON, size, size, 0));
        if (!icon) return;
        DrawIconEx(dc, rect.left, rect.top, icon, size, size, 0, nullptr, DI_NORMAL);
        DestroyIcon(icon);
    }

    void draw_badge(int width) {
        // A round status badge on the icon's lower right corner.
        const RECT icon = icon_rect(width);
        const float radius = static_cast<float>(scale(13));
        const float cx = icon.right - radius * 0.55f, cy = icon.bottom - radius * 0.55f;
        const float ring = radius + static_cast<float>(scale(3));
        renderer_.ellipse(cx, cy, ring, ring, palette_.background);
        const COLORREF color = stage_ == Stage::Done ? palette_.success : palette_.danger;
        renderer_.ellipse(cx, cy, radius, radius, color);
        const float u = radius / 10.0f, stroke = std::max(1.5f, 2.2f * u);
        const COLORREF mark = RGB(255, 255, 255);
        if (stage_ == Stage::Done) {
            renderer_.line(cx - 4.5f * u, cy + 0.2f * u, cx - 1.2f * u, cy + 3.4f * u, mark, stroke);
            renderer_.line(cx - 1.2f * u, cy + 3.4f * u, cx + 4.8f * u, cy - 3.6f * u, mark, stroke);
        } else {
            renderer_.line(cx, cy - 4.5f * u, cx, cy + 1.0f * u, mark, stroke);
            renderer_.ellipse(cx, cy + 4.2f * u, stroke * 0.6f, stroke * 0.6f, mark);
        }
    }

    void paint() {
        PAINTSTRUCT ps{};
        HDC target = BeginPaint(hwnd_, &ps);
        RECT client{};
        GetClientRect(hwnd_, &client);
        const int width = client.right, height = client.bottom;
        BITMAPINFO info{};
        info.bmiHeader = {sizeof(BITMAPINFOHEADER), width, -height, 1, 32, BI_RGB};
        void* bits = nullptr;
        HDC dc = CreateCompatibleDC(target);
        HBITMAP bitmap = CreateDIBSection(target, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (dc && bitmap) {
            const HGDIOBJ previous = SelectObject(dc, bitmap);
            render(dc, width, height);
            BitBlt(target, 0, 0, width, height, dc, 0, 0, SRCCOPY);
            SelectObject(dc, previous);
        }
        if (bitmap) DeleteObject(bitmap);
        if (dc) DeleteDC(dc);
        EndPaint(hwnd_, &ps);
    }

    bool extract_payload(std::wstring* path) {
        HRSRC resource = FindResourceW(nullptr, MAKEINTRESOURCEW(kPayloadResource), RT_RCDATA);
        HGLOBAL loaded = resource ? LoadResource(nullptr, resource) : nullptr;
        const void* data = loaded ? LockResource(loaded) : nullptr;
        const DWORD size = resource ? SizeofResource(nullptr, resource) : 0;
        if (!data || !size) return false;
        wchar_t temp[MAX_PATH]{};
        GetTempPathW(MAX_PATH, temp);
        const std::filesystem::path directory = std::filesystem::path(temp) / L"LocalPinyinIME-Setup";
        std::error_code error;
        std::filesystem::create_directories(directory, error);
        const auto file = directory / (std::wstring(L"LocalPinyinIME-") + LOCALPINYINIME_VERSION_NUMERIC_STRING + L"-setup.exe");
        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        out.write(static_cast<const char*>(data), size);
        if (!out) return false;
        out.close();
        *path = file.wstring();
        log_path_ = (directory / L"install.log").wstring();
        return true;
    }

    void start_install() {
        stage_ = Stage::Installing;
        shown_progress_ = target_progress_ = 0;
        setup_finished_ = false;
        started_ = GetTickCount64();
        failure_.clear();
        hot_ = pressed_ = Hit::None;
        EnableMenuItem(GetSystemMenu(hwnd_, FALSE), SC_CLOSE, MF_GRAYED);
        if (!preview_) {
            // Unpacking 110 MB and starting setup take a moment: they run on
            // a worker so the window keeps painting and moving meanwhile.
            preparing_ = true;
            elevated_at_ = 0;
            worker_ = std::thread([this] { prepare_setup(); });
        }
        SetTimer(hwnd_, kTimer, 33, nullptr);
        InvalidateRect(hwnd_, nullptr, FALSE);
    }

    // Worker thread: unpack the setup package and start it. The result is
    // read on the UI thread after it joins this thread.
    void prepare_setup() {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        prepared_error_.clear();
        std::wstring setup;
        if (!extract_payload(&setup)) {
            prepared_error_ = L"安装文件已损坏，请重新下载安装包。";
        } else {
            baseline_bytes_ = directory_bytes(install_dir());
            // Setup writes its log only once it runs elevated, so a fresh log
            // tells the administrator prompt was accepted.
            DeleteFileW(log_path_.c_str());
            std::wstring parameters = L"/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP- /LOG=\"" + log_path_ + L"\"";
            SHELLEXECUTEINFOW info{sizeof(info)};
            info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
            info.hwnd = hwnd_;
            info.lpFile = setup.c_str();
            info.lpParameters = parameters.c_str();
            info.nShow = SW_SHOWNORMAL;
            if (!ShellExecuteExW(&info) || !info.hProcess) {
                prepared_error_ = GetLastError() == ERROR_CANCELLED ? L"需要允许管理员权限，才能安装输入法。"
                                                                    : L"安装程序无法启动，请重新下载后再试。";
            } else {
                prepared_process_ = info.hProcess;
            }
        }
        CoUninitialize();
        PostMessageW(hwnd_, kPreparedMessage, 0, 0);
    }

    void on_prepared() {
        if (worker_.joinable()) worker_.join();
        preparing_ = false;
        if (!prepared_error_.empty()) return fail(prepared_error_);
        process_ = prepared_process_;
        prepared_process_ = nullptr;
        // Opened elevated: setup inherits it and starts at once.
        if (elevated_) elevated_at_ = GetTickCount64();
        // The installed size is measured on its own thread: walking the
        // growing folder would otherwise stall the progress animation.
        copied_bytes_ = baseline_bytes_;
        scanning_ = true;
        scanner_ = std::thread([this] {
            while (scanning_) {
                copied_bytes_ = directory_bytes(install_dir());
                for (int slice = 0; slice < 5 && scanning_; ++slice) std::this_thread::sleep_for(std::chrono::milliseconds(60));
            }
        });
    }

    void stop_scanner() {
        scanning_ = false;
        if (scanner_.joinable()) scanner_.join();
    }

    void fail(const std::wstring& reason) {
        stage_ = Stage::Failed;
        failure_ = reason;
        KillTimer(hwnd_, kTimer);
        EnableMenuItem(GetSystemMenu(hwnd_, FALSE), SC_CLOSE, MF_ENABLED);
        InvalidateRect(hwnd_, nullptr, FALSE);
    }

    void tick() {
        const double seconds = (GetTickCount64() - started_) / 1000.0;
        DWORD code = 0;
        if (preview_) {
            // Stands in for setup: copies for about four seconds, then exits.
            target_progress_ = std::max(target_progress_, std::min(0.92, seconds / 4.0 * 0.92));
            if (seconds >= 4.0) {
                target_progress_ = 1.0;
                setup_finished_ = true;
            }
        } else if (process_) {
            // Files land in a new versioned folder: their size tracks the copy.
            if (++polls_ % 8 == 0) {
                const std::uint64_t bytes = copied_bytes_;
                const double copied = bytes > baseline_bytes_
                    ? static_cast<double>(bytes - baseline_bytes_) / LOCALPINYIN_PAYLOAD_BYTES : 0.0;
                if (!elevated_at_ && GetFileAttributesW(log_path_.c_str()) != INVALID_FILE_ATTRIBUTES)
                    elevated_at_ = GetTickCount64();
                // Before the prompt is accepted nothing is installing yet.
                const double busy = elevated_at_ ? (GetTickCount64() - elevated_at_) / 1000.0 : 0.0;
                const double paced = elevated_at_ ? 0.9 * (1.0 - std::exp(-busy / 14.0)) : 0.0;
                target_progress_ = std::max(target_progress_, std::min(0.92, std::max(copied * 0.92, paced)));
            }
            if (WaitForSingleObject(process_, 0) == WAIT_OBJECT_0) {
                stop_scanner();
                GetExitCodeProcess(process_, &code);
                CloseHandle(process_);
                process_ = nullptr;
                if (code != 0) {
                    // No log and nothing copied: setup never ran elevated, so
                    // the administrator prompt was declined or timed out.
                    const bool declined = !elevated_ && GetFileAttributesW(log_path_.c_str()) == INVALID_FILE_ATTRIBUTES &&
                                          copied_bytes_ <= baseline_bytes_;
                    return fail(declined ? L"安装已取消：没有获得管理员权限。点“重试”后，在弹出的窗口中选择“是”。"
                                         : L"安装过程出错（代码 " + std::to_wstring(code) + L"），可查看安装日志。");
                }
                // Remembered across ticks: the bar still has to ease to 100%.
                target_progress_ = 1.0;
                setup_finished_ = true;
            }
        }
        shown_progress_ += (target_progress_ - shown_progress_) * 0.18;
        if (setup_finished_ && shown_progress_ > 0.995) {
            shown_progress_ = 1.0;
            stage_ = Stage::Done;
            if (!preview_) {
                // The new clipboard recorder takes over from a running one, and
                // the settings ask about clipboard history if not yet answered.
                open(install_dir() + L"\\LocalPinyinClipboard.exe", install_dir(), L"--replace");
                open(install_dir() + L"\\LocalPinyinSettings.exe", install_dir(), L"--first-run");
            }
            KillTimer(hwnd_, kTimer);
            EnableMenuItem(GetSystemMenu(hwnd_, FALSE), SC_CLOSE, MF_ENABLED);
        }
        InvalidateRect(hwnd_, nullptr, FALSE);
    }

    void activate(Hit hit) {
        if (hit == Hit::Close) {
            if (stage_ != Stage::Installing) DestroyWindow(hwnd_);
        } else if (hit == Hit::Minimize) {
            ShowWindow(hwnd_, SW_MINIMIZE);
        } else if (hit == Hit::Primary) {
            if (stage_ == Stage::Welcome || stage_ == Stage::Failed) start_install();
            else if (stage_ == Stage::Done) {
                if (!preview_) open(install_dir() + L"\\LocalPinyinSettings.exe", install_dir());
                DestroyWindow(hwnd_);
            }
        } else if (hit == Hit::Secondary) {
            if (stage_ == Stage::Done) DestroyWindow(hwnd_);
            else if (stage_ == Stage::Failed && !log_path_.empty()) open(log_path_, L"");
        }
    }

    LRESULT handle(UINT message, WPARAM wparam, LPARAM lparam) {
        switch (message) {
            case WM_NCCALCSIZE:
                if (wparam) return 0;  // the client area covers the whole window
                break;
            case WM_NCHITTEST: {
                POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
                ScreenToClient(hwnd_, &point);
                return hit_test(point) == Hit::None ? HTCAPTION : HTCLIENT;
            }
            case WM_MOUSEMOVE: {
                const Hit hit = hit_test(POINT{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)});
                if (hit != hot_) { hot_ = hit; InvalidateRect(hwnd_, nullptr, FALSE); }
                TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, hwnd_, 0};
                TrackMouseEvent(&track);
                return 0;
            }
            case WM_MOUSELEAVE:
                hot_ = pressed_ = Hit::None;
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            case WM_LBUTTONDOWN:
                pressed_ = hit_test(POINT{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)});
                SetCapture(hwnd_);
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            case WM_LBUTTONUP: {
                const Hit hit = hit_test(POINT{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)});
                const Hit pressed = pressed_;
                pressed_ = Hit::None;
                ReleaseCapture();
                InvalidateRect(hwnd_, nullptr, FALSE);
                if (hit == pressed && hit != Hit::None) activate(hit);
                return 0;
            }
            case WM_SETCURSOR:
                if (LOWORD(lparam) == HTCLIENT && (hot_ == Hit::Primary || hot_ == Hit::Secondary)) {
                    SetCursor(LoadCursorW(nullptr, IDC_HAND));
                    return TRUE;
                }
                break;
            case WM_KEYDOWN:
                if (wparam == VK_RETURN && has_primary()) activate(Hit::Primary);
                if (wparam == VK_ESCAPE && stage_ != Stage::Installing) DestroyWindow(hwnd_);
                return 0;
            case kPreparedMessage:
                on_prepared();
                return 0;
            case WM_TIMER:
                if (wparam == kTimer) tick();
                return 0;
            case WM_CLOSE:
                if (stage_ == Stage::Installing) return 0;
                break;
            case WM_SETTINGCHANGE:
                if (lparam && std::wstring(reinterpret_cast<const wchar_t*>(lparam)) == L"ImmersiveColorSet") {
                    palette_ = palette_for(system_dark());
                    apply_frame();
                    InvalidateRect(hwnd_, nullptr, FALSE);
                }
                break;
            case WM_DPICHANGED: {
                dpi_ = HIWORD(wparam);
                const auto* suggested = reinterpret_cast<const RECT*>(lparam);
                SetWindowPos(hwnd_, nullptr, suggested->left, suggested->top, suggested->right - suggested->left,
                             suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
                return 0;
            }
            case WM_ERASEBKGND:
                return 1;
            case WM_PAINT:
                paint();
                return 0;
            case WM_DESTROY:
                if (worker_.joinable()) worker_.join();
                stop_scanner();
                PostQuitMessage(0);
                return 0;
            default:
                break;
        }
        return DefWindowProcW(hwnd_, message, wparam, lparam);
    }

    HINSTANCE instance_ = nullptr;
    HWND hwnd_ = nullptr;
    bool preview_ = false;
    bool elevated_ = false;
    Palette palette_;
    localpinyin::ModernRenderer renderer_;
    int dpi_ = 96;
    Stage stage_ = Stage::Welcome;
    Hit hot_ = Hit::None;
    Hit pressed_ = Hit::None;
    double shown_progress_ = 0;
    double target_progress_ = 0;
    bool setup_finished_ = false;
    ULONGLONG elevated_at_ = 0;
    ULONGLONG started_ = 0;
    unsigned polls_ = 0;
    std::uint64_t baseline_bytes_ = 0;
    HANDLE process_ = nullptr;
    // Preparation and size measurement threads (see prepare_setup).
    std::thread worker_;
    std::thread scanner_;
    bool preparing_ = false;
    std::wstring prepared_error_;
    HANDLE prepared_process_ = nullptr;
    std::atomic<bool> scanning_{false};
    std::atomic<std::uint64_t> copied_bytes_{0};
    std::wstring failure_;
    std::wstring log_path_;
};

bool capture(const std::filesystem::path& directory) {
    Gdiplus::GdiplusStartupInput startup;
    ULONG_PTR token = 0;
    if (Gdiplus::GdiplusStartup(&token, &startup, nullptr) != Gdiplus::Ok) return false;
    std::filesystem::create_directories(directory);
    bool ok = true;
    const struct { const wchar_t* name; Stage stage; double progress; } states[] = {
        {L"welcome", Stage::Welcome, 0}, {L"installing", Stage::Installing, 0.46},
        {L"done", Stage::Done, 1}, {L"failed", Stage::Failed, 0}};
    for (const bool dark : {false, true}) {
        for (const auto& state : states) {
            InstallerWindow window(true);
            window.set_palette(dark);
            window.set_capture_state(state.stage, state.progress, 144);
            const int width = window.scale(InstallerWindow::kWidth), height = window.scale(InstallerWindow::kHeight);
            BITMAPINFO info{};
            info.bmiHeader = {sizeof(BITMAPINFOHEADER), width, -height, 1, 32, BI_RGB};
            void* bits = nullptr;
            HDC dc = CreateCompatibleDC(nullptr);
            HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
            const HGDIOBJ previous = SelectObject(dc, bitmap);
            window.render(dc, width, height);
            GdiFlush();
            auto* pixels = static_cast<unsigned char*>(bits);
            for (int i = 0; i < width * height; ++i) pixels[i * 4 + 3] = 255;
            {
                Gdiplus::Bitmap image(width, height, width * 4, PixelFormat32bppARGB, pixels);
                const CLSID png{0x557cf406, 0x1a04, 0x11d3, {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}};
                const auto file = directory / (std::wstring(dark ? L"dark-" : L"light-") + state.name + L".png");
                ok = image.Save(file.c_str(), &png, nullptr) == Gdiplus::Ok && ok;
            }
            SelectObject(dc, previous);
            DeleteObject(bitmap);
            DeleteDC(dc);
        }
    }
    Gdiplus::GdiplusShutdown(token);
    return ok;
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    int result = 0;
    if (argc == 3 && std::wstring(argv[1]) == L"--capture") {
        result = capture(argv[2]) ? 0 : 1;
    } else {
        InstallerWindow window(argc >= 2 && std::wstring(argv[1]) == L"--preview");
        result = window.run(instance, show);
    }
    LocalFree(argv);
    CoUninitialize();
    return result;
}
