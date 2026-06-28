#include "logging.h"

#include "utf_utils.h"
#include "win32_utils.h"

#include <windows.h>

#include <fstream>

namespace localpinyin {

void log_status(const std::wstring& component, const std::wstring& message) {
    const std::wstring base = local_app_data_path() + L"\\LocalPinyinIME";
    const std::wstring logs = base + L"\\logs";
    ensure_directory(base);
    ensure_directory(logs);

    SYSTEMTIME now{};
    GetLocalTime(&now);

    wchar_t line[512]{};
    swprintf_s(line, L"%04u-%02u-%02u %02u:%02u:%02u [%ls] %ls\r\n",
               now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond,
               component.c_str(), message.c_str());

    std::ofstream file(logs + L"\\status.log", std::ios::binary | std::ios::app);
    file << wide_to_utf8(line);
}

}  // namespace localpinyin
