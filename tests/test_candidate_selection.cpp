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
    localpinyin::ctrl_space_toggle_on_key_up(ctrl_space_tracker, VK_SPACE, true, false);
    REQUIRE_TRUE(ctrl_space_tracker.pending_release);
    localpinyin::ctrl_space_toggle_on_key_up(ctrl_space_tracker, VK_CONTROL, false, false);
    REQUIRE_TRUE(!ctrl_space_tracker.pending_release);
    REQUIRE_TRUE(!ctrl_space_tracker.toggled_this_chord);
    REQUIRE_TRUE(!localpinyin::ctrl_space_toggle_should_fire(ctrl_space_tracker, false));

    localpinyin::CtrlSpaceToggleTracker fallback_tracker;
    localpinyin::ctrl_space_chord_on_key_down(fallback_tracker, VK_LCONTROL);
    const auto left_ctrl_decision = localpinyin::ctrl_space_keyup_fallback_decision(
        fallback_tracker,
        VK_SPACE,
        localpinyin::modifier_key_state_from_readings(localpinyin::ModifierKeyReadings{false, true, false}));
    REQUIRE_TRUE(left_ctrl_decision.allowed);
    REQUIRE_TRUE(localpinyin::ctrl_space_toggle_should_fire(fallback_tracker, true));
    REQUIRE_TRUE(fallback_tracker.pending_release);
    REQUIRE_TRUE(fallback_tracker.toggled_this_chord);
    REQUIRE_TRUE(!localpinyin::ctrl_space_keyup_fallback_decision(
        fallback_tracker,
        VK_SPACE,
        localpinyin::modifier_key_state_from_readings(localpinyin::ModifierKeyReadings{false, true, false})).allowed);

    localpinyin::CtrlSpaceToggleTracker right_ctrl_fallback;
    localpinyin::ctrl_space_chord_on_key_down(right_ctrl_fallback, VK_RCONTROL);
    REQUIRE_TRUE(localpinyin::ctrl_space_keyup_fallback_decision(
        right_ctrl_fallback,
        VK_SPACE,
        localpinyin::modifier_key_state_from_readings(localpinyin::ModifierKeyReadings{false, false, true})).allowed);

    localpinyin::CtrlSpaceToggleTracker left_ctrl_released_first;
    localpinyin::ctrl_space_chord_on_key_down(left_ctrl_released_first, VK_LCONTROL);
    const auto left_latch_decision = localpinyin::ctrl_space_chord_on_ctrl_key_up(
        left_ctrl_released_first,
        VK_LCONTROL,
        none,
        true);
    REQUIRE_TRUE(left_latch_decision.allowed);
    REQUIRE_TRUE(left_ctrl_released_first.ctrl_released_while_space_down);
    const auto left_released_fallback = localpinyin::ctrl_space_keyup_fallback_decision(
        left_ctrl_released_first,
        VK_SPACE,
        none);
    REQUIRE_TRUE(left_released_fallback.allowed);
    REQUIRE_TRUE(!left_released_fallback.accepted_via_ctrl_still_down);
    REQUIRE_TRUE(left_released_fallback.accepted_via_ctrl_released_while_space_down);
    REQUIRE_TRUE(localpinyin::ctrl_space_toggle_should_fire(left_ctrl_released_first, true));
    REQUIRE_TRUE(!localpinyin::ctrl_space_toggle_should_fire(left_ctrl_released_first, true));
    localpinyin::ctrl_space_toggle_on_key_up(left_ctrl_released_first, VK_SPACE, false, false);
    REQUIRE_TRUE(!left_ctrl_released_first.ctrl_seen_in_chord);
    REQUIRE_TRUE(!left_ctrl_released_first.ctrl_released_while_space_down);
    REQUIRE_TRUE(!left_ctrl_released_first.pending_release);

    localpinyin::CtrlSpaceToggleTracker right_ctrl_released_first;
    localpinyin::ctrl_space_chord_on_key_down(right_ctrl_released_first, VK_RCONTROL);
    REQUIRE_TRUE(localpinyin::ctrl_space_chord_on_ctrl_key_up(
        right_ctrl_released_first,
        VK_RCONTROL,
        none,
        true).allowed);
    REQUIRE_TRUE(localpinyin::ctrl_space_keyup_fallback_decision(
        right_ctrl_released_first,
        VK_SPACE,
        none).allowed);

    localpinyin::CtrlSpaceToggleTracker ctrl_released_without_space;
    localpinyin::ctrl_space_chord_on_key_down(ctrl_released_without_space, VK_CONTROL);
    const auto no_space_latch = localpinyin::ctrl_space_chord_on_ctrl_key_up(
        ctrl_released_without_space,
        VK_CONTROL,
        none,
        false);
    REQUIRE_TRUE(!no_space_latch.allowed);
    REQUIRE_EQ(no_space_latch.reject_reason,
               localpinyin::CtrlSpaceCtrlKeyUpLatchRejectReason::SpaceNotDownOnCtrlKeyUp);
    localpinyin::ctrl_space_toggle_on_key_up(ctrl_released_without_space, VK_CONTROL, false, false);
    localpinyin::ctrl_space_chord_on_key_down(ctrl_released_without_space, VK_SPACE);
    REQUIRE_TRUE(!localpinyin::ctrl_space_keyup_fallback_decision(
        ctrl_released_without_space,
        VK_SPACE,
        none).allowed);

    localpinyin::CtrlSpaceToggleTracker keydown_toggled;
    localpinyin::ctrl_space_chord_on_key_down(keydown_toggled, VK_CONTROL);
    localpinyin::ctrl_space_chord_on_key_down(keydown_toggled, VK_SPACE);
    REQUIRE_TRUE(localpinyin::ctrl_space_toggle_should_fire(keydown_toggled, true));
    REQUIRE_TRUE(!localpinyin::ctrl_space_chord_on_ctrl_key_up(keydown_toggled, VK_CONTROL, none, true).allowed);
    const auto already_toggled_decision = localpinyin::ctrl_space_keyup_fallback_decision(keydown_toggled, VK_SPACE, ctrl);
    REQUIRE_TRUE(!already_toggled_decision.allowed);
    REQUIRE_EQ(already_toggled_decision.reject_reason,
               localpinyin::CtrlSpaceKeyUpFallbackRejectReason::AlreadyToggledThisChord);

    localpinyin::CtrlSpaceToggleTracker preserved_toggled;
    localpinyin::ctrl_space_chord_on_key_down(preserved_toggled, VK_LCONTROL);
    REQUIRE_TRUE(localpinyin::ctrl_space_toggle_should_fire(preserved_toggled, true));
    REQUIRE_TRUE(!localpinyin::ctrl_space_keyup_fallback_decision(preserved_toggled, VK_SPACE, ctrl).allowed);

    localpinyin::CtrlSpaceToggleTracker no_ctrl_seen;
    const auto ordinary_space_decision = localpinyin::ctrl_space_keyup_fallback_decision(no_ctrl_seen, VK_SPACE, ctrl);
    REQUIRE_TRUE(!ordinary_space_decision.allowed);
    REQUIRE_EQ(ordinary_space_decision.reject_reason,
               localpinyin::CtrlSpaceKeyUpFallbackRejectReason::NoCtrlSeenInChord);

    localpinyin::CtrlSpaceToggleTracker single_ctrl;
    localpinyin::ctrl_space_chord_on_key_down(single_ctrl, VK_CONTROL);
    const auto single_ctrl_decision = localpinyin::ctrl_space_keyup_fallback_decision(single_ctrl, VK_CONTROL, ctrl);
    REQUIRE_TRUE(!single_ctrl_decision.allowed);
    REQUIRE_EQ(single_ctrl_decision.reject_reason,
               localpinyin::CtrlSpaceKeyUpFallbackRejectReason::NotSpaceKey);

    localpinyin::CtrlSpaceToggleTracker pending_only;
    localpinyin::ctrl_space_chord_on_key_down(pending_only, VK_CONTROL);
    pending_only.pending_release = true;
    const auto pending_latch = localpinyin::ctrl_space_chord_on_ctrl_key_up(pending_only, VK_CONTROL, none, true);
    REQUIRE_TRUE(!pending_latch.allowed);
    REQUIRE_EQ(pending_latch.reject_reason,
               localpinyin::CtrlSpaceCtrlKeyUpLatchRejectReason::PendingReleaseActive);
    const auto pending_decision = localpinyin::ctrl_space_keyup_fallback_decision(pending_only, VK_SPACE, ctrl);
    REQUIRE_TRUE(!pending_decision.allowed);
    REQUIRE_EQ(pending_decision.reject_reason,
               localpinyin::CtrlSpaceKeyUpFallbackRejectReason::PendingReleaseActive);
    localpinyin::ctrl_space_toggle_on_key_up(pending_only, VK_SPACE, true, false);
    REQUIRE_TRUE(pending_only.pending_release);
    localpinyin::ctrl_space_toggle_on_key_up(pending_only, VK_CONTROL, false, false);
    REQUIRE_TRUE(!pending_only.pending_release);
    REQUIRE_TRUE(!pending_only.ctrl_seen_in_chord);

    localpinyin::ModifierKeyState ctrl_shift{};
    ctrl_shift.ctrl = true;
    ctrl_shift.shift = true;
    REQUIRE_TRUE(!localpinyin::is_ctrl_space(' ', ctrl_shift));

    localpinyin::ModifierKeyState ctrl_alt{};
    ctrl_alt.ctrl = true;
    ctrl_alt.alt = true;
    REQUIRE_TRUE(!localpinyin::is_ctrl_space(' ', ctrl_alt));
    localpinyin::CtrlSpaceToggleTracker alt_tracker;
    localpinyin::ctrl_space_chord_on_key_down(alt_tracker, VK_CONTROL);
    REQUIRE_TRUE(!localpinyin::ctrl_space_chord_on_ctrl_key_up(alt_tracker, VK_CONTROL, ctrl_alt, true).allowed);
    const auto alt_decision = localpinyin::ctrl_space_keyup_fallback_decision(alt_tracker, VK_SPACE, ctrl_alt);
    REQUIRE_TRUE(!alt_decision.allowed);
    REQUIRE_EQ(alt_decision.reject_reason,
               localpinyin::CtrlSpaceKeyUpFallbackRejectReason::AltOrWinModifier);

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
    REQUIRE_TRUE(!localpinyin::should_eat_candidate_navigation_key(VK_LEFT, false, 0));
    REQUIRE_TRUE(!localpinyin::should_eat_candidate_navigation_key(VK_RIGHT, false, 0));
    REQUIRE_TRUE(!localpinyin::should_eat_candidate_navigation_key(VK_LEFT, true, 0));
    REQUIRE_TRUE(localpinyin::should_eat_candidate_navigation_key(VK_LEFT, true, 3));
    REQUIRE_TRUE(localpinyin::should_eat_candidate_navigation_key(VK_RIGHT, true, 3));
    REQUIRE_TRUE(!localpinyin::should_eat_candidate_navigation_key(VK_LEFT, true, 3, false));
    REQUIRE_TRUE(!localpinyin::should_eat_candidate_navigation_key(VK_UP, true, 3));

    REQUIRE_TRUE(!localpinyin::transient_context_changed(0, 2, true));
    REQUIRE_TRUE(!localpinyin::transient_context_changed(1, 1, true));
    REQUIRE_TRUE(!localpinyin::transient_context_changed(1, 2, false));
    REQUIRE_TRUE(localpinyin::transient_context_changed(1, 2, true));

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
