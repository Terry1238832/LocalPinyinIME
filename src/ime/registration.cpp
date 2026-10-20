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

constexpr wchar_t kDisplayName[] = L"本地拼音";
constexpr wchar_t kEnglishDisplayName[] = L"本地英文";
constexpr LANGID kLangId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
// Profile icons are this DLL's icon resources by index, in resource ID order:
// 1 is the blue 中 tile (IDI_MODE_ZH), 2 the blue 英 tile (IDI_MODE_EN).
constexpr ULONG kProfileIconIndex = 1;
constexpr ULONG kEnglishProfileIconIndex = 2;

std::wstring g_last_registration_diagnostic;
std::wstring g_registration_warning;

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
    if (FAILED(hr)) {
        first_failure = preserve_first_registration_failure(first_failure, hr);
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
    g_registration_warning.clear();
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
           L" owner_clsid=" + guid_to_string(CLSID_LocalPinyinTextService) +
           L" item_guid=" + guid_to_string(*category.item_guid) +
           L" allow_register_category=" + bool_field(category.allow_register_category) +
           L" allow_inconclusive_post_register_verification=" +
               bool_field(category.allow_inconclusive_post_register_verification) +
           L" check_strategy=" + strategy_text(category.check_strategy) +
           L" profile_caps=" + format_hresult(static_cast<HRESULT>(category.profile_caps));
}

const wchar_t* category_verification_result_text(CategoryVerificationResult result) {
    switch (result) {
    case CategoryVerificationResult::RegisterCallFailed:
        return L"RegisterCallFailed";
    case CategoryVerificationResult::Verified:
        return L"Verified";
    case CategoryVerificationResult::VerificationInconclusive:
        return L"Inconclusive";
    }
    return L"Unknown";
}

void log_bidirectional_category_verification(
    const TsfProfileCategory& category,
    const std::wstring& target_dll,
    const BidirectionalCategoryVerification& verification,
    unsigned int attempt,
    bool continued_after_unobserved_registration) {
    const std::wstring line =
        L"stage=VerifyCategoryAfterRegister"
        L" operation=VerifyCategoryAfterRegister" +
        category_identity_fields(category) +
        L" clsid=" + guid_to_string(CLSID_LocalPinyinTextService) +
        L" profile=" + guid_to_string(GUID_LocalPinyinProfile) +
        L" langid=" + format_langid(kLangId) +
        L" register_called=TRUE"
        L" register_hr=" + format_hresult(verification.register_hr) +
        L" register_call_succeeded=" + bool_field(verification.register_call_succeeded) +
        L" category_to_items_enum_hr=" +
            format_hresult(verification.same_category_to_items.hr) +
        L" category_to_items_count=" +
            std::to_wstring(verification.same_category_to_items.count) +
        L" category_to_items_found=" +
            bool_field(verification.same_category_to_items.found) +
        L" item_to_categories_enum_hr=" +
            format_hresult(verification.same_item_to_categories.hr) +
        L" item_to_categories_count=" +
            std::to_wstring(verification.same_item_to_categories.count) +
        L" item_to_categories_found=" +
            bool_field(verification.same_item_to_categories.found) +
        L" fresh_manager_checked=" + bool_field(verification.fresh_manager_checked) +
        L" fresh_category_to_items_enum_hr=" +
            format_hresult(verification.fresh_category_to_items.hr) +
        L" fresh_category_to_items_count=" +
            std::to_wstring(verification.fresh_category_to_items.count) +
        L" fresh_category_to_items_found=" +
            bool_field(verification.fresh_category_to_items.found) +
        L" fresh_item_to_categories_enum_hr=" +
            format_hresult(verification.fresh_item_to_categories.hr) +
        L" fresh_item_to_categories_count=" +
            std::to_wstring(verification.fresh_item_to_categories.count) +
        L" fresh_item_to_categories_found=" +
            bool_field(verification.fresh_item_to_categories.found) +
        L" category_postcondition_observed=" +
            bool_field(verification.category_postcondition_observed) +
        L" verification_result=" +
            category_verification_result_text(verification.result) +
        L" verification_policy=" +
            std::wstring(category.allow_inconclusive_post_register_verification
                             ? L"keyboard_call_success_allows_inconclusive"
                             : L"required_observation") +
        L" continued_after_unobserved_registration=" +
            bool_field(continued_after_unobserved_registration) +
        L" verify_attempt=" + std::to_wstring(attempt) +
        L" target_dll=" + target_dll;
    log_status(L"registration", line);
    if (continued_after_unobserved_registration) {
        g_registration_warning = line;
    }
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
                              HRESULT rollback_hr = S_OK,
                              bool fresh_manager_checked = false,
                              HRESULT fresh_enum_after_hr = S_OK,
                              bool fresh_exists_after = false,
                              bool same_manager_stale = false,
                              bool fresh_manager_verified = false,
                              unsigned int verify_attempt = 0,
                              const wchar_t* manager_source = L"same_manager",
                              bool verification_fatal = true,
                              const wchar_t* policy_reason = L"required_category") {
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
        L" fresh_manager_checked=" + bool_field(fresh_manager_checked) +
        L" fresh_enum_after_hr=" + format_hresult(fresh_enum_after_hr) +
        L" fresh_exists_after=" + bool_field(fresh_exists_after) +
        L" same_manager_stale=" + bool_field(same_manager_stale) +
        L" fresh_manager_verified=" + bool_field(fresh_manager_verified) +
        L" verify_attempt=" + std::to_wstring(verify_attempt) +
        L" manager_source=" + (manager_source ? manager_source : L"unknown") +
        L" verification_fatal=" + bool_field(verification_fatal) +
        L" policy_reason=" + (policy_reason ? policy_reason : L"") +
        L" rollback_attempted=" + bool_field(rollback_attempted) +
        L" rollback_hr=" + format_hresult(rollback_hr);
    if (!target_dll.empty()) {
        line += L" target_dll=" + target_dll;
    }
    log_status(L"registration", line);
    if (FAILED(hr) && verification_fatal) {
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
    LSTATUS status = RegOpenKeyExW(root, subkey.c_str(), 0, KEY_READ | kRegistryView, &key);
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
                                     KEY_WRITE | kRegistryView,
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
                                       kRegistryView,
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
    status = RegOpenKeyExW(parent, child_name.c_str(), 0, KEY_READ | kRegistryView, &child);
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

    status = RegOpenKeyExW(parent, child_name.c_str(), 0, KEY_READ | kRegistryView, &child);
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

CategoryMembershipObservation observe_category_to_items(
    ITfCategoryMgr* category_mgr,
    REFGUID category,
    REFGUID expected_item) {
    CategoryMembershipObservation observation{};
    IEnumGUID* items = nullptr;
    observation.hr = category_mgr->EnumItemsInCategory(category, &items);
    if (FAILED(observation.hr) || !items) {
        return observation;
    }
    for (;;) {
        GUID current{};
        ULONG fetched = 0;
        const HRESULT next_hr = items->Next(1, &current, &fetched);
        if (next_hr == S_FALSE || fetched == 0) {
            break;
        }
        if (FAILED(next_hr)) {
            observation.hr = next_hr;
            break;
        }
        ++observation.count;
        if (IsEqualGUID(current, expected_item)) {
            observation.found = true;
        }
    }
    items->Release();
    return observation;
}

CategoryMembershipObservation observe_item_to_categories(
    ITfCategoryMgr* category_mgr,
    REFGUID item,
    REFGUID expected_category) {
    CategoryMembershipObservation observation{};
    IEnumGUID* categories = nullptr;
    observation.hr = category_mgr->EnumCategoriesInItem(item, &categories);
    if (FAILED(observation.hr) || !categories) {
        return observation;
    }
    for (;;) {
        GUID current{};
        ULONG fetched = 0;
        const HRESULT next_hr = categories->Next(1, &current, &fetched);
        if (next_hr == S_FALSE || fetched == 0) {
            break;
        }
        if (FAILED(next_hr)) {
            observation.hr = next_hr;
            break;
        }
        ++observation.count;
        if (IsEqualGUID(current, expected_category)) {
            observation.found = true;
        }
    }
    categories->Release();
    return observation;
}

struct FreshCategoryObservations {
    bool checked = false;
    CategoryMembershipObservation category_to_items;
    CategoryMembershipObservation item_to_categories;
};

FreshCategoryObservations observe_category_with_new_manager(
    REFGUID category,
    REFGUID item) {
    FreshCategoryObservations observations{};
    observations.checked = true;
    ITfCategoryMgr* fresh_category_mgr = nullptr;
    const HRESULT create_hr = CoCreateInstance(CLSID_TF_CategoryMgr,
                                               nullptr,
                                               CLSCTX_INPROC_SERVER,
                                               IID_PPV_ARGS(&fresh_category_mgr));
    if (FAILED(create_hr)) {
        observations.category_to_items.hr = create_hr;
        observations.item_to_categories.hr = create_hr;
        return observations;
    }
    observations.category_to_items =
        observe_category_to_items(fresh_category_mgr, category, item);
    observations.item_to_categories =
        observe_item_to_categories(fresh_category_mgr, item, category);
    fresh_category_mgr->Release();
    return observations;
}

BidirectionalCategoryVerification verify_category_bidirectionally_with_retry(
    ITfCategoryMgr* category_mgr,
    const TsfProfileCategory& category,
    const std::wstring& target_dll,
    bool existed_before,
    HRESULT register_hr) {
    BidirectionalCategoryVerification last{};
    for (unsigned int attempt = 1;
         attempt <= category_visibility_verification_attempts();
         ++attempt) {
        const CategoryMembershipObservation same_category_to_items =
            observe_category_to_items(category_mgr,
                                      *category.category_guid,
                                      *category.item_guid);
        const CategoryMembershipObservation same_item_to_categories =
            observe_item_to_categories(category_mgr,
                                       *category.item_guid,
                                       *category.category_guid);

        FreshCategoryObservations fresh{};
        if (!same_category_to_items.found && !same_item_to_categories.found) {
            fresh = observe_category_with_new_manager(*category.category_guid,
                                                      *category.item_guid);
        }

        last = evaluate_bidirectional_category_verification(
            register_hr,
            existed_before,
            same_category_to_items,
            same_item_to_categories,
            fresh.checked,
            fresh.category_to_items,
            fresh.item_to_categories);
        const bool continue_after_unobserved =
            last.result == CategoryVerificationResult::VerificationInconclusive &&
            category.allow_inconclusive_post_register_verification &&
            attempt == category_visibility_verification_attempts();
        log_bidirectional_category_verification(category,
                                                target_dll,
                                                last,
                                                attempt,
                                                continue_after_unobserved);
        if (last.result != CategoryVerificationResult::VerificationInconclusive ||
            attempt == category_visibility_verification_attempts()) {
            break;
        }
        Sleep(category_visibility_retry_delay_ms());
    }
    return last;
}

bool category_registered_with_new_manager(REFGUID category, REFGUID expected_item, HRESULT& enum_hr) {
    enum_hr = S_OK;
    ITfCategoryMgr* fresh_category_mgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr,
                                  nullptr,
                                  CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&fresh_category_mgr));
    if (FAILED(hr)) {
        enum_hr = hr;
        return false;
    }
    const bool found = category_registered(fresh_category_mgr, category, expected_item, enum_hr);
    fresh_category_mgr->Release();
    return found;
}

CategoryFreshManagerVerification verify_category_after_register_with_retry(
    ITfCategoryMgr* category_mgr,
    const TsfProfileCategory& category,
    const std::wstring& target_dll,
    HRESULT enum_before_hr,
    bool contains_before,
    HRESULT register_hr,
    bool verification_fatal = true) {
    CategoryFreshManagerVerification last_verification{
        HRESULT_FROM_WIN32(ERROR_NOT_FOUND),
        false,
        false,
        true,
        false,
        false,
        false,
        false,
    };

    for (unsigned int attempt = 1; attempt <= category_visibility_verification_attempts(); ++attempt) {
        HRESULT verify_hr = S_OK;
        const bool contains_after =
            category_registered(category_mgr, *category.category_guid, *category.item_guid, verify_hr);
        const CategoryPostRegisterVerification same_manager_verification =
            evaluate_category_after_register(verify_hr,
                                             contains_before,
                                             contains_after,
                                             category.allow_register_category);

        bool fresh_check_performed = false;
        HRESULT fresh_verify_hr = S_OK;
        bool fresh_contains_after = false;
        if (FAILED(same_manager_verification.hr) &&
            same_manager_verification.missing_required_item &&
            category.allow_register_category) {
            fresh_check_performed = true;
            fresh_contains_after = category_registered_with_new_manager(
                *category.category_guid,
                *category.item_guid,
                fresh_verify_hr);
        }

        last_verification =
            evaluate_category_after_register_with_fresh_manager(verify_hr,
                                                                contains_before,
                                                                contains_after,
                                                                category.allow_register_category,
                                                                fresh_verify_hr,
                                                                fresh_contains_after,
                                                                fresh_check_performed);
        log_category_diagnostic(L"VerifyCategoryAfterRegister",
                                category,
                                last_verification.hr,
                                target_dll,
                                enum_before_hr,
                                contains_before,
                                true,
                                register_hr,
                                verify_hr,
                                contains_after,
                                last_verification.added_this_call,
                                false,
                                S_OK,
                                fresh_check_performed,
                                fresh_verify_hr,
                                fresh_contains_after,
                                last_verification.same_manager_stale,
                                last_verification.fresh_manager_verified,
                                attempt,
                                fresh_check_performed ? L"same_manager+fresh_manager" : L"same_manager",
                                verification_fatal,
                                verification_fatal ? L"required_category" : L"compatibility_capability_diagnostic");

        if (SUCCEEDED(last_verification.hr) ||
            last_verification.query_failed ||
            !last_verification.missing_required_item ||
            attempt == category_visibility_verification_attempts()) {
            break;
        }
        Sleep(category_visibility_retry_delay_ms());
    }

    return last_verification;
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
                                   kProfileIconIndex,
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
                                       kProfileIconIndex,
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

bool english_profile_registered(ITfInputProcessorProfileMgr* profiles, HRESULT& enum_hr) {
    enum_hr = S_OK;
    IEnumTfInputProcessorProfiles* enum_profiles = nullptr;
    enum_hr = profiles->EnumProfiles(kLangId, &enum_profiles);
    if (FAILED(enum_hr)) return false;
    bool found = false;
    if (enum_profiles) {
        for (;;) {
            TF_INPUTPROCESSORPROFILE profile{};
            ULONG fetched = 0;
            if (enum_profiles->Next(1, &profile, &fetched) != S_OK || fetched == 0) break;
            if (profile.langid == kLangId && IsEqualGUID(profile.clsid, CLSID_LocalPinyinTextService) &&
                IsEqualGUID(profile.guidProfile, GUID_LocalPinyinEnglishProfile)) {
                found = true;
                break;
            }
        }
        enum_profiles->Release();
    }
    return found;
}

// 本地英文, the second input method of this text service. Registered after
// 本地拼音 and rolled back with it.
HRESULT register_english_profile(const std::wstring& icon_path, bool& added_profile) {
    added_profile = false;
    ITfInputProcessorProfileMgr* profiles = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&profiles));
    if (FAILED(hr)) {
        log_status(L"registration", L"operation=CreateProfileManagerForEnglish hr=" + format_hresult(hr));
        return hr;
    }
    HRESULT enum_hr = S_OK;
    const bool existed = english_profile_registered(profiles, enum_hr);
    hr = profiles->RegisterProfile(CLSID_LocalPinyinTextService,
                                   kLangId,
                                   GUID_LocalPinyinEnglishProfile,
                                   kEnglishDisplayName,
                                   static_cast<ULONG>(wcslen(kEnglishDisplayName)),
                                   icon_path.c_str(),
                                   static_cast<ULONG>(icon_path.size()),
                                   kEnglishProfileIconIndex,
                                   nullptr,
                                   0,
                                   TRUE,
                                   profile_registration_flags());
    if (hr == TF_E_ALREADY_EXISTS) hr = S_OK;
    if (SUCCEEDED(hr)) {
        HRESULT verify_hr = S_OK;
        if (!english_profile_registered(profiles, verify_hr)) hr = FAILED(verify_hr) ? verify_hr : E_FAIL;
        else added_profile = SUCCEEDED(enum_hr) && !existed;
    }
    log_status(L"registration", L"operation=RegisterEnglishProfile hr=" + format_hresult(hr) +
                                    L" profile=" + guid_to_string(GUID_LocalPinyinEnglishProfile) +
                                    L" added_this_call=" + bool_field(added_profile) + L" target_dll=" + icon_path);
    profiles->Release();
    return hr;
}

HRESULT unregister_english_profile() {
    ITfInputProcessorProfileMgr* profiles = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&profiles));
    if (FAILED(hr)) return hr;
    HRESULT enum_hr = S_OK;
    if (!english_profile_registered(profiles, enum_hr)) {
        profiles->Release();
        return FAILED(enum_hr) ? enum_hr : S_OK;
    }
    hr = profiles->UnregisterProfile(CLSID_LocalPinyinTextService, kLangId, GUID_LocalPinyinEnglishProfile, 0);
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

        if (should_track_category_for_cleanup(true, hr, contains_before)) {
            added_categories.push_back(&category);
        }
        const BidirectionalCategoryVerification verification =
            verify_category_bidirectionally_with_retry(category_mgr,
                                                       category,
                                                       target_dll,
                                                       contains_before,
                                                       hr);
        if (!category_verification_allows_continue(
                verification.result,
                category.allow_inconclusive_post_register_verification)) {
            const HRESULT verification_hr =
                verification.result == CategoryVerificationResult::RegisterCallFailed
                    ? verification.register_hr
                    : HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            log_category_diagnostic(L"VerifyCategoryAfterRegisterFatal",
                                    category,
                                    verification_hr,
                                    target_dll,
                                    enum_hr,
                                    contains_before,
                                    true,
                                    hr,
                                    verification.same_category_to_items.hr,
                                    verification.category_postcondition_observed,
                                    verification.added_this_call,
                                    false,
                                    S_OK,
                                    verification.fresh_manager_checked,
                                    verification.fresh_category_to_items.hr,
                                    verification.fresh_category_to_items.found,
                                    false,
                                    verification.category_postcondition_observed,
                                    category_visibility_verification_attempts(),
                                    verification.fresh_manager_checked
                                        ? L"same_manager+fresh_manager"
                                        : L"same_manager",
                                    true,
                                    L"required_category_observation");
            category_mgr->Release();
            return verification_hr;
        }
    }

    for (const auto& category : compatibility_tsf_profile_categories()) {
        HRESULT enum_hr = S_OK;
        const bool contains_before =
            category_registered(category_mgr, *category.category_guid, *category.item_guid, enum_hr);
        const CategoryRegistrationDecision decision =
            decide_category_registration(enum_hr, contains_before, category.allow_register_category);
        log_category_diagnostic(L"EnumCompatibilityCategoryBeforeRegister",
                                category,
                                apply_category_verification_policy(enum_hr, false),
                                target_dll,
                                enum_hr,
                                contains_before,
                                false,
                                S_OK,
                                S_OK,
                                contains_before,
                                false,
                                false,
                                S_OK,
                                false,
                                S_OK,
                                false,
                                false,
                                false,
                                0,
                                L"same_manager",
                                false,
                                L"compatibility_capability_diagnostic");
        if (decision.query_failed) {
            continue;
        }
        if (decision.skip_existing || !decision.should_register) {
            log_category_diagnostic(L"CompatibilityCategorySkipExisting",
                                    category,
                                    S_OK,
                                    target_dll,
                                    enum_hr,
                                    contains_before,
                                    false,
                                    S_OK,
                                    enum_hr,
                                    contains_before,
                                    false,
                                    false,
                                    S_OK,
                                    false,
                                    S_OK,
                                    false,
                                    false,
                                    false,
                                    0,
                                    L"same_manager",
                                    false,
                                    L"compatibility_capability_diagnostic");
            continue;
        }

        hr = category_mgr->RegisterCategory(CLSID_LocalPinyinTextService,
                                            *category.category_guid,
                                            *category.item_guid);
        if (hr == TF_E_ALREADY_EXISTS) {
            hr = S_OK;
        }
        log_step_result(std::wstring(L"register diagnostic ") + category.name, hr);
        log_category_diagnostic(L"RegisterCompatibilityCategory",
                                category,
                                apply_category_verification_policy(hr, false),
                                target_dll,
                                enum_hr,
                                contains_before,
                                true,
                                hr,
                                S_OK,
                                contains_before,
                                false,
                                false,
                                S_OK,
                                false,
                                S_OK,
                                false,
                                false,
                                false,
                                0,
                                L"same_manager",
                                false,
                                L"compatibility_capability_diagnostic");
        if (FAILED(hr)) {
            continue;
        }

        const CategoryFreshManagerVerification verification =
            verify_category_after_register_with_retry(category_mgr,
                                                      category,
                                                      target_dll,
                                                      enum_hr,
                                                      contains_before,
                                                      hr,
                                                      false);
        if (verification.verified && verification.added_this_call) {
            added_categories.push_back(&category);
        }
        if (FAILED(verification.hr)) {
            log_category_diagnostic(L"CompatibilityCategoryVerificationNonFatal",
                                    category,
                                    apply_category_verification_policy(verification.hr, false),
                                    target_dll,
                                    enum_hr,
                                    contains_before,
                                    true,
                                    hr,
                                    verification.hr,
                                    verification.verified,
                                    verification.added_this_call,
                                    false,
                                    S_OK,
                                    verification.fresh_manager_checked,
                                    verification.hr,
                                    verification.fresh_manager_verified,
                                    verification.same_manager_stale,
                                    verification.fresh_manager_verified,
                                    category_visibility_verification_attempts(),
                                    verification.fresh_manager_checked ? L"same_manager+fresh_manager" : L"same_manager",
                                    false,
                                    L"compatibility_capability_diagnostic");
        }
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
        const bool observed_before =
            category_registered(category_mgr,
                                *category.category_guid,
                                *category.item_guid,
                                enum_hr);
        log_step_result(std::wstring(L"enumerate before unregister ") +
                            category.name +
                            (observed_before ? L" present" : L" unobserved"),
                        enum_hr);
        merge_failure(enum_hr, first_failure);

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
    bool added_english_profile = false;
    std::vector<const TsfProfileCategory*> added_categories;
    HRESULT final_profile_register_hr = E_UNEXPECTED;
    bool final_profile_registered = false;
    bool final_profile_visible = false;

    hr = query_current_profile_snapshot(profile_before);
    if (SUCCEEDED(hr)) {
        hr = register_tsf_profile(target_dll, profile_before, added_profile);
        final_profile_register_hr = hr;
        final_profile_registered = SUCCEEDED(hr);
        final_profile_visible = SUCCEEDED(hr);
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
    } else {
        log_failure_line(L"SnapshotProfileBeforeRegister", hr, target_dll);
    }
    if (SUCCEEDED(hr)) {
        hr = register_tsf_categories(target_dll, added_categories);
    }
    if (SUCCEEDED(hr)) {
        hr = register_com_class(target_dll);
        if (FAILED(hr)) {
            log_failure_line(L"RegisterComInprocServer32", hr, target_dll);
        }
    }
    if (SUCCEEDED(hr)) {
        hr = register_english_profile(target_dll, added_english_profile);
        if (FAILED(hr)) log_failure_line(L"RegisterEnglishProfile", hr, target_dll);
    }

    if (FAILED(hr)) {
        const HRESULT first_failure = hr;
        const HRESULT english_rollback_hr = added_english_profile ? unregister_english_profile() : S_OK;
        log_step_result(L"rollback English profile", english_rollback_hr);
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
            L" rollback_com=" + format_hresult(com_rollback_hr) +
            L" rollback_reason=first_failure_" + format_hresult(first_failure) +
            L" final_profile_register_hr=" + format_hresult(final_profile_register_hr) +
            L" final_profile_registered=" + bool_field(final_profile_registered) +
            L" final_profile_visible=" + bool_field(final_profile_visible) +
            L" final_expected_dll_verified=NOT_CHECKED");
        if (!g_registration_warning.empty()) {
            append_last_registration_diagnostic(
                L" secondary_category_warning={" + g_registration_warning + L"}");
        }
        log_status(L"registration",
                   L"rollback completed categories=" + format_hresult(category_rollback_hr) +
                       L" profile=" + format_hresult(profile_rollback_hr) +
                       L" com=" + format_hresult(com_rollback_hr));
    } else {
        const std::wstring final_success =
            L"final_profile_register_hr=" + format_hresult(final_profile_register_hr) +
            L" final_profile_registered=" + bool_field(final_profile_registered) +
            L" final_profile_visible=" + bool_field(final_profile_visible) +
            L" final_expected_dll_verified=NOT_CHECKED"
            L" rollback_attempted=FALSE rollback_reason=none";
        log_status(L"registration", final_success);
        if (!g_registration_warning.empty()) {
            g_last_registration_diagnostic =
                g_registration_warning + L" " + final_success;
        }
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

    const HRESULT english_hr = unregister_english_profile();
    log_step_result(L"unregister English TSF profile", english_hr);
    merge_failure(english_hr, first_failure);

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
