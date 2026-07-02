#pragma once

#include "candidate_ui.h"
#include "../engine/candidate.h"

#include <windows.h>

#include <string>
#include <vector>

namespace localpinyin {

class CandidateWindow {
public:
    CandidateWindow();
    ~CandidateWindow();

    bool create();
    void show(const std::wstring& composition_text,
              const std::vector<Candidate>& candidates,
              size_t selected_index,
              const RECT& caret_rect);
    void hide(const wchar_t* reason = L"explicit");
    [[nodiscard]] bool is_visible() const noexcept;

private:
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT handle_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    void reload_options();
    void update_fonts(int dpi);
    void paint();
    void draw_settings_button(HDC hdc);
    bool hit_test_settings_button(POINT point) const noexcept;
    void update_settings_button_hot(bool hot);
    void track_mouse_leave();
    void open_settings_window();
    void log_show_event(bool set_window_pos_ok, DWORD set_window_pos_error) const;
    void release_fonts();

    HWND hwnd_ = nullptr;
    std::wstring composition_text_;
    CandidateWindowOptions options_;
    CandidateThemePalette palette_;
    CandidateLayoutResult layout_;
    HFONT composition_font_ = nullptr;
    HFONT candidate_font_ = nullptr;
    HFONT hint_font_ = nullptr;
    int font_dpi_ = 0;
    CandidateTextSize font_text_size_ = CandidateTextSize::Standard;
    bool settings_button_hot_ = false;
    bool settings_button_pressed_ = false;
    bool tracking_mouse_leave_ = false;
};

}  // namespace localpinyin
