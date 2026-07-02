#pragma once

#include "../engine/user_lexicon.h"
#include "settings_store.h"

#include <windows.h>
#include <commctrl.h>

#include <array>
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
    SettingsRect search_box;
    SettingsRect refresh_button;
    SettingsRect user_table;
    SettingsRect user_empty_state;
    SettingsRect user_editor_card;
    SettingsRect user_editor_title;
    SettingsRect user_status;
    SettingsRect about_card;
    std::array<SettingsRect, 7> about_labels{};
    std::array<SettingsRect, 7> about_values{};
    SettingsRect about_open_data_button;
    SettingsRect about_copy_data_button;
    SettingsRect about_open_github_button;
    SettingsRect about_copy_github_button;
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
[[nodiscard]] SettingsHeaderDrawStyle settings_header_draw_style(const SettingsThemePalette& palette);
[[nodiscard]] SettingsThemeRefreshPlan settings_theme_refresh_plan();
[[nodiscard]] SettingsAboutPathPolicy settings_about_path_policy();
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

class SettingsWindow {
public:
    int run(HINSTANCE instance, int show_command);

private:
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
    void draw_button(const DRAWITEMSTRUCT& item);
    bool draw_list_header_item(const NMCUSTOMDRAW& custom_draw);
    void paint_list_header(HWND header, HDC hdc);
    void layout_controls();
    void show_page(SettingsPage page);
    void apply_options_to_controls();
    void save_options_from_controls();
    void open_data_dir();
    void copy_data_dir_to_clipboard();
    void open_github_project();
    void copy_github_url_to_clipboard();
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
    int dpi_ = 96;
    SettingsThemePalette palette_;
    SettingsPage active_page_ = SettingsPage::UserLexicon;
    HWND lexicon_search_ = nullptr;
    HWND lexicon_list_ = nullptr;
    HWND lexicon_empty_state_ = nullptr;
    HWND lexicon_pinyin_ = nullptr;
    HWND lexicon_word_ = nullptr;
    HWND lexicon_frequency_ = nullptr;
    HWND lexicon_new_button_ = nullptr;
    HWND lexicon_save_button_ = nullptr;
    HWND lexicon_delete_button_ = nullptr;
    HWND lexicon_status_ = nullptr;
    SettingsStore store_;
    CandidateWindowOptions options_;
    std::vector<UserLexiconEntry> user_lexicon_entries_;
    std::vector<size_t> filtered_user_lexicon_indexes_;
};

}  // namespace localpinyin
