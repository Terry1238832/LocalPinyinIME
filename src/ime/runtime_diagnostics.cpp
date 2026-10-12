#include "runtime_diagnostics.h"

#include "../common/win32_utils.h"

#include <sstream>

namespace localpinyin {
namespace {

const wchar_t* bool_text(bool value) noexcept {
    return value ? L"true" : L"false";
}

}  // namespace

std::wstring make_activation_focus_diagnostic_log(
    const ActivationFocusDiagnostics& diagnostics) {
    std::wstringstream stream;
    stream << L"text_service_activated=" << bool_text(diagnostics.text_service_activated)
           << L" activation_flags=" << diagnostics.activation_flags
           << L" thread_mgr2_available=" << bool_text(diagnostics.thread_mgr2_available)
           << L" thread_mgr2_qi_hr=" << hresult_hex(diagnostics.thread_mgr2_qi_hr)
           << L" get_active_flags_called=" << bool_text(diagnostics.get_active_flags_called)
           << L" get_active_flags_hr=" << hresult_hex(diagnostics.get_active_flags_hr)
           << L" active_flags=" << diagnostics.active_flags
           << L" active_flag_immersive_mode="
           << bool_text((diagnostics.active_flags & TF_TMF_IMMERSIVEMODE) != 0)
           << L" active_flag_ui_element_enabled_only="
           << bool_text((diagnostics.active_flags & TF_TMF_UIELEMENTENABLEDONLY) != 0)
           << L" active_flag_comless=" << bool_text((diagnostics.active_flags & TF_TMF_COMLESS) != 0)
           << L" active_flag_secure_mode=" << bool_text((diagnostics.active_flags & TF_TMF_SECUREMODE) != 0)
           << L" active_flag_console=" << bool_text((diagnostics.active_flags & TF_TMF_CONSOLE) != 0)
           << L" active_flag_activated=" << bool_text((diagnostics.active_flags & TF_TMF_ACTIVATED) != 0)
           << L" thread_manager_identity=" << diagnostics.thread_manager_identity
           << L" client_id=" << diagnostics.client_id
           << L" key_sink_advise_called=" << bool_text(diagnostics.key_sink_advise_called)
           << L" key_sink_advise_hr=" << hresult_hex(diagnostics.key_sink_advise_hr)
           << L" key_sink_advised=" << bool_text(diagnostics.key_sink_advised)
           << L" thread_focus_set_received=" << bool_text(diagnostics.thread_focus_set_received)
           << L" thread_focus_kill_received=" << bool_text(diagnostics.thread_focus_kill_received)
           << L" document_focus_changed=" << bool_text(diagnostics.document_focus_changed)
           << L" old_document_mgr_identity=" << diagnostics.old_document_mgr_identity
           << L" new_document_mgr_identity=" << diagnostics.new_document_mgr_identity
           << L" document_mgr_identity=" << diagnostics.document_mgr_identity
           << L" context_stack_top_hr=" << hresult_hex(diagnostics.context_stack_top_hr)
           << L" context_stack_top_present=" << bool_text(diagnostics.context_stack_top_present)
           << L" context_identity=" << diagnostics.context_identity
           << L" on_test_key_down_received=" << bool_text(diagnostics.on_test_key_down_received)
           << L" on_key_down_received=" << bool_text(diagnostics.on_key_down_received)
           << L" compartment_read_hr=" << hresult_hex(diagnostics.compartment_read_hr)
           << L" compartment_open_close_state=" << bool_text(diagnostics.compartment_open_close_state)
           << L" language_bar_update_requested=" << bool_text(diagnostics.language_bar_update_requested)
           << L" language_bar_state=" << (diagnostics.language_bar_state ? diagnostics.language_bar_state : L"unknown")
           << L" deactivation_started=" << bool_text(diagnostics.deactivation_started)
           << L" key_sink_unadvise_called=" << bool_text(diagnostics.key_sink_unadvise_called)
           << L" key_sink_unadvise_hr=" << hresult_hex(diagnostics.key_sink_unadvise_hr);
    return stream.str();
}

}  // namespace localpinyin
