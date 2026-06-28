#include "globals.h"

#include <windows.h>

BOOL APIENTRY DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        localpinyin::g_instance = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}
