#pragma once

#include <msctf.h>
#include <windows.h>

#include <cstdint>
#include <string>

namespace localpinyin {

struct ActivationFocusDiagnostics {
    bool text_service_activated = false;
    DWORD activation_flags = 0;
    bool thread_mgr2_available = false;
    HRESULT thread_mgr2_qi_hr = E_NOINTERFACE;
    bool get_active_flags_called = false;
    HRESULT get_active_flags_hr = E_UNEXPECTED;
    DWORD active_flags = 0;
    std::uintptr_t thread_manager_identity = 0;
    TfClientId client_id = TF_CLIENTID_NULL;
    bool key_sink_advise_called = false;
    HRESULT key_sink_advise_hr = E_UNEXPECTED;
    bool key_sink_advised = false;
    bool thread_focus_set_received = false;
    bool thread_focus_kill_received = false;
    bool document_focus_changed = false;
    std::uintptr_t old_document_mgr_identity = 0;
    std::uintptr_t new_document_mgr_identity = 0;
    std::uintptr_t document_mgr_identity = 0;
    HRESULT context_stack_top_hr = E_UNEXPECTED;
    bool context_stack_top_present = false;
    std::uintptr_t context_identity = 0;
    bool on_test_key_down_received = false;
    bool on_key_down_received = false;
    HRESULT compartment_read_hr = E_UNEXPECTED;
    bool compartment_open_close_state = false;
    bool language_bar_update_requested = false;
    const wchar_t* language_bar_state = L"unknown";
    bool deactivation_started = false;
    bool key_sink_unadvise_called = false;
    HRESULT key_sink_unadvise_hr = E_UNEXPECTED;
};

std::wstring make_activation_focus_diagnostic_log(const ActivationFocusDiagnostics& diagnostics);

}  // namespace localpinyin
