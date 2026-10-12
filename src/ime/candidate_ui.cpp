#include "candidate_ui.h"

#include "local_pinyin_blue_theme.h"

#include <algorithm>
#include <cwctype>
#include <sstream>
#include <unordered_set>

namespace localpinyin {
namespace {

constexpr int kMaxCandidates = 9;
constexpr int kCandidateTextSizeMinLevel = 1;
constexpr int kCandidateTextSizeMaxLevel = 7;
constexpr int kCandidateTextSizeDefaultLevel = 4;
constexpr int kCandidateTextSizeDipByLevel[] = {14, 16, 17, 18, 20, 22, 24};

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

int candidate_text_size_min_level() noexcept {
    return kCandidateTextSizeMinLevel;
}

int candidate_text_size_max_level() noexcept {
    return kCandidateTextSizeMaxLevel;
}

int candidate_text_size_default_level() noexcept {
    return kCandidateTextSizeDefaultLevel;
}

int candidate_text_size_level(CandidateTextSize size) noexcept {
    return clamp_int(static_cast<int>(size), kCandidateTextSizeMinLevel, kCandidateTextSizeMaxLevel);
}

CandidateTextSize candidate_text_size_from_level(int level) noexcept {
    return static_cast<CandidateTextSize>(clamp_int(level, kCandidateTextSizeMinLevel, kCandidateTextSizeMaxLevel));
}

int candidate_text_size_dip(CandidateTextSize size) noexcept {
    const int level = candidate_text_size_level(size);
    return kCandidateTextSizeDipByLevel[level - 1];
}

std::wstring candidate_text_size_display_label(CandidateTextSize size) {
    const int level = candidate_text_size_level(size);
    if (level <= 2) {
        return L"\u5C0F";
    }
    if (level == kCandidateTextSizeDefaultLevel) {
        return L"\u6807\u51C6";
    }
    if (level >= 6) {
        return L"\u5927";
    }
    return L"\u7565" + std::wstring(level < kCandidateTextSizeDefaultLevel ? L"\u5C0F" : L"\u5927");
}

std::wstring candidate_text_size_display_text(CandidateTextSize size) {
    std::wstringstream stream;
    const int level = candidate_text_size_level(size);
    stream << L"\u7B2C " << level << L" \u6863 \u00B7 "
           << candidate_text_size_display_label(size) << L" \u00B7 "
           << candidate_text_size_dip(size) << L" px";
    return stream.str();
}

int candidate_font_px_for_size(CandidateTextSize size, int dpi) noexcept {
    return scale_dip(candidate_text_size_dip(size), dpi);
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

CandidateShellCloakRecoveryDecision decide_candidate_shell_cloak_recovery(
    const CandidateShellCloakRecoveryInput& input) {
    CandidateShellCloakRecoveryDecision decision;
    decision.shell_cloak_detected = SUCCEEDED(input.candidate_cloak_hr) &&
                                    (input.candidate_cloak_value & kCandidateDwmCloakedShell) != 0;
    decision.owner_same_process = input.current_process_id != 0 &&
                                  input.current_process_id == input.owner_root_process_id;
    if (!input.candidate_model_valid) {
        decision.reason = L"candidate_model_invalid";
        return decision;
    }
    if (!input.candidate_show_requested) {
        decision.reason = L"candidate_show_not_requested";
        return decision;
    }
    if (!input.candidate_hwnd_valid) {
        decision.reason = L"candidate_hwnd_invalid";
        return decision;
    }
    if (!input.candidate_hwnd_visible) {
        decision.reason = L"candidate_hwnd_not_visible";
        return decision;
    }
    if (FAILED(input.candidate_cloak_hr)) {
        decision.reason = L"candidate_cloak_query_failed";
        return decision;
    }
    if (!decision.shell_cloak_detected) {
        decision.reason = L"candidate_not_shell_cloaked";
        return decision;
    }
    if (!input.owner_root_valid) {
        decision.reason = L"owner_root_invalid";
        return decision;
    }
    if (input.current_process_id == 0 || input.owner_root_process_id == 0) {
        decision.reason = L"owner_process_unknown";
        return decision;
    }
    if (input.candidate_process_id != input.current_process_id) {
        decision.reason = L"candidate_process_mismatch";
        return decision;
    }
    if (input.recovery_attempts >= 1) {
        decision.reason = L"recovery_attempt_limit_reached";
        return decision;
    }
    if (decision.owner_same_process) {
        if (FAILED(input.owner_root_cloak_hr)) {
            decision.reason = L"owner_root_cloak_query_failed";
            return decision;
        }
        if ((input.owner_root_cloak_value & kCandidateDwmCloakedShell) != 0) {
            decision.reason = L"owner_root_shell_cloaked";
            return decision;
        }
        decision.strategy = CandidateShellCloakRecoveryStrategy::SameProcessContextRootOwner;
        decision.reason = L"eligible_context_root";
    } else {
        decision.reason = L"owner_root_cross_process";
        return decision;
    }
    decision.recovery_eligible = true;
    decision.recovery_attempt_allowed = true;
    return decision;
}

std::wstring candidate_shell_cloak_recovery_strategy_to_string(
    CandidateShellCloakRecoveryStrategy strategy) {
    switch (strategy) {
        case CandidateShellCloakRecoveryStrategy::SameProcessContextRootOwner:
            return L"same_process_context_root_owner";
        case CandidateShellCloakRecoveryStrategy::None:
        default:
            return L"none";
    }
}

bool should_commit_candidate_replacement(const CandidateReplacementCommitInput& input) noexcept {
    return input.replacement_hwnd_valid &&
           input.replacement_visible &&
           input.owner_verified &&
           input.dwm_query_succeeded &&
           !input.shell_cloaked &&
           input.rect_on_monitor;
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
    if (lowered.rfind(L"level", 0) == 0 && lowered.size() > 5) {
        int level = 0;
        for (size_t i = 5; i < lowered.size(); ++i) {
            if (lowered[i] < L'0' || lowered[i] > L'9') {
                return CandidateTextSize::Standard;
            }
            level = level * 10 + static_cast<int>(lowered[i] - L'0');
        }
        return candidate_text_size_from_level(level);
    }
    if (!lowered.empty() && std::all_of(lowered.begin(), lowered.end(), [](wchar_t ch) {
            return ch >= L'0' && ch <= L'9';
        })) {
        return candidate_text_size_from_level(std::stoi(lowered));
    }
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
    return L"level" + std::to_wstring(candidate_text_size_level(size));
}

CandidateThemePalette make_candidate_palette(CandidateThemeMode mode, bool system_dark) {
    const bool dark = mode == CandidateThemeMode::Dark || (mode == CandidateThemeMode::System && system_dark);
    if (dark) return CandidateThemePalette{true,
        RGB(38,38,42), RGB(57,57,63), RGB(246,247,250), RGB(176,178,187), RGB(140,143,156),
        RGB(96,175,255), RGB(96,175,255), RGB(96,175,255), RGB(17,35,56)};
    return CandidateThemePalette{false,
        RGB(255,255,255), RGB(215,218,225), RGB(26,28,34), RGB(100,104,116), RGB(138,143,154),
        RGB(0,103,192), RGB(0,103,192), RGB(0,103,192), RGB(255,255,255)};
}

std::optional<size_t> candidate_index_at_point(const CandidateLayoutResult& layout, POINT point) noexcept {
    if (!layout.visible) return std::nullopt;
    for (const auto& item : layout.items) {
        const RECT& bounds = item.bounds;
        if (point.x >= bounds.left && point.x < bounds.right &&
            point.y >= bounds.top && point.y < bounds.bottom) return item.candidate_index;
    }
    return std::nullopt;
}

std::optional<size_t> candidate_model_index(const CandidateLayoutResult& layout, size_t display_index,
    const std::vector<std::wstring>& candidates) noexcept {
    for (const auto& item : layout.items) {
        if (item.candidate_index != display_index) continue;
        // Layout removes duplicate labels. Resolve the visible word back to its
        // original model index instead of committing a different duplicate slot.
        const auto found = std::find(candidates.begin(), candidates.end(), item.display_text);
        if (found != candidates.end()) return static_cast<size_t>(found - candidates.begin());
        break;
    }
    return std::nullopt;
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
    const int item_gap = scale_dip(6, dpi);
    const int item_pad_x = scale_dip(10, dpi);
    const int item_pad_y = scale_dip(6, dpi);
    const int number_gap = scale_dip(6, dpi);
    const int placement_gap = scale_dip(6, dpi);
    const int action_button_size = scale_dip(30, dpi);
    const int action_button_gap = scale_dip(10, dpi);
    const int action_reserved_width = action_button_size + action_button_gap;
    result.corner_radius = scale_dip(12, dpi);
    result.selected_corner_radius = scale_dip(7, dpi);
    result.panel_padding_px = padding;
    result.item_gap_px = item_gap;
    result.row_gap_px = row_gap;
    result.composition_font_px = scale_dip(15, dpi);
    result.candidate_font_px = candidate_font_px_for_size(input.options.text_size, dpi);
    result.hint_font_px = scale_dip(13, dpi);
    result.reserves_action_icon_space = true;
    result.action_button_visible = true;
    result.action_button_size_px = action_button_size;
    result.action_button_gap_px = action_button_gap;
    const int text_slack = std::max(scale_dip(10, dpi), result.candidate_font_px / 2);

    const int composition_height = input.composition_text.empty()
                                       ? 0
                                       : result.composition_font_px + scale_dip(4, dpi);
    const int candidate_row_height = result.candidate_font_px + item_pad_y * 2;
    const int hint_height = input.options.show_key_hints ? result.hint_font_px + scale_dip(4, dpi) : 0;
    const int min_width = std::min(scale_dip(176, dpi), work_width);
    const int max_width = std::max(min_width, std::min(scale_dip(760, dpi), work_width));
    const int inner_max_width = std::max(1, max_width - padding * 2);
    const int candidate_inner_max_width = std::max(scale_dip(96, dpi), inner_max_width - action_reserved_width);
    const int max_item_width = std::max(scale_dip(96, dpi), candidate_inner_max_width);

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

        if (row_x > 0 && row_x + item_gap + item_width > candidate_inner_max_width) {
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
    int width = clamp_int(content_width + action_reserved_width + padding * 2, min_width, max_width);
    const int rows_height = row_y + candidate_row_height;
    const int composition_gap = composition_height > 0 ? scale_dip(8, dpi) : 0;
    int height = padding + composition_height + composition_gap + rows_height + padding;
    bool show_hint = input.options.show_key_hints;
    if (show_hint) {
        height += scale_dip(8, dpi) + hint_height;
        if (height > work_height * 3 / 5) {
            show_hint = false;
            height -= scale_dip(8, dpi) + hint_height;
        }
    }

    result.visible = true;
    result.width = width;
    result.height = std::min(height, work_height);
    result.show_hint = show_hint;

    const int candidate_top = padding + composition_height + composition_gap;
    result.composition_rect = RECT{padding, padding, width - padding, padding + composition_height};
    result.candidate_area_rect = RECT{padding,
                                      candidate_top,
                                      width - padding - action_reserved_width,
                                      candidate_top + rows_height};
    const int action_top = candidate_top + std::max(0, candidate_row_height - action_button_size) / 2;
    result.action_button_rect = RECT{width - padding - action_button_size,
                                     action_top,
                                     width - padding,
                                     action_top + action_button_size};
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
