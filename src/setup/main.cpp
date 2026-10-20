#include <windows.h>
#include <msctf.h>
#include <shlobj.h>

#include "../ime/guids.h"
#include "../ime/registration.h"
#include "../ime/tsf_profile_categories.h"
#include "localpinyin_version.h"
#include "profile_visibility_verification.h"
#include "verify_expected_dll.h"

#include <algorithm>
#include <clocale>
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
// 本地英文, the same text service's second input method.
constexpr wchar_t kEnglishProfileSpec[] =
    L"0x0804:{7C0B4B75-80B0-4E1F-A4A5-4D49A5440D8A}"
    L"{76D42B0B-C9A1-4106-A917-48D06C1E30A2};";
constexpr LANGID kLangId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
constexpr DWORD kInstallLayoutOrTipFlags = 0;
constexpr DWORD kIlotUninstall = 0x00000001;

using DllEntryFn = HRESULT(STDAPICALLTYPE*)();
using DllRegistrationDiagnosticFn = const wchar_t*(STDAPICALLTYPE*)();
using InstallLayoutOrTipFn = BOOL(WINAPI*)(LPCWSTR psz, DWORD dwFlags);

std::wstring g_diagnostic_log_path;

struct PeInfo {
    bool is_x64 = false;
    bool is_x86 = false;
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
    std::vector<localpinyin::setup::ProfileVisibilityAttempt> profile_attempts;
    localpinyin::setup::ProfileVisibilityVerification profile_verification;
    struct CategoryState {
        const wchar_t* name = L"";
        HRESULT category_to_items_hr = E_FAIL;
        unsigned int category_to_items_count = 0;
        bool category_to_items_found = false;
        HRESULT item_to_categories_hr = E_FAIL;
        unsigned int item_to_categories_count = 0;
        bool item_to_categories_found = false;
        bool contains = false;
        bool allow_inconclusive = false;
    };
    std::vector<CategoryState> categories;
    std::vector<CategoryState> compatibility_categories;
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
    info.is_x86 = file_header.Machine == IMAGE_FILE_MACHINE_I386;
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
               << L"DLL machine x86: " << bool_text(info.is_x86) << L"\n"
               << L"DLL PE32+: " << bool_text(info.is_pe32_plus) << L"\n"
               << L"DLL file type is DLL: " << bool_text(info.is_dll) << L"\n";
    // This tool registers a DLL of its own architecture: the 64-bit build the
    // x64 input method, the 32-bit build (x86\) the one 32-bit apps load.
#ifdef _WIN64
    const bool matches = info.is_x64 && info.is_pe32_plus;
    constexpr wchar_t kExpected[] = L"x64 PE32+";
#else
    const bool matches = info.is_x86 && !info.is_pe32_plus;
    constexpr wchar_t kExpected[] = L"x86 PE32";
#endif
    if (FAILED(hr) || !matches || !info.is_dll) {
        std::wcerr << L"error: DLL is not a valid " << kExpected << L" DLL\n";
        return false;
    }
    return true;
}

HRESULT call_dll_entry(const std::wstring& dll_path, const char* export_name) {
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_USER_DIRS);
    SetLastError(ERROR_SUCCESS);
    HMODULE module = LoadLibraryExW(dll_path.c_str(), nullptr,
                                    LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!module) {
        const DWORD load_error = GetLastError();
        std::wcout << L"LoadLibraryExW succeeded: FALSE\n"
                   << L"LoadLibraryExW Win32 error: " << load_error << L" / "
                   << win32_hex(load_error) << L"\n";
        append_diagnostic_line(L"operation=LoadLibraryExW success=FALSE win32=" +
                               std::to_wstring(load_error) +
                               L" target_dll=" + dll_path);
        return HRESULT_FROM_WIN32(load_error);
    }
    std::wcout << L"LoadLibraryExW succeeded: TRUE\n";
    append_diagnostic_line(L"operation=LoadLibraryExW success=TRUE target_dll=" +
                           dll_path);

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
    const LSTATUS status = RegOpenKeyExW(root, subkey.c_str(), 0, KEY_READ | localpinyin::kRegistryView, &key);
    if (status == ERROR_SUCCESS) {
        RegCloseKey(key);
        return true;
    }
    return false;
}

localpinyin::setup::InprocServerReadResult read_inproc_server() {
    const std::wstring subkey = std::wstring(L"CLSID\\") + kClsidText + L"\\InprocServer32";
    HKEY key = nullptr;
    LSTATUS status = RegOpenKeyExW(HKEY_CLASSES_ROOT, subkey.c_str(), 0, KEY_READ | localpinyin::kRegistryView, &key);
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

localpinyin::CategoryMembershipObservation enumerate_category_to_items(
    ITfCategoryMgr* category_mgr,
    REFGUID category,
    REFGUID expected_item) {
    localpinyin::CategoryMembershipObservation observation{};
    IEnumGUID* items = nullptr;
    observation.hr = category_mgr->EnumItemsInCategory(category, &items);
    if (FAILED(observation.hr) || !items) {
        return observation;
    }
    for (;;) {
        GUID item{};
        ULONG fetched = 0;
        const HRESULT next_hr = items->Next(1, &item, &fetched);
        if (next_hr == S_FALSE || fetched == 0) {
            break;
        }
        if (FAILED(next_hr)) {
            observation.hr = next_hr;
            break;
        }
        ++observation.count;
        observation.found = observation.found || IsEqualGUID(item, expected_item);
    }
    items->Release();
    return observation;
}

localpinyin::CategoryMembershipObservation enumerate_item_to_categories(
    ITfCategoryMgr* category_mgr,
    REFGUID item,
    REFGUID expected_category) {
    localpinyin::CategoryMembershipObservation observation{};
    IEnumGUID* categories = nullptr;
    observation.hr = category_mgr->EnumCategoriesInItem(item, &categories);
    if (FAILED(observation.hr) || !categories) {
        return observation;
    }
    for (;;) {
        GUID category{};
        ULONG fetched = 0;
        const HRESULT next_hr = categories->Next(1, &category, &fetched);
        if (next_hr == S_FALSE || fetched == 0) {
            break;
        }
        if (FAILED(next_hr)) {
            observation.hr = next_hr;
            break;
        }
        ++observation.count;
        observation.found = observation.found ||
                            IsEqualGUID(category, expected_category);
    }
    categories->Release();
    return observation;
}

TsfState query_tsf_state() {
    TsfState state{};
    TF_INPUTPROCESSORPROFILE final_matching_profile{};
    bool have_final_matching_profile = false;
    for (size_t index = 0;
         index < localpinyin::setup::kProfileVisibilityRetryDelaysMs.size();
         ++index) {
        const DWORD delay_ms =
            localpinyin::setup::kProfileVisibilityRetryDelaysMs[index];
        if (delay_ms != 0) {
            Sleep(delay_ms);
        }

        localpinyin::setup::ProfileVisibilityAttempt attempt{};
        ITfInputProcessorProfileMgr* profile_mgr = nullptr;
        attempt.manager_hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles,
                                              nullptr,
                                              CLSCTX_INPROC_SERVER,
                                              IID_PPV_ARGS(&profile_mgr));
        if (SUCCEEDED(attempt.manager_hr) && profile_mgr) {
            TF_INPUTPROCESSORPROFILE profile{};
            attempt.get_hr = profile_mgr->GetProfile(
                TF_PROFILETYPE_INPUTPROCESSOR,
                kLangId,
                localpinyin::CLSID_LocalPinyinTextService,
                localpinyin::GUID_LocalPinyinProfile,
                nullptr,
                &profile);
            if (SUCCEEDED(attempt.get_hr)) {
                attempt.get_matches_clsid = IsEqualGUID(
                    profile.clsid, localpinyin::CLSID_LocalPinyinTextService);
                attempt.get_matches_guid = IsEqualGUID(
                    profile.guidProfile, localpinyin::GUID_LocalPinyinProfile);
                attempt.get_matches_langid = profile.langid == kLangId;
                if (localpinyin::setup::profile_get_matches(attempt)) {
                    final_matching_profile = profile;
                    have_final_matching_profile = true;
                }
            }

            IEnumTfInputProcessorProfiles* enum_profiles = nullptr;
            attempt.enum_hr = profile_mgr->EnumProfiles(kLangId, &enum_profiles);
            if (SUCCEEDED(attempt.enum_hr) && enum_profiles) {
                for (;;) {
                    TF_INPUTPROCESSORPROFILE item{};
                    ULONG fetched = 0;
                    const HRESULT next_hr = enum_profiles->Next(1, &item, &fetched);
                    if (next_hr == S_FALSE || fetched == 0) {
                        break;
                    }
                    if (FAILED(next_hr)) {
                        attempt.enum_hr = next_hr;
                        break;
                    }
                    ++attempt.enum_count;
                    attempt.enum_found = attempt.enum_found || is_local_profile(item);
                }
                enum_profiles->Release();
            }
            profile_mgr->Release();
        } else {
            attempt.get_hr = attempt.manager_hr;
            attempt.enum_hr = attempt.manager_hr;
        }

        state.profile_attempts.push_back(attempt);
        append_diagnostic_line(
            L"operation=VerifyProfileVisibility"
            L" profile_verification_attempt=" + std::to_wstring(index + 1) +
            L" profile_manager_source=fresh_manager"
            L" profile_manager_hr=" + hresult_hex(attempt.manager_hr) +
            L" profile_get_hr=" + hresult_hex(attempt.get_hr) +
            L" profile_get_succeeded=" + bool_text(SUCCEEDED(attempt.get_hr)) +
            L" profile_get_matches_clsid=" + bool_text(attempt.get_matches_clsid) +
            L" profile_get_matches_guid=" + bool_text(attempt.get_matches_guid) +
            L" profile_get_matches_langid=" + bool_text(attempt.get_matches_langid) +
            L" profile_enum_hr=" + hresult_hex(attempt.enum_hr) +
            L" profile_enum_count=" + std::to_wstring(attempt.enum_count) +
            L" profile_enum_found=" + bool_text(attempt.enum_found) +
            L" profile_verification_result=" +
                std::wstring(localpinyin::setup::profile_visibility_attempt_succeeded(attempt)
                                 ? L"Visible"
                                 : L"RetryableNotVisible"));
        if (localpinyin::setup::profile_visibility_attempt_succeeded(attempt)) {
            break;
        }
    }

    state.profile_verification =
        localpinyin::setup::evaluate_profile_visibility_attempts(
            state.profile_attempts);
    if (!state.profile_attempts.empty()) {
        const auto& final_attempt = state.profile_verification.final_attempt;
        state.get_profile_hr = final_attempt.get_hr;
        state.get_profile_ok = localpinyin::setup::profile_get_matches(final_attempt);
        state.enum_profiles_hr = final_attempt.enum_hr;
        state.enum_profiles_contains = final_attempt.enum_found;
    }
    if (have_final_matching_profile) {
        state.profile_flags = final_matching_profile.dwFlags;
        state.profile_caps = final_matching_profile.dwCaps;
        state.profile_enabled_flag =
            (final_matching_profile.dwFlags & TF_IPP_FLAG_ENABLED) != 0;
        state.profile_active_flag =
            (final_matching_profile.dwFlags & TF_IPP_FLAG_ACTIVE) != 0;
        state.required_caps_present =
            (final_matching_profile.dwCaps & localpinyin::required_tsf_profile_caps()) ==
            localpinyin::required_tsf_profile_caps();
    }

    ITfCategoryMgr* category_mgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&category_mgr));
    if (FAILED(hr)) {
        for (const auto& category : localpinyin::required_tsf_profile_categories()) {
            state.categories.push_back(TsfState::CategoryState{
                category.name, hr, 0, false, hr, 0, false, false,
                category.allow_inconclusive_post_register_verification});
        }
        for (const auto& category : localpinyin::compatibility_tsf_profile_categories()) {
            state.compatibility_categories.push_back(TsfState::CategoryState{
                category.name, hr, 0, false, hr, 0, false, false,
                category.allow_inconclusive_post_register_verification});
        }
        return state;
    }

    for (const auto& category : localpinyin::required_tsf_profile_categories()) {
        const auto category_to_items =
            enumerate_category_to_items(category_mgr,
                                        *category.category_guid,
                                        *category.item_guid);
        const auto item_to_categories =
            enumerate_item_to_categories(category_mgr,
                                         *category.item_guid,
                                         *category.category_guid);
        state.categories.push_back(TsfState::CategoryState{
            category.name,
            category_to_items.hr,
            category_to_items.count,
            category_to_items.found,
            item_to_categories.hr,
            item_to_categories.count,
            item_to_categories.found,
            category_to_items.found || item_to_categories.found,
            category.allow_inconclusive_post_register_verification});
    }
    for (const auto& category : localpinyin::compatibility_tsf_profile_categories()) {
        const auto category_to_items =
            enumerate_category_to_items(category_mgr,
                                        *category.category_guid,
                                        *category.item_guid);
        const auto item_to_categories =
            enumerate_item_to_categories(category_mgr,
                                         *category.item_guid,
                                         *category.category_guid);
        state.compatibility_categories.push_back(TsfState::CategoryState{
            category.name,
            category_to_items.hr,
            category_to_items.count,
            category_to_items.found,
            item_to_categories.hr,
            item_to_categories.count,
            item_to_categories.found,
            category_to_items.found || item_to_categories.found,
            category.allow_inconclusive_post_register_verification});
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
    for (size_t index = 0; index < state.profile_attempts.size(); ++index) {
        const auto& attempt = state.profile_attempts[index];
        std::wcout << L"Profile visibility attempt: " << (index + 1) << L"\n"
                   << L"Profile manager source: fresh_manager\n"
                   << L"Profile manager HRESULT: " << hresult_hex(attempt.manager_hr) << L"\n"
                   << L"GetProfile HRESULT: " << hresult_hex(attempt.get_hr) << L"\n"
                   << L"GetProfile matches CLSID: " << bool_text(attempt.get_matches_clsid) << L"\n"
                   << L"GetProfile matches profile GUID: " << bool_text(attempt.get_matches_guid) << L"\n"
                   << L"GetProfile matches LANGID: " << bool_text(attempt.get_matches_langid) << L"\n"
                   << L"EnumProfiles HRESULT: " << hresult_hex(attempt.enum_hr) << L"\n"
                   << L"EnumProfiles count: " << attempt.enum_count << L"\n"
                   << L"EnumProfiles found LocalPinyinIME: " << bool_text(attempt.enum_found) << L"\n";
    }
    std::wcout << L"Profile CLSID: " << kClsidText << L"\n"
               << L"Profile GUID: " << kProfileGuidText << L"\n"
               << L"Profile LANGID: 0x0804\n"
               << L"GetProfile HRESULT: " << hresult_hex(state.get_profile_hr) << L"\n"
               << L"Profile valid: " << bool_text(state.get_profile_ok) << L"\n"
               << L"Profile registered: " << bool_text(state.enum_profiles_contains) << L"\n"
               << L"Profile enabled flag: " << bool_text(state.profile_enabled_flag) << L"\n"
               << L"Profile active flag: " << bool_text(state.profile_active_flag) << L"\n"
               << L"Profile flags: " << win32_hex(state.profile_flags) << L"\n"
               << L"EnumProfiles HRESULT: " << hresult_hex(state.enum_profiles_hr) << L"\n"
               << L"EnumProfiles contains LocalPinyinIME: " << bool_text(state.enum_profiles_contains) << L"\n"
               << L"Required TSF categories:\n"
               << L"All required TSF categories contain LocalPinyinIME CLSID: "
               << bool_text(all_categories_present) << L"\n";
    for (const auto& category : state.categories) {
        std::wcout << category.name << L" Category->Items HRESULT: "
                   << hresult_hex(category.category_to_items_hr) << L"\n"
                   << category.name << L" Category->Items count: "
                   << category.category_to_items_count << L"\n"
                   << category.name << L" Category->Items found: "
                   << bool_text(category.category_to_items_found) << L"\n"
                   << category.name << L" Item->Categories HRESULT: "
                   << hresult_hex(category.item_to_categories_hr) << L"\n"
                   << category.name << L" Item->Categories count: "
                   << category.item_to_categories_count << L"\n"
                   << category.name << L" Item->Categories found: "
                   << bool_text(category.item_to_categories_found) << L"\n"
                   << category.name << L" contains LocalPinyinIME CLSID: "
                   << bool_text(category.contains) << L"\n"
                   << category.name << L" unobserved membership fatal to core verification: "
                   << bool_text(!category.allow_inconclusive) << L"\n";
    }
    std::wcout << L"Compatibility / capability diagnostics:\n";
    for (const auto& category : state.compatibility_categories) {
        std::wcout << category.name << L" register policy: diagnostic\n"
                   << category.name << L" register attempted by verify command: FALSE\n"
                   << category.name << L" Category->Items HRESULT: "
                   << hresult_hex(category.category_to_items_hr) << L"\n"
                   << category.name << L" Item->Categories HRESULT: "
                   << hresult_hex(category.item_to_categories_hr) << L"\n"
                   << category.name << L" enum contains LocalPinyinIME CLSID: "
                   << bool_text(category.contains) << L"\n"
                   << category.name << L" fatal to system registration: FALSE\n"
                   << category.name << L" reason: compatibility/capability diagnostic; EnumItemsInCategory is not required for install success\n";
    }
    std::wcout << L"Profile internal caps diagnostic:\n"
               << L"Profile internal dwCaps: " << win32_hex(state.profile_caps) << L"\n"
               << L"Profile internal dwCaps required bits present (diagnostic only): " << bool_text(state.required_caps_present) << L"\n"
               << L"Profile internal dwCaps expected bits (diagnostic only): " << win32_hex(caps.expected_caps) << L"\n"
               << L"Profile internal dwCaps missing bits (diagnostic only): " << win32_hex(caps.missing_caps) << L"\n"
               << L"Profile internal dwCaps diagnostic only: TRUE\n";
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

HRESULT verify_system_registration(HRESULT expected_dll_hr,
                                   bool profile_registration_performed_by_dll,
                                   HRESULT dll_register_server_hr) {
    const TsfState state = query_tsf_state();
    print_tsf_state(state);
    const bool non_keyboard_required_categories_present =
        std::all_of(state.categories.begin(), state.categories.end(),
                    [](const TsfState::CategoryState& item) {
                        return item.contains || item.allow_inconclusive;
                    });
    const bool verified = state.profile_verification.visible &&
                          non_keyboard_required_categories_present &&
                          SUCCEEDED(expected_dll_hr);
    const HRESULT profile_visibility_hr = state.profile_verification.visible
        ? S_OK
        : state.profile_verification.failure_hr;
    append_diagnostic_line(
        L"operation=VerifySystemRegistration"
        L" profile_registration_performed_by_dll=" +
            bool_text(profile_registration_performed_by_dll) +
        L" dll_register_server_hr=" +
            std::wstring(profile_registration_performed_by_dll
                             ? hresult_hex(dll_register_server_hr)
                             : L"NOT_PERFORMED") +
        L" profile_verification_attempt=" +
            std::to_wstring(state.profile_verification.attempts_used) +
        L" profile_manager_source=fresh_manager_per_attempt"
        L" profile_get_hr=" + hresult_hex(state.get_profile_hr) +
        L" profile_get_succeeded=" + bool_text(SUCCEEDED(state.get_profile_hr)) +
        L" profile_get_matches_clsid=" +
            bool_text(state.profile_verification.final_attempt.get_matches_clsid) +
        L" profile_get_matches_guid=" +
            bool_text(state.profile_verification.final_attempt.get_matches_guid) +
        L" profile_get_matches_langid=" +
            bool_text(state.profile_verification.final_attempt.get_matches_langid) +
        L" profile_enum_hr=" + hresult_hex(state.enum_profiles_hr) +
        L" profile_enum_count=" +
            std::to_wstring(state.profile_verification.final_attempt.enum_count) +
        L" profile_enum_found=" + bool_text(state.enum_profiles_contains) +
        L" profile_verification_result=" +
            std::wstring(state.profile_verification.visible ? L"Visible" : L"RetryExhausted") +
        L" final_profile_get_succeeded=" + bool_text(SUCCEEDED(state.get_profile_hr)) +
        L" final_profile_enum_found=" + bool_text(state.enum_profiles_contains) +
        L" final_profile_visible=" + bool_text(state.profile_verification.visible) +
        L" final_expected_dll_verified=" + bool_text(SUCCEEDED(expected_dll_hr)) +
        L" non_keyboard_required_categories_present=" +
            bool_text(non_keyboard_required_categories_present) +
        L" result=" + bool_text(verified));
    if (verified) {
        return S_OK;
    }
    if (FAILED(profile_visibility_hr)) {
        return profile_visibility_hr;
    }
    if (FAILED(expected_dll_hr)) {
        return expected_dll_hr;
    }
    return E_FAIL;
}

struct RegistrationVerificationResult {
    localpinyin::setup::ExpectedDllVerification expected_dll;
    bool system_registered = false;
    HRESULT system_registration_hr = E_FAIL;

    bool ok() const {
        return SUCCEEDED(expected_dll.hr) && system_registered;
    }
};

RegistrationVerificationResult verify_registered(const std::wstring* expected_dll_path,
                                                  bool profile_registration_performed_by_dll,
                                                  HRESULT dll_register_server_hr) {
    RegistrationVerificationResult result{};
    result.expected_dll = verify_expected_dll(expected_dll_path);
    result.system_registration_hr = verify_system_registration(
        result.expected_dll.hr,
        profile_registration_performed_by_dll,
        dll_register_server_hr);
    result.system_registered = SUCCEEDED(result.system_registration_hr);
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

HRESULT query_current_user_enabled(bool& enabled, REFGUID profile = localpinyin::GUID_LocalPinyinProfile) {
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
                                            profile,
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

// Whether the user turned 本地英文 off in the settings ("englishIme": false
// in settings.json): an update then leaves it out of the list.
bool english_turned_off() {
    std::wstring base;
    wchar_t override_path[MAX_PATH * 2]{};
    const DWORD length = GetEnvironmentVariableW(L"LOCALPINYINIME_LOCALAPPDATA", override_path,
                                                 static_cast<DWORD>(std::size(override_path)));
    if (length > 0 && length < std::size(override_path)) {
        base = override_path;
    } else {
        PWSTR path = nullptr;
        if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path))) return false;
        base = path;
        CoTaskMemFree(path);
    }
    std::ifstream file(std::filesystem::path(base) / L"LocalPinyinIME" / L"settings.json", std::ios::binary);
    if (!file) return false;
    const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    const size_t key = text.find("\"englishIme\"");
    if (key == std::string::npos) return false;
    const size_t value = text.find_first_not_of(" \t\r\n:", key + 12);
    return value != std::string::npos && text.compare(value, 5, "false") == 0;
}

int enable_or_disable_current_user(bool enable) {
    std::wcout << L"ProfileSpec canonical: " << kProfileSpec << L"\n"
               << L"ProfileSpec compact equivalent: " << kProfileSpecCompact << L"\n"
               << L"English ProfileSpec: " << kEnglishProfileSpec << L"\n";

    bool before = false;
    HRESULT hr = query_current_user_enabled(before);
    std::wcout << L"before.IsEnabledLanguageProfile HRESULT: " << hresult_hex(hr) << L"\n"
               << L"enabled before: " << bool_text(before) << L"\n";
    if (FAILED(hr)) {
        return 1;
    }
    bool english_before = false;
    const HRESULT english_hr = query_current_user_enabled(english_before, localpinyin::GUID_LocalPinyinEnglishProfile);
    std::wcout << L"before.IsEnabledLanguageProfile English HRESULT: " << hresult_hex(english_hr) << L"\n"
               << L"English enabled before: " << bool_text(english_before) << L"\n";
    // An English profile that is not registered (an older DLL) is left alone,
    // and so is one the user turned off in the settings.
    const bool english_known = SUCCEEDED(english_hr);
    const bool english_wanted = english_known && !(enable && !english_before && english_turned_off());
    if (english_known && !english_wanted) std::wcout << L"English turned off in the settings: left out\n";

    if (enable && before && (english_before || !english_wanted)) {
        std::wcout << L"already enabled\n";
        return 0;
    }
    if (!enable && !before && !english_before) {
        std::wcout << L"already disabled\n";
        return 0;
    }
    // Upgrades enable 本地英文 beside an already enabled 本地拼音.
    std::wstring spec;
    if (before != enable) spec += kProfileSpec;
    if (english_wanted && english_before != enable) spec += kEnglishProfileSpec;

    HMODULE input = nullptr;
    InstallLayoutOrTipFn install = nullptr;
    hr = load_install_layout_or_tip(input, install);
    if (FAILED(hr)) {
        std::wcerr << L"Load InstallLayoutOrTip HRESULT: " << hresult_hex(hr) << L"\n";
        return 1;
    }

    const DWORD flags = enable ? kInstallLayoutOrTipFlags : kIlotUninstall;
    SetLastError(ERROR_SUCCESS);
    const BOOL ok = install(spec.c_str(), flags);
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
    bool english_after = false;
    const HRESULT english_after_hr = query_current_user_enabled(english_after, localpinyin::GUID_LocalPinyinEnglishProfile);
    std::wcout << L"after.IsEnabledLanguageProfile English HRESULT: " << hresult_hex(english_after_hr) << L"\n"
               << L"English enabled after: " << bool_text(english_after) << L"\n";
    std::wcout << L"current user InputMethodTips verification: use --verify --require-current-user-enabled\n";
    if (FAILED(hr)) {
        return 1;
    }
    return after == enable && (!english_wanted || english_after == enable) ? 0 : 1;
}

// Deletes `folder` and all in it, not following links (a junction is
// removed, not what it points to). What running programs still hold is
// deleted at the next restart (needs administrator rights).
void remove_folder(const std::wstring& folder, size_t& deleted, size_t& at_restart) {
    WIN32_FIND_DATAW data{};
    HANDLE find = FindFirstFileW((folder + L"\\*").c_str(), &data);
    if (find != INVALID_HANDLE_VALUE) {
        do {
            const std::wstring name = data.cFileName;
            if (name == L"." || name == L"..") continue;
            const std::wstring path = folder + L"\\" + name;
            const bool directory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            if (directory && !(data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                remove_folder(path, deleted, at_restart);
                continue;
            }
            if (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL);
            if (directory ? RemoveDirectoryW(path.c_str()) : DeleteFileW(path.c_str())) ++deleted;
            else if (MoveFileExW(path.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT)) ++at_restart;
        } while (FindNextFileW(find, &data));
        FindClose(find);
    }
    // After the files scheduled in it, so it is empty by then.
    if (!RemoveDirectoryW(folder.c_str())) MoveFileExW(folder.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
}

// --remove-old-releases: every version installs into a folder of its own,
// LocalPinyinIME\releases\<version>, beside the earlier ones, which would pile
// up (each holds the whole settings app). After an update this removes all
// but the folder this tool runs from. Only folders named like a version
// (0.20.3, 0.3.10-dev) go; files programs still hold (the input method in an
// open application, a running recorder or settings window) go at restart.
int remove_old_releases() {
    namespace fs = std::filesystem;
    wchar_t self[MAX_PATH * 2]{};
    GetModuleFileNameW(nullptr, self, static_cast<DWORD>(std::size(self)));
    const fs::path version_folder = fs::path(self).parent_path().parent_path();
    const fs::path releases = version_folder.parent_path();
    if (_wcsicmp(releases.filename().c_str(), L"releases") != 0 ||
        _wcsicmp(releases.parent_path().filename().c_str(), L"LocalPinyinIME") != 0) {
        std::wcerr << L"error: not run from a LocalPinyinIME\\releases\\<version> folder\n";
        return 2;
    }
    size_t folders = 0, deleted = 0, at_restart = 0;
    WIN32_FIND_DATAW data{};
    HANDLE find = FindFirstFileW((releases.wstring() + L"\\*").c_str(), &data);
    if (find != INVALID_HANDLE_VALUE) {
        do {
            const std::wstring name = data.cFileName;
            const bool version_like = !name.empty() && name[0] >= L'0' && name[0] <= L'9' &&
                name.find_first_not_of(L"0123456789.-abcdefghijklmnopqrstuvwxyz") == std::wstring::npos;
            if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
                !version_like || _wcsicmp(name.c_str(), version_folder.filename().c_str()) == 0)
                continue;
            ++folders;
            remove_folder((releases / name).wstring(), deleted, at_restart);
        } while (FindNextFileW(find, &data));
        FindClose(find);
    }
    const std::wstring summary = L"old releases: " + std::to_wstring(folders) + L" folders, " + std::to_wstring(deleted) +
                                 L" files deleted, " + std::to_wstring(at_restart) + L" at restart";
    std::wcout << summary << L"\n";
    append_diagnostic_line(L"LocalPinyinImeSetup " + summary);
    return 0;
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
        << L"  LocalPinyinImeSetup.exe --version\n"
        << L"  LocalPinyinImeSetup.exe --remove-old-releases\n";
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    std::setlocale(LC_ALL, ".UTF-8");
    std::wstring command;
    std::wstring dll_path;
    std::wstring expected_dll_path;
    bool require_current_user_enabled = false;

    for (int i = 1; i < argc; ++i) {
        const std::wstring arg = argv[i];
        if (arg == L"--register-system" || arg == L"--unregister-system" ||
            arg == L"--enable-current-user" || arg == L"--disable-current-user" ||
            arg == L"--verify" || arg == L"--version" || arg == L"--remove-old-releases") {
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

    if (command == L"--remove-old-releases") return remove_old_releases();
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
                const RegistrationVerificationResult registered =
                    verify_registered(&dll_path, true, register_hr);
                if (FAILED(register_hr)) {
                    write_failure_summary(L"register-system", L"DllRegisterServer", register_hr, dll_path);
                } else if (FAILED(registered.expected_dll.hr)) {
                    write_failure_summary(L"register-system", L"VerifyExpectedDll", registered.expected_dll.hr, dll_path);
                } else if (!registered.system_registered) {
                    write_failure_summary(L"register-system",
                                          L"VerifySystemRegistration/ProfileVisibility",
                                          registered.system_registration_hr,
                                          dll_path);
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
        const RegistrationVerificationResult registered =
            verify_registered(expected_ptr, false, E_UNEXPECTED);
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
