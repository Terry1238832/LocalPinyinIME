#include "candidate_ui.h"

#include "localpinyin_design_tokens.h"
#include "candidate_styles/paint_core.h"

#include <algorithm>
#include <cwctype>
#include <sstream>

namespace localpinyin {
namespace {

constexpr int kCandidateTextSizeMinLevel = 1;
constexpr int kCandidateTextSizeMaxLevel = 7;
constexpr int kCandidateTextSizeDefaultLevel = 4;
constexpr int kCandidateTextSizeDipByLevel[] = {14, 16, 17, 18, 20, 22, 24};

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

int candidate_number_font_px(int candidate_font_px, int dpi) noexcept {
    return (std::max)(scale_dip(10, dpi), static_cast<int>(candidate_font_px * 0.62f + 0.5f));
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

// Every style's generated light and dark token namespaces have identical members.
#define LOCALPINYIN_CANDIDATE_PALETTE(ns) [] {               \
    CandidateThemePalette palette;                           \
    palette.background = ns::kBackground;                    \
    palette.surface_top = ns::kSurfaceTop;                   \
    palette.border = ns::kBorder;                            \
    palette.inner_edge = ns::kInnerEdge;                     \
    palette.highlight = ns::kHighlight;                      \
    palette.text = ns::kText;                                \
    palette.text_muted = ns::kTextMuted;                     \
    palette.mark = ns::kMark;                                \
    palette.composition_text = ns::kComposition;             \
    palette.separator_text = ns::kSeparator;                 \
    palette.caret = ns::kCaret;                              \
    palette.focus_background = ns::kFocus;                   \
    palette.focus_top = ns::kFocusTop;                       \
    palette.focus_text = ns::kFocusText;                     \
    palette.focus_border = ns::kFocusEdge;                   \
    palette.focus_bar = ns::kFocusBar;                       \
    palette.focus_keycap = ns::kFocusKeycap;                 \
    palette.focus_number_text = ns::kFocusNumber;            \
    palette.chip_background = ns::kChip;                     \
    palette.chip_border = ns::kChipEdge;                     \
    palette.keycap = ns::kKeycap;                            \
    palette.number_text = ns::kNumber;                       \
    palette.hint_text = ns::kHint;                           \
    palette.hover_background = ns::kHover;                   \
    palette.pressed_background = ns::kPressed;               \
    palette.control = ns::kControl;                          \
    palette.control_disabled = ns::kControlDisabled;         \
    palette.divider = ns::kDivider;                          \
    palette.shadow_key_alpha = ns::kShadowKeyAlpha;          \
    palette.shadow_ambient_alpha = ns::kShadowAmbientAlpha;  \
    palette.focus_shadow_alpha = ns::kFocusShadowAlpha;      \
    return palette;                                          \
}()

CandidateThemePalette make_candidate_palette(CandidateThemeMode mode, bool system_dark, CandidateStyle style) {
    const bool dark = mode == CandidateThemeMode::Dark || (mode == CandidateThemeMode::System && system_dark);
    CandidateThemePalette palette;
    switch (style) {
        case CandidateStyle::CursorRail:
            palette = dark ? LOCALPINYIN_CANDIDATE_PALETTE(tokens::candidate_rail_dark)
                           : LOCALPINYIN_CANDIDATE_PALETTE(tokens::candidate_rail_light);
            break;
        case CandidateStyle::ModernType:
            palette = dark ? LOCALPINYIN_CANDIDATE_PALETTE(tokens::candidate_type_dark)
                           : LOCALPINYIN_CANDIDATE_PALETTE(tokens::candidate_type_light);
            break;
        case CandidateStyle::FloatingGlass:
            palette = dark ? LOCALPINYIN_CANDIDATE_PALETTE(tokens::candidate_glass_dark)
                           : LOCALPINYIN_CANDIDATE_PALETTE(tokens::candidate_glass_light);
            break;
        case CandidateStyle::Capsule:
            palette = dark ? LOCALPINYIN_CANDIDATE_PALETTE(tokens::candidate_capsule_dark)
                           : LOCALPINYIN_CANDIDATE_PALETTE(tokens::candidate_capsule_light);
            break;
        case CandidateStyle::ColumnList:
            palette = dark ? LOCALPINYIN_CANDIDATE_PALETTE(tokens::candidate_column_dark)
                           : LOCALPINYIN_CANDIDATE_PALETTE(tokens::candidate_column_light);
            break;
        case CandidateStyle::SplitFlap:
            palette = dark ? LOCALPINYIN_CANDIDATE_PALETTE(tokens::candidate_flap_dark)
                           : LOCALPINYIN_CANDIDATE_PALETTE(tokens::candidate_flap_light);
            break;
        case CandidateStyle::Highlighter:
            palette = dark ? LOCALPINYIN_CANDIDATE_PALETTE(tokens::candidate_marker_dark)
                           : LOCALPINYIN_CANDIDATE_PALETTE(tokens::candidate_marker_light);
            break;
        case CandidateStyle::AdaptiveFlow:
        default:
            palette = dark ? LOCALPINYIN_CANDIDATE_PALETTE(tokens::candidate_flow_dark)
                           : LOCALPINYIN_CANDIDATE_PALETTE(tokens::candidate_flow_light);
            break;
    }
    palette.dark = dark;
    return palette;
}
#undef LOCALPINYIN_CANDIDATE_PALETTE

CandidateThemePalette make_high_contrast_candidate_palette() {
    // System colors only: window text on window, the selection in the
    // highlight colors; no tints, gradients or shadows.
    const COLORREF window = GetSysColor(COLOR_WINDOW);
    const COLORREF ink = GetSysColor(COLOR_WINDOWTEXT);
    const COLORREF highlight = GetSysColor(COLOR_HIGHLIGHT);
    const COLORREF highlight_ink = GetSysColor(COLOR_HIGHLIGHTTEXT);
    const COLORREF disabled = GetSysColor(COLOR_GRAYTEXT);
    CandidateThemePalette palette;
    palette.high_contrast = true;
    palette.dark = (GetRValue(window) + GetGValue(window) + GetBValue(window)) < 384;
    palette.background = palette.surface_top = window;
    palette.border = palette.inner_edge = ink;
    palette.highlight = window;
    palette.text = palette.text_muted = palette.composition_text = palette.separator_text = ink;
    palette.caret = palette.mark = palette.focus_bar = ink;
    palette.focus_background = palette.focus_top = highlight;
    palette.focus_text = palette.focus_number_text = highlight_ink;
    palette.focus_border = highlight;
    palette.focus_keycap = highlight;
    palette.chip_background = window;
    palette.chip_border = ink;
    palette.keycap = window;
    palette.number_text = palette.hint_text = palette.control = ink;
    palette.hover_background = palette.pressed_background = window;
    palette.control_disabled = disabled;
    palette.divider = ink;
    palette.shadow_key_alpha = palette.shadow_ambient_alpha = palette.focus_shadow_alpha = 0;
    return palette;
}

CandidateStyle parse_candidate_style(const std::wstring& value) noexcept {
    const std::wstring lowered = lower_ascii(value);
    if (lowered == L"rail") return CandidateStyle::CursorRail;
    if (lowered == L"type") return CandidateStyle::ModernType;
    if (lowered == L"glass") return CandidateStyle::FloatingGlass;
    if (lowered == L"flow") return CandidateStyle::AdaptiveFlow;
    if (lowered == L"capsule") return CandidateStyle::Capsule;
    if (lowered == L"column") return CandidateStyle::ColumnList;
    if (lowered == L"flap") return CandidateStyle::SplitFlap;
    if (lowered == L"marker") return CandidateStyle::Highlighter;
    return kDefaultCandidateStyle;
}

std::wstring candidate_style_to_string(CandidateStyle style) {
    switch (style) {
        case CandidateStyle::CursorRail: return L"rail";
        case CandidateStyle::ModernType: return L"type";
        case CandidateStyle::FloatingGlass: return L"glass";
        case CandidateStyle::Capsule: return L"capsule";
        case CandidateStyle::ColumnList: return L"column";
        case CandidateStyle::SplitFlap: return L"flap";
        case CandidateStyle::Highlighter: return L"marker";
        case CandidateStyle::AdaptiveFlow:
        default: return L"flow";
    }
}

const wchar_t* candidate_font_family(CandidateFontFace face) noexcept {
    return face == CandidateFontFace::Numeral ? L"Segoe UI" : L"Microsoft YaHei UI";
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
    switch (input.options.style) {
        case CandidateStyle::CursorRail: return candidate_styles::layout_cursor_rail(input);
        case CandidateStyle::ModernType: return candidate_styles::layout_modern_type(input);
        case CandidateStyle::FloatingGlass: return candidate_styles::layout_floating_glass(input);
        case CandidateStyle::Capsule: return candidate_styles::layout_capsule(input);
        case CandidateStyle::ColumnList: return candidate_styles::layout_column_list(input);
        case CandidateStyle::SplitFlap: return candidate_styles::layout_split_flap(input);
        case CandidateStyle::Highlighter: return candidate_styles::layout_highlighter(input);
        case CandidateStyle::AdaptiveFlow:
        default: return candidate_styles::layout_adaptive_flow(input);
    }
}

}  // namespace localpinyin
