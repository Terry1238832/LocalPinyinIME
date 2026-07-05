#include "verify_expected_dll.h"

#include <cwctype>

namespace localpinyin::setup {
namespace {

bool is_path_separator(wchar_t ch) {
    return ch == L'\\' || ch == L'/';
}

size_t root_prefix_length(const std::wstring& path) {
    if (path.size() >= 3 && std::iswalpha(path[0]) && path[1] == L':' && is_path_separator(path[2])) {
        return 3;
    }
    if (path.rfind(L"\\\\?\\", 0) == 0 && path.size() >= 7 &&
        std::iswalpha(path[4]) && path[5] == L':' && is_path_separator(path[6])) {
        return 7;
    }
    if (path.rfind(L"\\\\", 0) == 0) {
        size_t first = path.find(L'\\', 2);
        if (first == std::wstring::npos) {
            return path.size();
        }
        size_t second = path.find(L'\\', first + 1);
        if (second == std::wstring::npos) {
            return path.size();
        }
        return second + 1;
    }
    return 0;
}

void trim_trailing_path_separators(std::wstring& path) {
    const size_t min_length = root_prefix_length(path);
    while (path.size() > min_length && is_path_separator(path.back())) {
        path.pop_back();
    }
}

bool same_windows_path_text(const std::wstring& left, const std::wstring& right) {
    return CompareStringOrdinal(left.c_str(), -1, right.c_str(), -1, TRUE) == CSTR_EQUAL;
}

}  // namespace

std::wstring normalize_dll_path_for_comparison(const std::wstring& path) {
    if (path.empty()) {
        return {};
    }
    const DWORD needed = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
    if (needed == 0) {
        std::wstring fallback = path;
        trim_trailing_path_separators(fallback);
        return fallback;
    }
    std::wstring result(needed, L'\0');
    const DWORD written = GetFullPathNameW(path.c_str(), needed, result.data(), nullptr);
    if (written == 0) {
        result = path;
    } else {
        result.resize(written);
    }
    trim_trailing_path_separators(result);
    return result;
}

ExpectedDllVerification verify_expected_dll_value(
    const std::wstring& expected_dll_path,
    const InprocServerReadResult& actual_inproc,
    ExpectedDllDiagnosticSink diagnostic_sink,
    void* diagnostic_context) {
    ExpectedDllVerification result{};
    result.expected_normalized = normalize_dll_path_for_comparison(expected_dll_path);
    result.actual_normalized = normalize_dll_path_for_comparison(actual_inproc.value);

    if (FAILED(actual_inproc.hr)) {
        result.hr = actual_inproc.hr;
    } else if (result.expected_normalized.empty() || result.actual_normalized.empty()) {
        result.hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    } else {
        result.matches = same_windows_path_text(result.expected_normalized, result.actual_normalized);
        result.hr = result.matches ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    if (diagnostic_sink) {
        const std::wstring line = L"operation=VerifyExpectedDll expected=" +
                                  (result.expected_normalized.empty() ? L"<missing>" : result.expected_normalized) +
                                  L" actual=" +
                                  (result.actual_normalized.empty() ? L"<missing>" : result.actual_normalized) +
                                  L" matches=" + (result.matches ? L"TRUE" : L"FALSE");
        (void)diagnostic_sink(line, diagnostic_context);
    }
    return result;
}

}  // namespace localpinyin::setup
