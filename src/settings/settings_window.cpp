#include "settings_window.h"

#include <shellapi.h>

namespace localpinyin {

int SettingsWindow::run(HINSTANCE instance, int show_command) {
    store_.ensure_data_dir();

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = SettingsWindow::window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"LocalPinyinSettingsWindow";
    RegisterClassExW(&wc);

    hwnd_ = CreateWindowExW(0, wc.lpszClassName, L"LocalPinyinIME Settings", WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT, 520, 260, nullptr, nullptr, instance, this);
    if (!hwnd_) {
        return 1;
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
            create_controls();
            return 0;
        case WM_COMMAND:
            if (LOWORD(wparam) == 1001) {
                open_data_dir();
                return 0;
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd_, message, wparam, lparam);
    }
}

void SettingsWindow::create_controls() {
    CreateWindowW(L"STATIC", L"LocalPinyinIME is configured for offline Chinese mode by default.",
                  WS_CHILD | WS_VISIBLE, 20, 20, 460, 24, hwnd_, nullptr, nullptr, nullptr);
    CreateWindowW(L"STATIC", store_.data_dir().c_str(),
                  WS_CHILD | WS_VISIBLE, 20, 58, 460, 24, hwnd_, nullptr, nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Open data folder",
                  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 100, 150, 32,
                  hwnd_, reinterpret_cast<HMENU>(1001), nullptr, nullptr);
}

void SettingsWindow::open_data_dir() {
    store_.ensure_data_dir();
    ShellExecuteW(hwnd_, L"open", store_.data_dir().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

}  // namespace localpinyin
