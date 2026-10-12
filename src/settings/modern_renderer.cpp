#include "modern_renderer.h"

#include <algorithm>
#include <cmath>

namespace localpinyin {
namespace {
thread_local ModernRenderer* current_renderer = nullptr;
D2D1_RECT_F rectf(const RECT& r, float inset = 0.0f) {
    return D2D1::RectF(static_cast<float>(r.left) + inset, static_cast<float>(r.top) + inset,
                       static_cast<float>(r.right) - inset, static_cast<float>(r.bottom) - inset);
}
}

bool ModernRenderer::initialize() {
    if (!factory_ && FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
            __uuidof(ID2D1Factory), nullptr, reinterpret_cast<void**>(factory_.GetAddressOf())))) return false;
    if (!write_factory_ && FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(write_factory_.GetAddressOf())))) return false;
    if (!target_) {
        const auto properties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 96, 96);
        if (FAILED(factory_->CreateDCRenderTarget(&properties, &target_))) return false;
        target_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        target_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
        if (FAILED(target_->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f), &brush_))) {
            target_.Reset();
            return false;
        }
    }
    if (!stroke_style_) {
        const auto properties = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND,
            D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND);
        factory_->CreateStrokeStyle(properties, nullptr, 0, &stroke_style_);
    }
    return true;
}

bool ModernRenderer::begin(HDC dc, const RECT& bounds) {
    if (drawing_ || !dc || bounds.right <= bounds.left || bounds.bottom <= bounds.top || !initialize()) return false;
    if (FAILED(target_->BindDC(dc, &bounds))) return false;
    target_->SetTransform(D2D1::Matrix3x2F::Translation(-static_cast<float>(bounds.left),
                                                     -static_cast<float>(bounds.top)));
    target_->BeginDraw();
    drawing_ = true;
    return true;
}

void ModernRenderer::finish() {
    if (!drawing_) return;
    drawing_ = false;
    const HRESULT result = target_->EndDraw();
    if (result == D2DERR_RECREATE_TARGET) {
        brush_.Reset();
        target_.Reset();
    }
}

void ModernRenderer::color(COLORREF value) {
    brush_->SetColor(D2D1::ColorF(GetRValue(value) / 255.0f, GetGValue(value) / 255.0f,
                                 GetBValue(value) / 255.0f));
}
void ModernRenderer::fill(const RECT& bounds, COLORREF value) {
    color(value);
    target_->FillRectangle(rectf(bounds), brush_.Get());
}
void ModernRenderer::rounded(const RECT& bounds, float radius, COLORREF fill_value,
                             COLORREF border, float stroke) {
    const auto geometry = D2D1::RoundedRect(rectf(bounds, stroke / 2), radius, radius);
    color(fill_value);
    target_->FillRoundedRectangle(geometry, brush_.Get());
    if (border != fill_value) {
        color(border);
        target_->DrawRoundedRectangle(geometry, brush_.Get(), stroke);
    }
}
void ModernRenderer::outline(const RECT& bounds, float radius, COLORREF value, float stroke) {
    color(value);
    target_->DrawRoundedRectangle(D2D1::RoundedRect(rectf(bounds, stroke / 2), radius, radius),
                                   brush_.Get(), stroke, stroke_style_.Get());
}
void ModernRenderer::ellipse(float x, float y, float rx, float ry, COLORREF value) {
    color(value);
    target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), rx, ry), brush_.Get());
}
void ModernRenderer::line(float x1, float y1, float x2, float y2, COLORREF value, float stroke) {
    color(value);
    target_->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), brush_.Get(), stroke, stroke_style_.Get());
}

IDWriteTextFormat* ModernRenderer::format(float size, int weight) {
    const std::pair<int, int> key{static_cast<int>(std::lround(size * 10)), weight};
    auto found = formats_.find(key);
    if (found != formats_.end()) return found->second.Get();
    Microsoft::WRL::ComPtr<IDWriteTextFormat> created;
    if (FAILED(write_factory_->CreateTextFormat(L"Microsoft YaHei UI", nullptr,
        static_cast<DWRITE_FONT_WEIGHT>(weight), DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, size, L"zh-CN", &created))) return nullptr;
    created->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    return formats_.emplace(key, std::move(created)).first->second.Get();
}

void ModernRenderer::text(std::wstring_view value, const RECT& bounds, COLORREF value_color,
                          float size, int weight, UINT flags) {
    if (value.empty() || bounds.right <= bounds.left || bounds.bottom <= bounds.top) return;
    IDWriteTextFormat* text_format = format(size, weight);
    if (!text_format) return;
    Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
    if (FAILED(write_factory_->CreateTextLayout(value.data(), static_cast<UINT32>(value.size()), text_format,
        static_cast<float>(bounds.right - bounds.left), static_cast<float>(bounds.bottom - bounds.top), &layout))) return;
    layout->SetTextAlignment((flags & DT_CENTER) ? DWRITE_TEXT_ALIGNMENT_CENTER :
                              (flags & DT_RIGHT) ? DWRITE_TEXT_ALIGNMENT_TRAILING : DWRITE_TEXT_ALIGNMENT_LEADING);
    layout->SetParagraphAlignment((flags & DT_VCENTER) ? DWRITE_PARAGRAPH_ALIGNMENT_CENTER :
                                   (flags & DT_BOTTOM) ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    layout->SetWordWrapping((flags & DT_SINGLELINE) ? DWRITE_WORD_WRAPPING_NO_WRAP : DWRITE_WORD_WRAPPING_WRAP);
    if (flags & DT_END_ELLIPSIS) {
        Microsoft::WRL::ComPtr<IDWriteInlineObject> sign;
        if (SUCCEEDED(write_factory_->CreateEllipsisTrimmingSign(text_format, &sign))) {
            const DWRITE_TRIMMING trim{DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0};
            layout->SetTrimming(&trim, sign.Get());
        }
    }
    color(value_color);
    target_->DrawTextLayout(D2D1::Point2F(static_cast<float>(bounds.left), static_cast<float>(bounds.top)),
                             layout.Get(), brush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

int ModernRenderer::measure_text(std::wstring_view value, float size, int weight) {
    if (value.empty() || !initialize()) return 0;
    IDWriteTextFormat* text_format = format(size, weight);
    if (!text_format) return 0;
    Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
    if (FAILED(write_factory_->CreateTextLayout(value.data(), static_cast<UINT32>(value.size()),
        text_format, 100000, 100000, &layout))) return 0;
    DWRITE_TEXT_METRICS metrics{};
    if (FAILED(layout->GetMetrics(&metrics))) return 0;
    return static_cast<int>(std::ceil(metrics.widthIncludingTrailingWhitespace)) + 1;
}

void ModernRenderer::icon(int kind, const RECT& bounds, COLORREF value, float stroke) {
    const float unit = static_cast<float>(std::min(bounds.right - bounds.left, bounds.bottom - bounds.top)) / 24;
    const float x = static_cast<float>(bounds.left);
    const float y = static_cast<float>(bounds.top);
    const auto seg = [&](float x1, float y1, float x2, float y2) {
        line(x + x1 * unit, y + y1 * unit, x + x2 * unit, y + y2 * unit, value, stroke);
    };
    const auto box = [&](int l, int t, int r, int b, float radius) {
        const RECT rr{static_cast<int>(x + l * unit), static_cast<int>(y + t * unit),
                      static_cast<int>(x + r * unit), static_cast<int>(y + b * unit)};
        outline(rr, radius * unit, value, stroke);
    };
    if (kind == 0) {
        box(3, 3, 21, 21, 4); seg(4, 9, 20, 9); seg(9, 10, 9, 20);
    } else if (kind == 1) {
        box(2, 4, 22, 19, 4); seg(6, 9, 18, 9); seg(6, 14, 13, 14); seg(6, 19, 6, 22);
    } else if (kind == 2) {
        box(4, 3, 21, 21, 3); seg(9, 4, 9, 20); seg(12, 9, 17, 9); seg(12, 14, 17, 14);
    } else if (kind == 3) {
        seg(12, 2, 15, 9); seg(15, 9, 22, 12); seg(22, 12, 15, 15); seg(15, 15, 12, 22);
        seg(12, 22, 9, 15); seg(9, 15, 2, 12); seg(2, 12, 9, 9); seg(9, 9, 12, 2);
    } else if (kind == 4) {
        box(3, 3, 21, 21, 9); seg(12, 11, 12, 17); ellipse(x + 12 * unit, y + 7 * unit, unit, unit, value);
    } else if (kind == 5) {
        box(3, 3, 17, 17, 7); seg(16, 16, 22, 22);
    } else if (kind == 6) {
        seg(5, 12, 10, 17); seg(10, 17, 20, 6);
    } else if (kind == 7) {
        seg(9, 5, 16, 12); seg(16, 12, 9, 19);
    } else {
        box(6, 6, 18, 18, 6); seg(12, 2, 12, 5); seg(12, 19, 12, 22);
        seg(2, 12, 5, 12); seg(19, 12, 22, 12);
    }
}

ModernRenderer* active_modern_renderer() noexcept { return current_renderer; }
ModernRenderScope::ModernRenderScope(ModernRenderer& renderer, HDC dc, const RECT& bounds)
    : renderer_(renderer), previous_(current_renderer), active_(renderer.begin(dc, bounds)) {
    // A failed nested bind must use the GDI fallback, never the parent's DC.
    current_renderer = active_ ? &renderer_ : nullptr;
}
ModernRenderScope::~ModernRenderScope() { finish(); }
void ModernRenderScope::finish() {
    if (finished_) return;
    if (active_) renderer_.finish();
    current_renderer = previous_;
    active_ = false;
    finished_ = true;
}

}  // namespace localpinyin
