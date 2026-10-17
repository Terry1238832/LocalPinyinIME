#pragma once

#include <windows.h>

#include <string>
#include <array>
#include <functional>
#include <vector>
#include <optional>

namespace localpinyin {

enum class CandidateThemeMode {
    System,
    Light,
    Dark
};

enum class CandidateTextSize {
    Level1 = 1,
    Level2 = 2,
    Level3 = 3,
    Standard = 4,
    Level5 = 5,
    Level6 = 6,
    Level7 = 7,
    Small = Level1,
    Large = Level7
};

// Candidate window styles. They share the input state, paging rules and
// keyboard behavior; each has its own layout and painter
// (src/ime/candidate_styles). New styles add an entry here, in
// kCandidateStyles, and in the design tokens.
enum class CandidateStyle {
    AdaptiveFlow,   // "flow": the layout follows the candidates (default)
    CursorRail,     // "rail": candidates along one rail, a gliding focus line
    ModernType,     // "type": a typeset line; size, weight and a short rule
    FloatingGlass,  // "glass": a layered panel with a ceramic selected key
    Capsule,        // "capsule": one rounded bar, a solid pill on the choice
    ColumnList,     // "column": one candidate per row under the pinyin
    Outline,        // "outline": a flat sheet, the choice ringed in accent
    InkDot          // "dot": type only, an accent dot under the choice
};

inline constexpr CandidateStyle kDefaultCandidateStyle = CandidateStyle::AdaptiveFlow;
inline constexpr std::array<CandidateStyle, 8> kCandidateStyles{
    CandidateStyle::CursorRail, CandidateStyle::ModernType,
    CandidateStyle::FloatingGlass, CandidateStyle::AdaptiveFlow,
    CandidateStyle::Capsule, CandidateStyle::ColumnList,
    CandidateStyle::Outline, CandidateStyle::InkDot};

// Adaptive Flow's three forms, chosen from the measured candidates.
enum class CandidateFlowForm {
    Compact,    // one row: short words
    Balanced,   // the current choice on its own row, alternatives below
    Expanded    // one candidate per row: long words and sentences
};

// Fonts the styles draw with (all ship with Windows).
enum class CandidateFontFace {
    Ui,       // Microsoft YaHei UI: Chinese, pinyin, labels
    Numeral   // Segoe UI: Modern Type's small figures
};

enum class CandidatePlacement {
    Hidden,
    Below,
    Above
};

enum class CandidateHideReason {
    None,
    EnglishMode,
    EmptyComposition,
    EmptyCandidates,
    NoCaretRect
};

enum class CandidateCreateParentKind {
    Null,
    ValidOwner,
    RejectedInvalidOwner
};

struct CandidateDisplayState {
    bool english_mode = false;
    bool composition_active = false;
    size_t candidate_count = 0;
    bool caret_rect_available = false;
};

struct CandidateDisplayDecision {
    bool request_show = false;
    CandidateHideReason hide_reason = CandidateHideReason::None;
};

struct CandidateWindowCreateConfig {
    DWORD ex_style = 0;
    DWORD style = 0;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    HWND parent = nullptr;
    HMENU menu = nullptr;
    CandidateCreateParentKind parent_kind = CandidateCreateParentKind::Null;
    bool parent_is_window = false;
    bool use_topmost_after_create = true;
};

inline constexpr DWORD kCandidateDwmCloakedShell = 0x00000002;

struct CandidateShellCloakRecoveryInput {
    bool candidate_model_valid = false;
    bool candidate_show_requested = false;
    bool candidate_hwnd_valid = false;
    bool candidate_hwnd_visible = false;
    DWORD current_process_id = 0;
    DWORD candidate_process_id = 0;
    HRESULT candidate_cloak_hr = E_FAIL;
    DWORD candidate_cloak_value = 0;
    bool owner_root_valid = false;
    DWORD owner_root_process_id = 0;
    HRESULT owner_root_cloak_hr = E_FAIL;
    DWORD owner_root_cloak_value = 0;
    unsigned int recovery_attempts = 0;
};

enum class CandidateShellCloakRecoveryStrategy {
    None,
    SameProcessContextRootOwner
};

struct CandidateShellCloakRecoveryDecision {
    bool shell_cloak_detected = false;
    bool recovery_eligible = false;
    bool recovery_attempt_allowed = false;
    bool owner_same_process = false;
    CandidateShellCloakRecoveryStrategy strategy = CandidateShellCloakRecoveryStrategy::None;
    std::wstring reason;
};

struct CandidateReplacementCommitInput {
    bool replacement_hwnd_valid = false;
    bool replacement_visible = false;
    bool owner_verified = false;
    bool dwm_query_succeeded = false;
    bool shell_cloaked = false;
    bool rect_on_monitor = false;
};

struct CandidateLayoutRetryDecision {
    bool schedule = false;
    unsigned int next_attempt = 0;
    UINT delay_ms = 0;
};

struct CandidateWindowOptions {
    CandidateThemeMode theme_mode = CandidateThemeMode::System;
    bool show_key_hints = true;
    CandidateTextSize text_size = CandidateTextSize::Standard;
    CandidateStyle style = kDefaultCandidateStyle;
};

// Filled from the design tokens (assets/branding/theme/design_tokens.json):
// every style fills the same fields with its own colors. The comments give
// Adaptive Flow's use; the other styles document theirs in their painters.
struct CandidateThemePalette {
    bool dark = false;
    // System high-contrast colors; styles drop material effects.
    bool high_contrast = false;
    COLORREF background = RGB(255, 255, 255);
    // The top of a panel that shades downward (Floating Glass).
    COLORREF surface_top = RGB(255, 255, 255);
    // Glass edge: the border, a lighter hairline inside it, and in dark mode a
    // lighter line along the top.
    COLORREF border = RGB(215, 219, 226);
    COLORREF inner_edge = RGB(255, 255, 255);
    COLORREF highlight = RGB(255, 255, 255);
    COLORREF text = RGB(22, 23, 27);
    // Alternatives' text where a style sets them back from the current choice.
    COLORREF text_muted = RGB(22, 23, 27);
    // Query bar: the brand mark, pinyin syllables, their lighter apostrophes,
    // and the caret where typing continues (the icon's "P|").
    COLORREF mark = RGB(26, 111, 232);
    COLORREF composition_text = RGB(79, 82, 90);
    COLORREF separator_text = RGB(169, 174, 184);
    COLORREF caret = RGB(26, 111, 232);
    // Focus row: tinted, edged, with the brand caret at its left edge and an
    // accent keycap; Space (Tab for suggestions) commits it.
    COLORREF focus_background = RGB(238, 244, 253);
    COLORREF focus_top = RGB(238, 244, 253);
    COLORREF focus_text = RGB(22, 23, 27);
    COLORREF focus_border = RGB(205, 223, 250);
    COLORREF focus_bar = RGB(26, 111, 232);
    COLORREF focus_keycap = RGB(26, 111, 232);
    COLORREF focus_number_text = RGB(255, 255, 255);
    // Alternatives: chips with keycap numbers; the hint keycap in the focus row.
    COLORREF chip_background = RGB(245, 246, 248);
    COLORREF chip_border = RGB(231, 234, 238);
    COLORREF keycap = RGB(232, 235, 240);
    COLORREF number_text = RGB(100, 106, 121);
    COLORREF hint_text = RGB(124, 127, 135);
    COLORREF hover_background = RGB(236, 239, 244);
    COLORREF pressed_background = RGB(226, 230, 237);
    // Pager chevrons, page dots or number, settings gear.
    COLORREF control = RGB(134, 139, 150);
    COLORREF control_disabled = RGB(211, 215, 222);
    COLORREF divider = RGB(228, 231, 236);
    // Peak opacity (0-255) of the tight key shadow and the wide ambient one.
    int shadow_key_alpha = 30;
    int shadow_ambient_alpha = 46;
    // Peak opacity of the shadow under a raised selected key (0: flat).
    int focus_shadow_alpha = 0;
};

// Measures text as the window will draw it (DirectWrite in the IME). Layouts
// fall back to estimates when no measurer is supplied (tests, tools).
using CandidateTextMeasurer = std::function<int(const std::wstring& text, int font_px, int weight, CandidateFontFace face)>;

struct CandidateLayoutInput {
    std::wstring composition_text;
    std::vector<std::wstring> candidate_texts;
    std::vector<int> candidate_text_widths_px;
    std::vector<int> number_text_widths_px;
    size_t selected_index = 0;
    // A host may supply its own valid page boundaries. Empty computes widths.
    std::vector<UINT> page_indices;
    int composition_text_width_px = 0;
    RECT caret_rect{};
    RECT work_area{};
    int dpi = 96;
    CandidateWindowOptions options{};
    bool settings_button = true;
    // The focus row is drawn larger; hosts may pass its measured text widths.
    std::vector<int> focus_text_widths_px;
    // The key that commits the focus row, shown as a hint keycap.
    std::wstring focus_hint = L"\u7A7A\u683C";
    int focus_hint_width_px = 0;
    CandidateTextMeasurer measure;
    // Adaptive Flow keeps its previous form unless the candidates clearly
    // call for another, so the popup does not flip shape on every key.
    std::optional<CandidateFlowForm> previous_flow_form;
};

struct CandidateItemLayout {
    size_t candidate_index = 0; // Original model index, never a truncated label lookup.
    size_t page_number = 1;
    RECT bounds{};
    RECT number_bounds{};
    RECT text_bounds{};
    bool selected = false;
    // The current choice, drawn as the focus row; the others are chips.
    bool featured = false;
    bool truncated = false;
    int number_text_width_px = 0;
    int candidate_text_width_px = 0;
    int text_available_width_px = 0;
    int horizontal_padding_px = 0;
    int number_gap_px = 0;
    std::wstring display_text;
};

// The popup is a rounded panel with a soft shadow. The window extends the
// panel by `shadow_margin_px` on every side; every RECT below except
// `window_rect` and `panel_screen_rect` is in window client coordinates.
// All styles fill the shared fields (panel, pinyin, controls, items, pages);
// the remaining ones are named for the styles that use them.
struct CandidateLayoutResult {
    bool visible = false;
    CandidateStyle style = kDefaultCandidateStyle;
    // Adaptive Flow only.
    CandidateFlowForm flow_form = CandidateFlowForm::Balanced;
    CandidatePlacement placement = CandidatePlacement::Hidden;
    RECT window_rect{};
    RECT panel_rect{};
    RECT panel_screen_rect{};
    int shadow_margin_px = 0;
    bool paging_visible = false;
    RECT query_rect{};
    RECT mark_rect{};
    RECT composition_rect{};
    RECT focus_rect{};
    RECT chips_rect{};
    // Cursor Rail: the rail the candidates sit on and the page track below;
    // Modern Type: the hairline under the pinyin.
    RECT rail_rect{};
    RECT track_rect{};
    // Focus row and chip row together.
    RECT candidate_area_rect{};
    RECT action_button_rect{};
    // The commit-key hint keycap at the right end of the focus row.
    RECT hint_rect{};
    RECT previous_page_rect{};
    RECT page_indicator_rect{};
    RECT next_page_rect{};
    std::vector<UINT> page_indices;
    size_t current_page = 0;
    bool has_previous_page = false;
    bool has_next_page = false;
    bool show_hint = false;
    int width = 0;
    int height = 0;
    int dpi = 96;
    int corner_radius = 0;
    // Focus row and chip corner radii.
    int selected_corner_radius = 0;
    int chip_corner_radius = 0;
    int panel_padding_px = 0;
    int item_gap_px = 0;
    int row_gap_px = 0;
    int composition_font_px = 0;
    int candidate_font_px = 0;
    int focus_font_px = 0;
    int number_font_px = 0;
    int hint_font_px = 0;
    // Cursor Rail and Modern Type set every text size on this baseline.
    int baseline_y = 0;
    // Text weights the layout measured with, so the painter draws the same.
    int focus_weight = 400;
    int item_weight = 400;
    CandidateFontFace number_face = CandidateFontFace::Ui;
    bool rect_clamped = false;
    bool reserves_action_icon_space = false;
    bool action_button_visible = false;
    int action_button_size_px = 0;
    int action_button_gap_px = 0;
    std::vector<CandidateItemLayout> items;
};

// Pointer feedback does not change the keyboard selection. A press only commits
// when it is released on the same item; capture loss or a changed model cancels it.
class CandidatePointerState {
public:
    [[nodiscard]] std::optional<size_t> hovered() const noexcept { return hovered_; }
    [[nodiscard]] std::optional<size_t> pressed() const noexcept { return pressed_; }
    void move(std::optional<size_t> index) noexcept { hovered_ = index; }
    void press(std::optional<size_t> index) noexcept { hovered_ = pressed_ = index; }
    [[nodiscard]] std::optional<size_t> release(std::optional<size_t> index) noexcept {
        const auto result = index && index == pressed_ ? index : std::nullopt;
        pressed_.reset();
        hovered_ = index;
        return result;
    }
    void cancel() noexcept { hovered_.reset(); pressed_.reset(); }
private:
    std::optional<size_t> hovered_;
    std::optional<size_t> pressed_;
};

[[nodiscard]] std::optional<size_t> candidate_index_at_point(
    const CandidateLayoutResult& layout, POINT point) noexcept;
[[nodiscard]] std::optional<size_t> candidate_model_index(
    const CandidateLayoutResult& layout, size_t display_index,
    const std::vector<std::wstring>& candidates) noexcept;

int scale_dip(int value, int dpi) noexcept;
int candidate_text_size_min_level() noexcept;
int candidate_text_size_max_level() noexcept;
int candidate_text_size_default_level() noexcept;
int candidate_text_size_level(CandidateTextSize size) noexcept;
CandidateTextSize candidate_text_size_from_level(int level) noexcept;
int candidate_text_size_dip(CandidateTextSize size) noexcept;
std::wstring candidate_text_size_display_label(CandidateTextSize size);
std::wstring candidate_text_size_display_text(CandidateTextSize size);
int candidate_font_px_for_size(CandidateTextSize size, int dpi) noexcept;
// Keycap digits scale with the candidates but stay legible at small sizes.
int candidate_number_font_px(int candidate_font_px, int dpi) noexcept;
int estimate_candidate_text_width_px(const std::wstring& text, int font_px);
UINT candidate_text_draw_flags(bool truncated) noexcept;
bool should_show_candidate_window(const std::wstring& composition_text,
                                  size_t candidate_count,
                                  bool english_mode) noexcept;
CandidateDisplayDecision decide_candidate_display(const CandidateDisplayState& state) noexcept;
std::wstring candidate_hide_reason_to_string(CandidateHideReason reason);
CandidateWindowCreateConfig make_candidate_window_create_config(HWND requested_owner,
                                                               bool requested_owner_is_window) noexcept;
CandidateShellCloakRecoveryDecision decide_candidate_shell_cloak_recovery(
    const CandidateShellCloakRecoveryInput& input);
std::wstring candidate_shell_cloak_recovery_strategy_to_string(
    CandidateShellCloakRecoveryStrategy strategy);
bool should_commit_candidate_replacement(const CandidateReplacementCommitInput& input) noexcept;
inline constexpr std::array<UINT, 3> kCandidateLayoutRetryDelaysMs{50, 150, 300};
[[nodiscard]] constexpr CandidateLayoutRetryDecision decide_candidate_layout_retry(
    bool show_requested,
    bool anchor_available,
    unsigned int completed_attempts) noexcept {
    if (!show_requested || anchor_available ||
        completed_attempts >= kCandidateLayoutRetryDelaysMs.size()) {
        return {};
    }
    return CandidateLayoutRetryDecision{
        true,
        completed_attempts + 1,
        kCandidateLayoutRetryDelaysMs[completed_attempts],
    };
}

CandidateThemeMode parse_candidate_theme_mode(const std::wstring& value) noexcept;
CandidateTextSize parse_candidate_text_size(const std::wstring& value) noexcept;
std::wstring candidate_theme_mode_to_string(CandidateThemeMode mode);
std::wstring candidate_text_size_to_string(CandidateTextSize size);
// Style ids are "flow", "rail", "type", "glass", "capsule", "column",
// "outline" and "dot"; anything else (a missing
// key, a value from a newer version) falls back to the default style.
CandidateStyle parse_candidate_style(const std::wstring& value) noexcept;
std::wstring candidate_style_to_string(CandidateStyle style);
const wchar_t* candidate_font_family(CandidateFontFace face) noexcept;
CandidateThemePalette make_candidate_palette(CandidateThemeMode mode, bool system_dark,
                                             CandidateStyle style = kDefaultCandidateStyle);
// Windows high-contrast colors, flat, for every style.
CandidateThemePalette make_high_contrast_candidate_palette();

// Lays out the candidates in input.options.style.
CandidateLayoutResult make_candidate_layout(const CandidateLayoutInput& input);

// Adaptive Flow's choice of form, from the candidates' measured text widths
// at the candidate font size. Hysteresis: a form is entered on a clear
// signal and kept on a weaker one, so typing one more letter rarely changes
// the shape of the popup.
struct CandidateFlowMeasures {
    int font_px = 0;
    int longest_px = 0;
    int median_px = 0;
    // Widest row the popup may use on this monitor.
    int inner_max_px = 0;
};
CandidateFlowForm decide_candidate_flow_form(const CandidateFlowMeasures& measures,
                                             std::optional<CandidateFlowForm> previous) noexcept;
std::wstring candidate_flow_form_to_string(CandidateFlowForm form);

}  // namespace localpinyin
