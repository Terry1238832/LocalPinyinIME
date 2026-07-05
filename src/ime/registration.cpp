#include "registration.h"

#include "globals.h"
#include "guids.h"
#include "tsf_profile_categories.h"
#include "../common/logging.h"
#include "../common/win32_utils.h"

#include <msctf.h>

#include <string>
#include <vector>

namespace localpinyin {
namespace {

constexpr wchar_t kDisplayName[] = L"LocalPinyinIME - \u79BB\u7EBF\u62FC\u97F3\u8F93\u5165\u6CD5";
constexpr LANGID kLangId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);

std::wstring g_last_registration_diagnostic;

std::wstring guid_to_string(REFGUID guid) {
    wchar_t buffer[64]{};
    StringFromGUID2(guid, buffer, static_cast<int>(_countof(buffer)));
    return buffer;
}

std::wstring win32_error_text(LSTATUS status) {
    wchar_t buffer[64]{};
    swprintf_s(buffer, L"%lu / 0x%08lX", static_cast<unsigned long>(status), static_cast<unsigned long>(status));
    return buffer;
}

std::wstring format_hresult(HRESULT hr) {
    wchar_t buffer[16]{};
    swprintf_s(buffer, L"0x%08lX", static_cast<unsigned long>(hr));
    return buffer;
}

std::wstring format_dword(DWORD value) {
    wchar_t buffer[16]{};
    swprintf_s(buffer, L"0x%08lX", static_cast<unsigned long>(value));
    return buffer;
}

std::wstring format_langid(LANGID langid) {
    wchar_t buffer[16]{};
    swprintf_s(buffer, L"0x%04X", static_cast<unsigned int>(langid));
    return buffer;
}

std::wstring format_step(const std::wstring& step, HRESULT hr) {
    return step + (FAILED(hr) ? L" failed " : L" succeeded ") + format_hresult(hr);
}

void log_step_result(const std::wstring& step, HRESULT hr) {
    log_status(L"registration", format_step(step, hr));
}

void merge_failure(HRESULT hr, HRESULT& first_failure) {
    if (FAILED(hr) && SUCCEEDED(first_failure)) {
        first_failure = hr;
    }
}

void log_operation_result(const std::wstring& operation,
                          HRESULT hr,
                          const std::wstring& target_dll = L"",
                          const GUID* category = nullptr) {
    std::wstring line = L"operation=" + operation +
                        L" hr=" + format_hresult(hr) +
                        L" clsid=" + guid_to_string(CLSID_LocalPinyinTextService) +
                        L" profile=" + guid_to_string(GUID_LocalPinyinProfile) +
                        L" langid=" + format_langid(kLangId);
    if (!target_dll.empty()) {
        line += L" target_dll=" + target_dll;
    }
    if (category) {
        line += L" category=" + guid_to_string(*category);
    }
    log_status(L"registration", line);
}

void clear_last_registration_diagnostic() {
    g_last_registration_diagnostic.clear();
}

void set_last_registration_diagnostic(const std::wstring& stage,
                                      const std::wstring& operation,
                                      HRESULT hr,
                                      const std::wstring& target_dll,
                                      const GUID* category = nullptr,
                                      bool added_state = false) {
    g_last_registration_diagnostic =
        L"stage=" + stage +
        L" operation=" + operation +
        L" hr=" + format_hresult(hr) +
        L" target_dll=" + target_dll +
        L" clsid=" + guid_to_string(CLSID_LocalPinyinTextService) +
        L" profile=" + guid_to_string(GUID_LocalPinyinProfile) +
        L" langid=" + format_langid(kLangId) +
        L" added_this_call=" + std::wstring(added_state ? L"TRUE" : L"FALSE");
    if (category) {
        g_last_registration_diagnostic += L" category=" + guid_to_string(*category);
    }
    log_status(L"registration", L"last-registration-diagnostic " + g_last_registration_diagnostic);
}

void append_last_registration_diagnostic(const std::wstring& detail) {
    if (g_last_registration_diagnostic.empty()) {
        g_last_registration_diagnostic = detail;
    } else {
        g_last_registration_diagnostic += L" " + detail;
    }
    log_status(L"registration", L"last-registration-diagnostic " + g_last_registration_diagnostic);
}

std::wstring strategy_text(TsfCapabilityCheckStrategy strategy) {
    switch (strategy) {
    case TsfCapabilityCheckStrategy::CategoryContainsItem:
        return L"CategoryContainsItem";
    case TsfCapabilityCheckStrategy::ProfileCaps:
        return L"ProfileCaps";
    }
    return L"Unknown";
}

std::wstring bool_field(bool value) {
    return value ? L"TRUE" : L"FALSE";
}

bool has_last_registration_diagnostic() {
    return !g_last_registration_diagnostic.empty();
}

std::wstring category_identity_fields(const TsfProfileCategory& category) {
    return L" category_name=" + std::wstring(category.name) +
           L" category_guid=" + guid_to_string(*category.category_guid) +
           L" item_guid=" + guid_to_string(*category.item_guid) +
           L" allow_register_category=" + bool_field(category.allow_register_category) +
           L" check_strategy=" + strategy_text(category.check_strategy) +
           L" profile_caps=" + format_hresult(static_cast<HRESULT>(category.profile_caps));
}

void log_category_diagnostic(const std::wstring& stage,
                             const TsfProfileCategory& category,
                             HRESULT hr,
                             const std::wstring& target_dll,
                             HRESULT enum_before_hr,
                             bool exists_before,
                             bool register_called,
                             HRESULT register_hr,
                             HRESULT enum_after_hr,
                             bool exists_after,
                             bool added_this_call,
                             bool rollback_attempted = false,
                             HRESULT rollback_hr = S_OK) {
    std::wstring line =
        L"stage=" + stage +
        L" operation=" + stage +
        L" hr=" + format_hresult(hr) +
        category_identity_fields(category) +
        L" clsid=" + guid_to_string(CLSID_LocalPinyinTextService) +
        L" profile=" + guid_to_string(GUID_LocalPinyinProfile) +
        L" langid=" + format_langid(kLangId) +
        L" enum_before_hr=" + format_hresult(enum_before_hr) +
        L" exists_before=" + bool_field(exists_before) +
        L" register_called=" + bool_field(register_called) +
        L" register_hr=" + format_hresult(register_hr) +
        L" enum_after_hr=" + format_hresult(enum_after_hr) +
        L" exists_after=" + bool_field(exists_after) +
        L" added_this_call=" + bool_field(added_this_call) +
        L" rollback_attempted=" + bool_field(rollback_attempted) +
        L" rollback_hr=" + format_hresult(rollback_hr);
    if (!target_dll.empty()) {
        line += L" target_dll=" + target_dll;
    }
    log_status(L"registration", line);
    if (FAILED(hr)) {
        if (g_last_registration_diagnostic.empty()) {
            g_last_registration_diagnostic = line;
        } else {
            g_last_registration_diagnostic += L" secondary_" + line;
        }
        log_status(L"registration", L"last-registration-diagnostic " + g_last_registration_diagnostic);
    }
}

void log_failure_line(const std::wstring& stage,
                      HRESULT hr,
                      const std::wstring& target_dll,
                      const GUID* category = nullptr,
                      bool added_state = false) {
    std::wstring line = L"register-system failed: stage=" + stage +
                        L" hr=" + format_hresult(hr) +
                        L" target_dll=" + target_dll +
                        L" clsid=" + guid_to_string(CLSID_LocalPinyinTextService) +
                        L" profile=" + guid_to_string(GUID_LocalPinyinProfile) +
                        L" langid=" + format_langid(kLangId);
    if (category) {
        line += L" category=" + guid_to_string(*category);
    }
    log_status(L"registration", line);
    set_last_registration_diagnostic(stage, stage, hr, target_dll, category, added_state);
}

bool is_missing_status(LSTATUS status) {
    return status == ERROR_FILE_NOT_FOUND || status == ERROR_PATH_NOT_FOUND || status == ERROR_NOT_FOUND;
}

std::wstring read_reg_string(HKEY root, const std::wstring& subkey, const wchar_t* name) {
    HKEY key = nullptr;
    LSTATUS status = RegOpenKeyExW(root, subkey.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &key);
    if (status != ERROR_SUCCESS) {
        return L"";
    }

    DWORD type = 0;
    DWORD bytes = 0;
    status = RegQueryValueExW(key, name, nullptr, &type, nullptr, &bytes);
    if (status != ERROR_SUCCESS || type != REG_SZ || bytes < sizeof(wchar_t)) {
        RegCloseKey(key);
        return L"";
    }

    std::wstring value(bytes / sizeof(wchar_t), L'\0');
    status = RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<BYTE*>(value.data()), &bytes);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS) {
        return L"";
    }
    while (!value.empty() && value.back() == L'\0') {
        value.pop_back();
    }
    return value;
}

HRESULT set_reg_string(HKEY root, const std::wstring& subkey, const wchar_t* name, const std::wstring& value) {
    HKEY key = nullptr;
    LSTATUS status = RegCreateKeyExW(root,
                                     subkey.c_str(),
                                     0,
                                     nullptr,
                                     0,
                                     KEY_WRITE | KEY_WOW64_64KEY,
                                     nullptr,
                                     &key,
                                     nullptr);
    if (status != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(status);
    }
    status = RegSetValueExW(key, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()),
                            static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
    return HRESULT_FROM_WIN32(status);
}

HRESULT delete_registry_tree(HKEY root, const std::wstring& parent_subkey, const std::wstring& child_name,
                             const std::wstring& log_name) {
    HKEY parent = nullptr;
    LSTATUS status = RegOpenKeyExW(root,
                                   parent_subkey.c_str(),
                                   0,
                                   DELETE | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_SET_VALUE |
                                       KEY_WOW64_64KEY,
                                   &parent);
    log_status(L"registration", log_name + L" open parent win32 " + win32_error_text(status));
    if (is_missing_status(status)) {
        log_step_result(log_name + L" already absent", S_OK);
        return S_OK;
    }
    if (status != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(status);
    }

    HKEY child = nullptr;
    status = RegOpenKeyExW(parent, child_name.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &child);
    log_status(L"registration", log_name + L" precheck win32 " + win32_error_text(status));
    if (is_missing_status(status)) {
        RegCloseKey(parent);
        log_step_result(log_name + L" already absent", S_OK);
        return S_OK;
    }
    if (status != ERROR_SUCCESS) {
        RegCloseKey(parent);
        return HRESULT_FROM_WIN32(status);
    }
    RegCloseKey(child);

    status = RegDeleteTreeW(parent, child_name.c_str());
    log_status(L"registration", log_name + L" delete win32 " + win32_error_text(status));
    if (is_missing_status(status)) {
        RegCloseKey(parent);
        return S_OK;
    }
    if (status != ERROR_SUCCESS) {
        RegCloseKey(parent);
        return HRESULT_FROM_WIN32(status);
    }

    status = RegOpenKeyExW(parent, child_name.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &child);
    log_status(L"registration", log_name + L" postcheck win32 " + win32_error_text(status));
    if (status == ERROR_SUCCESS) {
        RegCloseKey(child);
        RegCloseKey(parent);
        return HRESULT_FROM_WIN32(ERROR_DIR_NOT_EMPTY);
    }
    RegCloseKey(parent);
    return is_missing_status(status) ? S_OK : HRESULT_FROM_WIN32(status);
}

std::wstring read_com_inproc_server() {
    const std::wstring clsid = guid_to_string(CLSID_LocalPinyinTextService);
    return read_reg_string(HKEY_CLASSES_ROOT, L"CLSID\\" + clsid + L"\\InprocServer32", nullptr);
}

HRESULT unregister_com_class();

HRESULT register_com_class(const std::wstring& dll_path) {
    const std::wstring clsid = guid_to_string(CLSID_LocalPinyinTextService);
    const std::wstring base = L"CLSID\\" + clsid;
    const std::wstring inproc = base + L"\\InprocServer32";

    HRESULT hr = set_reg_string(HKEY_CLASSES_ROOT, base, nullptr, kDisplayName);
    if (FAILED(hr)) {
        log_operation_result(L"RegisterComDisplayName", hr, dll_path);
        return hr;
    }
    hr = set_reg_string(HKEY_CLASSES_ROOT, inproc, nullptr, dll_path);
    if (FAILED(hr)) {
        log_operation_result(L"RegisterComInprocServer32", hr, dll_path);
        return hr;
    }
    hr = set_reg_string(HKEY_CLASSES_ROOT, inproc, L"ThreadingModel", L"Apartment");
    log_operation_result(L"RegisterComThreadingModel", hr, dll_path);
    return hr;
}

HRESULT restore_com_class(const std::wstring& previous_dll_path) {
    if (previous_dll_path.empty()) {
        const HRESULT hr = unregister_com_class();
        log_operation_result(L"RollbackComRemoveNewClass", hr);
        return hr;
    }

    HRESULT hr = register_com_class(previous_dll_path);
    log_operation_result(L"RollbackComRestorePreviousInprocServer32", hr, previous_dll_path);
    return hr;
}

HRESULT unregister_com_class() {
    return delete_registry_tree(HKEY_CLASSES_ROOT,
                                L"CLSID",
                                guid_to_string(CLSID_LocalPinyinTextService),
                                L"delete COM CLSID root");
}

HRESULT unregister_tsf_tip_root() {
    return delete_registry_tree(HKEY_LOCAL_MACHINE,
                                L"SOFTWARE\\Microsoft\\CTF\\TIP",
                                guid_to_string(CLSID_LocalPinyinTextService),
                                L"delete TSF TIP root");
}

struct ProfileSnapshot {
    bool exists = false;
    DWORD flags = 0;
    DWORD caps = 0;
};

HRESULT query_profile_snapshot(ITfInputProcessorProfileMgr* profiles, ProfileSnapshot& snapshot) {
    snapshot = ProfileSnapshot{};
    IEnumTfInputProcessorProfiles* enum_profiles = nullptr;
    HRESULT enum_hr = profiles->EnumProfiles(kLangId, &enum_profiles);
    if (FAILED(enum_hr)) {
        return enum_hr;
    }

    if (enum_profiles) {
        for (;;) {
            TF_INPUTPROCESSORPROFILE profile{};
            ULONG fetched = 0;
            const HRESULT next_hr = enum_profiles->Next(1, &profile, &fetched);
            if (next_hr != S_OK || fetched == 0) {
                break;
            }
            if (profile.langid == kLangId &&
                IsEqualGUID(profile.clsid, CLSID_LocalPinyinTextService) &&
                IsEqualGUID(profile.guidProfile, GUID_LocalPinyinProfile)) {
                snapshot.exists = true;
                snapshot.flags = profile.dwFlags;
                snapshot.caps = profile.dwCaps;
                break;
            }
        }
        enum_profiles->Release();
    }
    return S_OK;
}

bool profile_registered(ITfInputProcessorProfileMgr* profiles, HRESULT& enum_hr) {
    enum_hr = S_OK;
    IEnumTfInputProcessorProfiles* enum_profiles = nullptr;
    enum_hr = profiles->EnumProfiles(kLangId, &enum_profiles);
    if (FAILED(enum_hr)) {
        return false;
    }

    bool found = false;
    if (enum_profiles) {
        for (;;) {
            TF_INPUTPROCESSORPROFILE profile{};
            ULONG fetched = 0;
            const HRESULT next_hr = enum_profiles->Next(1, &profile, &fetched);
            if (next_hr != S_OK || fetched == 0) {
                break;
            }
            if (profile.langid == kLangId &&
                IsEqualGUID(profile.clsid, CLSID_LocalPinyinTextService) &&
                IsEqualGUID(profile.guidProfile, GUID_LocalPinyinProfile)) {
                found = true;
                break;
            }
        }
        enum_profiles->Release();
    }
    return found;
}

bool category_registered(ITfCategoryMgr* category_mgr, REFGUID category, REFGUID expected_item, HRESULT& enum_hr) {
    enum_hr = S_OK;
    IEnumGUID* enum_items = nullptr;
    enum_hr = category_mgr->EnumItemsInCategory(category, &enum_items);
    if (FAILED(enum_hr)) {
        return false;
    }

    bool found = false;
    if (enum_items) {
        for (;;) {
            GUID current_item{};
            ULONG fetched = 0;
            const HRESULT next_hr = enum_items->Next(1, &current_item, &fetched);
            if (next_hr != S_OK || fetched == 0) {
                break;
            }
            if (IsEqualGUID(current_item, expected_item)) {
                found = true;
                break;
            }
        }
        enum_items->Release();
    }
    return found;
}

void log_profile_caps_observation(const ProfilePostRegisterVerification& verification,
                                  const std::wstring& target_dll) {
    std::wstring line =
        L"operation=ProfileCapsObservedAfterRegisterProfile" +
        std::wstring(L" hr=") + format_hresult(S_OK) +
        L" observed_profile_caps=" + format_dword(verification.caps.observed_caps) +
        L" expected_internal_caps=" + format_dword(verification.caps.expected_caps) +
        L" missing_internal_caps=" + format_dword(verification.caps.missing_caps) +
        L" all_required_internal_caps_present=" + bool_field(verification.caps.all_expected_caps_present) +
        L" blocking=FALSE" +
        L" clsid=" + guid_to_string(CLSID_LocalPinyinTextService) +
        L" profile=" + guid_to_string(GUID_LocalPinyinProfile) +
        L" langid=" + format_langid(kLangId);
    if (!target_dll.empty()) {
        line += L" target_dll=" + target_dll;
    }
    log_status(L"registration", line);
}

HRESULT register_tsf_profile(const std::wstring& icon_path,
                             const ProfileSnapshot& before,
                             bool& added_profile) {
    added_profile = false;
    const DWORD profile_flags = profile_registration_flags();
    ITfInputProcessorProfileMgr* profiles = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&profiles));
    if (FAILED(hr)) {
        log_operation_result(L"CreateProfileManager", hr, icon_path);
        return hr;
    }

    hr = profiles->RegisterProfile(CLSID_LocalPinyinTextService,
                                   kLangId,
                                   GUID_LocalPinyinProfile,
                                   kDisplayName,
                                   static_cast<ULONG>(wcslen(kDisplayName)),
                                   icon_path.c_str(),
                                   static_cast<ULONG>(icon_path.size()),
                                   0,
                                   nullptr,
                                   0,
                                   TRUE,
                                   profile_flags);
    if (hr == TF_E_ALREADY_EXISTS) {
        hr = S_OK;
    }
    log_operation_result(L"RegisterProfile", hr, icon_path);
    if (SUCCEEDED(hr)) {
        ProfileSnapshot after{};
        const HRESULT query_hr = query_profile_snapshot(profiles, after);
        const ProfilePostRegisterVerification verification =
            evaluate_profile_after_register(query_hr,
                                            after.exists,
                                            after.caps,
                                            before.exists,
                                            required_tsf_profile_caps());
        log_operation_result(L"VerifyProfileAfterRegisterProfile", query_hr, icon_path);
        if (verification.query_failed) {
            log_failure_line(L"VerifyProfileAfterRegisterProfile", verification.hr, icon_path);
            profiles->Release();
            return verification.hr;
        }
        if (verification.missing_profile) {
            log_failure_line(L"VerifyProfileAfterRegisterProfile",
                             verification.hr,
                             icon_path,
                             nullptr,
                             !before.exists);
            profiles->Release();
            return verification.hr;
        }
        log_profile_caps_observation(verification, icon_path);
        added_profile = verification.added_this_call;
    }
    profiles->Release();
    return hr;
}

HRESULT restore_tsf_profile(const ProfileSnapshot& before, const std::wstring& icon_path, bool added_profile) {
    const DWORD profile_flags = profile_registration_flags();
    ITfInputProcessorProfileMgr* profiles = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&profiles));
    if (FAILED(hr)) {
        log_operation_result(L"RollbackCreateProfileManager", hr, icon_path);
        return hr;
    }

    if (added_profile) {
        hr = profiles->UnregisterProfile(CLSID_LocalPinyinTextService, kLangId, GUID_LocalPinyinProfile, 0);
        log_operation_result(L"RollbackUnregisterNewProfile", hr, icon_path);
        profiles->Release();
        return hr;
    }

    if (before.exists) {
        const BOOL enabled = (before.flags & TF_IPP_FLAG_ENABLED) != 0 ? TRUE : FALSE;
        hr = profiles->RegisterProfile(CLSID_LocalPinyinTextService,
                                       kLangId,
                                       GUID_LocalPinyinProfile,
                                       kDisplayName,
                                       static_cast<ULONG>(wcslen(kDisplayName)),
                                       icon_path.c_str(),
                                       static_cast<ULONG>(icon_path.size()),
                                       0,
                                       nullptr,
                                       0,
                                       enabled,
                                       profile_flags);
        if (hr == TF_E_ALREADY_EXISTS) {
            hr = S_OK;
        }
        log_operation_result(L"RollbackRestoreExistingProfile", hr, icon_path);
    }
    profiles->Release();
    return hr;
}

HRESULT register_tsf_categories(const std::wstring& target_dll,
                                std::vector<const TsfProfileCategory*>& added_categories) {
    added_categories.clear();
    ITfCategoryMgr* category_mgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&category_mgr));
    if (FAILED(hr)) {
        log_operation_result(L"CreateCategoryManager", hr);
        return hr;
    }

    for (const auto& category : required_tsf_profile_categories()) {
        HRESULT enum_hr = S_OK;
        const bool contains_before =
            category_registered(category_mgr, *category.category_guid, *category.item_guid, enum_hr);
        const CategoryRegistrationDecision decision =
            decide_category_registration(enum_hr, contains_before, category.allow_register_category);
        log_category_diagnostic(L"EnumCategoryBeforeRegister",
                                category,
                                enum_hr,
                                target_dll,
                                enum_hr,
                                contains_before,
                                false,
                                S_OK,
                                S_OK,
                                contains_before,
                                false);
        if (decision.query_failed) {
            log_category_diagnostic(L"EnumItemsInCategory",
                                    category,
                                    enum_hr,
                                    target_dll,
                                    enum_hr,
                                    contains_before,
                                    false,
                                    S_OK,
                                    S_OK,
                                    contains_before,
                                    false);
            category_mgr->Release();
            return enum_hr;
        }
        if (decision.skip_existing) {
            log_category_diagnostic(L"RegisterCategorySkipExisting",
                                    category,
                                    S_OK,
                                    target_dll,
                                    enum_hr,
                                    contains_before,
                                    false,
                                    S_OK,
                                    enum_hr,
                                    contains_before,
                                    false);
            continue;
        }

        hr = category_mgr->RegisterCategory(CLSID_LocalPinyinTextService,
                                            *category.category_guid,
                                            *category.item_guid);
        if (hr == TF_E_ALREADY_EXISTS) {
            hr = S_OK;
        }
        log_step_result(std::wstring(L"register ") + category.name, hr);
        log_category_diagnostic(L"RegisterCategory",
                                category,
                                hr,
                                target_dll,
                                enum_hr,
                                contains_before,
                                true,
                                hr,
                                S_OK,
                                contains_before,
                                false);
        if (FAILED(hr)) {
            category_mgr->Release();
            return hr;
        }

        HRESULT verify_hr = S_OK;
        const bool contains_after =
            category_registered(category_mgr, *category.category_guid, *category.item_guid, verify_hr);
        const CategoryPostRegisterVerification verification =
            evaluate_category_after_register(verify_hr,
                                             contains_before,
                                             contains_after,
                                             category.allow_register_category);
        log_category_diagnostic(L"VerifyCategoryAfterRegister",
                                category,
                                verification.hr,
                                target_dll,
                                enum_hr,
                                contains_before,
                                true,
                                hr,
                                verify_hr,
                                contains_after,
                                verification.added_this_call);
        if (FAILED(verification.hr)) {
            category_mgr->Release();
            return verification.hr;
        }
        added_categories.push_back(&category);
    }
    category_mgr->Release();
    return S_OK;
}

HRESULT rollback_added_tsf_categories(const std::vector<const TsfProfileCategory*>& added_categories) {
    ITfCategoryMgr* category_mgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&category_mgr));
    if (FAILED(hr)) {
        log_operation_result(L"RollbackCreateCategoryManager", hr);
        return hr;
    }

    HRESULT first_failure = S_OK;
    for (auto it = added_categories.rbegin(); it != added_categories.rend(); ++it) {
        const TsfProfileCategory* category = *it;
        if (!category) {
            continue;
        }
        hr = category_mgr->UnregisterCategory(CLSID_LocalPinyinTextService,
                                              *category->category_guid,
                                              *category->item_guid);
        log_category_diagnostic(L"RollbackUnregisterNewCategory",
                                *category,
                                hr,
                                L"",
                                S_OK,
                                true,
                                false,
                                S_OK,
                                S_OK,
                                false,
                                false,
                                true,
                                hr);
        merge_failure(hr, first_failure);
    }
    category_mgr->Release();
    return first_failure;
}

HRESULT query_current_profile_snapshot(ProfileSnapshot& snapshot) {
    ITfInputProcessorProfileMgr* profiles = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&profiles));
    if (FAILED(hr)) {
        log_operation_result(L"CreateProfileManagerForSnapshot", hr);
        return hr;
    }
    hr = query_profile_snapshot(profiles, snapshot);
    log_operation_result(L"SnapshotProfileBeforeRegister", hr);
    profiles->Release();
    return hr;
}

HRESULT unregister_tsf_profile() {
    ITfInputProcessorProfileMgr* profiles = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&profiles));
    if (FAILED(hr)) {
        return hr;
    }
    HRESULT enum_hr = S_OK;
    if (!profile_registered(profiles, enum_hr)) {
        profiles->Release();
        return FAILED(enum_hr) ? enum_hr : S_OK;
    }

    HRESULT profile_hr = profiles->UnregisterProfile(CLSID_LocalPinyinTextService, kLangId, GUID_LocalPinyinProfile, 0);
    profiles->Release();
    return profile_hr;
}

HRESULT unregister_tsf_categories() {
    ITfCategoryMgr* category_mgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&category_mgr));
    if (FAILED(hr)) {
        return hr;
    }

    HRESULT first_failure = S_OK;
    for (const auto& category : required_tsf_profile_categories()) {
        HRESULT enum_hr = S_OK;
        if (!category_registered(category_mgr, *category.category_guid, *category.item_guid, enum_hr)) {
            const HRESULT absent_hr = FAILED(enum_hr) ? enum_hr : S_OK;
            log_step_result(std::wstring(L"unregister ") + category.name + L" absent", absent_hr);
            merge_failure(absent_hr, first_failure);
            continue;
        }

        hr = category_mgr->UnregisterCategory(CLSID_LocalPinyinTextService,
                                              *category.category_guid,
                                              *category.item_guid);
        log_step_result(std::wstring(L"unregister ") + category.name, hr);
        merge_failure(hr, first_failure);
    }
    category_mgr->Release();
    return first_failure;
}

}  // namespace

HRESULT register_server() {
    clear_last_registration_diagnostic();

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool co_initialized = SUCCEEDED(hr);
    if (hr == RPC_E_CHANGED_MODE) {
        co_initialized = false;
        hr = S_OK;
    }
    if (FAILED(hr)) {
        set_last_registration_diagnostic(L"CoInitializeEx", L"CoInitializeEx", hr, L"");
        return hr;
    }

    const std::wstring target_dll = module_path(g_instance);
    const std::wstring previous_dll = read_com_inproc_server();
    ProfileSnapshot profile_before{};
    bool added_profile = false;
    std::vector<const TsfProfileCategory*> added_categories;

    hr = query_current_profile_snapshot(profile_before);
    if (SUCCEEDED(hr)) {
        hr = register_tsf_categories(target_dll, added_categories);
    } else {
        log_failure_line(L"SnapshotProfileBeforeRegister", hr, target_dll);
    }
    if (SUCCEEDED(hr)) {
        hr = register_tsf_profile(target_dll, profile_before, added_profile);
        if (FAILED(hr)) {
            if (should_overwrite_registration_diagnostic(has_last_registration_diagnostic())) {
                log_failure_line(L"RegisterProfile", hr, target_dll, nullptr, added_profile);
            } else {
                append_last_registration_diagnostic(
                    L" outer_stage=RegisterProfile outer_hr=" + format_hresult(hr) +
                    L" target_dll=" + target_dll +
                    L" added_this_call=" + std::wstring(added_profile ? L"TRUE" : L"FALSE"));
            }
            append_last_registration_diagnostic(
                L"profile_registration_flags=" + format_dword(profile_registration_flags()));
        }
    }
    if (SUCCEEDED(hr)) {
        hr = register_com_class(target_dll);
        if (FAILED(hr)) {
            log_failure_line(L"RegisterComInprocServer32", hr, target_dll);
        }
    }

    if (FAILED(hr)) {
        const HRESULT category_rollback_hr = rollback_added_tsf_categories(added_categories);
        const std::wstring profile_restore_icon = previous_dll.empty() ? target_dll : previous_dll;
        const HRESULT profile_rollback_hr = restore_tsf_profile(profile_before, profile_restore_icon, added_profile);
        const HRESULT com_rollback_hr = restore_com_class(previous_dll);
        const bool rollback_succeeded = SUCCEEDED(category_rollback_hr) &&
                                        SUCCEEDED(profile_rollback_hr) &&
                                        SUCCEEDED(com_rollback_hr);
        append_last_registration_diagnostic(
            L"rollback_attempted=TRUE rollback_succeeded=" +
            std::wstring(rollback_succeeded ? L"TRUE" : L"FALSE") +
            L" rollback_categories=" + format_hresult(category_rollback_hr) +
            L" rollback_profile=" + format_hresult(profile_rollback_hr) +
            L" rollback_com=" + format_hresult(com_rollback_hr));
        log_status(L"registration",
                   L"rollback completed categories=" + format_hresult(category_rollback_hr) +
                       L" profile=" + format_hresult(profile_rollback_hr) +
                       L" com=" + format_hresult(com_rollback_hr));
    }

    log_status(L"registration", FAILED(hr) ? L"register failed " + format_hresult(hr) : L"register succeeded");
    if (co_initialized) {
        CoUninitialize();
    }
    return hr;
}

const wchar_t* last_registration_diagnostic() noexcept {
    return g_last_registration_diagnostic.c_str();
}

HRESULT unregister_server() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool co_initialized = SUCCEEDED(hr);
    if (hr == RPC_E_CHANGED_MODE) {
        co_initialized = false;
        hr = S_OK;
    }
    if (FAILED(hr)) {
        return hr;
    }

    HRESULT first_failure = S_OK;

    const HRESULT category_hr = unregister_tsf_categories();
    merge_failure(category_hr, first_failure);

    const HRESULT profile_hr = unregister_tsf_profile();
    log_step_result(L"unregister TSF profile", profile_hr);
    merge_failure(profile_hr, first_failure);

    const HRESULT tip_root_hr = unregister_tsf_tip_root();
    log_step_result(L"delete TSF TIP root", tip_root_hr);
    merge_failure(tip_root_hr, first_failure);

    const HRESULT com_hr = unregister_com_class();
    log_step_result(L"delete COM CLSID root", com_hr);
    merge_failure(com_hr, first_failure);

    hr = first_failure;

    log_status(L"registration", FAILED(hr) ? L"unregister failed " + format_hresult(hr) : L"unregister succeeded");
    if (co_initialized) {
        CoUninitialize();
    }
    return hr;
}

}  // namespace localpinyin

extern "C" __declspec(dllexport) const wchar_t* STDMETHODCALLTYPE LocalPinyinGetLastRegistrationDiagnostic(void) {
    return localpinyin::last_registration_diagnostic();
}
