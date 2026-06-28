#include "settings_window.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show_command) {
    localpinyin::SettingsWindow window;
    return window.run(instance, show_command);
}
