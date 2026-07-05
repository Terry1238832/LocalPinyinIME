#include "test_common.h"
#include "../src/engine/candidate_selection.h"
#include "../src/ime/input_mode.h"
#include "../src/ime/preserved_key.h"

int main() {
    REQUIRE_EQ(localpinyin::candidate_index_from_digit_key('1'), 0);
    REQUIRE_EQ(localpinyin::candidate_index_from_digit_key('9'), 8);
    REQUIRE_EQ(localpinyin::candidate_index_from_digit_key('0'), -1);

    REQUIRE_TRUE(localpinyin::can_select_candidate_by_digit('3', 5, true, true));
    REQUIRE_TRUE(!localpinyin::can_select_candidate_by_digit('6', 5, true, true));
    REQUIRE_TRUE(!localpinyin::can_select_candidate_by_digit('1', 5, false, true));
    REQUIRE_TRUE(!localpinyin::can_select_candidate_by_digit('1', 5, true, false));

    REQUIRE_EQ(localpinyin::composition_commit_action_for_key(' ', 3, true),
               localpinyin::CompositionCommitAction::CommitSelectedCandidate);
    REQUIRE_EQ(localpinyin::composition_commit_action_for_key(0x0D, 3, true),
               localpinyin::CompositionCommitAction::CommitRawComposition);
    REQUIRE_EQ(localpinyin::composition_commit_action_for_key(0x0D, 0, true),
               localpinyin::CompositionCommitAction::PassThrough);
    REQUIRE_EQ(localpinyin::composition_commit_action_for_key(0x0D, 3, false),
               localpinyin::CompositionCommitAction::PassThrough);
    REQUIRE_TRUE(localpinyin::should_eat_composition_commit_key(' ', 3, true));
    REQUIRE_TRUE(localpinyin::should_eat_composition_commit_key(0x0D, 3, true));
    REQUIRE_TRUE(!localpinyin::should_eat_composition_commit_key(0x0D, 0, true));
    REQUIRE_TRUE(!localpinyin::should_eat_composition_commit_key(0x0D, 3, false));
    REQUIRE_EQ(localpinyin::candidate_commit_index_for_key(' ', 5, true, 2), 2);
    REQUIRE_EQ(localpinyin::candidate_commit_index_for_key(' ', 5, true, 99), 0);
    REQUIRE_EQ(localpinyin::candidate_commit_index_for_key(0x0D, 5, true, 2), -1);
    REQUIRE_EQ(localpinyin::candidate_commit_index_for_key(0x0D, 0, true, 0), -1);
    REQUIRE_EQ(localpinyin::candidate_commit_index_for_key(0x0D, 5, false, 0), -1);

    localpinyin::ModifierKeyState none{};
    localpinyin::ModifierKeyState ctrl{};
    ctrl.ctrl = true;
    REQUIRE_TRUE(localpinyin::is_ctrl_space(' ', ctrl));
    REQUIRE_TRUE(!localpinyin::is_ctrl_space('A', ctrl));
    REQUIRE_TRUE(!localpinyin::is_ctrl_space(' ', none));

    localpinyin::CtrlSpaceToggleTracker ctrl_space_tracker;
    REQUIRE_TRUE(localpinyin::ctrl_space_toggle_should_fire(ctrl_space_tracker, true));
    REQUIRE_TRUE(ctrl_space_tracker.pending_release);
    REQUIRE_TRUE(!localpinyin::ctrl_space_toggle_should_fire(ctrl_space_tracker, true));
    localpinyin::ctrl_space_toggle_on_key_up(ctrl_space_tracker, VK_SPACE);
    REQUIRE_TRUE(!ctrl_space_tracker.pending_release);
    REQUIRE_TRUE(localpinyin::ctrl_space_toggle_should_fire(ctrl_space_tracker, true));
    localpinyin::ctrl_space_toggle_on_key_up(ctrl_space_tracker, VK_RETURN);
    REQUIRE_TRUE(ctrl_space_tracker.pending_release);
    localpinyin::ctrl_space_toggle_on_key_up(ctrl_space_tracker, VK_SPACE);
    REQUIRE_TRUE(!ctrl_space_tracker.pending_release);
    REQUIRE_TRUE(!localpinyin::ctrl_space_toggle_should_fire(ctrl_space_tracker, false));

    localpinyin::ModifierKeyState ctrl_shift{};
    ctrl_shift.ctrl = true;
    ctrl_shift.shift = true;
    REQUIRE_TRUE(!localpinyin::is_ctrl_space(' ', ctrl_shift));

    localpinyin::ModifierKeyState ctrl_alt{};
    ctrl_alt.ctrl = true;
    ctrl_alt.alt = true;
    REQUIRE_TRUE(!localpinyin::is_ctrl_space(' ', ctrl_alt));

    const TF_PRESERVEDKEY ctrl_space_key = localpinyin::ctrl_space_preserved_key();
    REQUIRE_EQ(ctrl_space_key.uVKey, static_cast<UINT>(VK_SPACE));
    REQUIRE_EQ(ctrl_space_key.uModifiers, static_cast<UINT>(TF_MOD_CONTROL));
    REQUIRE_TRUE(localpinyin::is_ctrl_space_preserved_key(ctrl_space_key));
    REQUIRE_TRUE(!localpinyin::is_ctrl_space_preserved_key(TF_PRESERVEDKEY{VK_SPACE, TF_MOD_CONTROL | TF_MOD_SHIFT}));
    REQUIRE_TRUE(!localpinyin::is_ctrl_space_preserved_key(TF_PRESERVEDKEY{VK_SPACE, TF_MOD_CONTROL | TF_MOD_ALT}));
    REQUIRE_TRUE(!localpinyin::is_ctrl_space_preserved_key(TF_PRESERVEDKEY{VK_RETURN, TF_MOD_CONTROL}));
    REQUIRE_EQ(localpinyin::preserved_key_action_for_guid(localpinyin::GUID_LocalPinyinCtrlSpacePreservedKey),
               localpinyin::PreservedKeyAction::ToggleInputMode);
    REQUIRE_EQ(localpinyin::preserved_key_action_for_guid(GUID_NULL),
               localpinyin::PreservedKeyAction::Ignore);

    REQUIRE_EQ(localpinyin::toggled_input_mode(localpinyin::InputMode::Chinese), localpinyin::InputMode::English);
    REQUIRE_EQ(localpinyin::toggled_input_mode(localpinyin::InputMode::English), localpinyin::InputMode::Chinese);
    REQUIRE_TRUE(localpinyin::should_eat_backspace_for_composition(localpinyin::InputMode::Chinese, true, false));
    REQUIRE_TRUE(!localpinyin::should_eat_backspace_for_composition(localpinyin::InputMode::Chinese, false, false));
    REQUIRE_TRUE(!localpinyin::should_eat_backspace_for_composition(localpinyin::InputMode::English, true, false));
    REQUIRE_TRUE(!localpinyin::should_eat_backspace_for_composition(localpinyin::InputMode::Chinese, true, true));

    REQUIRE_TRUE(localpinyin::can_start_shift_toggle(0x10, none, true));
    REQUIRE_TRUE(localpinyin::can_start_shift_toggle(0xA0, none, true));
    REQUIRE_TRUE(localpinyin::can_start_shift_toggle(0xA1, none, true));
    REQUIRE_TRUE(!localpinyin::can_start_shift_toggle(0x10, ctrl, true));
    REQUIRE_TRUE(!localpinyin::can_start_shift_toggle(0x10, none, false));

    localpinyin::ShiftToggleTracker tracker;
    localpinyin::shift_toggle_on_key_down(tracker, 0x10, none, true);
    REQUIRE_TRUE(tracker.pending);
    REQUIRE_TRUE(localpinyin::shift_toggle_on_key_up(tracker, 0x10, none, true));
    REQUIRE_TRUE(!tracker.pending);

    localpinyin::shift_toggle_on_key_down(tracker, 0x10, none, true);
    localpinyin::shift_toggle_on_key_down(tracker, 'A', none, true);
    REQUIRE_TRUE(!localpinyin::shift_toggle_on_key_up(tracker, 0x10, none, true));

    localpinyin::shift_toggle_on_key_down(tracker, 0x10, ctrl, true);
    REQUIRE_TRUE(!tracker.pending);
    return 0;
}
