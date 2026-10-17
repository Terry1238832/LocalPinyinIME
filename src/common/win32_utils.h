#pragma once

#include <windows.h>

#include <string>

namespace localpinyin {

std::wstring module_path(HINSTANCE instance);
// `name` beside the module, or, for the 32-bit input method installed in an
// x86 subfolder of the 64-bit release, in that parent folder. Empty when
// neither exists.
std::wstring install_file_path(HINSTANCE instance, const std::wstring& name);
std::wstring local_app_data_path();
bool ensure_directory(const std::wstring& path);
std::wstring hresult_hex(HRESULT hr);

}  // namespace localpinyin
