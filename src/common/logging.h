#pragma once

#include <string>
#include <windows.h>

namespace localpinyin {

std::wstring format_status_log_line(const SYSTEMTIME& timestamp,
                                    const std::wstring& component,
                                    const std::wstring& message);
void log_status(const std::wstring& component, const std::wstring& message);

}  // namespace localpinyin
