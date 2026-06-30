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
    void log_show_event(bool set_window_pos_ok, DWORD set_window_pos_error) const;
    void release_fonts();

    HWND hwnd_ = nullptr;
    std::wstring composition_text_;
    CandidateWindowOptions options_;
    CandidateThemePalette palette_;
    CandidateLayoutResult layout_;
    HFONT candidate_font_ = nullptr;
    HFONT hint_font_ = nullptr;
    int font_dpi_ = 0;
    CandidateTextSize font_text_size_ = CandidateTextSize::Standard;
};

}  // namespace localpinyin
