// Adaptive Flow: the layout follows the candidates, and the current choice
// is a drop of liquid. Every form shares the query bar (a drop mark, pinyin,
// pager, gear), so the pinyin stays under the caret whatever the form:
//   Compact   one row of chips, the current choice first: short words.
//   Balanced  the current choice on its own, larger, row with the commit key;
//             one row of chips below: mid-length or uneven candidates.
//   Expanded  one candidate per row, wrapping to two lines: long words and
//             sentences.
// decide_candidate_flow_form chooses with hysteresis from measured widths,
// so the popup does not flip shape on every key.
//
// The drop sits behind the current choice, hugging its text, and turns the
// text it covers white. It flows: moving to another candidate (the pointer,
// the arrow keys, a new first choice as you type), its leading edge runs
// ahead and a little past while the trailing edge follows, so it stretches
// out, draws in at the middle and gathers again, its surface sloshing. A new
// popup pours it in from the left, and candidates that changed rise into
// place one after another.
#include "layout_core.h"
#include "paint_core.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace localpinyin {

CandidateFlowForm decide_candidate_flow_form(const CandidateFlowMeasures& measures,
                                             std::optional<CandidateFlowForm> previous) noexcept {
    const float em = static_cast<float>((std::max)(1, measures.font_px));
    const float longest = static_cast<float>(measures.longest_px);
    const float inner = static_cast<float>((std::max)(1, measures.inner_max_px));
    const float spread = longest / static_cast<float>((std::max)(1, measures.median_px));
    // Entering a form takes a clear signal; staying in it a weaker one.
    const float long_enter = (std::max)(9.0f * em, 0.42f * inner);
    const float long_stay = (std::max)(7.0f * em, 0.33f * inner);
    if (longest >= long_enter || (previous == CandidateFlowForm::Expanded && longest >= long_stay))
        return CandidateFlowForm::Expanded;
    const bool compact = previous == CandidateFlowForm::Compact
        ? longest <= 4.2f * em && spread <= 4.0f
        : longest <= 3.2f * em && spread <= 2.6f;
    return compact ? CandidateFlowForm::Compact : CandidateFlowForm::Balanced;
}

std::wstring candidate_flow_form_to_string(CandidateFlowForm form) {
    switch (form) {
        case CandidateFlowForm::Compact: return L"compact";
        case CandidateFlowForm::Expanded: return L"expanded";
        case CandidateFlowForm::Balanced:
        default: return L"balanced";
    }
}

namespace candidate_styles {

using namespace candidate_layout;

namespace {

constexpr float kLineEm = 1.32f;
constexpr size_t kExpandedRows = 5;
constexpr float kPi = 3.14159265f;
// The drop pours in from its left end when the popup appears.
constexpr float kPourMs = 340.0f;
// Changed candidates rise into place, the next one this much later.
constexpr float kRiseMs = 220.0f;
constexpr float kRiseStaggerMs = 26.0f;

// Leading edges arrive in the first 60% of a move, a little past the target
// and back; trailing edges set off later and ease in and out, so the drop
// stretches out and gathers itself again.
float lead_ease(float t) noexcept {
    const float x = std::clamp(t / 0.6f, 0.0f, 1.0f) - 1.0f;
    constexpr float overshoot = 1.2f;
    return 1.0f + (overshoot + 1.0f) * x * x * x + overshoot * x * x;
}

float trail_ease(float t) noexcept {
    const float x = std::clamp((t - 0.18f) / 0.82f, 0.0f, 1.0f);
    return x < 0.5f ? 4.0f * x * x * x : 1.0f - std::pow(2.0f - 2.0f * x, 3.0f) / 2.0f;
}

struct DropEdges {
    float left = 0, top = 0, right = 0, bottom = 0;
};

DropEdges rest_edges(const RECT& rect) noexcept {
    return DropEdges{static_cast<float>(rect.left), static_cast<float>(rect.top), static_cast<float>(rect.right),
                     static_cast<float>(rect.bottom)};
}

// The edge on the side the drop travels toward leads, the one behind it
// trails (both lead when it only grows or shrinks about its middle).
DropEdges drop_edges(const RECT& from, const RECT& to, float t) noexcept {
    const float travel_x = (to.left + to.right - from.left - from.right) / 2.0f;
    const float travel_y = (to.top + to.bottom - from.top - from.bottom) / 2.0f;
    const auto edge = [t](LONG a, LONG b, bool leads) {
        return static_cast<float>(a) + static_cast<float>(b - a) * (leads ? lead_ease(t) : trail_ease(t));
    };
    const bool still_x = std::abs(travel_x) < 0.5f, still_y = std::abs(travel_y) < 0.5f;
    DropEdges e{edge(from.left, to.left, still_x || travel_x < 0.0f), edge(from.top, to.top, still_y || travel_y < 0.0f),
                edge(from.right, to.right, still_x || travel_x > 0.0f), edge(from.bottom, to.bottom, still_y || travel_y > 0.0f)};
    e.right = (std::max)(e.right, e.left + 2.0f);
    e.bottom = (std::max)(e.bottom, e.top + 2.0f);
    return e;
}

// A rounded bar whose long sides draw in toward the middle by `pinch`: the
// neck of a stretched drop (none at rest).
std::vector<D2D1_POINT_2F> drop_outline(const DropEdges& e, float radius, float pinch, bool vertical) {
    const float r = (std::max)(0.0f, (std::min)({radius, (e.right - e.left) / 2.0f, (e.bottom - e.top) / 2.0f}));
    std::vector<D2D1_POINT_2F> points;
    points.reserve(96);
    const auto arc = [&](float cx, float cy, float start) {
        for (int k = 0; k <= 6; ++k) {
            const float a = start + kPi / 2.0f * static_cast<float>(k) / 6.0f;
            points.push_back(D2D1::Point2F(cx + r * std::cos(a), cy + r * std::sin(a)));
        }
    };
    const auto side = [&](float x0, float y0, float x1, float y1, float nx, float ny, bool pinched) {
        constexpr int kSteps = 16;
        for (int k = 1; k < kSteps; ++k) {
            const float along = static_cast<float>(k) / kSteps;
            const float wave = std::sin(kPi * along);
            const float in = pinched ? pinch * wave * wave : 0.0f;
            points.push_back(D2D1::Point2F(x0 + (x1 - x0) * along + nx * in, y0 + (y1 - y0) * along + ny * in));
        }
    };
    arc(e.left + r, e.top + r, kPi);
    side(e.left + r, e.top, e.right - r, e.top, 0.0f, 1.0f, !vertical);
    arc(e.right - r, e.top + r, 1.5f * kPi);
    side(e.right, e.top + r, e.right, e.bottom - r, -1.0f, 0.0f, vertical);
    arc(e.right - r, e.bottom - r, 0.0f);
    side(e.right - r, e.bottom, e.left + r, e.bottom, 0.0f, -1.0f, !vertical);
    arc(e.left + r, e.bottom - r, 0.5f * kPi);
    side(e.left, e.bottom - r, e.left, e.top + r, 1.0f, 0.0f, vertical);
    return points;
}

// Where the drop rests on a candidate: from the row's left edge to a little
// past the end of its text, and at least a round pill.
RECT drop_rest(const CandidateLayoutResult& layout, const CandidateItemLayout& item) noexcept {
    RECT rest = item.bounds;
    const LONG lead = (std::max)(LONG{0}, item.number_bounds.left - item.bounds.left);
    const LONG text = (std::min)(static_cast<LONG>(item.candidate_text_width_px), item.text_bounds.right - item.text_bounds.left);
    const LONG hug = item.text_bounds.left + text + lead + scale_dip(2, layout.dpi);
    const LONG pill = rest.left + (rest.bottom - rest.top) * 3 / 2;
    rest.right = (std::min)(rest.right, (std::max)(hug, pill));
    return rest;
}

// The brand mark: a drop on its tip, shaded like the focus, with a glint.
void drop_mark(ModernRenderer& canvas, const RECT& box, const CandidateThemePalette& palette) {
    const float size = static_cast<float>((std::min)(box.right - box.left, box.bottom - box.top));
    const float r = size * 0.48f;
    const float cx = (box.left + box.right) / 2.0f;
    const float top = box.top + (box.bottom - box.top - 2.0f * r) / 2.0f;
    std::vector<D2D1_POINT_2F> points;
    for (int k = 0; k < 40; ++k) {
        const float a = 2.0f * kPi * static_cast<float>(k) / 40.0f;
        points.push_back(D2D1::Point2F(cx + r * std::sin(a) * std::sin(a / 2.0f), top + r * (1.0f - std::cos(a))));
    }
    if (palette.high_contrast) {
        canvas.polygon(points, palette.mark);
        return;
    }
    canvas.polygon_gradient(points, palette.focus_top, palette.mark, top, top + 2.0f * r);
    const auto at = [](float v) { return static_cast<LONG>(std::lround(v)); };
    canvas.rounded_fill(RECT{at(cx - 0.42f * r), at(top + 1.02f * r), at(cx - 0.14f * r), at(top + 1.30f * r)}, 0.14f * r,
                        RGB(255, 255, 255), 0.6f);
}

}  // namespace

RECT marker_adaptive_flow(const CandidateLayoutResult& layout, std::optional<size_t> hovered) {
    // Balanced gives the current choice its own, larger row: the drop stays
    // there. Elsewhere it flows to the candidate under the pointer.
    const CandidateItemLayout* item = nullptr;
    if (hovered && layout.flow_form != CandidateFlowForm::Balanced) item = candidate_paint::item_for(layout, *hovered);
    if (!item) {
        for (const auto& candidate : layout.items) {
            if (candidate.featured) item = &candidate;
        }
    }
    if (!item) item = candidate_paint::selected_item(layout);
    return item ? drop_rest(layout, *item) : RECT{};
}

RECT flow_drop_frame(const RECT& from, const RECT& to, float t) noexcept {
    if (candidate_paint::is_empty(from) || candidate_paint::is_empty(to) || t >= 1.0f) return to;
    const DropEdges e = drop_edges(from, to, t);
    return RECT{static_cast<LONG>(std::floor(e.left)), static_cast<LONG>(std::floor(e.top)),
                static_cast<LONG>(std::ceil(e.right)), static_cast<LONG>(std::ceil(e.bottom))};
}

CandidateLayoutResult layout_adaptive_flow(const CandidateLayoutInput& input) {
    CandidateLayoutResult result;
    result.style = CandidateStyle::AdaptiveFlow;
    result.dpi = (std::max)(1, input.dpi);
    const auto frame = Frame::begin(input);
    if (!frame) return result;
    const Frame& f = *frame;
    const size_t count = f.count();
    const int dpi = result.dpi;

    // Every measure shrinks proportionally on a work area narrower than the
    // nominal popup, so the panel and its rows always fit the monitor.
    const int max_width = (std::min)(f.dip(720), f.work_width());
    const int margin = f.dip(kShadowMarginDip);
    const int padding = (std::min)(f.dip(6), max_width / 16);
    const int gap = (std::min)(f.dip(4), max_width / 64);
    const ControlMetrics controls{(std::min)(f.dip(22), max_width / 10), (std::min)(f.dip(30), max_width / 12), gap};
    const int mark = (std::min)(f.dip(18), max_width / 12);
    result.composition_font_px = f.dip(14);
    result.candidate_font_px = f.candidate_font_px();
    result.number_font_px = candidate_number_font_px(result.candidate_font_px, dpi);
    result.hint_font_px = f.dip(11);
    const int keycap_height = result.number_font_px + f.dip(6);
    const int query_height = result.composition_font_px + f.dip(20);
    const int chip_height = result.candidate_font_px + f.dip(12);
    const int row_gap = f.dip(6);
    const int inner_max = (std::max)(1, max_width - padding * 2);
    const int chip_left = (std::min)(f.dip(5), inner_max / 16);
    const int chip_right = (std::min)(f.dip(9), inner_max / 12);
    const int number_gap = (std::min)(f.dip(5), inner_max / 16);
    const int focus_left = f.dip(12);
    const int focus_right = f.dip(10);
    const int focus_number_gap = f.dip(8);
    const int slack = f.dip(2);
    const int hint_width = input.focus_hint.empty() ? 0 : (input.focus_hint_width_px > 0 && !input.measure
        ? input.focus_hint_width_px : f.measure(input.focus_hint, result.hint_font_px)) + f.dip(10);
    const int hint_height = result.hint_font_px + f.dip(8);
    const int hint_space = hint_width ? f.dip(12) + hint_width : 0;

    std::vector<int> text_widths(count), number_widths(count), keycap_widths(count), chip_widths(count);
    for (size_t i = 0; i < count; ++i) {
        text_widths[i] = f.candidate_width(i, result.candidate_font_px);
        number_widths[i] = f.digit_width(i, result.number_font_px);
        keycap_widths[i] = (std::max)(keycap_height, number_widths[i] + f.dip(6));
        chip_widths[i] = (std::min)(inner_max, chip_left + keycap_widths[i] + number_gap + text_widths[i] + chip_right + slack);
    }

    // The form, from the measured candidates.
    std::vector<int> sorted(text_widths.begin(), text_widths.end());
    std::sort(sorted.begin(), sorted.end());
    CandidateFlowMeasures measures;
    measures.font_px = result.candidate_font_px;
    measures.longest_px = sorted.back();
    measures.median_px = sorted[sorted.size() / 2];
    measures.inner_max_px = inner_max;
    const CandidateFlowForm form = decide_candidate_flow_form(measures, input.previous_flow_form);
    result.flow_form = form;
    result.focus_font_px = form == CandidateFlowForm::Balanced
        ? static_cast<int>(std::lround(result.candidate_font_px * 1.17f)) : result.candidate_font_px;
    std::vector<int> focus_widths(count);
    for (size_t i = 0; i < count; ++i) {
        focus_widths[i] = form == CandidateFlowForm::Balanced
            ? (i < input.focus_text_widths_px.size() && input.focus_text_widths_px[i] > 0 && !input.measure
                   ? input.focus_text_widths_px[i] : f.candidate_width(i, result.focus_font_px))
            : text_widths[i];
    }
    // Compact: the leading chip makes room for the focus bar.
    const int lead_extra = f.dip(4);
    const int line = static_cast<int>(std::lround(result.candidate_font_px * kLineEm));
    const int expanded_inner_max = (std::min)(inner_max, f.dip(600) - padding * 2);
    const int row_pad = f.dip(8);
    const int expanded_row_gap = f.dip(4);

    // Pages.
    std::vector<UINT> pages;
    if (form == CandidateFlowForm::Compact) {
        pages = paginate(count, inner_max, gap,
            [&](size_t i, bool first) { return chip_widths[i] + (first ? lead_extra : 0); });
    } else if (form == CandidateFlowForm::Balanced) {
        pages = paginate(count, inner_max, gap, [&](size_t i, bool) { return chip_widths[i]; }, true);
    } else {
        // Up to five rows, and only as many as the monitor height allows.
        const int rows_height = (std::max)(1, f.work_height() - query_height - padding * 2);
        for (size_t start = 0; start < count;) {
            pages.push_back(static_cast<UINT>(start));
            size_t end = start + 1;
            int used = row_pad + line * 2;
            for (; end < count && end - start < kExpandedRows; ++end) {
                used += expanded_row_gap + row_pad + line * 2;
                if (used > rows_height) break;
            }
            start = end;
        }
    }
    const auto slice = apply_pages(result, input, std::move(pages), count);
    const size_t shown = slice.end - slice.begin;
    const bool has_chips = form == CandidateFlowForm::Balanced && shown > 1;

    // Widths the page needs.
    const int controls_space = controls_width(result.paging_visible, input.settings_button, controls);
    const int query_needed = f.dip(4) + mark + f.dip(8) + f.composition_width(result.composition_font_px) + f.dip(14) +
                             controls_space + f.dip(2);
    int content_needed = 0;
    bool hint_fits = false;
    if (form == CandidateFlowForm::Compact) {
        for (size_t i = slice.begin; i < slice.end; ++i)
            content_needed += (content_needed ? gap : 0) + chip_widths[i] + (i == slice.focus ? lead_extra : 0);
    } else if (form == CandidateFlowForm::Balanced) {
        int chips_used = 0;
        for (size_t i = slice.begin; i < slice.end; ++i)
            if (i != slice.focus) chips_used += (chips_used ? gap : 0) + chip_widths[i];
        const int focus_text_needed = focus_left + keycap_widths[slice.focus] + focus_number_gap +
                                      focus_widths[slice.focus] + focus_right;
        // The candidate text needs the room more than the commit-key hint.
        hint_fits = hint_width > 0 && focus_text_needed + hint_space <= inner_max;
        content_needed = (std::max)(chips_used, focus_text_needed + (hint_fits ? hint_space : 0));
    } else {
        for (size_t i = slice.begin; i < slice.end; ++i) {
            const int lead = i == slice.focus ? focus_left : chip_left;
            content_needed = (std::max)(content_needed, lead + keycap_widths[i] + focus_number_gap + text_widths[i] +
                                                        focus_right + (i == slice.focus ? hint_space : 0));
        }
        hint_fits = hint_width > 0;
    }
    const int width_cap = form == CandidateFlowForm::Expanded ? expanded_inner_max : inner_max;
    const int inner_min = (std::min)(f.dip(228), width_cap);
    const int inner_width = clamp_int((std::max)(content_needed, query_needed), inner_min, width_cap);
    const int panel_width = inner_width + padding * 2;

    // Rows and heights.
    const int focus_height = form == CandidateFlowForm::Balanced ? result.focus_font_px + f.dip(16) : chip_height;
    std::vector<int> row_lines;
    int body_height = 0;
    if (form == CandidateFlowForm::Compact) {
        body_height = chip_height;
    } else if (form == CandidateFlowForm::Balanced) {
        body_height = focus_height + (has_chips ? row_gap + chip_height : 0);
    } else {
        for (size_t i = slice.begin; i < slice.end; ++i) {
            const bool focus = i == slice.focus;
            const int lead = focus ? focus_left : chip_left;
            const int available = inner_width - lead - keycap_widths[i] - focus_number_gap - focus_right -
                                  (focus && hint_fits ? hint_space : 0);
            const int lines = text_widths[i] > (std::max)(1, available) ? 2 : 1;
            row_lines.push_back(lines);
            body_height += (body_height ? expanded_row_gap : 0) + row_pad + line * lines;
        }
    }
    const int panel_height = (std::min)(query_height + body_height + padding, f.work_height());
    set_panel(result, margin, panel_width, panel_height);
    result.panel_padding_px = padding;
    result.item_gap_px = gap;
    result.row_gap_px = form == CandidateFlowForm::Balanced ? (has_chips ? row_gap : 0)
                      : form == CandidateFlowForm::Expanded ? expanded_row_gap : 0;
    result.corner_radius = f.dip(14);
    // The drop: as round as its height allows, up to this.
    result.selected_corner_radius = f.dip(12);
    result.chip_corner_radius = f.dip(9);

    const RECT& panel = result.panel_rect;
    const int left = panel.left + padding;
    const int right = panel.right - padding;
    const int bottom = panel.bottom;
    const int top = panel.top;
    // Query bar: brand mark, pinyin, then the pager and the settings gear.
    const int query_bottom = (std::min)(top + query_height, bottom);
    result.query_rect = RECT{panel.left, top, panel.right, query_bottom};
    const int controls_right = right - f.dip(2);
    const int controls_left = place_controls(result, input, controls_right, top + query_height / 2, controls);
    const int mark_left = left + f.dip(4);
    const int mark_top = top + (query_height - mark) / 2;
    result.mark_rect = RECT{mark_left, mark_top, mark_left + mark, mark_top + mark};
    const int label_left = result.mark_rect.right + f.dip(8);
    result.composition_rect = RECT{label_left, top, (std::max)(label_left, controls_left - f.dip(8)), query_bottom};

    const int body_top = query_bottom;
    const int body_bottom = (std::min)(body_top + body_height, bottom);
    result.candidate_area_rect = RECT{left, body_top, right, body_bottom};
    result.show_hint = hint_fits && form != CandidateFlowForm::Compact;

    const auto keycap_box = [&](int x, int row_top, int row_height, size_t i) {
        const int keycap_top = row_top + (row_height - keycap_height) / 2;
        return RECT{x, keycap_top, x + keycap_widths[i], keycap_top + keycap_height};
    };
    const auto place_hint = [&](const RECT& row) {
        const int hint_top = row.top + (static_cast<int>(row.bottom - row.top) - hint_height) / 2;
        result.hint_rect = RECT{right - focus_right - hint_width, hint_top, right - focus_right, hint_top + hint_height};
    };

    if (form == CandidateFlowForm::Compact) {
        const int row_bottom = body_bottom;
        result.chips_rect = RECT{left, body_top, right, row_bottom};
        const bool fits = content_needed <= inner_width;
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
            const int lead = chip_left + (item.featured ? lead_extra : 0);
            item.horizontal_padding_px = lead;
            item.number_gap_px = number_gap;
            const int width = fits ? chip_widths[i] + (item.featured ? lead_extra : 0) : squeezed_width(inner_width, gap, shown);
            item.bounds = RECT{x, body_top, x + width, row_bottom};
            const int keycap_width = (std::min)(keycap_widths[i], (std::max)(0, width - lead - chip_right));
            item.number_bounds = keycap_box((std::min)(x + lead, x + width), body_top, chip_height, i);
            item.number_bounds.right = item.number_bounds.left + keycap_width;
            const int text_left = (std::min)(x + width, static_cast<int>(item.number_bounds.right) + number_gap);
            item.text_bounds = RECT{text_left, body_top, (std::max)(text_left, x + width - chip_right), row_bottom};
            finish_item(item);
            result.items.push_back(std::move(item));
            x += width + gap;
        }
        result.focus_rect = result.items.front().bounds;
        for (const auto& item : result.items) if (item.featured) result.focus_rect = item.bounds;
    } else if (form == CandidateFlowForm::Balanced) {
        const int focus_top = body_top;
        const int focus_bottom = (std::min)(focus_top + focus_height, bottom);
        result.focus_rect = RECT{left, focus_top, right, focus_bottom};
        if (result.show_hint) place_hint(result.focus_rect);
        const int chips_top = (std::min)(focus_bottom + row_gap, bottom);
        const int chips_bottom = (std::min)(chips_top + chip_height, bottom);
        if (has_chips) result.chips_rect = RECT{left, chips_top, right, chips_bottom};
        int chips_used = 0;
        for (size_t i = slice.begin; i < slice.end; ++i)
            if (i != slice.focus) chips_used += (chips_used ? gap : 0) + chip_widths[i];
        const int chip_count = static_cast<int>(shown) - 1;
        const bool chips_fit = chips_used <= inner_width;
        int x = left;
        for (size_t i = slice.begin; i < slice.end; ++i) {
            CandidateItemLayout item;
            item.candidate_index = i;
            item.page_number = i - slice.begin + 1;
            item.selected = i == slice.selection;
            item.featured = i == slice.focus;
            item.display_text = input.candidate_texts[i];
            item.number_text_width_px = number_widths[i];
            if (item.featured) {
                item.bounds = result.focus_rect;
                item.number_bounds = keycap_box(left + focus_left, focus_top, focus_height, i);
                const int text_left = item.number_bounds.right + focus_number_gap;
                const int text_right = result.show_hint ? result.hint_rect.left - f.dip(12) : right - focus_right;
                item.text_bounds = RECT{text_left, focus_top, (std::max)(text_left, text_right), focus_bottom};
                item.candidate_text_width_px = focus_widths[i];
                item.horizontal_padding_px = focus_left;
                item.number_gap_px = focus_number_gap;
            } else {
                const int chip_width = chips_fit ? chip_widths[i] : squeezed_width(inner_width, gap, chip_count);
                const int keycap_width = (std::min)(keycap_widths[i], (std::max)(0, chip_width - chip_left - chip_right));
                item.bounds = RECT{x, chips_top, x + chip_width, chips_bottom};
                item.number_bounds = keycap_box(x + (std::min)(chip_left, chip_width), chips_top, chip_height, i);
                item.number_bounds.right = item.number_bounds.left + keycap_width;
                const int text_left = (std::min)(x + chip_width, static_cast<int>(item.number_bounds.right) + number_gap);
                item.text_bounds = RECT{text_left, chips_top, (std::max)(text_left, x + chip_width - chip_right), chips_bottom};
                item.candidate_text_width_px = text_widths[i];
                item.horizontal_padding_px = chip_left;
                item.number_gap_px = number_gap;
                x += chip_width + gap;
            }
            finish_item(item);
            result.items.push_back(std::move(item));
        }
    } else {
        int y = body_top;
        for (size_t i = slice.begin; i < slice.end; ++i) {
            const size_t row = i - slice.begin;
            const int lines = row_lines[row];
            const int row_height = row_pad + line * lines;
            const int row_bottom = (std::min)(y + row_height, bottom);
            CandidateItemLayout item;
            item.candidate_index = i;
            item.page_number = row + 1;
            item.selected = i == slice.selection;
            item.featured = i == slice.focus;
            item.display_text = input.candidate_texts[i];
            item.number_text_width_px = number_widths[i];
            item.candidate_text_width_px = text_widths[i];
            const int lead = item.featured ? focus_left : chip_left;
            item.horizontal_padding_px = lead;
            item.number_gap_px = focus_number_gap;
            item.bounds = RECT{left, y, right, row_bottom};
            // The keycap sits level with the first line.
            item.number_bounds = keycap_box(left + lead, y + row_pad / 2, line, i);
            const int text_left = item.number_bounds.right + focus_number_gap;
            if (item.featured) {
                result.focus_rect = item.bounds;
                if (result.show_hint) {
                    const int hint_top = y + row_pad / 2 + (line - hint_height) / 2;
                    result.hint_rect = RECT{right - focus_right - hint_width, hint_top, right - focus_right, hint_top + hint_height};
                }
            }
            const int text_right = item.featured && result.show_hint ? result.hint_rect.left - f.dip(12) : right - focus_right;
            item.text_bounds = RECT{text_left, y + row_pad / 2, (std::max)(text_left, text_right), row_bottom - row_pad / 2};
            finish_item(item);
            // Two lines hold twice the text before it is cut.
            item.truncated = item.candidate_text_width_px > item.text_available_width_px * lines;
            result.items.push_back(std::move(item));
            y = row_bottom + expanded_row_gap;
        }
    }
    place_near_caret(result, input, result.composition_rect.left);
    return result;
}

void paint_adaptive_flow(const CandidatePaintContext& context) {
    auto& canvas = context.canvas;
    const auto& layout = context.layout;
    const auto& palette = context.palette;
    const auto& state = context.state;
    const float u = candidate_paint::unit(layout);
    const RECT& panel = layout.panel_rect;
    const float radius = static_cast<float>(layout.corner_radius);
    // High contrast: system colors, a plain drop, no sheen or glow.
    const bool plain = palette.high_contrast;

    canvas.rounded(panel, radius, palette.background, palette.background);
    if (palette.highlight != palette.background && !plain) {
        // Raised edge: a lighter line just inside the top border.
        const float y = panel.top + 1.5f;
        canvas.line(panel.left + radius * 0.7f, y, panel.right - radius * 0.7f, y, palette.highlight, 1.0f);
    }
    canvas.outline(panel, radius, palette.border, 1.0f);
    RECT inner = panel;
    InflateRect(&inner, -1, -1);
    canvas.outline(inner, (std::max)(1.0f, radius - 1.0f), palette.inner_edge, 1.0f);

    if (!candidate_paint::is_empty(layout.mark_rect)) drop_mark(canvas, layout.mark_rect, palette);
    candidate_paint::query_text(context, layout.composition_rect, static_cast<float>(layout.composition_font_px),
                                FW_NORMAL, palette.composition_text, palette.separator_text, palette.caret);
    candidate_paint::page_controls(context, 6.0f * u, palette.caret);
    candidate_paint::settings_gear(context, 6.0f * u);

    // Where the drop is this frame: at rest on its target, partway through a
    // move, or pouring in on a popup that has just appeared.
    const RECT target = candidate_paint::is_empty(state.marker_to) ? candidate_marker_target(layout, state.hovered)
                                                                    : state.marker_to;
    RECT from = state.marker_from;
    float t = state.marker_progress;
    if (t >= 1.0f && !plain && state.previous_texts.empty() && state.reveal_ms < kRevealAtRestMs) {
        const float pour = static_cast<float>(state.reveal_ms) / kPourMs;
        if (pour < 1.0f) {
            from = RECT{target.left, target.top, target.left + (target.bottom - target.top), target.bottom};
            t = pour;
        }
    }
    const bool moving = t < 1.0f && !candidate_paint::is_empty(from) && !EqualRect(&from, &target);
    DropEdges drop = moving ? drop_edges(from, target, t) : rest_edges(target);
    float pinch = 0.0f;
    bool vertical = false;
    if (moving) {
        // Stretched past both ends' own length, the middle draws in.
        const float travel_x = std::abs((target.left + target.right - from.left - from.right) / 2.0f);
        const float travel_y = std::abs((target.top + target.bottom - from.top - from.bottom) / 2.0f);
        vertical = travel_y > travel_x;
        const float extent = vertical ? drop.bottom - drop.top : drop.right - drop.left;
        const float at_rest = static_cast<float>(vertical
            ? (std::max)(from.bottom - from.top, target.bottom - target.top)
            : (std::max)(from.right - from.left, target.right - target.left));
        const float thickness = static_cast<float>((std::max)(LONG{1}, (std::min)(target.right - target.left,
                                                                                  target.bottom - target.top)));
        pinch = thickness * 0.22f * std::clamp((extent - at_rest) / (thickness * 1.4f), 0.0f, 1.0f);
    }
    // Pressed, the drop gives a little.
    if (state.pressed) {
        const auto* pressed = candidate_paint::item_for(layout, *state.pressed);
        if (pressed) {
            const RECT rest = drop_rest(layout, *pressed);
            if (EqualRect(&rest, &target)) {
                drop.left += 1.5f * u; drop.right -= 1.5f * u; drop.top += u; drop.bottom -= u;
            }
        }
    }
    const float drop_radius = (std::min)(static_cast<float>(layout.selected_corner_radius),
                                         static_cast<float>(target.bottom - target.top) / 2.0f);
    const bool has_drop = !candidate_paint::is_empty(target);
    const auto outline = has_drop ? drop_outline(drop, drop_radius, pinch, vertical) : std::vector<D2D1_POINT_2F>{};

    // Candidates that changed rise into place, one after another.
    struct Look { float alpha = 1.0f; float rise = 0.0f; };
    std::vector<Look> looks(layout.items.size());
    if (state.reveal_ms < kRevealAtRestMs && !plain) {
        for (size_t k = 0; k < layout.items.size(); ++k) {
            const auto& item = layout.items[k];
            const size_t slot = item.page_number - 1;
            if (slot < state.previous_texts.size() && state.previous_texts[slot] == item.display_text) continue;
            const float p = std::clamp((static_cast<float>(state.reveal_ms) - static_cast<float>(k) * kRiseStaggerMs) / kRiseMs,
                                       0.0f, 1.0f);
            const float eased = 1.0f - (1.0f - p) * (1.0f - p) * (1.0f - p);
            looks[k] = Look{eased, (1.0f - eased) * 6.0f * u};
        }
    }
    const float number_px = static_cast<float>(layout.number_font_px);
    const bool expanded = layout.flow_form == CandidateFlowForm::Expanded;
    const auto draw_items = [&](bool on_drop) {
        for (size_t k = 0; k < layout.items.size(); ++k) {
            const auto& item = layout.items[k];
            const Look& look = looks[k];
            if (look.alpha <= 0.01f) continue;
            const LONG rise = static_cast<LONG>(std::lround(look.rise));
            RECT number = item.number_bounds;
            RECT text = item.text_bounds;
            OffsetRect(&number, 0, rise);
            OffsetRect(&text, 0, rise);
            const bool faded = look.alpha < 0.999f;
            if (faded) canvas.push_layer({}, look.alpha);
            canvas.text(std::to_wstring(item.page_number), number, on_drop ? palette.focus_number_text : palette.number_text,
                        number_px, FW_SEMIBOLD, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
            // Two-line rows in Expanded wrap; everything else is one line.
            const bool wraps = expanded && item.text_bounds.bottom - item.text_bounds.top >
                                           static_cast<LONG>(layout.candidate_font_px * kLineEm * 1.5f);
            const UINT flags = wraps ? (DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX) : candidate_text_draw_flags(item.truncated);
            const bool large = item.featured && layout.flow_form == CandidateFlowForm::Balanced;
            canvas.text(item.display_text, text, on_drop ? palette.focus_text : palette.text,
                        static_cast<float>(large ? layout.focus_font_px : layout.candidate_font_px), FW_NORMAL, flags);
            if (faded) canvas.pop_layer();
        }
    };

    // Under the text: where the current choice is while the drop is away
    // (a faint trace), and the pointer over a candidate the drop leaves be.
    const float chip_radius = static_cast<float>(layout.chip_corner_radius);
    for (const auto& item : layout.items) {
        const RECT rest = drop_rest(layout, item);
        if (EqualRect(&rest, &target)) continue;
        const bool hovered = state.hovered == item.candidate_index;
        const bool pressed = state.pressed == item.candidate_index;
        if (pressed || hovered) {
            canvas.rounded_fill(rest, chip_radius, pressed ? palette.pressed_background : palette.hover_background, 1.0f);
        } else if (item.featured && !plain) {
            canvas.rounded_fill(rest, chip_radius, palette.focus_border, 1.0f);
        }
    }
    draw_items(false);

    if (has_drop) {
        if (plain) {
            canvas.polygon(outline, palette.focus_background);
        } else {
            // A glow of the drop's own color beneath it, in its own shape.
            if (palette.focus_shadow_alpha > 0) {
                const float strength = static_cast<float>(palette.focus_shadow_alpha) / 255.0f;
                const struct { float spread, fall, opacity; } glow[] = {{0.0f, 2.0f, 0.5f}, {2.0f, 4.0f, 0.24f}, {4.5f, 6.5f, 0.1f}};
                for (const auto& layer : glow) {
                    const float spread = layer.spread * u, fall = layer.fall * u;
                    const DropEdges halo{drop.left - spread, drop.top - spread + fall, drop.right + spread, drop.bottom + spread + fall};
                    canvas.polygon(drop_outline(halo, drop_radius + spread, pinch, vertical), palette.focus_background,
                                   strength * layer.opacity);
                }
            }
            canvas.polygon_gradient(outline, palette.focus_top, palette.focus_background, drop.top, drop.bottom);
        }
        canvas.push_layer(outline);
        if (!plain) {
            // The surface: a sheen over the upper half whose lower edge is a
            // slow wave, sloshing while the drop moves, and a bright line
            // just under the top.
            const float h = drop.bottom - drop.top;
            const float slosh = moving ? std::sin(kPi * t) : 0.0f;
            const float amplitude = h * (0.025f + 0.09f * slosh);
            const float wavelength = (std::max)(h * 2.4f, 36.0f * u);
            const float phase = moving ? 2.0f * kPi * 1.3f * t : 0.0f;
            const float level = drop.top + h * 0.46f;
            std::vector<D2D1_POINT_2F> sheen{D2D1::Point2F(drop.left - 1.0f, drop.top - 1.0f),
                                             D2D1::Point2F(drop.right + 1.0f, drop.top - 1.0f)};
            for (int k = 24; k >= 0; --k) {
                const float x = drop.left - 1.0f + (drop.right - drop.left + 2.0f) * static_cast<float>(k) / 24.0f;
                sheen.push_back(D2D1::Point2F(x, level + amplitude * std::sin(2.0f * kPi * (x - drop.left) / wavelength + phase)));
            }
            canvas.polygon(sheen, RGB(255, 255, 255), palette.dark ? 0.12f : 0.17f);
            const LONG line_top = static_cast<LONG>(std::lround(drop.top + u));
            canvas.rounded_fill(RECT{static_cast<LONG>(std::floor(drop.left)), line_top, static_cast<LONG>(std::ceil(drop.right)),
                                     line_top + (std::max)(LONG{1}, static_cast<LONG>(std::lround(u)))},
                                0.0f, RGB(255, 255, 255), palette.dark ? 0.22f : 0.32f);
        }
        draw_items(true);
        canvas.pop_layer();
    }

    if (layout.show_hint) {
        candidate_paint::keycap(canvas, layout.hint_rect, state.focus_hint, palette.keycap, palette.hint_text,
                                static_cast<float>(layout.hint_font_px), (std::max)(3.0f, 4.0f * u));
    }
}

}  // namespace candidate_styles
}  // namespace localpinyin
