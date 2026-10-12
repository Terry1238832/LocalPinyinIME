#include "test_common.h"
#include "../src/ime/composition_manager.h"
#include "../src/ime/runtime_diagnostics.h"

#include <string>

int main() {
    using namespace localpinyin;

    const auto request_failed = make_edit_session_request_result(E_FAIL, S_OK);
    REQUIRE_EQ(request_failed.request_hr, E_FAIL);
    REQUIRE_EQ(request_failed.session_hr, S_OK);
    REQUIRE_TRUE(!request_failed.session_invoked);
    REQUIRE_EQ(request_failed.effective_hr(), E_FAIL);

    const auto session_failed = make_edit_session_request_result(S_OK, E_ACCESSDENIED);
    REQUIRE_EQ(session_failed.request_hr, S_OK);
    REQUIRE_EQ(session_failed.session_hr, E_ACCESSDENIED);
    REQUIRE_TRUE(session_failed.session_invoked);
    REQUIRE_EQ(session_failed.effective_hr(), E_ACCESSDENIED);

    ActivationFocusDiagnostics activation{};
    activation.text_service_activated = true;
    activation.activation_flags = 5;
    activation.thread_mgr2_available = true;
    activation.thread_mgr2_qi_hr = S_OK;
    activation.get_active_flags_called = true;
    activation.get_active_flags_hr = S_OK;
    activation.active_flags = TF_TMF_IMMERSIVEMODE | TF_TMF_UIELEMENTENABLEDONLY | TF_TMF_ACTIVATED;
    activation.thread_manager_identity = 0x100;
    activation.client_id = 9;
    activation.key_sink_advise_called = true;
    activation.key_sink_advise_hr = S_OK;
    activation.key_sink_advised = true;
    activation.thread_focus_set_received = true;
    activation.document_focus_changed = true;
    activation.old_document_mgr_identity = 0x200;
    activation.new_document_mgr_identity = 0x300;
    activation.document_mgr_identity = 0x300;
    activation.context_stack_top_hr = S_OK;
    activation.context_stack_top_present = true;
    activation.context_identity = 0x400;
    activation.on_test_key_down_received = true;
    activation.on_key_down_received = true;
    activation.compartment_read_hr = S_OK;
    activation.compartment_open_close_state = true;
    activation.language_bar_update_requested = true;
    activation.language_bar_state = L"Chinese";
    activation.key_sink_unadvise_called = true;
    activation.key_sink_unadvise_hr = S_OK;
    const std::wstring activation_log = make_activation_focus_diagnostic_log(activation);
    REQUIRE_TRUE(activation_log.find(L"text_service_activated=true") != std::wstring::npos);
    REQUIRE_TRUE(activation_log.find(L"activation_flags=5") != std::wstring::npos);
    REQUIRE_TRUE(activation_log.find(L"thread_mgr2_available=true") != std::wstring::npos);
    REQUIRE_TRUE(activation_log.find(L"get_active_flags_called=true") != std::wstring::npos);
    REQUIRE_TRUE(activation_log.find(L"active_flag_immersive_mode=true") != std::wstring::npos);
    REQUIRE_TRUE(activation_log.find(L"active_flag_ui_element_enabled_only=true") != std::wstring::npos);
    REQUIRE_TRUE(activation_log.find(L"active_flag_activated=true") != std::wstring::npos);
    REQUIRE_TRUE(activation_log.find(L"active_flag_secure_mode=false") != std::wstring::npos);
    REQUIRE_TRUE(activation_log.find(L"context_stack_top_present=true") != std::wstring::npos);
    REQUIRE_TRUE(activation_log.find(L"nihao") == std::wstring::npos);
    REQUIRE_TRUE(activation_log.find(L"\u4F60\u597D") == std::wstring::npos);

    return 0;
}
