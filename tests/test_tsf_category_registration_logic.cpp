#include "test_common.h"
#include "../src/ime/registration.h"
#include "../src/ime/tsf_profile_categories.h"

#include <msctf.h>

#include <vector>

namespace {

bool contains_guid(const std::vector<GUID>& items, REFGUID expected) {
    for (const auto& item : items) {
        if (IsEqualGUID(item, expected)) {
            return true;
        }
    }
    return false;
}

HRESULT simulate_register_then_verify(const localpinyin::TsfProfileCategory& category,
                                      HRESULT register_hr,
                                      HRESULT enum_after_hr,
                                      const std::vector<GUID>& enum_items,
                                      bool existed_before,
                                      localpinyin::CategoryPostRegisterVerification& verification) {
    verification = localpinyin::CategoryPostRegisterVerification{
        E_UNEXPECTED,
        false,
        false,
        false,
        false,
    };
    if (FAILED(register_hr)) {
        return register_hr;
    }

    const bool exists_after = SUCCEEDED(enum_after_hr) &&
                              contains_guid(enum_items, *category.item_guid);
    verification = localpinyin::evaluate_category_after_register(enum_after_hr,
                                                                 existed_before,
                                                                 exists_after,
                                                                 category.allow_register_category);
    return verification.hr;
}

const localpinyin::TsfProfileCategory& keyboard_category() {
    for (const auto& category : localpinyin::required_tsf_profile_categories()) {
        if (IsEqualGUID(*category.category_guid, GUID_TFCAT_TIP_KEYBOARD)) {
            return category;
        }
    }
    std::abort();
}

localpinyin::TsfProfileCategory optional_immersive_capability() {
    for (const auto& capability : localpinyin::required_tsf_profile_capabilities()) {
        if (IsEqualGUID(*capability.category_guid, GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT)) {
            return capability;
        }
    }
    std::abort();
}

}  // namespace

int main() {
    const auto& required = keyboard_category();
    REQUIRE_TRUE(required.allow_register_category);
    REQUIRE_TRUE(required.allow_inconclusive_post_register_verification);
    REQUIRE_EQ(required.check_strategy, localpinyin::TsfCapabilityCheckStrategy::CategoryContainsItem);
    REQUIRE_EQ(required.profile_caps, static_cast<DWORD>(0));

    const localpinyin::CategoryMembershipObservation miss{S_OK, 4, false};
    const localpinyin::CategoryMembershipObservation category_found{S_OK, 4, true};
    const localpinyin::CategoryMembershipObservation item_found{S_OK, 2, true};

    {
        const auto verification =
            localpinyin::evaluate_bidirectional_category_verification(
                E_ACCESSDENIED, false, miss, miss, false, {}, {});
        REQUIRE_EQ(verification.result,
                   localpinyin::CategoryVerificationResult::RegisterCallFailed);
        REQUIRE_TRUE(!verification.register_call_succeeded);
        REQUIRE_TRUE(!localpinyin::category_verification_allows_continue(
            verification.result, true));
    }
    {
        const auto verification =
            localpinyin::evaluate_bidirectional_category_verification(
                S_OK, false, category_found, miss, false, {}, {});
        REQUIRE_EQ(verification.result,
                   localpinyin::CategoryVerificationResult::Verified);
        REQUIRE_TRUE(verification.category_postcondition_observed);
        REQUIRE_TRUE(verification.added_this_call);
    }
    {
        const auto verification =
            localpinyin::evaluate_bidirectional_category_verification(
                S_OK, false, miss, item_found, false, {}, {});
        REQUIRE_EQ(verification.result,
                   localpinyin::CategoryVerificationResult::Verified);
        REQUIRE_TRUE(verification.same_item_to_categories.found);
    }
    {
        const auto verification =
            localpinyin::evaluate_bidirectional_category_verification(
                S_OK, false, miss, miss, true, category_found, miss);
        REQUIRE_EQ(verification.result,
                   localpinyin::CategoryVerificationResult::Verified);
        REQUIRE_TRUE(verification.fresh_manager_checked);
        REQUIRE_TRUE(verification.fresh_category_to_items.found);
    }
    {
        const auto verification =
            localpinyin::evaluate_bidirectional_category_verification(
                S_OK, false, miss, miss, true, miss, miss);
        REQUIRE_EQ(verification.result,
                   localpinyin::CategoryVerificationResult::VerificationInconclusive);
        REQUIRE_TRUE(verification.register_call_succeeded);
        REQUIRE_TRUE(!verification.category_postcondition_observed);
        REQUIRE_TRUE(localpinyin::category_verification_allows_continue(
            verification.result,
            required.allow_inconclusive_post_register_verification));
    }

    {
        localpinyin::CategoryPostRegisterVerification verification{};
        const HRESULT hr = simulate_register_then_verify(required,
                                                         S_OK,
                                                         S_OK,
                                                         {*required.item_guid},
                                                         false,
                                                         verification);
        REQUIRE_EQ(hr, S_OK);
        REQUIRE_TRUE(verification.verified);
        REQUIRE_TRUE(verification.added_this_call);
        REQUIRE_TRUE(!verification.missing_required_item);
    }

    {
        localpinyin::CategoryPostRegisterVerification verification{};
        const HRESULT hr = simulate_register_then_verify(required,
                                                         S_OK,
                                                         S_OK,
                                                         {},
                                                         false,
                                                         verification);
        REQUIRE_EQ(hr, HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
        REQUIRE_TRUE(!verification.verified);
        REQUIRE_TRUE(verification.missing_required_item);
    }

    {
        localpinyin::CategoryPostRegisterVerification verification{};
        const HRESULT hr = simulate_register_then_verify(required,
                                                         S_OK,
                                                         S_FALSE,
                                                         {},
                                                         false,
                                                         verification);
        REQUIRE_EQ(hr, HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
        REQUIRE_TRUE(verification.missing_required_item);
    }

    {
        localpinyin::CategoryPostRegisterVerification verification{};
        const HRESULT hr = simulate_register_then_verify(required,
                                                         S_OK,
                                                         S_OK,
                                                         {},
                                                         false,
                                                         verification);
        REQUIRE_EQ(hr, HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
        REQUIRE_TRUE(verification.missing_required_item);
    }

    {
        localpinyin::CategoryPostRegisterVerification verification{};
        const HRESULT hr = simulate_register_then_verify(required,
                                                         E_ACCESSDENIED,
                                                         S_OK,
                                                         {*required.item_guid},
                                                         false,
                                                         verification);
        REQUIRE_EQ(hr, E_ACCESSDENIED);
        REQUIRE_EQ(verification.hr, E_UNEXPECTED);
    }

    {
        localpinyin::CategoryPostRegisterVerification verification{};
        const HRESULT hr = simulate_register_then_verify(required,
                                                         S_OK,
                                                         E_FAIL,
                                                         {},
                                                         false,
                                                         verification);
        REQUIRE_EQ(hr, E_FAIL);
        REQUIRE_TRUE(verification.query_failed);
    }

    {
        const auto verification = localpinyin::evaluate_category_after_register_with_fresh_manager(
            S_OK,
            false,
            false,
            true,
            S_OK,
            true,
            true);
        REQUIRE_EQ(verification.hr, S_OK);
        REQUIRE_TRUE(verification.verified);
        REQUIRE_TRUE(verification.added_this_call);
        REQUIRE_TRUE(verification.fresh_manager_checked);
        REQUIRE_TRUE(verification.same_manager_stale);
        REQUIRE_TRUE(verification.fresh_manager_verified);
        REQUIRE_TRUE(!verification.missing_required_item);
    }

    {
        const auto verification = localpinyin::evaluate_category_after_register_with_fresh_manager(
            S_OK,
            false,
            false,
            true,
            S_OK,
            false,
            true);
        REQUIRE_EQ(verification.hr, HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
        REQUIRE_TRUE(!verification.verified);
        REQUIRE_TRUE(verification.fresh_manager_checked);
        REQUIRE_TRUE(!verification.same_manager_stale);
        REQUIRE_TRUE(!verification.fresh_manager_verified);
        REQUIRE_TRUE(verification.missing_required_item);
    }

    {
        const auto verification = localpinyin::evaluate_category_after_register_with_fresh_manager(
            S_OK,
            false,
            false,
            true,
            E_ACCESSDENIED,
            false,
            true);
        REQUIRE_EQ(verification.hr, E_ACCESSDENIED);
        REQUIRE_TRUE(verification.query_failed);
        REQUIRE_TRUE(verification.fresh_manager_checked);
        REQUIRE_TRUE(!verification.fresh_manager_verified);
    }

    const auto optional = optional_immersive_capability();
    REQUIRE_TRUE(optional.allow_register_category);
    REQUIRE_TRUE(!optional.allow_inconclusive_post_register_verification);
    REQUIRE_EQ(optional.check_strategy, localpinyin::TsfCapabilityCheckStrategy::CategoryContainsItem);
    REQUIRE_EQ(optional.profile_caps, static_cast<DWORD>(TF_IPP_CAPS_IMMERSIVESUPPORT));
    REQUIRE_TRUE(!localpinyin::category_verification_allows_continue(
        localpinyin::CategoryVerificationResult::VerificationInconclusive,
        optional.allow_inconclusive_post_register_verification));

    {
        const auto decision = localpinyin::decide_category_registration(S_OK,
                                                                        false,
                                                                        optional.allow_register_category);
        REQUIRE_TRUE(decision.should_register);
        REQUIRE_TRUE(!decision.verify_by_profile_caps);
    }

    {
        localpinyin::CategoryPostRegisterVerification verification{};
        const HRESULT hr = simulate_register_then_verify(optional,
                                                         S_OK,
                                                         S_OK,
                                                         {},
                                                         false,
                                                         verification);
        REQUIRE_EQ(hr, HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
        REQUIRE_TRUE(!verification.verified);
        REQUIRE_TRUE(!verification.added_this_call);
        REQUIRE_TRUE(verification.missing_required_item);
        REQUIRE_EQ(localpinyin::apply_category_verification_policy(hr, false), S_OK);
    }

    bool registerable_contains_immersive = false;
    bool registerable_contains_input_mode_compartment = false;
    for (const auto& category : localpinyin::required_tsf_profile_categories()) {
        registerable_contains_immersive =
            registerable_contains_immersive ||
            IsEqualGUID(*category.category_guid, GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT);
        registerable_contains_input_mode_compartment =
            registerable_contains_input_mode_compartment ||
            IsEqualGUID(*category.category_guid, GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT);
    }
    REQUIRE_TRUE(!registerable_contains_immersive);
    REQUIRE_TRUE(!registerable_contains_input_mode_compartment);

    const auto& compatibility_categories = localpinyin::compatibility_tsf_profile_categories();
    REQUIRE_EQ(compatibility_categories.size(), static_cast<size_t>(1));
    REQUIRE_TRUE(IsEqualGUID(*compatibility_categories[0].category_guid,
                             GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT));

    bool capability_contains_input_mode_compartment = false;
    for (const auto& capability : localpinyin::required_tsf_profile_capabilities()) {
        capability_contains_input_mode_compartment =
            capability_contains_input_mode_compartment ||
            IsEqualGUID(*capability.category_guid, GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT);
    }
    REQUIRE_TRUE(!capability_contains_input_mode_compartment);

    REQUIRE_EQ((localpinyin::required_tsf_profile_caps() & TF_IPP_CAPS_IMMERSIVESUPPORT),
               static_cast<DWORD>(TF_IPP_CAPS_IMMERSIVESUPPORT));
    return 0;
}
