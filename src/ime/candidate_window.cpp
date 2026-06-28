#include "candidate_window.h"

#include "globals.h"

#include <algorithm>
#include <string>

namespace localpinyin {

CandidateWindow::CandidateWindow() = default;

CandidateWindow::~CandidateWindow() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
    }
}

bool CandidateWindow::create() {
    if (hwnd_) {
        return true;
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = CandidateWindow::window_proc;
    wc.hInstance = g_instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"LocalPinyinCandidateWindow";
    RegisterClassExW(&wc);

    hwnd_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                            wc.lpszClassName, L"", WS_POPUP,
                            CW_USEDEFAULT, CW_USEDEFAULT, 280, 36,
                            nullptr, nullptr, g_instance, this);
    return hwnd_ != nullptr;
}

void CandidateWindow::show(const std::vector<Candidate>& candidates, size_t selected_index, POINT anchor) {
    if (!create() || candidates.empty()) {
        hide();
        return;
    }
    candidates_ = candidates;
    if (candidates_.size() > kMaxCandidateCount) {
        candidates_.resize(kMaxCandidateCount);
    }
    selected_index_ = std::min(selected_index, candidates_.size() - 1);

    const int dpi = GetDpiForWindow(hwnd_);
    const int height = MulDiv(34, dpi, 96);
    const int width = MulDiv(560, dpi, 96);
    SetWindowPos(hwnd_, HWND_TOPMOST, anchor.x, anchor.y + MulDiv(22, dpi, 96), width, height,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(hwnd_, nullptr, TRUE);
}

void CandidateWindow::hide() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_HIDE);
    }
}

bool CandidateWindow::is_visible() const noexcept {
    return hwnd_ && IsWindowVisible(hwnd_);
}

LRESULT CALLBACK CandidateWindow::window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* self = reinterpret_cast<CandidateWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
        self = static_cast<CandidateWindow*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    return self ? self->handle_message(message, wparam, lparam) : DefWindowProcW(hwnd, message, wparam, lparam);
}

LRESULT CandidateWindow::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_PAINT) {
        paint();
        return 0;
    }
    return DefWindowProcW(hwnd_, message, wparam, lparam);
}

void CandidateWindow::paint() {
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hwnd_, &ps);
    RECT rect{};
    GetClientRect(hwnd_, &rect);
    FillRect(hdc, &rect, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));

    int x = 8;
    for (size_t i = 0; i < candidates_.size(); ++i) {
        std::wstring text = std::to_wstring(i + 1) + L". " + candidates_[i].text;
        SIZE size{};
        GetTextExtentPoint32W(hdc, text.c_str(), static_cast<int>(text.size()), &size);
        RECT item{x - 3, 5, x + size.cx + 8, rect.bottom - 5};
        if (i == selected_index_) {
            HBRUSH brush = CreateSolidBrush(RGB(220, 235, 255));
            FillRect(hdc, &item, brush);
            DeleteObject(brush);
        }
        TextOutW(hdc, x, 9, text.c_str(), static_cast<int>(text.size()));
        x += size.cx + 18;
    }

    EndPaint(hwnd_, &ps);
}

}  // namespace localpinyin
