#pragma once

#include "../engine/candidate.h"

#include <windows.h>

#include <vector>

namespace localpinyin {

class CandidateWindow {
public:
    CandidateWindow();
    ~CandidateWindow();

    bool create();
    void show(const std::vector<Candidate>& candidates, size_t selected_index, POINT anchor);
    void hide();
    [[nodiscard]] bool is_visible() const noexcept;

private:
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam);
    void paint();

    HWND hwnd_ = nullptr;
    std::vector<Candidate> candidates_;
    size_t selected_index_ = 0;
};

}  // namespace localpinyin
