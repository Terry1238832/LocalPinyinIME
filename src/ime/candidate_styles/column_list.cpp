// Column List: the candidates stacked one per row under the pinyin, like a
// menu. Long words and whole sentences read in full and the eye travels
// straight down. The current choice is a tinted row with its number in the
// accent color; the tint slides between rows. Every page keeps the height of
// the fullest page, so the window does not jump while paging.
#include "layout_core.h"
#include "paint_core.h"

#include <algorithm>
#include <cmath>

namespace localpinyin::candidate_styles {

using namespace candidate_layout;

namespace {

constexpr size_t kRowsPerPage = 6;

}  // namespace

CandidateLayoutResult layout_column_list(const CandidateLayoutInput& input) {
    CandidateLayoutResult result;
    result.style = CandidateStyle::ColumnList;
    result.dpi = (std::max)(1, input.dpi);
    const auto frame = Frame::begin(input);
    if (!frame) return result;
    const Frame& f = *frame;
    const size_t count = f.count();

    const int max_width = (std::min)(f.dip(440), f.work_width());
    const int margin = f.dip(kShadowMarginDip);
    const int padding = f.fit(6, 48);
    const int row_pad = f.fit(10, 48);
    const int number_gap = f.fit(10, 96);
    const int row_gap = f.dip(2);
    const ControlMetrics controls{f.fit(22, 12), f.fit(30, 14), f.fit(2, 160)};
    result.composition_font_px = f.dip(13);
    result.candidate_font_px = f.candidate_font_px();
    result.focus_font_px = result.candidate_font_px;
    result.number_font_px = candidate_number_font_px(result.candidate_font_px, result.dpi);
    result.hint_font_px = f.dip(11);
    const int query_height = f.dip(30);
    // A little air between the hairline under the pinyin and the first row.
    const int body_gap = f.dip(4);
    const int nominal_row = result.candidate_font_px + f.dip(14);

    // Rows per page: at most six, and only as many as the monitor height holds.
    const int rows_space = (std::max)(1, f.work_height() - query_height - body_gap - padding * 2);
    const size_t fitting = static_cast<size_t>((std::max)(1, (rows_space + row_gap) / (nominal_row + row_gap)));
    const size_t per_page = (std::min)(kRowsPerPage, fitting);
    std::vector<UINT> pages;
    for (size_t start = 0; start < count; start += per_page) pages.push_back(static_cast<UINT>(start));
    const auto slice = apply_pages(result, input, std::move(pages), count);

    // The fullest page sets the height; a host's own pages may hold more rows.
    size_t rows = 1;
    for (size_t page = 0; page < result.page_indices.size(); ++page) {
        const size_t begin = result.page_indices[page];
        const size_t end = page + 1 < result.page_indices.size() ? result.page_indices[page + 1] : count;
        rows = (std::max)(rows, end - begin);
    }
    const int rows_room = (std::max)(1, f.work_height() - query_height - body_gap - padding * 2 -
                                           row_gap * static_cast<int>(rows - 1));
    const int row_height = (std::max)(1, (std::min)(nominal_row, rows_room / static_cast<int>(rows)));

    std::vector<int> text_widths(count), number_widths(count);
    for (size_t i = 0; i < count; ++i) {
        text_widths[i] = f.candidate_width(i, result.candidate_font_px);
        number_widths[i] = f.digit_width(i, result.number_font_px);
    }
    const int controls_space = controls_width(result.paging_visible, input.settings_button, controls);
    int content_needed = f.dip(8) + f.composition_width(result.composition_font_px) + f.dip(14) + controls_space;
    for (size_t i = slice.begin; i < slice.end; ++i)
        content_needed = (std::max)(content_needed, row_pad * 2 + number_widths[i] + number_gap + text_widths[i]);
    const int inner_max = (std::max)(1, max_width - padding * 2);
    const int inner_width = clamp_int(content_needed, (std::min)(f.dip(200), inner_max), inner_max);
    const int body_height = static_cast<int>(rows) * row_height + static_cast<int>(rows - 1) * row_gap;
    const int panel_height = (std::min)(query_height + body_gap + body_height + padding, f.work_height());
    set_panel(result, margin, inner_width + padding * 2, panel_height);
    result.panel_padding_px = padding;
    result.item_gap_px = 0;
    result.row_gap_px = row_gap;
    result.corner_radius = f.dip(12);
    result.selected_corner_radius = f.dip(8);
    result.chip_corner_radius = f.dip(8);

    const RECT& panel = result.panel_rect;
    const int left = panel.left + padding;
    const int right = panel.right - padding;
    const int top = panel.top;
    const int query_bottom = (std::min)(top + query_height, static_cast<int>(panel.bottom));
    result.query_rect = RECT{panel.left, top, panel.right, query_bottom};
    const int controls_left = place_controls(result, input, right - f.dip(2), top + query_height / 2, controls);
    const int label_left = left + row_pad;
    result.composition_rect = RECT{label_left, top, (std::max)(label_left, controls_left - f.dip(8)), query_bottom};
    const int body_top = (std::min)(query_bottom + body_gap, static_cast<int>(panel.bottom));
    const int body_bottom = (std::min)(body_top + body_height, static_cast<int>(panel.bottom));
    result.candidate_area_rect = RECT{left, body_top, right, body_bottom};
    result.focus_rect = result.candidate_area_rect;

    int y = body_top;
    for (size_t i = slice.begin; i < slice.end; ++i) {
        CandidateItemLayout item;
        item.candidate_index = i;
        item.page_number = i - slice.begin + 1;
        item.selected = i == slice.selection;
        item.featured = i == slice.focus;
        item.display_text = input.candidate_texts[i];
        item.number_text_width_px = number_widths[i];
        item.candidate_text_width_px = text_widths[i];
        item.horizontal_padding_px = row_pad;
        item.number_gap_px = number_gap;
        const int row_bottom = (std::min)(y + row_height, body_bottom);
        item.bounds = RECT{left, y, right, (std::max)(y + 1, row_bottom)};
        const int number_left = (std::min)(left + row_pad, right);
        item.number_bounds = RECT{number_left, item.bounds.top, (std::min)(number_left + number_widths[i], right),
                                  item.bounds.bottom};
        const int text_left = (std::min)(static_cast<int>(item.number_bounds.right) + number_gap, right);
        item.text_bounds = RECT{text_left, item.bounds.top, (std::max)(text_left, right - row_pad), item.bounds.bottom};
        finish_item(item);
        result.items.push_back(std::move(item));
        y = static_cast<int>(result.items.back().bounds.bottom) + row_gap;
    }
    place_near_caret(result, input, result.composition_rect.left);
    return result;
}

RECT marker_column_list(const CandidateLayoutResult& layout, std::optional<size_t>) {
    const CandidateItemLayout* item = candidate_paint::selected_item(layout);
    return item ? item->bounds : RECT{};
}

void paint_column_list(const CandidatePaintContext& context) {
    auto& canvas = context.canvas;
    const auto& layout = context.layout;
    const auto& palette = context.palette;
    const auto& state = context.state;
    const float u = candidate_paint::unit(layout);
    const RECT& panel = layout.panel_rect;

    canvas.rounded(panel, static_cast<float>(layout.corner_radius), palette.background, palette.border);
    candidate_paint::query_text(context, layout.composition_rect, static_cast<float>(layout.composition_font_px),
                                FW_NORMAL, palette.composition_text, palette.separator_text, palette.caret);
    candidate_paint::page_controls(context, 6.0f * u, palette.focus_bar);
    candidate_paint::settings_gear(context, 6.0f * u);
    // A hairline under the pinyin, inset like the rows.
    const float rule_y = static_cast<float>(layout.query_rect.bottom) - 0.5f;
    canvas.line(static_cast<float>(layout.candidate_area_rect.left) + 6.0f * u, rule_y,
                static_cast<float>(layout.candidate_area_rect.right) - 6.0f * u, rule_y, palette.divider, 1.0f);

    const float radius = static_cast<float>(layout.selected_corner_radius);
    for (const auto& item : layout.items) {
        const bool hovered = state.hovered == item.candidate_index;
        const bool pressed = state.pressed == item.candidate_index;
        if (!item.featured && (hovered || pressed))
            canvas.rounded(item.bounds, radius, pressed ? palette.pressed_background : palette.hover_background,
                           pressed ? palette.pressed_background : palette.hover_background);
    }
    const CandidateItemLayout* chosen = candidate_paint::selected_item(layout);
    const RECT row = !candidate_paint::is_empty(state.marker) ? state.marker : chosen ? chosen->bounds : RECT{};
    if (!candidate_paint::is_empty(row)) {
        canvas.rounded(row, radius, palette.focus_background, palette.focus_border);
        // The brand caret's short bar at the row's left edge.
        const float bar_x = static_cast<float>(row.left) + 1.5f * u;
        const float inset = static_cast<float>(row.bottom - row.top) * 0.3f;
        canvas.line(bar_x, row.top + inset, bar_x, row.bottom - inset, palette.focus_bar, 2.0f * u);
    }

    const float number_px = static_cast<float>(layout.number_font_px);
    const float size = static_cast<float>(layout.candidate_font_px);
    for (const auto& item : layout.items) {
        const bool focused = item.featured;
        const bool hovered = state.hovered == item.candidate_index;
        canvas.text(std::to_wstring(item.page_number), item.number_bounds,
                    focused ? palette.focus_number_text : palette.number_text, number_px,
                    focused ? FW_SEMIBOLD : FW_NORMAL, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
        canvas.text(item.display_text, item.text_bounds,
                    focused ? palette.focus_text : hovered ? palette.text : palette.text_muted, size,
                    focused ? FW_SEMIBOLD : FW_NORMAL, candidate_text_draw_flags(item.truncated));
    }
}

}  // namespace localpinyin::candidate_styles
