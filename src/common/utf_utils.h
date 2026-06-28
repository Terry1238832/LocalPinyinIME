#pragma once

#include <string>

namespace localpinyin {

std::wstring utf8_to_wide(const std::string& value);
std::string wide_to_utf8(const std::wstring& value);

}  // namespace localpinyin
