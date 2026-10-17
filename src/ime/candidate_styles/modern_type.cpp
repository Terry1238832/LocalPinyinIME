// Modern Type: the candidates as a line of type. A small vermilion print
// mark and the pinyin sit above a hairline; below, the current choice is set
// larger and bold, the alternatives in a light weight, all on one baseline,
// each led by a small superscript figure. A short vermilion rule under the
// current choice moves to the candidate under the pointer. No fills, no
// boxes: size, weight, color and one rule carry the hierarchy.
#include "layout_core.h"
#include "paint_core.h"

#include <algorithm>
#include <cmath>

namespace localpinyin::candidate_styles {

using namespace candidate_layout;

namespace {

constexpr float kBaselineEm = 1.06f;
constexpr float kLineEm = 1.32f;
constexpr int kFocusWeight = 700;
constexpr int kItemWeight = 300;

}  // namespace

CandidateLayoutResult layout_modern_type(const CandidateLayoutInput& input) {
    CandidateLayoutResult result;
    result.style = CandidateStyle::ModernType;
    result.dpi = (std::max)(1, input.dpi);
    const auto frame = Frame::begin(input);
    if (!frame) return result;
    const Frame& f = *frame;
    const size_t count = f.count();

    const int max_width = (std::min)(f.dip(720), f.work_width());
    const int margin = f.dip(kShadowMarginDip);
    const int side = f.fit(18, 20);
    const ControlMetrics controls{f.fit(18, 14), f.fit(30, 14), f.fit(2, 160)};
    result.composition_font_px = f.dip(12);
    result.candidate_font_px = f.candidate_font_px();
    result.focus_font_px = static_cast<int>(std::lround(result.candidate_font_px * 1.3f));
    result.number_font_px = (std::max)(f.dip(9), static_cast<int>(std::lround(result.candidate_font_px * 0.5f)));
    result.hint_font_px = f.dip(11);
    result.focus_weight = kFocusWeight;
    result.item_weight = kItemWeight;
    result.number_face = CandidateFontFace::Numeral;
    const int label_height = f.dip(28);
    const int number_gap = f.fit(2, 160);
    // Rhythm: about one em between words; hit areas split the space.
    const int spacing = (std::max)(f.dip(16), result.candidate_font_px);
    // Hit areas never reach past the panel's side margin.
    const int hit_pad = (std::max)(0, (std::min)(side, spacing / 2 - f.dip(1)));
    const int gap = spacing - hit_pad * 2;
    const int row_top_space = f.dip(8);
    const int line = static_cast<int>(std::lround(result.focus_font_px * kLineEm));
    const int rule_gap = (std::max)(f.dip(3), static_cast<int>(std::lround(result.focus_font_px * 0.12f)));
    const int rule = (std::max)(2, f.dip(2));
    const int bottom_space = f.dip(10);
    const int inner_max = (std::max)(1, max_width - (side - hit_pad) * 2);

    std::vector<int> text_widths(count), focus_widths(count), number_widths(count);
    for (size_t i = 0; i < count; ++i) {
        text_widths[i] = f.candidate_width(i, result.candidate_font_px, kItemWeight);
        focus_widths[i] = f.candidate_width(i, result.focus_font_px, kFocusWeight);
        number_widths[i] = f.digit_width(i, result.number_font_px, CandidateFontFace::Numeral);
    }
    const auto slot_width = [&](size_t i, bool focus) {
        return hit_pad * 2 + number_widths[i] + number_gap + (focus ? focus_widths[i] : text_widths[i]);
    };
    const auto slice = apply_pages(result, input,
        paginate(count, inner_max, gap, [&](size_t i, bool first) { return slot_width(i, first); }), count);

    int used = 0;
    for (size_t i = slice.begin; i < slice.end; ++i) used += (used ? gap : 0) + slot_width(i, i == slice.focus);
    const int controls_space = controls_width(result.paging_visible, input.settings_button, controls);
    const int mark = (std::max)(4, f.dip(5));
    const int query_needed = hit_pad + mark + f.dip(8) + f.composition_width(result.composition_font_px) + f.dip(16) +
                             controls_space + hit_pad;
    const int inner_min = (std::min)(f.dip(220), inner_max);
    const int inner_width = clamp_int((std::max)(used, query_needed), inner_min, inner_max);
    const int panel_width = inner_width + (side - hit_pad) * 2;
    const int row_height = row_top_space + line + rule_gap + rule;
    const int panel_height = (std::min)(label_height + row_height + bottom_space, f.work_height());
    set_panel(result, margin, panel_width, panel_height);
    result.panel_padding_px = side;
    result.item_gap_px = gap;
    result.corner_radius = f.dip(7);
    result.selected_corner_radius = f.dip(4);
    result.chip_corner_radius = f.dip(4);

    const RECT& panel = result.panel_rect;
    const int left = panel.left + side;
    const int right = panel.right - side;
    const int label_bottom = (std::min)(static_cast<int>(panel.top) + label_height, static_cast<int>(panel.bottom));
    result.query_rect = RECT{panel.left, panel.top, panel.right, label_bottom};
    const int center = panel.top + label_height / 2 + f.dip(1);
    const int controls_left = place_controls(result, input, right + hit_pad / 2, center, controls);
    result.mark_rect = RECT{left, center - mark / 2, left + mark, center - mark / 2 + mark};
    const int label_left = result.mark_rect.right + f.dip(8);
    result.composition_rect = RECT{label_left, panel.top + f.dip(2), (std::max)(label_left, controls_left - f.dip(10)),
                                   label_bottom};
    // The hairline under the pinyin.
    result.rail_rect = RECT{left, label_bottom, right, label_bottom + 1};

    const int row_top = label_bottom + row_top_space;
    result.baseline_y = row_top + static_cast<int>(std::lround(result.focus_font_px * kBaselineEm));
    // The band the short rule moves along, just under the ideographs.
    const int rule_top = result.baseline_y + static_cast<int>(std::lround(result.focus_font_px * (kLineEm - kBaselineEm))) -
                         static_cast<int>(std::lround(result.focus_font_px * 0.1f)) + rule_gap;
    result.track_rect = RECT{left, rule_top, right, rule_top + rule};
    const int row_bottom = (std::min)(rule_top + rule + f.dip(3), static_cast<int>(panel.bottom));
    result.candidate_area_rect = RECT{left - hit_pad, label_bottom + 1, right + hit_pad, row_bottom};
    result.focus_rect = result.candidate_area_rect;

    const bool fits = used <= inner_width;
    const size_t shown = slice.end - slice.begin;
    int x = left - hit_pad;
    for (size_t i = slice.begin; i < slice.end; ++i) {
        CandidateItemLayout item;
        item.candidate_index = i;
        item.page_number = i - slice.begin + 1;
        item.selected = i == slice.selection;
        item.featured = i == slice.focus;
        item.display_text = input.candidate_texts[i];
        item.number_text_width_px = number_widths[i];
        item.candidate_text_width_px = item.featured ? focus_widths[i] : text_widths[i];
        item.horizontal_padding_px = hit_pad;
        item.number_gap_px = number_gap;
        const int width = fits ? slot_width(i, item.featured) : squeezed_width(inner_width, gap, shown);
        item.bounds = RECT{x, label_bottom + 1, x + width, row_bottom};
        const int number_left = (std::min)(x + hit_pad, x + width);
        item.number_bounds = RECT{number_left, row_top, (std::min)(number_left + number_widths[i], x + width), result.baseline_y};
        const int text_left = (std::min)(static_cast<int>(item.number_bounds.right) + number_gap, x + width);
        item.text_bounds = RECT{text_left, row_top, (std::max)(text_left, x + width - hit_pad), row_top + line};
        finish_item(item);
        result.items.push_back(std::move(item));
        x += width + gap;
    }
    place_near_caret(result, input, result.composition_rect.left);
    return result;
}

RECT marker_modern_type(const CandidateLayoutResult& layout, std::optional<size_t> hovered) {
    const CandidateItemLayout* item = hovered ? candidate_paint::item_for(layout, *hovered) : nullptr;
    if (!item) item = candidate_paint::selected_item(layout);
    if (!item) return RECT{};
    // A short rule: about one ideograph wide, never wider than the word.
    const int size = item->featured ? layout.focus_font_px : layout.candidate_font_px;
    const int word = (std::min)(item->candidate_text_width_px, item->text_available_width_px);
    const int width = (std::max)(0, (std::min)(word, (std::max)(scale_dip(14, layout.dpi), size)));
    return RECT{item->text_bounds.left, layout.track_rect.top, item->text_bounds.left + width, layout.track_rect.bottom};
}

void paint_modern_type(const CandidatePaintContext& context) {
    auto& canvas = context.canvas;
    const auto& layout = context.layout;
    const auto& palette = context.palette;
    const auto& state = context.state;
    const float u = candidate_paint::unit(layout);
    const RECT& panel = layout.panel_rect;
    const float radius = static_cast<float>(layout.corner_radius);

    // A sheet: paper (light) or deep ink (dark), one hairline edge.
    canvas.rounded(panel, radius, palette.background, palette.background);
    canvas.outline(panel, radius, palette.border, 1.0f);

    // The print mark, then the pinyin set small.
    if (!candidate_paint::is_empty(layout.mark_rect))
        canvas.fill(layout.mark_rect, palette.mark);
    candidate_paint::query_text(context, layout.composition_rect, static_cast<float>(layout.composition_font_px),
                                FW_NORMAL, palette.composition_text, palette.separator_text, palette.caret);
    candidate_paint::page_controls(context, 4.0f * u, palette.focus_bar);
    candidate_paint::settings_gear(context, 4.0f * u);
    canvas.fill(layout.rail_rect, palette.divider);

    const int baseline = layout.baseline_y;
    const float number_px = static_cast<float>(layout.number_font_px);
    const wchar_t* numeral = candidate_font_family(CandidateFontFace::Numeral);
    for (const auto& item : layout.items) {
        const bool hovered = state.hovered == item.candidate_index;
        const bool pressed = state.pressed == item.candidate_index;
        const bool focus = item.featured;
        const float size = static_cast<float>(focus ? layout.focus_font_px : layout.candidate_font_px);
        const int weight = focus ? layout.focus_weight : layout.item_weight;
        // The figure hangs from the word's top, like a superscript.
        RECT number = item.number_bounds;
        const float word_top = baseline - size * 0.86f;
        number.top = static_cast<LONG>(std::lround(word_top + number_px * 0.05f));
        number.bottom = number.top + static_cast<LONG>(std::lround(number_px * 1.5f));
        canvas.text(std::to_wstring(item.page_number), number,
                    focus ? palette.focus_number_text : palette.number_text, number_px, FW_NORMAL,
                    DT_SINGLELINE | DT_TOP | DT_NOPREFIX, numeral);
        RECT text = item.text_bounds;
        text.top = baseline - static_cast<LONG>(std::lround(canvas.baseline(size, weight)));
        text.bottom = text.top + static_cast<LONG>(std::lround(size * 1.4f));
        const COLORREF ink = focus ? palette.focus_text : (hovered || pressed) ? palette.text : palette.text_muted;
        canvas.text(item.display_text, text, ink, size, weight,
                    (candidate_text_draw_flags(item.truncated) & ~DT_VCENTER) | DT_TOP);
    }
    // The short rule, where the window's animation has it this frame.
    if (!candidate_paint::is_empty(state.marker)) canvas.fill(state.marker, palette.focus_bar);
}

}  // namespace localpinyin::candidate_styles
