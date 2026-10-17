#include "layout_core.h"

#include <algorithm>

namespace localpinyin::candidate_layout {

int rect_width(const RECT& rect) noexcept {
    return rect.right - rect.left;
}

int rect_height(const RECT& rect) noexcept {
    return rect.bottom - rect.top;
}

int clamp_int(int value, int low, int high) noexcept {
    if (high < low) return low;
    return (std::min)((std::max)(value, low), high);
}

std::optional<Frame> Frame::begin(const CandidateLayoutInput& input) {
    Frame frame;
    frame.input_ = &input;
    frame.dpi_ = (std::max)(1, input.dpi);
    frame.count_ = (std::min)(input.candidate_texts.size(), kMaxCandidates);
    frame.work_width_ = rect_width(input.work_area);
    frame.work_height_ = rect_height(input.work_area);
    if (!should_show_candidate_window(input.composition_text, frame.count_, false) ||
        frame.work_width_ <= 0 || frame.work_height_ <= 0) return std::nullopt;
    frame.candidate_font_px_ = candidate_font_px_for_size(input.options.text_size, frame.dpi_);
    return frame;
}

int Frame::fit(int dip_value, int parts) const noexcept {
    return (std::max)(1, (std::min)(dip(dip_value), work_width_ / (std::max)(1, parts)));
}

int Frame::measure(const std::wstring& text, int font_px, int weight, CandidateFontFace face) const {
    if (text.empty()) return 0;
    if (input_->measure) {
        const int measured = input_->measure(text, font_px, weight, face);
        if (measured > 0) return measured;
    }
    // Estimates: bold glyphs run wider, light ones slightly narrower.
    const int estimate = estimate_candidate_text_width_px(text, font_px);
    return weight >= 600 ? estimate * 106 / 100 : weight <= 300 ? estimate * 98 / 100 : estimate;
}

int Frame::candidate_width(size_t index, int font_px, int weight) const {
    if (!input_->measure && weight == 400 && font_px == candidate_font_px_ &&
        index < input_->candidate_text_widths_px.size() && input_->candidate_text_widths_px[index] > 0) {
        return input_->candidate_text_widths_px[index];
    }
    return measure(text(index), font_px, weight);
}

int Frame::digit_width(size_t index, int font_px, CandidateFontFace face, int weight) const {
    if (!input_->measure && face == CandidateFontFace::Ui && index < input_->number_text_widths_px.size() &&
        input_->number_text_widths_px[index] > 0) {
        return input_->number_text_widths_px[index];
    }
    return measure(std::to_wstring(index % kMaxCandidates + 1), font_px, weight, face);
}

int Frame::composition_width(int font_px) const {
    if (!input_->measure && input_->composition_text_width_px > 0) return input_->composition_text_width_px;
    return measure(input_->composition_text, font_px);
}

std::vector<UINT> paginate(size_t count, int available, int gap,
                           const std::function<int(size_t index, bool first)>& width,
                           bool lead_on_own_row, size_t max_per_page) {
    std::vector<UINT> pages;
    available = (std::max)(1, available);
    max_per_page = (std::max)(size_t{1}, max_per_page);
    for (size_t start = 0; start < count;) {
        pages.push_back(static_cast<UINT>(start));
        size_t end = start + 1;
        int used = lead_on_own_row ? 0 : (std::min)(available, width(start, true));
        bool row_empty = lead_on_own_row;
        for (; end < count && end - start < max_per_page; ++end) {
            const int item = (std::min)(available, width(end, false));
            const int next = used + (row_empty ? 0 : gap) + item;
            if (next > available) break;
            used = next;
            row_empty = false;
        }
        start = end;
    }
    return pages;
}

PageSlice apply_pages(CandidateLayoutResult& result, const CandidateLayoutInput& input,
                      std::vector<UINT> computed, size_t count) {
    // ITfCandidateListUIElement permits the host to choose page boundaries.
    // Preserve those boundaries in the popup and keyboard mapping as well.
    const auto& host = input.page_indices;
    const bool host_pages = !host.empty() && host.front() == 0 && host.back() < count &&
        std::adjacent_find(host.begin(), host.end(), [](UINT a, UINT b) { return a >= b; }) == host.end();
    result.page_indices = host_pages ? host : std::move(computed);
    if (result.page_indices.empty()) result.page_indices.push_back(0);
    result.paging_visible = result.page_indices.size() > 1;

    PageSlice slice;
    slice.selection = (std::min)(input.selected_index, count - 1);
    const auto next_page = std::upper_bound(result.page_indices.begin(), result.page_indices.end(),
                                            static_cast<UINT>(slice.selection));
    result.current_page = static_cast<size_t>(next_page - result.page_indices.begin() - 1);
    result.has_previous_page = result.current_page > 0;
    result.has_next_page = result.current_page + 1 < result.page_indices.size();
    slice.begin = result.page_indices[result.current_page];
    slice.end = result.has_next_page ? result.page_indices[result.current_page + 1] : count;
    slice.focus = slice.selection >= slice.begin && slice.selection < slice.end ? slice.selection : slice.begin;
    return slice;
}

void set_panel(CandidateLayoutResult& result, int margin, int panel_width, int panel_height) {
    result.visible = true;
    result.shadow_margin_px = margin;
    result.width = panel_width + margin * 2;
    result.height = panel_height + margin * 2;
    result.panel_rect = RECT{margin, margin, margin + panel_width, margin + panel_height};
}

void place_near_caret(CandidateLayoutResult& result, const CandidateLayoutInput& input, int anchor_x) {
    const int margin = result.shadow_margin_px;
    const int panel_width = rect_width(result.panel_rect);
    const int panel_height = rect_height(result.panel_rect);
    const int placement_gap = scale_dip(4, result.dpi);
    const RECT& work = input.work_area;
    const int desired_x = input.caret_rect.left - (anchor_x - margin);
    const int panel_x = clamp_int(desired_x, work.left, work.right - panel_width);
    const int below_y = input.caret_rect.bottom + placement_gap;
    const int above_y = input.caret_rect.top - panel_height - placement_gap;
    result.placement = CandidatePlacement::Below;
    int desired_y = below_y;
    if (below_y + panel_height > work.bottom &&
        (above_y >= work.top || above_y - work.top >= work.bottom - below_y)) {
        result.placement = CandidatePlacement::Above;
        desired_y = above_y;
    }
    const int panel_y = clamp_int(desired_y, work.top, work.bottom - panel_height);
    result.rect_clamped = panel_x != desired_x || panel_y != desired_y;
    result.panel_screen_rect = RECT{panel_x, panel_y, panel_x + panel_width, panel_y + panel_height};
    result.window_rect = RECT{panel_x - margin, panel_y - margin,
                              panel_x + panel_width + margin, panel_y + panel_height + margin};
}

int controls_width(bool paging, bool settings, const ControlMetrics& metrics) noexcept {
    return (paging ? metrics.button * 2 + metrics.indicator : 0) + (settings ? metrics.button : 0) +
           (paging && settings ? metrics.gap : 0);
}

int place_controls(CandidateLayoutResult& result, const CandidateLayoutInput& input, int right, int center_y,
                   const ControlMetrics& metrics) {
    const int top = center_y - metrics.button / 2;
    const int bottom = top + metrics.button;
    result.reserves_action_icon_space = input.settings_button;
    result.action_button_visible = input.settings_button;
    result.action_button_size_px = input.settings_button ? metrics.button : 0;
    result.action_button_gap_px = input.settings_button ? metrics.gap : 0;
    int edge = right;
    if (input.settings_button) {
        result.action_button_rect = RECT{edge - metrics.button, top, edge, bottom};
        edge -= metrics.button + (result.paging_visible ? metrics.gap : 0);
    }
    if (result.paging_visible) {
        result.next_page_rect = RECT{edge - metrics.button, top, edge, bottom};
        result.page_indicator_rect = RECT{edge - metrics.button - metrics.indicator, top, edge - metrics.button, bottom};
        result.previous_page_rect = RECT{edge - metrics.button * 2 - metrics.indicator, top,
                                         edge - metrics.button - metrics.indicator, bottom};
        edge = result.previous_page_rect.left;
    }
    return edge;
}

void finish_item(CandidateItemLayout& item) noexcept {
    item.text_available_width_px = (std::max)(0, static_cast<int>(item.text_bounds.right - item.text_bounds.left));
    item.truncated = item.candidate_text_width_px > item.text_available_width_px;
}

int squeezed_width(int available, int gap, size_t items) noexcept {
    const int n = static_cast<int>((std::max)(size_t{1}, items));
    return (std::max)(0, (available - gap * (n - 1)) / n);
}

}  // namespace localpinyin::candidate_layout
