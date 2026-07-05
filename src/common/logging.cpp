#include "logging.h"

#include "utf_utils.h"
#include "win32_utils.h"

#include <windows.h>

#include <fstream>
#include <iomanip>
#include <sstream>

namespace localpinyin {

std::wstring format_status_log_line(const SYSTEMTIME& timestamp,
                                    const std::wstring& component,
                                    const std::wstring& message) {
    std::wostringstream line;
    line << std::setfill(L'0')
         << std::setw(4) << static_cast<unsigned int>(timestamp.wYear) << L"-"
         << std::setw(2) << static_cast<unsigned int>(timestamp.wMonth) << L"-"
         << std::setw(2) << static_cast<unsigned int>(timestamp.wDay) << L" "
         << std::setw(2) << static_cast<unsigned int>(timestamp.wHour) << L":"
         << std::setw(2) << static_cast<unsigned int>(timestamp.wMinute) << L":"
         << std::setw(2) << static_cast<unsigned int>(timestamp.wSecond)
         << L" [" << component << L"] " << message << L"\r\n";
    return line.str();
}

void log_status(const std::wstring& component, const std::wstring& message) {
    const std::wstring base = local_app_data_path() + L"\\LocalPinyinIME";
    const std::wstring logs = base + L"\\logs";
    ensure_directory(base);
    ensure_directory(logs);

    SYSTEMTIME now{};
    GetLocalTime(&now);
    const std::wstring line = format_status_log_line(now, component, message);

    std::ofstream file(logs + L"\\status.log", std::ios::binary | std::ios::app);
    file << wide_to_utf8(line);
}

}  // namespace localpinyin
