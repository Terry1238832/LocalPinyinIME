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
