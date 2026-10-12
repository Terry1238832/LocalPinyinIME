#include "test_common.h"
#include "../src/engine/user_lexicon.h"
#include "../src/settings/dictionary_preview_window.h"
#include "../src/settings/settings_window.h"

#include <array>
#include <cwctype>
#include <cstdlib>
#include <vector>

namespace {

bool positive_rect(const localpinyin::SettingsRect& rect) {
    return rect.width > 0 && rect.height > 0;
}

bool rect_inside(const localpinyin::SettingsRect& outer, const localpinyin::SettingsRect& inner) {
    return inner.x >= outer.x &&
           inner.y >= outer.y &&
           inner.x + inner.width <= outer.x + outer.width &&
           inner.y + inner.height <= outer.y + outer.height;
}

int rect_bottom(const localpinyin::SettingsRect& rect) {
    return rect.y + rect.height;
}

int rect_right(const localpinyin::SettingsRect& rect) {
    return rect.x + rect.width;
}

int color_distance(COLORREF left, COLORREF right) {
    return std::abs(GetRValue(left) - GetRValue(right)) +
           std::abs(GetGValue(left) - GetGValue(right)) +
           std::abs(GetBValue(left) - GetBValue(right));
}

}  // namespace

int main() {
    using namespace localpinyin;

    const UserLexiconEntry entry{L"nihao", L"\u4F60\u597D", 5000};
    const UserLexiconTableRow row = make_user_lexicon_table_row(entry);
    REQUIRE_EQ(row.pinyin, std::wstring(L"nihao"));
    REQUIRE_EQ(row.word, std::wstring(L"\u4F60\u597D"));
    REQUIRE_EQ(row.frequency, std::wstring(L"5000"));

    std::vector<UserLexiconEntry> entries{
        {L"nihao", L"\u4F60\u597D", 9000},
        {L"xuexi", L"\u5B66\u4E60", 7000},
        {L"gongzuo", L"\u5DE5\u4F5C", 5000},
    };
    const auto pinyin_matches = filter_user_lexicon_entries(entries, L"xue");
    REQUIRE_EQ(pinyin_matches.size(), static_cast<size_t>(1));
    REQUIRE_EQ(pinyin_matches.front(), static_cast<size_t>(1));
    const auto word_matches = filter_user_lexicon_entries(entries, L"\u5DE5");
    REQUIRE_EQ(word_matches.size(), static_cast<size_t>(1));
    REQUIRE_EQ(word_matches.front(), static_cast<size_t>(2));

    const auto selected = user_lexicon_editor_state_for_selection(entries, pinyin_matches, 0);
    REQUIRE_TRUE(selected.delete_enabled);
    REQUIRE_EQ(selected.mode, UserLexiconEditorMode::EditExisting);
    REQUIRE_EQ(selected.primary_action_text, std::wstring(L"\u4FDD\u5B58\u4FEE\u6539"));
    REQUIRE_EQ(selected.pinyin, std::wstring(L"xuexi"));
    REQUIRE_EQ(selected.word, std::wstring(L"\u5B66\u4E60"));
    REQUIRE_EQ(selected.frequency, std::wstring(L"7000"));

    const auto not_selected = user_lexicon_editor_state_for_selection(entries, pinyin_matches, -1);
    REQUIRE_TRUE(!not_selected.delete_enabled);
    REQUIRE_EQ(not_selected.mode, UserLexiconEditorMode::NewEntry);
    REQUIRE_EQ(not_selected.primary_action_text, std::wstring(L"\u65B0\u589E\u8BCD\u6761"));
    REQUIRE_EQ(not_selected.pinyin, std::wstring());
    REQUIRE_EQ(not_selected.word, std::wstring());
    REQUIRE_EQ(not_selected.frequency, std::wstring());

    const auto explicit_new = make_new_user_lexicon_editor_state(true);
    REQUIRE_TRUE(!explicit_new.delete_enabled);
    REQUIRE_TRUE(explicit_new.focus_pinyin);
    REQUIRE_EQ(explicit_new.mode, UserLexiconEditorMode::NewEntry);
    REQUIRE_EQ(explicit_new.primary_action_text, std::wstring(L"\u65B0\u589E\u8BCD\u6761"));
    REQUIRE_EQ(explicit_new.pinyin, std::wstring());
    REQUIRE_EQ(explicit_new.word, std::wstring());
    REQUIRE_EQ(explicit_new.frequency, std::wstring());

    UserLexiconEntry new_entry{L"ceshi", L"\u6D4B\u8BD5", 3000};
    bool updated = false;
    REQUIRE_TRUE(upsert_user_lexicon_entry(entries, new_entry, &updated));
    REQUIRE_TRUE(!updated);
    REQUIRE_EQ(entries.size(), static_cast<size_t>(4));
    new_entry.frequency = 7000;
    REQUIRE_TRUE(upsert_user_lexicon_entry(entries, new_entry, &updated));
    REQUIRE_TRUE(updated);
    REQUIRE_EQ(entries.size(), static_cast<size_t>(4));
    REQUIRE_TRUE(remove_user_lexicon_entry(entries, L"ceshi", L"\u6D4B\u8BD5"));
    const auto after_delete = user_lexicon_editor_state_for_selection(entries, {}, -1);
    REQUIRE_EQ(after_delete.mode, UserLexiconEditorMode::NewEntry);
    REQUIRE_EQ(after_delete.primary_action_text, std::wstring(L"\u65B0\u589E\u8BCD\u6761"));
    REQUIRE_TRUE(!after_delete.delete_enabled);

    REQUIRE_EQ(resolve_settings_theme(CandidateThemeMode::Light, false), SettingsResolvedTheme::Light);
    REQUIRE_EQ(resolve_settings_theme(CandidateThemeMode::Light, true), SettingsResolvedTheme::Light);
    REQUIRE_EQ(resolve_settings_theme(CandidateThemeMode::Dark, false), SettingsResolvedTheme::Dark);
    REQUIRE_EQ(resolve_settings_theme(CandidateThemeMode::Dark, true), SettingsResolvedTheme::Dark);
    REQUIRE_EQ(resolve_settings_theme(CandidateThemeMode::System, false), SettingsResolvedTheme::Light);
    REQUIRE_EQ(resolve_settings_theme(CandidateThemeMode::System, true), SettingsResolvedTheme::Dark);

    const SettingsThemePalette light_palette = settings_theme_palette(SettingsResolvedTheme::Light);
    const SettingsThemePalette dark_palette = settings_theme_palette(SettingsResolvedTheme::Dark);
    REQUIRE_TRUE(!light_palette.dark);
    REQUIRE_TRUE(dark_palette.dark);
    REQUIRE_TRUE(light_palette.window_background != dark_palette.window_background);
    REQUIRE_TRUE(light_palette.card_background != dark_palette.card_background);
    REQUIRE_TRUE(light_palette.text != dark_palette.text);
    REQUIRE_TRUE(dark_palette.input_background != RGB(255, 255, 255));
    REQUIRE_TRUE(dark_palette.list_header_background != RGB(240, 240, 240));
    REQUIRE_TRUE(dark_palette.card_background != dark_palette.text);
    REQUIRE_TRUE(dark_palette.muted_text != dark_palette.card_background);

    const SettingsHeaderDrawStyle dark_header = settings_header_draw_style(dark_palette);
    const SettingsHeaderDrawStyle light_header = settings_header_draw_style(light_palette);
    REQUIRE_TRUE(dark_header.fills_entire_client_rect);
    REQUIRE_TRUE(!dark_header.uses_default_system_background);
    REQUIRE_TRUE(dark_header.background != RGB(255, 255, 255));
    REQUIRE_TRUE(dark_header.background != RGB(240, 240, 240));
    REQUIRE_TRUE(dark_header.separator != RGB(255, 255, 255));
    REQUIRE_TRUE(dark_header.text != dark_header.background);
    REQUIRE_TRUE(color_distance(dark_header.text, dark_header.background) > 240);
    REQUIRE_TRUE(light_header.background != dark_header.background);
    REQUIRE_TRUE(light_header.text != dark_header.text);
    REQUIRE_TRUE(light_header.separator != RGB(255, 255, 255));

    SettingsControlDrawState normal_state;
    const auto dark_nav_normal = settings_control_draw_style(dark_palette,
                                                            SettingsControlRole::Navigation,
                                                            SettingsControlSurface::Sidebar,
                                                            normal_state);
    REQUIRE_TRUE(dark_nav_normal.fills_entire_client_rect);
    REQUIRE_EQ(dark_nav_normal.background, dark_palette.sidebar_background);
    REQUIRE_TRUE(dark_nav_normal.background != RGB(255, 255, 255));
    REQUIRE_TRUE(dark_nav_normal.fill != RGB(255, 255, 255));

    SettingsControlDrawState hot_state;
    hot_state.hot = true;
    const auto dark_nav_hot = settings_control_draw_style(dark_palette,
                                                         SettingsControlRole::Navigation,
                                                         SettingsControlSurface::Sidebar,
                                                         hot_state);
    REQUIRE_TRUE(dark_nav_hot.fill != dark_nav_normal.fill);
    REQUIRE_TRUE(dark_nav_hot.fill != RGB(255, 255, 255));

    SettingsControlDrawState checked_focused_state;
    checked_focused_state.checked = true;
    checked_focused_state.focused = true;
    const auto dark_nav_selected = settings_control_draw_style(dark_palette,
                                                              SettingsControlRole::Navigation,
                                                              SettingsControlSurface::Sidebar,
                                                              checked_focused_state);
    REQUIRE_TRUE(dark_nav_selected.uses_focus_border);
    REQUIRE_EQ(dark_nav_selected.border, dark_palette.accent);
    REQUIRE_TRUE(dark_nav_selected.fill != dark_nav_normal.fill);

    const auto dark_secondary_button = settings_control_draw_style(dark_palette,
                                                                  SettingsControlRole::SecondaryButton,
                                                                  SettingsControlSurface::Card,
                                                                  normal_state);
    REQUIRE_TRUE(dark_secondary_button.fills_entire_client_rect);
    REQUIRE_EQ(dark_secondary_button.background, dark_palette.card_background);
    REQUIRE_TRUE(dark_secondary_button.background != RGB(255, 255, 255));
    REQUIRE_TRUE(dark_secondary_button.fill != RGB(255, 255, 255));

    SettingsControlDrawState pressed_state;
    pressed_state.pressed = true;
    const auto dark_pressed_button = settings_control_draw_style(dark_palette,
                                                               SettingsControlRole::SecondaryButton,
                                                               SettingsControlSurface::Card,
                                                               pressed_state);
    REQUIRE_TRUE(dark_pressed_button.fill != dark_secondary_button.fill);

    SettingsControlDrawState disabled_state;
    disabled_state.disabled = true;
    const auto dark_disabled_button = settings_control_draw_style(dark_palette,
                                                                SettingsControlRole::SecondaryButton,
                                                                SettingsControlSurface::Card,
                                                                disabled_state);
    REQUIRE_TRUE(dark_disabled_button.fill != RGB(255, 255, 255));
    REQUIRE_EQ(dark_disabled_button.text, dark_palette.disabled_text);

    const auto dark_option = settings_control_draw_style(dark_palette,
                                                        SettingsControlRole::Option,
                                                        SettingsControlSurface::Card,
                                                        normal_state);
    REQUIRE_TRUE(dark_option.fills_entire_client_rect);
    REQUIRE_TRUE(dark_option.uses_option_mark);
    REQUIRE_EQ(dark_option.background, dark_palette.card_background);
    REQUIRE_TRUE(dark_option.fill != RGB(255, 255, 255));
    REQUIRE_TRUE(dark_option.mark_fill != RGB(255, 255, 255));

    const auto dark_checked_option = settings_control_draw_style(dark_palette,
                                                                SettingsControlRole::Option,
                                                                SettingsControlSurface::Card,
                                                                checked_focused_state);
    REQUIRE_EQ(dark_checked_option.mark_fill, dark_palette.accent);
    REQUIRE_EQ(dark_checked_option.border, dark_palette.accent);
    REQUIRE_TRUE(dark_checked_option.fill != dark_option.fill);

    const auto light_secondary_button = settings_control_draw_style(light_palette,
                                                                    SettingsControlRole::SecondaryButton,
                                                                    SettingsControlSurface::Card,
                                                                    normal_state);
    REQUIRE_EQ(light_secondary_button.background, light_palette.card_background);
    REQUIRE_TRUE(light_secondary_button.background != dark_secondary_button.background);
    REQUIRE_TRUE(light_secondary_button.fill != dark_secondary_button.fill);

    const SettingsSwitchDrawStyle light_switch_on = settings_switch_draw_style(light_palette, true, true);
    const SettingsSwitchDrawStyle light_switch_off = settings_switch_draw_style(light_palette, false, true);
    const SettingsSwitchDrawStyle dark_switch_off = settings_switch_draw_style(dark_palette, false, false);
    REQUIRE_TRUE(light_switch_on.thumb_on_right);
    REQUIRE_TRUE(!light_switch_off.thumb_on_right);
    REQUIRE_TRUE(!light_switch_on.uses_dotted_focus_frame);
    REQUIRE_TRUE(!light_switch_off.uses_dotted_focus_frame);
    REQUIRE_TRUE(!light_switch_on.uses_heavy_focus_ring);
    REQUIRE_TRUE(!light_switch_off.uses_heavy_focus_ring);
    REQUIRE_EQ(light_switch_on.track, light_palette.accent);
    REQUIRE_TRUE(light_switch_off.track != light_palette.accent);
    REQUIRE_TRUE(light_switch_on.track != light_switch_off.track);
    REQUIRE_TRUE(light_switch_on.thumb != light_switch_on.track);
    REQUIRE_TRUE(light_switch_off.thumb != light_switch_off.track);
    REQUIRE_TRUE(dark_switch_off.track != dark_palette.accent);
    REQUIRE_TRUE(dark_switch_off.thumb != dark_switch_off.track);

    REQUIRE_TRUE(settings_keyboard_suppresses_control_activation(
        WM_KEYDOWN,
        VK_SPACE,
        SettingsKeyboardState{true, false}));
    REQUIRE_TRUE(settings_keyboard_suppresses_control_activation(
        WM_KEYDOWN,
        VK_SPACE,
        SettingsKeyboardState{false, true}));
    REQUIRE_TRUE(settings_keyboard_suppresses_control_activation(
        WM_SYSKEYDOWN,
        VK_SPACE,
        SettingsKeyboardState{true, false}));
    REQUIRE_TRUE(!settings_keyboard_suppresses_control_activation(
        WM_KEYDOWN,
        VK_SPACE,
        SettingsKeyboardState{false, false}));
    REQUIRE_TRUE(!settings_keyboard_suppresses_control_activation(
        WM_KEYDOWN,
        'C',
        SettingsKeyboardState{true, false}));
    REQUIRE_TRUE(!settings_keyboard_suppresses_control_activation(
        WM_KEYDOWN,
        'V',
        SettingsKeyboardState{false, true}));
    REQUIRE_TRUE(!settings_keyboard_suppresses_control_activation(
        WM_KEYDOWN,
        'A',
        SettingsKeyboardState{true, false}));
    REQUIRE_TRUE(!settings_keyboard_suppresses_control_activation(
        WM_KEYUP,
        VK_SPACE,
        SettingsKeyboardState{true, false}));
    bool shift_toggle = false;
    bool show_hints = true;
    int command_count = 0;
    int save_count = 0;
    auto run_switch_space_activation = [&](bool ctrl_space, bool shift_focus) {
        const bool suppressed = settings_keyboard_suppresses_control_activation(
            WM_KEYDOWN,
            VK_SPACE,
            ctrl_space ? SettingsKeyboardState{true, false} : SettingsKeyboardState{false, false});
        if (suppressed) {
            return;
        }
        ++command_count;
        ++save_count;
        if (shift_focus) {
            shift_toggle = !shift_toggle;
        } else {
            show_hints = !show_hints;
        }
    };
    run_switch_space_activation(true, true);
    REQUIRE_TRUE(!shift_toggle);
    REQUIRE_TRUE(show_hints);
    REQUIRE_EQ(command_count, 0);
    REQUIRE_EQ(save_count, 0);
    run_switch_space_activation(true, false);
    REQUIRE_TRUE(!shift_toggle);
    REQUIRE_TRUE(show_hints);
    REQUIRE_EQ(command_count, 0);
    REQUIRE_EQ(save_count, 0);
    run_switch_space_activation(false, true);
    REQUIRE_TRUE(shift_toggle);
    REQUIRE_TRUE(show_hints);
    REQUIRE_EQ(command_count, 1);
    REQUIRE_EQ(save_count, 1);
    std::wstring shortcut_input_text = L"abc";
    std::wstring user_lexicon_input_text = L"ni";
    auto run_input_space = [&](std::wstring& text, bool ctrl_space) {
        if (settings_keyboard_suppresses_control_activation(
                WM_KEYDOWN,
                VK_SPACE,
                ctrl_space ? SettingsKeyboardState{false, true} : SettingsKeyboardState{false, false})) {
            return;
        }
        text.push_back(L' ');
    };
    run_input_space(shortcut_input_text, true);
    run_input_space(user_lexicon_input_text, true);
    REQUIRE_EQ(shortcut_input_text, std::wstring(L"abc"));
    REQUIRE_EQ(user_lexicon_input_text, std::wstring(L"ni"));
    run_input_space(shortcut_input_text, false);
    REQUIRE_EQ(shortcut_input_text, std::wstring(L"abc "));

    CandidateSizeSliderDrawState slider_state;
    slider_state.focused = true;
    const CandidateSizeSliderDrawStyle dark_slider = candidate_size_slider_draw_style(dark_palette, slider_state);
    const CandidateSizeSliderDrawStyle light_slider = candidate_size_slider_draw_style(light_palette, slider_state);
    REQUIRE_TRUE(dark_slider.background != RGB(255, 255, 255));
    REQUIRE_TRUE(dark_slider.track != dark_slider.background);
    REQUIRE_TRUE(dark_slider.thumb != dark_slider.background);
    REQUIRE_TRUE(dark_slider.focus == dark_palette.accent);
    REQUIRE_TRUE(!dark_slider.focus_uses_dotted_frame);
    REQUIRE_TRUE(!dark_slider.focus_covers_entire_client_rect);
    REQUIRE_TRUE(light_slider.background != dark_slider.background);
    REQUIRE_TRUE(!light_slider.focus_uses_dotted_frame);
    REQUIRE_TRUE(!light_slider.focus_covers_entire_client_rect);

    const SettingsThemeRefreshPlan refresh_plan = settings_theme_refresh_plan();
    REQUIRE_TRUE(refresh_plan.invalidate_window);
    REQUIRE_TRUE(refresh_plan.redraw_child_controls);
    REQUIRE_TRUE(refresh_plan.redraw_list_view);
    REQUIRE_TRUE(refresh_plan.update_title_bar);
    REQUIRE_TRUE(!refresh_plan.recreate_controls);
    REQUIRE_TRUE(!refresh_plan.reset_user_lexicon_editor);
    REQUIRE_TRUE(!refresh_plan.reset_user_lexicon_selection);
    REQUIRE_TRUE(!refresh_plan.change_active_page);

    const SettingsPageVisibilityPolicy visibility_policy = settings_page_visibility_policy();
    REQUIRE_TRUE(visibility_policy.show_only_active_page_controls);
    REQUIRE_TRUE(visibility_policy.hide_inactive_page_controls);
    REQUIRE_TRUE(visibility_policy.keep_card_surfaces_parent_painted);
    REQUIRE_TRUE(visibility_policy.page_switch_invalidates_content_only);
    REQUIRE_TRUE(!visibility_policy.page_switch_erases_background);
    REQUIRE_TRUE(visibility_policy.shortcut_keys_invalidate_only_keycaps);
    REQUIRE_TRUE(visibility_policy.user_lexicon_focus_invalidates_only_input_frame);
    REQUIRE_TRUE(settings_page_control_visible(SettingsPage::Candidate, SettingsPage::Candidate));
    REQUIRE_TRUE(!settings_page_control_visible(SettingsPage::Candidate, SettingsPage::About));
    REQUIRE_TRUE(!settings_page_control_visible(SettingsPage::UserLexicon, SettingsPage::Candidate));

    const SettingsAboutPathPolicy about_path_policy = settings_about_path_policy();
    REQUIRE_TRUE(about_path_policy.uses_pixel_width_compaction);
    REQUIRE_TRUE(about_path_policy.exposes_full_path_with_copy_button);
    REQUIRE_EQ(about_path_policy.data_row_count, 7);

    REQUIRE_EQ(std::wstring(kDictionaryPreviewSearchPlaceholder),
               std::wstring(L"\u641C\u7D22\u62FC\u97F3\u6216\u8BCD\u8BED"));

    for (int dpi : std::array<int, 4>{96, 120, 144, 192}) {
        const SettingsSize minimum_client = settings_minimum_client_size(dpi);
        const SettingsSize initial_client = settings_initial_client_size(dpi);
        constexpr DWORD window_style = WS_OVERLAPPEDWINDOW;
        constexpr DWORD window_ex_style = 0;
        const SettingsSize minimum_outer = settings_outer_size_for_client(minimum_client, dpi, window_style, window_ex_style);
        const SettingsSize initial_outer = settings_outer_size_for_client(initial_client, dpi, window_style, window_ex_style);
        REQUIRE_TRUE(minimum_client.width >= scale_settings_value(980, dpi));
        REQUIRE_TRUE(minimum_client.height >= scale_settings_value(700, dpi));
        REQUIRE_TRUE(initial_client.width >= minimum_client.width);
        REQUIRE_TRUE(initial_client.height >= minimum_client.height);
        REQUIRE_TRUE(initial_outer.width > initial_client.width);
        REQUIRE_TRUE(initial_outer.height > initial_client.height);
        REQUIRE_TRUE(initial_outer.width >= minimum_outer.width);
        REQUIRE_TRUE(initial_outer.height >= minimum_outer.height);

        RECT work_area{0, 0, scale_settings_value(1920, dpi), scale_settings_value(1080, dpi)};
        const SettingsRect initial_window = calculate_settings_initial_window_rect(work_area, dpi, window_style, window_ex_style);
        REQUIRE_TRUE(positive_rect(initial_window));
        REQUIRE_TRUE(rect_inside(SettingsRect{0, 0, work_area.right, work_area.bottom}, initial_window));
        REQUIRE_TRUE(initial_window.width >= minimum_outer.width);
        REQUIRE_TRUE(initial_window.height >= minimum_outer.height);

        const int width = minimum_client.width;
        const int height = minimum_client.height;
        const SettingsVisualSpec visual = settings_visual_spec(dpi);
        REQUIRE_TRUE(visual.parent_paints_cards);
        REQUIRE_TRUE(visual.parent_uses_double_buffering);
        REQUIRE_TRUE(visual.owner_draw_buttons);
        REQUIRE_TRUE(visual.owner_draw_fills_client_rect);
        REQUIRE_TRUE(visual.static_controls_use_theme_brushes);
        REQUIRE_TRUE(!visual.uses_legacy_group_boxes);
        REQUIRE_TRUE(!visual.uses_full_card_child_windows);
        REQUIRE_TRUE(!visual.edit_uses_client_edge);
        REQUIRE_TRUE(!visual.list_view_uses_grid_lines);
        REQUIRE_TRUE(visual.user_lexicon_status_inside_editor);
        REQUIRE_EQ(visual.user_lexicon_status_region_count, 1);
        REQUIRE_TRUE(visual.title_font_px > visual.section_font_px);
        REQUIRE_TRUE(visual.section_font_px >= visual.body_font_px);
        REQUIRE_TRUE(visual.card_corner_radius >= scale_settings_value(10, dpi));
        REQUIRE_TRUE(visual.control_corner_radius >= scale_settings_value(8, dpi));

        const SettingsWindowLayout layout = calculate_settings_window_layout(width, height, dpi);
        REQUIRE_TRUE(positive_rect(layout.sidebar));
        REQUIRE_TRUE(positive_rect(layout.content));
        REQUIRE_TRUE(positive_rect(layout.page_title));
        REQUIRE_TRUE(positive_rect(layout.page_description));
        REQUIRE_TRUE(positive_rect(layout.candidate_options_card));
        REQUIRE_TRUE(positive_rect(layout.candidate_hint_title));
        REQUIRE_TRUE(positive_rect(layout.candidate_hint_description));
        REQUIRE_TRUE(positive_rect(layout.candidate_hint_toggle));
        REQUIRE_TRUE(positive_rect(layout.candidate_shift_title));
        REQUIRE_TRUE(positive_rect(layout.candidate_shift_description));
        REQUIRE_TRUE(positive_rect(layout.candidate_shift_toggle));
        REQUIRE_TRUE(positive_rect(layout.candidate_shift_note));
        REQUIRE_TRUE(positive_rect(layout.shortcut_test_card));
        REQUIRE_TRUE(positive_rect(layout.shortcut_test_title));
        REQUIRE_TRUE(positive_rect(layout.shortcut_test_description));
        REQUIRE_TRUE(positive_rect(layout.shortcut_test_ctrl_key));
        REQUIRE_TRUE(positive_rect(layout.shortcut_test_space_key));
        REQUIRE_TRUE(positive_rect(layout.shortcut_test_mode_label));
        REQUIRE_TRUE(positive_rect(layout.shortcut_test_mode_value));
        REQUIRE_TRUE(positive_rect(layout.shortcut_test_input_label));
        REQUIRE_TRUE(positive_rect(layout.shortcut_test_input_box));
        REQUIRE_TRUE(positive_rect(layout.shortcut_test_boundary_note));
        REQUIRE_TRUE(positive_rect(layout.appearance_preview_card));
        REQUIRE_TRUE(positive_rect(layout.appearance_theme_card));
        REQUIRE_TRUE(positive_rect(layout.appearance_preview_window));
        REQUIRE_TRUE(positive_rect(layout.candidate_size_slider));
        REQUIRE_TRUE(positive_rect(layout.candidate_size_value));
        REQUIRE_TRUE(positive_rect(layout.candidate_size_label_left));
        REQUIRE_TRUE(positive_rect(layout.candidate_size_label_middle));
        REQUIRE_TRUE(positive_rect(layout.candidate_size_label_right));
        REQUIRE_TRUE(positive_rect(layout.search_box));
        REQUIRE_TRUE(positive_rect(layout.refresh_button));
        REQUIRE_TRUE(positive_rect(layout.user_table));
        REQUIRE_TRUE(positive_rect(layout.user_editor_card));
        REQUIRE_TRUE(positive_rect(layout.user_editor_title));
        REQUIRE_TRUE(positive_rect(layout.user_frequency_help));
        REQUIRE_TRUE(positive_rect(layout.user_new_button));
        REQUIRE_TRUE(positive_rect(layout.user_save_button));
        REQUIRE_TRUE(positive_rect(layout.user_delete_button));
        REQUIRE_TRUE(positive_rect(layout.user_status));
        REQUIRE_TRUE(positive_rect(layout.about_card));
        REQUIRE_TRUE(positive_rect(layout.about_open_data_button));
        REQUIRE_TRUE(positive_rect(layout.about_copy_data_button));
        REQUIRE_TRUE(rect_inside(SettingsRect{0, 0, width, height}, layout.sidebar));
        REQUIRE_TRUE(rect_inside(SettingsRect{0, 0, width, height}, layout.content));
        REQUIRE_TRUE(rect_inside(layout.content, layout.page_title));
        REQUIRE_TRUE(rect_inside(layout.content, layout.page_description));
        const SettingsRect page_switch_dirty = settings_page_switch_dirty_rect(layout);
        REQUIRE_TRUE(positive_rect(page_switch_dirty));
        REQUIRE_TRUE(rect_inside(layout.content, page_switch_dirty));
        REQUIRE_EQ(page_switch_dirty.x, layout.content.x);
        REQUIRE_EQ(page_switch_dirty.y, layout.content.y);
        REQUIRE_EQ(page_switch_dirty.width, layout.content.width);
        REQUIRE_EQ(page_switch_dirty.height, layout.content.height);
        REQUIRE_TRUE(!settings_rects_overlap(layout.sidebar, page_switch_dirty));
        REQUIRE_TRUE(rect_inside(layout.content, layout.candidate_options_card));
        REQUIRE_TRUE(rect_inside(layout.content, layout.shortcut_test_card));
        REQUIRE_TRUE(rect_inside(layout.candidate_options_card, layout.candidate_hint_title));
        REQUIRE_TRUE(rect_inside(layout.candidate_options_card, layout.candidate_hint_description));
        REQUIRE_TRUE(rect_inside(layout.candidate_options_card, layout.candidate_hint_toggle));
        REQUIRE_TRUE(rect_inside(layout.candidate_options_card, layout.candidate_shift_title));
        REQUIRE_TRUE(rect_inside(layout.candidate_options_card, layout.candidate_shift_description));
        REQUIRE_TRUE(rect_inside(layout.candidate_options_card, layout.candidate_shift_toggle));
        REQUIRE_TRUE(rect_inside(layout.candidate_options_card, layout.candidate_shift_note));
        REQUIRE_EQ(layout.candidate_hint_toggle.width, scale_settings_value(48, dpi));
        REQUIRE_EQ(layout.candidate_hint_toggle.height, scale_settings_value(26, dpi));
        REQUIRE_EQ(layout.candidate_shift_toggle.width, layout.candidate_hint_toggle.width);
        REQUIRE_EQ(layout.candidate_shift_toggle.height, layout.candidate_hint_toggle.height);
        const SettingsSwitchMetrics off_switch_metrics =
            settings_switch_metrics(layout.candidate_shift_toggle, false, dpi);
        const SettingsSwitchMetrics on_switch_metrics =
            settings_switch_metrics(layout.candidate_shift_toggle, true, dpi);
        REQUIRE_TRUE(positive_rect(off_switch_metrics.track));
        REQUIRE_TRUE(positive_rect(off_switch_metrics.thumb));
        REQUIRE_TRUE(positive_rect(on_switch_metrics.thumb));
        REQUIRE_TRUE(rect_inside(layout.candidate_shift_toggle, off_switch_metrics.track));
        REQUIRE_TRUE(rect_inside(off_switch_metrics.track, off_switch_metrics.thumb));
        REQUIRE_TRUE(rect_inside(on_switch_metrics.track, on_switch_metrics.thumb));
        REQUIRE_TRUE(off_switch_metrics.thumb.width < off_switch_metrics.track.height);
        REQUIRE_EQ(off_switch_metrics.thumb.width, off_switch_metrics.thumb.height);
        REQUIRE_EQ(on_switch_metrics.thumb.width, off_switch_metrics.thumb.width);
        REQUIRE_TRUE(off_switch_metrics.thumb.x < on_switch_metrics.thumb.x);
        REQUIRE_TRUE(on_switch_metrics.thumb.x + on_switch_metrics.thumb.width <=
                     on_switch_metrics.track.x + on_switch_metrics.track.width - scale_settings_value(3, dpi));
        REQUIRE_EQ(off_switch_metrics.track_corner_radius, off_switch_metrics.track.height / 2);
        REQUIRE_EQ(off_switch_metrics.thumb_corner_radius, off_switch_metrics.thumb.height / 2);
        REQUIRE_TRUE(rect_inside(layout.shortcut_test_card, layout.shortcut_test_title));
        REQUIRE_TRUE(rect_inside(layout.shortcut_test_card, layout.shortcut_test_description));
        REQUIRE_TRUE(rect_inside(layout.shortcut_test_card, layout.shortcut_test_ctrl_key));
        REQUIRE_TRUE(rect_inside(layout.shortcut_test_card, layout.shortcut_test_space_key));
        REQUIRE_TRUE(rect_inside(layout.shortcut_test_card, layout.shortcut_test_mode_label));
        REQUIRE_TRUE(rect_inside(layout.shortcut_test_card, layout.shortcut_test_mode_value));
        REQUIRE_TRUE(rect_inside(layout.shortcut_test_card, layout.shortcut_test_input_label));
        REQUIRE_TRUE(rect_inside(layout.shortcut_test_card, layout.shortcut_test_input_box));
        REQUIRE_TRUE(rect_inside(layout.shortcut_test_card, layout.shortcut_test_boundary_note));
        REQUIRE_TRUE(rect_inside(layout.content, layout.appearance_preview_card));
        REQUIRE_TRUE(rect_inside(layout.content, layout.appearance_theme_card));
        REQUIRE_TRUE(rect_inside(layout.appearance_preview_card, layout.appearance_preview_window));
        REQUIRE_TRUE(rect_inside(layout.appearance_theme_card, layout.candidate_size_slider));
        REQUIRE_TRUE(rect_inside(layout.appearance_theme_card, layout.candidate_size_value));
        REQUIRE_TRUE(rect_inside(layout.appearance_theme_card, layout.candidate_size_label_left));
        REQUIRE_TRUE(rect_inside(layout.appearance_theme_card, layout.candidate_size_label_middle));
        REQUIRE_TRUE(rect_inside(layout.appearance_theme_card, layout.candidate_size_label_right));
        REQUIRE_TRUE(rect_inside(layout.content, layout.search_box));
        REQUIRE_TRUE(rect_inside(layout.content, layout.refresh_button));
        REQUIRE_TRUE(rect_inside(layout.content, layout.user_table));
        REQUIRE_TRUE(rect_right(layout.user_table) <= rect_right(layout.content));
        REQUIRE_TRUE(rect_right(layout.search_box) <= rect_right(layout.content));
        REQUIRE_TRUE(rect_right(layout.refresh_button) <= rect_right(layout.content));
        REQUIRE_TRUE(rect_inside(layout.content, layout.user_editor_card));
        REQUIRE_TRUE(rect_right(layout.user_editor_card) <= rect_right(layout.content));
        REQUIRE_TRUE(rect_inside(layout.user_editor_card, layout.user_editor_title));
        REQUIRE_TRUE(rect_inside(layout.user_editor_card, layout.user_frequency_help));
        REQUIRE_TRUE(rect_inside(layout.user_editor_card, layout.user_new_button));
        REQUIRE_TRUE(rect_inside(layout.user_editor_card, layout.user_save_button));
        REQUIRE_TRUE(rect_inside(layout.user_editor_card, layout.user_delete_button));
        REQUIRE_TRUE(rect_inside(layout.user_editor_card, layout.user_status));
        REQUIRE_TRUE(rect_inside(layout.content, layout.about_card));
        REQUIRE_TRUE(rect_inside(layout.content, layout.about_dictionary_card));
        REQUIRE_TRUE(rect_inside(layout.content, layout.about_offline_card));
        REQUIRE_TRUE(!settings_rects_overlap(layout.about_dictionary_card, layout.about_offline_card));
        REQUIRE_TRUE(rect_inside(layout.about_dictionary_card, layout.about_dictionary_title));
        REQUIRE_TRUE(rect_inside(layout.about_dictionary_card, layout.about_dictionary_count));
        REQUIRE_TRUE(rect_inside(layout.about_dictionary_card, layout.about_dictionary_preview_button));
        REQUIRE_TRUE(rect_inside(layout.about_card, layout.about_open_data_button));
        REQUIRE_TRUE(rect_inside(layout.about_card, layout.about_copy_data_button));
        REQUIRE_TRUE(rect_right(layout.about_card) <= rect_right(layout.content));
        REQUIRE_TRUE(rect_right(layout.about_open_data_button) <= rect_right(layout.about_card));
        REQUIRE_TRUE(rect_right(layout.about_copy_data_button) <= rect_right(layout.about_card));
        REQUIRE_TRUE(!settings_rects_overlap(layout.search_box, layout.refresh_button));
        REQUIRE_TRUE(!settings_rects_overlap(layout.candidate_options_card, layout.shortcut_test_card));
        REQUIRE_TRUE(!settings_rects_overlap(layout.candidate_hint_title, layout.candidate_shift_title));
        REQUIRE_TRUE(!settings_rects_overlap(layout.candidate_hint_description, layout.candidate_shift_description));
        REQUIRE_TRUE(!settings_rects_overlap(layout.candidate_hint_toggle, layout.candidate_shift_toggle));
        REQUIRE_TRUE(!settings_rects_overlap(layout.shortcut_test_description, layout.shortcut_test_ctrl_key));
        REQUIRE_TRUE(!settings_rects_overlap(layout.shortcut_test_ctrl_key, layout.shortcut_test_space_key));
        REQUIRE_TRUE(!settings_rects_overlap(layout.shortcut_test_space_key, layout.shortcut_test_mode_label));
        REQUIRE_TRUE(!settings_rects_overlap(layout.shortcut_test_mode_label, layout.shortcut_test_mode_value));
        REQUIRE_TRUE(!settings_rects_overlap(layout.shortcut_test_ctrl_key, layout.shortcut_test_input_label));
        REQUIRE_TRUE(!settings_rects_overlap(layout.shortcut_test_input_label, layout.shortcut_test_input_box));
        REQUIRE_TRUE(!settings_rects_overlap(layout.shortcut_test_input_box, layout.shortcut_test_boundary_note));
        REQUIRE_TRUE(layout.candidate_options_card.width <= scale_settings_value(760, dpi));
        REQUIRE_TRUE(layout.shortcut_test_card.width == layout.candidate_options_card.width);
        REQUIRE_TRUE(layout.shortcut_test_ctrl_key.width == layout.shortcut_test_space_key.width);
        REQUIRE_TRUE(layout.shortcut_test_ctrl_key.height == layout.shortcut_test_space_key.height);
        REQUIRE_TRUE(rect_right(layout.shortcut_test_mode_value) <= rect_right(layout.shortcut_test_card) - scale_settings_value(20, dpi));
        const auto key_dirty_rects = shortcut_test_key_dirty_rects(layout, dpi);
        REQUIRE_EQ(key_dirty_rects.size(), static_cast<size_t>(2));
        REQUIRE_TRUE(positive_rect(key_dirty_rects[0]));
        REQUIRE_TRUE(positive_rect(key_dirty_rects[1]));
        REQUIRE_TRUE(rect_inside(layout.shortcut_test_card, key_dirty_rects[0]));
        REQUIRE_TRUE(rect_inside(layout.shortcut_test_card, key_dirty_rects[1]));
        REQUIRE_TRUE(settings_rects_overlap(key_dirty_rects[0], layout.shortcut_test_ctrl_key));
        REQUIRE_TRUE(settings_rects_overlap(key_dirty_rects[1], layout.shortcut_test_space_key));
        REQUIRE_TRUE(!settings_rects_overlap(key_dirty_rects[0], layout.shortcut_test_space_key));
        REQUIRE_TRUE(!settings_rects_overlap(key_dirty_rects[1], layout.shortcut_test_ctrl_key));
        REQUIRE_TRUE(!settings_rects_overlap(key_dirty_rects[0], layout.shortcut_test_input_box));
        REQUIRE_TRUE(!settings_rects_overlap(key_dirty_rects[1], layout.shortcut_test_input_box));
        REQUIRE_TRUE(!settings_rects_overlap(layout.sidebar, key_dirty_rects[0]));
        REQUIRE_TRUE(!settings_rects_overlap(layout.sidebar, key_dirty_rects[1]));
        REQUIRE_TRUE(key_dirty_rects[0].width < layout.shortcut_test_card.width / 3);
        REQUIRE_TRUE(key_dirty_rects[1].width < layout.shortcut_test_card.width / 3);
        REQUIRE_TRUE(!settings_rects_overlap(layout.appearance_preview_card, layout.appearance_theme_card));
        REQUIRE_TRUE(!settings_rects_overlap(layout.candidate_size_slider, layout.candidate_size_value));
        REQUIRE_TRUE(!settings_rects_overlap(layout.user_table, layout.user_editor_card));
        REQUIRE_TRUE(!settings_rects_overlap(layout.user_editor_title, layout.user_status));
        REQUIRE_TRUE(!settings_rects_overlap(layout.user_editor_title, layout.user_frequency_help));
        REQUIRE_TRUE(!settings_rects_overlap(layout.user_frequency_help, layout.user_new_button));
        REQUIRE_TRUE(!settings_rects_overlap(layout.user_frequency_help, layout.user_save_button));
        REQUIRE_TRUE(!settings_rects_overlap(layout.user_frequency_help, layout.user_delete_button));
        REQUIRE_TRUE(!settings_rects_overlap(layout.user_new_button, layout.user_status));
        REQUIRE_TRUE(!settings_rects_overlap(layout.user_save_button, layout.user_status));
        REQUIRE_TRUE(!settings_rects_overlap(layout.user_delete_button, layout.user_status));
        REQUIRE_TRUE(!settings_rects_overlap(layout.user_frequency_help, layout.user_status));
        REQUIRE_TRUE(!settings_rects_overlap(layout.about_open_data_button, layout.about_copy_data_button));
        REQUIRE_TRUE(!settings_rects_overlap(layout.about_card, layout.about_dictionary_card));
        REQUIRE_TRUE(!settings_rects_overlap(layout.about_dictionary_count, layout.about_dictionary_preview_button));
        REQUIRE_TRUE(layout.user_status.y > layout.user_editor_title.y);
        REQUIRE_TRUE(layout.user_status.y >= rect_bottom(layout.user_new_button) + scale_settings_value(12, dpi));
        REQUIRE_TRUE(layout.user_new_button.height == layout.user_save_button.height);
        REQUIRE_TRUE(layout.user_save_button.height == layout.user_delete_button.height);
        REQUIRE_TRUE(layout.user_new_button.y == layout.user_save_button.y);
        REQUIRE_TRUE(layout.user_save_button.y == layout.user_delete_button.y);
        const SettingsRect pinyin_frame =
            user_lexicon_input_frame_rect(layout, UserLexiconInputField::Pinyin, dpi);
        const SettingsRect word_frame =
            user_lexicon_input_frame_rect(layout, UserLexiconInputField::Word, dpi);
        const SettingsRect frequency_frame =
            user_lexicon_input_frame_rect(layout, UserLexiconInputField::Frequency, dpi);
        REQUIRE_TRUE(positive_rect(pinyin_frame));
        REQUIRE_TRUE(positive_rect(word_frame));
        REQUIRE_TRUE(positive_rect(frequency_frame));
        REQUIRE_TRUE(rect_inside(layout.user_editor_card, pinyin_frame));
        REQUIRE_TRUE(rect_inside(layout.user_editor_card, word_frame));
        REQUIRE_TRUE(rect_inside(layout.user_editor_card, frequency_frame));
        REQUIRE_TRUE(!settings_rects_overlap(pinyin_frame, word_frame));
        REQUIRE_TRUE(!settings_rects_overlap(word_frame, frequency_frame));
        REQUIRE_TRUE(!settings_rects_overlap(pinyin_frame, frequency_frame));
        REQUIRE_TRUE(!settings_rects_overlap(pinyin_frame, layout.user_new_button));
        REQUIRE_TRUE(!settings_rects_overlap(word_frame, layout.user_save_button));
        REQUIRE_TRUE(!settings_rects_overlap(frequency_frame, layout.user_delete_button));
        REQUIRE_TRUE(pinyin_frame.width < layout.user_editor_card.width / 2);
        REQUIRE_TRUE(word_frame.width < layout.user_editor_card.width);
        REQUIRE_TRUE(frequency_frame.width < layout.user_editor_card.width / 3);
        REQUIRE_TRUE(layout.about_card.height > layout.primary_card.height);
        REQUIRE_TRUE(layout.about_open_data_button.y > layout.about_values[6].y);
        REQUIRE_TRUE(layout.about_copy_data_button.y == layout.about_open_data_button.y);
        for (size_t i = 0; i < layout.about_labels.size(); ++i) {
            REQUIRE_TRUE(positive_rect(layout.about_labels[i]));
            REQUIRE_TRUE(positive_rect(layout.about_values[i]));
            REQUIRE_TRUE(rect_inside(layout.about_card, layout.about_labels[i]));
            REQUIRE_TRUE(rect_inside(layout.about_card, layout.about_values[i]));
            REQUIRE_TRUE(!settings_rects_overlap(layout.about_labels[i], layout.about_values[i]));
            if (i > 0) {
                REQUIRE_TRUE(rect_bottom(layout.about_values[i - 1]) <= layout.about_values[i].y);
                REQUIRE_TRUE(!settings_rects_overlap(layout.about_labels[i - 1], layout.about_labels[i]));
                REQUIRE_TRUE(!settings_rects_overlap(layout.about_values[i - 1], layout.about_values[i]));
            }
        }
        REQUIRE_TRUE(!settings_rects_overlap(layout.about_values[6], layout.about_open_data_button));
        REQUIRE_TRUE(!settings_rects_overlap(layout.about_values[6], layout.about_copy_data_button));
        const UserLexiconColumnWidths columns = calculate_user_lexicon_column_widths(layout.user_table.width, dpi);
        const int total_columns = columns.pinyin + columns.word + columns.frequency;
        REQUIRE_TRUE(columns.pinyin > 0);
        REQUIRE_TRUE(columns.word > columns.pinyin);
        REQUIRE_TRUE(columns.frequency > 0);
        REQUIRE_TRUE(total_columns <= layout.user_table.width);
        REQUIRE_TRUE(columns.pinyin * 100 / total_columns >= 20);
        REQUIRE_TRUE(columns.word * 100 / total_columns >= 45);
        REQUIRE_TRUE(columns.frequency * 100 / total_columns >= 15);
        for (const SettingsRect& nav_item : layout.nav_items) {
            REQUIRE_TRUE(positive_rect(nav_item));
            REQUIRE_TRUE(rect_inside(layout.sidebar, nav_item));
        }

        const CandidateSizeSliderMetrics slider_metrics =
            calculate_candidate_size_slider_metrics(layout.candidate_size_slider, 5, dpi);
        REQUIRE_TRUE(positive_rect(slider_metrics.track));
        REQUIRE_TRUE(positive_rect(slider_metrics.thumb));
        REQUIRE_TRUE(rect_inside(layout.candidate_size_slider, slider_metrics.thumb));
        REQUIRE_EQ(candidate_size_slider_level_from_x(layout.candidate_size_slider,
                                                      slider_metrics.stops.front().x,
                                                      dpi),
                   1);
        REQUIRE_EQ(candidate_size_slider_level_from_x(layout.candidate_size_slider,
                                                      rect_right(slider_metrics.stops.back()),
                                                      dpi),
                   7);
        for (const SettingsRect& stop : slider_metrics.stops) {
            REQUIRE_TRUE(positive_rect(stop));
            REQUIRE_TRUE(rect_inside(layout.candidate_size_slider, stop));
        }
    }

    const ShortcutTestKeyState left_ctrl = shortcut_test_key_state_from_readings(
        ShortcutTestKeyReadings{true, false, false});
    REQUIRE_TRUE(left_ctrl.ctrl_down);
    REQUIRE_TRUE(!left_ctrl.space_down);
    const ShortcutTestKeyState right_ctrl_space = shortcut_test_key_state_from_readings(
        ShortcutTestKeyReadings{false, true, true});
    REQUIRE_TRUE(right_ctrl_space.ctrl_down);
    REQUIRE_TRUE(right_ctrl_space.space_down);
    const ShortcutTestKeyState cleared = shortcut_test_key_state_after_focus_loss();
    REQUIRE_TRUE(!cleared.ctrl_down);
    REQUIRE_TRUE(!cleared.space_down);
    const ShortcutTestKeyState stopped_sampling = shortcut_test_key_state_after_sampling_stop();
    REQUIRE_TRUE(!stopped_sampling.ctrl_down);
    REQUIRE_TRUE(!stopped_sampling.space_down);

    const UserLexiconRowVisualStyle normal_row = user_lexicon_row_visual_style(false, false);
    const UserLexiconRowVisualStyle focused_selected_row = user_lexicon_row_visual_style(true, true);
    const UserLexiconRowVisualStyle inactive_selected_row = user_lexicon_row_visual_style(true, false);
    REQUIRE_TRUE(!normal_row.selected_background);
    REQUIRE_TRUE(focused_selected_row.selected_background);
    REQUIRE_TRUE(inactive_selected_row.selected_background);
    REQUIRE_TRUE(normal_row.background != focused_selected_row.background);
    REQUIRE_TRUE(normal_row.background != inactive_selected_row.background);
    REQUIRE_TRUE(focused_selected_row.background != inactive_selected_row.background);
    REQUIRE_EQ(normal_row.text, focused_selected_row.text);
    REQUIRE_EQ(normal_row.text, inactive_selected_row.text);

    const UserLexiconRowVisualStyle dark_normal_row = user_lexicon_row_visual_style(dark_palette, false, false);
    const UserLexiconRowVisualStyle dark_focused_selected_row = user_lexicon_row_visual_style(dark_palette, true, true);
    const UserLexiconRowVisualStyle dark_inactive_selected_row = user_lexicon_row_visual_style(dark_palette, true, false);
    REQUIRE_TRUE(!dark_normal_row.selected_background);
    REQUIRE_TRUE(dark_focused_selected_row.selected_background);
    REQUIRE_TRUE(dark_inactive_selected_row.selected_background);
    REQUIRE_TRUE(dark_normal_row.background != RGB(255, 255, 255));
    REQUIRE_TRUE(dark_normal_row.background != dark_focused_selected_row.background);
    REQUIRE_TRUE(dark_normal_row.background != dark_inactive_selected_row.background);
    REQUIRE_TRUE(dark_focused_selected_row.background != dark_inactive_selected_row.background);
    REQUIRE_EQ(dark_normal_row.text, dark_focused_selected_row.text);
    REQUIRE_EQ(dark_normal_row.text, dark_inactive_selected_row.text);

    return 0;
}
