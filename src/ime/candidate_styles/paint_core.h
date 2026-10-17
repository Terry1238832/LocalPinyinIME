#pragma once

// Painting shared by the candidate styles. The window gathers pointer state
// and the animated focus positions into CandidatePaintState; a style's
// painter only draws from the layout, palette and that state.

#include "../candidate_ui.h"
#include "../../settings/modern_renderer.h"

#include <optional>
#include <string>

namespace localpinyin {

// A reveal this old has finished in every style.
inline constexpr ULONGLONG kRevealAtRestMs = 60000;
// Long enough for the slowest reveal (a full row of turning tiles).
inline constexpr ULONGLONG kRevealSpanMs = 900;

struct CandidatePaintState {
    std::optional<size_t> hovered;
    std::optional<size_t> pressed;
    int hot_page_direction = 0;
    int pressed_page_direction = 0;
    bool settings_hot = false;
    bool settings_pressed = false;
    // The pinyin fitted to composition_rect, or "联想" for suggestions.
    std::wstring query;
    // Suggestions follow a commit: nothing is being typed, so no caret.
    bool show_caret = true;
    // The key that commits the current choice ("空格", or "Tab" for suggestions).
    std::wstring focus_hint;
    // Where the style's moving focus is drawn this frame (animated by the
    // window from one candidate_marker_target to the next).
    RECT marker{};
    // Cursor Rail's page segment, animated the same way.
    RECT track_marker{};
    // How long ago the visible candidates changed, and what each slot showed
    // before, for styles that animate new content (Split-Flap turns only the
    // tiles whose character changed). At rest unless the window is animating.
    ULONGLONG reveal_ms = kRevealAtRestMs;
    std::vector<std::wstring> previous_texts;
};

struct CandidatePaintContext {
    ModernRenderer& canvas;
    const CandidateLayoutResult& layout;
    const CandidateThemePalette& palette;
    const CandidatePaintState& state;
};

// Draws the panel and its contents in the layout's style.
void paint_candidate_window(const CandidatePaintContext& context);
// Where the style's moving focus rests for this layout and pointer: the
// hovered candidate while the pointer is over one, otherwise the selected
// one. Empty for styles without a moving focus.
RECT candidate_marker_target(const CandidateLayoutResult& layout, std::optional<size_t> hovered);
// Cursor Rail's current page segment on the track; empty otherwise.
RECT candidate_track_target(const CandidateLayoutResult& layout);
// Whether the style animates new content (the window then runs its clock).
bool candidate_style_reveals(CandidateStyle style) noexcept;

namespace candidate_paint {

[[nodiscard]] float unit(const CandidateLayoutResult& layout) noexcept;
[[nodiscard]] const CandidateItemLayout* selected_item(const CandidateLayoutResult& layout) noexcept;
[[nodiscard]] const CandidateItemLayout* item_for(const CandidateLayoutResult& layout, size_t candidate_index) noexcept;
[[nodiscard]] bool is_empty(const RECT& rect) noexcept;

// Pinyin syllables, lighter apostrophes and the caret where typing continues.
void query_text(const CandidatePaintContext& context, const RECT& label, float size, int weight,
                COLORREF base, COLORREF separator, COLORREF caret);
// "‹ page ›": chevrons with up to three page dots (the current one a short
// accent bar) or "2/5" between them.
void page_controls(const CandidatePaintContext& context, float radius, COLORREF accent);
void settings_gear(const CandidatePaintContext& context, float radius);
void keycap(ModernRenderer& canvas, const RECT& rect, const std::wstring& label, COLORREF fill, COLORREF ink,
            float size, float radius, std::wstring_view family = {});
// A soft shadow under a raised element, `alpha` at its densest.
void soft_shadow(ModernRenderer& canvas, const RECT& rect, float radius, int alpha, float unit);

}  // namespace candidate_paint

namespace candidate_styles {

CandidateLayoutResult layout_adaptive_flow(const CandidateLayoutInput& input);
CandidateLayoutResult layout_cursor_rail(const CandidateLayoutInput& input);
CandidateLayoutResult layout_modern_type(const CandidateLayoutInput& input);
CandidateLayoutResult layout_floating_glass(const CandidateLayoutInput& input);
CandidateLayoutResult layout_capsule(const CandidateLayoutInput& input);
CandidateLayoutResult layout_column_list(const CandidateLayoutInput& input);
CandidateLayoutResult layout_split_flap(const CandidateLayoutInput& input);
CandidateLayoutResult layout_highlighter(const CandidateLayoutInput& input);

void paint_adaptive_flow(const CandidatePaintContext& context);
void paint_cursor_rail(const CandidatePaintContext& context);
void paint_modern_type(const CandidatePaintContext& context);
void paint_floating_glass(const CandidatePaintContext& context);
void paint_capsule(const CandidatePaintContext& context);
void paint_column_list(const CandidatePaintContext& context);
void paint_split_flap(const CandidatePaintContext& context);
void paint_highlighter(const CandidatePaintContext& context);

RECT marker_cursor_rail(const CandidateLayoutResult& layout, std::optional<size_t> hovered);
RECT marker_modern_type(const CandidateLayoutResult& layout, std::optional<size_t> hovered);
RECT marker_floating_glass(const CandidateLayoutResult& layout, std::optional<size_t> hovered);
RECT marker_capsule(const CandidateLayoutResult& layout, std::optional<size_t> hovered);
RECT marker_column_list(const CandidateLayoutResult& layout, std::optional<size_t> hovered);
RECT marker_split_flap(const CandidateLayoutResult& layout, std::optional<size_t> hovered);
RECT marker_highlighter(const CandidateLayoutResult& layout, std::optional<size_t> hovered);
RECT track_cursor_rail(const CandidateLayoutResult& layout);

}  // namespace candidate_styles

}  // namespace localpinyin
