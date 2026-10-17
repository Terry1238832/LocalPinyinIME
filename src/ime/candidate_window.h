#pragma once

#include "candidate_ui.h"
#include "../engine/candidate.h"
#include "../settings/modern_renderer.h"
#include "candidate_styles/paint_core.h"

#include <windows.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <functional>
#include <vector>

namespace localpinyin {

struct CandidateWindowOwnerInfo {
    HWND context_hwnd = nullptr;
    HWND root_hwnd = nullptr;
    bool context_valid = false;
    bool root_valid = false;
};

using CandidateImeWinEventNotifier =
    void (*)(DWORD event, HWND hwnd, LONG object_id, LONG child_id, void* context) noexcept;

class CandidateImeWinEventState {
public:
    explicit CandidateImeWinEventState(CandidateImeWinEventNotifier notifier = nullptr,
                                       void* context = nullptr) noexcept
        : notifier_(notifier), context_(context) {}

    bool notify_visible(HWND hwnd, const RECT& rect, bool presentation_succeeded = true) noexcept {
        if (!presentation_succeeded || !hwnd || !notifier_) {
            return false;
        }

        if (show_notified_ && notified_hwnd_ != hwnd) {
            const HWND previous_hwnd = notified_hwnd_;
            clear_visible_state();
            notifier_(EVENT_OBJECT_IME_HIDE, previous_hwnd, OBJID_CLIENT, CHILDID_SELF, context_);
        }

        if (!show_notified_) {
            show_notified_ = true;
            notified_hwnd_ = hwnd;
            last_notified_rect_ = rect;
            last_notified_rect_valid_ = true;
            notifier_(EVENT_OBJECT_IME_SHOW, hwnd, OBJID_CLIENT, CHILDID_SELF, context_);
            return true;
        }

        return notify_rect_changed(hwnd, rect);
    }

    bool notify_rect_changed(HWND hwnd, const RECT& rect) noexcept {
        if (!show_notified_ || hwnd != notified_hwnd_ || !notifier_) {
            return false;
        }
        if (last_notified_rect_valid_ && rect_equal(last_notified_rect_, rect)) {
            return false;
        }

        last_notified_rect_ = rect;
        last_notified_rect_valid_ = true;
        notifier_(EVENT_OBJECT_IME_CHANGE, hwnd, OBJID_CLIENT, CHILDID_SELF, context_);
        return true;
    }

    bool notify_hidden(HWND hwnd) noexcept {
        if (!show_notified_ || hwnd != notified_hwnd_ || !notifier_) {
            return false;
        }

        const HWND notified_hwnd = notified_hwnd_;
        clear_visible_state();
        notifier_(EVENT_OBJECT_IME_HIDE, notified_hwnd, OBJID_CLIENT, CHILDID_SELF, context_);
        return true;
    }

    [[nodiscard]] bool show_notified() const noexcept { return show_notified_; }
    [[nodiscard]] HWND notified_hwnd() const noexcept { return notified_hwnd_; }
    [[nodiscard]] bool last_notified_rect_valid() const noexcept { return last_notified_rect_valid_; }

private:
    static bool rect_equal(const RECT& left, const RECT& right) noexcept {
        return left.left == right.left && left.top == right.top &&
               left.right == right.right && left.bottom == right.bottom;
    }

    void clear_visible_state() noexcept {
        show_notified_ = false;
        notified_hwnd_ = nullptr;
        last_notified_rect_ = RECT{};
        last_notified_rect_valid_ = false;
    }

    CandidateImeWinEventNotifier notifier_ = nullptr;
    void* context_ = nullptr;
    bool show_notified_ = false;
    HWND notified_hwnd_ = nullptr;
    RECT last_notified_rect_{};
    bool last_notified_rect_valid_ = false;
};

class CandidateWindow {
public:
    CandidateWindow();
    ~CandidateWindow();

    bool create();
    void show(const std::wstring& composition_text,
              const std::vector<Candidate>& candidates,
              size_t selected_index,
              const RECT& caret_rect,
              const CandidateWindowOwnerInfo& owner_info = {},
              const std::vector<UINT>& page_indices = {});
    // Builds the same measured page model without showing or requiring an anchor.
    CandidateLayoutResult prepare_layout(const std::wstring& composition_text,
              const std::vector<Candidate>& candidates, size_t selected_index,
              const RECT& caret_rect, const CandidateWindowOwnerInfo& owner_info = {},
              const std::vector<UINT>& page_indices = {});
    void hide(const wchar_t* reason = L"explicit");
    bool schedule_layout_retry(std::function<void()> callback,
                               unsigned int attempt,
                               UINT delay_ms);
    void cancel_layout_retry();
    [[nodiscard]] bool is_visible() const noexcept;
    void set_selection_callback(std::function<void(size_t)> callback);
    void set_page_callback(std::function<void(int)> callback) { page_callback_ = std::move(callback); }
    void set_options_changed_callback(std::function<void()> callback);
    // Called after Windows switches light/dark or high contrast.
    void set_theme_changed_callback(std::function<void()> callback) { theme_changed_callback_ = std::move(callback); }
    void set_prediction_mode(bool enabled) noexcept { prediction_mode_ = enabled; }
    // The query bar shows the pinyin, or "联想" for suggestions after a commit
    // (the focus row's hint keycap then reads Tab).
    [[nodiscard]] std::wstring query_text() const { return prediction_mode_ ? L"\u8054\u60F3" : composition_text_; }

private:
    friend class CandidateWindowCapture;
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT handle_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    bool create_with_owner(HWND requested_owner);
    HWND create_replacement_hwnd(HWND requested_owner, HWND original_hwnd);
    void destroy_hwnd_for_recreate();
    void reset_window_lifecycle_state() noexcept;
    bool position_and_show_current_layout();
    void maybe_recover_from_shell_cloak();
    void reload_options();
    void update_fonts(int dpi);
    // The layout input for the current state; shared by the IME and the
    // capture tool so both measure and lay out identically.
    CandidateLayoutInput layout_input(int dpi, const RECT& work_area);
    bool rebuild_layout(int dpi);
    void apply_options_changed();
    // The popup is a per-pixel alpha layered window: present() renders the
    // panel and its shadow and pushes them with UpdateLayeredWindow.
    bool present();
    void redraw();
    void render(HDC hdc);
    void render_layered(HDC dc, std::uint8_t* pixels, int width, int height);
    // Paints the panel in the current style (src/ime/candidate_styles).
    void paint_content();
    // Moves the style's focus marker toward its target: eased over a few
    // frames, or at once when `snap`, on first show, or when Windows has
    // animations turned off.
    void update_motion(bool snap);
    void stop_motion() noexcept;
    [[nodiscard]] bool settings_button_at_point(POINT point) const noexcept;
    void composite_shadow(std::uint8_t* pixels, int width, int height);
    [[nodiscard]] std::wstring fitted_composition_text(int available_px);
    int page_direction_at_point(POINT point) const noexcept;
    void update_pointer_hot(POINT point);
    void cancel_pointer_interaction() noexcept;
    void track_mouse_leave();
    void reset_presentation_state() noexcept;
    bool notify_ime_window_shown(HWND hwnd);
    void notify_ime_window_hidden(HWND hwnd) noexcept;
    void release_fonts();

    HWND hwnd_ = nullptr;
    HWND owner_hwnd_ = nullptr;
    std::wstring composition_text_;
    std::vector<std::wstring> candidate_texts_;
    size_t selected_index_ = 0;
    std::vector<UINT> page_indices_;
    RECT caret_rect_{};
    CandidateWindowOwnerInfo owner_info_{};
    bool has_cached_show_state_ = false;
    CandidateWindowOptions options_;
    CandidateThemePalette palette_;
    ModernRenderer renderer_;
    CandidateLayoutResult layout_;
    HFONT composition_font_ = nullptr;
    HFONT candidate_font_ = nullptr;
    int font_dpi_ = 0;
    CandidateTextSize font_text_size_ = CandidateTextSize::Standard;
    int hot_page_direction_ = 0;
    bool settings_hot_ = false;
    bool settings_pressed_ = false;
    int pressed_page_direction_ = 0;
    bool tracking_mouse_leave_ = false;
    CandidatePointerState candidate_pointer_;
    std::function<void(size_t)> selection_callback_;
    std::function<void(int)> page_callback_;
    std::function<void()> options_changed_callback_;
    std::function<void()> theme_changed_callback_;
    UINT_PTR layout_retry_timer_ = 0;
    unsigned int layout_retry_attempt_ = 0;
    std::function<void()> layout_retry_callback_;
    bool show_requested_ = false;
    bool prediction_mode_ = false;
    std::wstring focus_hint_ = L"\u7A7A\u683C";
    // Adaptive Flow's form while the popup stays up (hysteresis).
    std::optional<CandidateFlowForm> flow_form_memory_;
    // The style's moving focus (and Cursor Rail's page segment).
    struct Motion {
        RECT from{};
        RECT to{};
        RECT current{};
        bool valid = false;
    };
    Motion marker_motion_;
    Motion track_motion_;
    ULONGLONG motion_start_ = 0;
    bool motion_active_ = false;
    bool snap_motion_ = true;
    UINT_PTR motion_timer_ = 0;
    // New content (Split-Flap's turning tiles): when the visible candidates
    // last changed, and what each slot showed before that.
    std::vector<std::wstring> shown_texts_;
    std::vector<std::wstring> previous_texts_;
    ULONGLONG reveal_start_ = 0;
    bool reveal_active_ = false;
    // Offline capture only: paint the reveal frozen this long after it began.
    std::optional<ULONGLONG> reveal_frozen_ms_;
    // The blurred shadow depends only on the panel geometry: kept between frames.
    std::vector<float> shadow_cache_;
    std::array<int, 11> shadow_key_{};
    bool paint_seen_ = false;
    unsigned int shell_cloak_recovery_attempts_ = 0;
    CandidateImeWinEventState ime_win_event_state_;
};

}  // namespace localpinyin
