#include "globals.h"

namespace localpinyin {

HINSTANCE g_instance = nullptr;
long g_dll_ref_count = 0;

void dll_add_ref() {
    InterlockedIncrement(&g_dll_ref_count);
}

void dll_release() {
    InterlockedDecrement(&g_dll_ref_count);
}

bool can_unload_now() {
    return g_dll_ref_count == 0;
}

}  // namespace localpinyin
