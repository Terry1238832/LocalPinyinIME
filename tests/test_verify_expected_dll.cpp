#include "setup/verify_expected_dll.h"

#include <windows.h>

#include <iostream>
#include <string>

namespace {

int failures = 0;

void expect_true(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}

void expect_hr(HRESULT actual, HRESULT expected, const char* message) {
    if (actual != expected) {
        std::cerr << "FAIL: " << message << " actual=0x" << std::hex
                  << static_cast<unsigned long>(actual) << " expected=0x"
                  << static_cast<unsigned long>(expected) << std::dec << "\n";
        ++failures;
    }
}

HRESULT failing_diagnostic_sink(const std::wstring&, void*) {
    return E_FAIL;
}

localpinyin::setup::InprocServerReadResult read_ok(const std::wstring& value) {
    return {S_OK, value};
}

void test_exact_match_returns_s_ok() {
    const auto result = localpinyin::setup::verify_expected_dll_value(
        L"C:\\Program Files\\LocalPinyinIME\\releases\\0.3.9-dev\\x64\\LocalPinyinIME.dll",
        read_ok(L"C:\\Program Files\\LocalPinyinIME\\releases\\0.3.9-dev\\x64\\LocalPinyinIME.dll"));
    expect_hr(result.hr, S_OK, "exact expected/actual DLL path should verify");
    expect_true(result.matches, "exact expected/actual DLL path should set matches");
}

void test_case_insensitive_match_returns_s_ok() {
    const auto result = localpinyin::setup::verify_expected_dll_value(
        L"C:\\Program Files\\LocalPinyinIME\\releases\\0.3.9-dev\\x64\\LocalPinyinIME.dll",
        read_ok(L"c:\\program files\\localpinyinime\\RELEASES\\0.3.9-DEV\\x64\\localpinyinime.DLL"));
    expect_hr(result.hr, S_OK, "Windows DLL path comparison should ignore case");
    expect_true(result.matches, "case-insensitive match should set matches");
}

void test_normalized_match_returns_s_ok() {
    const auto result = localpinyin::setup::verify_expected_dll_value(
        L"C:\\Program Files\\LocalPinyinIME\\releases\\0.3.9-dev\\x64\\.\\LocalPinyinIME.dll",
        read_ok(L"C:\\Program Files\\LocalPinyinIME\\releases\\0.3.9-dev\\x64\\LocalPinyinIME.dll\\"));
    expect_hr(result.hr, S_OK, "normalized equivalent DLL path should verify");
    expect_true(result.matches, "normalized equivalent path should set matches");
}

void test_different_dll_fails() {
    const auto result = localpinyin::setup::verify_expected_dll_value(
        L"C:\\Program Files\\LocalPinyinIME\\releases\\0.3.9-dev\\x64\\LocalPinyinIME.dll",
        read_ok(L"C:\\Program Files\\LocalPinyinIME\\releases\\0.3.15-dev\\x64\\LocalPinyinIME.dll"));
    expect_hr(result.hr, HRESULT_FROM_WIN32(ERROR_INVALID_DATA), "different DLL path should fail explicitly");
    expect_true(!result.matches, "different DLL path should not set matches");
}

void test_registry_query_failure_is_preserved() {
    const HRESULT query_hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    const auto result = localpinyin::setup::verify_expected_dll_value(
        L"C:\\Program Files\\LocalPinyinIME\\releases\\0.3.9-dev\\x64\\LocalPinyinIME.dll",
        {query_hr, L""});
    expect_hr(result.hr, query_hr, "registry query HRESULT should be preserved");
    expect_true(!result.matches, "registry query failure should not set matches");
}

void test_optional_diagnostic_failure_does_not_pollute_success() {
    const auto result = localpinyin::setup::verify_expected_dll_value(
        L"C:\\Program Files\\LocalPinyinIME\\releases\\0.3.9-dev\\x64\\LocalPinyinIME.dll",
        read_ok(L"C:\\Program Files\\LocalPinyinIME\\releases\\0.3.9-dev\\x64\\LocalPinyinIME.dll"),
        failing_diagnostic_sink,
        nullptr);
    expect_hr(result.hr, S_OK, "optional diagnostic failure must not pollute successful core verification");
    expect_true(result.matches, "diagnostic failure must not clear matches");
}

}  // namespace

int main() {
    test_exact_match_returns_s_ok();
    test_case_insensitive_match_returns_s_ok();
    test_normalized_match_returns_s_ok();
    test_different_dll_fails();
    test_registry_query_failure_is_preserved();
    test_optional_diagnostic_failure_does_not_pollute_success();
    return failures == 0 ? 0 : 1;
}
