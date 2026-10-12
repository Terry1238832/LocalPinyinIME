#include "test_common.h"
#include "../src/ime/registration.h"
#include "../src/ime/tsf_profile_categories.h"

#include <msctf.h>

#include <cstddef>
#include <string>

namespace {

std::wstring guid_string(REFGUID guid) {
    wchar_t buffer[64]{};
    StringFromGUID2(guid, buffer, static_cast<int>(_countof(buffer)));
    return buffer;
}

}  // namespace

int main() {
    REQUIRE_TRUE(localpinyin::registration_operation_precedes(
        localpinyin::RegistrationOperation::SnapshotProfile,
        localpinyin::RegistrationOperation::RegisterProfile));
    REQUIRE_TRUE(localpinyin::registration_operation_precedes(
        localpinyin::RegistrationOperation::RegisterProfile,
        localpinyin::RegistrationOperation::RegisterCategories));
    REQUIRE_TRUE(localpinyin::registration_operation_precedes(
        localpinyin::RegistrationOperation::RegisterCategories,
        localpinyin::RegistrationOperation::RegisterComClass));
    REQUIRE_TRUE(!localpinyin::registration_operation_precedes(
        localpinyin::RegistrationOperation::RegisterCategories,
        localpinyin::RegistrationOperation::RegisterProfile));

    REQUIRE_TRUE(!localpinyin::should_unregister_previous_shared_clsid_before_register());
    REQUIRE_TRUE(!localpinyin::should_unregister_previous_shared_clsid_after_register());
    REQUIRE_TRUE(localpinyin::should_restore_previous_inproc_on_register_failure(true, false));
    REQUIRE_TRUE(!localpinyin::should_restore_previous_inproc_on_register_failure(false, false));
    REQUIRE_TRUE(!localpinyin::should_restore_previous_inproc_on_register_failure(true, true));

    {
        const auto decision = localpinyin::decide_category_registration(S_OK, true);
        REQUIRE_TRUE(!decision.query_failed);
        REQUIRE_TRUE(!decision.should_register);
        REQUIRE_TRUE(decision.skip_existing);
        REQUIRE_TRUE(!decision.verify_by_profile_caps);
    }
    {
        const auto decision = localpinyin::decide_category_registration(S_OK, false);
        REQUIRE_TRUE(!decision.query_failed);
        REQUIRE_TRUE(decision.should_register);
        REQUIRE_TRUE(!decision.skip_existing);
        REQUIRE_TRUE(!decision.verify_by_profile_caps);
    }
    {
        const auto decision = localpinyin::decide_category_registration(S_OK, false, false);
        REQUIRE_TRUE(!decision.query_failed);
        REQUIRE_TRUE(!decision.should_register);
        REQUIRE_TRUE(!decision.skip_existing);
        REQUIRE_TRUE(decision.verify_by_profile_caps);
    }
    {
        const auto decision = localpinyin::decide_category_registration(E_FAIL, false);
        REQUIRE_TRUE(decision.query_failed);
        REQUIRE_TRUE(!decision.should_register);
        REQUIRE_TRUE(!decision.skip_existing);
        REQUIRE_TRUE(!decision.verify_by_profile_caps);
    }

    REQUIRE_TRUE(!localpinyin::should_rollback_category(true, false));
    REQUIRE_TRUE(!localpinyin::should_rollback_category(true, true));
    REQUIRE_TRUE(!localpinyin::should_rollback_category(false, false));
    REQUIRE_TRUE(localpinyin::should_rollback_category(false, true));
    REQUIRE_TRUE(localpinyin::should_track_category_for_cleanup(true, S_OK, false));
    REQUIRE_TRUE(!localpinyin::should_track_category_for_cleanup(true, E_FAIL, false));
    REQUIRE_TRUE(!localpinyin::should_track_category_for_cleanup(false, S_OK, false));
    REQUIRE_TRUE(!localpinyin::should_track_category_for_cleanup(true, S_OK, true));
    REQUIRE_TRUE(localpinyin::should_attempt_category_cleanup(true, false));
    REQUIRE_TRUE(localpinyin::should_attempt_category_cleanup(false, true));
    REQUIRE_TRUE(!localpinyin::should_attempt_category_cleanup(false, false));
    REQUIRE_EQ(localpinyin::category_visibility_verification_attempts(), 3u);
    REQUIRE_EQ(localpinyin::category_visibility_retry_delay_ms(), 50u);

    const auto inconclusive =
        localpinyin::CategoryVerificationResult::VerificationInconclusive;
    REQUIRE_TRUE(localpinyin::category_verification_allows_continue(
        inconclusive, true));
    REQUIRE_TRUE(!localpinyin::category_verification_allows_continue(
        inconclusive, false));

    // An unobserved keyboard category can continue only while all core
    // profile and expected-DLL postconditions remain successful.
    REQUIRE_TRUE(localpinyin::core_registration_verification_succeeded(
        true, true, true, S_OK));
    REQUIRE_TRUE(!localpinyin::core_registration_verification_succeeded(
        false, true, true, S_OK));
    REQUIRE_TRUE(!localpinyin::core_registration_verification_succeeded(
        true, false, true, S_OK));
    REQUIRE_TRUE(!localpinyin::core_registration_verification_succeeded(
        true, true, false, S_OK));
    REQUIRE_TRUE(!localpinyin::core_registration_verification_succeeded(
        true, true, true, E_FAIL));

    // A rollback-side missing InprocServer32 result must not replace the
    // original category/profile/COM failure.
    REQUIRE_EQ(localpinyin::preserve_first_registration_failure(
                   E_ACCESSDENIED,
                   HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)),
               E_ACCESSDENIED);
    REQUIRE_EQ(localpinyin::preserve_first_registration_failure(
                   S_OK,
                   HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)),
               HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

    {
        const auto observation = localpinyin::observe_profile_caps(0, localpinyin::required_tsf_profile_caps());
        REQUIRE_EQ(observation.observed_caps, static_cast<DWORD>(0));
        REQUIRE_EQ(observation.expected_caps, localpinyin::required_tsf_profile_caps());
        REQUIRE_EQ(observation.missing_caps, localpinyin::required_tsf_profile_caps());
        REQUIRE_TRUE(!observation.all_expected_caps_present);
    }
    {
        const DWORD observed = TF_IPP_CAPS_UIELEMENTENABLED;
        const auto observation = localpinyin::observe_profile_caps(observed, localpinyin::required_tsf_profile_caps());
        REQUIRE_EQ(observation.observed_caps, observed);
        REQUIRE_EQ(observation.expected_caps, localpinyin::required_tsf_profile_caps());
        REQUIRE_EQ(observation.missing_caps,
                   static_cast<DWORD>(TF_IPP_CAPS_IMMERSIVESUPPORT | TF_IPP_CAPS_SYSTRAYSUPPORT));
        REQUIRE_TRUE(!observation.all_expected_caps_present);
    }
    {
        const auto observation = localpinyin::observe_profile_caps(localpinyin::required_tsf_profile_caps(),
                                                                   localpinyin::required_tsf_profile_caps());
        REQUIRE_EQ(observation.missing_caps, static_cast<DWORD>(0));
        REQUIRE_TRUE(observation.all_expected_caps_present);
    }
    {
        const auto verification = localpinyin::evaluate_profile_after_register(S_OK,
                                                                               true,
                                                                               0,
                                                                               false,
                                                                               localpinyin::required_tsf_profile_caps());
        REQUIRE_EQ(verification.hr, S_OK);
        REQUIRE_TRUE(!verification.query_failed);
        REQUIRE_TRUE(!verification.missing_profile);
        REQUIRE_TRUE(verification.added_this_call);
        REQUIRE_EQ(verification.caps.missing_caps, localpinyin::required_tsf_profile_caps());
        REQUIRE_TRUE(!verification.caps.all_expected_caps_present);
    }
    {
        const auto verification = localpinyin::evaluate_profile_after_register(
            S_OK,
            true,
            TF_IPP_CAPS_UIELEMENTENABLED,
            true,
            localpinyin::required_tsf_profile_caps());
        REQUIRE_EQ(verification.hr, S_OK);
        REQUIRE_TRUE(!verification.query_failed);
        REQUIRE_TRUE(!verification.missing_profile);
        REQUIRE_TRUE(!verification.added_this_call);
        REQUIRE_EQ(verification.caps.missing_caps,
                   static_cast<DWORD>(TF_IPP_CAPS_IMMERSIVESUPPORT | TF_IPP_CAPS_SYSTRAYSUPPORT));
        REQUIRE_TRUE(!verification.caps.all_expected_caps_present);
    }
    {
        const auto verification = localpinyin::evaluate_profile_after_register(E_ACCESSDENIED,
                                                                               false,
                                                                               0,
                                                                               false,
                                                                               localpinyin::required_tsf_profile_caps());
        REQUIRE_EQ(verification.hr, E_ACCESSDENIED);
        REQUIRE_TRUE(verification.query_failed);
        REQUIRE_TRUE(!verification.missing_profile);
    }
    {
        const auto verification = localpinyin::evaluate_profile_after_register(S_OK,
                                                                               false,
                                                                               0,
                                                                               false,
                                                                               localpinyin::required_tsf_profile_caps());
        REQUIRE_EQ(verification.hr, HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
        REQUIRE_TRUE(!verification.query_failed);
        REQUIRE_TRUE(verification.missing_profile);
    }
    REQUIRE_TRUE(localpinyin::is_system_registration_verified(true, true, true, false));
    REQUIRE_TRUE(localpinyin::is_system_registration_verified(true, true, true, true));
    REQUIRE_TRUE(!localpinyin::is_system_registration_verified(false, true, true, false));
    REQUIRE_TRUE(!localpinyin::is_system_registration_verified(true, false, true, false));
    REQUIRE_TRUE(!localpinyin::is_system_registration_verified(true, true, false, false));
    REQUIRE_TRUE(localpinyin::should_overwrite_registration_diagnostic(false));
    REQUIRE_TRUE(!localpinyin::should_overwrite_registration_diagnostic(true));

    const auto& capabilities = localpinyin::required_tsf_profile_capabilities();
    REQUIRE_EQ(capabilities.size(), static_cast<size_t>(4));
    bool has_keyboard = false;
    bool has_immersive_profile_cap = false;
    bool has_ui_element_profile_cap = false;
    bool has_systray_profile_cap = false;
    for (const auto& capability : capabilities) {
        REQUIRE_TRUE(capability.name != nullptr);
        REQUIRE_TRUE(capability.category_guid != nullptr);
        REQUIRE_TRUE(capability.item_guid != nullptr);
        REQUIRE_TRUE(IsEqualGUID(*capability.item_guid, localpinyin::CLSID_LocalPinyinTextService));
        if (IsEqualGUID(*capability.category_guid, GUID_TFCAT_TIP_KEYBOARD)) {
            has_keyboard = true;
            REQUIRE_TRUE(capability.allow_register_category);
            REQUIRE_EQ(capability.check_strategy, localpinyin::TsfCapabilityCheckStrategy::CategoryContainsItem);
        }
        if (IsEqualGUID(*capability.category_guid, GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT)) {
            has_immersive_profile_cap = true;
            REQUIRE_TRUE(capability.allow_register_category);
            REQUIRE_EQ(capability.check_strategy, localpinyin::TsfCapabilityCheckStrategy::CategoryContainsItem);
            REQUIRE_EQ(capability.profile_caps, static_cast<DWORD>(TF_IPP_CAPS_IMMERSIVESUPPORT));
        }
        if (IsEqualGUID(*capability.category_guid, GUID_TFCAT_TIPCAP_UIELEMENTENABLED)) {
            has_ui_element_profile_cap = true;
            REQUIRE_TRUE(!capability.allow_register_category);
            REQUIRE_EQ(capability.check_strategy, localpinyin::TsfCapabilityCheckStrategy::ProfileCaps);
            REQUIRE_EQ(capability.profile_caps, static_cast<DWORD>(TF_IPP_CAPS_UIELEMENTENABLED));
            REQUIRE_TRUE(guid_string(*capability.category_guid) == L"{49D2F9CF-1F5E-11D7-A6D3-00065B84435C}");
            REQUIRE_TRUE(guid_string(*capability.item_guid) == L"{7C0B4B75-80B0-4E1F-A4A5-4D49A5440D8A}");
        }
        REQUIRE_TRUE(!IsEqualGUID(*capability.category_guid, GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT));
        if (IsEqualGUID(*capability.category_guid, GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT)) {
            has_systray_profile_cap = true;
            REQUIRE_TRUE(!capability.allow_register_category);
            REQUIRE_EQ(capability.check_strategy, localpinyin::TsfCapabilityCheckStrategy::ProfileCaps);
            REQUIRE_EQ(capability.profile_caps, static_cast<DWORD>(TF_IPP_CAPS_SYSTRAYSUPPORT));
        }
    }
    REQUIRE_TRUE(has_keyboard);
    REQUIRE_TRUE(has_immersive_profile_cap);
    REQUIRE_TRUE(has_ui_element_profile_cap);
    REQUIRE_TRUE(has_systray_profile_cap);

    const auto& categories = localpinyin::required_tsf_profile_categories();
    REQUIRE_EQ(categories.size(), static_cast<size_t>(1));
    bool registerable_has_keyboard = false;
    bool registerable_has_immersive = false;
    for (const auto& category : categories) {
        REQUIRE_TRUE(category.allow_register_category);
        REQUIRE_EQ(category.check_strategy, localpinyin::TsfCapabilityCheckStrategy::CategoryContainsItem);
        registerable_has_keyboard = registerable_has_keyboard ||
            IsEqualGUID(*category.category_guid, GUID_TFCAT_TIP_KEYBOARD);
        registerable_has_immersive = registerable_has_immersive ||
            IsEqualGUID(*category.category_guid, GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT);
        REQUIRE_TRUE(!IsEqualGUID(*category.category_guid, GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT));
    }
    REQUIRE_TRUE(registerable_has_keyboard);
    REQUIRE_TRUE(!registerable_has_immersive);
    const auto& compatibility_categories = localpinyin::compatibility_tsf_profile_categories();
    REQUIRE_EQ(compatibility_categories.size(), static_cast<size_t>(1));
    REQUIRE_TRUE(IsEqualGUID(*compatibility_categories[0].category_guid,
                             GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT));
    REQUIRE_TRUE(compatibility_categories[0].allow_register_category);
    REQUIRE_EQ(localpinyin::apply_category_verification_policy(
                   HRESULT_FROM_WIN32(ERROR_NOT_FOUND), false),
               S_OK);
    REQUIRE_EQ(localpinyin::apply_category_verification_policy(
                   HRESULT_FROM_WIN32(ERROR_NOT_FOUND), true),
               HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
    REQUIRE_EQ((localpinyin::required_tsf_profile_caps() & TF_IPP_CAPS_IMMERSIVESUPPORT),
               static_cast<DWORD>(TF_IPP_CAPS_IMMERSIVESUPPORT));
    REQUIRE_EQ((localpinyin::required_tsf_profile_caps() & TF_IPP_CAPS_UIELEMENTENABLED),
               static_cast<DWORD>(TF_IPP_CAPS_UIELEMENTENABLED));
    REQUIRE_EQ((localpinyin::required_tsf_profile_caps() & TF_IPP_CAPS_SYSTRAYSUPPORT),
               static_cast<DWORD>(TF_IPP_CAPS_SYSTRAYSUPPORT));

    REQUIRE_EQ(localpinyin::profile_registration_flags(), static_cast<DWORD>(0));
    REQUIRE_TRUE(localpinyin::profile_registration_flags() != localpinyin::required_tsf_profile_caps());
    REQUIRE_EQ((localpinyin::profile_registration_flags() & TF_RP_HIDDENINSETTINGUI), static_cast<DWORD>(0));
    REQUIRE_EQ((localpinyin::profile_registration_flags() & TF_RP_LOCALPROCESS), static_cast<DWORD>(0));
    REQUIRE_EQ((localpinyin::profile_registration_flags() & TF_RP_LOCALTHREAD), static_cast<DWORD>(0));
    return 0;
}
