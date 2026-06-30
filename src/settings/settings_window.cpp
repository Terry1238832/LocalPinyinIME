#include "settings_window.h"

#include "localpinyin_version.h"

#include <shellapi.h>

namespace localpinyin {
namespace {

constexpr int kIdOpenDataFolder = 1001;
constexpr int kIdThemeSystem = 1101;
constexpr int kIdThemeLight = 1102;
constexpr int kIdThemeDark = 1103;
constexpr int kIdShowKeyHints = 1201;
constexpr int kIdTextSmall = 1301;
constexpr int kIdTextStandard = 1302;
constexpr int kIdTextLarge = 1303;
constexpr int kIdClearLearning = 1401;

HWND make_static(HWND parent, const wchar_t* text, int x, int y, int width, int height) {
    return CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE,
                         x, y, width, height, parent, nullptr, nullptr, nullptr);
}

HWND make_button(HWND parent, const wchar_t* text, int id, int x, int y, int width, int height, DWORD style) {
    // For child controls, CreateWindowW's hMenu parameter carries the integer control ID.
    const auto control_id = reinterpret_cast<HMENU>(static_cast<INT_PTR>(id));
    return CreateWindowW(L"BUTTON", text, WS_CHILD | WS_VISIBLE | style,
                         x, y, width, height, parent, control_id, nullptr, nullptr);
}

bool is_checked(HWND parent, int id) {
    return SendMessageW(GetDlgItem(parent, id), BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void set_checked(HWND parent, int id, bool checked) {
    SendMessageW(GetDlgItem(parent, id), BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

}  // namespace

int SettingsWindow::run(HINSTANCE instance, int show_command) {
    store_.ensure_data_dir();
    options_ = store_.load_candidate_options();

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = SettingsWindow::window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"LocalPinyinSettingsWindow";
    RegisterClassExW(&wc);

    hwnd_ = CreateWindowExW(0, wc.lpszClassName, L"LocalPinyinIME Settings", WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT, 640, 560, nullptr, nullptr, instance, this);
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
            apply_options_to_controls();
            return 0;
        case WM_COMMAND:
            switch (LOWORD(wparam)) {
                case kIdOpenDataFolder:
                    open_data_dir();
                    return 0;
                case kIdThemeSystem:
                case kIdThemeLight:
                case kIdThemeDark:
                case kIdShowKeyHints:
                case kIdTextSmall:
                case kIdTextStandard:
                case kIdTextLarge:
                    save_options_from_controls();
                    return 0;
                case kIdClearLearning:
                    clear_learning_data();
                    return 0;
                default:
                    return 0;
            }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd_, message, wparam, lparam);
    }
}

void SettingsWindow::create_controls() {
    make_static(hwnd_, L"\u5916\u89C2", 24, 20, 160, 24);
    make_button(hwnd_, L"\u8DDF\u968F\u7CFB\u7EDF", kIdThemeSystem, 40, 52, 120, 24, BS_AUTORADIOBUTTON);
    make_button(hwnd_, L"\u6D45\u8272", kIdThemeLight, 170, 52, 90, 24, BS_AUTORADIOBUTTON);
    make_button(hwnd_, L"\u6DF1\u8272", kIdThemeDark, 270, 52, 90, 24, BS_AUTORADIOBUTTON);

    make_static(hwnd_, L"\u5019\u9009\u7A97", 24, 96, 160, 24);
    make_button(hwnd_, L"\u663E\u793A\u6309\u952E\u63D0\u793A", kIdShowKeyHints, 40, 128, 180, 24, BS_AUTOCHECKBOX);
    make_static(hwnd_, L"\u5019\u9009\u6587\u5B57\u5927\u5C0F", 40, 164, 150, 24);
    make_button(hwnd_, L"\u5C0F", kIdTextSmall, 190, 164, 70, 24, BS_AUTORADIOBUTTON);
    make_button(hwnd_, L"\u6807\u51C6", kIdTextStandard, 270, 164, 80, 24, BS_AUTORADIOBUTTON);
    make_button(hwnd_, L"\u5927", kIdTextLarge, 360, 164, 70, 24, BS_AUTORADIOBUTTON);

    make_static(hwnd_, L"\u5B66\u4E60\u6570\u636E", 24, 216, 160, 24);
    make_static(hwnd_,
                L"\u672C\u5730\u5B66\u4E60\u6570\u636E\u53EA\u7528\u4E8E\u5019\u9009\u6392\u5E8F\uFF0C\u4E0D\u4E0A\u4F20\uFF0C\u4E0D\u5199\u5165\u8BCD\u5178 TSV\u3002",
                40, 248, 540, 42);
    make_button(hwnd_, L"\u6E05\u7A7A\u5B66\u4E60\u6570\u636E", kIdClearLearning, 40, 296, 180, 32, BS_PUSHBUTTON);

    make_static(hwnd_, L"\u5173\u4E8E", 24, 352, 160, 24);
    make_static(hwnd_, L"LocalPinyinIME", 40, 384, 260, 24);
    make_static(hwnd_, LOCALPINYINIME_VERSION_STRING, 40, 412, 260, 24);
    make_static(hwnd_, L"x64", 40, 440, 260, 24);
    make_static(hwnd_, L"\u79BB\u7EBF\u8FD0\u884C", 150, 440, 120, 24);
    make_static(hwnd_, L"\u672A\u7B7E\u540D\u5F00\u53D1\u7248", 270, 440, 180, 24);

    make_static(hwnd_, store_.data_dir().c_str(), 40, 476, 430, 24);
    make_button(hwnd_, L"Open data folder", kIdOpenDataFolder, 480, 472, 120, 30, BS_PUSHBUTTON);
}

void SettingsWindow::apply_options_to_controls() {
    CheckRadioButton(hwnd_, kIdThemeSystem, kIdThemeDark,
                     options_.theme_mode == CandidateThemeMode::Light ? kIdThemeLight :
                     options_.theme_mode == CandidateThemeMode::Dark ? kIdThemeDark : kIdThemeSystem);
    set_checked(hwnd_, kIdShowKeyHints, options_.show_key_hints);
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
    if (is_checked(hwnd_, kIdTextSmall)) {
        options_.text_size = CandidateTextSize::Small;
    } else if (is_checked(hwnd_, kIdTextLarge)) {
        options_.text_size = CandidateTextSize::Large;
    } else {
        options_.text_size = CandidateTextSize::Standard;
    }

    store_.save_candidate_options(options_);
}

void SettingsWindow::open_data_dir() {
    store_.ensure_data_dir();
    ShellExecuteW(hwnd_, L"open", store_.data_dir().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
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

}  // namespace localpinyin
