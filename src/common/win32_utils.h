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
// Store apps (UWP: Notepads, the Mail app…) run in an app container, a
// sandbox that cannot read the user's data folder or start other programs.
bool running_in_app_container() noexcept;
// Lets app containers read `folder` and what is put in it (the stickers, so
// the small panel shows them there too). Leaves it as it is when already so.
bool share_with_store_apps(const std::wstring& folder);

}  // namespace localpinyin
