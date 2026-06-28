#include <windows.h>
#include <msctf.h>

#include "../ime/guids.h"

#include <cwchar>
#include <iostream>
#include <string>

namespace {

constexpr wchar_t kProfileSpec[] =
    L"0x0804:{7C0B4B75-80B0-4E1F-A4A5-4D49A5440D8A}"
    L"{84D58E7C-481E-4D20-A951-4ED39F01D8D5};";
constexpr DWORD kInstallFlags = 0;

using InstallLayoutOrTipFn = BOOL(WINAPI*)(LPCWSTR psz, DWORD dwFlags);

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

void print_usage() {
    std::wcout << L"Usage:\n"
               << L"  LocalPinyinImeEnable.exe --current-user\n\n"
               << L"This tool adds the registered LocalPinyinIME TSF profile to the current user's enabled input list.\n"
               << L"It does not activate the profile and does not set it as the default input method.\n";
}

HRESULT query_enabled(bool& enabled) {
    enabled = false;

    ITfInputProcessorProfiles* profiles = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&profiles));
    if (FAILED(hr)) {
        return hr;
    }

    BOOL is_enabled = FALSE;
    hr = profiles->IsEnabledLanguageProfile(localpinyin::CLSID_LocalPinyinTextService,
                                            MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
                                            localpinyin::GUID_LocalPinyinProfile,
                                            &is_enabled);
    profiles->Release();

    if (SUCCEEDED(hr)) {
        enabled = is_enabled == TRUE;
    }
    return hr;
}

HRESULT load_install_layout_or_tip(HMODULE& input_module, InstallLayoutOrTipFn& install_fn) {
    input_module = nullptr;
    install_fn = nullptr;

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

}  // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc != 2 || std::wstring(argv[1]) != L"--current-user") {
        print_usage();
        return 2;
    }

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool co_initialized = SUCCEEDED(hr);
    if (hr == RPC_E_CHANGED_MODE) {
        hr = S_OK;
    }
    if (FAILED(hr)) {
        std::wcerr << L"CoInitializeEx failed: " << hresult_hex(hr) << L"\n";
        return 1;
    }

    std::wcout << L"ProfileSpec: " << kProfileSpec << L"\n";

    bool enabled_before = false;
    hr = query_enabled(enabled_before);
    std::wcout << L"before.IsEnabledLanguageProfile HRESULT: " << hresult_hex(hr) << L"\n"
               << L"enabled before: " << (enabled_before ? L"TRUE" : L"FALSE") << L"\n";
    if (FAILED(hr)) {
        if (co_initialized) {
            CoUninitialize();
        }
        return 1;
    }

    HMODULE input = nullptr;
    InstallLayoutOrTipFn install = nullptr;
    hr = load_install_layout_or_tip(input, install);
    if (FAILED(hr)) {
        std::wcerr << L"Load InstallLayoutOrTip failed: " << hresult_hex(hr) << L"\n";
        if (co_initialized) {
            CoUninitialize();
        }
        return 1;
    }

    SetLastError(ERROR_SUCCESS);
    const BOOL install_ok = install(kProfileSpec, kInstallFlags);
    const DWORD install_last_error = install_ok ? ERROR_SUCCESS : GetLastError();
    FreeLibrary(input);

    std::wcout << L"InstallLayoutOrTip returned: " << (install_ok ? L"TRUE" : L"FALSE") << L"\n"
               << L"GetLastError: " << install_last_error << L" / " << win32_hex(install_last_error) << L"\n";
    if (!install_ok && install_last_error == ERROR_SUCCESS) {
        std::wcout << L"InstallLayoutOrTip failure detail: FALSE with ERROR_SUCCESS / unspecified failure\n";
    }
    if (!install_ok) {
        if (co_initialized) {
            CoUninitialize();
        }
        return 1;
    }

    bool enabled_after = false;
    hr = query_enabled(enabled_after);
    std::wcout << L"after.IsEnabledLanguageProfile HRESULT: " << hresult_hex(hr) << L"\n"
               << L"enabled after: " << (enabled_after ? L"TRUE" : L"FALSE") << L"\n";

    if (co_initialized) {
        CoUninitialize();
    }

    return SUCCEEDED(hr) && enabled_after ? 0 : 1;
}
