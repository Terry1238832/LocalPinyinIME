#include "../ime/candidate_window.h"
#include "../ime/globals.h"
#include "../ime/language_bar.h"
#include <gdiplus.h>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace localpinyin {
// Test-only capture uses the actual native renderer in a hidden HWND. It never
// presents a window, activates the desktop, or registers an OS input profile.
class CandidateWindowCapture {
public:
    struct Scene {
        std::wstring name;
        CandidateStyle style = kDefaultCandidateStyle;
        bool dark = false;
        int dpi = 96;
        int level = 4;
        int work_width = 1920;
        bool last_page = false;
        std::wstring composition;
        std::vector<std::wstring> candidates;
        bool prediction = false;
        // Pointer over this page slot (shows a style's moving focus at rest there).
        std::optional<size_t> hover;
    };

    // Shows the real layered popup for a moment and copies that screen area,
    // verifying UpdateLayeredWindow presentation (per-pixel alpha + shadow).
    // The style comes from settings.json, as in the IME (point
    // LOCALPINYINIME_LOCALAPPDATA at a scratch folder to choose it).
    static bool capture_live(const std::filesystem::path& directory, bool dark) {
        CandidateWindow window;
        std::vector<Candidate> candidates;
        for (const auto* text : {L"你好世界", L"你好", L"拟好", L"你", L"泥"}) candidates.push_back(Candidate{text, 1, 0});
        const RECT caret{240, 200, 241, 222};
        window.show(L"ni'hao'shi'jie", candidates, 0, caret);
        if (!window.is_visible()) return false;
        window.options_.theme_mode = dark ? CandidateThemeMode::Dark : CandidateThemeMode::Light;
        window.palette_ = make_candidate_palette(window.options_.theme_mode, dark, window.options_.style);
        window.present();
        const auto output = directory / (L"live-" + candidate_style_to_string(window.options_.style) +
                                         (dark ? L"-dark.png" : L"-light.png"));
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

    static bool capture(const std::filesystem::path& directory, const Scene& scene) {
        CandidateWindow window;
        if (!window.create()) return false;
        window.options_.theme_mode = scene.dark ? CandidateThemeMode::Dark : CandidateThemeMode::Light;
        window.options_.text_size = candidate_text_size_from_level(scene.level);
        window.options_.style = scene.style;
        window.palette_ = make_candidate_palette(window.options_.theme_mode, scene.dark, scene.style);
        window.composition_text_ = scene.composition;
        window.candidate_texts_ = scene.candidates;
        window.prediction_mode_ = scene.prediction;
        window.caret_rect_ = RECT{60, 40, 61, 62};
        window.update_fonts(scene.dpi);
        const RECT work{0, 0, scene.work_width, 800};
        window.layout_ = make_candidate_layout(window.layout_input(scene.dpi, work));
        if (scene.last_page) {
            // As the IME does after a page turn: the first slot of the last
            // page is selected and the page boundaries are kept.
            window.selected_index_ = window.layout_.page_indices.back();
            window.page_indices_ = window.layout_.page_indices;
            window.layout_ = make_candidate_layout(window.layout_input(scene.dpi, work));
        }
        if (scene.hover && *scene.hover < window.layout_.items.size())
            window.candidate_pointer_.move(window.layout_.items[*scene.hover].candidate_index);
        const auto output = directory / (scene.name + L".png");
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
            << window.layout_.current_page + 1 << L"/" << window.layout_.page_indices.size();
        if (scene.style == CandidateStyle::AdaptiveFlow)
            std::wcout << L" form " << candidate_flow_form_to_string(window.layout_.flow_form);
        std::wcout << L"\n";
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
        const bool live = localpinyin::CandidateWindowCapture::capture_live(output, false) &&
                          localpinyin::CandidateWindowCapture::capture_live(output, true);
        Gdiplus::GdiplusShutdown(token);
        CoUninitialize();
        return live ? 0 : 1;
    }
    using Scene = localpinyin::CandidateWindowCapture::Scene;
    using localpinyin::CandidateStyle;
    const std::vector<std::wstring> sentence = {L"你好世界", L"你好", L"拟好", L"你", L"泥", L"尼", L"妮", L"逆", L"倪"};
    const std::vector<std::wstring> long_list = {L"你好", L"你好世界", L"这是一个非常长的候选词用来验证单行省略与完整上屏",
        L"离线输入", L"本地拼音", L"分页显示", L"左右翻页", L"原始候选完整保留", L"最后一页的长候选也必须完整保留"};
    const std::vector<std::wstring> single = {L"你", L"泥", L"尼", L"呢", L"拟", L"逆", L"妮", L"倪", L"腻"};
    const std::vector<std::wstring> sentences = {L"我今天下午要去图书馆还书", L"我今天下午要去图书馆", L"我今天下午",
        L"我今天", L"我"};
    const std::vector<std::wstring> mixed = {L"Windows 输入法", L"Win", L"微信", L"文档", L"维度"};
    std::vector<Scene> scenes;
    for (const auto style : localpinyin::kCandidateStyles) {
        const std::wstring name = localpinyin::candidate_style_to_string(style);
        scenes.push_back({name + L"-light", style, false, 96, 4, 1920, false, L"ni'hao'shi'jie", sentence});
        scenes.push_back({name + L"-dark", style, true, 96, 4, 1920, false, L"ni'hao'shi'jie", sentence});
        scenes.push_back({name + L"-single", style, false, 96, 4, 1920, false, L"ni", single});
        scenes.push_back({name + L"-paged", style, false, 144, 4, 900, false, L"ce'shi'yi'ge'hen'chang'de'pin'yin", long_list});
        scenes.push_back({name + L"-large-last", style, true, 192, 7, 900, true, L"ce'shi'yi'ge'hen'chang'de'pin'yin", long_list});
        scenes.push_back({name + L"-sentence", style, false, 96, 4, 1920, false, L"wo'jin'tian'xia'wu'yao'qu'tu'shu'guan'huan'shu", sentences});
        scenes.push_back({name + L"-mixed", style, true, 120, 3, 1920, false, L"win", mixed});
        scenes.push_back({name + L"-prediction-narrow", style, false, 144, 7, 360, true, L"联想 · Tab", long_list, true});
        scenes.push_back({name + L"-hover", style, false, 96, 4, 1920, false, L"ni'hao'shi'jie", sentence, false, size_t{3}});
    }
    bool ok = true;
    for (const auto& scene : scenes) ok = localpinyin::CandidateWindowCapture::capture(output, scene) && ok;
    Gdiplus::GdiplusShutdown(token);
    CoUninitialize();
    return ok ? 0 : 1;
}
