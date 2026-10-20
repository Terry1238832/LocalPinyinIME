"""Static checks for shared Shift and Ctrl+Space mode-toggle paths."""

from __future__ import annotations

import argparse
from pathlib import Path


def fail(message: str) -> None:
    raise AssertionError(message)


def block_after(text: str, marker: str, stop_markers: tuple[str, ...]) -> str:
    start = text.find(marker)
    if start < 0:
        fail(f"missing marker: {marker}")
    end = len(text)
    for stop in stop_markers:
        stop_at = text.find(stop, start + len(marker))
        if stop_at >= 0:
            end = min(end, stop_at)
    return text[start:end]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--text-service", type=Path, required=True)
    parser.add_argument("--input-mode", type=Path, required=True)
    args = parser.parse_args()

    text_service = args.text_service.read_text(encoding="utf-8")
    input_mode = args.input_mode.read_text(encoding="utf-8")

    toggle_block = block_after(
        text_service,
        "HRESULT TextService::toggle_input_mode",
        ("bool TextService::handle_chinese_key",),
    )
    if toggle_block.count("mode_ = toggled_input_mode(mode_, english_profile_)") != 1:
        fail("toggle_input_mode must be the single TextService mode flip point")
    if "language_bar_.set_mode(mode_)" not in toggle_block:
        fail("shared mode toggle must update Language Bar state")
    if "TF_LBI_ICON" not in text_service and "TF_LBI_ICON" not in Path(args.text_service).with_name("language_bar.cpp").read_text(encoding="utf-8"):
        fail("Language Bar mode changes must be able to notify icon updates")
    if "commit_raw_composition(target)" not in toggle_block or "candidate_window_.hide(reason)" not in toggle_block:
        fail("shared mode toggle must keep typed letters and clean up candidates")

    test_down = block_after(
        text_service,
        "HRESULT TextService::OnTestKeyDown",
        ("HRESULT TextService::OnKeyDown",),
    )
    key_down = block_after(
        text_service,
        "HRESULT TextService::OnKeyDown",
        ("HRESULT TextService::OnTestKeyUp",),
    )
    key_up = block_after(
        text_service,
        "HRESULT TextService::OnKeyUp",
        ("HRESULT TextService::OnPreservedKey",),
    )
    keyup_fallback = block_after(
        text_service,
        "bool TextService::handle_ctrl_space_keyup_fallback",
        ("HRESULT TextService::OnSetFocus",),
    )
    preserved = block_after(
        text_service,
        "HRESULT TextService::OnPreservedKey",
        ("HRESULT TextService::OnCompositionTerminated",),
    )
    ctrl_gate = block_after(
        text_service,
        "bool TextService::handle_ctrl_space_toggle",
        ("HRESULT TextService::toggle_input_mode",),
    )
    thread_hook = block_after(
        text_service,
        "bool TextService::handle_thread_keyboard_hook",
        ("HRESULT TextService::OnSetFocus",),
    )

    for name, block in (
        ("OnTestKeyDown", test_down),
        ("OnKeyDown", key_down),
        ("OnPreservedKey", preserved),
    ):
        if "handle_ctrl_space_toggle(context" not in block:
            fail(f"{name} must enter the shared Ctrl+Space gate")
        if "toggle_input_mode(context" in block:
            fail(f"{name} must not bypass the Ctrl+Space one-shot gate")

    if 'toggle_input_mode(context, L"shift_toggle")' not in key_up:
        fail("Shift must keep using the shared toggle_input_mode action")
    if "toggle_input_mode(context, reason)" not in ctrl_gate:
        fail("Ctrl+Space gate must call the same shared toggle_input_mode action")
    if "ctrl_space_toggle_should_fire(ctrl_space_toggle_, true)" not in ctrl_gate:
        fail("Ctrl+Space gate must enforce one toggle per chord")
    if 'SetWindowsHookExW(WH_KEYBOARD' not in text_service:
        fail("Ctrl+Space fallback must install a current-thread keyboard hook, not a global low-level hook")
    if "WH_KEYBOARD_LL" in text_service:
        fail("Ctrl+Space fallback must not use a global low-level keyboard hook")
    if 'handle_ctrl_space_toggle(nullptr, L"ctrl_space_thread_keyboard_hook")' not in thread_hook:
        fail("thread keyboard hook must enter the same Ctrl+Space gate")
    for forbidden in (
        "ctrl_space_diagnostic_message",
        "hook_relevant_key_observed",
        "read_context_composition_summary",
        "log_key_event_runtime_diagnostics",
    ):
        if forbidden in text_service:
            fail(f"production key path must not retain synchronous diagnostic helper: {forbidden}")
    if "publish_runtime_input_mode_status(reason, true)" not in toggle_block:
        fail("shared mode toggle must publish runtime input-mode status for Settings")
    if "HRESULT TextService::toggle_input_mode_from_language_bar" not in text_service:
        fail("Language Bar click/menu must enter a TextService wrapper")
    language_bar_toggle = block_after(
        text_service,
        "HRESULT TextService::toggle_input_mode_from_language_bar",
        ("HRESULT TextService::open_settings_from_language_bar",),
    )
    if "toggle_input_mode(nullptr, reason ? reason : L\"mode_icon_toggle\")" not in language_bar_toggle:
        fail("Language Bar click/menu must route through the shared toggle_input_mode action")
    if "set_toggle_callback" not in text_service or "clear_callbacks" not in text_service:
        fail("TextService must attach and clear Language Bar callbacks")
    settings_block = block_after(
        text_service,
        "HRESULT TextService::open_settings_from_language_bar",
        ("bool TextService::handle_chinese_key",),
    )
    for needle in (
        # Beside the DLL, or one folder up for the 32-bit DLL in x86\.
        "install_file_path(g_instance",
        "LocalPinyinSettings.exe",
        "GetFileAttributesW",
        "CreateProcessW",
        "event=open_settings source=",
    ):
        if needle not in text_service and needle not in settings_block:
            fail(f"missing Settings launcher evidence: {needle}")
    language_bar = Path(args.text_service).with_name("language_bar.cpp").read_text(encoding="utf-8")
    for needle in (
        "TF_LBI_CLK_RIGHT",
        "show_mode_icon_context_menu",
        "CreatePopupMenu",
        "AppendMenuW",
        "TrackPopupMenuEx",
        "TPM_RETURNCMD",
        "GetCursorPos",
        "CreateWindowExW",
        "HWND_MESSAGE",
        "PostMessageW(owner, WM_NULL",
        "event=mode_icon_popup_menu_show requested=true",
        "event=mode_icon_popup_menu_result command=",
        "popup_menu",
        "event=language_bar_initial_update_deferred",
        "event=language_bar_initial_update_retry",
    ):
        if needle not in language_bar:
            fail(f"missing Language Bar popup/retry evidence: {needle}")
    if 'handle_ctrl_space_keyup_fallback(context, key, lparam, L"OnKeyUp"' not in key_up:
        fail("OnKeyUp must consider the Space-keyup fallback")
    if 'handle_ctrl_space_keyup_fallback(nullptr, key, lparam, L"thread_keyboard_hook"' not in thread_hook:
        fail("thread keyboard hook keyup must consider the Space-keyup fallback")
    if 'handle_ctrl_space_ctrl_keyup_latch(key, lparam, L"OnKeyUp")' not in key_up:
        fail("OnKeyUp must latch Ctrl release while Space is still down")
    if 'handle_ctrl_space_ctrl_keyup_latch(key, lparam, L"thread_keyboard_hook")' not in thread_hook:
        fail("thread keyboard hook keyup must latch Ctrl release while Space is still down")
    if 'handle_ctrl_space_toggle(context, L"ctrl_space_keyup_fallback")' not in keyup_fallback:
        fail("Space-keyup fallback must enter the shared Ctrl+Space gate")
    for needle in (
        "foreground_window_thread_id=",
        "focus_window_thread_id=",
        "hook_target_thread_id=",
        "activate_begin",
        "activate_complete",
        "deactivate_begin",
        "deactivate_complete",
        "hook_install_attempt",
        "hook_install_success",
        "hook_install_failed",
        "hook_uninstall_attempt",
        "hook_uninstall_success",
        "hook_uninstall_failed",
        "UnhookWindowsHookEx(hook)",
    ):
        if needle not in text_service:
            fail(f"missing auditable hook lifecycle marker: {needle}")

    required_input_mode = [
        "bool shift_toggle_enabled = false",
        "return key == ' ' && modifiers.ctrl && !modifiers.shift && !modifiers.alt && !modifiers.win",
        "return is_ctrl_space(key, modifiers) ||",
        "key == ' ' && state.accepted_by_test_key_down",
        "tracker.pending_release = true",
        "tracker = CtrlSpaceToggleTracker{}",
        "ctrl_seen_in_chord",
        "space_keydown_seen",
        "toggled_this_chord",
        "ctrl_released_while_space_down",
        "CtrlSpaceKeyUpFallbackDecision",
        "CtrlSpaceCtrlKeyUpLatchDecision",
        "CtrlSpaceKeyUpFallbackRejectReason::AlreadyToggledThisChord",
        "CtrlSpaceKeyUpFallbackRejectReason::PendingReleaseActive",
        "CtrlSpaceCtrlKeyUpLatchRejectReason::SpaceNotDownOnCtrlKeyUp",
    ]
    for needle in required_input_mode:
        if needle not in input_mode:
            fail(f"missing input-mode rule: {needle}")

    print("input mode toggle path checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
