#pragma once

#include <windows.h>

#include <string>
#include <array>
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
};

struct CandidateThemePalette {
    bool dark = false;
    COLORREF background = RGB(255, 255, 255);
    COLORREF border = RGB(210, 210, 210);
    COLORREF text = RGB(24, 24, 24);
    COLORREF muted_text = RGB(105, 105, 105);
    COLORREF hint_text = RGB(120, 120, 120);
    COLORREF number_text = RGB(0, 92, 180);
    COLORREF selected_background = RGB(220, 236, 255);
    COLORREF selected_border = RGB(120, 170, 230);
    COLORREF selected_text = RGB(255, 255, 255);
    // Pinyin row, pointer feedback, the selected item's number and the
    // drop shadow's peak opacity (0-255).
    COLORREF composition_text = RGB(80, 86, 96);
    COLORREF hover_background = RGB(238, 243, 252);
    COLORREF selected_number_text = RGB(214, 230, 255);
    int shadow_alpha = 60;
};

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
};

struct CandidateItemLayout {
    size_t candidate_index = 0; // Original model index, never a truncated label lookup.
    size_t page_number = 1;
    RECT bounds{};
    RECT number_bounds{};
    RECT text_bounds{};
    bool selected = false;
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
// The top row shows the segmented pinyin, the bottom row the candidates.
struct CandidateLayoutResult {
    bool visible = false;
    CandidatePlacement placement = CandidatePlacement::Hidden;
    RECT window_rect{};
    RECT panel_rect{};
    RECT panel_screen_rect{};
    int shadow_margin_px = 0;
    bool paging_visible = false;
    RECT composition_rect{};
    RECT candidate_area_rect{};
    RECT action_button_rect{};
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
    int selected_corner_radius = 0;
    int panel_padding_px = 0;
    int item_gap_px = 0;
    int row_gap_px = 0;
    int composition_font_px = 0;
    int candidate_font_px = 0;
    int hint_font_px = 0;
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
CandidateThemePalette make_candidate_palette(CandidateThemeMode mode, bool system_dark);

CandidateLayoutResult make_candidate_layout(const CandidateLayoutInput& input);

}  // namespace localpinyin
