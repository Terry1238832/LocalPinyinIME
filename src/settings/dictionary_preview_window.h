#pragma once

#include "dictionary_catalog.h"
#include "modern_renderer.h"

#include <windows.h>
#include <commctrl.h>

namespace localpinyin {

inline constexpr wchar_t kDictionaryPreviewSearchPlaceholder[] = L"\u641C\u7D22\u62FC\u97F3\u6216\u8BCD\u8BED";

struct DictionaryPreviewLayout {
    RECT title{};
    RECT subtitle{};
    RECT badge{};
    RECT search_frame{};
    RECT search_edit{};
    RECT table_frame{};
    RECT list{};
    RECT status{};
};

[[nodiscard]] DictionaryPreviewLayout calculate_dictionary_preview_layout(int width, int height, int dpi) noexcept;

class DictionaryPreviewWindow {
public:
    static HWND show(HWND owner, HINSTANCE instance, const DictionaryCatalog& catalog, bool dark_theme,
                     int show_command = SW_SHOWNORMAL);
    static void set_theme(HWND hwnd, bool dark_theme);

private:
    friend class SettingsWindowCapture;
    DictionaryPreviewWindow(const DictionaryCatalog& catalog, bool dark_theme);
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam);
    void create_controls();
    void layout();
    void populate();
    void apply_theme();
    void refresh_metrics();
    void render(HDC hdc);
    void paint_empty_list(HDC hdc);
    bool draw_list_row(const NMLVCUSTOMDRAW& custom_draw);
    void install_list_header_theme_subclass();
    void paint_list_header(HWND header, HDC hdc);
    static LRESULT CALLBACK control_subclass_proc(
        HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
        UINT_PTR subclass_id, DWORD_PTR ref_data);
    static LRESULT CALLBACK list_header_subclass_proc(
        HWND hwnd,
        UINT message,
        WPARAM wparam,
        LPARAM lparam,
        UINT_PTR subclass_id,
        DWORD_PTR ref_data);

    DictionaryCatalog catalog_;
    bool dark_theme_ = false;
    HWND hwnd_ = nullptr;
    HWND search_ = nullptr;
    HWND list_ = nullptr;
    HWND status_ = nullptr;
    HFONT font_ = nullptr;
    HBRUSH background_ = nullptr;
    HBRUSH input_background_ = nullptr;
    ModernRenderer renderer_;
    HIMAGELIST row_metrics_ = nullptr;
    int dpi_ = 96;
    bool creation_finished_ = false;
};

}  // namespace localpinyin
