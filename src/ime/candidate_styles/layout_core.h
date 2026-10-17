#pragma once

// Shared core of the candidate styles: measurement, paging and placement.
// Every style lays out the same input state with these helpers, so paging,
// keyboard mapping (digits are page slots) and caret anchoring behave the
// same whatever the window looks like.

#include "../candidate_ui.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace localpinyin::candidate_layout {

inline constexpr size_t kMaxCandidates = 9;
inline constexpr int kShadowMarginDip = 16;

int rect_width(const RECT& rect) noexcept;
int rect_height(const RECT& rect) noexcept;
int clamp_int(int value, int low, int high) noexcept;

// One layout pass over a validated input.
class Frame {
public:
    // Empty when the window must stay hidden (no candidates, no work area).
    static std::optional<Frame> begin(const CandidateLayoutInput& input);

    [[nodiscard]] const CandidateLayoutInput& input() const noexcept { return *input_; }
    [[nodiscard]] int dpi() const noexcept { return dpi_; }
    [[nodiscard]] size_t count() const noexcept { return count_; }
    [[nodiscard]] int work_width() const noexcept { return work_width_; }
    [[nodiscard]] int work_height() const noexcept { return work_height_; }
    [[nodiscard]] int dip(int value) const noexcept { return scale_dip(value, dpi_); }
    // A nominal DIP measure that shrinks on a work area narrower than `parts`
    // times it, so every row still fits a small monitor.
    [[nodiscard]] int fit(int dip_value, int parts) const noexcept;
    [[nodiscard]] int candidate_font_px() const noexcept { return candidate_font_px_; }
    [[nodiscard]] const std::wstring& text(size_t index) const { return input_->candidate_texts[index]; }
    // Width of `text` as drawn; the host measurer when present, otherwise the
    // host's precomputed widths or an estimate.
    [[nodiscard]] int measure(const std::wstring& text, int font_px, int weight = 400,
                              CandidateFontFace face = CandidateFontFace::Ui) const;
    [[nodiscard]] int candidate_width(size_t index, int font_px, int weight = 400) const;
    // Slot digits restart at 1 on every page.
    [[nodiscard]] int digit_width(size_t index, int font_px, CandidateFontFace face = CandidateFontFace::Ui,
                                  int weight = 400) const;
    [[nodiscard]] int composition_width(int font_px) const;

private:
    const CandidateLayoutInput* input_ = nullptr;
    int dpi_ = 96;
    size_t count_ = 0;
    int work_width_ = 0;
    int work_height_ = 0;
    int candidate_font_px_ = 0;
};

// Greedy pages: from each page's first item, add items while the row still
// fits `available` with `gap` between items. `width(index, first)` sizes an
// item, `first` being the page's leading slot (the current choice). With
// `lead_on_own_row` the leading item is not on the row being filled. Every
// page holds at least one item and at most `max_per_page`.
std::vector<UINT> paginate(size_t count, int available, int gap,
                           const std::function<int(size_t index, bool first)>& width,
                           bool lead_on_own_row = false, size_t max_per_page = kMaxCandidates);

// The current page and its slots.
struct PageSlice {
    size_t begin = 0;
    size_t end = 0;
    size_t selection = 0;
    // The selected item when it is on this page, otherwise the page's first.
    size_t focus = 0;
};

// Applies `computed` pages, or the host's own valid boundaries, to `result`
// (page_indices, paging_visible, current page) and returns the shown slice.
PageSlice apply_pages(CandidateLayoutResult& result, const CandidateLayoutInput& input,
                      std::vector<UINT> computed, size_t count);

// The panel inside its shadow margin.
void set_panel(CandidateLayoutResult& result, int margin, int panel_width, int panel_height);

// Places the panel against the caret: client x `anchor_x` (where the pinyin
// starts) sits under the caret, below it when there is room, otherwise above.
void place_near_caret(CandidateLayoutResult& result, const CandidateLayoutInput& input, int anchor_x);

struct ControlMetrics {
    int button = 0;
    int indicator = 0;
    int gap = 0;
};
[[nodiscard]] int controls_width(bool paging, bool settings, const ControlMetrics& metrics) noexcept;
// "‹ page ›" then the settings gear, right-aligned at `right` and centered
// on `center_y`. Returns the controls' left edge (`right` when none).
int place_controls(CandidateLayoutResult& result, const CandidateLayoutInput& input, int right, int center_y,
                   const ControlMetrics& metrics);

// Fills text_available_width_px and truncated from the item's text bounds.
void finish_item(CandidateItemLayout& item) noexcept;

// When forced host pages do not fit, items share the row evenly.
[[nodiscard]] int squeezed_width(int available, int gap, size_t items) noexcept;

}  // namespace localpinyin::candidate_layout
