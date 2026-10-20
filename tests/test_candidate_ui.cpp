#include "test_common.h"
#include "../src/ime/candidate_ui.h"
#include "../src/ime/candidate_window.h"
#include "../src/ime/candidate_styles/paint_core.h"
#include "localpinyin_design_tokens.h"

#include <dwmapi.h>

#include <algorithm>
#include <optional>
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

    // Pointer selection must preserve keyboard selection and target the word
    // actually displayed, including duplicate and empty model entries.
    const std::vector<std::wstring> pointer_candidates{L"", L"你好", L"你好", L"您好", L"世界"};
    const auto pointer_layout = layout_for(pointer_candidates, make_rect(200, 200, 202, 220),
                                            make_rect(0, 0, 1920, 1080));
    REQUIRE_EQ(pointer_layout.items.size(), pointer_candidates.size());
    for (const auto& item : pointer_layout.items) {
        const POINT center{(item.bounds.left + item.bounds.right) / 2,
                             (item.bounds.top + item.bounds.bottom) / 2};
        REQUIRE_EQ(candidate_index_at_point(pointer_layout, center), std::optional<size_t>{item.candidate_index});
        REQUIRE_EQ(candidate_index_at_point(pointer_layout, POINT{item.bounds.left, item.bounds.top}),
                     std::optional<size_t>{item.candidate_index});
    }
    REQUIRE_TRUE(!candidate_index_at_point(pointer_layout, POINT{-1, -1}));
    REQUIRE_TRUE(!candidate_index_at_point(pointer_layout,
        POINT{pointer_layout.items.back().bounds.right, pointer_layout.items.back().bounds.bottom}));
    REQUIRE_TRUE(!candidate_index_at_point(pointer_layout,
        POINT{pointer_layout.action_button_rect.left + 1, pointer_layout.action_button_rect.top + 1}));
    REQUIRE_EQ(candidate_model_index(pointer_layout, 0, pointer_candidates), std::optional<size_t>{0});
    REQUIRE_EQ(candidate_model_index(pointer_layout, 1, pointer_candidates), std::optional<size_t>{1});
    REQUIRE_EQ(candidate_model_index(pointer_layout, 2, pointer_candidates), std::optional<size_t>{2});
    REQUIRE_TRUE(!candidate_model_index(pointer_layout, 9, pointer_candidates));
    auto hidden_pointer_layout = pointer_layout;
    hidden_pointer_layout.visible = false;
    REQUIRE_TRUE(!candidate_index_at_point(hidden_pointer_layout,
        POINT{pointer_layout.items.front().bounds.left, pointer_layout.items.front().bounds.top}));

    CandidatePointerState pointer;
    pointer.move(1);
    REQUIRE_EQ(pointer.hovered(), std::optional<size_t>{1});
    REQUIRE_TRUE(!pointer.pressed());
    REQUIRE_TRUE(!pointer.release(1));
    REQUIRE_TRUE(pointer_layout.items.front().selected);
    REQUIRE_TRUE(!pointer_layout.items[1].selected);
    pointer.press(1);
    REQUIRE_EQ(pointer.release(1), std::optional<size_t>{1});
    REQUIRE_TRUE(!pointer.pressed());
    pointer.press(1);
    pointer.move(2);
    REQUIRE_TRUE(!pointer.release(2));
    pointer.press(1);
    REQUIRE_TRUE(!pointer.release(std::nullopt));
    pointer.press(1);
    pointer.cancel();
    REQUIRE_TRUE(!pointer.hovered());
    REQUIRE_TRUE(!pointer.release(1));

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
        REQUIRE_TRUE(is_inside(work, result.panel_screen_rect));
        REQUIRE_TRUE(width(result.panel_screen_rect) <= width(work));
        REQUIRE_TRUE(height(result.panel_screen_rect) <= height(work));
        REQUIRE_TRUE(!result.items.empty());
        REQUIRE_TRUE(result.items.size() <= count);
    }

    const auto three = layout_for({
        L"\u4F60\u597D",
        L"\u4F60\u597D\u4E16\u754C",
        L"\u6211\u60F3\u53BB\u5317\u4EAC"
    }, middle, work);
    REQUIRE_TRUE(three.visible);
    REQUIRE_EQ(three.items.size(), size_t{3});
    REQUIRE_TRUE(is_inside(work, three.panel_screen_rect));
    // A rounded panel inside a shadow margin; the window adds the margin.
    REQUIRE_TRUE(three.shadow_margin_px == scale_dip(16, three.dpi));
    REQUIRE_EQ(width(three.window_rect), width(three.panel_screen_rect) + three.shadow_margin_px * 2);
    REQUIRE_EQ(height(three.window_rect), height(three.panel_screen_rect) + three.shadow_margin_px * 2);
    REQUIRE_EQ(three.panel_rect.left, three.shadow_margin_px);
    REQUIRE_EQ(three.panel_rect.top, three.shadow_margin_px);
    REQUIRE_TRUE(three.panel_padding_px == scale_dip(6, three.dpi));
    REQUIRE_TRUE(three.corner_radius == scale_dip(14, three.dpi));
    REQUIRE_TRUE(three.selected_corner_radius == scale_dip(12, three.dpi));
    REQUIRE_TRUE(three.chip_corner_radius == scale_dip(9, three.dpi));
    REQUIRE_EQ(three.composition_font_px, scale_dip(14, three.dpi));
    REQUIRE_EQ(three.candidate_font_px, scale_dip(18, three.dpi));
    REQUIRE_TRUE(three.candidate_font_px > three.composition_font_px);
    // The current choice is drawn larger than the alternatives.
    REQUIRE_TRUE(three.focus_font_px > three.candidate_font_px);
    // The settings button sits in the query bar, never over the candidates.
    REQUIRE_TRUE(three.reserves_action_icon_space && three.action_button_visible);
    REQUIRE_TRUE(is_inside(three.panel_rect, three.action_button_rect));
    REQUIRE_TRUE(is_inside(three.query_rect, three.action_button_rect));
    REQUIRE_TRUE(rects_do_not_overlap(three.candidate_area_rect, three.action_button_rect));
    for (const auto& item : three.items) REQUIRE_TRUE(rects_do_not_overlap(item.bounds, three.action_button_rect));
    // Query bar (brand mark, pinyin, controls) above the candidates.
    REQUIRE_TRUE(is_inside(three.panel_rect, three.composition_rect));
    REQUIRE_TRUE(is_inside(three.panel_rect, three.query_rect));
    REQUIRE_EQ(three.query_rect.left, three.panel_rect.left);
    REQUIRE_EQ(three.query_rect.right, three.panel_rect.right);
    REQUIRE_TRUE(three.query_rect.bottom <= three.candidate_area_rect.top);
    REQUIRE_TRUE(is_inside(three.query_rect, three.composition_rect));
    REQUIRE_TRUE(is_inside(three.query_rect, three.mark_rect));
    REQUIRE_TRUE(three.mark_rect.right <= three.composition_rect.left);
    REQUIRE_TRUE(three.composition_rect.right <= three.action_button_rect.left);
    // Focus row: the current choice, with the commit-key hint at its right.
    REQUIRE_TRUE(three.items.front().featured && three.items.front().selected);
    REQUIRE_TRUE(EqualRect(&three.items.front().bounds, &three.focus_rect));
    REQUIRE_TRUE(is_inside(three.candidate_area_rect, three.focus_rect));
    REQUIRE_TRUE(three.show_hint && is_inside(three.focus_rect, three.hint_rect));
    REQUIRE_TRUE(rects_do_not_overlap(three.items.front().text_bounds, three.hint_rect));
    REQUIRE_TRUE(rects_do_not_overlap(three.items.front().number_bounds, three.items.front().text_bounds));
    // Chip row: the alternatives, side by side below the focus row.
    REQUIRE_TRUE(three.row_gap_px > 0 && three.focus_rect.bottom + three.row_gap_px <= three.chips_rect.top);
    for (size_t i = 1; i < three.items.size(); ++i) {
        const auto& chip = three.items[i];
        REQUIRE_TRUE(!chip.featured && !chip.selected);
        REQUIRE_TRUE(is_inside(three.chips_rect, chip.bounds));
        REQUIRE_EQ(chip.bounds.top, three.items[1].bounds.top);
        REQUIRE_TRUE(chip.bounds.left >= three.items[i - 1].bounds.left);
    }
    // Keycap numbers fit inside their candidate item.
    for (const auto& item : three.items) REQUIRE_TRUE(is_inside(item.bounds, item.number_bounds));
    REQUIRE_TRUE(is_inside(three.panel_rect, three.candidate_area_rect));
    REQUIRE_TRUE(three.composition_rect.bottom <= three.candidate_area_rect.top);
    REQUIRE_TRUE(rects_do_not_overlap(three.composition_rect, three.candidate_area_rect));
    // A single page needs no page buttons.
    REQUIRE_TRUE(!three.paging_visible && three.page_indices.size() == 1);
    REQUIRE_TRUE(three.items.front().selected);
    REQUIRE_TRUE(candidate_items_do_not_overlap(three.items));
    for (const auto& item : three.items) REQUIRE_TRUE(is_inside(three.candidate_area_rect, item.bounds));

    // Nine single characters fit one line; nine longer words page, with
    // page buttons beside (never over) the candidates.
    // Hosts that cannot open settings (Store app sandboxes) omit the button.
    {
        CandidateLayoutInput no_button;
        no_button.composition_text = L"ni";
        no_button.candidate_texts = {L"你"};
        no_button.caret_rect = middle;
        no_button.work_area = work;
        no_button.settings_button = false;
        const auto plain = make_candidate_layout(no_button);
        REQUIRE_TRUE(plain.visible && !plain.action_button_visible && !plain.reserves_action_icon_space);
    }

    const auto nine = layout_for({
        L"\u4E00", L"\u4E8C", L"\u4E09", L"\u56DB", L"\u4E94",
        L"\u516D", L"\u4E03", L"\u516B", L"\u4E5D"
    }, middle, work);
    REQUIRE_TRUE(nine.visible);
    REQUIRE_EQ(nine.items.size(), size_t{9});
    REQUIRE_TRUE(!nine.paging_visible);
    const auto paged = layout_for({
        L"\u7B2C\u4E00\u4E2A\u5019\u9009", L"\u7B2C\u4E8C\u4E2A\u5019\u9009", L"\u7B2C\u4E09\u4E2A\u5019\u9009",
        L"\u7B2C\u56DB\u4E2A\u5019\u9009", L"\u7B2C\u4E94\u4E2A\u5019\u9009", L"\u7B2C\u516D\u4E2A\u5019\u9009",
        L"\u7B2C\u4E03\u4E2A\u5019\u9009", L"\u7B2C\u516B\u4E2A\u5019\u9009", L"\u7B2C\u4E5D\u4E2A\u5019\u9009"
    }, middle, work);
    REQUIRE_TRUE(paged.visible);
    REQUIRE_TRUE(paged.items.size() < size_t{9});
    REQUIRE_TRUE(paged.page_indices.size() > 1 && paged.paging_visible);
    REQUIRE_TRUE(!paged.has_previous_page && paged.has_next_page);
    REQUIRE_TRUE(is_inside(work, paged.panel_screen_rect));
    REQUIRE_TRUE(candidate_items_do_not_overlap(paged.items));
    REQUIRE_TRUE(is_inside(paged.panel_rect, paged.previous_page_rect));
    REQUIRE_TRUE(is_inside(paged.panel_rect, paged.next_page_rect));
    REQUIRE_TRUE(rects_do_not_overlap(paged.previous_page_rect, paged.next_page_rect));
    REQUIRE_TRUE(rects_do_not_overlap(paged.next_page_rect, paged.action_button_rect));
    REQUIRE_TRUE(paged.next_page_rect.right <= paged.action_button_rect.left);
    // The page number sits between the chevrons: "<  1/2  >".
    REQUIRE_TRUE(is_inside(paged.panel_rect, paged.page_indicator_rect));
    REQUIRE_TRUE(width(paged.page_indicator_rect) > 0);
    REQUIRE_EQ(paged.page_indicator_rect.left, paged.previous_page_rect.right);
    REQUIRE_EQ(paged.page_indicator_rect.right, paged.next_page_rect.left);
    // The pager sits in the query bar, after the pinyin and above the candidates.
    REQUIRE_TRUE(is_inside(paged.query_rect, paged.previous_page_rect));
    REQUIRE_TRUE(is_inside(paged.query_rect, paged.next_page_rect));
    REQUIRE_TRUE(paged.composition_rect.right <= paged.previous_page_rect.left);
    REQUIRE_TRUE(paged.next_page_rect.bottom <= paged.candidate_area_rect.top);
    // Without paging there is no page number.
    REQUIRE_TRUE(width(three.page_indicator_rect) == 0);
    for (const auto& item : paged.items) {
        REQUIRE_TRUE(rects_do_not_overlap(item.bounds, paged.previous_page_rect));
        REQUIRE_TRUE(rects_do_not_overlap(item.bounds, paged.next_page_rect));
    }

    // A single-row style cuts a long candidate with an ellipsis...
    CandidateWindowOptions rail_options;
    rail_options.style = CandidateStyle::CursorRail;
    const std::wstring long_text =
        L"\u8FD9\u662F\u4E00\u4E2A\u975E\u5E38\u975E\u5E38\u957F\u7684\u5019\u9009\u8BCD\u7528\u4E8E\u9A8C\u8BC1\u622A\u65AD\u884C\u4E3A";
    const auto long_word = layout_for({long_text}, middle, make_rect(0, 0, 360, 240), 96, rail_options);
    REQUIRE_TRUE(long_word.visible);
    REQUIRE_TRUE(width(long_word.panel_screen_rect) <= 360);
    REQUIRE_TRUE(long_word.items.front().truncated);
    REQUIRE_TRUE(long_word.items.front().candidate_text_width_px > long_word.items.front().text_available_width_px);
    REQUIRE_TRUE((candidate_text_draw_flags(long_word.items.front().truncated) & DT_END_ELLIPSIS) == DT_END_ELLIPSIS);
    // ...while Adaptive Flow expands and wraps it onto a second line.
    const auto long_flow = layout_for({long_text}, middle, make_rect(0, 0, 360, 240));
    REQUIRE_EQ(long_flow.flow_form, CandidateFlowForm::Expanded);
    REQUIRE_TRUE(width(long_flow.panel_screen_rect) <= 360);
    REQUIRE_TRUE(height(long_flow.items.front().text_bounds) >= long_flow.candidate_font_px * 2);
    REQUIRE_TRUE(!long_flow.items.front().truncated);
    // The candidate text needs the room more than the commit-key hint does.
    {
        CandidateLayoutInput tight;
        tight.composition_text = L"ce";
        tight.candidate_texts = {std::wstring(30, L'\u957F'), L"\u4E16\u754C"};
        tight.caret_rect = middle;
        tight.work_area = make_rect(0, 0, 360, 600);
        tight.previous_flow_form = CandidateFlowForm::Balanced;
        const auto balanced = make_candidate_layout(tight);
        REQUIRE_TRUE(!balanced.show_hint || balanced.flow_form == CandidateFlowForm::Expanded);
    }
    // Suggestions are accepted with Tab; the focus row says so.
    {
        CandidateLayoutInput suggestion;
        suggestion.composition_text = L"\u8054\u60F3";
        suggestion.candidate_texts = {L"\u4E16\u754C\u548C\u5E73", L"\u670B\u53CB", L"\u6211\u4EEC\u7684\u670B\u53CB"};
        suggestion.caret_rect = middle;
        suggestion.work_area = work;
        suggestion.focus_hint = L"Tab";
        const auto tab = make_candidate_layout(suggestion);
        REQUIRE_EQ(tab.flow_form, CandidateFlowForm::Balanced);
        REQUIRE_TRUE(tab.visible && tab.show_hint && is_inside(tab.focus_rect, tab.hint_rect));
        suggestion.focus_hint.clear();
        REQUIRE_TRUE(!make_candidate_layout(suggestion).show_hint);
    }

    const auto bottom = layout_for({L"\u4F60\u597D"}, make_rect(100, 560, 101, 580), work);
    REQUIRE_TRUE(bottom.visible);
    REQUIRE_EQ(bottom.placement, CandidatePlacement::Above);
    REQUIRE_TRUE(is_inside(work, bottom.panel_screen_rect));
    REQUIRE_TRUE(bottom.panel_screen_rect.bottom <= 560);

    const auto top = layout_for({L"\u4F60\u597D"}, make_rect(100, 4, 101, 24), work);
    REQUIRE_TRUE(top.visible);
    REQUIRE_EQ(top.placement, CandidatePlacement::Below);
    REQUIRE_TRUE(is_inside(work, top.panel_screen_rect));
    REQUIRE_TRUE(top.panel_screen_rect.top >= 24);

    const auto right_edge = layout_for({L"\u4F60\u597D\u4E16\u754C"}, make_rect(790, 200, 791, 220), work);
    REQUIRE_TRUE(right_edge.visible);
    REQUIRE_TRUE(is_inside(work, right_edge.panel_screen_rect));
    REQUIRE_TRUE(right_edge.rect_clamped);

    const auto left_edge = layout_for({L"\u4F60\u597D\u4E16\u754C"}, make_rect(-40, 200, -39, 220), work);
    REQUIRE_TRUE(left_edge.visible);
    REQUIRE_TRUE(is_inside(work, left_edge.panel_screen_rect));
    REQUIRE_TRUE(left_edge.rect_clamped);

    int previous_width = 0;
    for (const int dpi : {96, 120, 144, 192}) {
        const auto dpi_layout = layout_for({L"\u4F60\u597D"}, middle, make_rect(0, 0, 1600, 1200), dpi);
        REQUIRE_TRUE(dpi_layout.visible);
        REQUIRE_TRUE(dpi_layout.dpi == dpi);
        REQUIRE_TRUE(dpi_layout.width > previous_width);
        REQUIRE_TRUE(dpi_layout.shadow_margin_px == scale_dip(16, dpi));
        previous_width = dpi_layout.width;
    }

    // Every original candidate appears on exactly one page, in model order,
    // in every style, whatever the fonts, themes, DPI or monitor width. A
    // style changes only how a page looks: never which candidate a digit or
    // Space commits.
    for (const auto style : kCandidateStyles) {
        for (const int dpi : {96, 120, 144, 192}) {
            for (int level = 1; level <= 7; ++level) {
                for (const auto theme : {CandidateThemeMode::Light, CandidateThemeMode::Dark, CandidateThemeMode::System}) {
                    for (const int work_width : {80, 160, 360, 800, 1920}) {
                        CandidateLayoutInput input;
                        input.composition_text = std::wstring(128, L'a');
                        input.candidate_texts = {L"一", L"你好", L"你好", L"", std::wstring(80, L'长'),
                            L"第六候选", L"第七候选", L"第八候选", L"第九候选"};
                        input.work_area = make_rect(30, 40, 30 + work_width, 600);
                        input.caret_rect = make_rect(900, 590, 901, 610);
                        input.dpi = dpi;
                        input.options.theme_mode = theme;
                        input.options.text_size = candidate_text_size_from_level(level);
                        input.options.style = style;
                        const auto first = make_candidate_layout(input);
                        REQUIRE_TRUE(first.visible);
                        REQUIRE_EQ(first.style, style);
                        REQUIRE_TRUE(is_inside(input.work_area, first.panel_screen_rect));
                        REQUIRE_EQ(first.page_indices.front(), UINT{0});
                        // Column List stacks one candidate per row.
                        const bool stacked = style == CandidateStyle::ColumnList;
                        const bool one_row = !stacked && (style != CandidateStyle::AdaptiveFlow ||
                                                          first.flow_form == CandidateFlowForm::Compact);
                        std::vector<size_t> visited;
                        for (size_t page = 0; page < first.page_indices.size(); ++page) {
                            input.selected_index = first.page_indices[page];
                            input.page_indices = first.page_indices;
                            const auto current = make_candidate_layout(input);
                            input.page_indices.clear();
                            REQUIRE_EQ(current.current_page, page);
                            REQUIRE_EQ(current.page_indices, first.page_indices);
                            REQUIRE_TRUE(!current.items.empty());
                            REQUIRE_TRUE(is_inside(input.work_area, current.panel_screen_rect));
                            REQUIRE_TRUE(is_inside(current.panel_rect, current.composition_rect));
                            REQUIRE_TRUE(is_inside(make_rect(0, 0, current.width, current.height), current.panel_rect));
                            REQUIRE_EQ(current.paging_visible, first.page_indices.size() > 1);
                            // A one-row style keeps its height from page to page, and
                            // so does the column: the fullest page sets it.
                            if (one_row || stacked) REQUIRE_EQ(current.height, first.height);
                            REQUIRE_TRUE(candidate_items_do_not_overlap(current.items));
                            REQUIRE_TRUE(current.items.front().selected && current.items.front().featured);
                            for (size_t slot = 0; slot < current.items.size(); ++slot) {
                                const auto& item = current.items[slot];
                                REQUIRE_EQ(item.page_number, slot + 1);
                                REQUIRE_EQ(item.featured, slot == 0);
                                REQUIRE_TRUE(is_inside(current.panel_rect, item.bounds));
                                if (one_row) {
                                    REQUIRE_EQ(item.bounds.top, current.items.front().bounds.top);
                                    REQUIRE_EQ(item.bounds.bottom, current.items.front().bounds.bottom);
                                } else if (stacked) {
                                    REQUIRE_EQ(item.bounds.left, current.items.front().bounds.left);
                                    if (slot > 0) REQUIRE_TRUE(item.bounds.top >= current.items[slot - 1].bounds.bottom);
                                } else if (current.flow_form == CandidateFlowForm::Balanced && slot > 0) {
                                    REQUIRE_EQ(item.bounds.top, current.items[1].bounds.top);
                                    REQUIRE_TRUE(item.bounds.top >= current.focus_rect.bottom);
                                } else if (current.flow_form == CandidateFlowForm::Expanded && slot > 0) {
                                    REQUIRE_TRUE(item.bounds.top >= current.items[slot - 1].bounds.bottom);
                                }
                                REQUIRE_EQ(item.display_text, input.candidate_texts[item.candidate_index]);
                                REQUIRE_EQ(candidate_model_index(current, item.candidate_index, input.candidate_texts),
                                    std::optional<size_t>{item.candidate_index});
                                if (item.candidate_index == 4) {
                                    REQUIRE_TRUE(item.truncated || current.flow_form == CandidateFlowForm::Expanded);
                                    REQUIRE_EQ(item.display_text.size(), size_t{80});
                                }
                                visited.push_back(item.candidate_index);
                            }
                        }
                        REQUIRE_EQ(visited.size(), input.candidate_texts.size());
                        for (size_t i = 0; i < visited.size(); ++i) REQUIRE_EQ(visited[i], i);
                    }
                }
            }
        }
    }

    // A page holds only what fits: short words are never squeezed into
    // ellipses, however tightly the page is filled.
    for (const auto style : kCandidateStyles) {
        CandidateWindowOptions options;
        options.style = style;
        for (const int dpi : {96, 144}) {
            for (int work_width = 420; work_width <= 1200; work_width += 2) {
                const auto full = layout_for({L"你好世界", L"你好", L"拟好", L"你", L"泥", L"尼", L"妮", L"逆", L"倪",
                                              L"腻", L"匿", L"昵"}, middle, make_rect(0, 0, work_width, 1080), dpi, options);
                for (const auto& item : full.items) REQUIRE_TRUE(!item.truncated);
            }
        }
    }

    // A host may force every candidate onto one page of a tiny work area.
    for (const auto style : kCandidateStyles) {
        CandidateLayoutInput forced_host;
        forced_host.composition_text = L"longcompositionlabel";
        forced_host.candidate_texts.assign(9, std::wstring(80, L'词'));
        forced_host.page_indices = {0};
        forced_host.options.text_size = CandidateTextSize::Level7;
        forced_host.options.style = style;
        forced_host.dpi = 192;
        forced_host.work_area = make_rect(0, 0, 240, 200);
        forced_host.caret_rect = make_rect(230, 180, 231, 200);
        const auto forced = make_candidate_layout(forced_host);
        REQUIRE_EQ(forced.items.size(), size_t{9});
        REQUIRE_EQ(forced.page_indices.size(), size_t{1});
        REQUIRE_TRUE(candidate_items_do_not_overlap(forced.items));
        REQUIRE_TRUE(is_inside(forced_host.work_area, forced.panel_screen_rect));
        for (size_t i = 0; i < forced.items.size(); ++i) {
            const auto& item = forced.items[i];
            REQUIRE_TRUE(width(item.bounds) > 0 && item.truncated);
            REQUIRE_EQ(item.candidate_index, i);
            REQUIRE_EQ(candidate_model_index(forced, i, forced_host.candidate_texts), std::optional<size_t>{i});
        }
    }

    // Cursor Rail: candidates on one rail; the focus line rests under the
    // current choice and moves to the candidate under the pointer.
    {
        CandidateWindowOptions options;
        options.style = CandidateStyle::CursorRail;
        const auto rail = layout_for({L"你好世界", L"你好", L"拟好", L"你"}, middle, work, 96, options);
        REQUIRE_TRUE(rail.visible && !rail.paging_visible);
        REQUIRE_TRUE(rail.focus_font_px > rail.candidate_font_px && rail.focus_weight > rail.item_weight);
        REQUIRE_TRUE(is_inside(rail.panel_rect, rail.rail_rect) && height(rail.rail_rect) == 1);
        REQUIRE_TRUE(candidate_paint::is_empty(rail.track_rect));
        for (const auto& item : rail.items) {
            REQUIRE_TRUE(item.text_bounds.bottom <= rail.rail_rect.top);
            REQUIRE_TRUE(rail.baseline_y > item.text_bounds.top && rail.baseline_y <= rail.rail_rect.top);
        }
        const RECT rest = candidate_marker_target(rail, std::nullopt);
        REQUIRE_TRUE(rest.left == rail.items[0].number_bounds.left && rest.right <= rail.items[0].bounds.right);
        REQUIRE_TRUE(rest.top <= rail.rail_rect.top && rest.bottom > rail.rail_rect.top);
        const RECT hover = candidate_marker_target(rail, rail.items[2].candidate_index);
        REQUIRE_TRUE(hover.left == rail.items[2].number_bounds.left && hover.top == rest.top);
        // Pages show on a track under the rail, the current page's segment lit.
        std::vector<std::wstring> many;
        for (int i = 0; i < 9; ++i) many.push_back(std::wstring(12, static_cast<wchar_t>(L'甲' + i)));
        auto paged_input = CandidateLayoutInput{};
        paged_input.composition_text = L"jia";
        paged_input.candidate_texts = many;
        paged_input.caret_rect = middle;
        paged_input.work_area = work;
        paged_input.options = options;
        const auto first_page = make_candidate_layout(paged_input);
        REQUIRE_TRUE(first_page.paging_visible && !candidate_paint::is_empty(first_page.track_rect));
        REQUIRE_TRUE(first_page.track_rect.top >= first_page.rail_rect.bottom);
        const RECT segment = candidate_track_target(first_page);
        REQUIRE_TRUE(is_inside(first_page.track_rect, segment) && segment.left == first_page.track_rect.left);
        paged_input.selected_index = first_page.page_indices[1];
        paged_input.page_indices = first_page.page_indices;
        const auto second_page = make_candidate_layout(paged_input);
        REQUIRE_TRUE(candidate_track_target(second_page).left > segment.left);
    }

    // Modern Type: one baseline for every size, superscript figures in Segoe
    // UI, a hairline under the pinyin and a short rule under the choice.
    {
        CandidateWindowOptions options;
        options.style = CandidateStyle::ModernType;
        const auto type = layout_for({L"你好世界", L"你好", L"拟好", L"你"}, middle, work, 96, options);
        REQUIRE_TRUE(type.visible);
        REQUIRE_EQ(type.number_face, CandidateFontFace::Numeral);
        REQUIRE_EQ(type.focus_font_px, static_cast<int>(type.candidate_font_px * 1.3f + 0.5f));
        REQUIRE_TRUE(type.focus_weight == 700 && type.item_weight == 300);
        REQUIRE_TRUE(type.rail_rect.top >= type.composition_rect.bottom - 1 && type.rail_rect.bottom <= type.candidate_area_rect.top);
        for (const auto& item : type.items) {
            REQUIRE_TRUE(type.baseline_y > item.text_bounds.top && type.baseline_y < item.text_bounds.bottom);
            REQUIRE_TRUE(item.number_bounds.bottom <= type.baseline_y);
        }
        const RECT rule = candidate_marker_target(type, std::nullopt);
        REQUIRE_TRUE(rule.top > type.baseline_y && rule.left == type.items[0].text_bounds.left);
        REQUIRE_TRUE(width(rule) <= type.items[0].candidate_text_width_px && width(rule) > 0);
        const RECT moved = candidate_marker_target(type, type.items[3].candidate_index);
        REQUIRE_TRUE(moved.left == type.items[3].text_bounds.left && moved.top == rule.top);
    }

    // Floating Glass: the ceramic key stays with the current choice.
    {
        CandidateWindowOptions options;
        options.style = CandidateStyle::FloatingGlass;
        const auto glass = layout_for({L"你好世界", L"你好", L"拟好", L"你"}, middle, work, 96, options);
        REQUIRE_TRUE(glass.visible && glass.corner_radius == scale_dip(16, 96));
        const RECT key = candidate_marker_target(glass, std::nullopt);
        REQUIRE_TRUE(EqualRect(&key, &glass.items[0].bounds));
        const RECT still = candidate_marker_target(glass, glass.items[2].candidate_index);
        REQUIRE_TRUE(EqualRect(&still, &key));
        REQUIRE_TRUE(candidate_paint::is_empty(candidate_track_target(glass)));
        REQUIRE_TRUE(make_candidate_palette(CandidateThemeMode::Light, false, CandidateStyle::FloatingGlass).focus_shadow_alpha > 0);
    }

    // Capsule: one fully rounded bar, the pinyin at its left end; the solid
    // pill stays with the current choice.
    {
        CandidateWindowOptions options;
        options.style = CandidateStyle::Capsule;
        const auto capsule = layout_for({L"你好世界", L"你好", L"拟好", L"你"}, middle, work, 96, options);
        REQUIRE_TRUE(capsule.visible && capsule.corner_radius == height(capsule.panel_rect) / 2);
        REQUIRE_TRUE(capsule.composition_rect.right <= capsule.mark_rect.left);
        REQUIRE_TRUE(capsule.mark_rect.right <= capsule.items.front().bounds.left);
        for (const auto& item : capsule.items) REQUIRE_EQ(item.bounds.top, capsule.items.front().bounds.top);
        const RECT pill = candidate_marker_target(capsule, capsule.items[2].candidate_index);
        REQUIRE_TRUE(EqualRect(&pill, &capsule.items[0].bounds));
        const auto palette = make_candidate_palette(CandidateThemeMode::Light, false, CandidateStyle::Capsule);
        REQUIRE_TRUE(palette.focus_background == palette.focus_bar && palette.focus_text != palette.text);
    }

    // Column List: one candidate per row under the pinyin, at full width.
    {
        CandidateWindowOptions options;
        options.style = CandidateStyle::ColumnList;
        const auto column = layout_for({L"你好世界", L"你好", L"拟好", L"你"}, middle, work, 96, options);
        REQUIRE_TRUE(column.visible && column.items.size() == 4);
        REQUIRE_TRUE(column.composition_rect.bottom <= column.items.front().bounds.top);
        for (size_t slot = 1; slot < column.items.size(); ++slot) {
            REQUIRE_TRUE(column.items[slot].bounds.top >= column.items[slot - 1].bounds.bottom);
            REQUIRE_EQ(width(column.items[slot].bounds), width(column.items.front().bounds));
        }
        const RECT row = candidate_marker_target(column, column.items[3].candidate_index);
        REQUIRE_TRUE(EqualRect(&row, &column.items[0].bounds));
        // Seven candidates fill one six-row page and part of another; both
        // pages are as tall as the full one.
        const auto seven = layout_for({L"一", L"二", L"三", L"四", L"五", L"六", L"七"}, middle, work, 96, options);
        REQUIRE_EQ(seven.page_indices.size(), size_t{2});
        REQUIRE_EQ(seven.items.size(), size_t{6});
    }

    // Capsule and Column List draw the current choice semibold, which runs
    // wider: it still gets the room it needs ("what a beautiful" kept its l).
    for (const auto style : {CandidateStyle::Capsule, CandidateStyle::ColumnList}) {
        const auto width_at = [](const std::wstring& text, int font_px, int weight) {
            return static_cast<int>(text.size()) * font_px * (weight >= 600 ? 62 : 52) / 100;
        };
        CandidateLayoutInput input;
        input.composition_text = L"what a beautifule";
        input.candidate_texts = {L"what a beautiful", L"what a beautifule"};
        input.caret_rect = middle;
        input.work_area = work;
        input.options.style = style;
        input.measure = [&](const std::wstring& text, int font_px, int weight, CandidateFontFace) {
            return width_at(text, font_px, weight);
        };
        for (const size_t selected : {size_t{0}, size_t{1}}) {
            input.selected_index = selected;
            const auto layout = make_candidate_layout(input);
            REQUIRE_EQ(layout.items.size(), size_t{2});
            for (const auto& item : layout.items) {
                REQUIRE_TRUE(!item.truncated);
                REQUIRE_TRUE(width(item.text_bounds) >= width_at(item.display_text, layout.candidate_font_px, FW_SEMIBOLD));
            }
        }
    }

    // Split-Flap: every character on a tile of one shared width; long
    // candidates show as many tiles as fit. Nothing slides: the amber tiles
    // stay with the current choice.
    {
        CandidateWindowOptions options;
        options.style = CandidateStyle::SplitFlap;
        const auto flap = layout_for({L"你好世界", L"你好", L"拟好", L"你"}, middle, work, 96, options);
        REQUIRE_TRUE(flap.visible && flap.composition_rect.bottom <= flap.items.front().text_bounds.top);
        const int tile = flap.items.front().horizontal_padding_px;
        const int gap = scale_dip(2, 96);
        REQUIRE_TRUE(tile >= flap.candidate_font_px);
        REQUIRE_EQ(width(flap.items[0].text_bounds), 4 * tile + 3 * gap);
        REQUIRE_EQ(width(flap.items[3].text_bounds), tile);
        for (const auto& item : flap.items) {
            REQUIRE_EQ(item.horizontal_padding_px, tile);
            REQUIRE_TRUE(!item.truncated && is_inside(item.bounds, item.text_bounds));
        }
        REQUIRE_TRUE(candidate_paint::is_empty(candidate_marker_target(flap, flap.items[2].candidate_index)));
        REQUIRE_TRUE(candidate_style_reveals(CandidateStyle::SplitFlap) && !candidate_style_reveals(CandidateStyle::CursorRail));
        // A long candidate shows fewer tiles and is marked cut.
        const auto long_flap = layout_for({std::wstring(60, L'长'), L"你好"}, middle, make_rect(0, 0, 500, 400), 96, options);
        REQUIRE_TRUE(long_flap.items.front().truncated);
        REQUIRE_TRUE(width(long_flap.items.front().text_bounds) % (tile + gap) == tile);
        const auto palette = make_candidate_palette(CandidateThemeMode::Dark, false, CandidateStyle::SplitFlap);
        REQUIRE_TRUE(palette.focus_background != palette.inner_edge && palette.surface_top != palette.inner_edge);
    }

    // Highlighter: a stroke behind the lower part of the current word,
    // a little past both ends; it moves to the word under the pointer.
    {
        CandidateWindowOptions options;
        options.style = CandidateStyle::Highlighter;
        const auto marker = layout_for({L"你好世界", L"你好", L"拟好", L"你"}, middle, work, 96, options);
        REQUIRE_TRUE(marker.visible && !candidate_paint::is_empty(marker.rail_rect));
        const auto covers = [&](const RECT& band, const CandidateItemLayout& item) {
            const int center = (item.text_bounds.top + item.text_bounds.bottom) / 2;
            return band.left < item.text_bounds.left && band.right > item.text_bounds.left &&
                   band.top < center + marker.candidate_font_px / 2 && band.bottom > center &&
                   is_inside(item.bounds, band);
        };
        REQUIRE_TRUE(covers(candidate_marker_target(marker, std::nullopt), marker.items[0]));
        REQUIRE_TRUE(covers(candidate_marker_target(marker, marker.items[2].candidate_index), marker.items[2]));
        REQUIRE_TRUE(candidate_style_reveals(CandidateStyle::Highlighter));
    }

    // Adaptive Flow chooses its form from the measured candidates...
    {
        CandidateFlowMeasures short_words{18, 36, 18, 700};
        REQUIRE_EQ(decide_candidate_flow_form(short_words, std::nullopt), CandidateFlowForm::Compact);
        CandidateFlowMeasures four{18, 72, 36, 700};
        // ...with hysteresis: a four-character word does not leave Compact,
        // but does not enter it either.
        REQUIRE_EQ(decide_candidate_flow_form(four, std::nullopt), CandidateFlowForm::Balanced);
        REQUIRE_EQ(decide_candidate_flow_form(four, CandidateFlowForm::Compact), CandidateFlowForm::Compact);
        CandidateFlowMeasures sentence{18, 320, 90, 700};
        REQUIRE_EQ(decide_candidate_flow_form(sentence, std::nullopt), CandidateFlowForm::Expanded);
        CandidateFlowMeasures shorter{18, 260, 90, 700};
        REQUIRE_EQ(decide_candidate_flow_form(shorter, std::nullopt), CandidateFlowForm::Balanced);
        REQUIRE_EQ(decide_candidate_flow_form(shorter, CandidateFlowForm::Expanded), CandidateFlowForm::Expanded);
        CandidateFlowMeasures much_shorter{18, 120, 60, 700};
        REQUIRE_EQ(decide_candidate_flow_form(much_shorter, CandidateFlowForm::Expanded), CandidateFlowForm::Balanced);
        // A decision is stable: deciding again from its own result keeps it.
        for (const auto& measures : {short_words, four, sentence, shorter, much_shorter}) {
            for (const auto previous : {std::optional<CandidateFlowForm>{}, std::optional{CandidateFlowForm::Compact},
                                        std::optional{CandidateFlowForm::Balanced}, std::optional{CandidateFlowForm::Expanded}}) {
                const auto once = decide_candidate_flow_form(measures, previous);
                REQUIRE_EQ(decide_candidate_flow_form(measures, once), once);
            }
        }
        // Every form keeps the pinyin under the caret: the panel does not jump
        // sideways when the form changes.
        const RECT caret = make_rect(300, 200, 301, 220);
        const auto compact = layout_for({L"你", L"泥", L"尼"}, caret, work);
        const auto balanced = layout_for({L"你好世界", L"你好", L"拟好"}, caret, work);
        const auto expanded = layout_for({std::wstring(22, L'句'), L"你好"}, caret, work);
        REQUIRE_EQ(compact.flow_form, CandidateFlowForm::Compact);
        REQUIRE_EQ(balanced.flow_form, CandidateFlowForm::Balanced);
        REQUIRE_EQ(expanded.flow_form, CandidateFlowForm::Expanded);
        for (const auto* layout : {&compact, &balanced, &expanded}) {
            REQUIRE_EQ(layout->panel_screen_rect.left + (layout->composition_rect.left - layout->shadow_margin_px), caret.left);
            REQUIRE_EQ(layout->panel_screen_rect.top, balanced.panel_screen_rect.top);
            // The drop rests on the current choice, hugging its text.
            const RECT drop = candidate_marker_target(*layout, std::nullopt);
            const auto& first = layout->items.front();
            REQUIRE_TRUE(first.featured && !candidate_paint::is_empty(drop) && is_inside(first.bounds, drop));
            REQUIRE_TRUE(drop.left == first.bounds.left && drop.right > first.text_bounds.left);
        }
        REQUIRE_TRUE(compact.height < balanced.height);
        // Balanced's large row is longer than its short word: the drop ends
        // after the text, not at the row's end.
        {
            const auto short_first = layout_for({L"你好", L"你好世界啊", L"拟好"}, caret, work);
            if (short_first.flow_form == CandidateFlowForm::Balanced) {
                const RECT drop = candidate_marker_target(short_first, std::nullopt);
                REQUIRE_TRUE(drop.right < short_first.items.front().bounds.right);
            }
        }
        // Compact and Expanded: the drop flows to the candidate under the
        // pointer; Balanced keeps it on its large row.
        REQUIRE_TRUE(is_inside(compact.items[2].bounds, candidate_marker_target(compact, compact.items[2].candidate_index)));
        REQUIRE_TRUE(is_inside(expanded.items[1].bounds, candidate_marker_target(expanded, expanded.items[1].candidate_index)));
        const RECT balanced_hovered = candidate_marker_target(balanced, balanced.items[2].candidate_index);
        const RECT balanced_rest = candidate_marker_target(balanced, std::nullopt);
        REQUIRE_TRUE(EqualRect(&balanced_hovered, &balanced_rest));
        REQUIRE_TRUE(candidate_style_reveals(CandidateStyle::AdaptiveFlow));
        REQUIRE_TRUE(candidate_style_motion_ms(CandidateStyle::AdaptiveFlow) > candidate_style_motion_ms(CandidateStyle::CursorRail));
        // On its way the drop's leading edge runs ahead of an even glide and
        // its trailing edge behind, so it is longer than either end; it
        // arrives exactly, from where it started.
        {
            const RECT from = make_rect(100, 50, 160, 80);
            const RECT to = make_rect(260, 50, 300, 80);
            const RECT start = candidate_marker_frame(CandidateStyle::AdaptiveFlow, from, to, 0.0f);
            REQUIRE_TRUE(EqualRect(&start, &from));
            const RECT end = candidate_marker_frame(CandidateStyle::AdaptiveFlow, from, to, 1.0f);
            REQUIRE_TRUE(EqualRect(&end, &to));
            const RECT mid = candidate_marker_frame(CandidateStyle::AdaptiveFlow, from, to, 0.35f);
            REQUIRE_TRUE(mid.right > 160 + (300 - 160) * 35 / 100 && mid.left < 100 + (260 - 100) * 35 / 100);
            REQUIRE_TRUE(width(mid) > width(from) && width(mid) > width(to));
            REQUIRE_TRUE(mid.top == 50 && mid.bottom == 80);
            // Moving left, the left edge leads.
            const RECT back = candidate_marker_frame(CandidateStyle::AdaptiveFlow, to, from, 0.35f);
            REQUIRE_TRUE(back.left < 260 - (260 - 100) * 35 / 100 && back.right > 300 - (300 - 160) * 35 / 100);
            // Other styles glide, both edges together: never longer than either end.
            const RECT glide = candidate_marker_frame(CandidateStyle::Capsule, from, to, 0.35f);
            REQUIRE_TRUE(width(glide) <= (std::max)(width(from), width(to)) && width(glide) >= (std::min)(width(from), width(to)));
        }
    }

    // Styles are saved by id; anything unknown falls back to the default.
    for (const auto style : kCandidateStyles)
        REQUIRE_EQ(parse_candidate_style(candidate_style_to_string(style)), style);
    REQUIRE_EQ(parse_candidate_style(L""), kDefaultCandidateStyle);
    REQUIRE_EQ(parse_candidate_style(L"aurora"), kDefaultCandidateStyle);
    REQUIRE_EQ(parse_candidate_style(L"RAIL"), CandidateStyle::CursorRail);
    REQUIRE_EQ(kDefaultCandidateStyle, CandidateStyle::AdaptiveFlow);
    REQUIRE_EQ(std::wstring(candidate_font_family(CandidateFontFace::Numeral)), std::wstring(L"Segoe UI"));

    REQUIRE_TRUE(!make_candidate_palette(CandidateThemeMode::Light, true).dark);
    const auto dark_palette = make_candidate_palette(CandidateThemeMode::Dark, false);
    REQUIRE_TRUE(dark_palette.dark);
    // Every palette comes from the shared design tokens; the default is Flow's.
    REQUIRE_EQ(dark_palette.background, tokens::candidate_flow_dark::kBackground);
    REQUIRE_EQ(dark_palette.text, tokens::candidate_flow_dark::kText);
    REQUIRE_EQ(dark_palette.focus_background, tokens::candidate_flow_dark::kFocus);
    REQUIRE_EQ(dark_palette.caret, tokens::candidate_flow_dark::kCaret);
    REQUIRE_EQ(dark_palette.focus_number_text, tokens::candidate_flow_dark::kFocusNumber);
    // Flow's text on its drop is white.
    REQUIRE_EQ(dark_palette.focus_text, RGB(255, 255, 255));
    const auto light_palette = make_candidate_palette(CandidateThemeMode::Light, false);
    REQUIRE_EQ(light_palette.focus_background, tokens::candidate_flow_light::kFocus);
    REQUIRE_EQ(light_palette.chip_background, tokens::candidate_flow_light::kChip);
    REQUIRE_EQ(make_candidate_palette(CandidateThemeMode::Dark, false, CandidateStyle::CursorRail).background,
               tokens::candidate_rail_dark::kBackground);
    REQUIRE_EQ(make_candidate_palette(CandidateThemeMode::Light, false, CandidateStyle::ModernType).focus_bar,
               tokens::candidate_type_light::kFocusBar);
    REQUIRE_EQ(make_candidate_palette(CandidateThemeMode::Dark, false, CandidateStyle::FloatingGlass).focus_top,
               tokens::candidate_glass_dark::kFocusTop);
    // Each style has its own palette in both themes.
    for (const bool dark : {false, true}) {
        const auto mode = dark ? CandidateThemeMode::Dark : CandidateThemeMode::Light;
        std::vector<COLORREF> backgrounds;
        for (const auto style : kCandidateStyles) {
            const auto palette = make_candidate_palette(mode, false, style);
            REQUIRE_EQ(palette.dark, dark);
            backgrounds.push_back(palette.background);
        }
        std::sort(backgrounds.begin(), backgrounds.end());
        REQUIRE_TRUE(std::adjacent_find(backgrounds.begin(), backgrounds.end()) == backgrounds.end());
    }
    // The focus is a drop: a fill shading down to the focus color under white
    // text, and a light trace where the current choice is while it is away.
    REQUIRE_TRUE(light_palette.focus_top != light_palette.focus_background);
    REQUIRE_EQ(light_palette.focus_text, RGB(255, 255, 255));
    REQUIRE_TRUE(light_palette.focus_border != light_palette.background);
    REQUIRE_TRUE(light_palette.focus_border != light_palette.focus_background);
    REQUIRE_EQ(light_palette.focus_bar, light_palette.caret);
    REQUIRE_EQ(dark_palette.focus_bar, dark_palette.caret);
    REQUIRE_TRUE(light_palette.chip_background != light_palette.background);
    REQUIRE_TRUE(dark_palette.chip_background != dark_palette.background);
    REQUIRE_EQ(light_palette.background, RGB(255,255,255));
    REQUIRE_TRUE(light_palette.highlight == light_palette.background && dark_palette.highlight != dark_palette.background);
    REQUIRE_TRUE(light_palette.shadow_ambient_alpha > 0 && dark_palette.shadow_ambient_alpha > light_palette.shadow_ambient_alpha);
    REQUIRE_TRUE(dark_palette.shadow_key_alpha > light_palette.shadow_key_alpha);
    // High contrast: system colors, no material.
    const auto contrast = make_high_contrast_candidate_palette();
    REQUIRE_TRUE(contrast.high_contrast && contrast.shadow_key_alpha == 0 && contrast.focus_shadow_alpha == 0);
    REQUIRE_EQ(contrast.focus_background, GetSysColor(COLOR_HIGHLIGHT));
    REQUIRE_EQ(contrast.text, GetSysColor(COLOR_WINDOWTEXT));
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
