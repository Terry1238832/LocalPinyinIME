#include "paint_core.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace localpinyin {

void paint_candidate_window(const CandidatePaintContext& context) {
    switch (context.layout.style) {
        case CandidateStyle::CursorRail: candidate_styles::paint_cursor_rail(context); break;
        case CandidateStyle::ModernType: candidate_styles::paint_modern_type(context); break;
        case CandidateStyle::FloatingGlass: candidate_styles::paint_floating_glass(context); break;
        case CandidateStyle::Capsule: candidate_styles::paint_capsule(context); break;
        case CandidateStyle::ColumnList: candidate_styles::paint_column_list(context); break;
        case CandidateStyle::SplitFlap: candidate_styles::paint_split_flap(context); break;
        case CandidateStyle::Highlighter: candidate_styles::paint_highlighter(context); break;
        case CandidateStyle::AdaptiveFlow:
        default: candidate_styles::paint_adaptive_flow(context); break;
    }
}

RECT candidate_marker_target(const CandidateLayoutResult& layout, std::optional<size_t> hovered) {
    if (!layout.visible) return RECT{};
    switch (layout.style) {
        case CandidateStyle::CursorRail: return candidate_styles::marker_cursor_rail(layout, hovered);
        case CandidateStyle::ModernType: return candidate_styles::marker_modern_type(layout, hovered);
        case CandidateStyle::FloatingGlass: return candidate_styles::marker_floating_glass(layout, hovered);
        case CandidateStyle::Capsule: return candidate_styles::marker_capsule(layout, hovered);
        case CandidateStyle::ColumnList: return candidate_styles::marker_column_list(layout, hovered);
        case CandidateStyle::SplitFlap: return candidate_styles::marker_split_flap(layout, hovered);
        case CandidateStyle::Highlighter: return candidate_styles::marker_highlighter(layout, hovered);
        case CandidateStyle::AdaptiveFlow:
        default: return candidate_styles::marker_adaptive_flow(layout, hovered);
    }
}

RECT candidate_track_target(const CandidateLayoutResult& layout) {
    return layout.visible && layout.style == CandidateStyle::CursorRail ? candidate_styles::track_cursor_rail(layout)
                                                                        : RECT{};
}

bool candidate_style_reveals(CandidateStyle style) noexcept {
    return style == CandidateStyle::SplitFlap || style == CandidateStyle::Highlighter ||
           style == CandidateStyle::AdaptiveFlow;
}

ULONGLONG candidate_style_motion_ms(CandidateStyle style) noexcept {
    // Flow's drop needs the time to stretch out and gather again.
    return style == CandidateStyle::AdaptiveFlow ? 380 : 140;
}

RECT candidate_marker_frame(CandidateStyle style, const RECT& from, const RECT& to, float t) noexcept {
    t = std::clamp(t, 0.0f, 1.0f);
    if (style == CandidateStyle::AdaptiveFlow) return candidate_styles::flow_drop_frame(from, to, t);
    // Ease out: quick to leave, precise to arrive; no overshoot.
    const float eased = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
    const auto mix = [eased](LONG a, LONG b) { return static_cast<LONG>(std::lround(a + (b - a) * eased)); };
    return RECT{mix(from.left, to.left), mix(from.top, to.top), mix(from.right, to.right), mix(from.bottom, to.bottom)};
}

namespace candidate_paint {

float unit(const CandidateLayoutResult& layout) noexcept {
    return static_cast<float>(layout.dpi) / 96.0f;
}

const CandidateItemLayout* selected_item(const CandidateLayoutResult& layout) noexcept {
    for (const auto& item : layout.items) if (item.selected) return &item;
    for (const auto& item : layout.items) if (item.featured) return &item;
    return layout.items.empty() ? nullptr : &layout.items.front();
}

const CandidateItemLayout* item_for(const CandidateLayoutResult& layout, size_t candidate_index) noexcept {
    for (const auto& item : layout.items) if (item.candidate_index == candidate_index) return &item;
    return nullptr;
}

bool is_empty(const RECT& rect) noexcept {
    return rect.right <= rect.left || rect.bottom <= rect.top;
}

void query_text(const CandidatePaintContext& context, const RECT& label, float size, int weight,
                COLORREF base, COLORREF separator, COLORREF caret) {
    auto& canvas = context.canvas;
    const float u = unit(context.layout);
    const float caret_gap = (std::max)(2.0f, 2.0f * u);
    const std::wstring& shown = context.state.query;
    std::vector<ModernRenderer::ColorRun> runs;
    for (UINT32 index = 0; index < shown.size(); ++index) {
        if (shown[index] == L'\'') runs.push_back({index, 1, separator});
    }
    const float width = canvas.text_runs(shown, label, base, runs, size, weight, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
    if (width > 0 && context.state.show_caret) {
        const float x = std::round(label.left + width + caret_gap) + 0.5f;
        const float cy = (label.top + label.bottom) / 2.0f;
        const float half = size * 0.55f;
        canvas.line(x, cy - half, x, cy + half, caret, (std::max)(1.5f, 1.5f * u));
    }
}

namespace {

void page_button(const CandidatePaintContext& context, int direction, float radius) {
    const auto& layout = context.layout;
    const auto& palette = context.palette;
    const auto& state = context.state;
    const RECT rect = direction < 0 ? layout.previous_page_rect : layout.next_page_rect;
    const bool enabled = direction < 0 ? layout.has_previous_page : layout.has_next_page;
    const bool hot = enabled && state.hot_page_direction == direction;
    if (enabled && (hot || state.pressed_page_direction == direction)) {
        const COLORREF fill = state.pressed_page_direction == direction ? palette.pressed_background
                                                                         : palette.hover_background;
        context.canvas.rounded(rect, radius, fill, fill);
    }
    const COLORREF color = !enabled ? palette.control_disabled : hot ? palette.text : palette.control;
    const float cx = (rect.left + rect.right) / 2.0f;
    const float cy = (rect.top + rect.bottom) / 2.0f;
    const float u = unit(layout);
    const float arm = (std::max)(3.0f, std::round(4.0f * u));
    const float stroke = (std::max)(1.25f, 1.5f * u);
    const float tip = cx + (direction < 0 ? -arm / 2 : arm / 2);
    const float back = cx + (direction < 0 ? arm / 2 : -arm / 2);
    context.canvas.line(back, cy - arm, tip, cy, color, stroke);
    context.canvas.line(tip, cy, back, cy + arm, color, stroke);
}

}  // namespace

void page_controls(const CandidatePaintContext& context, float radius, COLORREF accent) {
    const auto& layout = context.layout;
    if (!layout.paging_visible) return;
    page_button(context, -1, radius);
    page_button(context, 1, radius);
    const RECT& slot = layout.page_indicator_rect;
    const size_t pages = layout.page_indices.size();
    const float u = unit(layout);
    if (pages <= 3) {
        const float dot = 4.0f * u, active = 12.0f * u, gap = 3.0f * u;
        const float total = active + (pages - 1) * (dot + gap);
        float x = (slot.left + slot.right) / 2.0f - total / 2.0f;
        const float cy = (slot.top + slot.bottom) / 2.0f;
        for (size_t page = 0; page < pages; ++page) {
            const bool current = page == layout.current_page;
            const float w = current ? active : dot;
            const RECT pill{static_cast<LONG>(std::lround(x)), static_cast<LONG>(std::lround(cy - dot / 2)),
                            static_cast<LONG>(std::lround(x + w)), static_cast<LONG>(std::lround(cy + dot / 2))};
            const COLORREF color = current ? accent : context.palette.control_disabled;
            context.canvas.rounded(pill, dot / 2, color, color);
            x += w + gap;
        }
    } else {
        const std::wstring page = std::to_wstring(layout.current_page + 1) + L"/" + std::to_wstring(pages);
        context.canvas.text(page, slot, context.palette.control, 11.0f * u, FW_NORMAL,
                            DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
    }
}

void settings_gear(const CandidatePaintContext& context, float radius) {
    const auto& layout = context.layout;
    const auto& palette = context.palette;
    const auto& state = context.state;
    if (!layout.action_button_visible) return;
    auto& canvas = context.canvas;
    const RECT rect = layout.action_button_rect;
    // The hollow parts of the gear are drawn in the fill behind them.
    COLORREF fill = palette.background;
    if (state.settings_hot || state.settings_pressed) {
        fill = state.settings_pressed ? palette.pressed_background : palette.hover_background;
        canvas.rounded(rect, radius, fill, fill);
    } else if (palette.surface_top != palette.background) {
        // On a shaded surface, match the shade at the gear's height.
        const RECT& panel = layout.panel_rect;
        const float t = std::clamp(((rect.top + rect.bottom) / 2.0f - panel.top) /
                                   (std::max)(1.0f, static_cast<float>(panel.bottom - panel.top)), 0.0f, 1.0f);
        const auto mix = [t](int a, int b) { return static_cast<int>(std::lround(a + (b - a) * t)); };
        fill = RGB(mix(GetRValue(palette.surface_top), GetRValue(palette.background)),
                   mix(GetGValue(palette.surface_top), GetGValue(palette.background)),
                   mix(GetBValue(palette.surface_top), GetBValue(palette.background)));
    }
    // Gear: eight rounded teeth around a ring with a hollow centre.
    const COLORREF color = state.settings_hot ? palette.text : palette.control;
    const float cx = (rect.left + rect.right) / 2.0f;
    const float cy = (rect.top + rect.bottom) / 2.0f;
    const float u = (rect.right - rect.left) / 24.0f;
    const float stroke = (std::max)(1.2f, u * 1.5f);
    for (int tooth = 0; tooth < 8; ++tooth) {
        const float angle = tooth * 3.14159265f / 4.0f;
        const float c = std::cos(angle), s = std::sin(angle);
        canvas.line(cx + c * u * 5.0f, cy + s * u * 5.0f, cx + c * u * 7.2f, cy + s * u * 7.2f, color, stroke * 1.35f);
    }
    canvas.ellipse(cx, cy, u * 5.6f, u * 5.6f, color);
    canvas.ellipse(cx, cy, u * 5.6f - stroke, u * 5.6f - stroke, fill);
    canvas.ellipse(cx, cy, u * 2.2f, u * 2.2f, color);
    canvas.ellipse(cx, cy, u * 2.2f - stroke, u * 2.2f - stroke, fill);
}

void keycap(ModernRenderer& canvas, const RECT& rect, const std::wstring& label, COLORREF fill, COLORREF ink,
            float size, float radius, std::wstring_view family) {
    if (rect.right <= rect.left) return;
    canvas.rounded(rect, radius, fill, fill);
    canvas.text(label, rect, ink, size, FW_NORMAL, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX, family);
}

void soft_shadow(ModernRenderer& canvas, const RECT& rect, float radius, int alpha, float u) {
    if (alpha <= 0) return;
    // Three stacked, spreading layers read as one soft contact shadow.
    const float strength = alpha / 255.0f;
    const struct { float spread, drop, opacity; } layers[] = {{0.0f, 1.0f, 0.55f}, {1.0f, 2.0f, 0.28f}, {2.5f, 3.5f, 0.12f}};
    for (const auto& layer : layers) {
        RECT shadow = rect;
        const int spread = static_cast<int>(std::lround(layer.spread * u));
        InflateRect(&shadow, spread, spread);
        OffsetRect(&shadow, 0, static_cast<int>(std::lround(layer.drop * u)));
        canvas.rounded_fill(shadow, radius + layer.spread * u, RGB(0, 0, 0), strength * layer.opacity);
    }
}

}  // namespace candidate_paint

}  // namespace localpinyin
