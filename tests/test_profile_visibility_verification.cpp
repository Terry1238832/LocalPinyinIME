#include "test_common.h"

#include "../src/setup/profile_visibility_verification.h"

#include <array>

namespace {

localpinyin::setup::ProfileVisibilityAttempt visible_attempt() {
    return {S_OK, S_OK, true, true, true, S_OK, 3, true};
}

localpinyin::setup::ProfileVisibilityAttempt get_failure(HRESULT hr) {
    return {S_OK, hr, false, false, false, S_OK, 0, false};
}

}  // namespace

int main() {
    using localpinyin::setup::ProfileVisibilityAttempt;
    using localpinyin::setup::evaluate_profile_visibility_attempts;

    {
        const std::array attempts{visible_attempt()};
        const auto result = evaluate_profile_visibility_attempts(attempts);
        REQUIRE_TRUE(result.visible);
        REQUIRE_EQ(result.successful_attempt, static_cast<size_t>(1));
    }
    {
        const std::array attempts{get_failure(E_FAIL), visible_attempt()};
        const auto result = evaluate_profile_visibility_attempts(attempts);
        REQUIRE_TRUE(result.visible);
        REQUIRE_EQ(result.successful_attempt, static_cast<size_t>(2));
    }
    {
        ProfileVisibilityAttempt manager_failure{};
        manager_failure.manager_hr = REGDB_E_CLASSNOTREG;
        manager_failure.get_hr = REGDB_E_CLASSNOTREG;
        manager_failure.enum_hr = REGDB_E_CLASSNOTREG;
        const std::array attempts{manager_failure, visible_attempt()};
        const auto result = evaluate_profile_visibility_attempts(attempts);
        REQUIRE_TRUE(result.visible);
        REQUIRE_EQ(result.successful_attempt, static_cast<size_t>(2));
    }
    {
        const std::array attempts{
            get_failure(E_FAIL), get_failure(E_FAIL), get_failure(E_FAIL),
            get_failure(E_FAIL), get_failure(E_FAIL), visible_attempt()};
        const auto result = evaluate_profile_visibility_attempts(attempts);
        REQUIRE_TRUE(result.visible);
        REQUIRE_EQ(result.successful_attempt, static_cast<size_t>(6));
    }
    {
        const std::array attempts{get_failure(E_FAIL), get_failure(E_FAIL)};
        const auto result = evaluate_profile_visibility_attempts(attempts);
        REQUIRE_TRUE(!result.visible);
        REQUIRE_EQ(result.failure_hr, E_FAIL);
    }
    {
        ProfileVisibilityAttempt attempt = visible_attempt();
        attempt.enum_found = false;
        const std::array attempts{attempt};
        const auto result = evaluate_profile_visibility_attempts(attempts);
        REQUIRE_TRUE(!result.visible);
        REQUIRE_EQ(result.failure_hr, HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
    }
    for (int mismatch = 0; mismatch < 3; ++mismatch) {
        ProfileVisibilityAttempt attempt = visible_attempt();
        if (mismatch == 0) attempt.get_matches_clsid = false;
        if (mismatch == 1) attempt.get_matches_guid = false;
        if (mismatch == 2) attempt.get_matches_langid = false;
        const std::array attempts{attempt};
        const auto result = evaluate_profile_visibility_attempts(attempts);
        REQUIRE_TRUE(!result.visible);
        REQUIRE_EQ(result.failure_hr, HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
    }
    {
        ProfileVisibilityAttempt attempt = visible_attempt();
        attempt.enum_hr = E_ACCESSDENIED;
        attempt.enum_found = false;
        const std::array attempts{attempt};
        const auto result = evaluate_profile_visibility_attempts(attempts);
        REQUIRE_TRUE(!result.visible);
        REQUIRE_EQ(result.failure_hr, E_ACCESSDENIED);
    }
    {
        ProfileVisibilityAttempt miss = visible_attempt();
        miss.enum_found = false;
        const std::array attempts{miss, visible_attempt()};
        const auto result = evaluate_profile_visibility_attempts(attempts);
        REQUIRE_TRUE(result.visible);
        REQUIRE_EQ(result.successful_attempt, static_cast<size_t>(2));
    }

    REQUIRE_EQ(localpinyin::setup::kProfileVisibilityRetryDelaysMs.size(),
               static_cast<size_t>(6));
    REQUIRE_EQ(localpinyin::setup::profile_visibility_max_total_wait_ms(), 2000u);

    const auto load_success =
        localpinyin::setup::evaluate_load_library_diagnostic(true, ERROR_ENVVAR_NOT_FOUND);
    REQUIRE_TRUE(load_success.success);
    REQUIRE_TRUE(!load_success.include_win32_error);
    REQUIRE_EQ(load_success.win32_error, ERROR_SUCCESS);

    const auto load_failure =
        localpinyin::setup::evaluate_load_library_diagnostic(false, ERROR_MOD_NOT_FOUND);
    REQUIRE_TRUE(!load_failure.success);
    REQUIRE_TRUE(load_failure.include_win32_error);
    REQUIRE_EQ(load_failure.win32_error, ERROR_MOD_NOT_FOUND);
    return 0;
}
