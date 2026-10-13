#include "../ime/candidate_window.h"
#include "../ime/globals.h"
#include <gdiplus.h>
#include <filesystem>
#include <iostream>
#include <string>

namespace localpinyin {
// Test-only capture uses the actual native renderer in a hidden HWND. It never
// presents a window, activates the desktop, or registers an OS input profile.
class CandidateWindowCapture {
public:
    static bool capture(const std::filesystem::path& output, bool dark, int dpi, int level,
                        int work_width, bool last_page, bool prediction) {
        CandidateWindow window;
        if (!window.create()) return false;
        window.options_.theme_mode = dark ? CandidateThemeMode::Dark : CandidateThemeMode::Light;
        window.options_.text_size = candidate_text_size_from_level(level);
        window.palette_ = make_candidate_palette(window.options_.theme_mode, dark);
        window.prediction_mode_ = prediction;
        window.composition_text_ = prediction ? L"联想 · Tab" : L"nihaoshijiezheshiyigehenchangdepinyin";
        window.candidate_texts_ = {L"你好", L"你好世界", L"这是一个非常长的候选词用来验证单行省略与完整上屏",
            L"离线输入", L"本地拼音", L"分页显示", L"左右翻页", L"原始候选完整保留", L"最后一页的长候选也必须完整保留"};
        window.update_fonts(dpi);
        CandidateLayoutInput input;
        input.composition_text = window.composition_text_;
        input.candidate_texts = window.candidate_texts_;
        input.work_area = RECT{0, 0, work_width, 800};
        input.caret_rect = RECT{40, 40, 41, 62};
        input.dpi = dpi;
        input.options = window.options_;
        for (size_t i = 0; i < input.candidate_texts.size(); ++i) {
            input.candidate_text_widths_px.push_back(window.renderer_.measure_text(input.candidate_texts[i],
                static_cast<float>(candidate_font_px_for_size(input.options.text_size, dpi)), FW_MEDIUM));
            input.number_text_widths_px.push_back(window.renderer_.measure_text(std::to_wstring(i + 1),
                static_cast<float>(candidate_font_px_for_size(input.options.text_size, dpi))));
        }
        window.layout_ = make_candidate_layout(input);
        if (last_page) {
            input.selected_index = window.layout_.page_indices.back();
            window.layout_ = make_candidate_layout(input);
        }
        const int width = window.layout_.width, height = window.layout_.height;
        if (!SetWindowPos(window.hwnd_, nullptr, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOZORDER)) return false;
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
    if (argc != 2) { std::cerr << "Usage: LocalPinyinCandidateCapture <output directory>\n"; return 2; }
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    localpinyin::g_instance = GetModuleHandleW(nullptr);
    Gdiplus::GdiplusStartupInput startup;
    ULONG_PTR token = 0;
    if (Gdiplus::GdiplusStartup(&token, &startup, nullptr) != Gdiplus::Ok) return 1;
    const std::filesystem::path output(argv[1]);
    std::filesystem::create_directories(output);
    const bool ok = localpinyin::CandidateWindowCapture::capture(output / L"candidate-light-first.png", false, 96, 4, 1920, false, false) &&
        localpinyin::CandidateWindowCapture::capture(output / L"candidate-light-last.png", false, 96, 4, 1920, true, false) &&
        localpinyin::CandidateWindowCapture::capture(output / L"candidate-dark-large-first.png", true, 192, 7, 900, false, false) &&
        localpinyin::CandidateWindowCapture::capture(output / L"candidate-dark-large-last.png", true, 192, 7, 900, true, false) &&
        localpinyin::CandidateWindowCapture::capture(output / L"prediction-narrow-last.png", false, 144, 7, 360, true, true);
    Gdiplus::GdiplusShutdown(token);
    CoUninitialize();
    return ok ? 0 : 1;
}
