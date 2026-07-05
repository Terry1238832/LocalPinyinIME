#pragma once

#include <windows.h>

namespace localpinyin {

enum class RegistrationOperation {
    SnapshotProfile,
    RegisterCategories,
    RegisterProfile,
    RegisterComClass,
};

[[nodiscard]] constexpr int registration_operation_rank(RegistrationOperation operation) noexcept {
    switch (operation) {
    case RegistrationOperation::SnapshotProfile:
        return 0;
    case RegistrationOperation::RegisterCategories:
        return 1;
    case RegistrationOperation::RegisterProfile:
        return 2;
    case RegistrationOperation::RegisterComClass:
        return 3;
    }
    return -1;
}

[[nodiscard]] constexpr bool registration_operation_precedes(RegistrationOperation before,
                                                             RegistrationOperation after) noexcept {
    return registration_operation_rank(before) < registration_operation_rank(after);
}

[[nodiscard]] constexpr bool should_unregister_previous_shared_clsid_before_register() noexcept {
    return false;
}

[[nodiscard]] constexpr bool should_unregister_previous_shared_clsid_after_register() noexcept {
    return false;
}

[[nodiscard]] constexpr bool should_restore_previous_inproc_on_register_failure(bool had_previous_inproc,
                                                                                bool new_registration_verified) noexcept {
    return had_previous_inproc && !new_registration_verified;
}

struct CategoryRegistrationDecision {
    bool query_failed;
    bool should_register;
    bool skip_existing;
    bool verify_by_profile_caps;
};

[[nodiscard]] constexpr CategoryRegistrationDecision decide_category_registration(HRESULT enum_hr,
                                                                                  bool already_contains,
                                                                                  bool allow_register_category = true) noexcept {
    return CategoryRegistrationDecision{
        FAILED(enum_hr),
        SUCCEEDED(enum_hr) && !already_contains && allow_register_category,
        SUCCEEDED(enum_hr) && already_contains,
        SUCCEEDED(enum_hr) && !already_contains && !allow_register_category,
    };
}

struct CategoryPostRegisterVerification {
    HRESULT hr;
    bool query_failed;
    bool verified;
    bool missing_required_item;
    bool added_this_call;
};

struct ProfileCapsObservation {
    DWORD observed_caps;
    DWORD expected_caps;
    DWORD missing_caps;
    bool all_expected_caps_present;
};

[[nodiscard]] constexpr ProfileCapsObservation observe_profile_caps(DWORD observed_caps,
                                                                    DWORD expected_caps) noexcept {
    const DWORD missing_caps = expected_caps & ~observed_caps;
    return ProfileCapsObservation{
        observed_caps,
        expected_caps,
        missing_caps,
        missing_caps == 0,
    };
}

struct ProfilePostRegisterVerification {
    HRESULT hr;
    bool query_failed;
    bool missing_profile;
    bool added_this_call;
    ProfileCapsObservation caps;
};

[[nodiscard]] constexpr ProfilePostRegisterVerification evaluate_profile_after_register(
    HRESULT query_hr,
    bool exists_after,
    DWORD observed_caps,
    bool existed_before,
    DWORD expected_caps) noexcept {
    const ProfileCapsObservation caps = observe_profile_caps(observed_caps, expected_caps);
    if (FAILED(query_hr)) {
        return ProfilePostRegisterVerification{
            query_hr,
            true,
            false,
            false,
            caps,
        };
    }

    if (!exists_after) {
        return ProfilePostRegisterVerification{
            HRESULT_FROM_WIN32(ERROR_NOT_FOUND),
            false,
            true,
            false,
            caps,
        };
    }

    return ProfilePostRegisterVerification{
        S_OK,
        false,
        false,
        !existed_before,
        caps,
    };
}

[[nodiscard]] constexpr bool should_overwrite_registration_diagnostic(bool has_existing_diagnostic) noexcept {
    return !has_existing_diagnostic;
}

[[nodiscard]] constexpr bool is_system_registration_verified(bool get_profile_ok,
                                                             bool enum_profiles_contains,
                                                             bool keyboard_category_present,
                                                             bool required_caps_present) noexcept {
    (void)required_caps_present;
    return get_profile_ok && enum_profiles_contains && keyboard_category_present;
}

[[nodiscard]] inline CategoryPostRegisterVerification evaluate_category_after_register(
    HRESULT enum_after_hr,
    bool existed_before,
    bool exists_after,
    bool allow_register_category) noexcept {
    if (FAILED(enum_after_hr)) {
        return CategoryPostRegisterVerification{
            enum_after_hr,
            true,
            false,
            false,
            false,
        };
    }

    if (exists_after) {
        return CategoryPostRegisterVerification{
            S_OK,
            false,
            true,
            false,
            !existed_before,
        };
    }

    if (allow_register_category) {
        return CategoryPostRegisterVerification{
            HRESULT_FROM_WIN32(ERROR_NOT_FOUND),
            false,
            false,
            true,
            false,
        };
    }

    return CategoryPostRegisterVerification{
        S_OK,
        false,
        false,
        false,
        false,
    };
}

[[nodiscard]] constexpr bool should_rollback_category(bool existed_before, bool added_this_call) noexcept {
    return !existed_before && added_this_call;
}

[[nodiscard]] constexpr DWORD profile_registration_flags() noexcept {
    return 0;
}

HRESULT register_server();
HRESULT unregister_server();
const wchar_t* last_registration_diagnostic() noexcept;

}  // namespace localpinyin
