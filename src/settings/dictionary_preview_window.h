#pragma once

#include "dictionary_catalog.h"

#include <windows.h>
#include <commctrl.h>

namespace localpinyin {

inline constexpr wchar_t kDictionaryPreviewSearchPlaceholder[] = L"\u641C\u7D22\u62FC\u97F3\u6216\u8BCD\u8BED";

class DictionaryPreviewWindow {
public:
    static HWND show(HWND owner, HINSTANCE instance, const DictionaryCatalog& catalog, bool dark_theme);

private:
    DictionaryPreviewWindow(const DictionaryCatalog& catalog, bool dark_theme);
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam);
    void create_controls();
    void layout();
    void populate();
    void apply_theme();
    void install_list_header_theme_subclass();
    bool draw_list_header_item(const NMCUSTOMDRAW& custom_draw);
    void paint_list_header(HWND header, HDC hdc);
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
};

}  // namespace localpinyin
