#include "test_common.h"
#include "../src/ime/candidate_ui.h"
#include "local_pinyin_blue_theme.h"

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

    const std::wstring attempt_log = make_candidate_window_create_attempt_log(popup_config);
    REQUIRE_TRUE(attempt_log.find(L"create_parent_kind=null") != std::wstring::npos);
    REQUIRE_TRUE(attempt_log.find(L"create_menu_is_null=true") != std::wstring::npos);
    REQUIRE_TRUE(attempt_log.find(L"create_style_popup=true") != std::wstring::npos);
    REQUIRE_TRUE(attempt_log.find(L"create_ex_noactivate=true") != std::wstring::npos);
    REQUIRE_TRUE(attempt_log.find(L"create_ex_toolwindow=true") != std::wstring::npos);

    const std::wstring failure_log = make_candidate_window_create_result_log(false, ERROR_INVALID_WINDOW_HANDLE);
    REQUIRE_TRUE(failure_log.find(L"window_created=false") != std::wstring::npos);
    REQUIRE_TRUE(failure_log.find(L"create_last_error=1400") != std::wstring::npos);
    REQUIRE_TRUE(failure_log.find(L"nihao") == std::wstring::npos);
    REQUIRE_TRUE(failure_log.find(L"\u4F60\u597D") == std::wstring::npos);

    const auto shown_status = make_candidate_window_show_status(true);
    REQUIRE_TRUE(shown_status.set_window_pos_ok);
    REQUIRE_TRUE(shown_status.show_window_called);
    REQUIRE_TRUE(shown_status.shown);

    const auto failed_show_status = make_candidate_window_show_status(false);
    REQUIRE_TRUE(!failed_show_status.set_window_pos_ok);
    REQUIRE_TRUE(!failed_show_status.show_window_called);
    REQUIRE_TRUE(!failed_show_status.shown);

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
    REQUIRE_TRUE(three.corner_radius >= scale_dip(14, three.dpi));
    REQUIRE_TRUE(three.selected_corner_radius >= scale_dip(8, three.dpi));
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
    REQUIRE_EQ(dark_palette.selected_background, brand::kLocalPinyinBlueDarkAccent_soft);
    REQUIRE_EQ(dark_palette.selected_border, brand::kLocalPinyinBlueDarkAccent_border);
    REQUIRE_TRUE(make_candidate_palette(CandidateThemeMode::System, true).dark);
    REQUIRE_TRUE(!make_candidate_palette(CandidateThemeMode::System, false).dark);
    REQUIRE_EQ(candidate_theme_mode_to_string(parse_candidate_theme_mode(L"dark")), std::wstring(L"dark"));
    REQUIRE_EQ(candidate_text_size_to_string(parse_candidate_text_size(L"large")), std::wstring(L"large"));

    return 0;
}
