#pragma once

#include "../engine/input_options.h"

#include <cstddef>
#include <cstdint>

namespace localpinyin {

enum class InputMode {
    Chinese,
    English
};

struct InputModeOptions {
    bool shift_toggle_enabled = false;
    // Next-word suggestions after a commit are opt-in. Context always ranks
    // the candidates of newly typed pinyin regardless of this option.
    bool prediction_enabled = false;
    // Helpers (辅助功能). After a digit the period key types "." (3.14), and
    // , / : stay ASCII too (1,000, 1/2, 12:30); both on by default. After an
    // English letter every punctuation key types ASCII (Hello, world.); off
    // by default, as it would also end 我用Python with "." instead of 。.
    // Code editors, IDEs and terminals get ASCII punctuation (<({[!?,).
    bool digit_period = true;
    bool digit_punctuation = true;
    bool letter_punctuation = false;
    bool code_punctuation = true;
    // Fuzzy pinyin, spelling correction, Chinese-English mixing, double pinyin.
    EngineInputOptions engine;
};

struct ModifierKeyState {
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
    bool win = false;
};

struct ModifierKeyReadings {
    bool ctrl = false;
    bool left_ctrl = false;
    bool right_ctrl = false;
    bool shift = false;
    bool left_shift = false;
    bool right_shift = false;
    bool alt = false;
    bool left_alt = false;
    bool right_alt = false;
    bool left_win = false;
    bool right_win = false;
};

struct ShiftToggleTracker {
    bool pending = false;
    bool saw_other_key = false;
};

struct CtrlSpaceToggleTracker {
    bool pending_release = false;
    bool ctrl_seen_in_chord = false;
    bool space_keydown_seen = false;
    bool toggled_this_chord = false;
    bool ctrl_released_while_space_down = false;
};

struct CtrlSpaceKeyDownState {
    bool accepted_by_test_key_down = false;
};

enum class CtrlSpaceKeyUpFallbackRejectReason {
    None,
    NotSpaceKey,
    NoCtrlSeenInChord,
    CtrlNotDownOnSpaceKeyUp,
    AlreadyToggledThisChord,
    PendingReleaseActive,
    AltOrWinModifier
};

struct CtrlSpaceKeyUpFallbackDecision {
    bool allowed = false;
    CtrlSpaceKeyUpFallbackRejectReason reject_reason = CtrlSpaceKeyUpFallbackRejectReason::None;
    bool accepted_via_ctrl_still_down = false;
    bool accepted_via_ctrl_released_while_space_down = false;
};

enum class CtrlSpaceCtrlKeyUpLatchRejectReason {
    None,
    NotCtrlKey,
    NoCtrlSeenInChord,
    SpaceNotDownOnCtrlKeyUp,
    AlreadyToggledThisChord,
    PendingReleaseActive,
    AltOrWinModifier
};

struct CtrlSpaceCtrlKeyUpLatchDecision {
    bool allowed = false;
    CtrlSpaceCtrlKeyUpLatchRejectReason reject_reason = CtrlSpaceCtrlKeyUpLatchRejectReason::None;
};

[[nodiscard]] constexpr InputMode toggled_input_mode(InputMode mode) noexcept {
    return mode == InputMode::Chinese ? InputMode::English : InputMode::Chinese;
}

[[nodiscard]] constexpr ModifierKeyState modifier_key_state_from_readings(
    ModifierKeyReadings readings) noexcept {
    return ModifierKeyState{
        readings.ctrl || readings.left_ctrl || readings.right_ctrl,
        readings.shift || readings.left_shift || readings.right_shift,
        readings.alt || readings.left_alt || readings.right_alt,
        readings.left_win || readings.right_win
    };
}

[[nodiscard]] constexpr bool is_shift_key(unsigned long key) noexcept {
    return key == 0x10 || key == 0xA0 || key == 0xA1;
}

[[nodiscard]] constexpr bool is_ctrl_key(unsigned long key) noexcept {
    return key == 0x11 || key == 0xA2 || key == 0xA3;
}

[[nodiscard]] constexpr bool is_ctrl_space(unsigned long key, ModifierKeyState modifiers) noexcept {
    return key == ' ' && modifiers.ctrl && !modifiers.shift && !modifiers.alt && !modifiers.win;
}

[[nodiscard]] constexpr bool should_handle_ctrl_space_key_down(unsigned long key,
                                                               ModifierKeyState modifiers,
                                                               CtrlSpaceKeyDownState state) noexcept {
    return is_ctrl_space(key, modifiers) ||
           (key == ' ' && state.accepted_by_test_key_down);
}

[[nodiscard]] constexpr bool ctrl_space_toggle_should_fire(CtrlSpaceToggleTracker& tracker,
                                                           bool ctrl_space_key) noexcept {
    if (!ctrl_space_key || tracker.pending_release || tracker.toggled_this_chord) {
        return false;
    }
    tracker.pending_release = true;
    tracker.toggled_this_chord = true;
    return true;
}

constexpr void ctrl_space_chord_on_key_down(CtrlSpaceToggleTracker& tracker, unsigned long key) noexcept {
    if (is_ctrl_key(key)) {
        tracker.ctrl_seen_in_chord = true;
    }
    if (key == ' ') {
        tracker.space_keydown_seen = true;
    }
}

constexpr void ctrl_space_toggle_on_key_up(CtrlSpaceToggleTracker& tracker,
                                           unsigned long key,
                                           bool ctrl_down_after,
                                           bool space_down_after) noexcept {
    if ((key == ' ' || is_ctrl_key(key)) && !ctrl_down_after && !space_down_after) {
        tracker = CtrlSpaceToggleTracker{};
    }
}

[[nodiscard]] constexpr CtrlSpaceKeyUpFallbackDecision ctrl_space_keyup_fallback_decision(
    const CtrlSpaceToggleTracker& tracker,
    unsigned long key,
    ModifierKeyState modifiers) noexcept {
    if (key != ' ') {
        return {false, CtrlSpaceKeyUpFallbackRejectReason::NotSpaceKey};
    }
    if (modifiers.alt || modifiers.win) {
        return {false, CtrlSpaceKeyUpFallbackRejectReason::AltOrWinModifier};
    }
    if (!tracker.ctrl_seen_in_chord) {
        return {false, CtrlSpaceKeyUpFallbackRejectReason::NoCtrlSeenInChord};
    }
    if (tracker.toggled_this_chord) {
        return {false, CtrlSpaceKeyUpFallbackRejectReason::AlreadyToggledThisChord};
    }
    if (tracker.pending_release) {
        return {false, CtrlSpaceKeyUpFallbackRejectReason::PendingReleaseActive};
    }
    if (modifiers.ctrl) {
        return {true, CtrlSpaceKeyUpFallbackRejectReason::None, true, false};
    }
    if (tracker.ctrl_released_while_space_down) {
        return {true, CtrlSpaceKeyUpFallbackRejectReason::None, false, true};
    }
    return {false, CtrlSpaceKeyUpFallbackRejectReason::CtrlNotDownOnSpaceKeyUp};
}

[[nodiscard]] constexpr CtrlSpaceCtrlKeyUpLatchDecision ctrl_space_ctrl_keyup_latch_decision(
    const CtrlSpaceToggleTracker& tracker,
    unsigned long key,
    ModifierKeyState modifiers,
    bool space_down_after) noexcept {
    if (!is_ctrl_key(key)) {
        return {false, CtrlSpaceCtrlKeyUpLatchRejectReason::NotCtrlKey};
    }
    if (modifiers.alt || modifiers.win) {
        return {false, CtrlSpaceCtrlKeyUpLatchRejectReason::AltOrWinModifier};
    }
    if (!tracker.ctrl_seen_in_chord) {
        return {false, CtrlSpaceCtrlKeyUpLatchRejectReason::NoCtrlSeenInChord};
    }
    if (!space_down_after) {
        return {false, CtrlSpaceCtrlKeyUpLatchRejectReason::SpaceNotDownOnCtrlKeyUp};
    }
    if (tracker.toggled_this_chord) {
        return {false, CtrlSpaceCtrlKeyUpLatchRejectReason::AlreadyToggledThisChord};
    }
    if (tracker.pending_release) {
        return {false, CtrlSpaceCtrlKeyUpLatchRejectReason::PendingReleaseActive};
    }
    return {true, CtrlSpaceCtrlKeyUpLatchRejectReason::None};
}

constexpr CtrlSpaceCtrlKeyUpLatchDecision ctrl_space_chord_on_ctrl_key_up(
    CtrlSpaceToggleTracker& tracker,
    unsigned long key,
    ModifierKeyState modifiers,
    bool space_down_after) noexcept {
    const CtrlSpaceCtrlKeyUpLatchDecision decision =
        ctrl_space_ctrl_keyup_latch_decision(tracker, key, modifiers, space_down_after);
    if (decision.allowed) {
        tracker.ctrl_released_while_space_down = true;
    }
    return decision;
}

[[nodiscard]] constexpr bool should_eat_backspace_for_composition(InputMode mode,
                                                                  bool composition_active,
                                                                  bool pass_through) noexcept {
    return mode == InputMode::Chinese && composition_active && !pass_through;
}

[[nodiscard]] constexpr bool should_eat_candidate_navigation_key(
    unsigned long key,
    bool composition_active,
    size_t candidate_count,
    bool context_matches = true) noexcept {
    return (key == 0x25 || key == 0x27) && context_matches &&
           composition_active && candidate_count > 0;
}

[[nodiscard]] constexpr bool transient_context_changed(
    std::uintptr_t owner_context,
    std::uintptr_t current_context,
    bool transient_state_active) noexcept {
    return transient_state_active && owner_context != 0 &&
           current_context != 0 && owner_context != current_context;
}

[[nodiscard]] constexpr bool can_start_shift_toggle(unsigned long key,
                                                    ModifierKeyState modifiers,
                                                    bool shift_toggle_enabled) noexcept {
    return shift_toggle_enabled && is_shift_key(key) && !modifiers.ctrl && !modifiers.alt && !modifiers.win;
}

constexpr void shift_toggle_on_key_down(ShiftToggleTracker& tracker,
                                        unsigned long key,
                                        ModifierKeyState modifiers,
                                        bool shift_toggle_enabled) noexcept {
    if (can_start_shift_toggle(key, modifiers, shift_toggle_enabled)) {
        if (!tracker.pending) {
            tracker.pending = true;
            tracker.saw_other_key = false;
        }
        return;
    }
    if (tracker.pending) {
        tracker.saw_other_key = true;
    }
}

[[nodiscard]] constexpr bool shift_toggle_on_key_up(ShiftToggleTracker& tracker,
                                                    unsigned long key,
                                                    ModifierKeyState modifiers,
                                                    bool shift_toggle_enabled) noexcept {
    const bool should_toggle = shift_toggle_enabled &&
                               tracker.pending &&
                               is_shift_key(key) &&
                               !tracker.saw_other_key &&
                               !modifiers.ctrl &&
                               !modifiers.alt &&
                               !modifiers.win;
    if (is_shift_key(key)) {
        tracker.pending = false;
        tracker.saw_other_key = false;
    }
    return should_toggle;
}

}  // namespace localpinyin
