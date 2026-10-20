#include "tool_panel_window.h"

#include "globals.h"
#include "../settings/settings_store.h"

#include <windowsx.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace localpinyin {
namespace {

constexpr wchar_t kWindowClassName[] = L"LocalPinyinToolPanel";
// Sizes in DIPs: about the width of a long candidate row, a few rows tall.
constexpr int kPanelWidth = 456;
constexpr int kPadding = 10;
constexpr int kTabsHeight = 32;
constexpr int kCategoriesHeight = 26;
constexpr int kContentHeight = 152;
constexpr int kFooterHeight = 30;
constexpr int kShadowMargin = 14;
constexpr int kGutter = 10;

const wchar_t* font_family(PanelFont font) noexcept {
    switch (font) {
        case PanelFont::Symbol: return L"Segoe UI Symbol";
        case PanelFont::Emoji: return L"Segoe UI Emoji";
        default: return L"";
    }
}

float rounded_coverage(float x, float y, const RECT& rect, float radius) noexcept {
    const float cx = std::clamp(x, rect.left + radius, rect.right - radius);
    const float cy = std::clamp(y, rect.top + radius, rect.bottom - radius);
    return std::clamp(0.5f - (std::hypot(x - cx, y - cy) - radius), 0.0f, 1.0f);
}

void box_blur(std::vector<float>& values, int width, int height, int radius) {
    if (radius <= 0) return;
    std::vector<float> scratch(values.size());
    const float scale = 1.0f / static_cast<float>(radius * 2 + 1);
    for (int pass = 0; pass < 3; ++pass) {
        for (int y = 0; y < height; ++y) {
            const float* row = values.data() + static_cast<size_t>(y) * width;
            float sum = 0;
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

int monitor_dpi(const RECT& rect) {
    using MonitorDpiFn = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);
    static const HMODULE shcore = LoadLibraryW(L"shcore.dll");
    static const auto function = shcore ? reinterpret_cast<MonitorDpiFn>(GetProcAddress(shcore, "GetDpiForMonitor")) : nullptr;
    UINT x = 0, y = 0;
    if (function && SUCCEEDED(function(MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST), 0, &x, &y)) && x) return static_cast<int>(x);
    return static_cast<int>(GetDpiForSystem());
}

// A clock face (the recently used category) centered on (x, y).
void clock_icon(ModernRenderer& canvas, float x, float y, float radius, COLORREF color, float stroke) {
    const RECT face{static_cast<int>(x - radius), static_cast<int>(y - radius), static_cast<int>(x + radius), static_cast<int>(y + radius)};
    canvas.outline(face, radius, color, stroke);
    canvas.line(x, y - radius * 0.55f, x, y, color, stroke);
    canvas.line(x, y, x + radius * 0.45f, y + radius * 0.3f, color, stroke);
}

bool same_hit(const PanelHit& left, const PanelHit& right) noexcept {
    return left.kind == right.kind && left.index == right.index;
}

}  // namespace

ToolPanelWindow::~ToolPanelWindow() {
    if (hwnd_) DestroyWindow(hwnd_);
}

bool ToolPanelWindow::create(HWND owner) {
    if (hwnd_) return true;
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = ToolPanelWindow::window_proc;
    window_class.hInstance = g_instance;
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.lpszClassName = kWindowClassName;
    if (!RegisterClassExW(&window_class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
    const auto config = make_candidate_window_create_config(owner, owner && IsWindow(owner));
    hwnd_ = CreateWindowExW(config.ex_style | WS_EX_TOPMOST, kWindowClassName, L"", config.style, 0, 0, 1, 1, config.parent,
                            nullptr, g_instance, this);
    owner_hwnd_ = hwnd_ ? config.parent : nullptr;
    return hwnd_ != nullptr;
}

void ToolPanelWindow::load_palette() {
    const auto options = SettingsStore().load_candidate_options();
    palette_ = high_contrast_enabled() ? make_high_contrast_candidate_palette()
                                       : make_candidate_palette(options.theme_mode, windows_apps_use_dark_theme(), options.style);
}

void ToolPanelWindow::show(const ToolPanelView& view, const RECT& caret_rect, const CandidateWindowOwnerInfo& owner_info) {
    view_ = view;
    caret_ = caret_rect;
    const HWND owner = immersive_process() && owner_info.context_valid ? owner_info.context_hwnd : nullptr;
    if (hwnd_ && owner != owner_hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    if (!create(owner)) return;
    hover_ = {};
    load_palette();
    layout(monitor_dpi(caret_));
    if (!present()) return;
    SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    NotifyWinEvent(EVENT_OBJECT_IME_SHOW, hwnd_, OBJID_CLIENT, CHILDID_SELF);
}

void ToolPanelWindow::update(const ToolPanelView& view) {
    view_ = view;
    if (is_visible()) present();
}

void ToolPanelWindow::hide() {
    hover_ = {};
    if (hwnd_ && IsWindowVisible(hwnd_)) {
        ShowWindow(hwnd_, SW_HIDE);
        NotifyWinEvent(EVENT_OBJECT_IME_HIDE, hwnd_, OBJID_CLIENT, CHILDID_SELF);
    }
}

bool ToolPanelWindow::is_visible() const noexcept {
    return hwnd_ && IsWindowVisible(hwnd_);
}

void ToolPanelWindow::layout(int dpi) {
    dpi_ = std::max(96, dpi);
    margin_ = dip(kShadowMargin);
    const int width = dip(kPanelWidth);
    panel_ = RECT{margin_, margin_, margin_ + width, margin_};
    const int left = panel_.left + dip(kPadding), right = panel_.right - dip(kPadding);
    tabs_ = RECT{left, panel_.top + dip(6), right, panel_.top + dip(6) + dip(kTabsHeight)};
    categories_ = RECT{left, tabs_.bottom + dip(4), right, tabs_.bottom + dip(4) + dip(kCategoriesHeight)};
    content_ = RECT{left, categories_.bottom + dip(2), right, categories_.bottom + dip(2) + dip(kContentHeight)};
    footer_ = RECT{left, content_.bottom + dip(6), right, content_.bottom + dip(6) + dip(kFooterHeight)};
    panel_.bottom = footer_.bottom + dip(8);
    size_ = SIZE{width + margin_ * 2, panel_.bottom + margin_};
    // Below the caret, or above it when the screen ends; never off screen.
    const RECT work = monitor_work_area_for(caret_);
    const int panel_height = panel_.bottom - panel_.top;
    int x = caret_.left - dip(14);
    int y = caret_.bottom + dip(6);
    if (y + panel_height > work.bottom) y = caret_.top - dip(6) - panel_height;
    x = std::clamp(x, static_cast<int>(work.left), std::max(static_cast<int>(work.left), static_cast<int>(work.right) - width));
    y = std::clamp(y, static_cast<int>(work.top), std::max(static_cast<int>(work.top), static_cast<int>(work.bottom) - panel_height));
    origin_ = POINT{x - margin_, y - margin_};
}

void ToolPanelWindow::composite_shadow(std::uint8_t* pixels, int width, int height) {
    if (palette_.shadow_key_alpha <= 0 && palette_.shadow_ambient_alpha <= 0) return;
    if (shadow_size_.cx != width || shadow_size_.cy != height || shadow_.size() != static_cast<size_t>(width) * height) {
        const float radius = static_cast<float>(dip(10));
        const auto silhouette = [&](int offset, int blur) {
            RECT casting = panel_;
            OffsetRect(&casting, 0, offset);
            std::vector<float> mask(static_cast<size_t>(width) * height);
            for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x)
                    mask[static_cast<size_t>(y) * width + x] = rounded_coverage(x + 0.5f, y + 0.5f, casting, radius);
            box_blur(mask, width, height, blur);
            return mask;
        };
        const auto key = silhouette(dip(1), dip(1));
        const auto ambient = silhouette(dip(5), std::max(1, margin_ / 3));
        shadow_.resize(key.size());
        for (size_t index = 0; index < key.size(); ++index) {
            const float k = key[index] * palette_.shadow_key_alpha / 255.0f;
            const float a = ambient[index] * palette_.shadow_ambient_alpha / 255.0f;
            shadow_[index] = 1.0f - (1.0f - k) * (1.0f - a);
        }
        shadow_size_ = SIZE{width, height};
    }
    for (size_t index = 0; index < shadow_.size(); ++index) {
        std::uint8_t* pixel = pixels + index * 4;
        const float remaining = 1.0f - pixel[3] / 255.0f;
        pixel[3] = static_cast<std::uint8_t>(std::clamp(pixel[3] + shadow_[index] * 255.0f * remaining + 0.5f, 0.0f, 255.0f));
    }
}

void ToolPanelWindow::render(std::uint8_t* pixels, HDC dc) {
    std::memset(pixels, 0, static_cast<size_t>(size_.cx) * size_.cy * 4);
    const RECT bounds{0, 0, size_.cx, size_.cy};
    ModernRenderScope modern(renderer_, dc, bounds, true);
    if (!modern.active()) return;
    paint();
    modern.finish();
    GdiFlush();
    composite_shadow(pixels, size_.cx, size_.cy);
}

bool ToolPanelWindow::present() {
    if (!hwnd_ || size_.cx <= 0 || size_.cy <= 0) return false;
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = size_.cx;
    info.bmiHeader.biHeight = -size_.cy;
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
        render(static_cast<std::uint8_t*>(bits), dc);
        POINT source{0, 0};
        BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
        presented = UpdateLayeredWindow(hwnd_, screen, &origin_, &size_, dc, &source, 0, &blend, ULW_ALPHA) != FALSE;
        SelectObject(dc, previous);
    }
    if (dib) DeleteObject(dib);
    if (dc) DeleteDC(dc);
    if (screen) ReleaseDC(nullptr, screen);
    return presented;
}

bool ToolPanelWindow::render_offline(const ToolPanelView& view, int dpi, const CandidateThemePalette& palette,
                                     std::vector<std::uint8_t>* pixels, SIZE* size) {
    view_ = view;
    palette_ = palette;
    caret_ = RECT{40, 40, 41, 60};
    layout(dpi);
    origin_ = POINT{0, 0};
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = size_.cx;
    info.bmiHeader.biHeight = -size_.cy;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    HDC screen = GetDC(nullptr);
    HDC dc = screen ? CreateCompatibleDC(screen) : nullptr;
    void* bits = nullptr;
    HBITMAP dib = dc ? CreateDIBSection(screen, &info, DIB_RGB_COLORS, &bits, nullptr, 0) : nullptr;
    bool rendered = false;
    if (dib && bits) {
        const HGDIOBJ previous = SelectObject(dc, dib);
        render(static_cast<std::uint8_t*>(bits), dc);
        pixels->assign(static_cast<std::uint8_t*>(bits), static_cast<std::uint8_t*>(bits) + static_cast<size_t>(size_.cx) * size_.cy * 4);
        *size = size_;
        rendered = true;
        SelectObject(dc, previous);
    }
    if (dib) DeleteObject(dib);
    if (dc) DeleteDC(dc);
    if (screen) ReleaseDC(nullptr, screen);
    return rendered;
}

void ToolPanelWindow::paint() {
    auto* canvas = active_modern_renderer();
    if (!canvas) return;
    hits_.clear();
    canvas->rounded(panel_, static_cast<float>(dip(10)), palette_.background, palette_.border, 1.0f);
    paint_tabs();
    canvas->line(static_cast<float>(panel_.left + 1), static_cast<float>(tabs_.bottom + dip(2)), static_cast<float>(panel_.right - 1),
                 static_cast<float>(tabs_.bottom + dip(2)), palette_.divider, 1.0f);
    if (view_.preview) {
        paint_preview();
    } else {
        paint_categories();
        if (view_.grid) paint_grid();
        else paint_list();
    }
    paint_footer();
}

int ToolPanelWindow::paint_key_hint(std::wstring_view name, int right, int middle) {
    auto& canvas = *active_modern_renderer();
    const float size = static_cast<float>(dip(10));
    const int width = canvas.measure_text(name, size, 600) + dip(8);
    const int height = dip(15);
    const RECT key{right - width, middle - height / 2, right, middle - height / 2 + height};
    canvas.outline(key, static_cast<float>(dip(3)), palette_.control, 1.0f);
    canvas.text(name, key, palette_.composition_text, size, 600, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
    return key.left;
}

void ToolPanelWindow::paint_tabs() {
    auto& canvas = *active_modern_renderer();
    const float size = static_cast<float>(dip(13));
    int x = tabs_.left - dip(4);
    for (int page = 0; page < kPanelPageCount; ++page) {
        const std::wstring label = panel_page_label(static_cast<PanelPage>(page));
        const bool current = static_cast<int>(view_.page) == page;
        const int width = canvas.measure_text(label, size, current ? 700 : 400) + dip(14);
        const RECT rect{x, tabs_.top + dip(2), x + width, tabs_.bottom - dip(2)};
        const PanelHit hit{PanelHit::Kind::Page, page};
        if (same_hit(hover_, hit) && !current) canvas.rounded_fill(rect, static_cast<float>(dip(6)), palette_.hover_background, 1.0f);
        canvas.text(label, rect, current ? palette_.text : palette_.composition_text, size, current ? 700 : 400,
                    DT_SINGLELINE | DT_VCENTER | DT_CENTER);
        if (current) {
            const int bar = std::max(dip(12), width - dip(24));
            const RECT underline{rect.left + (width - bar) / 2, rect.bottom - dip(2), rect.left + (width - bar) / 2 + bar, rect.bottom};
            canvas.rounded_fill(underline, static_cast<float>(dip(1)), palette_.focus_bar, 1.0f);
        }
        hits_.emplace_back(rect, hit);
        x += width;
    }
    // Close, and the large window (a box with an arrow out of it).
    const int button = dip(24);
    const int middle = (tabs_.top + tabs_.bottom) / 2;
    const RECT close{tabs_.right - button, middle - button / 2, tabs_.right, middle + button / 2};
    const RECT expand{close.left - button - dip(2), close.top, close.left - dip(2), close.bottom};
    for (const auto& [rect, kind] : {std::pair{expand, PanelHit::Kind::Expand}, std::pair{close, PanelHit::Kind::Close}}) {
        const PanelHit hit{kind, 0};
        if (same_hit(hover_, hit)) canvas.rounded_fill(rect, static_cast<float>(dip(6)), palette_.hover_background, 1.0f);
        hits_.emplace_back(rect, hit);
    }
    const float unit = static_cast<float>(button) / 24.0f;
    const float stroke = std::max(1.2f, unit * 1.5f);
    const auto at = [&](const RECT& rect, float px, float py) { return std::pair{rect.left + px * unit, rect.top + py * unit}; };
    {
        const auto [x1, y1] = at(close, 8, 8);
        const auto [x2, y2] = at(close, 16, 16);
        canvas.line(x1, y1, x2, y2, palette_.control, stroke);
        canvas.line(x1, y2, x2, y1, palette_.control, stroke);
    }
    {
        const RECT box{static_cast<int>(expand.left + 6 * unit), static_cast<int>(expand.top + 8 * unit),
                       static_cast<int>(expand.left + 16 * unit), static_cast<int>(expand.top + 18 * unit)};
        canvas.outline(box, 2 * unit, palette_.control, stroke);
        const auto [ax, ay] = at(expand, 11, 13);
        const auto [bx, by] = at(expand, 18, 6);
        canvas.line(ax, ay, bx, by, palette_.control, stroke);
        canvas.line(bx - 4 * unit, by, bx, by, palette_.control, stroke);
        canvas.line(bx, by, bx, by + 4 * unit, palette_.control, stroke);
    }
    if (x + dip(30) <= expand.left - dip(6)) paint_key_hint(L"Tab", expand.left - dip(6), middle);
}

void ToolPanelWindow::paint_categories() {
    auto& canvas = *active_modern_renderer();
    const float size = static_cast<float>(dip(12));
    const std::wstring side = !view_.message.empty() && !view_.cells.empty() ? view_.message : view_.info;
    int right = categories_.right;
    if (view_.caps_hint && view_.categories.size() > 1)
        right = paint_key_hint(L"Caps", right, (categories_.top + categories_.bottom) / 2) - dip(8);
    if (!side.empty()) {
        const bool notice = !view_.message.empty() && !view_.cells.empty();
        const int width = std::min(canvas.measure_text(side, size) + dip(4),
                                   static_cast<int>(right - categories_.left) / 2);
        if (view_.categories.empty() && notice) {
            canvas.text(side, categories_, palette_.text, size, 400, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
            return;
        }
        const RECT rect{right - width, categories_.top, right, categories_.bottom};
        canvas.text(side, rect, notice ? palette_.text : palette_.hint_text, size, 400, DT_SINGLELINE | DT_VCENTER | DT_RIGHT | DT_END_ELLIPSIS);
        right = rect.left - dip(10);
    }
    if (view_.categories.empty()) return;
    std::vector<int> widths;
    for (size_t index = 0; index < view_.categories.size(); ++index)
        widths.push_back(static_cast<int>(index) == view_.recent_category
                             ? dip(30)
                             : canvas.measure_text(view_.categories[index], size, static_cast<int>(index) == view_.category ? 700 : 400) + dip(14));
    // Start late enough that the current category shows.
    const int chevron = dip(12);
    const int available = right - categories_.left - chevron * 2;
    size_t start = 0;
    if (view_.category > 0) {
        int span = 0;
        start = static_cast<size_t>(view_.category);
        while (start > 0 && span + widths[start] + widths[start - 1] <= available) span += widths[start--];
        // Fill toward the end too, so the bar is not half empty.
        int total = 0;
        for (size_t index = start; index < widths.size(); ++index) total += widths[index];
        while (start > 0 && total + widths[start - 1] <= available) total += widths[--start];
    }
    int x = categories_.left;
    if (start > 0) canvas.text(L"‹", RECT{x, categories_.top, x + chevron, categories_.bottom}, palette_.control, size, 400,
                               DT_SINGLELINE | DT_VCENTER | DT_CENTER);
    x += chevron;
    for (size_t index = start; index < view_.categories.size(); ++index) {
        if (x + widths[index] > right - chevron) {
            canvas.text(L"›", RECT{right - chevron, categories_.top, right, categories_.bottom}, palette_.control, size, 400,
                        DT_SINGLELINE | DT_VCENTER | DT_CENTER);
            break;
        }
        const bool current = static_cast<int>(index) == view_.category;
        const RECT rect{x, categories_.top + dip(2), x + widths[index], categories_.bottom - dip(2)};
        const PanelHit hit{PanelHit::Kind::Category, static_cast<int>(index)};
        if (current) canvas.rounded_fill(rect, static_cast<float>((rect.bottom - rect.top) / 2), palette_.focus_background, 1.0f);
        else if (same_hit(hover_, hit)) canvas.rounded_fill(rect, static_cast<float>((rect.bottom - rect.top) / 2), palette_.hover_background, 1.0f);
        const COLORREF color = current ? palette_.focus_text : palette_.composition_text;
        if (static_cast<int>(index) == view_.recent_category)
            clock_icon(canvas, (rect.left + rect.right) / 2.0f, (rect.top + rect.bottom) / 2.0f, static_cast<float>(dip(6)), color,
                       std::max(1.2f, static_cast<float>(dip(1)) * (current ? 1.5f : 1.3f)));
        else
            canvas.text(view_.categories[index], rect, color, size, current ? 700 : 400, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
        hits_.emplace_back(rect, hit);
        x += widths[index];
    }
}

void ToolPanelWindow::paint_grid() {
    auto& canvas = *active_modern_renderer();
    const int columns = std::max(1, view_.columns);
    const int rows = std::max(1, view_.rows);
    const int cell_width = (content_.right - content_.left - dip(kGutter)) / columns;
    const int cell_height = (content_.bottom - content_.top) / rows;
    const float text_size = static_cast<float>(view_.font == PanelFont::Emoji ? dip(21) : view_.font == PanelFont::Symbol ? dip(18) : dip(13));
    for (size_t index = 0; index < view_.cells.size(); ++index) {
        const auto& cell = view_.cells[index];
        if (cell.text.empty() && cell.image.empty()) continue;
        const int row = static_cast<int>(index) / columns, column = static_cast<int>(index) % columns;
        const RECT rect{content_.left + column * cell_width + dip(1), content_.top + row * cell_height + dip(1),
                        content_.left + (column + 1) * cell_width - dip(1), content_.top + (row + 1) * cell_height - dip(1)};
        const bool highlighted = static_cast<int>(index) == view_.highlight;
        const PanelHit hit{PanelHit::Kind::Cell, static_cast<int>(index)};
        const float radius = static_cast<float>(dip(6));
        if (highlighted) canvas.rounded(rect, radius, palette_.focus_background, palette_.focus_border, 1.0f);
        else if (same_hit(hover_, hit)) canvas.rounded_fill(rect, radius, palette_.hover_background, 1.0f);
        if (!cell.image.empty()) {
            const RECT picture{rect.left + dip(5), rect.top + dip(5), rect.right - dip(5), rect.bottom - dip(5)};
            if (!canvas.image(cell.image, picture))
                canvas.text(cell.text, picture, palette_.hint_text, static_cast<float>(dip(11)), 400,
                            DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_END_ELLIPSIS);
        } else {
            const RECT label{rect.left + dip(4), rect.top, rect.right - dip(4), rect.bottom};
            canvas.text(cell.text, label, highlighted ? palette_.focus_text : palette_.text, text_size, 400,
                        DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_END_ELLIPSIS, font_family(view_.font));
        }
        if (row == view_.numbered_row && column < 10) {
            const std::wstring number = std::to_wstring((column + 1) % 10);
            const RECT corner{rect.left + dip(3), rect.top + dip(1), rect.left + dip(15), rect.top + dip(13)};
            canvas.text(number, corner, highlighted ? palette_.focus_text : palette_.composition_text, static_cast<float>(dip(9)), 600,
                        DT_SINGLELINE | DT_VCENTER);
        }
        if (cell.favorite)
            canvas.ellipse(static_cast<float>(rect.right - dip(5)), static_cast<float>(rect.top + dip(5)), static_cast<float>(dip(2)),
                           static_cast<float>(dip(2)), palette_.mark);
        hits_.emplace_back(rect, hit);
    }
    if (view_.cells.empty() && !view_.message.empty())
        canvas.text(view_.message, RECT{content_.left + dip(20), content_.top, content_.right - dip(20), content_.bottom},
                    palette_.hint_text, static_cast<float>(dip(13)), 400, DT_VCENTER | DT_CENTER);
    // More rows above or below: small chevrons in the gutter.
    const float x = static_cast<float>(content_.right - dip(kGutter) / 2);
    const float unit = static_cast<float>(dip(3));
    if (view_.more_above) {
        const float y = static_cast<float>(content_.top + dip(8));
        canvas.line(x - unit, y + unit / 2, x, y - unit / 2, palette_.control, 1.4f);
        canvas.line(x, y - unit / 2, x + unit, y + unit / 2, palette_.control, 1.4f);
    }
    if (view_.more_below) {
        const float y = static_cast<float>(content_.bottom - dip(8));
        canvas.line(x - unit, y - unit / 2, x, y + unit / 2, palette_.control, 1.4f);
        canvas.line(x, y + unit / 2, x + unit, y - unit / 2, palette_.control, 1.4f);
    }
}

void ToolPanelWindow::paint_list() {
    auto& canvas = *active_modern_renderer();
    const int rows = std::max(1, view_.rows);
    const int row_height = (content_.bottom - content_.top) / rows;
    const float title_size = static_cast<float>(dip(13));
    const float detail_size = static_cast<float>(dip(12));
    for (size_t index = 0; index < view_.cells.size(); ++index) {
        const auto& cell = view_.cells[index];
        const RECT rect{content_.left, content_.top + static_cast<int>(index) * row_height + dip(1), content_.right - dip(kGutter),
                        content_.top + (static_cast<int>(index) + 1) * row_height - dip(1)};
        const bool highlighted = static_cast<int>(index) == view_.highlight;
        const PanelHit hit{PanelHit::Kind::Cell, static_cast<int>(index)};
        const float radius = static_cast<float>(dip(6));
        if (highlighted) canvas.rounded(rect, radius, palette_.focus_background, palette_.focus_border, 1.0f);
        else if (same_hit(hover_, hit)) canvas.rounded_fill(rect, radius, palette_.hover_background, 1.0f);
        int x = rect.left + dip(6);
        if (view_.numbered_list) {
            const int key = dip(18);
            const int top = (rect.top + rect.bottom - key) / 2;
            const RECT keycap{x, top, x + key, top + key};
            const COLORREF ink = highlighted ? palette_.focus_text : palette_.text;
            canvas.rounded_fill(keycap, static_cast<float>(dip(4)), ink, highlighted ? 0.14f : 0.09f);
            canvas.text(std::to_wstring(index + 1), keycap, ink, static_cast<float>(dip(11)), 700, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
            x = keycap.right + dip(8);
        }
        if (!cell.image.empty()) {
            const RECT thumb{x, rect.top + dip(3), x + dip(44), rect.bottom - dip(3)};
            canvas.image(cell.image, thumb);
            x = thumb.right + dip(8);
        }
        int right = rect.right - dip(8);
        if (view_.magnifier) {
            // The magnifier: the whole text, or the picture larger with its words.
            const int button = dip(24);
            const int top = (rect.top + rect.bottom - button) / 2;
            const RECT lens{rect.right - dip(4) - button, top, rect.right - dip(4), top + button};
            const PanelHit lens_hit{PanelHit::Kind::Preview, static_cast<int>(index)};
            if (same_hit(hover_, lens_hit)) canvas.rounded_fill(lens, static_cast<float>(dip(6)), palette_.pressed_background, 1.0f);
            const RECT glyph{lens.left + dip(4), lens.top + dip(4), lens.right - dip(4), lens.bottom - dip(4)};
            canvas.icon(5, glyph, highlighted ? palette_.focus_text : palette_.control, std::max(1.3f, static_cast<float>(dip(1)) * 1.5f));
            hits_.emplace_back(lens, lens_hit);
            right = lens.left - dip(6);
        }
        if (!cell.detail.empty()) {
            const int width = std::min(canvas.measure_text(cell.detail, detail_size), static_cast<int>(rect.right - rect.left) * 2 / 5);
            const RECT detail{right - width, rect.top, right, rect.bottom};
            canvas.text(cell.detail, detail, highlighted ? palette_.focus_text : palette_.hint_text, detail_size, 400,
                        DT_SINGLELINE | DT_VCENTER | DT_RIGHT | DT_END_ELLIPSIS);
            right = detail.left - dip(8);
        }
        if (!cell.tag.empty()) {
            const float tag_size = static_cast<float>(dip(11));
            const int width = canvas.measure_text(cell.tag, tag_size) + dip(10);
            const int height = dip(18);
            const int top = (rect.top + rect.bottom - height) / 2;
            const RECT chip{right - width, top, right, top + height};
            canvas.rounded_fill(chip, static_cast<float>(height / 2), palette_.chip_background, 1.0f);
            canvas.text(cell.tag, chip, palette_.number_text, tag_size, 400, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
            right = chip.left - dip(8);
        }
        canvas.text(cell.text, RECT{x, rect.top, std::max(x, right), rect.bottom}, highlighted ? palette_.focus_text : palette_.text,
                    title_size, 400, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        hits_.emplace_back(rect, hit);
    }
    if (view_.cells.empty() && !view_.message.empty())
        canvas.text(view_.message, RECT{content_.left + dip(20), content_.top, content_.right - dip(20), content_.bottom},
                    palette_.hint_text, static_cast<float>(dip(13)), 400, DT_VCENTER | DT_CENTER);
    const float x = static_cast<float>(content_.right - dip(kGutter) / 2);
    const float unit = static_cast<float>(dip(3));
    if (view_.more_above) {
        const float y = static_cast<float>(content_.top + dip(8));
        canvas.line(x - unit, y + unit / 2, x, y - unit / 2, palette_.control, 1.4f);
        canvas.line(x, y - unit / 2, x + unit, y + unit / 2, palette_.control, 1.4f);
    }
    if (view_.more_below) {
        const float y = static_cast<float>(content_.bottom - dip(8));
        canvas.line(x - unit, y - unit / 2, x, y + unit / 2, palette_.control, 1.4f);
        canvas.line(x, y + unit / 2, x + unit, y - unit / 2, palette_.control, 1.4f);
    }
}

void ToolPanelWindow::paint_preview() {
    auto& canvas = *active_modern_renderer();
    // The header: back, then what this is.
    const float size = static_cast<float>(dip(12));
    const int back_width = canvas.measure_text(L"‹ 返回", size) + dip(14);
    const RECT back{categories_.left, categories_.top + dip(2), categories_.left + back_width, categories_.bottom - dip(2)};
    const PanelHit back_hit{PanelHit::Kind::Preview, -1};
    canvas.rounded_fill(back, static_cast<float>((back.bottom - back.top) / 2),
                        same_hit(hover_, back_hit) ? palette_.pressed_background : palette_.hover_background, 1.0f);
    canvas.text(L"‹ 返回", back, palette_.composition_text, size, 400, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
    hits_.emplace_back(back, back_hit);
    canvas.text(view_.preview_title, RECT{back.right + dip(10), categories_.top, categories_.right, categories_.bottom}, palette_.hint_text, size,
                400, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    const RECT area{content_.left, content_.top + dip(2), content_.right, content_.bottom};
    canvas.rounded(area, static_cast<float>(dip(8)), palette_.chip_background, palette_.chip_border, 1.0f);
    RECT body{area.left + dip(10), area.top + dip(8), area.right - dip(12), area.bottom - dip(8)};
    if (!view_.preview_image.empty()) {
        // The picture as large as it fits; its words, if any, beside it.
        RECT picture = body;
        if (!view_.preview_text.empty()) {
            picture.right = picture.left + (body.right - body.left) * 11 / 20;
            body.left = picture.right + dip(12);
        }
        canvas.image(view_.preview_image, picture);
        if (view_.preview_text.empty()) return;
    }
    // The text, wrapped and scrolled by whole lines; nothing spills past the box.
    const float text_size = static_cast<float>(dip(13));
    const int line = static_cast<int>(text_size * 1.32f + 0.5f);  // Microsoft YaHei's line height
    const auto edge = [](const RECT& rect) {
        return std::vector<D2D1_POINT_2F>{{static_cast<float>(rect.left), static_cast<float>(rect.top)},
                                          {static_cast<float>(rect.right), static_cast<float>(rect.top)},
                                          {static_cast<float>(rect.right), static_cast<float>(rect.bottom)},
                                          {static_cast<float>(rect.left), static_cast<float>(rect.bottom)}};
    };
    canvas.push_layer(edge(body));
    const RECT flow{body.left, body.top - view_.preview_scroll * line, body.right, body.bottom};
    canvas.text(view_.preview_text, flow, palette_.text, text_size, 400, DT_TOP | DT_WORDBREAK);
    canvas.pop_layer();
    const float x = static_cast<float>(area.right - dip(6));
    const float unit = static_cast<float>(dip(3));
    if (view_.preview_scroll > 0) {
        const float y = static_cast<float>(area.top + dip(8));
        canvas.line(x - unit, y + unit / 2, x, y - unit / 2, palette_.control, 1.4f);
        canvas.line(x, y - unit / 2, x + unit, y + unit / 2, palette_.control, 1.4f);
    }
}

void ToolPanelWindow::paint_footer() {
    auto& canvas = *active_modern_renderer();
    const float hint_size = static_cast<float>(dip(11));
    const int hint_width = canvas.measure_text(view_.hint, hint_size);
    const RECT hint{footer_.right - hint_width, footer_.top, footer_.right, footer_.bottom};
    canvas.text(view_.hint, hint, palette_.hint_text, hint_size, 400, DT_SINGLELINE | DT_VCENTER | DT_RIGHT);
    int x = footer_.left;
    if (!view_.prompt.empty()) {
        const RECT box{footer_.left, footer_.top + dip(3), footer_.left + dip(176), footer_.bottom - dip(3)};
        canvas.rounded(box, static_cast<float>((box.bottom - box.top) / 2), palette_.chip_background, palette_.chip_border, 1.0f);
        const float size = static_cast<float>(dip(12));
        const int prompt_width = canvas.measure_text(view_.prompt, size);
        const RECT prompt{box.left + dip(10), box.top, box.left + dip(10) + prompt_width, box.bottom};
        canvas.text(view_.prompt, prompt, palette_.hint_text, size, 400, DT_SINGLELINE | DT_VCENTER);
        const int start = prompt.right + dip(6);
        if (view_.input.empty()) {
            const std::wstring placeholder = view_.prompt == L"搜索" ? L"打拼音" : view_.prompt == L"算式" ? L"如 55+15、123、sj" : L"";
            canvas.text(placeholder, RECT{start + dip(3), box.top, box.right - dip(8), box.bottom}, palette_.control, size, 400,
                        DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        }
        // Keep the end of long input in view, where typing happens.
        const float input_size = static_cast<float>(dip(13));
        std::wstring shown = view_.input;
        while (shown.size() > 1 && canvas.measure_text(shown, input_size) > box.right - dip(12) - start) shown.erase(0, 1);
        const int input_width = shown.empty() ? 0 : canvas.measure_text(shown, input_size);
        canvas.text(shown, RECT{start, box.top, box.right - dip(8), box.bottom}, palette_.text, input_size, 400, DT_SINGLELINE | DT_VCENTER);
        if (view_.input_active) {
            const float caret_x = static_cast<float>(start + input_width + dip(1));
            canvas.line(caret_x, static_cast<float>(box.top + dip(6)), caret_x, static_cast<float>(box.bottom - dip(6)), palette_.caret,
                        static_cast<float>(std::max(1, dip(2))) * 0.75f);
        }
        x = box.right + dip(8);
    }
    // The tray: what Enter will type, each removable by a click.
    const float tray_size = static_cast<float>(dip(15));
    for (size_t index = 0; index < view_.tray.size(); ++index) {
        const int width = std::max(dip(22), canvas.measure_text(view_.tray[index], tray_size, 400, L"Segoe UI Emoji") + dip(10));
        if (x + width > hint.left - dip(24)) {
            canvas.text(L"…", RECT{x, footer_.top, x + dip(16), footer_.bottom}, palette_.hint_text, tray_size, 400,
                        DT_SINGLELINE | DT_VCENTER);
            break;
        }
        const RECT chip{x, footer_.top + dip(3), x + width, footer_.bottom - dip(3)};
        const PanelHit hit{PanelHit::Kind::Tray, static_cast<int>(index)};
        canvas.rounded_fill(chip, static_cast<float>(dip(6)), same_hit(hover_, hit) ? palette_.pressed_background : palette_.focus_background, 1.0f);
        canvas.text(view_.tray[index], chip, palette_.focus_text, tray_size, 400, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_END_ELLIPSIS,
                    L"Segoe UI Emoji");
        hits_.emplace_back(chip, hit);
        x = chip.right + dip(4);
    }
}

PanelHit ToolPanelWindow::hit_at(POINT point) const {
    for (const auto& [rect, hit] : hits_)
        if (PtInRect(&rect, point)) return hit;
    return {};
}

LRESULT CALLBACK ToolPanelWindow::window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* self = reinterpret_cast<ToolPanelWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        self = static_cast<ToolPanelWindow*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
        if (self) self->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    return self ? self->handle_message(hwnd, message, wparam, lparam) : DefWindowProcW(hwnd, message, wparam, lparam);
}

LRESULT ToolPanelWindow::handle_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case WM_MOUSEMOVE: {
            const PanelHit hit = hit_at(POINT{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)});
            if (!same_hit(hit, hover_)) {
                hover_ = hit;
                present();
            }
            TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, hwnd, 0};
            TrackMouseEvent(&track);
            return 0;
        }
        case WM_MOUSELEAVE:
            if (hover_.kind != PanelHit::Kind::None) {
                hover_ = {};
                present();
            }
            return 0;
        case WM_SETCURSOR:
            if (hover_.kind != PanelHit::Kind::None) {
                SetCursor(LoadCursorW(nullptr, IDC_HAND));
                return TRUE;
            }
            break;
        case WM_LBUTTONUP: {
            const PanelHit hit = hit_at(POINT{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)});
            if (hit.kind != PanelHit::Kind::None && click_callback_) {
                // The callback may hide or destroy the panel.
                const auto callback = click_callback_;
                callback(hit);
            }
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT paint{};
            BeginPaint(hwnd, &paint);
            EndPaint(hwnd, &paint);
            return 0;
        }
        case WM_NCDESTROY:
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            if (hwnd_ == hwnd) hwnd_ = nullptr;
            break;
        default:
            break;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

}  // namespace localpinyin
