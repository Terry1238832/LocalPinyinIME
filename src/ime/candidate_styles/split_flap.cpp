// Split-Flap: a departure board. Every character of a candidate sits on its
// own flap tile, split by a hinge line; all tiles share one width, so the row
// reads as a board. The current choice is a row of amber tiles. New content
// turns over tile by tile from left to right, and only where the character
// changed, as on a real board: typing one more letter turns the new tiles.
#include "layout_core.h"
#include "paint_core.h"

#include <algorithm>
#include <cmath>

namespace localpinyin::candidate_styles {

using namespace candidate_layout;

namespace {

constexpr int kTileWeight = 500;
// Each tile turns over in this long; the next one along starts this much later.
constexpr float kFlipMs = 170.0f;
constexpr float kStaggerMs = 16.0f;

// A candidate's characters, a surrogate pair counted as one.
std::vector<std::wstring> glyphs(const std::wstring& text) {
    std::vector<std::wstring> result;
    for (size_t index = 0; index < text.size(); ++index) {
        if (text[index] >= 0xD800 && text[index] <= 0xDBFF && index + 1 < text.size()) {
            result.push_back(text.substr(index, 2));
            ++index;
        } else {
            result.emplace_back(1, text[index]);
        }
    }
    return result;
}

int tile_gap(int dpi) noexcept { return (std::max)(1, scale_dip(2, dpi)); }

}  // namespace

CandidateLayoutResult layout_split_flap(const CandidateLayoutInput& input) {
    CandidateLayoutResult result;
    result.style = CandidateStyle::SplitFlap;
    result.dpi = (std::max)(1, input.dpi);
    const auto frame = Frame::begin(input);
    if (!frame) return result;
    const Frame& f = *frame;
    const size_t count = f.count();

    const int max_width = (std::min)(f.dip(760), f.work_width());
    const int margin = f.dip(kShadowMarginDip);
    const int side = f.fit(10, 48);
    const int gap = f.fit(10, 64);
    const int number_gap = f.fit(4, 96);
    const ControlMetrics controls{f.fit(22, 12), f.fit(30, 14), f.fit(2, 160)};
    result.composition_font_px = f.dip(12);
    result.candidate_font_px = f.candidate_font_px();
    result.focus_font_px = result.candidate_font_px;
    result.number_font_px = candidate_number_font_px(result.candidate_font_px, result.dpi);
    result.hint_font_px = f.dip(11);
    const int query_height = f.dip(28);
    const int between = tile_gap(result.dpi);
    // One width for every tile, from a full-width character.
    const int tile_width = f.measure(L"国", result.candidate_font_px, kTileWeight) + f.fit(3, 96) * 2;
    const int tile_height = result.candidate_font_px + f.dip(12);
    const int bottom_space = f.dip(9);
    const int inner_max = (std::max)(1, max_width - side * 2);

    std::vector<size_t> tile_counts(count);
    std::vector<int> number_widths(count);
    for (size_t i = 0; i < count; ++i) {
        tile_counts[i] = (std::max)(size_t{1}, glyphs(f.text(i)).size());
        number_widths[i] = f.digit_width(i, result.number_font_px, CandidateFontFace::Ui, FW_SEMIBOLD);
    }
    const auto span = [&](size_t tiles) {
        return static_cast<int>(tiles) * tile_width + static_cast<int>(tiles - 1) * between;
    };
    const auto slot_width = [&](size_t i) { return number_widths[i] + number_gap + span(tile_counts[i]); };
    const auto slice = apply_pages(result, input,
        paginate(count, inner_max, gap, [&](size_t i, bool) { return slot_width(i); }), count);

    int used = 0;
    for (size_t i = slice.begin; i < slice.end; ++i) used += (used ? gap : 0) + slot_width(i);
    const int controls_space = controls_width(result.paging_visible, input.settings_button, controls);
    const int query_needed = f.dip(6) + f.composition_width(result.composition_font_px) + f.dip(18) + controls_space;
    const int inner_width = clamp_int((std::max)(used, query_needed), (std::min)(f.dip(220), inner_max), inner_max);
    const int panel_height = (std::min)(query_height + tile_height + bottom_space, f.work_height());
    set_panel(result, margin, inner_width + side * 2, panel_height);
    result.panel_padding_px = side;
    result.item_gap_px = gap;
    result.corner_radius = f.dip(12);
    result.selected_corner_radius = (std::max)(1, f.dip(3));
    result.chip_corner_radius = result.selected_corner_radius;

    const RECT& panel = result.panel_rect;
    const int left = panel.left + side;
    const int right = panel.right - side;
    const int query_bottom = (std::min)(static_cast<int>(panel.top) + query_height, static_cast<int>(panel.bottom));
    result.query_rect = RECT{panel.left, panel.top, panel.right, query_bottom};
    const int controls_left = place_controls(result, input, right - f.dip(2), panel.top + query_height / 2 + f.dip(1), controls);
    const int label_left = left + f.dip(2);
    result.composition_rect = RECT{label_left, panel.top + f.dip(2), (std::max)(label_left, controls_left - f.dip(10)),
                                   query_bottom};
    const int tile_top = query_bottom;
    const int tile_bottom = (std::min)(tile_top + tile_height, static_cast<int>(panel.bottom));
    result.candidate_area_rect = RECT{left, tile_top, right, tile_bottom};
    result.focus_rect = result.candidate_area_rect;
    const int item_top = (std::max)(static_cast<int>(panel.top), tile_top - f.dip(2));
    const int item_bottom = (std::min)(static_cast<int>(panel.bottom), tile_bottom + f.dip(2));

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
        // The full board width of the candidate; fewer tiles show when it is cut.
        item.candidate_text_width_px = span(tile_counts[i]);
        // The painter reads the tile width from here.
        item.horizontal_padding_px = tile_width;
        item.number_gap_px = number_gap;
        const int width = fits ? slot_width(i) : squeezed_width(inner_width, gap, shown);
        const int tiles_room = (std::max)(0, width - number_widths[i] - number_gap);
        size_t tiles = tile_counts[i];
        while (tiles > 1 && span(tiles) > tiles_room) --tiles;
        item.bounds = RECT{x, item_top, x + width, (std::max)(item_top + 1, item_bottom)};
        const int number_right = (std::min)(x + number_widths[i], x + width);
        item.number_bounds = RECT{x, tile_top, number_right, tile_bottom};
        const int tiles_left = (std::min)(number_right + number_gap, x + width);
        item.text_bounds = RECT{tiles_left, tile_top, (std::min)(tiles_left + span(tiles), x + width), tile_bottom};
        finish_item(item);
        result.items.push_back(std::move(item));
        x += width + gap;
    }
    place_near_caret(result, input, result.composition_rect.left);
    return result;
}

RECT marker_split_flap(const CandidateLayoutResult&, std::optional<size_t>) {
    // The amber tiles stay with the current choice; nothing slides.
    return RECT{};
}

void paint_split_flap(const CandidatePaintContext& context) {
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

    const float radius = static_cast<float>(layout.selected_corner_radius);
    const float size = static_cast<float>(layout.candidate_font_px);
    const int between = tile_gap(layout.dpi);
    const bool turning = state.reveal_ms < kRevealAtRestMs;
    size_t order = 0;  // the tile's place along the row, for the left-to-right wave
    for (const auto& item : layout.items) {
        const bool focused = item.featured;
        const bool hovered = state.hovered == item.candidate_index;
        const bool pressed = state.pressed == item.candidate_index;
        const COLORREF upper_fill = focused ? palette.focus_top : pressed ? palette.pressed_background
                                  : hovered ? palette.hover_background : palette.surface_top;
        const COLORREF lower_fill = focused ? palette.focus_background : (hovered || pressed) ? palette.pressed_background
                                  : palette.inner_edge;
        const COLORREF hinge = focused ? palette.focus_border : palette.divider;
        const COLORREF ink = focused ? palette.focus_text : palette.text;

        canvas.text(std::to_wstring(item.page_number), item.number_bounds,
                    focused ? palette.focus_number_text : palette.number_text,
                    static_cast<float>(layout.number_font_px), FW_SEMIBOLD,
                    DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);

        const auto characters = glyphs(item.display_text);
        const auto before = item.page_number - 1 < state.previous_texts.size()
                                ? glyphs(state.previous_texts[item.page_number - 1]) : std::vector<std::wstring>{};
        const int room = item.text_bounds.right - item.text_bounds.left;
        int tile_width = item.horizontal_padding_px;
        size_t tiles = tile_width > 0 ? static_cast<size_t>((std::max)(0, room + between) / (tile_width + between)) : 0;
        tiles = (std::min)(tiles, characters.size());
        if (tiles == 0 && room > 0 && !characters.empty()) {
            tiles = 1;  // squeezed by the host: one narrower tile
            tile_width = room;
        }
        int x = item.text_bounds.left;
        for (size_t k = 0; k < tiles; ++k, ++order, x += tile_width + between) {
            const RECT tile{x, item.text_bounds.top, x + tile_width, item.text_bounds.bottom};
            const float mid = std::round((tile.top + tile.bottom) / 2.0f);
            // The lower half first; the upper half's rounded hinge corners
            // leave the small notch a real flap has at its pivot.
            canvas.rounded(tile, radius, lower_fill, lower_fill);
            canvas.rounded(RECT{tile.left, tile.top, tile.right, static_cast<LONG>(mid)}, radius, upper_fill, upper_fill);
            const bool cut = item.truncated && k + 1 == tiles;
            canvas.text(cut ? std::wstring(L"…") : characters[k], tile, ink, size, focused ? FW_SEMIBOLD : kTileWeight,
                        DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);

            // Turning over: the old top flap folds down onto the hinge, then
            // the new bottom flap falls from it. Blank flaps stand in for the
            // old characters; a hairline marks the moving edge.
            const bool changed = k >= before.size() || before[k] != characters[k];
            const float progress = turning && changed
                ? std::clamp((static_cast<float>(state.reveal_ms) - order * kStaggerMs) / kFlipMs, 0.0f, 1.0f) : 1.0f;
            if (progress < 1.0f) {
                if (progress < 0.5f) {
                    const float folded = progress / 0.5f;
                    const float edge = mid - (mid - tile.top) * (1.0f - folded);
                    canvas.rounded(RECT{tile.left, static_cast<LONG>(std::floor(edge)), tile.right, static_cast<LONG>(mid)},
                                   radius * (1.0f - folded), upper_fill, upper_fill);
                    canvas.rounded_fill(RECT{tile.left, static_cast<LONG>(std::floor(edge)), tile.right, static_cast<LONG>(mid)},
                                        0.0f, hinge, 0.35f * folded);
                    canvas.rounded(RECT{tile.left, static_cast<LONG>(mid), tile.right, tile.bottom}, radius, lower_fill, lower_fill);
                    canvas.line(static_cast<float>(tile.left), edge, static_cast<float>(tile.right), edge, hinge, u);
                } else {
                    const float fallen = (progress - 0.5f) / 0.5f;
                    const float edge = mid + (tile.bottom - mid) * fallen;
                    canvas.rounded(RECT{tile.left, static_cast<LONG>(std::ceil(edge)), tile.right, tile.bottom}, radius,
                                   lower_fill, lower_fill);
                    canvas.line(static_cast<float>(tile.left), edge, static_cast<float>(tile.right), edge, hinge, u);
                }
            }
            canvas.line(static_cast<float>(tile.left), mid, static_cast<float>(tile.right), mid, hinge, (std::max)(1.0f, u));
        }
    }
}

}  // namespace localpinyin::candidate_styles
