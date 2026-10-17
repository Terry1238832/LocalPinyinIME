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

bool ModernRenderer::initialize(bool premultiplied_alpha) {
    if (!factory_ && FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
            __uuidof(ID2D1Factory), nullptr, reinterpret_cast<void**>(factory_.GetAddressOf())))) return false;
    if (!write_factory_ && FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(write_factory_.GetAddressOf())))) return false;
    Target& slot = premultiplied_alpha ? alpha_ : opaque_;
    if (!slot.target) {
        const auto properties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                              premultiplied_alpha ? D2D1_ALPHA_MODE_PREMULTIPLIED : D2D1_ALPHA_MODE_IGNORE), 96, 96);
        if (FAILED(factory_->CreateDCRenderTarget(&properties, &slot.target))) return false;
        slot.target->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        slot.target->SetTextAntialiasMode(premultiplied_alpha ? D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE
                                                              : D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
        if (FAILED(slot.target->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f), &slot.brush))) {
            slot.target.Reset();
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

bool ModernRenderer::begin(HDC dc, const RECT& bounds, bool premultiplied_alpha) {
    if (drawing_ || !dc || bounds.right <= bounds.left || bounds.bottom <= bounds.top ||
        !initialize(premultiplied_alpha)) return false;
    active_ = premultiplied_alpha ? &alpha_ : &opaque_;
    if (FAILED(active_->target->BindDC(dc, &bounds))) return false;
    active_->target->SetTransform(D2D1::Matrix3x2F::Translation(-static_cast<float>(bounds.left),
                                                               -static_cast<float>(bounds.top)));
    active_->target->BeginDraw();
    if (premultiplied_alpha) active_->target->Clear(D2D1::ColorF(0, 0, 0, 0));
    drawing_ = true;
    return true;
}

void ModernRenderer::finish() {
    if (!drawing_) return;
    drawing_ = false;
    const HRESULT result = active_->target->EndDraw();
    if (result == D2DERR_RECREATE_TARGET) {
        active_->brush.Reset();
        active_->target.Reset();
    }
}

void ModernRenderer::color(COLORREF value) {
    active_->brush->SetColor(D2D1::ColorF(GetRValue(value) / 255.0f, GetGValue(value) / 255.0f,
                                         GetBValue(value) / 255.0f));
}
void ModernRenderer::fill(const RECT& bounds, COLORREF value) {
    color(value);
    active_->target->FillRectangle(rectf(bounds), active_->brush.Get());
}
void ModernRenderer::rounded(const RECT& bounds, float radius, COLORREF fill_value,
                             COLORREF border, float stroke) {
    const auto geometry = D2D1::RoundedRect(rectf(bounds, stroke / 2), radius, radius);
    color(fill_value);
    active_->target->FillRoundedRectangle(geometry, active_->brush.Get());
    if (border != fill_value) {
        color(border);
        active_->target->DrawRoundedRectangle(geometry, active_->brush.Get(), stroke);
    }
}
void ModernRenderer::rounded_fill(const RECT& bounds, float radius, COLORREF value, float opacity) {
    color(value);
    active_->brush->SetOpacity(opacity);
    active_->target->FillRoundedRectangle(D2D1::RoundedRect(rectf(bounds, 0), radius, radius), active_->brush.Get());
    active_->brush->SetOpacity(1.0f);
}
void ModernRenderer::rounded_gradient(const RECT& bounds, float radius, COLORREF top, COLORREF bottom) {
    const auto rgba = [](COLORREF value) {
        return D2D1::ColorF(GetRValue(value) / 255.0f, GetGValue(value) / 255.0f, GetBValue(value) / 255.0f, 1.0f);
    };
    const D2D1_GRADIENT_STOP stops[] = {{0.0f, rgba(top)}, {1.0f, rgba(bottom)}};
    Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> collection;
    Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> brush;
    if (FAILED(active_->target->CreateGradientStopCollection(stops, 2, &collection)) ||
        FAILED(active_->target->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(D2D1::Point2F(0, static_cast<float>(bounds.top)),
                                                D2D1::Point2F(0, static_cast<float>(bounds.bottom))),
            collection.Get(), &brush))) {
        rounded(bounds, radius, bottom, bottom);
        return;
    }
    active_->target->FillRoundedRectangle(D2D1::RoundedRect(rectf(bounds, 0), radius, radius), brush.Get());
}
void ModernRenderer::outline(const RECT& bounds, float radius, COLORREF value, float stroke) {
    color(value);
    active_->target->DrawRoundedRectangle(D2D1::RoundedRect(rectf(bounds, stroke / 2), radius, radius),
                                   active_->brush.Get(), stroke, stroke_style_.Get());
}
void ModernRenderer::ellipse(float x, float y, float rx, float ry, COLORREF value) {
    color(value);
    active_->target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), rx, ry), active_->brush.Get());
}
void ModernRenderer::line(float x1, float y1, float x2, float y2, COLORREF value, float stroke) {
    color(value);
    active_->target->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), active_->brush.Get(), stroke, stroke_style_.Get());
}

IDWriteTextFormat* ModernRenderer::format(float size, int weight, std::wstring_view family) {
    const std::wstring name = family.empty() ? std::wstring(L"Microsoft YaHei UI") : std::wstring(family);
    const std::tuple<int, int, std::wstring> key{static_cast<int>(std::lround(size * 10)), weight, name};
    auto found = formats_.find(key);
    if (found != formats_.end()) return found->second.Get();
    Microsoft::WRL::ComPtr<IDWriteTextFormat> created;
    if (FAILED(write_factory_->CreateTextFormat(name.c_str(), nullptr,
        static_cast<DWRITE_FONT_WEIGHT>(weight), DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, size, L"zh-CN", &created))) return nullptr;
    created->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    return formats_.emplace(key, std::move(created)).first->second.Get();
}

void ModernRenderer::text(std::wstring_view value, const RECT& bounds, COLORREF value_color,
                          float size, int weight, UINT flags, std::wstring_view family) {
    if (value.empty() || bounds.right <= bounds.left || bounds.bottom <= bounds.top) return;
    IDWriteTextFormat* text_format = format(size, weight, family);
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
    active_->target->DrawTextLayout(D2D1::Point2F(static_cast<float>(bounds.left), static_cast<float>(bounds.top)),
                             layout.Get(), active_->brush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

float ModernRenderer::text_runs(std::wstring_view value, const RECT& bounds, COLORREF base,
                                const std::vector<ColorRun>& runs, float size, int weight, UINT flags,
                                std::wstring_view family) {
    if (value.empty() || bounds.right <= bounds.left || bounds.bottom <= bounds.top) return 0;
    IDWriteTextFormat* text_format = format(size, weight, family);
    if (!text_format) return 0;
    Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
    if (FAILED(write_factory_->CreateTextLayout(value.data(), static_cast<UINT32>(value.size()), text_format,
        static_cast<float>(bounds.right - bounds.left), static_cast<float>(bounds.bottom - bounds.top), &layout))) return 0;
    layout->SetParagraphAlignment((flags & DT_VCENTER) ? DWRITE_PARAGRAPH_ALIGNMENT_CENTER : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    // Each run's brush rides on the layout as its drawing effect.
    std::vector<Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> brushes;
    for (const auto& run : runs) {
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
        const D2D1_COLOR_F rgba{GetRValue(run.color) / 255.0f, GetGValue(run.color) / 255.0f, GetBValue(run.color) / 255.0f, 1.0f};
        if (FAILED(active_->target->CreateSolidColorBrush(rgba, &brush))) continue;
        layout->SetDrawingEffect(brush.Get(), DWRITE_TEXT_RANGE{run.start, run.length});
        brushes.push_back(std::move(brush));
    }
    color(base);
    active_->target->DrawTextLayout(D2D1::Point2F(static_cast<float>(bounds.left), static_cast<float>(bounds.top)),
                                    layout.Get(), active_->brush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
    DWRITE_TEXT_METRICS metrics{};
    return SUCCEEDED(layout->GetMetrics(&metrics)) ? metrics.widthIncludingTrailingWhitespace : 0.0f;
}

int ModernRenderer::measure_text(std::wstring_view value, float size, int weight, std::wstring_view family) {
    if (value.empty() || !initialize(false)) return 0;
    IDWriteTextFormat* text_format = format(size, weight, family);
    if (!text_format) return 0;
    Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
    if (FAILED(write_factory_->CreateTextLayout(value.data(), static_cast<UINT32>(value.size()),
        text_format, 100000, 100000, &layout))) return 0;
    DWRITE_TEXT_METRICS metrics{};
    if (FAILED(layout->GetMetrics(&metrics))) return 0;
    return static_cast<int>(std::ceil(metrics.widthIncludingTrailingWhitespace)) + 1;
}

float ModernRenderer::baseline(float size, int weight, std::wstring_view family) {
    const std::wstring name = family.empty() ? std::wstring(L"Microsoft YaHei UI") : std::wstring(family);
    const std::tuple<int, int, std::wstring> key{static_cast<int>(std::lround(size * 10)), weight, name};
    const auto found = baselines_.find(key);
    if (found != baselines_.end()) return found->second;
    float result = size * 0.8f;
    IDWriteTextFormat* text_format = initialize(false) ? format(size, weight, family) : nullptr;
    Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
    if (text_format && SUCCEEDED(write_factory_->CreateTextLayout(L"国Ag", 3, text_format, 100000, 100000, &layout))) {
        DWRITE_LINE_METRICS line{};
        UINT32 lines = 0;
        if (SUCCEEDED(layout->GetLineMetrics(&line, 1, &lines)) && lines) result = line.baseline;
    }
    baselines_.emplace(key, result);
    return result;
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
ModernRenderScope::ModernRenderScope(ModernRenderer& renderer, HDC dc, const RECT& bounds, bool premultiplied_alpha)
    : renderer_(renderer), previous_(current_renderer), active_(renderer.begin(dc, bounds, premultiplied_alpha)) {
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
