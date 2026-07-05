#pragma once

namespace localpinyin {

enum class InputMode {
    Chinese,
    English
};

struct InputModeOptions {
    bool shift_toggle_enabled = true;
};

struct ModifierKeyState {
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
    bool win = false;
};

struct ShiftToggleTracker {
    bool pending = false;
    bool saw_other_key = false;
};

struct CtrlSpaceToggleTracker {
    bool pending_release = false;
};

[[nodiscard]] constexpr InputMode toggled_input_mode(InputMode mode) noexcept {
    return mode == InputMode::Chinese ? InputMode::English : InputMode::Chinese;
}

[[nodiscard]] constexpr bool is_shift_key(unsigned long key) noexcept {
    return key == 0x10 || key == 0xA0 || key == 0xA1;
}

[[nodiscard]] constexpr bool is_ctrl_space(unsigned long key, ModifierKeyState modifiers) noexcept {
    return key == ' ' && modifiers.ctrl && !modifiers.shift && !modifiers.alt && !modifiers.win;
}

[[nodiscard]] constexpr bool ctrl_space_toggle_should_fire(CtrlSpaceToggleTracker& tracker,
                                                           bool ctrl_space_key) noexcept {
    if (!ctrl_space_key || tracker.pending_release) {
        return false;
    }
    tracker.pending_release = true;
    return true;
}

constexpr void ctrl_space_toggle_on_key_up(CtrlSpaceToggleTracker& tracker, unsigned long key) noexcept {
    if (key == ' ') {
        tracker.pending_release = false;
    }
}

[[nodiscard]] constexpr bool should_eat_backspace_for_composition(InputMode mode,
                                                                  bool composition_active,
                                                                  bool pass_through) noexcept {
    return mode == InputMode::Chinese && composition_active && !pass_through;
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
