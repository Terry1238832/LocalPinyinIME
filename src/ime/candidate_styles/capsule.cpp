// Capsule: one fully rounded bar. The pinyin sits at its left end, a hairline
// divider sets it off, and the candidates follow on the same line; the
// current choice is a solid accent pill that slides along the bar. Nothing
// is stacked, so the window is as short as a candidate window can be.
#include "layout_core.h"
#include "paint_core.h"

#include <algorithm>
#include <cmath>

namespace localpinyin::candidate_styles {

using namespace candidate_layout;

CandidateLayoutResult layout_capsule(const CandidateLayoutInput& input) {
    CandidateLayoutResult result;
    result.style = CandidateStyle::Capsule;
    result.dpi = (std::max)(1, input.dpi);
    const auto frame = Frame::begin(input);
    if (!frame) return result;
    const Frame& f = *frame;
    const size_t count = f.count();

    const int max_width = (std::min)(f.dip(800), f.work_width());
    const int margin = f.dip(kShadowMarginDip);
    const int end_pad = f.fit(14, 48);
    const int gap = f.fit(2, 160);
    const int pill_pad = f.fit(11, 48);
    const int number_gap = f.fit(5, 96);
    const ControlMetrics controls{f.fit(22, 12), f.fit(30, 14), f.fit(2, 160)};
    result.composition_font_px = f.dip(13);
    result.candidate_font_px = f.candidate_font_px();
    result.focus_font_px = result.candidate_font_px;
    result.number_font_px = candidate_number_font_px(result.candidate_font_px, result.dpi);
    result.hint_font_px = f.dip(11);
    const int panel_height = (std::min)(result.candidate_font_px + f.dip(20), f.work_height());
    const int inner_max = (std::max)(1, max_width - end_pad * 2);

    // The pinyin keeps a modest share of the bar; long input is ellipsized.
    const int composition_width = clamp_int(f.composition_width(result.composition_font_px) + f.dip(6),
                                            f.dip(28), (std::max)(f.dip(28), inner_max / 4));
    const int divider_space = f.fit(10, 96) * 2 + 1;
    const int lead = composition_width + divider_space;
    // The candidates stop a little short of the page controls.
    const int controls_gap = f.dip(4);

    std::vector<int> text_widths(count), number_widths(count);
    for (size_t i = 0; i < count; ++i) {
        text_widths[i] = f.candidate_width(i, result.candidate_font_px);
        number_widths[i] = f.digit_width(i, result.number_font_px);
    }
    const auto slot_width = [&](size_t i) { return pill_pad * 2 + number_widths[i] + number_gap + text_widths[i]; };
    const int controls_estimate = controls_width(true, input.settings_button, controls);
    const int row_available = (std::max)(1, inner_max - lead - controls_gap - controls_estimate);
    const auto slice = apply_pages(result, input,
        paginate(count, row_available, gap, [&](size_t i, bool) { return slot_width(i); }), count);

    int used = 0;
    for (size_t i = slice.begin; i < slice.end; ++i) used += (used ? gap : 0) + slot_width(i);
    const int controls_space = controls_width(result.paging_visible, input.settings_button, controls);
    const int candidates_max = (std::max)(1, inner_max - lead - controls_gap - controls_space);
    const int candidates_width = (std::min)(used, candidates_max);
    const int inner_width = clamp_int(lead + candidates_width + controls_gap + controls_space,
                                      (std::min)(f.dip(240), inner_max), inner_max);
    set_panel(result, margin, inner_width + end_pad * 2, panel_height);
    result.panel_padding_px = end_pad;
    result.item_gap_px = gap;
    result.corner_radius = panel_height / 2;
    const int pill_inset = (std::max)(1, f.dip(4));
    result.selected_corner_radius = (std::max)(1, (panel_height - pill_inset * 2) / 2);
    result.chip_corner_radius = result.selected_corner_radius;

    const RECT& panel = result.panel_rect;
    const int left = panel.left + end_pad;
    const int right = panel.right - end_pad;
    const int top = panel.top;
    const int bottom = panel.bottom;
    result.query_rect = RECT{panel.left, top, (std::min)(left + lead, right), bottom};
    result.composition_rect = RECT{left, top, (std::min)(left + composition_width, right), bottom};
    // The divider sits midway between the pinyin and the first candidate.
    result.mark_rect = RECT{left + composition_width + divider_space / 2, top, left + composition_width + divider_space / 2 + 1, bottom};
    const int controls_left = place_controls(result, input, right, top + panel_height / 2, controls);

    const int row_left = (std::min)(left + lead, right);
    const int row_right = (std::max)(row_left, (std::min)(controls_left - controls_gap, right));
    result.candidate_area_rect = RECT{row_left, top, row_right, bottom};
    result.focus_rect = result.candidate_area_rect;

    const int row_space = static_cast<int>(row_right - row_left);
    const bool fits = used <= row_space;
    const size_t shown = slice.end - slice.begin;
    int x = row_left;
    for (size_t i = slice.begin; i < slice.end; ++i) {
        CandidateItemLayout item;
        item.candidate_index = i;
        item.page_number = i - slice.begin + 1;
        item.selected = i == slice.selection;
        item.featured = i == slice.focus;
        item.display_text = input.candidate_texts[i];
        item.number_text_width_px = number_widths[i];
        item.candidate_text_width_px = text_widths[i];
        item.horizontal_padding_px = pill_pad;
        item.number_gap_px = number_gap;
        const int width = fits ? slot_width(i) : squeezed_width(row_space, gap, shown);
        item.bounds = RECT{x, top + pill_inset, x + width, bottom - pill_inset};
        const int number_left = (std::min)(x + pill_pad, x + width);
        item.number_bounds = RECT{number_left, item.bounds.top, (std::min)(number_left + number_widths[i], x + width),
                                  item.bounds.bottom};
        const int text_left = (std::min)(static_cast<int>(item.number_bounds.right) + number_gap, x + width);
        item.text_bounds = RECT{text_left, item.bounds.top, (std::max)(text_left, x + width - pill_pad), item.bounds.bottom};
        finish_item(item);
        result.items.push_back(std::move(item));
        x += width + gap;
    }
    place_near_caret(result, input, result.composition_rect.left);
    return result;
}

RECT marker_capsule(const CandidateLayoutResult& layout, std::optional<size_t>) {
    // The pill marks the current choice; the pointer only tints.
    const CandidateItemLayout* item = candidate_paint::selected_item(layout);
    return item ? item->bounds : RECT{};
}

void paint_capsule(const CandidatePaintContext& context) {
    auto& canvas = context.canvas;
    const auto& layout = context.layout;
    const auto& palette = context.palette;
    const auto& state = context.state;
    const float u = candidate_paint::unit(layout);
    const RECT& panel = layout.panel_rect;

    canvas.rounded(panel, static_cast<float>(layout.corner_radius), palette.background, palette.border);
    candidate_paint::query_text(context, layout.composition_rect, static_cast<float>(layout.composition_font_px),
                                FW_NORMAL, palette.composition_text, palette.separator_text, palette.caret);
    const float divider_x = static_cast<float>(layout.mark_rect.left) + 0.5f;
    const float height = static_cast<float>(panel.bottom - panel.top);
    canvas.line(divider_x, panel.top + height * 0.28f, divider_x, panel.bottom - height * 0.28f, palette.divider, 1.0f);
    candidate_paint::page_controls(context, 6.0f * u, palette.focus_bar);
    candidate_paint::settings_gear(context, 6.0f * u);

    const float pill_radius = static_cast<float>(layout.selected_corner_radius);
    for (const auto& item : layout.items) {
        const bool hovered = state.hovered == item.candidate_index;
        const bool pressed = state.pressed == item.candidate_index;
        if (!item.featured && (hovered || pressed))
            canvas.rounded(item.bounds, pill_radius, pressed ? palette.pressed_background : palette.hover_background,
                           pressed ? palette.pressed_background : palette.hover_background);
    }
    const CandidateItemLayout* chosen = candidate_paint::selected_item(layout);
    const RECT pill = !candidate_paint::is_empty(state.marker) ? state.marker : chosen ? chosen->bounds : RECT{};
    if (!candidate_paint::is_empty(pill)) {
        if (!palette.high_contrast)
            candidate_paint::soft_shadow(canvas, pill, pill_radius, palette.focus_shadow_alpha, u);
        canvas.rounded(pill, pill_radius, palette.focus_background, palette.focus_border);
    }

    const float number_px = static_cast<float>(layout.number_font_px);
    const float size = static_cast<float>(layout.candidate_font_px);
    for (const auto& item : layout.items) {
        const bool on_pill = item.featured;
        const bool hovered = state.hovered == item.candidate_index;
        canvas.text(std::to_wstring(item.page_number), item.number_bounds,
                    on_pill ? palette.focus_number_text : palette.number_text, number_px, FW_NORMAL,
                    DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
        canvas.text(item.display_text, item.text_bounds,
                    on_pill ? palette.focus_text : hovered ? palette.text : palette.text_muted, size,
                    on_pill ? FW_SEMIBOLD : FW_NORMAL, candidate_text_draw_flags(item.truncated));
    }
}

}  // namespace localpinyin::candidate_styles
