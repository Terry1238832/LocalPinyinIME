#include "win32_utils.h"

#include <aclapi.h>
#include <sddl.h>
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

std::wstring install_file_path(HINSTANCE instance, const std::wstring& name) {
    std::wstring directory = module_path(instance);
    for (int level = 0; level < 2; ++level) {
        const size_t slash = directory.find_last_of(L"\\/");
        if (slash == std::wstring::npos) return L"";
        directory.resize(slash);
        const std::wstring candidate = directory + L"\\" + name;
        if (GetFileAttributesW(candidate.c_str()) != INVALID_FILE_ATTRIBUTES) return candidate;
    }
    return L"";
}

bool running_in_app_container() noexcept {
    static const bool contained = [] {
        HANDLE token = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return false;
        DWORD value = 0, size = 0;
        const bool result = GetTokenInformation(token, TokenIsAppContainer, &value, sizeof(value), &size) && value != 0;
        CloseHandle(token);
        return result;
    }();
    return contained;
}

bool share_with_store_apps(const std::wstring& folder) {
    if (folder.empty() || GetFileAttributesW(folder.c_str()) == INVALID_FILE_ATTRIBUTES) return false;
    // ALL APPLICATION PACKAGES: read and list, for the folder and all in it.
    PSID packages = nullptr;
    if (!ConvertStringSidToSidW(L"S-1-15-2-1", &packages)) return false;
    PACL current = nullptr;
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    bool done = false;
    if (GetNamedSecurityInfoW(folder.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &current, nullptr,
                              &descriptor) == ERROR_SUCCESS) {
        constexpr DWORD kRead = FILE_GENERIC_READ | FILE_GENERIC_EXECUTE;
        constexpr DWORD kInherit = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
        for (WORD index = 0; current && index < current->AceCount && !done; ++index) {
            ACE_HEADER* header = nullptr;
            if (!GetAce(current, index, reinterpret_cast<void**>(&header)) || header->AceType != ACCESS_ALLOWED_ACE_TYPE) continue;
            auto* ace = reinterpret_cast<ACCESS_ALLOWED_ACE*>(header);
            done = EqualSid(&ace->SidStart, packages) && (ace->Mask & kRead) == kRead && (header->AceFlags & kInherit) == kInherit;
        }
        if (!done) {
            EXPLICIT_ACCESSW access{};
            access.grfAccessPermissions = kRead;
            access.grfAccessMode = GRANT_ACCESS;
            access.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
            access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
            access.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
            access.Trustee.ptstrName = static_cast<LPWSTR>(packages);
            PACL updated = nullptr;
            if (SetEntriesInAclW(1, &access, current, &updated) == ERROR_SUCCESS) {
                done = SetNamedSecurityInfoW(const_cast<LPWSTR>(folder.c_str()), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr,
                                             nullptr, updated, nullptr) == ERROR_SUCCESS;
                LocalFree(updated);
            }
        }
        LocalFree(descriptor);
    }
    LocalFree(packages);
    return done;
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
