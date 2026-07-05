#include <windows.h>
#include <msctf.h>

#include "../ime/guids.h"
#include "../ime/registration.h"
#include "../ime/tsf_profile_categories.h"

#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

constexpr LANGID kLangId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);

std::wstring bool_text(bool value) {
    return value ? L"TRUE" : L"FALSE";
}

std::wstring hresult_hex(HRESULT hr) {
    wchar_t buffer[16]{};
    swprintf_s(buffer, L"0x%08lX", static_cast<unsigned long>(hr));
    return buffer;
}

std::wstring dword_hex(DWORD value) {
    wchar_t buffer[16]{};
    swprintf_s(buffer, L"0x%08lX", static_cast<unsigned long>(value));
    return buffer;
}

std::wstring langid_hex(LANGID langid) {
    wchar_t buffer[16]{};
    swprintf_s(buffer, L"0x%04X", static_cast<unsigned int>(langid));
    return buffer;
}

std::wstring guid_to_string(REFGUID guid) {
    wchar_t buffer[64]{};
    StringFromGUID2(guid, buffer, static_cast<int>(_countof(buffer)));
    return buffer;
}

std::wstring hkl_hex(HKL hkl) {
    wchar_t buffer[32]{};
    swprintf_s(buffer,
               L"0x%0*IX",
               static_cast<int>(sizeof(std::uintptr_t) * 2),
               reinterpret_cast<std::uintptr_t>(hkl));
    return buffer;
}

void print_usage() {
    std::wcout << L"Usage:\n"
               << L"  LocalPinyinImeAudit.exe --read-only\n";
}

bool is_local_profile(const TF_INPUTPROCESSORPROFILE& profile) {
    return IsEqualGUID(profile.clsid, localpinyin::CLSID_LocalPinyinTextService) &&
           IsEqualGUID(profile.guidProfile, localpinyin::GUID_LocalPinyinProfile) &&
           profile.langid == kLangId;
}

void print_profile(const wchar_t* prefix, const TF_INPUTPROCESSORPROFILE& profile) {
    std::wcout << prefix << L".dwProfileType: " << dword_hex(profile.dwProfileType) << L"\n"
               << prefix << L".langid: " << langid_hex(profile.langid) << L"\n"
               << prefix << L".clsid: " << guid_to_string(profile.clsid) << L"\n"
               << prefix << L".guidProfile: " << guid_to_string(profile.guidProfile) << L"\n"
               << prefix << L".catid: " << guid_to_string(profile.catid) << L"\n"
               << prefix << L".dwFlags: " << dword_hex(profile.dwFlags) << L"\n"
               << prefix << L".hkl: " << hkl_hex(profile.hkl) << L"\n"
               << prefix << L".hklSubstitute: " << hkl_hex(profile.hklSubstitute) << L"\n"
               << prefix << L".dwCaps: " << dword_hex(profile.dwCaps) << L"\n";
}

void print_static_registration_implementation() {
    std::wcout << L"Current registration implementation:\n"
               << L"- uses legacy Register: FALSE\n"
               << L"- uses legacy AddLanguageProfile: FALSE\n"
               << L"- uses RegisterProfile: TRUE\n"
               << L"- treats profile caps as diagnostic-only verification data: TRUE\n"
               << L"- uses RegisterCategory for explicit category items: TRUE\n";
}

HRESULT audit_profiles() {
    ITfInputProcessorProfileMgr* profile_mgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles,
                                  nullptr,
                                  CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&profile_mgr));
    std::wcout << L"CoCreateInstance ITfInputProcessorProfileMgr HRESULT: " << hresult_hex(hr) << L"\n";
    if (FAILED(hr)) {
        return hr;
    }

    TF_INPUTPROCESSORPROFILE profile{};
    hr = profile_mgr->GetProfile(TF_PROFILETYPE_INPUTPROCESSOR,
                                 kLangId,
                                 localpinyin::CLSID_LocalPinyinTextService,
                                 localpinyin::GUID_LocalPinyinProfile,
                                 nullptr,
                                 &profile);
    std::wcout << L"GetProfile HRESULT: " << hresult_hex(hr) << L"\n";
    if (SUCCEEDED(hr)) {
        print_profile(L"GetProfile", profile);
    }

    IEnumTfInputProcessorProfiles* enum_profiles = nullptr;
    const HRESULT enum_hr = profile_mgr->EnumProfiles(kLangId, &enum_profiles);
    std::wcout << L"EnumProfiles(0x0804) HRESULT: " << hresult_hex(enum_hr) << L"\n";

    bool found = false;
    HRESULT next_hr = enum_hr;
    if (SUCCEEDED(enum_hr) && enum_profiles) {
        for (;;) {
            TF_INPUTPROCESSORPROFILE item{};
            ULONG fetched = 0;
            next_hr = enum_profiles->Next(1, &item, &fetched);
            if (next_hr != S_OK || fetched == 0) {
                break;
            }
            if (is_local_profile(item)) {
                found = true;
                print_profile(L"EnumProfiles.LocalPinyinIME", item);
            }
        }
        enum_profiles->Release();
    }

    std::wcout << L"EnumProfiles.Next final HRESULT: " << hresult_hex(next_hr) << L"\n"
               << L"EnumProfiles contains LocalPinyinIME: " << bool_text(found) << L"\n";

    profile_mgr->Release();
    return FAILED(hr) ? hr : enum_hr;
}

HRESULT audit_category(const localpinyin::TsfProfileCategory& check) {
    ITfCategoryMgr* category_mgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&category_mgr));
    if (FAILED(hr)) {
        std::wcout << L"Category: " << check.name << L"\n"
                   << L"HRESULT: " << hresult_hex(hr) << L"\n"
                   << L"Contains LocalPinyinIME CLSID: FALSE\n";
        return hr;
    }

    IEnumGUID* enum_items = nullptr;
    hr = category_mgr->EnumItemsInCategory(*check.category_guid, &enum_items);
    bool contains = false;
    HRESULT next_hr = hr;
    if (SUCCEEDED(hr) && enum_items) {
        for (;;) {
            GUID item{};
            ULONG fetched = 0;
            next_hr = enum_items->Next(1, &item, &fetched);
            if (next_hr != S_OK || fetched == 0) {
                break;
            }
            if (IsEqualGUID(item, *check.item_guid)) {
                contains = true;
            }
        }
        enum_items->Release();
    }

    std::wcout << L"Category: " << check.name << L"\n"
               << L"Category GUID: " << guid_to_string(*check.category_guid) << L"\n"
               << L"Item GUID: " << guid_to_string(*check.item_guid) << L"\n"
               << L"HRESULT: " << hresult_hex(hr) << L"\n"
               << L"EnumItemsInCategory.Next final HRESULT: " << hresult_hex(next_hr) << L"\n"
               << L"Contains LocalPinyinIME CLSID: " << bool_text(contains) << L"\n";

    category_mgr->Release();
    return hr;
}

HRESULT audit_categories() {
    HRESULT first_failure = S_OK;
    for (const auto& category : localpinyin::required_tsf_profile_categories()) {
        const HRESULT hr = audit_category(category);
        if (SUCCEEDED(first_failure) && FAILED(hr)) {
            first_failure = hr;
        }
    }
    return first_failure;
}

void print_required_capabilities() {
    std::wcout << L"Required TSF category/capability table:\n";
    for (const auto& capability : localpinyin::required_tsf_profile_capabilities()) {
        const auto caps = localpinyin::observe_profile_caps(capability.profile_caps,
                                                            localpinyin::required_tsf_profile_caps());
        std::wcout << L"- name: " << capability.name << L"\n"
                   << L"  category_guid: " << guid_to_string(*capability.category_guid) << L"\n"
                   << L"  item_guid: " << guid_to_string(*capability.item_guid) << L"\n"
                   << L"  allow_register_category: " << bool_text(capability.allow_register_category) << L"\n"
                   << L"  profile_caps: " << dword_hex(capability.profile_caps) << L"\n"
                   << L"  expected_internal_caps_overlap: " << dword_hex(caps.observed_caps) << L"\n";
    }
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc != 2 || std::wstring(argv[1]) != L"--read-only") {
        print_usage();
        return 2;
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

    std::wcout << L"LocalPinyinIME TSF audit: read-only\n";
    print_static_registration_implementation();
    print_required_capabilities();

    const HRESULT profile_hr = audit_profiles();
    const HRESULT category_hr = audit_categories();

    if (co_initialized) {
        CoUninitialize();
    }

    return FAILED(profile_hr) || FAILED(category_hr) ? 1 : 0;
}
