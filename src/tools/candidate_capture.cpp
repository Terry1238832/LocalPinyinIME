#include "../ime/candidate_window.h"
#include "../ime/globals.h"
#include "../ime/language_bar.h"
#include <gdiplus.h>
#include <filesystem>
#include <iostream>
#include <string>

namespace localpinyin {
// Test-only capture uses the actual native renderer in a hidden HWND. It never
// presents a window, activates the desktop, or registers an OS input profile.
class CandidateWindowCapture {
public:
    struct Scene {
        bool dark = false;
        int dpi = 96;
        int level = 4;
        int work_width = 1920;
        bool last_page = false;
        std::wstring composition;
        std::vector<std::wstring> candidates;
    };

    // Shows the real layered popup for a moment and copies that screen area,
    // verifying UpdateLayeredWindow presentation (per-pixel alpha + shadow).
    static bool capture_live(const std::filesystem::path& output, bool dark) {
        CandidateWindow window;
        std::vector<Candidate> candidates;
        for (const auto* text : {L"你好世界", L"你好", L"拟好", L"你", L"泥"}) candidates.push_back(Candidate{text, 1, 0});
        const RECT caret{240, 200, 241, 222};
        window.show(L"ni'hao'shi'jie", candidates, 0, caret);
        if (!window.is_visible()) return false;
        window.options_.theme_mode = dark ? CandidateThemeMode::Dark : CandidateThemeMode::Light;
        window.palette_ = make_candidate_palette(window.options_.theme_mode, dark);
        window.present();
        for (int i = 0; i < 20; ++i) {
            MSG message;
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) DispatchMessageW(&message);
            Sleep(25);
        }
        RECT rect{};
        GetWindowRect(window.hwnd_, &rect);
        const int width = rect.right - rect.left, height = rect.bottom - rect.top;
        HDC screen = GetDC(nullptr);
        HDC dc = CreateCompatibleDC(screen);
        HBITMAP bitmap = CreateCompatibleBitmap(screen, width, height);
        const HGDIOBJ previous = SelectObject(dc, bitmap);
        BitBlt(dc, 0, 0, width, height, screen, rect.left, rect.top, SRCCOPY | CAPTUREBLT);
        SelectObject(dc, previous);
        Gdiplus::Bitmap image(bitmap, nullptr);
        const CLSID png{0x557cf406, 0x1a04, 0x11d3, {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}};
        const bool ok = image.Save(output.c_str(), &png, nullptr) == Gdiplus::Ok;
        DeleteObject(bitmap); DeleteDC(dc); ReleaseDC(nullptr, screen);
        window.hide();
        std::wcout << output.filename().wstring() << L" live " << width << L"x" << height << L"\n";
        return ok;
    }

    static bool capture(const std::filesystem::path& output, const Scene& scene) {
        CandidateWindow window;
        if (!window.create()) return false;
        window.options_.theme_mode = scene.dark ? CandidateThemeMode::Dark : CandidateThemeMode::Light;
        window.options_.text_size = candidate_text_size_from_level(scene.level);
        window.palette_ = make_candidate_palette(window.options_.theme_mode, scene.dark);
        window.composition_text_ = scene.composition;
        window.candidate_texts_ = scene.candidates;
        window.update_fonts(scene.dpi);
        CandidateLayoutInput input;
        input.composition_text = window.composition_text_;
        input.candidate_texts = window.candidate_texts_;
        input.work_area = RECT{0, 0, scene.work_width, 800};
        input.caret_rect = RECT{60, 40, 61, 62};
        input.dpi = scene.dpi;
        input.options = window.options_;
        const float font_px = static_cast<float>(candidate_font_px_for_size(input.options.text_size, scene.dpi));
        input.composition_text_width_px = window.renderer_.measure_text(input.composition_text,
            static_cast<float>(scale_dip(13, scene.dpi)));
        for (size_t i = 0; i < input.candidate_texts.size(); ++i) {
            input.candidate_text_widths_px.push_back(window.renderer_.measure_text(input.candidate_texts[i], font_px));
            input.number_text_widths_px.push_back(window.renderer_.measure_text(std::to_wstring(i % 9 + 1), font_px * 0.78f));
        }
        window.layout_ = make_candidate_layout(input);
        if (scene.last_page) {
            input.selected_index = window.layout_.page_indices.back();
            window.layout_ = make_candidate_layout(input);
        }
        const int width = window.layout_.width, height = window.layout_.height;
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = width;
        info.bmiHeader.biHeight = -height;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        HDC dc = CreateCompatibleDC(nullptr);
        HBITMAP dib = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (!dib) { DeleteDC(dc); return false; }
        const HGDIOBJ previous = SelectObject(dc, dib);
        // A desktop-like backdrop makes the translucent shadow visible.
        RECT all{0, 0, width, height};
        HBRUSH backdrop = CreateSolidBrush(scene.dark ? RGB(24, 26, 30) : RGB(236, 239, 244));
        FillRect(dc, &all, backdrop);
        DeleteObject(backdrop);
        window.render(dc);
        GdiFlush();
        auto* pixels = static_cast<unsigned char*>(bits);
        for (int i = 0; i < width * height; ++i) pixels[i * 4 + 3] = 255;
        Gdiplus::Bitmap bitmap(width, height, width * 4, PixelFormat32bppARGB, pixels);
        const CLSID png{0x557cf406, 0x1a04, 0x11d3, {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}};
        const bool ok = bitmap.Save(output.c_str(), &png, nullptr) == Gdiplus::Ok;
        SelectObject(dc, previous); DeleteObject(dib); DeleteDC(dc);
        std::wcout << output.filename().wstring() << L" " << width << L"x" << height << L" page "
            << window.layout_.current_page + 1 << L"/" << window.layout_.page_indices.size() << L"\n";
        return ok;
    }
};
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 2 && !(argc == 3 && (std::wstring(argv[2]) == L"--live" || std::wstring(argv[2]) == L"--icons"))) {
        std::cerr << "Usage: LocalPinyinCandidateCapture <output directory> [--live|--icons]\n";
        return 2;
    }
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    localpinyin::g_instance = GetModuleHandleW(nullptr);
    Gdiplus::GdiplusStartupInput startup;
    ULONG_PTR token = 0;
    if (Gdiplus::GdiplusStartup(&token, &startup, nullptr) != Gdiplus::Ok) return 1;
    const std::filesystem::path output(argv[1]);
    std::filesystem::create_directories(output);
    if (argc == 3 && std::wstring(argv[2]) == L"--icons") {
        // Taskbar mode glyphs at every small-icon size on dark and light bars.
        const int sizes[] = {16, 20, 24, 32, 40, 48};
        int width = 8;
        for (const int size : sizes) width += (size + 8) * 4;
        const int height = 2 * (48 + 16);
        bool saved = false;
        {
            Gdiplus::Bitmap sheet(width, height, PixelFormat32bppARGB);
            Gdiplus::Graphics graphics(&sheet);
            graphics.Clear(Gdiplus::Color(255, 32, 32, 32));
            Gdiplus::SolidBrush light(Gdiplus::Color(255, 238, 238, 238));
            graphics.FillRectangle(&light, 0, height / 2, width, height / 2);
            HDC dc = graphics.GetHDC();
            for (int row = 0; row < 2; ++row) {
                const COLORREF color = row == 0 ? RGB(255, 255, 255) : RGB(26, 26, 26);
                int x = 8;
                for (const int size : sizes) {
                    for (const auto mode : {localpinyin::InputMode::Chinese, localpinyin::InputMode::English}) {
                        HICON icon = localpinyin::render_mode_icon(mode, size, color);
                        if (!icon) continue;
                        DrawIconEx(dc, x, row * height / 2 + 8, icon, size, size, 0, nullptr, DI_NORMAL);
                        DestroyIcon(icon);
                        x += size + 8;
                    }
                    x += (size + 8) * 2;
                }
            }
            graphics.ReleaseHDC(dc);
            const CLSID png{0x557cf406, 0x1a04, 0x11d3, {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}};
            saved = sheet.Save((output / L"mode-icons.png").c_str(), &png, nullptr) == Gdiplus::Ok;
        }
        Gdiplus::GdiplusShutdown(token);
        CoUninitialize();
        return saved ? 0 : 1;
    }
    if (argc == 3) {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        const bool live = localpinyin::CandidateWindowCapture::capture_live(output / L"live-light.png", false) &&
                          localpinyin::CandidateWindowCapture::capture_live(output / L"live-dark.png", true);
        Gdiplus::GdiplusShutdown(token);
        CoUninitialize();
        return live ? 0 : 1;
    }
    using Scene = localpinyin::CandidateWindowCapture::Scene;
    const std::vector<std::wstring> sentence = {L"你好世界", L"你好", L"拟好", L"你", L"泥", L"尼", L"妮", L"逆", L"倪"};
    const std::vector<std::wstring> long_list = {L"你好", L"你好世界", L"这是一个非常长的候选词用来验证单行省略与完整上屏",
        L"离线输入", L"本地拼音", L"分页显示", L"左右翻页", L"原始候选完整保留", L"最后一页的长候选也必须完整保留"};
    const bool ok =
        localpinyin::CandidateWindowCapture::capture(output / L"candidate-light.png",
            Scene{false, 96, 4, 1920, false, L"ni'hao'shi'jie", sentence}) &&
        localpinyin::CandidateWindowCapture::capture(output / L"candidate-dark.png",
            Scene{true, 96, 4, 1920, false, L"ni'hao'shi'jie", sentence}) &&
        localpinyin::CandidateWindowCapture::capture(output / L"candidate-light-paged.png",
            Scene{false, 144, 4, 900, false, L"ce'shi'yi'ge'hen'chang'de'pin'yin", long_list}) &&
        localpinyin::CandidateWindowCapture::capture(output / L"candidate-dark-large-last.png",
            Scene{true, 192, 7, 900, true, L"ce'shi'yi'ge'hen'chang'de'pin'yin", long_list}) &&
        localpinyin::CandidateWindowCapture::capture(output / L"prediction-narrow.png",
            Scene{false, 144, 7, 360, true, L"联想 · Tab", long_list});
    Gdiplus::GdiplusShutdown(token);
    CoUninitialize();
    return ok ? 0 : 1;
}
