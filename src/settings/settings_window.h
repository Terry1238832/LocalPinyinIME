#pragma once

#include "settings_store.h"

#include <windows.h>

namespace localpinyin {

class SettingsWindow {
public:
    int run(HINSTANCE instance, int show_command);

private:
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam);
    void create_controls();
    void open_data_dir();

    HWND hwnd_ = nullptr;
    SettingsStore store_;
};

}  // namespace localpinyin
