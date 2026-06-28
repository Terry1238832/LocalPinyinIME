#pragma once

#include <windows.h>

#include <string>

namespace localpinyin {

std::wstring module_path(HINSTANCE instance);
std::wstring local_app_data_path();
bool ensure_directory(const std::wstring& path);
std::wstring hresult_hex(HRESULT hr);

}  // namespace localpinyin
