#include "../settings/settings_window.h"
#include "../settings/dictionary_preview_window.h"
#include "../ime/candidate_window.h"
#include "../ime/globals.h"
#include "../engine/pinyin_engine.h"
#include "../common/win32_utils.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdint>

namespace localpinyin {

// This executable is a test-only consumer of the actual application window.
// Drawing stays on its UI thread, because Direct2D and HDC handles cannot be
// reliably marshalled into an unrelated screenshot process.
class SettingsWindowCapture {
public:
    static int run(HINSTANCE instance, const std::filesystem::path& output) {
        g_instance = instance;
        PinyinEngine engine(DictionaryLoadMode::Empty);
        if (!engine.load_dictionary_resource_directory(std::filesystem::path(module_path(instance)).parent_path().wstring())) {
            std::wcerr << L"Could not load the product dictionaries for candidate capture.\n";
            return 1;
        }
        SettingsWindow window;
        int capture_status = 0;
        const int app_status = window.run(instance, SW_HIDE, [&](HWND hwnd) {
            if (!verify_native_controls(window)) capture_status = 1;
            if (!verify_candidate_preview(window, engine)) capture_status = 1;
            const wchar_t* names[] = {L"appearance", L"input", L"dictionary", L"learning", L"about"};
            for (int theme = 0; theme < 2; ++theme) {
                // Use the same application state and refresh path, while
                // avoiding the normal cross-process settings broadcast.
                window.options_.theme_mode = theme == 0 ? CandidateThemeMode::Light : CandidateThemeMode::Dark;
                window.refresh_theme();
                window.apply_options_to_controls();
                for (int page = 0; page < 5; ++page) {
                    window.show_page(static_cast<SettingsPage>(page));
                    const auto file = output / ((theme == 0 ? std::wstring(L"light-") : std::wstring(L"dark-")) + names[page] + L".bmp");
                    if (!save(window, file)) {
                        std::wcerr << L"Capture failed: " << file << L"\n";
                        capture_status = 1;
                    } else {
                        std::wcout << file << L"\n";
                    }
                }
                window.show_page(SettingsPage::Appearance);
                const wchar_t* preview_queries[] = {L"suibian", L"zzzzzzzzzzzz", L""};
                const wchar_t* preview_states[] = {L"appearance-suibian", L"appearance-unknown", L"appearance-empty"};
                for (int state = 0; state < 3; ++state) {
                    SetWindowTextW(window.candidate_preview_input_, preview_queries[state]);
                    const auto path = output / ((theme == 0 ? std::wstring(L"light-") : std::wstring(L"dark-")) + preview_states[state] + L".bmp");
                    if (!save(window, path)) capture_status = 1;
                    else std::wcout << path << L"\n";
                }
                const auto previous_size = window.options_.text_size;
                SetWindowTextW(window.candidate_preview_input_, L"ninengbangwokankanzhegewenjianzenmedakaima");
                const auto long_path = output / ((theme == 0 ? std::wstring(L"light-") : std::wstring(L"dark-")) + L"appearance-long-large.bmp");
                // Render the actual window at every supported size, checking
                // the stored geometry from that paint pass. Keep the last,
                // largest-size image as the review artifact.
                for (int level = candidate_text_size_min_level(); level <= candidate_text_size_max_level(); ++level) {
                    window.options_.text_size = candidate_text_size_from_level(level);
                    window.sync_candidate_text_size_controls();
                    if (!save(window, long_path)) capture_status = 1;
                }
                std::wcout << long_path << L"\n";
                window.options_.text_size = previous_size;
                window.sync_candidate_text_size_controls();
                SetWindowTextW(window.candidate_preview_input_, L"nihao");
                HWND preview = DictionaryPreviewWindow::show(hwnd, instance, window.dictionary_catalog_, theme != 0, SW_HIDE);
                if (!preview) { capture_status = 1; continue; }
                auto* browser = reinterpret_cast<DictionaryPreviewWindow*>(GetWindowLongPtrW(preview, GWLP_USERDATA));
                if (GetNextDlgTabItem(preview, browser->search_, FALSE) != browser->list_ ||
                    GetNextDlgTabItem(preview, browser->list_, TRUE) != browser->search_) {
                    std::wcerr << L"Dictionary browser Tab order failed.\n";
                    capture_status = 1;
                }
                DictionaryPreviewWindow::set_theme(preview, theme == 0);
                if (browser->dark_theme_ != (theme == 0)) capture_status = 1;
                DictionaryPreviewWindow::set_theme(preview, theme != 0);
                if (browser->dark_theme_ != (theme != 0)) capture_status = 1;
                const std::wstring prefix = theme == 0 ? L"light-" : L"dark-";
                const wchar_t* queries[] = {L"", L"nihao", L"zzzzzzzzzzzz"};
                const wchar_t* states[] = {L"lexicon-browser", L"lexicon-search", L"lexicon-empty"};
                for (int state = 0; state < 3; ++state) {
                    SetWindowTextW(browser->search_, queries[state]);
                    browser->populate();
                    const auto file = output / (prefix + states[state] + L".bmp");
                    if (!save(preview, file)) capture_status = 1;
                    else std::wcout << file << L"\n";
                }
                DestroyWindow(preview);
                if (!capture_candidates(engine, theme != 0, output)) capture_status = 1;
            }
            PostMessageW(hwnd, WM_CLOSE, 0, 0);
        });
        return capture_status != 0 ? capture_status : app_status;
    }

private:
    static bool verify_native_controls(SettingsWindow& window) {
        const HWND foreground_before = GetForegroundWindow();
        const HWND focus_before = GetFocus();
        window.show_page(SettingsPage::Candidate);
        const HWND hints = GetDlgItem(window.hwnd_, 1201);
        const HWND shift = GetDlgItem(window.hwnd_, 1202);
        bool valid = true;
        const auto check = [&](bool condition, const wchar_t* test, int id = 0) {
            if (!condition) {
                std::wcerr << L"Native control verification failed: " << test << L" (ID " << id << L").\n";
                valid = false;
            }
        };
        const auto check_tab = [&] {
            check(hints && shift && GetNextDlgTabItem(window.hwnd_, hints, FALSE) == shift,
                  L"forward checkbox Tab order", 1201);
            check(hints && shift && GetNextDlgTabItem(window.hwnd_, shift, TRUE) == hints,
                  L"reverse checkbox Tab order", 1202);
        };
        check_tab();
        window.show_page(SettingsPage::Appearance);
        window.show_page(SettingsPage::Candidate);
        check_tab();
        HWND probe = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, L"STATIC", L"", WS_POPUP,
                                       0, 0, 100, 100, nullptr, nullptr, g_instance, nullptr);
        if (!probe) return false;
        // Reparent each actual application control briefly so its BN_CLICKED
        // notifications stay in this hidden probe and do not broadcast changes
        // to any running input methods. Its native behavior and D2D subclass
        // remain intact; no replacement controls or mock handlers are used.
        for (HWND control : {hints, shift}) {
            if (!control) { check(false, L"checkbox exists"); continue; }
            const int id = GetDlgCtrlID(control);
            wchar_t name[64]{};
            check(GetWindowTextW(control, name, static_cast<int>(std::size(name))) > 0,
                  L"accessible native name", id);
            check((GetWindowLongPtrW(control, GWL_STYLE) & BS_TYPEMASK) == BS_AUTOCHECKBOX,
                  L"native auto-checkbox role", id);
            const LRESULT initial = SendMessageW(control, BM_GETCHECK, 0, 0);
            const HWND previous_sibling = GetWindow(control, GW_HWNDPREV);
            SetParent(control, probe);
            SendMessageW(control, BM_SETCHECK, BST_UNCHECKED, 0);
            SendMessageW(control, BM_CLICK, 0, 0);
            check(SendMessageW(control, BM_GETCHECK, 0, 0) == BST_CHECKED,
                  L"native click toggles check state", id);
            SetFocus(control);
            SendMessageW(control, WM_KEYDOWN, VK_SPACE, 0);
            SendMessageW(control, WM_KEYUP, VK_SPACE, 0);
            check(SendMessageW(control, BM_GETCHECK, 0, 0) == BST_UNCHECKED,
                  L"native Space toggles check state", id);
            SendMessageW(control, BM_SETCHECK, initial, 0);
            SetParent(control, window.hwnd_);
            // SetParent moves the control in the sibling list. Restore its
            // original position so the probe does not alter product Tab order.
            SetWindowPos(control, previous_sibling ? previous_sibling : HWND_TOP, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
        SetFocus(focus_before);
        DestroyWindow(probe);
        for (int id : {2001, 2002, 2003, 2004, 2005, 1101, 1102, 1103}) {
            HWND control = GetDlgItem(window.hwnd_, id);
            check(control && (GetWindowLongPtrW(control, GWL_STYLE) & BS_TYPEMASK) == BS_AUTORADIOBUTTON,
                  L"native auto-radio role", id);
        }
        check(SendMessageW(GetDlgItem(window.hwnd_, 2002), BM_GETCHECK, 0, 0) == BST_CHECKED &&
              SendMessageW(GetDlgItem(window.hwnd_, 2001), BM_GETCHECK, 0, 0) == BST_UNCHECKED,
              L"selected navigation radio state", 2002);
        check(GetForegroundWindow() == foreground_before, L"foreground preserved");
        check_tab();
        window.apply_options_to_controls();
        window.layout_controls();
        std::wcout << L"Native button names/check state/Space/Tab verification: " << (valid ? L"PASS" : L"FAIL") << L"\n";
        return valid;
    }
    static bool verify_candidate_preview(SettingsWindow& window, PinyinEngine& reference) {
        bool valid = window.candidate_preview_ready_ && window.candidate_preview_input_;
        for (const wchar_t* pinyin : {L"nihao", L"suibian", L"zzzzzzzzzzzz", L""}) {
            SetWindowTextW(window.candidate_preview_input_, pinyin);
            std::vector<std::wstring> expected;
            for (const auto& candidate : reference.lookup(pinyin)) expected.push_back(candidate.text);
            if (window.candidate_preview_pinyin_ != pinyin || window.candidate_preview_texts_ != expected) {
                std::wcerr << L"Live candidate preview lookup failed: " << pinyin << L".\n";
                valid = false;
            }
        }
        SetWindowTextW(window.candidate_preview_input_, L"nihao");
        window.show_page(SettingsPage::Appearance);
        valid = valid && GetNextDlgTabItem(window.hwnd_, window.candidate_size_slider_, FALSE) == window.candidate_preview_input_ &&
                GetNextDlgTabItem(window.hwnd_, window.candidate_preview_input_, FALSE) == GetDlgItem(window.hwnd_, 1215);
        if (!valid) std::wcerr << L"Live preview readiness, candidates, or Tab order failed.\n";
        std::wcout << L"Live candidate preview input/lookup/Tab verification: " << (valid ? L"PASS" : L"FAIL") << L"\n";
        return valid;
    }
    static bool capture_candidates(PinyinEngine& engine, bool dark, const std::filesystem::path& output) {
        const HWND foreground_before = GetForegroundWindow();
        const HWND focus_before = GetFocus();
        const auto candidates = engine.lookup(L"nihao");
        CandidateWindow window;
        if (candidates.size() < 2 || !window.create()) return false;
        // Install the real lookup model without showing the popup on the user's
        // desktop. The normal layout and rendering paths remain unchanged.
        for (const auto& candidate : candidates) window.candidate_texts_.push_back(candidate.text);
        window.composition_text_ = L"ni hao";
        window.selected_index_ = 0;
        window.caret_rect_ = RECT{200, 200, 202, 220};
        window.options_.theme_mode = dark ? CandidateThemeMode::Dark : CandidateThemeMode::Light;
        window.palette_ = make_candidate_palette(window.options_.theme_mode, dark);
        if (!window.rebuild_layout(static_cast<int>(GetDpiForWindow(window.hwnd_))) || window.layout_.items.size() < 2) return false;
        SetWindowPos(window.hwnd_, nullptr, 0, 0, window.layout_.width, window.layout_.height, SWP_NOZORDER | SWP_NOACTIVATE);
        size_t commit_count = 0;
        size_t committed_index = static_cast<size_t>(-1);
        window.set_selection_callback([&](size_t index) { ++commit_count; committed_index = index; });
        const auto center = [](const CandidateItemLayout& item) {
            return POINT{(item.bounds.left + item.bounds.right) / 2, (item.bounds.top + item.bounds.bottom) / 2};
        };
        const POINT first = center(window.layout_.items[0]);
        const POINT second = center(window.layout_.items[1]);
        const auto send = [&](UINT message, POINT point) {
            SendMessageW(window.hwnd_, message, message == WM_LBUTTONDOWN ? MK_LBUTTON : 0,
                         MAKELPARAM(static_cast<WORD>(point.x), static_cast<WORD>(point.y)));
        };
        const std::wstring prefix = dark ? L"dark-" : L"light-";
        const auto snapshot = [&](const wchar_t* state) {
            const auto path = output / (prefix + state + L".bmp");
            if (!save(window.hwnd_, path)) return false;
            std::wcout << path << L"\n";
            return true;
        };
        if (!snapshot(L"candidate")) return false;
        send(WM_MOUSEMOVE, second);
        if (!snapshot(L"candidate-hover")) return false;
        send(WM_LBUTTONDOWN, second);
        if (!snapshot(L"candidate-pressed")) return false;
        send(WM_LBUTTONUP, first);
        if (commit_count != 0) return false;
        send(WM_LBUTTONDOWN, second);
        SendMessageW(window.hwnd_, WM_CANCELMODE, 0, 0);
        send(WM_LBUTTONUP, second);
        if (commit_count != 0) return false;
        send(WM_LBUTTONDOWN, second);
        send(WM_LBUTTONUP, second);
        const auto expected = candidate_model_index(window.layout_, window.layout_.items[1].candidate_index, window.candidate_texts_);
        const bool valid = expected && commit_count == 1 && committed_index == *expected &&
                           window.selected_index_ == 0 && !IsWindowVisible(window.hwnd_) &&
                           SendMessageW(window.hwnd_, WM_MOUSEACTIVATE, 0, 0) == MA_NOACTIVATE &&
                           GetForegroundWindow() == foreground_before && GetFocus() == focus_before;
        std::wcout << L"Candidate pointer verification: " << (valid ? L"PASS" : L"FAIL") << L"\n";
        return valid;
    }
    struct Children {
        HWND window = nullptr;
        HDC dc = nullptr;
    };
    static BOOL CALLBACK draw_child(HWND child, LPARAM value) {
        auto* capture = reinterpret_cast<Children*>(value);
        if (GetParent(child) != capture->window ||
            (GetWindowLongPtrW(child, GWL_STYLE) & WS_VISIBLE) == 0) return TRUE;
        RECT bounds{};
        GetWindowRect(child, &bounds);
        MapWindowPoints(nullptr, capture->window, reinterpret_cast<POINT*>(&bounds), 2);
        const int saved = SaveDC(capture->dc);
        SetViewportOrgEx(capture->dc, bounds.left, bounds.top, nullptr);
        IntersectClipRect(capture->dc, 0, 0, bounds.right - bounds.left, bounds.bottom - bounds.top);
        SendMessageW(child, WM_PRINT, reinterpret_cast<WPARAM>(capture->dc), PRF_CLIENT | PRF_NONCLIENT | PRF_CHILDREN);
        RestoreDC(capture->dc, saved);
        return TRUE;
    }
    static bool save(SettingsWindow& window, const std::filesystem::path& path) {
        if (!save(window.hwnd_, path)) return false;
        if (window.active_page_ != SettingsPage::Appearance) return true;
        const auto& preview = window.candidate_preview_layout_;
        if (window.candidate_preview_texts_.empty()) return !preview.visible;
        RECT client{};
        GetClientRect(window.hwnd_, &client);
        const auto layout = calculate_settings_window_layout(client.right, client.bottom, window.dpi_);
        const auto& area = layout.appearance_preview_window;
        const RECT bounds{area.x, area.y, area.x + area.width, area.y + area.height};
        const auto contains = [](const RECT& outer, const RECT& inner) {
            return inner.left >= outer.left && inner.top >= outer.top &&
                   inner.right <= outer.right && inner.bottom <= outer.bottom;
        };
        bool fit = preview.visible && !preview.items.empty() &&
                   preview.items.front().display_text == window.candidate_preview_texts_.front() &&
                   contains(bounds, preview.window_rect) &&
                   contains(preview.window_rect, preview.composition_rect) &&
                   contains(preview.window_rect, preview.action_button_rect) &&
                   (!preview.show_hint || contains(preview.window_rect, preview.hint_rect));
        for (const auto& item : preview.items) fit = fit && contains(preview.window_rect, item.bounds);
        if (!fit) std::wcerr << L"Actual candidate preview overflows its card or loses the first result: " << path << L".\n";
        return fit;
    }
    static bool save(HWND window, const std::filesystem::path& path) {
        RECT bounds{};
        GetClientRect(window, &bounds);
        const int width = bounds.right;
        const int height = bounds.bottom;
        if (width <= 0 || height <= 0) return false;
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = width;
        info.bmiHeader.biHeight = -height;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        info.bmiHeader.biSizeImage = static_cast<DWORD>(width * height * 4);
        HDC dc = CreateCompatibleDC(nullptr);
        void* pixels = nullptr;
        HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
        if (!dc || !bitmap || !pixels) {
            if (bitmap) DeleteObject(bitmap);
            if (dc) DeleteDC(dc);
            return false;
        }
        HGDIOBJ previous = SelectObject(dc, bitmap);
        SendMessageW(window, WM_PRINTCLIENT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT);
        Children children{window, dc};
        EnumChildWindows(window, draw_child, reinterpret_cast<LPARAM>(&children));
        GdiFlush();
        BITMAPFILEHEADER header{};
        header.bfType = 0x4D42;
        header.bfOffBits = sizeof(header) + sizeof(BITMAPINFOHEADER);
        header.bfSize = header.bfOffBits + info.bmiHeader.biSizeImage;
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file.write(reinterpret_cast<const char*>(&header), sizeof(header));
        file.write(reinterpret_cast<const char*>(&info.bmiHeader), sizeof(BITMAPINFOHEADER));
        file.write(static_cast<const char*>(pixels), info.bmiHeader.biSizeImage);
        const bool success = file.good();
        SelectObject(dc, previous);
        DeleteObject(bitmap);
        DeleteDC(dc);
        return success;
    }
};

}  // namespace localpinyin

int wmain(int argc, wchar_t** argv) {
    if (argc != 3) {
        std::wcerr << L"Usage: LocalPinyinSettingsCapture <absolute-output-directory> <absolute-data-directory>\n";
        return 2;
    }
    const std::filesystem::path output(argv[1]);
    const std::filesystem::path data(argv[2]);
    if (!output.is_absolute() || !data.is_absolute()) return 2;
    std::filesystem::create_directories(output);
    std::filesystem::create_directories(data / L"LocalPinyinIME");
    SetEnvironmentVariableW(L"LOCALPINYINIME_LOCALAPPDATA", data.c_str());
    SetEnvironmentVariableW(L"LOCALPINYINIME_USER_LEXICON_PATH", nullptr);
    // A real fixture file lets the real loader, table, and editor participate
    // in visual validation without reading the user's private dictionary.
    std::ofstream fixture(data / L"LocalPinyinIME" / L"user_lexicon.tsv", std::ios::binary | std::ios::trunc);
    fixture << "nihao\t你好\t5000\nxiandaijiemian\t现代界面\t8000\nxuexi\t学习\t7000\n";
    fixture.close();
    return localpinyin::SettingsWindowCapture::run(GetModuleHandleW(nullptr), output);
}
