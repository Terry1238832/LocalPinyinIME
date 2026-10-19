#pragma once

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

#include <map>
#include <string>
#include <tuple>
#include <string_view>
#include <utility>
#include <vector>

namespace localpinyin {

// Windows supplies both rendering libraries. No browser, bundled fonts, or
// graphics runtime is distributed with the input method.
class ModernRenderer {
public:
    // With `premultiplied_alpha`, the DC must select a 32-bit DIB; drawing keeps
    // per-pixel transparency (for UpdateLayeredWindow) and text is grayscale
    // antialiased, since ClearType needs an opaque destination.
    bool begin(HDC dc, const RECT& bounds, bool premultiplied_alpha = false);
    void finish();
    [[nodiscard]] bool drawing() const noexcept { return drawing_; }

    void fill(const RECT& bounds, COLORREF color);
    void rounded(const RECT& bounds, float radius, COLORREF fill, COLORREF border,
                 float stroke = 1.0f);
    void outline(const RECT& bounds, float radius, COLORREF color, float stroke = 1.5f);
    // A rounded fill at partial opacity, e.g. a soft glow under a highlight.
    void rounded_fill(const RECT& bounds, float radius, COLORREF color, float opacity);
    // A rounded fill shading from `top` to `bottom`.
    void rounded_gradient(const RECT& bounds, float radius, COLORREF top, COLORREF bottom);
    void ellipse(float x, float y, float rx, float ry, COLORREF color);
    // A filled polygon at partial opacity (a marker stroke, a turning flap).
    void polygon(const std::vector<D2D1_POINT_2F>& points, COLORREF color, float opacity = 1.0f);
    // A filled polygon shading from `top` at `top_y` to `bottom` at `bottom_y`.
    void polygon_gradient(const std::vector<D2D1_POINT_2F>& points, COLORREF top, COLORREF bottom,
                          float top_y, float bottom_y, float opacity = 1.0f);
    // What is drawn until the matching pop_layer shows only inside `shape`
    // (everywhere when it is empty), at `opacity`.
    void push_layer(const std::vector<D2D1_POINT_2F>& shape, float opacity = 1.0f);
    // The same, showing only what lies in `area` outside `hole`.
    void push_layer_outside(const std::vector<D2D1_POINT_2F>& hole, const RECT& area);
    void pop_layer();
    void line(float x1, float y1, float x2, float y2, COLORREF color, float stroke = 1.5f);
    // `family` defaults to Microsoft YaHei UI.
    void text(std::wstring_view text, const RECT& bounds, COLORREF color, float size,
              int weight = 400, UINT flags = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS,
              std::wstring_view family = {});
    [[nodiscard]] int measure_text(std::wstring_view text, float size, int weight = 400,
                                   std::wstring_view family = {});
    // Distance from the top of a single text line to its baseline, so text in
    // several sizes can share one baseline (draw with DT_TOP at baseline - this).
    [[nodiscard]] float baseline(float size, int weight = 400, std::wstring_view family = {});
    // One text layout with per-range colors (e.g. pinyin syllables and their
    // separators). Returns the drawn text width in pixels, 0 when nothing drew.
    struct ColorRun { UINT32 start = 0; UINT32 length = 0; COLORREF color = 0; };
    float text_runs(std::wstring_view text, const RECT& bounds, COLORREF base, const std::vector<ColorRun>& runs,
                    float size, int weight = 400, UINT flags = DT_SINGLELINE | DT_VCENTER,
                    std::wstring_view family = {});
    void icon(int kind, const RECT& bounds, COLORREF color, float stroke = 1.6f);

private:
    struct Target {
        Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> target;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    };
    bool initialize(bool premultiplied_alpha);
    void color(COLORREF color);
    Microsoft::WRL::ComPtr<ID2D1PathGeometry> path(const std::vector<D2D1_POINT_2F>& points);
    IDWriteTextFormat* format(float size, int weight, std::wstring_view family = {});

    Microsoft::WRL::ComPtr<ID2D1Factory> factory_;
    Microsoft::WRL::ComPtr<IDWriteFactory> write_factory_;
    Target opaque_;
    Target alpha_;
    Target* active_ = nullptr;
    Microsoft::WRL::ComPtr<ID2D1StrokeStyle> stroke_style_;
    std::map<std::tuple<int, int, std::wstring>, Microsoft::WRL::ComPtr<IDWriteTextFormat>> formats_;
    std::map<std::tuple<int, int, std::wstring>, float> baselines_;
    // Open layers and their masks, kept alive until popped.
    std::vector<std::pair<Microsoft::WRL::ComPtr<ID2D1Layer>, Microsoft::WRL::ComPtr<ID2D1PathGeometry>>> layers_;
    bool drawing_ = false;
};

// Existing native controls retain keyboard navigation, accessibility names,
// and TSF integration, while custom surfaces use one DirectWrite path.
ModernRenderer* active_modern_renderer() noexcept;
class ModernRenderScope {
public:
    ModernRenderScope(ModernRenderer& renderer, HDC dc, const RECT& bounds, bool premultiplied_alpha = false);
    ~ModernRenderScope();
    void finish();
    [[nodiscard]] bool active() const noexcept { return active_; }
private:
    ModernRenderer& renderer_;
    ModernRenderer* previous_ = nullptr;
    bool active_ = false;
    bool finished_ = false;
};

}  // namespace localpinyin
