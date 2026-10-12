#include "win32_utils.h"

#include <shlobj.h>

#include <sstream>

namespace localpinyin {

std::wstring module_path(HINSTANCE instance) {
    std::wstring buffer(MAX_PATH, L'\0');
    DWORD size = GetModuleFileNameW(instance, buffer.data(), static_cast<DWORD>(buffer.size()));
    while (size == buffer.size()) {
        buffer.resize(buffer.size() * 2);
        size = GetModuleFileNameW(instance, buffer.data(), static_cast<DWORD>(buffer.size()));
    }
    buffer.resize(size);
    return buffer;
}

std::wstring local_app_data_path() {
    // A process can choose an isolated data location for smoke tests or portable use.
    const DWORD required = GetEnvironmentVariableW(L"LOCALPINYINIME_LOCALAPPDATA", nullptr, 0);
    if (required > 1 && required < 32768) {
        std::wstring override_path(required, L'\0');
        const DWORD length = GetEnvironmentVariableW(L"LOCALPINYINIME_LOCALAPPDATA", override_path.data(), required);
        if (length > 0 && length < required) {
            override_path.resize(length);
            if ((override_path.size() > 2 && override_path[1] == L':' &&
                 (override_path[2] == L'\\' || override_path[2] == L'/')) ||
                override_path.starts_with(L"\\\\")) return override_path;
        }
    }
    PWSTR path = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path))) {
        return L"";
    }
    std::wstring result(path);
    CoTaskMemFree(path);
    return result;
}

bool ensure_directory(const std::wstring& path) {
    if (path.empty()) {
        return false;
    }
    if (CreateDirectoryW(path.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS) {
        return true;
    }
    return false;
}

std::wstring hresult_hex(HRESULT hr) {
    std::wstringstream stream;
    stream << L"0x" << std::hex << static_cast<unsigned long>(hr);
    return stream.str();
}

}  // namespace localpinyin
