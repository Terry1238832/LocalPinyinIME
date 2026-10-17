// Cursor Rail: a compact instrument. Candidates run along one horizontal
// rail; the current choice is set larger and bolder on a soft focus field,
// and a 2 px focus line on the rail glides to whichever candidate the pointer
// is over and back to the current choice. Below, a page track shows which
// page of the candidates is on screen.
#include "layout_core.h"
#include "paint_core.h"

#include <algorithm>
#include <cmath>

namespace localpinyin::candidate_styles {

using namespace candidate_layout;

namespace {

// YaHei UI's baseline sits about 1.06 em below the top of its line box.
constexpr float kBaselineEm = 1.06f;
constexpr int kFocusWeight = 700;
constexpr int kItemWeight = 400;

}  // namespace

CandidateLayoutResult layout_cursor_rail(const CandidateLayoutInput& input) {
    CandidateLayoutResult result;
    result.style = CandidateStyle::CursorRail;
    result.dpi = (std::max)(1, input.dpi);
    const auto frame = Frame::begin(input);
    if (!frame) return result;
    const Frame& f = *frame;
    const size_t count = f.count();

    const int max_width = (std::min)(f.dip(760), f.work_width());
    const int margin = f.dip(kShadowMarginDip);
    // Items' hit areas reach `side` from the panel edge; their text starts
    // `item_pad` further in, so the rail and the pinyin share one left edge.
    const int side = f.fit(7, 48);
    const int item_pad = f.fit(7, 48);
    const int gap = f.fit(4, 96);
    const ControlMetrics controls{f.fit(20, 12), f.fit(28, 14), f.fit(2, 160)};
    result.composition_font_px = f.dip(13);
    result.candidate_font_px = f.candidate_font_px();
    result.focus_font_px = static_cast<int>(std::lround(result.candidate_font_px * 1.12f));
    result.number_font_px = (std::max)(f.dip(9), static_cast<int>(std::lround(result.candidate_font_px * 0.55f)));
    result.hint_font_px = f.dip(11);
    result.focus_weight = kFocusWeight;
    result.item_weight = kItemWeight;
    const int number_gap = f.fit(4, 120);
    const int query_height = f.dip(30);
    const int row_height = result.focus_font_px + f.dip(18);
    const int track_height = f.dip(12);
    const int inner_max = (std::max)(1, max_width - side * 2);

    std::vector<int> text_widths(count), focus_widths(count), number_widths(count);
    for (size_t i = 0; i < count; ++i) {
        text_widths[i] = f.candidate_width(i, result.candidate_font_px, kItemWeight);
        focus_widths[i] = f.candidate_width(i, result.focus_font_px, kFocusWeight);
        number_widths[i] = f.digit_width(i, result.number_font_px);
    }
    const auto slot_width = [&](size_t i, bool focus) {
        return item_pad * 2 + number_widths[i] + number_gap + (focus ? focus_widths[i] : text_widths[i]);
    };
    // Every page starts with the current choice, which is set larger.
    const auto slice = apply_pages(result, input,
        paginate(count, inner_max, gap, [&](size_t i, bool first) { return slot_width(i, first); }), count);

    int used = 0;
    for (size_t i = slice.begin; i < slice.end; ++i) used += (used ? gap : 0) + slot_width(i, i == slice.focus);
    const int controls_space = controls_width(result.paging_visible, input.settings_button, controls);
    const int query_needed = item_pad + f.composition_width(result.composition_font_px) + f.dip(18) + controls_space;
    const int inner_min = (std::min)(f.dip(200), inner_max);
    const int inner_width = clamp_int((std::max)(used, query_needed), inner_min, inner_max);
    const int panel_width = inner_width + side * 2;
    const int panel_height = (std::min)(query_height + row_height + (result.paging_visible ? track_height : f.dip(6)),
                                        f.work_height());
    set_panel(result, margin, panel_width, panel_height);
    result.panel_padding_px = side;
    result.item_gap_px = gap;
    result.corner_radius = f.dip(10);
    result.selected_corner_radius = f.dip(6);
    result.chip_corner_radius = f.dip(6);

    const RECT& panel = result.panel_rect;
    const int left = panel.left + side;
    const int right = panel.right - side;
    const int content_left = left + item_pad;
    const int content_right = right - item_pad;
    const int query_bottom = (std::min)(static_cast<int>(panel.top) + query_height, static_cast<int>(panel.bottom));
    result.query_rect = RECT{panel.left, panel.top, panel.right, query_bottom};
    const int controls_left = place_controls(result, input, right - f.dip(2), panel.top + query_height / 2 + f.dip(1), controls);
    result.composition_rect = RECT{content_left, panel.top + f.dip(4),
                                   (std::max)(content_left, controls_left - f.dip(10)), query_bottom};

    const int row_top = query_bottom;
    const int row_bottom = (std::min)(row_top + row_height, static_cast<int>(panel.bottom));
    const int rail_y = row_bottom - f.dip(5);
    result.rail_rect = RECT{content_left, rail_y, content_right, rail_y + 1};
    result.candidate_area_rect = RECT{left, row_top, right, row_bottom};
    result.focus_rect = result.candidate_area_rect;
    if (result.paging_visible) {
        const int track_y = row_bottom + track_height / 2;
        result.track_rect = RECT{content_left, track_y - 1, content_right, track_y + 1};
    }

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
        item.candidate_text_width_px = item.featured ? focus_widths[i] : text_widths[i];
        item.horizontal_padding_px = item_pad;
        item.number_gap_px = number_gap;
        const int width = fits ? slot_width(i, item.featured) : squeezed_width(inner_width, gap, shown);
        item.bounds = RECT{x, row_top, x + width, rail_y + f.dip(3)};
        const int number_left = (std::min)(x + item_pad, x + width);
        item.number_bounds = RECT{number_left, row_top, (std::min)(number_left + number_widths[i], x + width), rail_y};
        const int text_left = (std::min)(static_cast<int>(item.number_bounds.right) + number_gap, x + width);
        item.text_bounds = RECT{text_left, row_top, (std::max)(text_left, x + width - item_pad), rail_y};
        finish_item(item);
        result.items.push_back(std::move(item));
        x += width + gap;
    }
    result.focus_rect.bottom = rail_y;
    // One baseline for the number and both text sizes: the larger text's line
    // centred in the space above the rail.
    result.baseline_y = row_top + (rail_y - row_top - static_cast<int>(result.focus_font_px * 1.32f)) / 2 +
                        static_cast<int>(result.focus_font_px * kBaselineEm);
    place_near_caret(result, input, content_left);
    return result;
}

RECT marker_cursor_rail(const CandidateLayoutResult& layout, std::optional<size_t> hovered) {
    const CandidateItemLayout* item = hovered ? candidate_paint::item_for(layout, *hovered) : nullptr;
    if (!item) item = candidate_paint::selected_item(layout);
    if (!item) return RECT{};
    const int text_right = item->text_bounds.left + (std::min)(item->candidate_text_width_px, item->text_available_width_px);
    const int y = layout.rail_rect.top;
    const int thickness = (std::max)(2, scale_dip(2, layout.dpi));
    return RECT{item->number_bounds.left, y - thickness / 2, text_right, y - thickness / 2 + thickness};
}

RECT track_cursor_rail(const CandidateLayoutResult& layout) {
    if (!layout.paging_visible || candidate_paint::is_empty(layout.track_rect)) return RECT{};
    const int pages = static_cast<int>(layout.page_indices.size());
    const int gap = (std::max)(2, scale_dip(3, layout.dpi));
    const int width = rect_width(layout.track_rect);
    const int segment = (std::max)(gap, (width - gap * (pages - 1)) / pages);
    const int x = layout.track_rect.left + static_cast<int>(layout.current_page) * (segment + gap);
    return RECT{x, layout.track_rect.top, x + segment, layout.track_rect.bottom};
}

void paint_cursor_rail(const CandidatePaintContext& context) {
    auto& canvas = context.canvas;
    const auto& layout = context.layout;
    const auto& palette = context.palette;
    const auto& state = context.state;
    const float u = candidate_paint::unit(layout);
    const RECT& panel = layout.panel_rect;
    const float radius = static_cast<float>(layout.corner_radius);

    // Graphite (dark) or soft paper-white (light) with a crisp edge.
    if (palette.surface_top != palette.background && !palette.high_contrast)
        canvas.rounded_gradient(panel, radius, palette.surface_top, palette.background);
    else
        canvas.rounded(panel, radius, palette.background, palette.background);
    canvas.outline(panel, radius, palette.border, 1.0f);
    if (palette.highlight != palette.background && !palette.high_contrast) {
        const float y = panel.top + 1.5f;
        canvas.line(panel.left + radius, y, panel.right - radius, y, palette.highlight, 1.0f);
    }

    candidate_paint::query_text(context, layout.composition_rect, static_cast<float>(layout.composition_font_px),
                                FW_NORMAL, palette.composition_text, palette.separator_text, palette.caret);
    candidate_paint::page_controls(context, 5.0f * u, palette.focus_bar);
    candidate_paint::settings_gear(context, 5.0f * u);

    // The rail, with a fine tick between neighboring candidates, like the
    // graduations of an instrument scale.
    const float rail_y = layout.rail_rect.top + 0.5f;
    canvas.line(static_cast<float>(layout.rail_rect.left), rail_y, static_cast<float>(layout.rail_rect.right), rail_y,
                palette.divider, 1.0f);
    const float tick = (std::max)(2.0f, std::round(3.0f * u));
    for (size_t index = 1; index < layout.items.size(); ++index) {
        const float x = std::round((layout.items[index - 1].bounds.right + layout.items[index].bounds.left) / 2.0f) + 0.5f;
        canvas.line(x, rail_y - tick, x, rail_y + tick, palette.separator_text, 1.0f);
    }

    const int baseline = layout.baseline_y;
    const float number_px = static_cast<float>(layout.number_font_px);
    for (const auto& item : layout.items) {
        const bool hovered = state.hovered == item.candidate_index;
        const bool pressed = state.pressed == item.candidate_index;
        const bool focus = item.featured;
        if (focus || pressed) {
            RECT field = item.bounds;
            field.top += static_cast<LONG>(std::lround(4 * u));
            field.bottom = layout.rail_rect.top - static_cast<LONG>(std::lround(1 * u));
            canvas.rounded(field, static_cast<float>(layout.selected_corner_radius),
                           pressed ? palette.pressed_background : palette.focus_background,
                           pressed ? palette.pressed_background : palette.focus_background);
        }
        const float size = static_cast<float>(focus ? layout.focus_font_px : layout.candidate_font_px);
        const int weight = focus ? layout.focus_weight : layout.item_weight;
        const COLORREF ink = focus ? palette.focus_text : hovered ? palette.text : palette.text_muted;
        RECT number = item.number_bounds;
        number.top = baseline - static_cast<LONG>(std::lround(canvas.baseline(number_px)));
        number.bottom = number.top + static_cast<LONG>(std::lround(number_px * 1.4f));
        canvas.text(std::to_wstring(item.page_number), number, focus ? palette.focus_number_text : palette.number_text,
                    number_px, FW_NORMAL, DT_SINGLELINE | DT_TOP | DT_NOPREFIX);
        RECT text = item.text_bounds;
        text.top = baseline - static_cast<LONG>(std::lround(canvas.baseline(size, weight)));
        text.bottom = text.top + static_cast<LONG>(std::lround(size * 1.4f));
        canvas.text(item.display_text, text, ink, size, weight,
                    (candidate_text_draw_flags(item.truncated) & ~DT_VCENTER) | DT_TOP);
    }

    // The focus line, where the window's animation has it this frame.
    if (!candidate_paint::is_empty(state.marker)) {
        canvas.rounded(state.marker, (state.marker.bottom - state.marker.top) / 2.0f, palette.focus_bar, palette.focus_bar);
    }
    // The page track and the current page's segment.
    if (layout.paging_visible && !candidate_paint::is_empty(layout.track_rect)) {
        canvas.rounded(layout.track_rect, 1.0f, palette.divider, palette.divider);
        if (!candidate_paint::is_empty(state.track_marker))
            canvas.rounded(state.track_marker, 1.0f, palette.focus_bar, palette.focus_bar);
    }
}

}  // namespace localpinyin::candidate_styles
