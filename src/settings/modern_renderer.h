#pragma once

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

#include <map>
#include <string_view>
#include <utility>

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
    void ellipse(float x, float y, float rx, float ry, COLORREF color);
    void line(float x1, float y1, float x2, float y2, COLORREF color, float stroke = 1.5f);
    void text(std::wstring_view text, const RECT& bounds, COLORREF color, float size,
              int weight = 400, UINT flags = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    [[nodiscard]] int measure_text(std::wstring_view text, float size, int weight = 400);
    void icon(int kind, const RECT& bounds, COLORREF color, float stroke = 1.6f);

private:
    struct Target {
        Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> target;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    };
    bool initialize(bool premultiplied_alpha);
    void color(COLORREF color);
    IDWriteTextFormat* format(float size, int weight);

    Microsoft::WRL::ComPtr<ID2D1Factory> factory_;
    Microsoft::WRL::ComPtr<IDWriteFactory> write_factory_;
    Target opaque_;
    Target alpha_;
    Target* active_ = nullptr;
    Microsoft::WRL::ComPtr<ID2D1StrokeStyle> stroke_style_;
    std::map<std::pair<int, int>, Microsoft::WRL::ComPtr<IDWriteTextFormat>> formats_;
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
