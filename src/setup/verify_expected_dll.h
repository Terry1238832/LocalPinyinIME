#pragma once

#include <windows.h>

#include <string>

namespace localpinyin::setup {

struct InprocServerReadResult {
    HRESULT hr = E_FAIL;
    std::wstring value;
};

struct ExpectedDllVerification {
    HRESULT hr = E_FAIL;
    bool matches = false;
    std::wstring expected_normalized;
    std::wstring actual_normalized;
};

using ExpectedDllDiagnosticSink = HRESULT (*)(const std::wstring& line, void* context);

std::wstring normalize_dll_path_for_comparison(const std::wstring& path);

ExpectedDllVerification verify_expected_dll_value(
    const std::wstring& expected_dll_path,
    const InprocServerReadResult& actual_inproc,
    ExpectedDllDiagnosticSink diagnostic_sink = nullptr,
    void* diagnostic_context = nullptr);

}  // namespace localpinyin::setup
