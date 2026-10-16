#include "candidate_ui.h"

#include "local_pinyin_blue_theme.h"

#include <algorithm>
#include <cwctype>
#include <sstream>

namespace localpinyin {
namespace {

constexpr int kMaxCandidates = 9;
constexpr int kCandidateTextSizeMinLevel = 1;
constexpr int kCandidateTextSizeMaxLevel = 7;
constexpr int kCandidateTextSizeDefaultLevel = 4;
constexpr int kCandidateTextSizeDipByLevel[] = {14, 16, 17, 18, 20, 22, 24};
constexpr int kShadowMarginDip = 12;
constexpr int kMinimumPanelWidthDip = 140;

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
    CandidateThemePalette palette;
    palette.dark = dark;
    if (dark) {
        palette.background = RGB(43, 45, 50);
        palette.border = RGB(64, 68, 76);
        palette.text = RGB(240, 242, 245);
        palette.muted_text = RGB(150, 156, 166);
        palette.hint_text = RGB(128, 134, 145);
        palette.number_text = RGB(140, 146, 157);
        palette.selected_background = RGB(61, 130, 246);
        palette.selected_border = RGB(61, 130, 246);
        palette.selected_text = RGB(255, 255, 255);
        palette.composition_text = RGB(186, 192, 202);
        palette.hover_background = RGB(56, 59, 66);
        palette.selected_number_text = RGB(205, 224, 255);
        palette.shadow_alpha = 110;
    } else {
        palette.background = RGB(255, 255, 255);
        palette.border = RGB(221, 225, 232);
        palette.text = RGB(28, 31, 36);
        palette.muted_text = RGB(124, 131, 143);
        palette.hint_text = RGB(140, 146, 157);
        palette.number_text = RGB(140, 146, 157);
        palette.selected_background = RGB(25, 118, 243);
        palette.selected_border = RGB(25, 118, 243);
        palette.selected_text = RGB(255, 255, 255);
        palette.composition_text = RGB(78, 86, 99);
        palette.hover_background = RGB(239, 243, 250);
        palette.selected_number_text = RGB(208, 226, 255);
        palette.shadow_alpha = 52;
    }
    return palette;
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

std::optional<size_t> candidate_model_index(const CandidateLayoutResult& layout, size_t model_index,
    const std::vector<std::wstring>& candidates) noexcept {
    for (const auto& item : layout.items) {
        if (item.candidate_index == model_index && model_index < candidates.size()) return model_index;
    }
    return std::nullopt;
}

CandidateLayoutResult make_candidate_layout(const CandidateLayoutInput& input) {
    CandidateLayoutResult result;
    result.dpi = std::max(1, input.dpi);
    const size_t count = (std::min)(input.candidate_texts.size(), size_t{kMaxCandidates});
    const int work_width = rect_width(input.work_area);
    const int work_height = rect_height(input.work_area);
    if (!should_show_candidate_window(input.composition_text, count, false) ||
        work_width <= 0 || work_height <= 0) return result;

    const int dpi = result.dpi;
    // Every measure shrinks proportionally on a work area narrower than the
    // nominal popup, so the panel and its rows always fit the monitor.
    const int max_width = (std::min)(scale_dip(720, dpi), work_width);
    const int margin = scale_dip(kShadowMarginDip, dpi);
    const int padding_x = (std::min)(scale_dip(10, dpi), max_width / 16);
    const int padding_y = (std::min)(scale_dip(6, dpi), max_width / 24);
    const int gap = (std::min)(scale_dip(2, dpi), max_width / 64);
    const int button = (std::min)(scale_dip(24, dpi), max_width / 10);
    const int pager_width = button * 2 + gap;
    result.composition_font_px = scale_dip(13, dpi);
    result.candidate_font_px = candidate_font_px_for_size(input.options.text_size, dpi);
    result.hint_font_px = scale_dip(12, dpi);
    const int composition_row = result.composition_font_px + scale_dip(9, dpi);
    const int candidate_row = (std::max)(result.candidate_font_px + scale_dip(14, dpi), button);
    const int row_gap = scale_dip(1, dpi);
    const int panel_height = (std::min)(padding_y * 2 + composition_row + row_gap + candidate_row, work_height);
    const int inner_width = (std::max)(1, max_width - padding_x * 2);
    const int item_pad = (std::min)(scale_dip(8, dpi), inner_width / 10);
    const int number_gap = (std::min)(scale_dip(5, dpi), inner_width / 16);
    const int slack = scale_dip(2, dpi);

    std::vector<int> text_widths(count), number_widths(count), item_widths(count);
    const auto paginate = [&](int available) {
        std::vector<UINT> pages{0};
        int used = 0;
        for (size_t i = 0; i < count; ++i) {
            item_widths[i] = (std::min)(available,
                item_pad * 2 + number_widths[i] + number_gap + text_widths[i] + slack);
            if (used > 0 && used + gap + item_widths[i] > available) {
                pages.push_back(static_cast<UINT>(i));
                used = 0;
            }
            used += (used ? gap : 0) + item_widths[i];
        }
        return pages;
    };
    for (size_t i = 0; i < count; ++i) {
        text_widths[i] = width_from_input_or_estimate(input.candidate_text_widths_px, i,
            input.candidate_texts[i], result.candidate_font_px);
        number_widths[i] = width_from_input_or_estimate(input.number_text_widths_px, i,
            std::to_wstring(i + 1), result.candidate_font_px);
    }
    // Page buttons only appear when the candidates need more than one page.
    // A settings button closes the candidate row (omitted where it cannot
    // open the settings app, e.g. in Windows Store app sandboxes).
    const int action_space = input.settings_button ? button + gap * 3 : 0;
    int candidate_width = (std::max)(1, inner_width - action_space);
    result.page_indices = paginate(candidate_width);
    const bool host_pages = !input.page_indices.empty() && input.page_indices.front() == 0 &&
        input.page_indices.back() < count &&
        std::adjacent_find(input.page_indices.begin(), input.page_indices.end(),
            [](UINT a, UINT b) { return a >= b; }) == input.page_indices.end();
    if (result.page_indices.size() > 1 || (host_pages && input.page_indices.size() > 1)) {
        candidate_width = (std::max)(1, inner_width - action_space - pager_width - gap);
        result.page_indices = paginate(candidate_width);
    }
    // ITfCandidateListUIElement permits the host to choose page boundaries.
    // Preserve those boundaries in the popup and keyboard mapping as well.
    if (host_pages) result.page_indices = input.page_indices;
    result.paging_visible = result.page_indices.size() > 1;

    const size_t selection = (std::min)(input.selected_index, count - 1);
    const auto next_page = std::upper_bound(result.page_indices.begin(), result.page_indices.end(), selection);
    result.current_page = static_cast<size_t>(next_page - result.page_indices.begin() - 1);
    result.has_previous_page = result.current_page > 0;
    result.has_next_page = result.current_page + 1 < result.page_indices.size();
    const size_t begin = result.page_indices[result.current_page];
    const size_t end = result.has_next_page ? result.page_indices[result.current_page + 1] : count;
    int page_used = 0;
    for (size_t i = begin; i < end; ++i) page_used += item_widths[i] + (i > begin ? gap : 0);
    const int visible_width = (std::min)(candidate_width, page_used);
    const int label_measured = input.composition_text_width_px > 0 ? input.composition_text_width_px :
        estimate_candidate_text_width_px(input.composition_text, result.composition_font_px);
    const int content_width = (std::max)(
        visible_width + (result.paging_visible ? gap + pager_width : 0) + action_space,
        (std::min)(label_measured + item_pad, inner_width));
    const int panel_width = (std::min)(max_width,
        (std::max)(padding_x * 2 + content_width, (std::min)(scale_dip(kMinimumPanelWidthDip, dpi), max_width)));

    result.visible = true;
    result.shadow_margin_px = margin;
    result.width = panel_width + margin * 2;
    result.height = panel_height + margin * 2;
    result.panel_rect = RECT{margin, margin, margin + panel_width, margin + panel_height};
    result.panel_padding_px = padding_x;
    result.item_gap_px = gap;
    result.row_gap_px = 0;
    result.corner_radius = scale_dip(8, dpi);
    result.selected_corner_radius = scale_dip(6, dpi);
    result.show_hint = false;
    result.reserves_action_icon_space = input.settings_button;
    result.action_button_visible = input.settings_button;
    result.action_button_size_px = input.settings_button ? button : 0;
    result.action_button_gap_px = input.settings_button ? gap : 0;

    const int left = margin + padding_x;
    const int right = margin + panel_width - padding_x;
    const int top = margin + padding_y;
    const int composition_bottom = (std::min)(top + composition_row, margin + panel_height);
    const int candidate_top = (std::min)(composition_bottom + row_gap, margin + panel_height);
    const int candidate_bottom = (std::max)(candidate_top, margin + panel_height - padding_y);
    // The pinyin aligns with the first candidate's number.
    result.composition_rect = RECT{left + item_pad, top, right, composition_bottom};
    const int controls_right = right - action_space;
    const int pager_left = controls_right - (result.paging_visible ? pager_width : 0);
    result.candidate_area_rect = RECT{left, candidate_top,
        result.paging_visible ? (std::max)(left, pager_left - gap) : (std::max)(left, controls_right), candidate_bottom};
    const int button_top = candidate_top + (candidate_bottom - candidate_top - button) / 2;
    if (result.paging_visible) {
        result.previous_page_rect = RECT{pager_left, button_top, pager_left + button, button_top + button};
        result.next_page_rect = RECT{pager_left + button + gap, button_top, controls_right, button_top + button};
    }
    result.page_indicator_rect = RECT{};
    result.action_button_rect = input.settings_button ? RECT{right - button, button_top, right, button_top + button} : RECT{};
    result.hint_rect = RECT{};

    const int available = (std::max)(1, static_cast<int>(result.candidate_area_rect.right - left));
    int x = left;
    for (size_t i = begin; i < end; ++i) {
        CandidateItemLayout item;
        item.candidate_index = i;
        item.page_number = i - begin + 1;
        item.selected = i == selection;
        item.display_text = input.candidate_texts[i];
        const int item_width = page_used <= available ? item_widths[i] :
            (std::max)(0, (available - gap * static_cast<int>(end - begin - 1)) / static_cast<int>(end - begin));
        const int number_width = (std::min)(number_widths[i], (std::max)(0, item_width - item_pad * 2));
        const int text_left = (std::min)(x + item_width, x + item_pad + number_width + number_gap);
        const int text_right = (std::max)(text_left, x + item_width - item_pad);
        item.bounds = RECT{x, candidate_top, x + item_width, candidate_bottom};
        item.number_bounds = RECT{x + (std::min)(item_pad, item_width), candidate_top,
            x + (std::min)(item_pad, item_width) + number_width, candidate_bottom};
        item.text_bounds = RECT{text_left, candidate_top, text_right, candidate_bottom};
        item.number_text_width_px = number_widths[i];
        item.candidate_text_width_px = text_widths[i];
        item.text_available_width_px = text_right - text_left;
        item.horizontal_padding_px = item_pad;
        item.number_gap_px = number_gap;
        item.truncated = text_widths[i] > item.text_available_width_px;
        result.items.push_back(std::move(item));
        x += item_width + gap;
    }

    // The panel (not its shadow) is placed against the caret: the pinyin
    // starts under the caret, below it when there is room, otherwise above.
    const int placement_gap = scale_dip(4, dpi);
    const int desired_x = input.caret_rect.left - padding_x - item_pad;
    const int panel_x = clamp_int(desired_x, input.work_area.left, input.work_area.right - panel_width);
    const int below_y = input.caret_rect.bottom + placement_gap;
    const int above_y = input.caret_rect.top - panel_height - placement_gap;
    result.placement = CandidatePlacement::Below;
    int desired_y = below_y;
    if (below_y + panel_height > input.work_area.bottom &&
        (above_y >= input.work_area.top || above_y - input.work_area.top >= input.work_area.bottom - below_y)) {
        result.placement = CandidatePlacement::Above;
        desired_y = above_y;
    }
    const int panel_y = clamp_int(desired_y, input.work_area.top, input.work_area.bottom - panel_height);
    result.rect_clamped = panel_x != desired_x || panel_y != desired_y;
    result.panel_screen_rect = RECT{panel_x, panel_y, panel_x + panel_width, panel_y + panel_height};
    result.window_rect = RECT{panel_x - margin, panel_y - margin,
                              panel_x + panel_width + margin, panel_y + panel_height + margin};
    return result;
}

}  // namespace localpinyin
