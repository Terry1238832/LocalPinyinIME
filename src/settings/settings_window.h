#pragma once

#include "../engine/user_lexicon.h"
#include "../engine/pinyin_engine.h"
#include "dictionary_catalog.h"
#include "settings_store.h"
#include "modern_renderer.h"

#include <windows.h>
#include <commctrl.h>

#include <array>
#include <functional>
#include <string>
#include <vector>

namespace localpinyin {

enum class SettingsPage {
    Appearance = 0,
    Candidate = 1,
    UserLexicon = 2,
    Learning = 3,
    About = 4,
};

enum class UserLexiconEditorMode {
    NewEntry,
    EditExisting,
};

enum class UserLexiconInputField {
    Pinyin,
    Word,
    Frequency,
};

struct SettingsRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct SettingsSize {
    int width = 0;
    int height = 0;
};

struct SettingsWindowLayout {
    int dpi = 96;
    SettingsRect sidebar;
    SettingsRect content;
    SettingsRect page_title;
    SettingsRect page_description;
    std::array<SettingsRect, 5> nav_items{};
    SettingsRect primary_card;
    SettingsRect secondary_card;
    SettingsRect candidate_options_card;
    SettingsRect candidate_hint_title;
    SettingsRect candidate_hint_description;
    SettingsRect candidate_hint_toggle;
    SettingsRect candidate_shift_title;
    SettingsRect candidate_shift_description;
    SettingsRect candidate_shift_toggle;
    SettingsRect candidate_shift_note;
    SettingsRect shortcut_test_card;
    SettingsRect shortcut_test_title;
    SettingsRect shortcut_test_description;
    SettingsRect shortcut_test_ctrl_key;
    SettingsRect shortcut_test_space_key;
    SettingsRect shortcut_test_mode_label;
    SettingsRect shortcut_test_mode_value;
    SettingsRect shortcut_test_input_label;
    SettingsRect shortcut_test_input_box;
    SettingsRect shortcut_test_boundary_note;
    SettingsRect appearance_preview_card;
    SettingsRect appearance_theme_card;
    SettingsRect appearance_preview_window;
    SettingsRect candidate_size_slider;
    SettingsRect candidate_size_value;
    SettingsRect candidate_size_label_left;
    SettingsRect candidate_size_label_middle;
    SettingsRect candidate_size_label_right;
    SettingsRect search_box;
    SettingsRect refresh_button;
    SettingsRect builtin_dictionary_button;
    SettingsRect personal_dictionary_title;
    SettingsRect user_table;
    SettingsRect user_empty_state;
    SettingsRect user_editor_card;
    SettingsRect user_editor_title;
    SettingsRect user_frequency_help;
    SettingsRect user_new_button;
    SettingsRect user_save_button;
    SettingsRect user_delete_button;
    SettingsRect user_status;
    SettingsRect about_card;
    std::array<SettingsRect, 7> about_labels{};
    std::array<SettingsRect, 7> about_values{};
    SettingsRect about_open_data_button;
    SettingsRect about_copy_data_button;
    SettingsRect about_dictionary_card;
    SettingsRect about_dictionary_title;
    SettingsRect about_dictionary_count;
    SettingsRect about_dictionary_preview_button;
    SettingsRect about_offline_card;
};

struct CandidateSizeSliderMetrics {
    SettingsRect track;
    std::array<SettingsRect, 7> stops{};
    SettingsRect thumb;
};

struct CandidateSizeSliderDrawState {
    bool hot = false;
    bool pressed = false;
    bool focused = false;
    bool disabled = false;
};

struct CandidateSizeSliderDrawStyle {
    COLORREF background = 0;
    COLORREF track = 0;
    COLORREF track_active = 0;
    COLORREF stop = 0;
    COLORREF stop_active = 0;
    COLORREF thumb = 0;
    COLORREF thumb_border = 0;
    COLORREF focus = 0;
    bool focus_uses_dotted_frame = false;
    bool focus_covers_entire_client_rect = false;
};

struct SettingsSwitchDrawStyle {
    COLORREF track = 0;
    COLORREF track_border = 0;
    COLORREF thumb = 0;
    COLORREF thumb_border = 0;
    bool thumb_on_right = false;
    bool uses_dotted_focus_frame = false;
    bool uses_heavy_focus_ring = false;
};

struct SettingsSwitchMetrics {
    SettingsRect track;
    SettingsRect thumb;
    int track_corner_radius = 0;
    int thumb_corner_radius = 0;
};

struct SettingsVisualSpec {
    int body_font_px = 0;
    int title_font_px = 0;
    int section_font_px = 0;
    int card_corner_radius = 0;
    int control_corner_radius = 0;
    int nav_corner_radius = 0;
    bool parent_paints_cards = true;
    bool parent_uses_double_buffering = true;
    bool owner_draw_buttons = true;
    bool owner_draw_fills_client_rect = true;
    bool static_controls_use_theme_brushes = true;
    bool uses_legacy_group_boxes = false;
    bool uses_full_card_child_windows = false;
    bool edit_uses_client_edge = false;
    bool list_view_uses_grid_lines = false;
    bool user_lexicon_status_inside_editor = true;
    int user_lexicon_status_region_count = 1;
};

enum class SettingsResolvedTheme {
    Light,
    Dark,
};

struct SettingsThemePalette {
    bool dark = false;
    COLORREF window_background = 0;
    COLORREF sidebar_background = 0;
    COLORREF card_background = 0;
    COLORREF card_border = 0;
    COLORREF text = 0;
    COLORREF muted_text = 0;
    COLORREF weak_text = 0;
    COLORREF accent = 0;
    COLORREF accent_soft = 0;
    COLORREF accent_border = 0;
    COLORREF input_background = 0;
    COLORREF input_border = 0;
    COLORREF secondary_button = 0;
    COLORREF secondary_button_pressed = 0;
    COLORREF primary_button_pressed = 0;
    COLORREF danger = 0;
    COLORREF danger_soft = 0;
    COLORREF danger_border = 0;
    COLORREF disabled_background = 0;
    COLORREF disabled_text = 0;
    COLORREF list_header_background = 0;
    COLORREF list_header_text = 0;
    COLORREF inactive_selection = 0;
};

enum class SettingsControlRole {
    Navigation,
    Option,
    PrimaryButton,
    DangerButton,
    SecondaryButton,
};

enum class SettingsControlSurface {
    Window,
    Sidebar,
    Card,
};

struct SettingsControlDrawState {
    bool checked = false;
    bool hot = false;
    bool pressed = false;
    bool disabled = false;
    bool focused = false;
};

struct SettingsControlDrawStyle {
    COLORREF background = 0;
    COLORREF fill = 0;
    COLORREF border = 0;
    COLORREF text = 0;
    COLORREF mark_fill = 0;
    COLORREF mark_border = 0;
    bool fills_entire_client_rect = true;
    bool uses_option_mark = false;
    bool uses_focus_border = false;
};

struct SettingsHeaderDrawStyle {
    COLORREF background = 0;
    COLORREF text = 0;
    COLORREF border = 0;
    COLORREF separator = 0;
    bool fills_entire_client_rect = true;
    bool uses_default_system_background = false;
};

struct SettingsThemeRefreshPlan {
    bool invalidate_window = true;
    bool redraw_child_controls = true;
    bool redraw_list_view = true;
    bool update_title_bar = true;
    bool recreate_controls = false;
    bool reset_user_lexicon_editor = false;
    bool reset_user_lexicon_selection = false;
    bool change_active_page = false;
};

struct SettingsPageVisibilityPolicy {
    bool show_only_active_page_controls = true;
    bool hide_inactive_page_controls = true;
    bool keep_card_surfaces_parent_painted = true;
    bool page_switch_invalidates_content_only = true;
    bool page_switch_erases_background = false;
    bool shortcut_keys_invalidate_only_keycaps = true;
    bool user_lexicon_focus_invalidates_only_input_frame = true;
};

struct SettingsAboutPathPolicy {
    bool uses_pixel_width_compaction = true;
    bool exposes_full_path_with_copy_button = true;
    int data_row_count = 7;
};

struct UserLexiconColumnWidths {
    int pinyin = 0;
    int word = 0;
    int frequency = 0;
};

struct UserLexiconTableRow {
    std::wstring pinyin;
    std::wstring word;
    std::wstring frequency;
};

struct UserLexiconRowVisualStyle {
    COLORREF text = 0;
    COLORREF background = 0;
    bool selected_background = false;
};

struct UserLexiconEditorState {
    std::wstring pinyin;
    std::wstring word;
    std::wstring frequency;
    bool delete_enabled = false;
    bool focus_pinyin = false;
    UserLexiconEditorMode mode = UserLexiconEditorMode::NewEntry;
    std::wstring primary_action_text = L"\u65B0\u589E\u8BCD\u6761";
};

struct ShortcutTestKeyReadings {
    bool left_ctrl_down = false;
    bool right_ctrl_down = false;
    bool space_down = false;
};

struct ShortcutTestKeyState {
    bool ctrl_down = false;
    bool space_down = false;
};

struct SettingsKeyboardState {
    bool left_ctrl_down = false;
    bool right_ctrl_down = false;
};

[[nodiscard]] int scale_settings_value(int value, int dpi);
[[nodiscard]] SettingsSize settings_minimum_client_size(int dpi);
[[nodiscard]] SettingsSize settings_initial_client_size(int dpi);
[[nodiscard]] SettingsSize settings_outer_size_for_client(
    SettingsSize client_size,
    int dpi,
    DWORD style,
    DWORD ex_style);
[[nodiscard]] SettingsRect calculate_settings_initial_window_rect(
    const RECT& work_area,
    int dpi,
    DWORD style,
    DWORD ex_style);
[[nodiscard]] SettingsWindowLayout calculate_settings_window_layout(int client_width, int client_height, int dpi);
[[nodiscard]] SettingsVisualSpec settings_visual_spec(int dpi);
[[nodiscard]] SettingsResolvedTheme resolve_settings_theme(CandidateThemeMode mode, bool system_uses_dark_theme);
[[nodiscard]] SettingsThemePalette settings_theme_palette(SettingsResolvedTheme theme);
[[nodiscard]] SettingsControlDrawStyle settings_control_draw_style(
    const SettingsThemePalette& palette,
    SettingsControlRole role,
    SettingsControlSurface surface,
    const SettingsControlDrawState& state);
[[nodiscard]] SettingsSwitchDrawStyle settings_switch_draw_style(
    const SettingsThemePalette& palette,
    bool checked,
    bool focused);
[[nodiscard]] SettingsSwitchMetrics settings_switch_metrics(
    const SettingsRect& bounds,
    bool checked,
    int dpi);
[[nodiscard]] SettingsHeaderDrawStyle settings_header_draw_style(const SettingsThemePalette& palette);
[[nodiscard]] SettingsThemeRefreshPlan settings_theme_refresh_plan();
[[nodiscard]] SettingsPageVisibilityPolicy settings_page_visibility_policy();
[[nodiscard]] SettingsAboutPathPolicy settings_about_path_policy();
void apply_settings_dark_title_bar(HWND hwnd, bool dark);
void apply_settings_control_theme(HWND hwnd, bool dark);
[[nodiscard]] CandidateSizeSliderMetrics calculate_candidate_size_slider_metrics(
    const SettingsRect& bounds,
    int level,
    int dpi);
[[nodiscard]] int candidate_size_slider_level_from_x(const SettingsRect& bounds, int x, int dpi);
[[nodiscard]] CandidateSizeSliderDrawStyle candidate_size_slider_draw_style(
    const SettingsThemePalette& palette,
    const CandidateSizeSliderDrawState& state);
[[nodiscard]] bool settings_page_control_visible(SettingsPage active_page, SettingsPage owner_page);
[[nodiscard]] SettingsRect settings_page_switch_dirty_rect(const SettingsWindowLayout& layout);
[[nodiscard]] std::array<SettingsRect, 2> shortcut_test_key_dirty_rects(
    const SettingsWindowLayout& layout,
    int dpi);
[[nodiscard]] SettingsRect user_lexicon_input_frame_rect(
    const SettingsWindowLayout& layout,
    UserLexiconInputField field,
    int dpi);
[[nodiscard]] bool settings_keyboard_suppresses_control_activation(
    UINT message,
    WPARAM key,
    SettingsKeyboardState state) noexcept;
[[nodiscard]] UserLexiconColumnWidths calculate_user_lexicon_column_widths(int table_width, int dpi);
[[nodiscard]] UserLexiconRowVisualStyle user_lexicon_row_visual_style(bool selected, bool list_has_focus);
[[nodiscard]] UserLexiconRowVisualStyle user_lexicon_row_visual_style(
    const SettingsThemePalette& palette,
    bool selected,
    bool list_has_focus);
[[nodiscard]] bool settings_rects_overlap(const SettingsRect& left, const SettingsRect& right);
[[nodiscard]] UserLexiconTableRow make_user_lexicon_table_row(const UserLexiconEntry& entry);
[[nodiscard]] UserLexiconEditorState user_lexicon_editor_state_for_selection(
    const std::vector<UserLexiconEntry>& entries,
    const std::vector<size_t>& filtered_indexes,
    int selected_row);
[[nodiscard]] UserLexiconEditorState make_new_user_lexicon_editor_state(bool focus_pinyin);
[[nodiscard]] constexpr ShortcutTestKeyState shortcut_test_key_state_from_readings(
    ShortcutTestKeyReadings readings) noexcept {
    return ShortcutTestKeyState{readings.left_ctrl_down || readings.right_ctrl_down, readings.space_down};
}
[[nodiscard]] constexpr ShortcutTestKeyState shortcut_test_key_state_after_focus_loss() noexcept {
    return ShortcutTestKeyState{};
}
[[nodiscard]] constexpr ShortcutTestKeyState shortcut_test_key_state_after_sampling_stop() noexcept {
    return ShortcutTestKeyState{};
}

class SettingsWindow {
public:
    int run(HINSTANCE instance, int show_command, std::function<void(HWND)> ready = {});

private:
    friend class SettingsWindowCapture;
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam);
    void create_controls();
    void create_fonts();
    void destroy_fonts();
    void apply_fonts();
    void refresh_theme();
    void apply_theme_to_controls();
    void apply_title_bar_theme();
    void destroy_theme_brushes();
    void redraw_theme_children();
    void install_list_header_theme_subclass();
    HBRUSH brush_for_static_control(int id) const;
    HBRUSH brush_for_control_surface(SettingsControlSurface surface) const;
    SettingsControlRole control_role_for_id(int id) const;
    SettingsControlSurface control_surface_for_id(int id) const;
    void paint();
    void render(HDC hdc);
    void install_modern_controls();
    void render_static_control(HWND control, HDC hdc);
    void render_edit_hint(HWND control, HDC hdc, bool printing = false);
    static LRESULT CALLBACK modern_static_subclass_proc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    static LRESULT CALLBACK modern_edit_subclass_proc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    static LRESULT CALLBACK modern_button_subclass_proc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    void draw_button(const DRAWITEMSTRUCT& item);
    bool draw_list_header_item(const NMCUSTOMDRAW& custom_draw);
    void paint_list_header(HWND header, HDC hdc);
    void layout_controls();
    void show_page(SettingsPage page);
    void apply_options_to_controls();
    void sync_candidate_text_size_controls();
    void persist_candidate_display_options();
    void update_candidate_text_size_from_level(int level);
    void invalidate_candidate_size_preview();
    void update_candidate_preview();
    void draw_candidate_size_slider(HDC hdc, const RECT& paint_rect, const CandidateSizeSliderDrawState& state);
    void draw_shortcut_test_keys(HDC hdc, const SettingsWindowLayout& layout);
    void update_shortcut_test_key_state();
    void update_runtime_input_mode_status();
    void clear_shortcut_test_key_state();
    void invalidate_shortcut_test_key_state_rects();
    void invalidate_user_lexicon_input_frame(int control_id);
    void set_shortcut_test_timer_enabled(bool enabled);
    bool shortcut_test_should_sample() const;
    static LRESULT CALLBACK candidate_size_slider_subclass_proc(
        HWND hwnd,
        UINT message,
        WPARAM wparam,
        LPARAM lparam,
        UINT_PTR subclass_id,
        DWORD_PTR ref_data);
    void open_data_dir();
    void copy_data_dir_to_clipboard();
    void open_dictionary_preview();
    void load_dictionary_catalog();
    bool copy_text_to_clipboard(const std::wstring& text, const wchar_t* success_message, const wchar_t* failure_message);
    void clear_learning_data();
    void load_user_lexicon();
    void populate_user_lexicon_list();
    void populate_user_lexicon_editor(size_t entry_index);
    void populate_user_lexicon_editor(const UserLexiconEditorState& state);
    void enter_new_user_lexicon_state(bool focus_pinyin, const wchar_t* status_text = nullptr);
    void enter_edit_user_lexicon_state(size_t entry_index);
    bool select_user_lexicon_entry(const UserLexiconEntry& entry);
    void save_user_lexicon_entry();
    void delete_user_lexicon_entry();
    void refresh_user_lexicon();
    void update_user_lexicon_action_controls();
    int selected_user_lexicon_row() const;
    std::wstring user_lexicon_path() const;
    static LRESULT CALLBACK list_header_subclass_proc(
        HWND hwnd,
        UINT message,
        WPARAM wparam,
        LPARAM lparam,
        UINT_PTR subclass_id,
        DWORD_PTR ref_data);

    HWND hwnd_ = nullptr;
    HINSTANCE instance_ = nullptr;
    std::array<HWND, 5> nav_buttons_{};
    std::array<std::vector<HWND>, 5> page_controls_{};
    HFONT body_font_ = nullptr;
    HFONT title_font_ = nullptr;
    HFONT section_font_ = nullptr;
    HBRUSH window_background_brush_ = nullptr;
    HBRUSH sidebar_background_brush_ = nullptr;
    HBRUSH card_background_brush_ = nullptr;
    HBRUSH border_background_brush_ = nullptr;
    HBRUSH edit_background_brush_ = nullptr;
    HICON app_icon_big_ = nullptr;
    HICON app_icon_small_ = nullptr;
    int dpi_ = 96;
    SettingsThemePalette palette_;
    ModernRenderer renderer_;
    HIMAGELIST lexicon_row_metrics_ = nullptr;
    SettingsPage active_page_ = SettingsPage::Appearance;
    HWND lexicon_search_ = nullptr;
    HWND lexicon_list_ = nullptr;
    HWND lexicon_empty_state_ = nullptr;
    HWND lexicon_pinyin_ = nullptr;
    HWND lexicon_word_ = nullptr;
    HWND lexicon_frequency_ = nullptr;
    HWND candidate_size_slider_ = nullptr;
    HWND candidate_size_value_ = nullptr;
    HWND candidate_preview_input_ = nullptr;
    PinyinEngine candidate_preview_engine_{DictionaryLoadMode::Empty};
    bool candidate_preview_ready_ = false;
    std::wstring candidate_preview_pinyin_;
    std::vector<std::wstring> candidate_preview_texts_;
    CandidateLayoutResult candidate_preview_layout_;
    bool candidate_size_slider_dragging_ = false;
    bool shortcut_test_timer_active_ = false;
    ShortcutTestKeyState shortcut_test_key_state_;
    RuntimeInputModeStatus runtime_input_mode_status_;
    HWND shortcut_test_input_ = nullptr;
    HWND shortcut_mode_value_ = nullptr;
    HWND lexicon_new_button_ = nullptr;
    HWND lexicon_save_button_ = nullptr;
    HWND lexicon_delete_button_ = nullptr;
    HWND lexicon_status_ = nullptr;
    SettingsStore store_;
    CandidateWindowOptions options_;
    InputModeOptions input_mode_options_;
    DictionaryCatalog dictionary_catalog_;
    HWND dictionary_preview_hwnd_ = nullptr;
    std::vector<UserLexiconEntry> user_lexicon_entries_;
    std::vector<size_t> filtered_user_lexicon_indexes_;
};

}  // namespace localpinyin
