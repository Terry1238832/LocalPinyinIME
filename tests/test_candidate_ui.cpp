#include "test_common.h"
#include "../src/ime/candidate_ui.h"
#include "../src/ime/candidate_window.h"
#include "local_pinyin_blue_theme.h"

#include <dwmapi.h>

#include <string>
#include <vector>

namespace {

RECT make_rect(LONG left, LONG top, LONG right, LONG bottom) {
    return RECT{left, top, right, bottom};
}

int width(const RECT& rect) {
    return rect.right - rect.left;
}

int height(const RECT& rect) {
    return rect.bottom - rect.top;
}

bool is_inside(const RECT& outer, const RECT& inner) {
    return inner.left >= outer.left &&
           inner.top >= outer.top &&
           inner.right <= outer.right &&
           inner.bottom <= outer.bottom;
}

bool candidate_items_do_not_overlap(const std::vector<localpinyin::CandidateItemLayout>& items) {
    for (size_t i = 1; i < items.size(); ++i) {
        const RECT& previous = items[i - 1].bounds;
        const RECT& current = items[i].bounds;
        const bool separate_horizontally = previous.right <= current.left || current.right <= previous.left;
        const bool separate_vertically = previous.bottom <= current.top || current.bottom <= previous.top;
        if (!separate_horizontally && !separate_vertically) {
            return false;
        }
    }
    return true;
}

bool rects_do_not_overlap(const RECT& first, const RECT& second) {
    if (first.right <= first.left || first.bottom <= first.top ||
        second.right <= second.left || second.bottom <= second.top) {
        return true;
    }
    const bool separate_horizontally = first.right <= second.left || second.right <= first.left;
    const bool separate_vertically = first.bottom <= second.top || second.bottom <= first.top;
    return separate_horizontally || separate_vertically;
}

struct RecordedImeWinEvent {
    DWORD event = 0;
    HWND hwnd = nullptr;
    LONG object_id = 0;
    LONG child_id = 0;
};

struct ImeWinEventRecorder {
    std::vector<RecordedImeWinEvent> events;
};

void record_ime_win_event(DWORD event,
                          HWND hwnd,
                          LONG object_id,
                          LONG child_id,
                          void* context) noexcept {
    auto* recorder = static_cast<ImeWinEventRecorder*>(context);
    if (recorder) {
        recorder->events.push_back(RecordedImeWinEvent{event, hwnd, object_id, child_id});
    }
}

localpinyin::CandidateLayoutResult layout_for(const std::vector<std::wstring>& candidates,
                                              RECT caret,
                                              RECT work,
                                              int dpi = 96,
                                              localpinyin::CandidateWindowOptions options = {}) {
    localpinyin::CandidateLayoutInput input;
    input.composition_text = L"nihaoshijie";
    input.candidate_texts = candidates;
    input.selected_index = 0;
    input.caret_rect = caret;
    input.work_area = work;
    input.dpi = dpi;
    input.options = options;
    return localpinyin::make_candidate_layout(input);
}

}  // namespace

int main() {
    using namespace localpinyin;

    REQUIRE_TRUE(!should_show_candidate_window(L"", 3, false));
    REQUIRE_TRUE(!should_show_candidate_window(L"nihao", 0, false));
    REQUIRE_TRUE(!should_show_candidate_window(L"nihao", 3, true));
    REQUIRE_TRUE(should_show_candidate_window(L"nihao", 1, false));
    REQUIRE_EQ(candidate_text_size_min_level(), 1);
    REQUIRE_EQ(candidate_text_size_max_level(), 7);
    REQUIRE_EQ(candidate_text_size_default_level(), 4);
    REQUIRE_EQ(candidate_text_size_level(CandidateTextSize::Standard), 4);
    REQUIRE_EQ(candidate_text_size_from_level(0), CandidateTextSize::Level1);
    REQUIRE_EQ(candidate_text_size_from_level(8), CandidateTextSize::Level7);
    REQUIRE_EQ(candidate_text_size_dip(CandidateTextSize::Standard), 18);
    REQUIRE_EQ(candidate_font_px_for_size(CandidateTextSize::Standard, 96), 18);
    REQUIRE_EQ(candidate_font_px_for_size(CandidateTextSize::Level7, 192), scale_dip(24, 192));
    REQUIRE_EQ(candidate_text_size_to_string(CandidateTextSize::Level5), std::wstring(L"level5"));
    REQUIRE_EQ(parse_candidate_text_size(L"level5"), CandidateTextSize::Level5);
    REQUIRE_EQ(parse_candidate_text_size(L"6"), CandidateTextSize::Level6);
    REQUIRE_EQ(parse_candidate_text_size(L"small"), CandidateTextSize::Small);
    REQUIRE_EQ(parse_candidate_text_size(L"large"), CandidateTextSize::Large);
    REQUIRE_TRUE(candidate_text_size_display_text(CandidateTextSize::Standard).find(L"18 px") != std::wstring::npos);

    const auto show_decision = decide_candidate_display(CandidateDisplayState{false, true, 3, true});
    REQUIRE_TRUE(show_decision.request_show);
    REQUIRE_EQ(show_decision.hide_reason, CandidateHideReason::None);

    const auto english_decision = decide_candidate_display(CandidateDisplayState{true, true, 3, true});
    REQUIRE_TRUE(!english_decision.request_show);
    REQUIRE_EQ(english_decision.hide_reason, CandidateHideReason::EnglishMode);

    const auto empty_composition_decision = decide_candidate_display(CandidateDisplayState{false, false, 3, true});
    REQUIRE_TRUE(!empty_composition_decision.request_show);
    REQUIRE_EQ(empty_composition_decision.hide_reason, CandidateHideReason::EmptyComposition);

    const auto empty_candidates_decision = decide_candidate_display(CandidateDisplayState{false, true, 0, true});
    REQUIRE_TRUE(!empty_candidates_decision.request_show);
    REQUIRE_EQ(empty_candidates_decision.hide_reason, CandidateHideReason::EmptyCandidates);

    const auto no_caret_decision = decide_candidate_display(CandidateDisplayState{false, true, 3, false});
    REQUIRE_TRUE(!no_caret_decision.request_show);
    REQUIRE_EQ(no_caret_decision.hide_reason, CandidateHideReason::NoCaretRect);
    REQUIRE_EQ(candidate_hide_reason_to_string(no_caret_decision.hide_reason), std::wstring(L"no_caret_rect"));

    const auto popup_config = make_candidate_window_create_config(nullptr, false);
    REQUIRE_TRUE(popup_config.parent == nullptr);
    REQUIRE_TRUE(popup_config.menu == nullptr);
    REQUIRE_EQ(popup_config.parent_kind, CandidateCreateParentKind::Null);
    REQUIRE_TRUE((popup_config.style & WS_POPUP) == WS_POPUP);
    REQUIRE_TRUE((popup_config.ex_style & WS_EX_NOACTIVATE) == WS_EX_NOACTIVATE);
    REQUIRE_TRUE((popup_config.ex_style & WS_EX_TOOLWINDOW) == WS_EX_TOOLWINDOW);
    REQUIRE_TRUE((popup_config.ex_style & WS_EX_TOPMOST) == 0);
    REQUIRE_TRUE(popup_config.use_topmost_after_create);

    const HWND fake_owner = reinterpret_cast<HWND>(static_cast<INT_PTR>(0x1234));
    const auto valid_owner_config = make_candidate_window_create_config(fake_owner, true);
    REQUIRE_TRUE(valid_owner_config.parent == fake_owner);
    REQUIRE_EQ(valid_owner_config.parent_kind, CandidateCreateParentKind::ValidOwner);
    REQUIRE_TRUE(valid_owner_config.menu == nullptr);

    const auto invalid_owner_config = make_candidate_window_create_config(fake_owner, false);
    REQUIRE_TRUE(invalid_owner_config.parent == nullptr);
    REQUIRE_EQ(invalid_owner_config.parent_kind, CandidateCreateParentKind::RejectedInvalidOwner);

    const auto topmost_config = make_candidate_window_create_config(HWND_TOPMOST, true);
    REQUIRE_TRUE(topmost_config.parent == nullptr);
    REQUIRE_EQ(topmost_config.parent_kind, CandidateCreateParentKind::RejectedInvalidOwner);

    const auto message_config = make_candidate_window_create_config(HWND_MESSAGE, true);
    REQUIRE_TRUE(message_config.parent == nullptr);
    REQUIRE_EQ(message_config.parent_kind, CandidateCreateParentKind::RejectedInvalidOwner);

    CandidateShellCloakRecoveryInput inactive_recovery_input;
    inactive_recovery_input.candidate_cloak_hr = S_OK;
    inactive_recovery_input.candidate_cloak_value = kCandidateDwmCloakedShell;
    auto inactive_recovery = decide_candidate_shell_cloak_recovery(inactive_recovery_input);
    REQUIRE_EQ(inactive_recovery.reason, std::wstring(L"candidate_model_invalid"));
    inactive_recovery_input.candidate_model_valid = true;
    inactive_recovery = decide_candidate_shell_cloak_recovery(inactive_recovery_input);
    REQUIRE_EQ(inactive_recovery.reason, std::wstring(L"candidate_show_not_requested"));

    CandidateShellCloakRecoveryInput recovery_input;
    recovery_input.candidate_model_valid = true;
    recovery_input.candidate_show_requested = true;
    recovery_input.candidate_hwnd_valid = true;
    recovery_input.candidate_hwnd_visible = true;
    recovery_input.current_process_id = 100;
    recovery_input.candidate_process_id = 100;
    recovery_input.candidate_cloak_hr = S_OK;
    recovery_input.owner_root_valid = true;
    recovery_input.owner_root_cloak_hr = S_OK;

    auto recovery = decide_candidate_shell_cloak_recovery(recovery_input);
    REQUIRE_TRUE(!recovery.shell_cloak_detected);
    REQUIRE_TRUE(!recovery.recovery_eligible);
    REQUIRE_TRUE(!recovery.recovery_attempt_allowed);
    REQUIRE_EQ(recovery.reason, std::wstring(L"candidate_not_shell_cloaked"));

    recovery_input.candidate_cloak_value = DWM_CLOAKED_APP;
    recovery = decide_candidate_shell_cloak_recovery(recovery_input);
    REQUIRE_TRUE(!recovery.shell_cloak_detected);
    REQUIRE_TRUE(!recovery.recovery_eligible);
    REQUIRE_EQ(recovery.reason, std::wstring(L"candidate_not_shell_cloaked"));

    recovery_input.candidate_cloak_value = DWM_CLOAKED_INHERITED;
    recovery = decide_candidate_shell_cloak_recovery(recovery_input);
    REQUIRE_TRUE(!recovery.shell_cloak_detected);
    REQUIRE_TRUE(!recovery.recovery_eligible);
    REQUIRE_EQ(recovery.reason, std::wstring(L"candidate_not_shell_cloaked"));

    recovery_input.candidate_cloak_value = kCandidateDwmCloakedShell;
    recovery_input.owner_root_valid = false;
    recovery = decide_candidate_shell_cloak_recovery(recovery_input);
    REQUIRE_TRUE(recovery.shell_cloak_detected);
    REQUIRE_TRUE(!recovery.recovery_eligible);
    REQUIRE_EQ(recovery.reason, std::wstring(L"owner_root_invalid"));

    recovery_input.owner_root_valid = true;
    recovery_input.recovery_attempts = 0;
    recovery_input.owner_root_process_id = 100;
    recovery = decide_candidate_shell_cloak_recovery(recovery_input);
    REQUIRE_TRUE(recovery.shell_cloak_detected);
    REQUIRE_TRUE(recovery.recovery_eligible);
    REQUIRE_TRUE(recovery.recovery_attempt_allowed);
    REQUIRE_TRUE(recovery.owner_same_process);
    REQUIRE_EQ(recovery.strategy, CandidateShellCloakRecoveryStrategy::SameProcessContextRootOwner);
    REQUIRE_EQ(candidate_shell_cloak_recovery_strategy_to_string(recovery.strategy),
               std::wstring(L"same_process_context_root_owner"));
    REQUIRE_EQ(recovery.reason, std::wstring(L"eligible_context_root"));

    recovery_input.owner_root_process_id = 200;
    recovery = decide_candidate_shell_cloak_recovery(recovery_input);
    REQUIRE_TRUE(recovery.shell_cloak_detected);
    REQUIRE_TRUE(!recovery.recovery_eligible);
    REQUIRE_TRUE(!recovery.recovery_attempt_allowed);
    REQUIRE_TRUE(!recovery.owner_same_process);
    REQUIRE_EQ(recovery.strategy, CandidateShellCloakRecoveryStrategy::None);
    REQUIRE_EQ(recovery.reason, std::wstring(L"owner_root_cross_process"));

    recovery_input.recovery_attempts = 1;
    recovery = decide_candidate_shell_cloak_recovery(recovery_input);
    REQUIRE_TRUE(recovery.shell_cloak_detected);
    REQUIRE_TRUE(!recovery.recovery_eligible);
    REQUIRE_TRUE(!recovery.recovery_attempt_allowed);
    REQUIRE_EQ(recovery.strategy, CandidateShellCloakRecoveryStrategy::None);
    REQUIRE_EQ(recovery.reason, std::wstring(L"recovery_attempt_limit_reached"));

    recovery_input.recovery_attempts = 0;
    recovery_input.owner_root_process_id = 100;
    recovery_input.owner_root_cloak_value = kCandidateDwmCloakedShell;
    recovery = decide_candidate_shell_cloak_recovery(recovery_input);
    REQUIRE_TRUE(recovery.shell_cloak_detected);
    REQUIRE_TRUE(!recovery.recovery_eligible);
    REQUIRE_EQ(recovery.reason, std::wstring(L"owner_root_shell_cloaked"));
    recovery_input.owner_root_cloak_value = 0;

    CandidateReplacementCommitInput commit_input;
    commit_input.replacement_hwnd_valid = true;
    commit_input.replacement_visible = true;
    commit_input.owner_verified = true;
    commit_input.dwm_query_succeeded = true;
    commit_input.shell_cloaked = false;
    commit_input.rect_on_monitor = true;
    REQUIRE_TRUE(should_commit_candidate_replacement(commit_input));
    commit_input.owner_verified = false;
    REQUIRE_TRUE(!should_commit_candidate_replacement(commit_input));
    commit_input.owner_verified = true;
    commit_input.shell_cloaked = true;
    REQUIRE_TRUE(!should_commit_candidate_replacement(commit_input));
    commit_input.shell_cloaked = false;
    commit_input.replacement_visible = false;
    REQUIRE_TRUE(!should_commit_candidate_replacement(commit_input));

    const auto first_retry = decide_candidate_layout_retry(true, false, 0);
    REQUIRE_TRUE(first_retry.schedule);
    REQUIRE_EQ(first_retry.next_attempt, 1U);
    REQUIRE_EQ(first_retry.delay_ms, 50U);
    const auto last_retry = decide_candidate_layout_retry(true, false, 2);
    REQUIRE_TRUE(last_retry.schedule);
    REQUIRE_EQ(last_retry.next_attempt, 3U);
    REQUIRE_EQ(last_retry.delay_ms, 300U);
    REQUIRE_TRUE(!decide_candidate_layout_retry(true, false, 3).schedule);
    REQUIRE_TRUE(!decide_candidate_layout_retry(true, true, 0).schedule);
    REQUIRE_TRUE(!decide_candidate_layout_retry(false, false, 0).schedule);

    const RECT work = make_rect(0, 0, 800, 600);
    const RECT middle = make_rect(100, 100, 101, 122);

    for (const int dpi : {96, 120, 144, 192}) {
        for (const auto& word : {
                 std::wstring(L"\u5F88\u68D2"),
                 std::wstring(L"\u4F60\u597D"),
                 std::wstring(L"\u4F60\u597D\u4E16\u754C"),
                 std::wstring(L"\u6211\u60F3\u53BB\u5317\u4EAC")
             }) {
            const auto short_word = layout_for({word}, middle, make_rect(0, 0, 1600, 1200), dpi);
            REQUIRE_TRUE(short_word.visible);
            REQUIRE_EQ(short_word.items.size(), size_t{1});
            const auto& item = short_word.items.front();
            REQUIRE_TRUE(!item.truncated);
            REQUIRE_TRUE(width(item.text_bounds) > item.candidate_text_width_px);
            REQUIRE_TRUE(width(item.bounds) >=
                         item.horizontal_padding_px * 2 +
                         item.number_text_width_px +
                         item.number_gap_px +
                         item.candidate_text_width_px);
            REQUIRE_TRUE((candidate_text_draw_flags(item.truncated) & DT_END_ELLIPSIS) == 0);
        }
    }

    for (const size_t count : {size_t{1}, size_t{3}, size_t{9}}) {
        std::vector<std::wstring> candidates;
        for (size_t i = 0; i < count; ++i) {
            candidates.push_back(L"\u4F60\u597D");
        }
        const auto result = layout_for(candidates, middle, work);
        REQUIRE_TRUE(result.visible);
        REQUIRE_TRUE(is_inside(work, result.window_rect));
        REQUIRE_TRUE(width(result.window_rect) <= width(work));
        REQUIRE_TRUE(height(result.window_rect) <= height(work));
        REQUIRE_TRUE(result.items.size() == 1);
    }

    const auto three = layout_for({
        L"\u4F60\u597D",
        L"\u4F60\u597D\u4E16\u754C",
        L"\u6211\u60F3\u53BB\u5317\u4EAC"
    }, middle, work);
    REQUIRE_TRUE(three.visible);
    REQUIRE_EQ(three.items.size(), size_t{3});
    REQUIRE_TRUE(is_inside(work, three.window_rect));
    REQUIRE_TRUE(three.panel_padding_px >= scale_dip(12, three.dpi));
    REQUIRE_TRUE(three.corner_radius >= scale_dip(12, three.dpi));
    REQUIRE_TRUE(three.selected_corner_radius >= scale_dip(7, three.dpi));
    REQUIRE_EQ(three.composition_font_px, scale_dip(15, three.dpi));
    REQUIRE_EQ(three.candidate_font_px, scale_dip(18, three.dpi));
    REQUIRE_EQ(three.hint_font_px, scale_dip(13, three.dpi));
    REQUIRE_TRUE(three.candidate_font_px > three.composition_font_px);
    REQUIRE_TRUE(three.composition_font_px > three.hint_font_px);
    REQUIRE_TRUE(three.reserves_action_icon_space);
    REQUIRE_TRUE(three.action_button_visible);
    REQUIRE_TRUE(three.action_button_size_px >= scale_dip(28, three.dpi));
    REQUIRE_TRUE(three.action_button_gap_px >= scale_dip(8, three.dpi));
    REQUIRE_TRUE(width(three.action_button_rect) == three.action_button_size_px);
    REQUIRE_TRUE(height(three.action_button_rect) == three.action_button_size_px);
    REQUIRE_TRUE(is_inside(make_rect(0, 0, three.width, three.height), three.action_button_rect));
    REQUIRE_TRUE(rects_do_not_overlap(three.composition_rect, three.candidate_area_rect));
    REQUIRE_TRUE(!three.show_hint || rects_do_not_overlap(three.candidate_area_rect, three.hint_rect));
    REQUIRE_TRUE(rects_do_not_overlap(three.candidate_area_rect, three.action_button_rect));
    REQUIRE_TRUE(!three.show_hint || rects_do_not_overlap(three.action_button_rect, three.hint_rect));
    REQUIRE_TRUE(three.items.front().selected);
    REQUIRE_TRUE(candidate_items_do_not_overlap(three.items));
    for (const auto& item : three.items) {
        REQUIRE_TRUE(rects_do_not_overlap(item.bounds, three.action_button_rect));
    }

    const auto nine = layout_for({
        L"\u4E00", L"\u4E8C", L"\u4E09", L"\u56DB", L"\u4E94",
        L"\u516D", L"\u4E03", L"\u516B", L"\u4E5D"
    }, middle, work);
    REQUIRE_TRUE(nine.visible);
    REQUIRE_EQ(nine.items.size(), size_t{9});
    REQUIRE_TRUE(is_inside(work, nine.window_rect));
    REQUIRE_TRUE(candidate_items_do_not_overlap(nine.items));

    const auto long_word = layout_for({
        L"\u8FD9\u662F\u4E00\u4E2A\u975E\u5E38\u975E\u5E38\u957F\u7684\u5019\u9009\u8BCD\u7528\u4E8E\u9A8C\u8BC1\u622A\u65AD\u884C\u4E3A"
    }, middle, make_rect(0, 0, 360, 240));
    REQUIRE_TRUE(long_word.visible);
    REQUIRE_TRUE(width(long_word.window_rect) <= 360);
    REQUIRE_TRUE(long_word.items.front().truncated);
    REQUIRE_TRUE(long_word.items.front().candidate_text_width_px > long_word.items.front().text_available_width_px);
    REQUIRE_TRUE((candidate_text_draw_flags(long_word.items.front().truncated) & DT_END_ELLIPSIS) == DT_END_ELLIPSIS);

    const auto bottom = layout_for({L"\u4F60\u597D"}, make_rect(100, 560, 101, 580), work);
    REQUIRE_TRUE(bottom.visible);
    REQUIRE_EQ(bottom.placement, CandidatePlacement::Above);
    REQUIRE_TRUE(is_inside(work, bottom.window_rect));

    const auto top = layout_for({L"\u4F60\u597D"}, make_rect(100, 4, 101, 24), work);
    REQUIRE_TRUE(top.visible);
    REQUIRE_EQ(top.placement, CandidatePlacement::Below);
    REQUIRE_TRUE(is_inside(work, top.window_rect));

    const auto right_edge = layout_for({L"\u4F60\u597D\u4E16\u754C"}, make_rect(790, 200, 791, 220), work);
    REQUIRE_TRUE(right_edge.visible);
    REQUIRE_TRUE(is_inside(work, right_edge.window_rect));
    REQUIRE_TRUE(right_edge.rect_clamped);

    const auto left_edge = layout_for({L"\u4F60\u597D\u4E16\u754C"}, make_rect(-40, 200, -39, 220), work);
    REQUIRE_TRUE(left_edge.visible);
    REQUIRE_TRUE(is_inside(work, left_edge.window_rect));
    REQUIRE_TRUE(left_edge.rect_clamped);

    int previous_width = 0;
    for (const int dpi : {96, 120, 144, 192}) {
        const auto dpi_layout = layout_for({L"\u4F60\u597D"}, middle, make_rect(0, 0, 1600, 1200), dpi);
        REQUIRE_TRUE(dpi_layout.visible);
        REQUIRE_TRUE(dpi_layout.dpi == dpi);
        REQUIRE_TRUE(dpi_layout.width > previous_width);
        REQUIRE_TRUE(dpi_layout.action_button_visible);
        REQUIRE_TRUE(dpi_layout.action_button_size_px == scale_dip(30, dpi));
        previous_width = dpi_layout.width;
    }

    REQUIRE_TRUE(!make_candidate_palette(CandidateThemeMode::Light, true).dark);
    const auto dark_palette = make_candidate_palette(CandidateThemeMode::Dark, false);
    REQUIRE_TRUE(dark_palette.dark);
    REQUIRE_EQ(dark_palette.background, brand::kLocalPinyinBlueDarkCard_background);
    REQUIRE_EQ(dark_palette.muted_text, brand::kLocalPinyinBlueDarkMuted);
    REQUIRE_EQ(dark_palette.hint_text, brand::kLocalPinyinBlueDarkWeak);
    REQUIRE_EQ(dark_palette.text, brand::kLocalPinyinBlueDarkPrimary);
    REQUIRE_EQ(dark_palette.selected_background, brand::kLocalPinyinBlueDarkAccent);
    REQUIRE_EQ(dark_palette.selected_border, brand::kLocalPinyinBlueDarkAccent);
    REQUIRE_EQ(dark_palette.selected_text, RGB(255, 255, 255));
    REQUIRE_TRUE(make_candidate_palette(CandidateThemeMode::System, true).dark);
    REQUIRE_TRUE(!make_candidate_palette(CandidateThemeMode::System, false).dark);
    REQUIRE_EQ(candidate_theme_mode_to_string(parse_candidate_theme_mode(L"dark")), std::wstring(L"dark"));
    REQUIRE_EQ(candidate_text_size_to_string(parse_candidate_text_size(L"large")), std::wstring(L"level7"));

    const HWND first_candidate_hwnd = reinterpret_cast<HWND>(static_cast<INT_PTR>(0x101));
    const HWND replacement_candidate_hwnd = reinterpret_cast<HWND>(static_cast<INT_PTR>(0x202));
    const RECT initial_candidate_rect = make_rect(10, 20, 210, 120);
    const RECT moved_candidate_rect = make_rect(30, 40, 230, 140);
    const RECT resized_candidate_rect = make_rect(30, 40, 260, 160);

    ImeWinEventRecorder event_recorder;
    event_recorder.events.reserve(16);
    CandidateImeWinEventState event_state(record_ime_win_event, &event_recorder);

    REQUIRE_TRUE(event_state.notify_visible(first_candidate_hwnd, initial_candidate_rect));
    REQUIRE_EQ(event_recorder.events.size(), size_t{1});
    REQUIRE_EQ(event_recorder.events[0].event, static_cast<DWORD>(EVENT_OBJECT_IME_SHOW));
    REQUIRE_TRUE(event_recorder.events[0].hwnd == first_candidate_hwnd);
    REQUIRE_EQ(event_recorder.events[0].object_id, static_cast<LONG>(OBJID_CLIENT));
    REQUIRE_EQ(event_recorder.events[0].child_id, static_cast<LONG>(CHILDID_SELF));

    REQUIRE_TRUE(!event_state.notify_visible(first_candidate_hwnd, initial_candidate_rect));
    REQUIRE_TRUE(!event_state.notify_rect_changed(first_candidate_hwnd, initial_candidate_rect));
    REQUIRE_EQ(event_recorder.events.size(), size_t{1});

    REQUIRE_TRUE(event_state.notify_rect_changed(first_candidate_hwnd, moved_candidate_rect));
    REQUIRE_EQ(event_recorder.events.size(), size_t{2});
    REQUIRE_EQ(event_recorder.events[1].event, static_cast<DWORD>(EVENT_OBJECT_IME_CHANGE));
    REQUIRE_TRUE(event_recorder.events[1].hwnd == first_candidate_hwnd);
    REQUIRE_EQ(event_recorder.events[1].object_id, static_cast<LONG>(OBJID_CLIENT));
    REQUIRE_EQ(event_recorder.events[1].child_id, static_cast<LONG>(CHILDID_SELF));
    REQUIRE_TRUE(!event_state.notify_rect_changed(first_candidate_hwnd, moved_candidate_rect));

    REQUIRE_TRUE(event_state.notify_rect_changed(first_candidate_hwnd, resized_candidate_rect));
    REQUIRE_EQ(event_recorder.events.size(), size_t{3});
    REQUIRE_EQ(event_recorder.events[2].event, static_cast<DWORD>(EVENT_OBJECT_IME_CHANGE));

    REQUIRE_TRUE(event_state.notify_hidden(first_candidate_hwnd));
    REQUIRE_EQ(event_recorder.events.size(), size_t{4});
    REQUIRE_EQ(event_recorder.events[3].event, static_cast<DWORD>(EVENT_OBJECT_IME_HIDE));
    REQUIRE_TRUE(event_recorder.events[3].hwnd == first_candidate_hwnd);
    REQUIRE_EQ(event_recorder.events[3].object_id, static_cast<LONG>(OBJID_CLIENT));
    REQUIRE_EQ(event_recorder.events[3].child_id, static_cast<LONG>(CHILDID_SELF));
    REQUIRE_TRUE(!event_state.notify_hidden(first_candidate_hwnd));
    REQUIRE_TRUE(!event_state.notify_rect_changed(first_candidate_hwnd, moved_candidate_rect));
    REQUIRE_EQ(event_recorder.events.size(), size_t{4});

    ImeWinEventRecorder failed_show_recorder;
    CandidateImeWinEventState failed_show_state(record_ime_win_event, &failed_show_recorder);
    REQUIRE_TRUE(!failed_show_state.notify_visible(first_candidate_hwnd, initial_candidate_rect, false));
    REQUIRE_TRUE(failed_show_recorder.events.empty());
    REQUIRE_TRUE(!failed_show_state.notify_hidden(first_candidate_hwnd));

    ImeWinEventRecorder destroy_recorder;
    CandidateImeWinEventState destroy_state(record_ime_win_event, &destroy_recorder);
    REQUIRE_TRUE(destroy_state.notify_visible(first_candidate_hwnd, initial_candidate_rect));
    REQUIRE_TRUE(destroy_state.notify_hidden(first_candidate_hwnd));
    REQUIRE_EQ(destroy_recorder.events.size(), size_t{2});
    REQUIRE_EQ(destroy_recorder.events[0].event, static_cast<DWORD>(EVENT_OBJECT_IME_SHOW));
    REQUIRE_EQ(destroy_recorder.events[1].event, static_cast<DWORD>(EVENT_OBJECT_IME_HIDE));

    ImeWinEventRecorder destroy_without_show_recorder;
    CandidateImeWinEventState destroy_without_show_state(record_ime_win_event,
                                                          &destroy_without_show_recorder);
    REQUIRE_TRUE(!destroy_without_show_state.notify_hidden(first_candidate_hwnd));
    REQUIRE_TRUE(destroy_without_show_recorder.events.empty());

    ImeWinEventRecorder replacement_recorder;
    replacement_recorder.events.reserve(4);
    CandidateImeWinEventState replacement_state(record_ime_win_event, &replacement_recorder);
    REQUIRE_TRUE(replacement_state.notify_visible(first_candidate_hwnd, initial_candidate_rect));
    REQUIRE_TRUE(replacement_state.notify_visible(replacement_candidate_hwnd, moved_candidate_rect));
    REQUIRE_EQ(replacement_recorder.events.size(), size_t{3});
    REQUIRE_EQ(replacement_recorder.events[0].event, static_cast<DWORD>(EVENT_OBJECT_IME_SHOW));
    REQUIRE_EQ(replacement_recorder.events[1].event, static_cast<DWORD>(EVENT_OBJECT_IME_HIDE));
    REQUIRE_TRUE(replacement_recorder.events[1].hwnd == first_candidate_hwnd);
    REQUIRE_EQ(replacement_recorder.events[2].event, static_cast<DWORD>(EVENT_OBJECT_IME_SHOW));
    REQUIRE_TRUE(replacement_recorder.events[2].hwnd == replacement_candidate_hwnd);

    ImeWinEventRecorder failed_replacement_recorder;
    CandidateImeWinEventState failed_replacement_state(record_ime_win_event,
                                                        &failed_replacement_recorder);
    REQUIRE_TRUE(failed_replacement_state.notify_visible(first_candidate_hwnd,
                                                         initial_candidate_rect));
    REQUIRE_TRUE(!failed_replacement_state.notify_visible(replacement_candidate_hwnd,
                                                          moved_candidate_rect,
                                                          false));
    REQUIRE_EQ(failed_replacement_recorder.events.size(), size_t{1});
    REQUIRE_TRUE(failed_replacement_state.notified_hwnd() == first_candidate_hwnd);

    ImeWinEventRecorder host_managed_recorder;
    CandidateImeWinEventState host_managed_state(record_ime_win_event, &host_managed_recorder);
    REQUIRE_TRUE(!host_managed_state.show_notified());
    REQUIRE_TRUE(host_managed_recorder.events.empty());

    return 0;
}
