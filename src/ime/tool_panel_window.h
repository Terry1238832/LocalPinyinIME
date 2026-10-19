#pragma once

#include "candidate_ui.h"
#include "candidate_window.h"
#include "tool_panel.h"
#include "../settings/modern_renderer.h"

#include <windows.h>

#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace localpinyin {

// The small tool panel's popup: a per-pixel alpha layered window beside the
// caret, in the candidate window's colors, that never takes the focus. It
// draws a ToolPanelView and reports clicks; keys reach the panel through the
// input method.
class ToolPanelWindow {
public:
    ToolPanelWindow() = default;
    ~ToolPanelWindow();
    ToolPanelWindow(const ToolPanelWindow&) = delete;
    ToolPanelWindow& operator=(const ToolPanelWindow&) = delete;

    void show(const ToolPanelView& view, const RECT& caret_rect, const CandidateWindowOwnerInfo& owner_info);
    // Draws a changed view where the panel already is.
    void update(const ToolPanelView& view);
    void hide();
    [[nodiscard]] bool is_visible() const noexcept;
    [[nodiscard]] HWND window() const noexcept { return hwnd_; }
    void set_click_callback(std::function<void(const PanelHit&)> callback) { click_callback_ = std::move(callback); }

    // The panel's premultiplied BGRA image, for offline capture.
    bool render_offline(const ToolPanelView& view, int dpi, const CandidateThemePalette& palette,
                        std::vector<std::uint8_t>* pixels, SIZE* size);

private:
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT handle_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    bool create(HWND owner);
    void load_palette();
    void layout(int dpi);
    bool present();
    void paint();
    void paint_tabs();
    void paint_categories();
    void paint_grid();
    void paint_list();
    void paint_footer();
    void render(std::uint8_t* pixels, HDC dc);
    void composite_shadow(std::uint8_t* pixels, int width, int height);
    [[nodiscard]] PanelHit hit_at(POINT point) const;
    [[nodiscard]] int dip(int value) const noexcept { return scale_dip(value, dpi_); }

    HWND hwnd_ = nullptr;
    HWND owner_hwnd_ = nullptr;
    ToolPanelView view_;
    RECT caret_{};
    int dpi_ = 96;
    CandidateThemePalette palette_;
    ModernRenderer renderer_;
    // Geometry in window pixels (the panel inside a shadow margin).
    int margin_ = 0;
    SIZE size_{};
    POINT origin_{};
    RECT panel_{};
    RECT tabs_{};
    RECT categories_{};
    RECT content_{};
    RECT footer_{};
    std::vector<std::pair<RECT, PanelHit>> hits_;
    PanelHit hover_{};
    // The blurred shadow, kept while the size stays.
    std::vector<float> shadow_;
    SIZE shadow_size_{};
    std::function<void(const PanelHit&)> click_callback_;
};

}  // namespace localpinyin
