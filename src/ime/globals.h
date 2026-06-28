#pragma once

#include <windows.h>

namespace localpinyin {

extern HINSTANCE g_instance;
extern long g_dll_ref_count;

void dll_add_ref();
void dll_release();
bool can_unload_now();

}  // namespace localpinyin
