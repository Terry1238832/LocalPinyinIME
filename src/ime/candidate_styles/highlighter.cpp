// Highlighter: a slip of notebook paper. The pinyin is a quiet line on top,
// the candidates plain ink on one ruled line below. The current choice is
// swiped with a highlighter: a translucent chisel stroke with slanted, slightly
// ragged ends behind the word. The stroke glides to the word under the pointer
// and draws itself on, left to right, when the current word changes.
#include "layout_core.h"
#include "paint_core.h"

#include <algorithm>
#include <cmath>

namespace localpinyin::candidate_styles {

using namespace candidate_layout;

namespace {

constexpr int kFocusWeight = 500;
// The stroke draws itself on in this long.
constexpr float kDrawMs = 190.0f;

// The stroke's own small irregularities, the same for a word every frame.
float jitter(const std::wstring& text, unsigned salt) noexcept {
    unsigned hash = 2166136261u ^ salt;
    for (wchar_t ch : text) hash = (hash ^ static_cast<unsigned>(ch)) * 16777619u;
    hash ^= hash >> 13;
    hash *= 0x5bd1e995u;
    hash ^= hash >> 15;
    return static_cast<float>(hash % 1000u) / 1000.0f - 0.5f;  // -0.5 .. 0.5
}

// A chisel-tip stroke filling `band`, drawn to `reach` of its length.
// Returns its outline (empty when nothing was drawn).
std::vector<D2D1_POINT_2F> stroke(ModernRenderer& canvas, const RECT& band, float reach, COLORREF color, float opacity,
                                  const std::wstring& seed) {
    const float left = static_cast<float>(band.left);
    const float top = static_cast<float>(band.top);
    const float bottom = static_cast<float>(band.bottom);
    const float height = bottom - top;
    const float right = left + (static_cast<float>(band.right) - left) * std::clamp(reach, 0.0f, 1.0f);
    if (right - left < 2.0f || height < 2.0f) return {};
    const float slant = height * 0.22f;
    const float wobble = height * 0.05f;
    const float middle = (left + right) / 2.0f;
    const std::vector<D2D1_POINT_2F> outline{
        {left + slant, top + wobble * jitter(seed, 1)},
        {middle, top - wobble * 0.6f + wobble * jitter(seed, 2)},
        {right, top + wobble * jitter(seed, 3)},
        {right + slant * 0.12f, top + height * (0.4f + 0.08f * jitter(seed, 4))},
        {right - slant, bottom + wobble * jitter(seed, 5)},
        {middle, bottom + wobble * 0.5f + wobble * jitter(seed, 6)},
        {left, bottom + wobble * jitter(seed, 7)},
        {left - slant * 0.12f, top + height * (0.6f + 0.08f * jitter(seed, 8))},
    };
    canvas.polygon(outline, color, opacity);
    // A second, lighter pass a little lower: the overlap of a real swipe.
    const float lift = height * 0.16f;
    const std::vector<D2D1_POINT_2F> overlap{
        {left + slant * 0.6f, top + lift},
        {right - slant * 0.1f, top + lift + wobble * jitter(seed, 9)},
        {right - slant * 0.9f, bottom - lift * 0.4f},
        {left - slant * 0.1f, bottom - lift * 0.4f + wobble * jitter(seed, 10)},
    };
    canvas.polygon(overlap, color, opacity * 0.35f);
    return outline;
}

}  // namespace

CandidateLayoutResult layout_highlighter(const CandidateLayoutInput& input) {
    CandidateLayoutResult result;
    result.style = CandidateStyle::Highlighter;
    result.dpi = (std::max)(1, input.dpi);
    const auto frame = Frame::begin(input);
    if (!frame) return result;
    const Frame& f = *frame;
    const size_t count = f.count();

    const int max_width = (std::min)(f.dip(720), f.work_width());
    const int margin = f.dip(kShadowMarginDip);
    const int side = f.fit(12, 48);
    const int gap = f.fit(6, 120);
    const int word_pad = f.fit(8, 64);
    const int number_gap = f.fit(3, 120);
    const ControlMetrics controls{f.fit(22, 12), f.fit(30, 14), f.fit(2, 160)};
    result.composition_font_px = f.dip(13);
    result.candidate_font_px = f.candidate_font_px();
    result.focus_font_px = result.candidate_font_px;
    result.number_font_px = candidate_number_font_px(result.candidate_font_px, result.dpi);
    result.hint_font_px = f.dip(11);
    const int query_height = f.dip(28);
    const int row_height = result.candidate_font_px + f.dip(18);
    const int bottom_space = f.dip(6);
    const int inner_max = (std::max)(1, max_width - side * 2);

    std::vector<int> text_widths(count), number_widths(count);
    for (size_t i = 0; i < count; ++i) {
        // Measured at the current word's weight, so moving the stroke never
        // pushes the words beside it.
        text_widths[i] = f.candidate_width(i, result.candidate_font_px, kFocusWeight);
        number_widths[i] = f.digit_width(i, result.number_font_px);
    }
    const auto slot_width = [&](size_t i) { return word_pad * 2 + number_widths[i] + number_gap + text_widths[i]; };
    const auto slice = apply_pages(result, input,
        paginate(count, inner_max, gap, [&](size_t i, bool) { return slot_width(i); }), count);

    int used = 0;
    for (size_t i = slice.begin; i < slice.end; ++i) used += (used ? gap : 0) + slot_width(i);
    const int controls_space = controls_width(result.paging_visible, input.settings_button, controls);
    const int query_needed = f.dip(8) + f.composition_width(result.composition_font_px) + f.dip(18) + controls_space;
    const int inner_width = clamp_int((std::max)(used, query_needed), (std::min)(f.dip(220), inner_max), inner_max);
    const int panel_height = (std::min)(query_height + row_height + bottom_space, f.work_height());
    set_panel(result, margin, inner_width + side * 2, panel_height);
    result.panel_padding_px = side;
    result.item_gap_px = gap;
    result.corner_radius = f.dip(10);
    result.selected_corner_radius = f.dip(4);
    result.chip_corner_radius = f.dip(4);

    const RECT& panel = result.panel_rect;
    const int left = panel.left + side;
    const int right = panel.right - side;
    const int query_bottom = (std::min)(static_cast<int>(panel.top) + query_height, static_cast<int>(panel.bottom));
    result.query_rect = RECT{panel.left, panel.top, panel.right, query_bottom};
    const int controls_left = place_controls(result, input, right - f.dip(2), panel.top + query_height / 2 + f.dip(2), controls);
    const int label_left = left + word_pad;
    result.composition_rect = RECT{label_left, panel.top + f.dip(3), (std::max)(label_left, controls_left - f.dip(10)),
                                   query_bottom};
    const int row_top = query_bottom;
    const int row_bottom = (std::min)(row_top + row_height, static_cast<int>(panel.bottom));
    result.candidate_area_rect = RECT{left, row_top, right, row_bottom};
    result.focus_rect = result.candidate_area_rect;
    // The ruled line the words sit on.
    result.rail_rect = RECT{left, row_bottom - f.dip(5), right, row_bottom - f.dip(5) + 1};

    const bool fits = used <= inner_width;
    const size_t shown = slice.end - slice.begin;
    int x = left;
    for (size_t i = slice.begin; i < slice.end; ++i) {
        CandidateItemLayout item;
        item.candidate_index = i;
        item.page_number = i - slice.begin + 1;
        item.selected = i == slice.selection;
        item.featured = i == slice.focus;
        item.display_text = input.candidate_texts[i];
        item.number_text_width_px = number_widths[i];
        item.candidate_text_width_px = text_widths[i];
        item.horizontal_padding_px = word_pad;
        item.number_gap_px = number_gap;
        const int width = fits ? slot_width(i) : squeezed_width(inner_width, gap, shown);
        item.bounds = RECT{x, row_top, x + width, (std::max)(row_top + 1, row_bottom)};
        const int number_left = (std::min)(x + word_pad, x + width);
        item.number_bounds = RECT{number_left, row_top, (std::min)(number_left + number_widths[i], x + width), row_bottom};
        const int text_left = (std::min)(static_cast<int>(item.number_bounds.right) + number_gap, x + width);
        item.text_bounds = RECT{text_left, row_top, (std::max)(text_left, x + width - word_pad), row_bottom};
        finish_item(item);
        result.items.push_back(std::move(item));
        x += width + gap;
    }
    place_near_caret(result, input, result.composition_rect.left);
    return result;
}

RECT marker_highlighter(const CandidateLayoutResult& layout, std::optional<size_t> hovered) {
    // The stroke covers most of the word, sitting slightly low as a quick
    // swipe does.
    const CandidateItemLayout* item = hovered ? candidate_paint::item_for(layout, *hovered) : nullptr;
    if (!item) item = candidate_paint::selected_item(layout);
    if (!item) return RECT{};
    const int font = layout.candidate_font_px;
    const int center = (item->text_bounds.top + item->text_bounds.bottom) / 2;
    const int words_right = (std::min)(static_cast<int>(item->text_bounds.left) + item->candidate_text_width_px,
                                       static_cast<int>(item->text_bounds.right));
    // A swipe starts just before the word and trails off past its end.
    const int lead = (std::max)(1, font / 12);
    const int reach = (std::max)(2, font / 4);
    return RECT{(std::max)(static_cast<int>(item->bounds.left), static_cast<int>(item->text_bounds.left) - lead),
                center - font * 3 / 10,
                (std::min)(static_cast<int>(item->bounds.right), words_right + reach),
                (std::min)(static_cast<int>(item->bounds.bottom), center + font * 9 / 20)};
}

void paint_highlighter(const CandidatePaintContext& context) {
    auto& canvas = context.canvas;
    const auto& layout = context.layout;
    const auto& palette = context.palette;
    const auto& state = context.state;
    const float u = candidate_paint::unit(layout);

    canvas.rounded(layout.panel_rect, static_cast<float>(layout.corner_radius), palette.background, palette.border);
    candidate_paint::query_text(context, layout.composition_rect, static_cast<float>(layout.composition_font_px),
                                FW_NORMAL, palette.composition_text, palette.separator_text, palette.caret);
    candidate_paint::page_controls(context, 4.0f * u, palette.focus_bar);
    candidate_paint::settings_gear(context, 6.0f * u);
    if (!candidate_paint::is_empty(layout.rail_rect)) {
        const float y = static_cast<float>(layout.rail_rect.top) + 0.5f;
        canvas.line(static_cast<float>(layout.rail_rect.left), y, static_cast<float>(layout.rail_rect.right), y,
                    palette.divider, 1.0f);
    }

    // A near-opaque swipe in both themes; the part of the word under it turns
    // to dark ink, the rest (above the swipe) stays in the page's ink.
    constexpr float opacity = 0.86f;
    std::vector<D2D1_POINT_2F> swipe;
    for (const auto& item : layout.items) {
        if (state.pressed == item.candidate_index && !item.featured)
            stroke(canvas, marker_highlighter(layout, item.candidate_index), 1.0f, palette.focus_background,
                   opacity * 0.45f, item.display_text);
    }
    const CandidateItemLayout* chosen = candidate_paint::selected_item(layout);
    const RECT band = !candidate_paint::is_empty(state.marker) ? state.marker : marker_highlighter(layout, std::nullopt);
    if (!candidate_paint::is_empty(band)) {
        // A new current word gets a fresh swipe, drawn on from the left.
        float reach = 1.0f;
        if (state.reveal_ms < kRevealAtRestMs && chosen &&
            (state.previous_texts.empty() || state.previous_texts.front() != chosen->display_text)) {
            const float t = std::clamp(static_cast<float>(state.reveal_ms) / kDrawMs, 0.0f, 1.0f);
            reach = 1.0f - (1.0f - t) * (1.0f - t);
        }
        const CandidateItemLayout* under = state.hovered ? candidate_paint::item_for(layout, *state.hovered) : chosen;
        swipe = stroke(canvas, band, reach, palette.focus_background, opacity, under ? under->display_text : std::wstring());
    }

    const float number_px = static_cast<float>(layout.number_font_px);
    const float size = static_cast<float>(layout.candidate_font_px);
    // The word the stroke rests on: the hovered one, else the current choice.
    const auto marked = state.hovered && candidate_paint::item_for(layout, *state.hovered)
                            ? *state.hovered : chosen ? chosen->candidate_index : layout.items.size() + 1;
    for (const auto& item : layout.items) {
        const bool focused = item.featured;
        const bool under = item.candidate_index == marked;
        canvas.text(std::to_wstring(item.page_number), item.number_bounds,
                    focused ? palette.focus_number_text : palette.number_text, number_px, FW_NORMAL,
                    DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
        const COLORREF ink = focused || under ? palette.text : palette.text_muted;
        const int weight = focused || under ? kFocusWeight : FW_NORMAL;
        if (!under || swipe.empty()) {
            canvas.text(item.display_text, item.text_bounds, ink, size, weight, candidate_text_draw_flags(item.truncated));
            continue;
        }
        // The page's ink outside the stroke, dark ink inside it, so neither
        // shows through the other's edges.
        RECT area = item.text_bounds;
        InflateRect(&area, static_cast<int>(size), static_cast<int>(size));
        canvas.push_layer_outside(swipe, area);
        canvas.text(item.display_text, item.text_bounds, ink, size, weight, candidate_text_draw_flags(item.truncated));
        canvas.pop_layer();
        canvas.push_layer(swipe);
        canvas.text(item.display_text, item.text_bounds, palette.focus_text, size, weight, candidate_text_draw_flags(item.truncated));
        canvas.pop_layer();
    }
}

}  // namespace localpinyin::candidate_styles
