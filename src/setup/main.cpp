#include <windows.h>
#include <msctf.h>
#include <shlobj.h>

#include "../ime/guids.h"
#include "../ime/registration.h"
#include "../ime/tsf_profile_categories.h"
#include "localpinyin_version.h"
#include "verify_expected_dll.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr wchar_t kClsidText[] = L"{7C0B4B75-80B0-4E1F-A4A5-4D49A5440D8A}";
constexpr wchar_t kProfileGuidText[] = L"{84D58E7C-481E-4D20-A951-4ED39F01D8D5}";
constexpr wchar_t kProfileSpec[] =
    L"0x0804:{7C0B4B75-80B0-4E1F-A4A5-4D49A5440D8A}"
    L"{84D58E7C-481E-4D20-A951-4ED39F01D8D5};";
constexpr wchar_t kProfileSpecCompact[] =
    L"0804:{7C0B4B75-80B0-4E1F-A4A5-4D49A5440D8A}"
    L"{84D58E7C-481E-4D20-A951-4ED39F01D8D5}";
constexpr LANGID kLangId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
constexpr DWORD kInstallLayoutOrTipFlags = 0;
constexpr DWORD kIlotUninstall = 0x00000001;

using DllEntryFn = HRESULT(STDAPICALLTYPE*)();
using DllRegistrationDiagnosticFn = const wchar_t*(STDAPICALLTYPE*)();
using InstallLayoutOrTipFn = BOOL(WINAPI*)(LPCWSTR psz, DWORD dwFlags);

std::wstring g_diagnostic_log_path;

struct PeInfo {
    bool is_x64 = false;
    bool is_pe32_plus = false;
    bool is_dll = false;
};

struct TsfState {
    HRESULT get_profile_hr = E_FAIL;
    HRESULT enum_profiles_hr = E_FAIL;
    bool get_profile_ok = false;
    bool enum_profiles_contains = false;
    DWORD profile_flags = 0;
    DWORD profile_caps = 0;
    bool profile_enabled_flag = false;
    bool profile_active_flag = false;
    bool required_caps_present = false;
    struct CategoryState {
        const wchar_t* name = L"";
        HRESULT hr = E_FAIL;
        bool contains = false;
    };
    std::vector<CategoryState> categories;
};

std::wstring hresult_hex(HRESULT hr) {
    wchar_t buffer[16]{};
    swprintf_s(buffer, L"0x%08lX", static_cast<unsigned long>(hr));
    return buffer;
}

std::wstring win32_hex(DWORD error) {
    wchar_t buffer[16]{};
    swprintf_s(buffer, L"0x%08lX", static_cast<unsigned long>(error));
    return buffer;
}

std::wstring bool_text(bool value) {
    return value ? L"TRUE" : L"FALSE";
}

std::wstring ascii_to_wide(const char* text) {
    std::wstring result;
    if (!text) {
        return result;
    }
    while (*text) {
        result.push_back(static_cast<unsigned char>(*text));
        ++text;
    }
    return result;
}

std::string utf8_from_wide(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int needed = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (needed <= 1) {
        return {};
    }
    std::string result(static_cast<size_t>(needed - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, result.data(), needed, nullptr, nullptr);
    return result;
}

void append_diagnostic_line(const std::wstring& line) {
    if (g_diagnostic_log_path.empty()) {
        return;
    }
    std::ofstream file(std::filesystem::path(g_diagnostic_log_path), std::ios::binary | std::ios::app);
    if (!file) {
        return;
    }
    file << utf8_from_wide(line) << "\r\n";
}

std::wstring status_log_path() {
    PWSTR local_app_data = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local_app_data))) {
        return L"<unavailable>";
    }
    std::wstring path(local_app_data);
    CoTaskMemFree(local_app_data);
    return path + L"\\LocalPinyinIME\\logs\\status.log";
}

void write_failure_summary(const std::wstring& command,
                           const std::wstring& stage,
                           HRESULT hr,
                           const std::wstring& target_dll) {
    std::wstring line = command + L" failed: stage=" + stage +
                        L" hr=" + hresult_hex(hr) +
                        L" target_dll=" + target_dll +
                        L" clsid=" + kClsidText +
                        L" profile=" + kProfileGuidText +
                        L" langid=0x0804";
    std::wcerr << line << L"\n";
    append_diagnostic_line(line);
    append_diagnostic_line(L"status_log=" + status_log_path());
}

bool same_text(const std::wstring& left, const std::wstring& right) {
    return CompareStringOrdinal(left.c_str(), -1, right.c_str(), -1, TRUE) == CSTR_EQUAL;
}

std::wstring full_path(const std::wstring& path) {
    return localpinyin::setup::normalize_dll_path_for_comparison(path);
}

std::wstring guid_to_string(REFGUID guid) {
    wchar_t buffer[64]{};
    StringFromGUID2(guid, buffer, static_cast<int>(_countof(buffer)));
    return buffer;
}

bool is_elevated() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        return false;
    }
    TOKEN_ELEVATION elevation{};
    DWORD size = 0;
    const BOOL ok = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size);
    CloseHandle(token);
    return ok && elevation.TokenIsElevated != 0;
}

bool read_exact(HANDLE file, void* buffer, DWORD size) {
    DWORD read = 0;
    return ReadFile(file, buffer, size, &read, nullptr) && read == size;
}

bool seek_file(HANDLE file, LONG offset) {
    LARGE_INTEGER distance{};
    distance.QuadPart = offset;
    return SetFilePointerEx(file, distance, nullptr, FILE_BEGIN);
}

HRESULT inspect_pe(const std::wstring& path, PeInfo& info) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    IMAGE_DOS_HEADER dos{};
    if (!read_exact(file, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE) {
        CloseHandle(file);
        return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    }

    if (!seek_file(file, dos.e_lfanew)) {
        const HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        CloseHandle(file);
        return hr;
    }

    DWORD signature = 0;
    IMAGE_FILE_HEADER file_header{};
    WORD optional_magic = 0;
    if (!read_exact(file, &signature, sizeof(signature)) || signature != IMAGE_NT_SIGNATURE ||
        !read_exact(file, &file_header, sizeof(file_header)) ||
        !read_exact(file, &optional_magic, sizeof(optional_magic))) {
        CloseHandle(file);
        return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    }

    CloseHandle(file);
    info.is_x64 = file_header.Machine == IMAGE_FILE_MACHINE_AMD64;
    info.is_pe32_plus = optional_magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    info.is_dll = (file_header.Characteristics & IMAGE_FILE_DLL) != 0;
    return S_OK;
}

bool validate_dll_path(const std::wstring& dll_path) {
    const std::filesystem::path path(dll_path);
    if (!path.is_absolute()) {
        std::wcerr << L"error: --dll must be an absolute path\n";
        return false;
    }
    if (!same_text(path.filename().wstring(), L"LocalPinyinIME.dll")) {
        std::wcerr << L"error: DLL file name must be LocalPinyinIME.dll\n";
        return false;
    }

    PeInfo info{};
    const HRESULT hr = inspect_pe(dll_path, info);
    std::wcout << L"DLL PE inspection HRESULT: " << hresult_hex(hr) << L"\n"
               << L"DLL machine x64: " << bool_text(info.is_x64) << L"\n"
               << L"DLL PE32+: " << bool_text(info.is_pe32_plus) << L"\n"
               << L"DLL file type is DLL: " << bool_text(info.is_dll) << L"\n";
    if (FAILED(hr) || !info.is_x64 || !info.is_pe32_plus || !info.is_dll) {
        std::wcerr << L"error: DLL is not a valid x64 PE32+ DLL\n";
        return false;
    }
    return true;
}

HRESULT call_dll_entry(const std::wstring& dll_path, const char* export_name) {
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_USER_DIRS);
    SetLastError(ERROR_SUCCESS);
    HMODULE module = LoadLibraryExW(dll_path.c_str(), nullptr,
                                    LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    const DWORD load_error = GetLastError();
    std::wcout << L"LoadLibraryExW Win32 error: " << load_error << L" / " << win32_hex(load_error) << L"\n";
    append_diagnostic_line(L"operation=LoadLibraryExW win32=" + std::to_wstring(load_error) +
                           L" target_dll=" + dll_path);
    if (!module) {
        return HRESULT_FROM_WIN32(load_error);
    }

    SetLastError(ERROR_SUCCESS);
    auto* proc = reinterpret_cast<DllEntryFn>(GetProcAddress(module, export_name));
    const DWORD proc_error = GetLastError();
    std::wcout << L"GetProcAddress(" << export_name << L") Win32 error: "
               << proc_error << L" / " << win32_hex(proc_error) << L"\n";
    append_diagnostic_line(L"operation=GetProcAddress export=" +
                           ascii_to_wide(export_name) +
                           L" win32=" + std::to_wstring(proc_error) +
                           L" target_dll=" + dll_path);
    if (!proc) {
        FreeLibrary(module);
        return HRESULT_FROM_WIN32(proc_error);
    }

    const HRESULT hr = proc();
    append_diagnostic_line(L"operation=" +
                           ascii_to_wide(export_name) +
                           L" hr=" + hresult_hex(hr) +
                           L" target_dll=" + dll_path);
    auto* diagnostic_proc = reinterpret_cast<DllRegistrationDiagnosticFn>(
        GetProcAddress(module, "LocalPinyinGetLastRegistrationDiagnostic"));
    if (diagnostic_proc) {
        const wchar_t* diagnostic = diagnostic_proc();
        if (diagnostic && diagnostic[0] != L'\0') {
            std::wcout << L"Last registration diagnostic: " << diagnostic << L"\n";
            append_diagnostic_line(L"operation=LocalPinyinGetLastRegistrationDiagnostic " +
                                   std::wstring(diagnostic) +
                                   L" target_dll=" + dll_path);
        }
    }
    FreeLibrary(module);
    return hr;
}

bool key_exists(HKEY root, const std::wstring& subkey) {
    HKEY key = nullptr;
    const LSTATUS status = RegOpenKeyExW(root, subkey.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &key);
    if (status == ERROR_SUCCESS) {
        RegCloseKey(key);
        return true;
    }
    return false;
}

localpinyin::setup::InprocServerReadResult read_inproc_server() {
    const std::wstring subkey = std::wstring(L"CLSID\\") + kClsidText + L"\\InprocServer32";
    HKEY key = nullptr;
    LSTATUS status = RegOpenKeyExW(HKEY_CLASSES_ROOT, subkey.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &key);
    if (status != ERROR_SUCCESS) {
        return {HRESULT_FROM_WIN32(status), L""};
    }

    DWORD type = 0;
    DWORD bytes = 0;
    status = RegQueryValueExW(key, nullptr, nullptr, &type, nullptr, &bytes);
    if (status != ERROR_SUCCESS || type != REG_SZ || bytes < sizeof(wchar_t)) {
        RegCloseKey(key);
        return {status == ERROR_SUCCESS ? HRESULT_FROM_WIN32(ERROR_INVALID_DATATYPE) : HRESULT_FROM_WIN32(status), L""};
    }

    std::wstring value(bytes / sizeof(wchar_t), L'\0');
    status = RegQueryValueExW(key, nullptr, nullptr, &type, reinterpret_cast<BYTE*>(value.data()), &bytes);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS) {
        return {HRESULT_FROM_WIN32(status), L""};
    }
    while (!value.empty() && value.back() == L'\0') {
        value.pop_back();
    }
    return {S_OK, value};
}

bool is_local_profile(const TF_INPUTPROCESSORPROFILE& profile) {
    return profile.langid == kLangId &&
           IsEqualGUID(profile.clsid, localpinyin::CLSID_LocalPinyinTextService) &&
           IsEqualGUID(profile.guidProfile, localpinyin::GUID_LocalPinyinProfile);
}

bool category_contains(ITfCategoryMgr* category_mgr, REFGUID category, REFGUID expected_item, HRESULT& hr) {
    IEnumGUID* enum_items = nullptr;
    hr = category_mgr->EnumItemsInCategory(category, &enum_items);
    if (FAILED(hr) || !enum_items) {
        return false;
    }

    bool contains = false;
    for (;;) {
            GUID item{};
            ULONG fetched = 0;
            const HRESULT next_hr = enum_items->Next(1, &item, &fetched);
        if (next_hr != S_OK || fetched == 0) {
            break;
        }
        if (IsEqualGUID(item, expected_item)) {
            contains = true;
            break;
        }
    }
    enum_items->Release();
    return contains;
}

TsfState query_tsf_state() {
    TsfState state{};

    ITfInputProcessorProfileMgr* profile_mgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&profile_mgr));
    if (FAILED(hr)) {
        state.get_profile_hr = hr;
        state.enum_profiles_hr = hr;
    } else {
        TF_INPUTPROCESSORPROFILE profile{};
        hr = profile_mgr->GetProfile(TF_PROFILETYPE_INPUTPROCESSOR,
                                     kLangId,
                                     localpinyin::CLSID_LocalPinyinTextService,
                                     localpinyin::GUID_LocalPinyinProfile,
                                     nullptr,
                                     &profile);
        state.get_profile_hr = hr;
        state.get_profile_ok = SUCCEEDED(hr) && is_local_profile(profile);
        if (state.get_profile_ok) {
            state.profile_flags = profile.dwFlags;
            state.profile_caps = profile.dwCaps;
            state.profile_enabled_flag = (profile.dwFlags & TF_IPP_FLAG_ENABLED) != 0;
            state.profile_active_flag = (profile.dwFlags & TF_IPP_FLAG_ACTIVE) != 0;
            state.required_caps_present =
                (profile.dwCaps & localpinyin::required_tsf_profile_caps()) ==
                localpinyin::required_tsf_profile_caps();
        }

        IEnumTfInputProcessorProfiles* enum_profiles = nullptr;
        hr = profile_mgr->EnumProfiles(kLangId, &enum_profiles);
        state.enum_profiles_hr = hr;
        if (SUCCEEDED(hr) && enum_profiles) {
            for (;;) {
                TF_INPUTPROCESSORPROFILE item{};
                ULONG fetched = 0;
                const HRESULT next_hr = enum_profiles->Next(1, &item, &fetched);
                if (next_hr != S_OK || fetched == 0) {
                    break;
                }
                if (is_local_profile(item)) {
                    state.enum_profiles_contains = true;
                    break;
                }
            }
            enum_profiles->Release();
        }
        profile_mgr->Release();
    }

    ITfCategoryMgr* category_mgr = nullptr;
    hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&category_mgr));
    if (FAILED(hr)) {
        for (const auto& category : localpinyin::required_tsf_profile_categories()) {
            state.categories.push_back(TsfState::CategoryState{category.name, hr, false});
        }
        return state;
    }

    for (const auto& category : localpinyin::required_tsf_profile_categories()) {
        HRESULT category_hr = E_FAIL;
        const bool contains =
            category_contains(category_mgr, *category.category_guid, *category.item_guid, category_hr);
        state.categories.push_back(TsfState::CategoryState{category.name, category_hr, contains});
    }
    category_mgr->Release();
    return state;
}

void print_tsf_state(const TsfState& state) {
    const bool all_categories_present = std::all_of(state.categories.begin(), state.categories.end(),
                                                    [](const TsfState::CategoryState& item) {
                                                        return item.contains;
                                                    });
    const auto caps = localpinyin::observe_profile_caps(state.profile_caps, localpinyin::required_tsf_profile_caps());
    std::wcout << L"Profile CLSID: " << kClsidText << L"\n"
               << L"Profile GUID: " << kProfileGuidText << L"\n"
               << L"Profile LANGID: 0x0804\n"
               << L"GetProfile HRESULT: " << hresult_hex(state.get_profile_hr) << L"\n"
               << L"Profile valid: " << bool_text(state.get_profile_ok) << L"\n"
               << L"Profile registered: " << bool_text(state.enum_profiles_contains) << L"\n"
               << L"Profile enabled flag: " << bool_text(state.profile_enabled_flag) << L"\n"
               << L"Profile active flag: " << bool_text(state.profile_active_flag) << L"\n"
               << L"Profile flags: " << win32_hex(state.profile_flags) << L"\n"
               << L"Profile caps: " << win32_hex(state.profile_caps) << L"\n"
               << L"Profile required caps present (diagnostic only): " << bool_text(state.required_caps_present) << L"\n"
               << L"Profile expected internal caps (diagnostic only): " << win32_hex(caps.expected_caps) << L"\n"
               << L"Profile missing internal caps (diagnostic only): " << win32_hex(caps.missing_caps) << L"\n"
               << L"EnumProfiles HRESULT: " << hresult_hex(state.enum_profiles_hr) << L"\n"
               << L"EnumProfiles contains LocalPinyinIME: " << bool_text(state.enum_profiles_contains) << L"\n"
               << L"All required TSF categories contain LocalPinyinIME CLSID: "
               << bool_text(all_categories_present) << L"\n";
    for (const auto& category : state.categories) {
        std::wcout << category.name << L" HRESULT: " << hresult_hex(category.hr) << L"\n"
                   << category.name << L" contains LocalPinyinIME CLSID: "
                   << bool_text(category.contains) << L"\n";
    }
}

HRESULT append_expected_dll_diagnostic(const std::wstring& line, void*) {
    append_diagnostic_line(line);
    return S_OK;
}

localpinyin::setup::ExpectedDllVerification verify_expected_dll(const std::wstring* expected_dll_path) {
    const auto inproc = read_inproc_server();
    std::wcout << L"InprocServer32: " << (inproc.value.empty() ? L"<missing>" : inproc.value) << L"\n";
    std::wcout << L"InprocServer32 read HRESULT: " << hresult_hex(inproc.hr) << L"\n";
    if (!expected_dll_path) {
        localpinyin::setup::ExpectedDllVerification result{};
        result.actual_normalized = full_path(inproc.value);
        result.matches = SUCCEEDED(inproc.hr) && !result.actual_normalized.empty();
        result.hr = result.matches ? S_OK :
                    (FAILED(inproc.hr) ? inproc.hr : HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
        return result;
    }

    auto result = localpinyin::setup::verify_expected_dll_value(
        *expected_dll_path, inproc, append_expected_dll_diagnostic, nullptr);
    std::wcout << L"Expected InprocServer32: "
               << (result.expected_normalized.empty() ? L"<missing>" : result.expected_normalized) << L"\n"
               << L"Actual InprocServer32: "
               << (result.actual_normalized.empty() ? L"<missing>" : result.actual_normalized) << L"\n";
    std::wcout << L"InprocServer32 matches expected DLL: " << bool_text(result.matches) << L"\n";
    return result;
}

bool verify_system_registration() {
    const TsfState state = query_tsf_state();
    print_tsf_state(state);
    const bool all_categories_present = std::all_of(state.categories.begin(), state.categories.end(),
                                                    [](const TsfState::CategoryState& item) {
                                                        return item.contains;
                                                    });
    return localpinyin::is_system_registration_verified(state.get_profile_ok,
                                                        state.enum_profiles_contains,
                                                        all_categories_present,
                                                        state.required_caps_present);
}

struct RegistrationVerificationResult {
    localpinyin::setup::ExpectedDllVerification expected_dll;
    bool system_registered = false;

    bool ok() const {
        return SUCCEEDED(expected_dll.hr) && system_registered;
    }
};

RegistrationVerificationResult verify_registered(const std::wstring* expected_dll_path) {
    RegistrationVerificationResult result{};
    result.expected_dll = verify_expected_dll(expected_dll_path);
    result.system_registered = verify_system_registration();
    return result;
}

bool verify_unregistered() {
    const std::wstring com_key = std::wstring(L"CLSID\\") + kClsidText;
    const std::wstring tip_key = std::wstring(L"SOFTWARE\\Microsoft\\CTF\\TIP\\") + kClsidText;
    const bool com_exists = key_exists(HKEY_CLASSES_ROOT, com_key);
    const bool tip_exists = key_exists(HKEY_LOCAL_MACHINE, tip_key);

    std::wcout << L"COM CLSID root exists: " << bool_text(com_exists) << L"\n"
               << L"TSF TIP root exists: " << bool_text(tip_exists) << L"\n";
    if (com_exists || tip_exists) {
        std::wcerr << L"cleanup incomplete\n";
        return false;
    }
    return true;
}

HRESULT query_current_user_enabled(bool& enabled) {
    enabled = false;
    ITfInputProcessorProfiles* profiles = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&profiles));
    if (FAILED(hr)) {
        return hr;
    }
    BOOL is_enabled = FALSE;
    hr = profiles->IsEnabledLanguageProfile(localpinyin::CLSID_LocalPinyinTextService,
                                            kLangId,
                                            localpinyin::GUID_LocalPinyinProfile,
                                            &is_enabled);
    profiles->Release();
    if (SUCCEEDED(hr)) {
        enabled = is_enabled == TRUE;
    }
    return hr;
}

HRESULT load_install_layout_or_tip(HMODULE& input_module, InstallLayoutOrTipFn& install_fn) {
    input_module = LoadLibraryExW(L"input.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!input_module) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    install_fn = reinterpret_cast<InstallLayoutOrTipFn>(GetProcAddress(input_module, "InstallLayoutOrTip"));
    if (!install_fn) {
        const HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        FreeLibrary(input_module);
        input_module = nullptr;
        return hr;
    }
    return S_OK;
}

int enable_or_disable_current_user(bool enable) {
    std::wcout << L"ProfileSpec canonical: " << kProfileSpec << L"\n"
               << L"ProfileSpec compact equivalent: " << kProfileSpecCompact << L"\n";

    bool before = false;
    HRESULT hr = query_current_user_enabled(before);
    std::wcout << L"before.IsEnabledLanguageProfile HRESULT: " << hresult_hex(hr) << L"\n"
               << L"enabled before: " << bool_text(before) << L"\n";
    if (FAILED(hr)) {
        return 1;
    }

    if (enable && before) {
        std::wcout << L"already enabled\n";
        return 0;
    }
    if (!enable && !before) {
        std::wcout << L"already disabled\n";
        return 0;
    }

    HMODULE input = nullptr;
    InstallLayoutOrTipFn install = nullptr;
    hr = load_install_layout_or_tip(input, install);
    if (FAILED(hr)) {
        std::wcerr << L"Load InstallLayoutOrTip HRESULT: " << hresult_hex(hr) << L"\n";
        return 1;
    }

    const DWORD flags = enable ? kInstallLayoutOrTipFlags : kIlotUninstall;
    SetLastError(ERROR_SUCCESS);
    const BOOL ok = install(kProfileSpec, flags);
    const DWORD last_error = ok ? ERROR_SUCCESS : GetLastError();
    FreeLibrary(input);

    std::wcout << L"InstallLayoutOrTip flags: " << flags << L"\n"
               << L"InstallLayoutOrTip returned: " << (ok ? L"TRUE" : L"FALSE") << L"\n"
               << L"GetLastError: " << last_error << L" / " << win32_hex(last_error) << L"\n";
    if (!ok) {
        return 1;
    }

    bool after = false;
    hr = query_current_user_enabled(after);
    std::wcout << L"after.IsEnabledLanguageProfile HRESULT: " << hresult_hex(hr) << L"\n"
               << L"enabled after: " << bool_text(after) << L"\n";
    std::wcout << L"current user InputMethodTips verification: run Verify-LocalPinyinIME.ps1\n";
    if (FAILED(hr)) {
        return 1;
    }
    return after == enable ? 0 : 1;
}

void print_usage() {
    std::wcout
        << L"LocalPinyinImeSetup " << LOCALPINYINIME_VERSION_STRING << L"\n"
        << L"Usage:\n"
        << L"  LocalPinyinImeSetup.exe --register-system --dll <absolute LocalPinyinIME.dll>\n"
        << L"  LocalPinyinImeSetup.exe --unregister-system --dll <absolute LocalPinyinIME.dll>\n"
        << L"  LocalPinyinImeSetup.exe --enable-current-user\n"
        << L"  LocalPinyinImeSetup.exe --disable-current-user\n"
        << L"  LocalPinyinImeSetup.exe --verify [--expected-dll <absolute LocalPinyinIME.dll>] [--require-current-user-enabled]\n"
        << L"  LocalPinyinImeSetup.exe --version\n";
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    std::wstring command;
    std::wstring dll_path;
    std::wstring expected_dll_path;
    bool require_current_user_enabled = false;

    for (int i = 1; i < argc; ++i) {
        const std::wstring arg = argv[i];
        if (arg == L"--register-system" || arg == L"--unregister-system" ||
            arg == L"--enable-current-user" || arg == L"--disable-current-user" ||
            arg == L"--verify" || arg == L"--version") {
            if (!command.empty()) {
                std::wcerr << L"error: only one command is allowed\n";
                print_usage();
                return 2;
            }
            command = arg;
        } else if (arg == L"--dll") {
            if (i + 1 >= argc) {
                std::wcerr << L"error: --dll requires a value\n";
                return 2;
            }
            dll_path = argv[++i];
        } else if (arg == L"--expected-dll") {
            if (i + 1 >= argc) {
                std::wcerr << L"error: --expected-dll requires a value\n";
                return 2;
            }
            expected_dll_path = argv[++i];
        } else if (arg == L"--require-current-user-enabled") {
            require_current_user_enabled = true;
        } else if (arg == L"--diagnostic-log") {
            if (i + 1 >= argc) {
                std::wcerr << L"error: --diagnostic-log requires a value\n";
                return 2;
            }
            g_diagnostic_log_path = full_path(argv[++i]);
        } else {
            std::wcerr << L"error: unknown argument: " << arg << L"\n";
            print_usage();
            return 2;
        }
    }

    if (command.empty()) {
        print_usage();
        return 2;
    }
    if (!g_diagnostic_log_path.empty()) {
        append_diagnostic_line(L"LocalPinyinImeSetup command=" + command +
                               L" diagnostic_log=" + g_diagnostic_log_path);
    }

    if (command == L"--version") {
        std::wcout << L"product: LocalPinyinIME\n"
                   << L"displayName: " << LOCALPINYINIME_DISPLAY_NAME << L"\n"
                   << L"version: " << LOCALPINYINIME_VERSION_STRING << L"\n"
                   << L"releaseType: " << LOCALPINYINIME_RELEASE_TYPE << L"\n"
                   << L"architecture: x64\n"
                   << L"clsid: " << kClsidText << L"\n"
                   << L"profileGuid: " << kProfileGuidText << L"\n"
                   << L"langid: 0x0804\n";
        return 0;
    }

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool co_initialized = SUCCEEDED(hr);
    if (hr == RPC_E_CHANGED_MODE) {
        hr = S_OK;
    }
    if (FAILED(hr)) {
        std::wcerr << L"CoInitializeEx HRESULT: " << hresult_hex(hr) << L"\n";
        return 1;
    }

    int exit_code = 1;
    if (command == L"--register-system" || command == L"--unregister-system") {
        if (!is_elevated()) {
            std::wcerr << L"error: this command requires an elevated administrator console\n";
            exit_code = 1;
        } else if (dll_path.empty()) {
            std::wcerr << L"error: --dll is required\n";
            exit_code = 2;
        } else {
            dll_path = full_path(dll_path);
            std::wcout << L"DLL path: " << dll_path << L"\n";
            if (!validate_dll_path(dll_path)) {
                exit_code = 1;
            } else if (command == L"--register-system") {
                const HRESULT register_hr = call_dll_entry(dll_path, "DllRegisterServer");
                std::wcout << L"DllRegisterServer HRESULT: " << hresult_hex(register_hr) << L"\n";
                const RegistrationVerificationResult registered = verify_registered(&dll_path);
                if (FAILED(register_hr)) {
                    write_failure_summary(L"register-system", L"DllRegisterServer", register_hr, dll_path);
                } else if (FAILED(registered.expected_dll.hr)) {
                    write_failure_summary(L"register-system", L"VerifyExpectedDll", registered.expected_dll.hr, dll_path);
                } else if (!registered.system_registered) {
                    write_failure_summary(L"register-system", L"VerifySystemRegistration", E_FAIL, dll_path);
                }
                exit_code = SUCCEEDED(register_hr) && registered.ok() ? 0 : 1;
            } else {
                const HRESULT unregister_hr = call_dll_entry(dll_path, "DllUnregisterServer");
                std::wcout << L"DllUnregisterServer HRESULT: " << hresult_hex(unregister_hr) << L"\n";
                exit_code = SUCCEEDED(unregister_hr) && verify_unregistered() ? 0 : 1;
            }
        }
    } else if (command == L"--enable-current-user") {
        exit_code = enable_or_disable_current_user(true);
    } else if (command == L"--disable-current-user") {
        exit_code = enable_or_disable_current_user(false);
    } else if (command == L"--verify") {
        std::wstring expected;
        const std::wstring* expected_ptr = nullptr;
        if (!expected_dll_path.empty()) {
            expected = full_path(expected_dll_path);
            expected_ptr = &expected;
        }
        const RegistrationVerificationResult registered = verify_registered(expected_ptr);
        bool enabled = false;
        const HRESULT enabled_hr = query_current_user_enabled(enabled);
        std::wcout << L"IsEnabledLanguageProfile HRESULT: " << hresult_hex(enabled_hr) << L"\n"
                   << L"current user enabled: " << bool_text(SUCCEEDED(enabled_hr) && enabled) << L"\n"
                   << L"current user InputMethodTips verification: not performed by native setup\n";
        if (FAILED(registered.expected_dll.hr)) {
            write_failure_summary(L"verify", L"VerifyExpectedDll", registered.expected_dll.hr, expected);
        } else if (!registered.system_registered) {
            write_failure_summary(L"verify", L"VerifySystemRegistration", E_FAIL, expected);
        } else if (FAILED(enabled_hr)) {
            write_failure_summary(L"verify", L"IsEnabledLanguageProfile", enabled_hr, expected);
        } else if (require_current_user_enabled && !enabled) {
            write_failure_summary(L"verify", L"RequireCurrentUserEnabled", E_FAIL, expected);
        }
        exit_code = registered.ok() && SUCCEEDED(enabled_hr) &&
                    (!require_current_user_enabled || enabled) ? 0 : 1;
    } else {
        print_usage();
        exit_code = 2;
    }

    if (co_initialized) {
        CoUninitialize();
    }
    return exit_code;
}
