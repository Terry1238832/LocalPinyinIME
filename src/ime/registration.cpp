#include "registration.h"

#include "globals.h"
#include "guids.h"
#include "../common/logging.h"
#include "../common/win32_utils.h"

#include <msctf.h>

#include <string>

namespace localpinyin {
namespace {

constexpr wchar_t kDisplayName[] = L"LocalPinyinIME - \u79BB\u7EBF\u62FC\u97F3\u8F93\u5165\u6CD5";
constexpr LANGID kLangId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);

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

std::wstring format_step(const std::wstring& step, HRESULT hr) {
    return step + (FAILED(hr) ? L" failed " : L" succeeded ") + hresult_hex(hr);
}

void log_step_result(const std::wstring& step, HRESULT hr) {
    log_status(L"registration", format_step(step, hr));
}

void merge_failure(HRESULT hr, HRESULT& first_failure) {
    if (FAILED(hr) && SUCCEEDED(first_failure)) {
        first_failure = hr;
    }
}

bool is_missing_status(LSTATUS status) {
    return status == ERROR_FILE_NOT_FOUND || status == ERROR_PATH_NOT_FOUND || status == ERROR_NOT_FOUND;
}

HRESULT set_reg_string(HKEY root, const std::wstring& subkey, const wchar_t* name, const std::wstring& value) {
    HKEY key = nullptr;
    LSTATUS status = RegCreateKeyExW(root, subkey.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr);
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

HRESULT register_com_class() {
    const std::wstring clsid = guid_to_string(CLSID_LocalPinyinTextService);
    const std::wstring base = L"CLSID\\" + clsid;
    const std::wstring inproc = base + L"\\InprocServer32";
    const std::wstring dll_path = module_path(g_instance);

    HRESULT hr = set_reg_string(HKEY_CLASSES_ROOT, base, nullptr, kDisplayName);
    if (FAILED(hr)) {
        return hr;
    }
    hr = set_reg_string(HKEY_CLASSES_ROOT, inproc, nullptr, dll_path);
    if (FAILED(hr)) {
        return hr;
    }
    return set_reg_string(HKEY_CLASSES_ROOT, inproc, L"ThreadingModel", L"Apartment");
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

bool category_registered(ITfCategoryMgr* category_mgr, HRESULT& enum_hr) {
    enum_hr = S_OK;
    IEnumGUID* enum_items = nullptr;
    enum_hr = category_mgr->EnumItemsInCategory(GUID_TFCAT_TIP_KEYBOARD, &enum_items);
    if (FAILED(enum_hr)) {
        return false;
    }

    bool found = false;
    if (enum_items) {
        for (;;) {
            GUID item{};
            ULONG fetched = 0;
            const HRESULT next_hr = enum_items->Next(1, &item, &fetched);
            if (next_hr != S_OK || fetched == 0) {
                break;
            }
            if (IsEqualGUID(item, CLSID_LocalPinyinTextService)) {
                found = true;
                break;
            }
        }
        enum_items->Release();
    }
    return found;
}

HRESULT register_tsf_profile() {
    ITfInputProcessorProfileMgr* profiles = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&profiles));
    if (FAILED(hr)) {
        return hr;
    }

    const std::wstring icon_path = module_path(g_instance);
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
                                   0);
    profiles->Release();
    return hr == TF_E_ALREADY_EXISTS ? S_OK : hr;
}

HRESULT register_tsf_categories() {
    ITfCategoryMgr* category_mgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&category_mgr));
    if (FAILED(hr)) {
        return hr;
    }

    hr = category_mgr->RegisterCategory(CLSID_LocalPinyinTextService,
                                        GUID_TFCAT_TIP_KEYBOARD,
                                        CLSID_LocalPinyinTextService);
    category_mgr->Release();
    return hr == TF_E_ALREADY_EXISTS ? S_OK : hr;
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

    HRESULT enum_hr = S_OK;
    if (!category_registered(category_mgr, enum_hr)) {
        category_mgr->Release();
        return FAILED(enum_hr) ? enum_hr : S_OK;
    }

    hr = category_mgr->UnregisterCategory(CLSID_LocalPinyinTextService,
                                          GUID_TFCAT_TIP_KEYBOARD,
                                          CLSID_LocalPinyinTextService);
    category_mgr->Release();
    return hr;
}

}  // namespace

HRESULT register_server() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool co_initialized = SUCCEEDED(hr);
    if (hr == RPC_E_CHANGED_MODE) {
        co_initialized = false;
        hr = S_OK;
    }
    if (FAILED(hr)) {
        return hr;
    }

    hr = register_com_class();
    if (SUCCEEDED(hr)) {
        hr = register_tsf_profile();
    }
    if (SUCCEEDED(hr)) {
        hr = register_tsf_categories();
    }

    log_status(L"registration", FAILED(hr) ? L"register failed " + hresult_hex(hr) : L"register succeeded");
    if (co_initialized) {
        CoUninitialize();
    }
    return hr;
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
    log_step_result(L"unregister GUID_TFCAT_TIP_KEYBOARD", category_hr);
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

    log_status(L"registration", FAILED(hr) ? L"unregister failed " + hresult_hex(hr) : L"unregister succeeded");
    if (co_initialized) {
        CoUninitialize();
    }
    return hr;
}

}  // namespace localpinyin
