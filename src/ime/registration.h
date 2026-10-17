#pragma once

#include <windows.h>

namespace localpinyin {

// The registry view this build registers its COM class in: 32-bit
// applications load the 32-bit input method from the 32-bit view, 64-bit
// ones the 64-bit DLL from the 64-bit view. The TSF profile under
// SOFTWARE\Microsoft\CTF is shared by both views.
#ifdef _WIN64
inline constexpr REGSAM kRegistryView = KEY_WOW64_64KEY;
#else
inline constexpr REGSAM kRegistryView = KEY_WOW64_32KEY;
#endif

enum class RegistrationOperation {
    SnapshotProfile,
    RegisterProfile,
    RegisterCategories,
    RegisterComClass,
};

[[nodiscard]] constexpr int registration_operation_rank(RegistrationOperation operation) noexcept {
    switch (operation) {
    case RegistrationOperation::SnapshotProfile:
        return 0;
    case RegistrationOperation::RegisterProfile:
        return 1;
    case RegistrationOperation::RegisterCategories:
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

enum class CategoryVerificationResult {
    RegisterCallFailed,
    Verified,
    VerificationInconclusive,
};

struct CategoryMembershipObservation {
    HRESULT hr = E_UNEXPECTED;
    unsigned int count = 0;
    bool found = false;
};

struct BidirectionalCategoryVerification {
    CategoryVerificationResult result = CategoryVerificationResult::VerificationInconclusive;
    HRESULT register_hr = E_UNEXPECTED;
    CategoryMembershipObservation same_category_to_items;
    CategoryMembershipObservation same_item_to_categories;
    bool fresh_manager_checked = false;
    CategoryMembershipObservation fresh_category_to_items;
    CategoryMembershipObservation fresh_item_to_categories;
    bool register_call_succeeded = false;
    bool category_postcondition_observed = false;
    bool added_this_call = false;
};

[[nodiscard]] constexpr BidirectionalCategoryVerification evaluate_bidirectional_category_verification(
    HRESULT register_hr,
    bool existed_before,
    CategoryMembershipObservation same_category_to_items,
    CategoryMembershipObservation same_item_to_categories,
    bool fresh_manager_checked,
    CategoryMembershipObservation fresh_category_to_items,
    CategoryMembershipObservation fresh_item_to_categories) noexcept {
    BidirectionalCategoryVerification result{};
    result.register_hr = register_hr;
    result.same_category_to_items = same_category_to_items;
    result.same_item_to_categories = same_item_to_categories;
    result.fresh_manager_checked = fresh_manager_checked;
    result.fresh_category_to_items = fresh_category_to_items;
    result.fresh_item_to_categories = fresh_item_to_categories;
    result.register_call_succeeded = SUCCEEDED(register_hr);
    result.category_postcondition_observed =
        same_category_to_items.found || same_item_to_categories.found ||
        (fresh_manager_checked &&
         (fresh_category_to_items.found || fresh_item_to_categories.found));
    result.added_this_call = result.register_call_succeeded && !existed_before;
    result.result = FAILED(register_hr)
        ? CategoryVerificationResult::RegisterCallFailed
        : (result.category_postcondition_observed
               ? CategoryVerificationResult::Verified
               : CategoryVerificationResult::VerificationInconclusive);
    return result;
}

[[nodiscard]] constexpr bool category_verification_allows_continue(
    CategoryVerificationResult result,
    bool allow_inconclusive) noexcept {
    return result == CategoryVerificationResult::Verified ||
           (result == CategoryVerificationResult::VerificationInconclusive &&
            allow_inconclusive);
}

[[nodiscard]] constexpr bool should_track_category_for_cleanup(
    bool register_called,
    HRESULT register_hr,
    bool existed_before) noexcept {
    return register_called && SUCCEEDED(register_hr) && !existed_before;
}

[[nodiscard]] constexpr bool should_attempt_category_cleanup(
    bool register_call_succeeded,
    bool category_postcondition_observed) noexcept {
    return register_call_succeeded || category_postcondition_observed;
}

[[nodiscard]] constexpr bool core_registration_verification_succeeded(
    bool get_profile_ok,
    bool enum_profiles_contains,
    bool non_keyboard_required_categories_present,
    HRESULT expected_dll_hr) noexcept {
    return get_profile_ok && enum_profiles_contains &&
           non_keyboard_required_categories_present && SUCCEEDED(expected_dll_hr);
}

[[nodiscard]] constexpr HRESULT preserve_first_registration_failure(
    HRESULT first_failure,
    HRESULT later_failure) noexcept {
    return FAILED(first_failure) ? first_failure : later_failure;
}

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

struct CategoryFreshManagerVerification {
    HRESULT hr;
    bool query_failed;
    bool verified;
    bool missing_required_item;
    bool added_this_call;
    bool fresh_manager_checked;
    bool same_manager_stale;
    bool fresh_manager_verified;
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

[[nodiscard]] constexpr HRESULT apply_category_verification_policy(HRESULT verification_hr,
                                                                   bool fatal) noexcept {
    return fatal ? verification_hr : S_OK;
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

[[nodiscard]] inline CategoryFreshManagerVerification evaluate_category_after_register_with_fresh_manager(
    HRESULT same_manager_hr,
    bool existed_before,
    bool same_manager_exists_after,
    bool allow_register_category,
    HRESULT fresh_manager_hr,
    bool fresh_manager_exists_after,
    bool fresh_manager_checked) noexcept {
    const CategoryPostRegisterVerification same_manager =
        evaluate_category_after_register(same_manager_hr,
                                         existed_before,
                                         same_manager_exists_after,
                                         allow_register_category);
    if (SUCCEEDED(same_manager.hr) || same_manager.query_failed || !same_manager.missing_required_item) {
        return CategoryFreshManagerVerification{
            same_manager.hr,
            same_manager.query_failed,
            same_manager.verified,
            same_manager.missing_required_item,
            same_manager.added_this_call,
            false,
            false,
            false,
        };
    }

    if (!fresh_manager_checked) {
        return CategoryFreshManagerVerification{
            same_manager.hr,
            false,
            false,
            true,
            false,
            false,
            false,
            false,
        };
    }

    if (FAILED(fresh_manager_hr)) {
        return CategoryFreshManagerVerification{
            fresh_manager_hr,
            true,
            false,
            false,
            false,
            true,
            false,
            false,
        };
    }

    if (fresh_manager_exists_after) {
        return CategoryFreshManagerVerification{
            S_OK,
            false,
            true,
            false,
            !existed_before,
            true,
            true,
            true,
        };
    }

    return CategoryFreshManagerVerification{
        HRESULT_FROM_WIN32(ERROR_NOT_FOUND),
        false,
        false,
        true,
        false,
        true,
        false,
        false,
    };
}

[[nodiscard]] constexpr bool should_rollback_category(bool existed_before, bool added_this_call) noexcept {
    return !existed_before && added_this_call;
}

[[nodiscard]] constexpr unsigned int category_visibility_verification_attempts() noexcept {
    return 3;
}

[[nodiscard]] constexpr unsigned int category_visibility_retry_delay_ms() noexcept {
    return 50;
}

[[nodiscard]] constexpr DWORD profile_registration_flags() noexcept {
    return 0;
}

HRESULT register_server();
HRESULT unregister_server();
const wchar_t* last_registration_diagnostic() noexcept;

}  // namespace localpinyin
