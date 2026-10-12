#pragma once

#include <windows.h>

#include <array>
#include <cstddef>
#include <span>

namespace localpinyin::setup {

struct ProfileVisibilityAttempt {
    HRESULT manager_hr = E_UNEXPECTED;
    HRESULT get_hr = E_UNEXPECTED;
    bool get_matches_clsid = false;
    bool get_matches_guid = false;
    bool get_matches_langid = false;
    HRESULT enum_hr = E_UNEXPECTED;
    unsigned int enum_count = 0;
    bool enum_found = false;
};

[[nodiscard]] constexpr bool profile_get_matches(
    const ProfileVisibilityAttempt& attempt) noexcept {
    return SUCCEEDED(attempt.get_hr) && attempt.get_matches_clsid &&
           attempt.get_matches_guid && attempt.get_matches_langid;
}

[[nodiscard]] constexpr bool profile_visibility_attempt_succeeded(
    const ProfileVisibilityAttempt& attempt) noexcept {
    return SUCCEEDED(attempt.manager_hr) && profile_get_matches(attempt) &&
           SUCCEEDED(attempt.enum_hr) && attempt.enum_found;
}

struct ProfileVisibilityVerification {
    bool visible = false;
    size_t attempts_used = 0;
    size_t successful_attempt = 0;
    HRESULT failure_hr = E_FAIL;
    ProfileVisibilityAttempt final_attempt{};
};

[[nodiscard]] constexpr HRESULT profile_visibility_attempt_failure(
    const ProfileVisibilityAttempt& attempt) noexcept {
    if (FAILED(attempt.manager_hr)) {
        return attempt.manager_hr;
    }
    if (FAILED(attempt.get_hr)) {
        return attempt.get_hr;
    }
    if (!attempt.get_matches_clsid || !attempt.get_matches_guid ||
        !attempt.get_matches_langid) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }
    if (FAILED(attempt.enum_hr)) {
        return attempt.enum_hr;
    }
    if (!attempt.enum_found) {
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }
    return S_OK;
}

[[nodiscard]] inline ProfileVisibilityVerification evaluate_profile_visibility_attempts(
    std::span<const ProfileVisibilityAttempt> attempts) noexcept {
    ProfileVisibilityVerification result{};
    for (size_t index = 0; index < attempts.size(); ++index) {
        result.attempts_used = index + 1;
        result.final_attempt = attempts[index];
        if (profile_visibility_attempt_succeeded(attempts[index])) {
            result.visible = true;
            result.successful_attempt = index + 1;
            result.failure_hr = S_OK;
            return result;
        }
        result.failure_hr = profile_visibility_attempt_failure(attempts[index]);
    }
    return result;
}

inline constexpr std::array<DWORD, 6> kProfileVisibilityRetryDelaysMs{
    0, 50, 150, 300, 500, 1000,
};

[[nodiscard]] constexpr DWORD profile_visibility_max_total_wait_ms() noexcept {
    DWORD total = 0;
    for (const DWORD delay : kProfileVisibilityRetryDelaysMs) {
        total += delay;
    }
    return total;
}

struct LoadLibraryDiagnosticDecision {
    bool success = false;
    bool include_win32_error = false;
    DWORD win32_error = ERROR_SUCCESS;
};

[[nodiscard]] constexpr LoadLibraryDiagnosticDecision evaluate_load_library_diagnostic(
    bool loaded,
    DWORD failure_error) noexcept {
    return loaded
        ? LoadLibraryDiagnosticDecision{true, false, ERROR_SUCCESS}
        : LoadLibraryDiagnosticDecision{false, true, failure_error};
}

}  // namespace localpinyin::setup
