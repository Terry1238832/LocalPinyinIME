// Outline: a flat sheet with a hairline frame and almost no shadow. The
// pinyin runs along the top; the candidates sit on one line below. Nothing is
// filled: the current choice is ringed by a thin accent outline, and its
// number and text take the accent color. The ring slides with the choice.
#include "layout_core.h"
#include "paint_core.h"

#include <algorithm>
#include <cmath>

namespace localpinyin::candidate_styles {

using namespace candidate_layout;

CandidateLayoutResult layout_outline(const CandidateLayoutInput& input) {
    CandidateLayoutResult result;
    result.style = CandidateStyle::Outline;
    result.dpi = (std::max)(1, input.dpi);
    const auto frame = Frame::begin(input);
    if (!frame) return result;
    const Frame& f = *frame;
    const size_t count = f.count();

    const int max_width = (std::min)(f.dip(720), f.work_width());
    const int margin = f.dip(kShadowMarginDip);
    const int side = f.fit(8, 48);
    const int gap = f.fit(4, 120);
    const int key_pad = f.fit(10, 48);
    const int number_gap = f.fit(6, 80);
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
        text_widths[i] = f.candidate_width(i, result.candidate_font_px);
        number_widths[i] = f.digit_width(i, result.number_font_px);
    }
    const auto slot_width = [&](size_t i) { return key_pad * 2 + number_widths[i] + number_gap + text_widths[i]; };
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
    result.selected_corner_radius = f.dip(7);
    result.chip_corner_radius = f.dip(7);

    const RECT& panel = result.panel_rect;
    const int left = panel.left + side;
    const int right = panel.right - side;
    const int query_bottom = (std::min)(static_cast<int>(panel.top) + query_height, static_cast<int>(panel.bottom));
    result.query_rect = RECT{panel.left, panel.top, panel.right, query_bottom};
    const int controls_left = place_controls(result, input, right - f.dip(2), panel.top + query_height / 2 + f.dip(2), controls);
    const int label_left = left + key_pad;
    result.composition_rect = RECT{label_left, panel.top + f.dip(3), (std::max)(label_left, controls_left - f.dip(10)),
                                   query_bottom};
    const int row_top = query_bottom;
    const int row_bottom = (std::min)(row_top + row_height, static_cast<int>(panel.bottom));
    result.candidate_area_rect = RECT{left, row_top, right, row_bottom};
    result.focus_rect = result.candidate_area_rect;

    const bool fits = used <= inner_width;
    const size_t shown = slice.end - slice.begin;
    const int inset = f.dip(3);
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
        item.horizontal_padding_px = key_pad;
        item.number_gap_px = number_gap;
        const int width = fits ? slot_width(i) : squeezed_width(inner_width, gap, shown);
        item.bounds = RECT{x, row_top + inset, x + width, (std::max)(row_top + inset + 1, row_bottom - inset)};
        const int number_left = (std::min)(x + key_pad, x + width);
        item.number_bounds = RECT{number_left, item.bounds.top, (std::min)(number_left + number_widths[i], x + width),
                                  item.bounds.bottom};
        const int text_left = (std::min)(static_cast<int>(item.number_bounds.right) + number_gap, x + width);
        item.text_bounds = RECT{text_left, item.bounds.top, (std::max)(text_left, x + width - key_pad), item.bounds.bottom};
        finish_item(item);
        result.items.push_back(std::move(item));
        x += width + gap;
    }
    place_near_caret(result, input, result.composition_rect.left);
    return result;
}

RECT marker_outline(const CandidateLayoutResult& layout, std::optional<size_t>) {
    const CandidateItemLayout* item = candidate_paint::selected_item(layout);
    return item ? item->bounds : RECT{};
}

void paint_outline(const CandidatePaintContext& context) {
    auto& canvas = context.canvas;
    const auto& layout = context.layout;
    const auto& palette = context.palette;
    const auto& state = context.state;
    const float u = candidate_paint::unit(layout);

    canvas.rounded(layout.panel_rect, static_cast<float>(layout.corner_radius), palette.background, palette.border);
    candidate_paint::query_text(context, layout.composition_rect, static_cast<float>(layout.composition_font_px),
                                FW_NORMAL, palette.composition_text, palette.separator_text, palette.caret);
    candidate_paint::page_controls(context, 6.0f * u, palette.focus_bar);
    candidate_paint::settings_gear(context, 6.0f * u);

    const float radius = static_cast<float>(layout.selected_corner_radius);
    // The pointer draws a faint ring of its own.
    for (const auto& item : layout.items) {
        const bool hovered = state.hovered == item.candidate_index;
        const bool pressed = state.pressed == item.candidate_index;
        if (item.featured || !(hovered || pressed)) continue;
        if (pressed) canvas.rounded(item.bounds, radius, palette.pressed_background, palette.chip_border);
        else canvas.outline(item.bounds, radius, palette.chip_border, 1.0f);
    }
    const CandidateItemLayout* chosen = candidate_paint::selected_item(layout);
    const RECT ring = !candidate_paint::is_empty(state.marker) ? state.marker : chosen ? chosen->bounds : RECT{};
    if (!candidate_paint::is_empty(ring)) {
        canvas.rounded(ring, radius, palette.focus_background, palette.focus_background);
        canvas.outline(ring, radius, palette.focus_border, 1.5f * u);
    }

    const float number_px = static_cast<float>(layout.number_font_px);
    const float size = static_cast<float>(layout.candidate_font_px);
    for (const auto& item : layout.items) {
        const bool focused = item.featured;
        const bool hovered = state.hovered == item.candidate_index;
        canvas.text(std::to_wstring(item.page_number), item.number_bounds,
                    focused ? palette.focus_number_text : palette.number_text, number_px, FW_NORMAL,
                    DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
        canvas.text(item.display_text, item.text_bounds,
                    focused ? palette.focus_text : hovered ? palette.text : palette.text_muted, size, FW_NORMAL,
                    candidate_text_draw_flags(item.truncated));
    }
}

}  // namespace localpinyin::candidate_styles
