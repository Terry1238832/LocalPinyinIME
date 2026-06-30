#include "candidate_ui.h"

#include <algorithm>
#include <cwctype>
#include <sstream>
#include <unordered_set>

namespace localpinyin {
namespace {

constexpr int kMaxCandidates = 9;

int rect_width(const RECT& rect) noexcept {
    return std::max<LONG>(0, rect.right - rect.left);
}

int rect_height(const RECT& rect) noexcept {
    return std::max<LONG>(0, rect.bottom - rect.top);
}

int clamp_int(int value, int low, int high) noexcept {
    if (high < low) {
        return low;
    }
    return std::max(low, std::min(value, high));
}

std::wstring lower_ascii(std::wstring value) {
    for (auto& ch : value) {
        if (ch >= L'A' && ch <= L'Z') {
            ch = static_cast<wchar_t>(ch - L'A' + L'a');
        }
    }
    return value;
}

bool is_cjk_or_wide(wchar_t ch) noexcept {
    return ch >= 0x2E80;
}

const wchar_t* bool_text(bool value) noexcept {
    return value ? L"true" : L"false";
}

std::vector<std::wstring> unique_candidates(const std::vector<std::wstring>& candidates) {
    std::vector<std::wstring> result;
    std::unordered_set<std::wstring> seen;
    for (const auto& candidate : candidates) {
        if (candidate.empty()) {
            continue;
        }
        if (!seen.insert(candidate).second) {
            continue;
        }
        result.push_back(candidate);
        if (result.size() >= kMaxCandidates) {
            break;
        }
    }
    return result;
}

int width_from_input_or_estimate(const std::vector<int>& widths,
                                 size_t index,
                                 const std::wstring& text,
                                 int font_px) {
    if (index < widths.size() && widths[index] > 0) {
        return widths[index];
    }
    return estimate_candidate_text_width_px(text, font_px);
}

}  // namespace

int scale_dip(int value, int dpi) noexcept {
    return MulDiv(value, std::max(1, dpi), 96);
}

int candidate_font_px_for_size(CandidateTextSize size, int dpi) noexcept {
    switch (size) {
        case CandidateTextSize::Small:
            return scale_dip(14, dpi);
        case CandidateTextSize::Large:
            return scale_dip(18, dpi);
        case CandidateTextSize::Standard:
        default:
            return scale_dip(16, dpi);
    }
}

int estimate_candidate_text_width_px(const std::wstring& text, int font_px) {
    int width = 0;
    for (const wchar_t ch : text) {
        if (ch == L' ') {
            width += std::max(4, font_px / 3);
        } else if (is_cjk_or_wide(ch)) {
            width += font_px;
        } else if (std::iswdigit(ch)) {
            width += std::max(6, font_px * 3 / 5);
        } else {
            width += std::max(5, font_px * 11 / 20);
        }
    }
    return width;
}

UINT candidate_text_draw_flags(bool truncated) noexcept {
    UINT flags = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX;
    if (truncated) {
        flags |= DT_END_ELLIPSIS;
    }
    return flags;
}

bool should_show_candidate_window(const std::wstring& composition_text,
                                  size_t candidate_count,
                                  bool english_mode) noexcept {
    const CandidateDisplayDecision decision = decide_candidate_display(CandidateDisplayState{
        english_mode,
        !composition_text.empty(),
        candidate_count,
        true
    });
    return decision.request_show;
}

CandidateDisplayDecision decide_candidate_display(const CandidateDisplayState& state) noexcept {
    if (state.english_mode) {
        return CandidateDisplayDecision{false, CandidateHideReason::EnglishMode};
    }
    if (!state.composition_active) {
        return CandidateDisplayDecision{false, CandidateHideReason::EmptyComposition};
    }
    if (state.candidate_count == 0) {
        return CandidateDisplayDecision{false, CandidateHideReason::EmptyCandidates};
    }
    if (!state.caret_rect_available) {
        return CandidateDisplayDecision{false, CandidateHideReason::NoCaretRect};
    }
    return CandidateDisplayDecision{true, CandidateHideReason::None};
}

std::wstring candidate_hide_reason_to_string(CandidateHideReason reason) {
    switch (reason) {
        case CandidateHideReason::EnglishMode:
            return L"english_mode";
        case CandidateHideReason::EmptyComposition:
            return L"empty_composition";
        case CandidateHideReason::EmptyCandidates:
            return L"empty_candidates";
        case CandidateHideReason::NoCaretRect:
            return L"no_caret_rect";
        case CandidateHideReason::None:
        default:
            return L"none";
    }
}

CandidateWindowCreateConfig make_candidate_window_create_config(HWND requested_owner,
                                                               bool requested_owner_is_window) noexcept {
    CandidateWindowCreateConfig config;
    config.ex_style = WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED;
    config.style = WS_POPUP;
    config.x = CW_USEDEFAULT;
    config.y = CW_USEDEFAULT;
    config.width = 280;
    config.height = 80;
    config.menu = nullptr;
    config.use_topmost_after_create = true;

    if (!requested_owner) {
        config.parent = nullptr;
        config.parent_kind = CandidateCreateParentKind::Null;
        config.parent_is_window = false;
        return config;
    }

    config.parent_is_window = requested_owner_is_window;
    if (requested_owner_is_window && requested_owner != HWND_TOPMOST && requested_owner != HWND_MESSAGE) {
        config.parent = requested_owner;
        config.parent_kind = CandidateCreateParentKind::ValidOwner;
        return config;
    }

    config.parent = nullptr;
    config.parent_kind = CandidateCreateParentKind::RejectedInvalidOwner;
    return config;
}

std::wstring candidate_create_parent_kind_to_string(CandidateCreateParentKind kind) {
    switch (kind) {
        case CandidateCreateParentKind::ValidOwner:
            return L"valid_owner";
        case CandidateCreateParentKind::RejectedInvalidOwner:
            return L"rejected_invalid_owner";
        case CandidateCreateParentKind::Null:
        default:
            return L"null";
    }
}

std::wstring make_candidate_window_create_attempt_log(const CandidateWindowCreateConfig& config) {
    std::wstringstream stream;
    stream << L"window_create_attempt=true"
           << L" create_parent_kind=" << candidate_create_parent_kind_to_string(config.parent_kind)
           << L" create_parent_is_window=" << bool_text(config.parent_is_window)
           << L" create_menu_is_null=" << bool_text(config.menu == nullptr)
           << L" create_style_popup=" << bool_text((config.style & WS_POPUP) == WS_POPUP)
           << L" create_ex_noactivate=" << bool_text((config.ex_style & WS_EX_NOACTIVATE) == WS_EX_NOACTIVATE)
           << L" create_ex_toolwindow=" << bool_text((config.ex_style & WS_EX_TOOLWINDOW) == WS_EX_TOOLWINDOW);
    return stream.str();
}

std::wstring make_candidate_window_create_result_log(bool created, DWORD last_error) {
    std::wstringstream stream;
    stream << L"window_created=" << bool_text(created)
           << L" create_last_error=" << last_error;
    return stream.str();
}

CandidateWindowShowStatus make_candidate_window_show_status(bool set_window_pos_ok) noexcept {
    return CandidateWindowShowStatus{set_window_pos_ok, set_window_pos_ok, set_window_pos_ok};
}

CandidateThemeMode parse_candidate_theme_mode(const std::wstring& value) noexcept {
    const std::wstring lowered = lower_ascii(value);
    if (lowered == L"light") {
        return CandidateThemeMode::Light;
    }
    if (lowered == L"dark") {
        return CandidateThemeMode::Dark;
    }
    return CandidateThemeMode::System;
}

CandidateTextSize parse_candidate_text_size(const std::wstring& value) noexcept {
    const std::wstring lowered = lower_ascii(value);
    if (lowered == L"small") {
        return CandidateTextSize::Small;
    }
    if (lowered == L"large") {
        return CandidateTextSize::Large;
    }
    return CandidateTextSize::Standard;
}

std::wstring candidate_theme_mode_to_string(CandidateThemeMode mode) {
    switch (mode) {
        case CandidateThemeMode::Light:
            return L"light";
        case CandidateThemeMode::Dark:
            return L"dark";
        case CandidateThemeMode::System:
        default:
            return L"system";
    }
}

std::wstring candidate_text_size_to_string(CandidateTextSize size) {
    switch (size) {
        case CandidateTextSize::Small:
            return L"small";
        case CandidateTextSize::Large:
            return L"large";
        case CandidateTextSize::Standard:
        default:
            return L"standard";
    }
}

CandidateThemePalette make_candidate_palette(CandidateThemeMode mode, bool system_dark) {
    const bool dark = mode == CandidateThemeMode::Dark ||
                      (mode == CandidateThemeMode::System && system_dark);
    if (dark) {
        return CandidateThemePalette{
            true,
            RGB(34, 34, 34),
            RGB(78, 78, 78),
            RGB(242, 242, 242),
            RGB(174, 174, 174),
            RGB(133, 186, 255),
            RGB(50, 88, 132),
            RGB(95, 150, 220)
        };
    }
    return CandidateThemePalette{};
}

CandidateLayoutResult make_candidate_layout(const CandidateLayoutInput& input) {
    CandidateLayoutResult result;
    result.dpi = std::max(1, input.dpi);

    const auto candidates = unique_candidates(input.candidate_texts);
    if (!should_show_candidate_window(input.composition_text, candidates.size(), false)) {
        return result;
    }

    const int work_width = rect_width(input.work_area);
    const int work_height = rect_height(input.work_area);
    if (work_width <= 0 || work_height <= 0) {
        return result;
    }

    const int dpi = result.dpi;
    const int padding = scale_dip(12, dpi);
    const int row_gap = scale_dip(6, dpi);
    const int item_gap = scale_dip(8, dpi);
    const int item_pad_x = scale_dip(8, dpi);
    const int item_pad_y = scale_dip(5, dpi);
    const int number_gap = scale_dip(5, dpi);
    const int placement_gap = scale_dip(5, dpi);
    result.corner_radius = scale_dip(8, dpi);
    result.candidate_font_px = candidate_font_px_for_size(input.options.text_size, dpi);
    result.hint_font_px = scale_dip(12, dpi);
    const int text_slack = std::max(scale_dip(10, dpi), result.candidate_font_px / 2);

    const int composition_height = result.candidate_font_px + scale_dip(4, dpi);
    const int candidate_row_height = result.candidate_font_px + item_pad_y * 2;
    const int hint_height = input.options.show_key_hints ? result.hint_font_px + scale_dip(4, dpi) : 0;
    const int min_width = std::min(scale_dip(240, dpi), work_width);
    const int max_width = std::max(min_width, std::min(scale_dip(760, dpi), work_width));
    const int inner_max_width = std::max(1, max_width - padding * 2);
    const int max_item_width = std::max(scale_dip(96, dpi), inner_max_width);

    std::vector<CandidateItemLayout> items;
    int row_x = 0;
    int row_y = 0;
    int row_width = 0;
    int content_width = 0;

    for (size_t i = 0; i < candidates.size(); ++i) {
        const std::wstring number = std::to_wstring(i + 1);
        const int number_width = width_from_input_or_estimate(input.number_text_widths_px,
                                                              i,
                                                              number,
                                                              result.candidate_font_px);
        const int text_width = width_from_input_or_estimate(input.candidate_text_widths_px,
                                                            i,
                                                            candidates[i],
                                                            result.candidate_font_px);
        int item_width = item_pad_x * 2 + number_width + number_gap + text_width + text_slack;
        item_width = std::min(item_width, max_item_width);
        item_width = std::max(item_width, scale_dip(58, dpi));
        const int text_available_width = std::max(0, item_width - item_pad_x * 2 - number_width - number_gap);

        if (row_x > 0 && row_x + item_gap + item_width > inner_max_width) {
            content_width = std::max(content_width, row_width);
            row_x = 0;
            row_width = 0;
            row_y += candidate_row_height + row_gap;
        }

        CandidateItemLayout item;
        item.candidate_index = i;
        item.selected = i == std::min(input.selected_index, candidates.size() - 1);
        item.display_text = candidates[i];
        item.truncated = text_width > text_available_width;
        item.number_text_width_px = number_width;
        item.candidate_text_width_px = text_width;
        item.text_available_width_px = text_available_width;
        item.horizontal_padding_px = item_pad_x;
        item.number_gap_px = number_gap;
        item.bounds = RECT{row_x, row_y, row_x + item_width, row_y + candidate_row_height};
        item.number_bounds = RECT{
            item.bounds.left + item_pad_x,
            item.bounds.top,
            item.bounds.left + item_pad_x + number_width,
            item.bounds.bottom
        };
        item.text_bounds = RECT{
            item.number_bounds.right + number_gap,
            item.bounds.top,
            item.bounds.right - item_pad_x,
            item.bounds.bottom
        };
        items.push_back(std::move(item));

        row_x += item_width + item_gap;
        row_width = row_x > 0 ? row_x - item_gap : 0;
    }

    content_width = std::max(content_width, row_width);
    int width = clamp_int(content_width + padding * 2, min_width, max_width);
    const int rows_height = row_y + candidate_row_height;
    int height = padding + composition_height + scale_dip(7, dpi) + rows_height + padding;
    bool show_hint = input.options.show_key_hints;
    if (show_hint) {
        height += row_gap + hint_height;
        if (height > work_height * 3 / 5) {
            show_hint = false;
            height -= row_gap + hint_height;
        }
    }

    result.visible = true;
    result.width = width;
    result.height = std::min(height, work_height);
    result.show_hint = show_hint;

    const int candidate_top = padding + composition_height + scale_dip(7, dpi);
    result.composition_rect = RECT{padding, padding, width - padding, padding + composition_height};
    if (show_hint) {
        result.hint_rect = RECT{
            padding,
            result.height - padding - hint_height,
            width - padding,
            result.height - padding
        };
    }

    for (auto& item : items) {
        OffsetRect(&item.bounds, padding, candidate_top);
        OffsetRect(&item.number_bounds, padding, candidate_top);
        OffsetRect(&item.text_bounds, padding, candidate_top);
    }
    result.items = std::move(items);

    const int desired_x = input.caret_rect.left;
    int x = clamp_int(desired_x, input.work_area.left, input.work_area.right - width);

    const int below_y = input.caret_rect.bottom + placement_gap;
    const int above_y = input.caret_rect.top - result.height - placement_gap;
    const bool fits_below = below_y + result.height <= input.work_area.bottom;
    const bool fits_above = above_y >= input.work_area.top;

    int y = below_y;
    result.placement = CandidatePlacement::Below;
    if (!fits_below && (fits_above || above_y - input.work_area.top >= input.work_area.bottom - below_y)) {
        y = above_y;
        result.placement = CandidatePlacement::Above;
    }
    const int desired_y = y;
    y = clamp_int(y, input.work_area.top, input.work_area.bottom - result.height);

    result.rect_clamped = x != desired_x || y != desired_y;
    result.window_rect = RECT{x, y, x + width, y + result.height};
    return result;
}

}  // namespace localpinyin
