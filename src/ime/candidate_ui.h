#pragma once

#include <windows.h>

#include <string>
#include <vector>

namespace localpinyin {

enum class CandidateThemeMode {
    System,
    Light,
    Dark
};

enum class CandidateTextSize {
    Small,
    Standard,
    Large
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

struct CandidateWindowShowStatus {
    bool set_window_pos_ok = false;
    bool show_window_called = false;
    bool shown = false;
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
};

struct CandidateLayoutInput {
    std::wstring composition_text;
    std::vector<std::wstring> candidate_texts;
    std::vector<int> candidate_text_widths_px;
    std::vector<int> number_text_widths_px;
    size_t selected_index = 0;
    RECT caret_rect{};
    RECT work_area{};
    int dpi = 96;
    CandidateWindowOptions options{};
};

struct CandidateItemLayout {
    size_t candidate_index = 0;
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

struct CandidateLayoutResult {
    bool visible = false;
    CandidatePlacement placement = CandidatePlacement::Hidden;
    RECT window_rect{};
    RECT composition_rect{};
    RECT candidate_area_rect{};
    RECT action_button_rect{};
    RECT hint_rect{};
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

int scale_dip(int value, int dpi) noexcept;
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
std::wstring candidate_create_parent_kind_to_string(CandidateCreateParentKind kind);
std::wstring make_candidate_window_create_attempt_log(const CandidateWindowCreateConfig& config);
std::wstring make_candidate_window_create_result_log(bool created, DWORD last_error);
CandidateWindowShowStatus make_candidate_window_show_status(bool set_window_pos_ok) noexcept;

CandidateThemeMode parse_candidate_theme_mode(const std::wstring& value) noexcept;
CandidateTextSize parse_candidate_text_size(const std::wstring& value) noexcept;
std::wstring candidate_theme_mode_to_string(CandidateThemeMode mode);
std::wstring candidate_text_size_to_string(CandidateTextSize size);
CandidateThemePalette make_candidate_palette(CandidateThemeMode mode, bool system_dark);

CandidateLayoutResult make_candidate_layout(const CandidateLayoutInput& input);

}  // namespace localpinyin
