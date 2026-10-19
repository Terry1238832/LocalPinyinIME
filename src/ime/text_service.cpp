#include "text_service.h"

#include "globals.h"
#include "chinese_punctuation.h"
#include "../common/logging.h"
#include "../common/win32_utils.h"
#include "../engine/candidate_selection.h"

#include <algorithm>
#include <array>
#include <cwchar>
#include <cwctype>
#include <cstdint>
#include <mutex>
#include <oleauto.h>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <wrl/client.h>

namespace localpinyin {
namespace {

// The host's executable name, e.g. code.exe. Tests name a host through
// LOCALPINYINIME_TEST_HOST_EXECUTABLE.
std::wstring host_executable_name() {
    wchar_t buffer[MAX_PATH]{};
    if (GetEnvironmentVariableW(L"LOCALPINYINIME_TEST_HOST_EXECUTABLE", buffer, MAX_PATH) - 1u < MAX_PATH - 1u)
        return buffer;
    const std::wstring path = module_path(nullptr);
    return path.substr(path.find_last_of(L"\\/") + 1);
}

thread_local TextService* g_thread_keyboard_hook_service = nullptr;
thread_local TextService* g_thread_mouse_hook_service = nullptr;
// Remembered commits are a ranking hint only while the user keeps typing.
constexpr ULONGLONG kRecentCommitLifetimeMs = 120000;

const wchar_t* bool_text(bool value) noexcept {
    return value ? L"true" : L"false";
}

constexpr std::array<UINT, 3> kAnchorFollowDelaysMs{40, 120, 300};

// Tab to English: a run ends after this long without typing, and holds at
// most this much (the end of it is translated).
constexpr ULONGLONG kTranslationRunLifetimeMs = 120000;
constexpr size_t kMaxTranslationUnits = 500;
// Stands for a character the application typed itself.
constexpr wchar_t kAnyCharacter = 0xFFFF;
// Said only when the answer takes longer than this.
constexpr UINT kTranslationNoticeDelayMs = 350;
// Tab twice within this opens the tool panel.
constexpr ULONGLONG kDoubleTabMs = 300;
constexpr int kTabTask = 0;
constexpr int kPanelTask = 1;
// WM_COPYDATA from the tool panel: the session token, a line break, the text.
constexpr ULONG_PTR kPanelInsertMessage = 0x4C50494E;  // 'LPIN'
constexpr UINT kTranslationErrorShownMs = 2600;
// An application may take an edit into the input method's view of its
// document and not onto the screen; it is checked again after this long.
constexpr UINT kTranslationCheckMs = 250;

bool is_modifier_key(unsigned long key) noexcept {
    switch (key) {
        case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT: case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
        case VK_MENU: case VK_LMENU: case VK_RMENU: case VK_CAPITAL: case VK_LWIN: case VK_RWIN:
            return true;
        default:
            return false;
    }
}

bool is_han(wchar_t ch) noexcept {
    return (ch >= 0x3400 && ch <= 0x9FFF) || (ch >= 0xF900 && ch <= 0xFAFF);
}

// Whether `before` (text before the caret) ends with `run`, where the run's
// kAnyCharacter matches whatever the application typed.
bool ends_with_run(const std::wstring& before, const std::wstring& run) noexcept {
    if (before.size() < run.size()) return false;
    const size_t start = before.size() - run.size();
    for (size_t index = 0; index < run.size(); ++index) {
        if (run[index] != kAnyCharacter && run[index] != before[start + index]) return false;
    }
    return true;
}

bool is_space(wchar_t ch) noexcept {
    return ch == L' ' || ch == 0x3000 || ch == L'\t';
}

bool usable_anchor_rect(const RECT& rect) noexcept {
    return rect.right >= rect.left && rect.bottom > rect.top && !(rect.left == rect.right && rect.top == rect.bottom);
}

bool caps_lock_on(TextService::KeyStateReader reader, void* context) noexcept {
    // Test hosts report CapsLock through the same reader as held keys.
    return reader ? reader(VK_CAPITAL, context) : (GetKeyState(VK_CAPITAL) & 1) != 0;
}

bool win32_key_is_down(int virtual_key) noexcept {
    return (GetKeyState(virtual_key) & 0x8000) != 0 ||
           (GetAsyncKeyState(virtual_key) & 0x8000) != 0;
}

bool key_is_down(TextService::KeyStateReader reader,
                 void* context,
                 int virtual_key) noexcept {
    return reader ? reader(virtual_key, context) : win32_key_is_down(virtual_key);
}


struct RouteThreadSnapshot {
    DWORD process_id = 0;
    DWORD current_thread_id = 0;
    DWORD hook_target_thread_id = 0;
    DWORD foreground_window_thread_id = 0;
    DWORD focus_window_thread_id = 0;
};

DWORD thread_id_for_window(HWND hwnd) noexcept {
    return hwnd ? GetWindowThreadProcessId(hwnd, nullptr) : 0;
}

RouteThreadSnapshot current_route_thread_snapshot(DWORD hook_target_thread_id) noexcept {
    RouteThreadSnapshot snapshot{};
    snapshot.process_id = GetCurrentProcessId();
    snapshot.current_thread_id = GetCurrentThreadId();
    snapshot.hook_target_thread_id = hook_target_thread_id;
    snapshot.foreground_window_thread_id = thread_id_for_window(GetForegroundWindow());

    GUITHREADINFO gui_info{};
    gui_info.cbSize = sizeof(gui_info);
    if (GetGUIThreadInfo(0, &gui_info)) {
        snapshot.focus_window_thread_id = thread_id_for_window(gui_info.hwndFocus ? gui_info.hwndFocus : gui_info.hwndActive);
    }
    return snapshot;
}

const wchar_t* input_mode_text(InputMode mode) noexcept {
    return mode == InputMode::Chinese ? L"Chinese" : L"English";
}

bool is_ctrl_space_key(unsigned long key) noexcept {
    return key == VK_SPACE || is_ctrl_key(key);
}

ModifierKeyState current_modifier_key_state(TextService::KeyStateReader reader,
                                            void* context) {
    return modifier_key_state_from_readings(ModifierKeyReadings{
        key_is_down(reader, context, VK_CONTROL),
        key_is_down(reader, context, VK_LCONTROL),
        key_is_down(reader, context, VK_RCONTROL),
        key_is_down(reader, context, VK_SHIFT),
        key_is_down(reader, context, VK_LSHIFT),
        key_is_down(reader, context, VK_RSHIFT),
        key_is_down(reader, context, VK_MENU),
        key_is_down(reader, context, VK_LMENU),
        key_is_down(reader, context, VK_RMENU),
        key_is_down(reader, context, VK_LWIN),
        key_is_down(reader, context, VK_RWIN)
    });
}

struct CtrlSpaceRuntimeState {
    ModifierKeyState modifiers;
    bool generic_ctrl_down = false;
    bool left_ctrl_down = false;
    bool right_ctrl_down = false;
    bool space_down = false;
};

CtrlSpaceRuntimeState current_ctrl_space_runtime_state(TextService::KeyStateReader reader,
                                                       void* context) {
    CtrlSpaceRuntimeState state{};
    state.generic_ctrl_down = key_is_down(reader, context, VK_CONTROL);
    state.left_ctrl_down = key_is_down(reader, context, VK_LCONTROL);
    state.right_ctrl_down = key_is_down(reader, context, VK_RCONTROL);
    state.space_down = key_is_down(reader, context, VK_SPACE);
    state.modifiers = modifier_key_state_from_readings(ModifierKeyReadings{
        state.generic_ctrl_down,
        state.left_ctrl_down,
        state.right_ctrl_down,
        key_is_down(reader, context, VK_SHIFT),
        key_is_down(reader, context, VK_LSHIFT),
        key_is_down(reader, context, VK_RSHIFT),
        key_is_down(reader, context, VK_MENU),
        key_is_down(reader, context, VK_LMENU),
        key_is_down(reader, context, VK_RMENU),
        key_is_down(reader, context, VK_LWIN),
        key_is_down(reader, context, VK_RWIN)
    });
    return state;
}

std::wstring hook_lifecycle_message(const wchar_t* event_name,
                                    HHOOK hook,
                                    DWORD thread_id,
                                    bool result,
                                    DWORD win32_error,
                                    bool uninstall_failed) {
    const RouteThreadSnapshot threads = current_route_thread_snapshot(thread_id);
    std::wstringstream stream;
    stream << L"event=" << (event_name ? event_name : L"unknown")
           << L" hook_type=WH_KEYBOARD"
           << L" hook_scope=current_thread"
           << L" process_id=" << threads.process_id
           << L" current_thread_id=" << threads.current_thread_id
           << L" hook_thread_id=" << thread_id
           << L" hook_target_thread_id=" << threads.hook_target_thread_id
           << L" foreground_window_thread_id=" << threads.foreground_window_thread_id
           << L" focus_window_thread_id=" << threads.focus_window_thread_id
           << L" hook_handle=0x" << std::hex
           << reinterpret_cast<std::uintptr_t>(hook)
           << std::dec
           << L" result=" << bool_text(result)
           << L" win32_error=" << win32_error
           << L" uninstall_failed=" << bool_text(uninstall_failed);
    return stream.str();
}

std::wstring activation_lifecycle_message(const wchar_t* event_name,
                                          HHOOK hook,
                                          DWORD hook_target_thread_id,
                                          bool hook_installed,
                                          InputMode mode,
                                          bool runtime_active_before,
                                          bool runtime_active_after,
                                          bool pending_release,
                                          bool hook_uninstall_failed) {
    const RouteThreadSnapshot threads = current_route_thread_snapshot(hook_target_thread_id);
    std::wstringstream stream;
    stream << L"event=" << (event_name ? event_name : L"unknown")
           << L" process_id=" << threads.process_id
           << L" current_thread_id=" << threads.current_thread_id
           << L" hook_target_thread_id=" << threads.hook_target_thread_id
           << L" foreground_window_thread_id=" << threads.foreground_window_thread_id
           << L" focus_window_thread_id=" << threads.focus_window_thread_id
           << L" hook_handle=0x" << std::hex << reinterpret_cast<std::uintptr_t>(hook) << std::dec
           << L" hook_installed=" << bool_text(hook_installed)
           << L" input_mode=" << input_mode_text(mode)
           << L" runtime_status_active_before=" << bool_text(runtime_active_before)
           << L" runtime_status_active_after=" << bool_text(runtime_active_after)
           << L" pending_release=" << bool_text(pending_release)
           << L" hook_uninstall_failed=" << bool_text(hook_uninstall_failed);
    return stream.str();
}

void query_focus_context(ITfThreadMgr* thread_mgr,
                         ITfDocumentMgr** document_mgr,
                         ITfContext** context,
                         HRESULT* focus_hr,
                         HRESULT* top_hr) {
    if (document_mgr) {
        *document_mgr = nullptr;
    }
    if (context) {
        *context = nullptr;
    }
    if (focus_hr) {
        *focus_hr = E_POINTER;
    }
    if (top_hr) {
        *top_hr = E_POINTER;
    }
    if (!thread_mgr || !document_mgr) {
        return;
    }
    ITfDocumentMgr* focused = nullptr;
    const HRESULT get_focus_hr = thread_mgr->GetFocus(&focused);
    if (focus_hr) {
        *focus_hr = get_focus_hr;
    }
    if (FAILED(get_focus_hr) || !focused) {
        return;
    }
    *document_mgr = focused;
    if (context) {
        const HRESULT get_top_hr = focused->GetTop(context);
        if (top_hr) {
            *top_hr = get_top_hr;
        }
    }
}

std::uintptr_t document_mgr_identity_for_context(ITfContext* context) noexcept {
    if (!context) {
        return 0;
    }
    ITfDocumentMgr* document_mgr = nullptr;
    if (FAILED(context->GetDocumentMgr(&document_mgr)) || !document_mgr) {
        return 0;
    }
    const auto identity = reinterpret_cast<std::uintptr_t>(document_mgr);
    document_mgr->Release();
    return identity;
}

void read_open_close_compartment(ITfThreadMgr* thread_mgr, HRESULT* hr, bool* open) {
    if (hr) {
        *hr = E_POINTER;
    }
    if (open) {
        *open = false;
    }
    if (!thread_mgr) {
        return;
    }
    ITfCompartmentMgr* compartments = nullptr;
    HRESULT result = thread_mgr->GetGlobalCompartment(&compartments);
    if (FAILED(result) || !compartments) {
        if (hr) {
            *hr = result;
        }
        return;
    }
    ITfCompartment* compartment = nullptr;
    result = compartments->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &compartment);
    compartments->Release();
    if (FAILED(result) || !compartment) {
        if (hr) {
            *hr = result;
        }
        return;
    }
    VARIANT value;
    VariantInit(&value);
    result = compartment->GetValue(&value);
    if (SUCCEEDED(result) && open) {
        if (value.vt == VT_I4) {
            *open = value.lVal != 0;
        } else if (value.vt == VT_BOOL) {
            *open = value.boolVal != VARIANT_FALSE;
        }
    }
    VariantClear(&value);
    compartment->Release();
    if (hr) {
        *hr = result;
    }
}

}  // namespace

// A translation on its way: the background thread fills in the result and
// posts the serial to the candidate window, whose thread finishes it.
struct TranslationJob {
    unsigned long long serial = 0;
    std::wstring source;
    Translator translator;
    HWND target = nullptr;
    // Keeps this DLL loaded while the thread runs.
    HMODULE module = nullptr;
    std::mutex lock;
    std::optional<TranslationResult> result;
};

namespace {

void run_translation_job(const std::shared_ptr<TranslationJob>& job) {
    TranslationResult result;
    try {
        result = job->translator(job->source);
    } catch (...) {
        result = TranslationResult{false, {}, L"翻译失败。"};
    }
    {
        std::lock_guard<std::mutex> guard(job->lock);
        job->result = std::move(result);
    }
    PostMessageW(job->target, CandidateWindow::async_message(), static_cast<WPARAM>(job->serial), 0);
}

DWORD WINAPI translation_thread(void* parameter) {
    HMODULE module = nullptr;
    {
        std::unique_ptr<std::shared_ptr<TranslationJob>> job(static_cast<std::shared_ptr<TranslationJob>*>(parameter));
        module = (*job)->module;
        run_translation_job(*job);
    }
    FreeLibraryAndExitThread(module, 0);
}

}  // namespace

TextService::TextService() {
    dll_add_ref();
    candidate_window_.set_async_callback([this](WPARAM serial) { finish_translation(serial); });
    candidate_window_.set_copydata_callback([this](const COPYDATASTRUCT& data) { return accept_panel_message(data); });
    candidate_window_.set_selection_callback([this](size_t index) { accept_candidate_from_ui(index); });
    candidate_window_.set_page_callback([this](int direction) {
        if (candidate_context_) change_candidate_page(candidate_context_, direction);
    });
    candidate_ui_element_session_.set_selection_changed_callback([this](size_t index) {
        selected_index_ = index;
        refresh_candidate_window_from_host();
    });
    candidate_ui_element_session_.set_page_indices_changed_callback([this](const std::vector<UINT>& pages) {
        candidate_page_indices_ = pages;
        host_page_indices_ = true;
        refresh_candidate_window_from_host();
    });
    candidate_ui_element_session_.set_finalize_callback([this](size_t index) { accept_candidate_from_ui(index); });
    candidate_ui_element_session_.set_abort_callback([this]() {
        if (prediction_active_) dismiss_prediction(L"host_abort");
        else if (composition_context_) cancel_composition(composition_context_);
    });
    candidate_window_.set_theme_changed_callback([this]() { language_bar_.refresh_icon(); });
    candidate_window_.set_options_changed_callback([this]() {
        const auto options = settings_store_.load_input_mode_options();
        if (prediction_active_ && (!options.prediction_enabled || options.tab_translate))
            dismiss_prediction(L"prediction_disabled");
        else if (candidate_context_) update_candidate_window(candidate_context_);
    });
}

void TextService::set_key_state_reader_for_testing(KeyStateReader reader,
                                                   void* context) noexcept {
    key_state_reader_ = reader;
    key_state_reader_context_ = reader ? context : nullptr;
}

TextService::~TextService() {
    cancel_translation();
    candidate_window_.set_async_callback({});
    candidate_window_.set_copydata_callback({});
    for (int slot = 0; slot < CandidateWindow::kTaskSlots; ++slot) candidate_window_.cancel_task(slot);
    candidate_window_.set_selection_callback({});
    candidate_window_.set_page_callback({});
    candidate_ui_element_session_.set_selection_changed_callback({});
    candidate_ui_element_session_.set_page_indices_changed_callback({});
    candidate_window_.set_options_changed_callback({});
    candidate_window_.set_theme_changed_callback({});
    candidate_ui_element_session_.set_finalize_callback({});
    candidate_ui_element_session_.set_abort_callback({});
    unadvise_prediction_edits();
    unadvise_thread_events();
    if (prediction_caret_) prediction_caret_->Release();
    end_candidate_ui_element(L"teardown");
    clear_transient_contexts();
    language_bar_.clear_callbacks();
    language_bar_.unregister_item();
    uninstall_thread_keyboard_hook();
    uninstall_thread_mouse_hook();
    unpreserve_ctrl_space_key();
    unadvise_key_event_sink();
    if (thread_mgr_) {
        thread_mgr_->Release();
    }
    dll_release();
}

HRESULT TextService::QueryInterface(REFIID riid, void** object) {
    if (!object) {
        return E_POINTER;
    }
    *object = nullptr;
    if (riid == IID_IUnknown || riid == __uuidof(ITfTextInputProcessor) || riid == __uuidof(ITfTextInputProcessorEx)) {
        *object = static_cast<ITfTextInputProcessorEx*>(this);
    } else if (riid == __uuidof(ITfKeyEventSink)) {
        *object = static_cast<ITfKeyEventSink*>(this);
    } else if (riid == __uuidof(ITfCompositionSink)) {
        *object = static_cast<ITfCompositionSink*>(this);
    } else if (riid == __uuidof(ITfThreadMgrEventSink)) {
        *object = static_cast<ITfThreadMgrEventSink*>(this);
    } else if (riid == __uuidof(ITfTextEditSink)) {
        *object = static_cast<ITfTextEditSink*>(this);
    } else if (riid == __uuidof(ITfTextLayoutSink)) {
        *object = static_cast<ITfTextLayoutSink*>(this);
    } else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG TextService::AddRef() {
    return static_cast<ULONG>(InterlockedIncrement(&ref_count_));
}

ULONG TextService::Release() {
    const long count = InterlockedDecrement(&ref_count_);
    if (count == 0) {
        delete this;
    }
    return static_cast<ULONG>(count);
}

HRESULT TextService::Activate(ITfThreadMgr* thread_mgr, TfClientId client_id) {
    return ActivateEx(thread_mgr, client_id, 0);
}

HRESULT TextService::ActivateEx(ITfThreadMgr* thread_mgr, TfClientId client_id, DWORD flags) {
    if (!thread_mgr) {
        return E_POINTER;
    }
    ActivationFocusDiagnostics activation{};
    activation.text_service_activated = true;
    activation.activation_flags = flags;
    ITfThreadMgr2* thread_mgr2 = nullptr;
    activation.thread_mgr2_qi_hr = thread_mgr->QueryInterface(IID_PPV_ARGS(&thread_mgr2));
    activation.thread_mgr2_available = SUCCEEDED(activation.thread_mgr2_qi_hr) && thread_mgr2 != nullptr;
    if (activation.thread_mgr2_available) {
        activation.get_active_flags_called = true;
        activation.get_active_flags_hr = thread_mgr2->GetActiveFlags(&activation.active_flags);
        thread_mgr2->Release();
    }
    activation.thread_manager_identity = reinterpret_cast<std::uintptr_t>(thread_mgr);
    activation.client_id = client_id;
    activation.language_bar_update_requested = true;
    activation.language_bar_state = mode_ == InputMode::Chinese ? L"Chinese" : L"English";
    read_open_close_compartment(thread_mgr, &activation.compartment_read_hr,
                                &activation.compartment_open_close_state);
    log_status(L"ctrl-space",
               activation_lifecycle_message(L"activate_begin",
                                            keyboard_hook_,
                                            GetCurrentThreadId(),
                                            keyboard_hook_ != nullptr,
                                            mode_,
                                            false,
                                            false,
                                            ctrl_space_toggle_.pending_release,
                                            keyboard_hook_uninstall_failed_));
    thread_mgr_ = thread_mgr;
    thread_mgr_->AddRef();
    client_id_ = client_id;
    secure_mode_ = ((flags | activation.active_flags) & TF_TMAE_SECUREMODE) != 0;
    code_application_ = is_code_application(host_executable_name());
    language_bar_.set_toggle_callback([this](const wchar_t* source) {
        return toggle_input_mode_from_language_bar(source ? source : L"mode_icon");
    });
    language_bar_.set_settings_callback([this](const wchar_t* source) {
        return open_settings_from_language_bar(source ? source : L"mode_icon");
    });
    language_bar_.set_mode(mode_);
    const HRESULT langbar_hr = language_bar_.register_item(thread_mgr_);
    if (FAILED(langbar_hr)) {
        log_status(L"tsf", L"LanguageBar AddItem failed " + hresult_hex(langbar_hr));
    }
    language_bar_.publish_initial_mode();

    const HRESULT hr = advise_key_event_sink();
    activation.key_sink_advise_called = true;
    activation.key_sink_advise_hr = hr;
    activation.key_sink_advised = key_sink_advised_;
    HRESULT preserve_hr = S_OK;
    if (SUCCEEDED(hr)) {
        preserve_hr = preserve_ctrl_space_key();
    }
    const HRESULT result = FAILED(hr) ? hr : preserve_hr;
    if (SUCCEEDED(result)) {
        advise_thread_events();
        const bool hook_installed = install_thread_keyboard_hook();
        install_thread_mouse_hook();
        log_status(L"ctrl-space",
                   std::wstring(L"event=activate_hook_install_result hook_installed=") +
                       bool_text(hook_installed));
        publish_runtime_input_mode_status(L"activate", true);
    }
    log_status(L"ctrl-space",
               activation_lifecycle_message(L"activate_complete",
                                            keyboard_hook_,
                                            keyboard_hook_thread_id_ ? keyboard_hook_thread_id_ : GetCurrentThreadId(),
                                            keyboard_hook_ != nullptr,
                                            mode_,
                                            false,
                                            SUCCEEDED(result),
                                            ctrl_space_toggle_.pending_release,
                                            keyboard_hook_uninstall_failed_));
    log_status(L"tsf", FAILED(result) ? L"ActivateEx failed " + hresult_hex(result) : L"ActivateEx succeeded");
    log_activation_focus_diagnostics(activation);
    return result;
}

HRESULT TextService::Deactivate() {
    forget_committed_text();
    end_translation_run();
    cancel_translation();
    ActivationFocusDiagnostics deactivation{};
    deactivation.deactivation_started = true;
    deactivation.text_service_activated = false;
    deactivation.thread_manager_identity = reinterpret_cast<std::uintptr_t>(thread_mgr_);
    deactivation.client_id = client_id_;
    deactivation.key_sink_unadvise_called = key_sink_advised_;
    log_status(L"ctrl-space",
               activation_lifecycle_message(L"deactivate_begin",
                                            keyboard_hook_,
                                            keyboard_hook_thread_id_ ? keyboard_hook_thread_id_ : GetCurrentThreadId(),
                                            keyboard_hook_ != nullptr,
                                            mode_,
                                            true,
                                            true,
                                            ctrl_space_toggle_.pending_release,
                                            keyboard_hook_uninstall_failed_));
    uninstall_thread_keyboard_hook();
    uninstall_thread_mouse_hook();
    unadvise_thread_events();
    const HRESULT unpreserve_hr = unpreserve_ctrl_space_key();
    language_bar_.clear_callbacks();
    const HRESULT langbar_hr = language_bar_.unregister_item();
    const bool was_key_sink_advised = key_sink_advised_;
    unadvise_key_event_sink();
    deactivation.key_sink_unadvise_called = was_key_sink_advised;
    deactivation.key_sink_unadvise_hr = key_sink_advised_ ? E_FAIL : S_OK;
    reset_transient_input_state(L"deactivate");
    ctrl_space_toggle_ = CtrlSpaceToggleTracker{};
    ctrl_space_key_down_ = CtrlSpaceKeyDownState{};
    shift_toggle_ = ShiftToggleTracker{};
    has_last_caret_rect_ = false;
    if (thread_mgr_) {
        thread_mgr_->Release();
        thread_mgr_ = nullptr;
    }
    client_id_ = TF_CLIENTID_NULL;
    publish_runtime_input_mode_status(L"deactivate", false);
    log_status(L"ctrl-space",
               activation_lifecycle_message(L"deactivate_complete",
                                            keyboard_hook_,
                                            keyboard_hook_thread_id_,
                                            keyboard_hook_ != nullptr,
                                            mode_,
                                            true,
                                            false,
                                            ctrl_space_toggle_.pending_release,
                                            keyboard_hook_uninstall_failed_));
    log_status(L"tsf", L"Deactivate");
    log_activation_focus_diagnostics(deactivation);
    if (FAILED(unpreserve_hr)) {
        return unpreserve_hr;
    }
    return FAILED(langbar_hr) ? langbar_hr : S_OK;
}

HRESULT TextService::advise_key_event_sink() {
    if (!thread_mgr_ || key_sink_advised_) {
        return S_OK;
    }
    ITfKeystrokeMgr* keystroke_mgr = nullptr;
    HRESULT hr = thread_mgr_->QueryInterface(IID_PPV_ARGS(&keystroke_mgr));
    if (FAILED(hr)) {
        log_status(L"compatibility",
                   L"key_sink_advise_called=true key_sink_advise_hr=" + hresult_hex(hr) +
                       L" key_sink_advised=false");
        return hr;
    }
    hr = keystroke_mgr->AdviseKeyEventSink(client_id_, static_cast<ITfKeyEventSink*>(this), TRUE);
    if (SUCCEEDED(hr)) {
        key_sink_advised_ = true;
    }
    keystroke_mgr->Release();
    log_status(L"compatibility",
               L"key_sink_advise_called=true key_sink_advise_hr=" + hresult_hex(hr) +
                   L" key_sink_advised=" + bool_text(key_sink_advised_));
    return hr;
}

void TextService::unadvise_key_event_sink() {
    if (!thread_mgr_ || !key_sink_advised_) {
        return;
    }
    ITfKeystrokeMgr* keystroke_mgr = nullptr;
    HRESULT hr = E_NOINTERFACE;
    if (SUCCEEDED(thread_mgr_->QueryInterface(IID_PPV_ARGS(&keystroke_mgr)))) {
        hr = keystroke_mgr->UnadviseKeyEventSink(client_id_);
        keystroke_mgr->Release();
    }
    key_sink_advised_ = false;
    log_status(L"compatibility",
               L"key_sink_unadvise_called=true key_sink_unadvise_hr=" + hresult_hex(hr));
}

HRESULT TextService::preserve_ctrl_space_key() {
    if (!thread_mgr_ || ctrl_space_preserved_) {
        return S_OK;
    }

    ITfKeystrokeMgr* keystroke_mgr = nullptr;
    HRESULT hr = thread_mgr_->QueryInterface(IID_PPV_ARGS(&keystroke_mgr));
    if (FAILED(hr)) {
        log_status(L"tsf", L"PreserveKey QueryInterface failed " + hresult_hex(hr));
        return hr;
    }

    const TF_PRESERVEDKEY key = ctrl_space_preserved_key();
    const wchar_t* description = ctrl_space_preserved_key_description();
    hr = keystroke_mgr->PreserveKey(client_id_,
                                    GUID_LocalPinyinCtrlSpacePreservedKey,
                                    &key,
                                    description,
                                    static_cast<ULONG>(std::wcslen(description)));
    if (SUCCEEDED(hr)) {
        ctrl_space_preserved_ = true;
        log_status(L"tsf", L"PreserveKey Ctrl+Space succeeded");
    } else {
        log_status(L"tsf", L"PreserveKey Ctrl+Space failed " + hresult_hex(hr));
    }
    keystroke_mgr->Release();
    return hr;
}

HRESULT TextService::unpreserve_ctrl_space_key() {
    if (!thread_mgr_ || !ctrl_space_preserved_) {
        return S_OK;
    }

    ITfKeystrokeMgr* keystroke_mgr = nullptr;
    HRESULT hr = thread_mgr_->QueryInterface(IID_PPV_ARGS(&keystroke_mgr));
    if (FAILED(hr)) {
        log_status(L"tsf", L"UnpreserveKey QueryInterface failed " + hresult_hex(hr));
        return hr;
    }

    const TF_PRESERVEDKEY key = ctrl_space_preserved_key();
    hr = keystroke_mgr->UnpreserveKey(GUID_LocalPinyinCtrlSpacePreservedKey, &key);
    if (SUCCEEDED(hr)) {
        ctrl_space_preserved_ = false;
        log_status(L"tsf", L"UnpreserveKey Ctrl+Space succeeded");
    } else {
        log_status(L"tsf", L"UnpreserveKey Ctrl+Space failed " + hresult_hex(hr));
    }
    keystroke_mgr->Release();
    return hr;
}

LRESULT CALLBACK TextService::thread_mouse_hook_proc(int code, WPARAM wparam, LPARAM lparam) {
    if (code == HC_ACTION && g_thread_mouse_hook_service) {
        switch (wparam) {
            case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN: case WM_XBUTTONDOWN:
            case WM_NCLBUTTONDOWN: case WM_NCRBUTTONDOWN: case WM_NCMBUTTONDOWN: case WM_NCXBUTTONDOWN: {
                const auto* info = reinterpret_cast<const MOUSEHOOKSTRUCT*>(lparam);
                g_thread_mouse_hook_service->note_mouse_press(info ? info->hwnd : nullptr);
                break;
            }
            default:
                break;
        }
    }
    return CallNextHookEx(nullptr, code, wparam, lparam);
}

void TextService::install_thread_mouse_hook() {
    if (mouse_hook_) return;
    mouse_hook_ = SetWindowsHookExW(WH_MOUSE, thread_mouse_hook_proc, nullptr, GetCurrentThreadId());
    if (mouse_hook_) g_thread_mouse_hook_service = this;
}

void TextService::uninstall_thread_mouse_hook() {
    if (mouse_hook_) UnhookWindowsHookEx(mouse_hook_);
    mouse_hook_ = nullptr;
    if (g_thread_mouse_hook_service == this) g_thread_mouse_hook_service = nullptr;
}

void TextService::note_mouse_press(HWND window) {
    // Clicking a candidate in the popup is typing, not moving the caret.
    if (window && window == candidate_window_.window()) return;
    end_translation_run();
    cancel_translation();
}

LRESULT CALLBACK TextService::thread_keyboard_hook_proc(int code, WPARAM wparam, LPARAM lparam) {
    if (code == HC_ACTION && g_thread_keyboard_hook_service) {
        if (g_thread_keyboard_hook_service->handle_thread_keyboard_hook(code, static_cast<unsigned long>(wparam), lparam)) {
            return 1;
        }
    }
    return CallNextHookEx(g_thread_keyboard_hook_service ? g_thread_keyboard_hook_service->keyboard_hook_ : nullptr,
                          code,
                          wparam,
                          lparam);
}

bool TextService::install_thread_keyboard_hook() {
    const DWORD thread_id = GetCurrentThreadId();
    log_status(L"ctrl-space",
               hook_lifecycle_message(L"hook_install_attempt",
                                      keyboard_hook_,
                                      thread_id,
                                      false,
                                      0,
                                      keyboard_hook_uninstall_failed_));
    if (keyboard_hook_) {
        if (!g_thread_keyboard_hook_service) {
            g_thread_keyboard_hook_service = this;
        }
        log_status(L"ctrl-space",
                   hook_lifecycle_message(L"hook_install_existing",
                                          keyboard_hook_,
                                          thread_id,
                                          true,
                                          0,
                                          keyboard_hook_uninstall_failed_));
        return true;
    }
    GUITHREADINFO gui_info{};
    gui_info.cbSize = sizeof(gui_info);
    if (!GetGUIThreadInfo(thread_id, &gui_info)) {
        const DWORD error = GetLastError();
        log_status(L"ctrl-space",
                   hook_lifecycle_message(L"hook_install_failed_no_gui_thread",
                                          nullptr,
                                          thread_id,
                                          false,
                                          error,
                                          keyboard_hook_uninstall_failed_));
        return false;
    }
    if (g_thread_keyboard_hook_service && g_thread_keyboard_hook_service != this) {
        log_status(L"ctrl-space",
                   hook_lifecycle_message(L"hook_install_failed_thread_already_has_service",
                                          nullptr,
                                          thread_id,
                                          false,
                                          ERROR_ALREADY_EXISTS,
                                          keyboard_hook_uninstall_failed_));
        return false;
    }
    SetLastError(ERROR_SUCCESS);
    keyboard_hook_ = SetWindowsHookExW(WH_KEYBOARD,
                                       &TextService::thread_keyboard_hook_proc,
                                       nullptr,
                                       thread_id);
    if (!keyboard_hook_) {
        const DWORD error = GetLastError();
        log_status(L"ctrl-space",
                   hook_lifecycle_message(L"hook_install_failed",
                                          nullptr,
                                          thread_id,
                                          false,
                                          error,
                                          keyboard_hook_uninstall_failed_));
        return false;
    }
    g_thread_keyboard_hook_service = this;
    keyboard_hook_thread_id_ = thread_id;
    keyboard_hook_uninstall_failed_ = false;
    log_status(L"ctrl-space",
               hook_lifecycle_message(L"hook_install_success",
                                      keyboard_hook_,
                                      thread_id,
                                      true,
                                      0,
                                      false));
    return true;
}

void TextService::uninstall_thread_keyboard_hook() {
    if (!keyboard_hook_) {
        log_status(L"ctrl-space",
                   hook_lifecycle_message(L"hook_uninstall_skipped_no_hook",
                                          nullptr,
                                          GetCurrentThreadId(),
                                          true,
                                          0,
                                          keyboard_hook_uninstall_failed_));
        return;
    }
    const HHOOK hook = keyboard_hook_;
    const DWORD thread_id = GetCurrentThreadId();
    log_status(L"ctrl-space",
               hook_lifecycle_message(L"hook_uninstall_attempt",
                                      hook,
                                      thread_id,
                                      false,
                                      0,
                                      keyboard_hook_uninstall_failed_));
    SetLastError(ERROR_SUCCESS);
    if (!UnhookWindowsHookEx(hook)) {
        const DWORD error = GetLastError();
        keyboard_hook_uninstall_failed_ = true;
        if (g_thread_keyboard_hook_service == this) {
            g_thread_keyboard_hook_service = nullptr;
        }
        log_status(L"ctrl-space",
                   hook_lifecycle_message(L"hook_uninstall_failed",
                                          hook,
                                          keyboard_hook_thread_id_ ? keyboard_hook_thread_id_ : thread_id,
                                          false,
                                          error,
                                          true));
        return;
    }
    keyboard_hook_ = nullptr;
    keyboard_hook_uninstall_failed_ = false;
    const DWORD hook_thread_id = keyboard_hook_thread_id_ ? keyboard_hook_thread_id_ : thread_id;
    keyboard_hook_thread_id_ = 0;
    if (g_thread_keyboard_hook_service == this) {
        g_thread_keyboard_hook_service = nullptr;
    }
    log_status(L"ctrl-space",
               hook_lifecycle_message(L"hook_uninstall_success",
                                      hook,
                                      hook_thread_id,
                                      true,
                                      0,
                                      false));
}

bool TextService::handle_thread_keyboard_hook(int, unsigned long key, LPARAM lparam) {
    const bool key_up = (static_cast<unsigned long long>(lparam) & 0x80000000ULL) != 0;
    if (!is_ctrl_space_key(key)) {
        return false;
    }
    if (key_up) {
        if (is_ctrl_key(key)) {
            handle_ctrl_space_ctrl_keyup_latch(key, lparam, L"thread_keyboard_hook");
        }
        if (key == VK_SPACE) {
            handle_ctrl_space_keyup_fallback(nullptr, key, lparam, L"thread_keyboard_hook", nullptr);
        }
        const CtrlSpaceRuntimeState state =
            current_ctrl_space_runtime_state(key_state_reader_, key_state_reader_context_);
        ctrl_space_toggle_on_key_up(ctrl_space_toggle_, key, state.modifiers.ctrl, state.space_down);
        return false;
    }

    const CtrlSpaceRuntimeState state =
        current_ctrl_space_runtime_state(key_state_reader_, key_state_reader_context_);
    ctrl_space_chord_on_key_down(ctrl_space_toggle_, key);
    const bool chord_matched = is_ctrl_space(key, state.modifiers);
    if (!thread_mgr_ || client_id_ == TF_CLIENTID_NULL) {
        return false;
    }
    if (!chord_matched) {
        return false;
    }

    handle_ctrl_space_toggle(nullptr, L"ctrl_space_thread_keyboard_hook");
    return true;
}

bool TextService::handle_ctrl_space_ctrl_keyup_latch(unsigned long key,
                                                     LPARAM,
                                                     const wchar_t*) {
    const CtrlSpaceRuntimeState state =
        current_ctrl_space_runtime_state(key_state_reader_, key_state_reader_context_);
    const CtrlSpaceCtrlKeyUpLatchDecision decision =
        ctrl_space_chord_on_ctrl_key_up(ctrl_space_toggle_, key, state.modifiers, state.space_down);
    return decision.allowed;
}

bool TextService::handle_ctrl_space_keyup_fallback(ITfContext* context,
                                                   unsigned long key,
                                                   LPARAM,
                                                   const wchar_t*,
                                                   bool* toggled) {
    if (toggled) {
        *toggled = false;
    }
    const CtrlSpaceRuntimeState state =
        current_ctrl_space_runtime_state(key_state_reader_, key_state_reader_context_);
    const CtrlSpaceKeyUpFallbackDecision decision =
        ctrl_space_keyup_fallback_decision(ctrl_space_toggle_, key, state.modifiers);
    if (!decision.allowed) {
        return false;
    }

    const bool did_toggle = handle_ctrl_space_toggle(context, L"ctrl_space_keyup_fallback");
    if (toggled) {
        *toggled = did_toggle;
    }
    return did_toggle;
}

HRESULT TextService::OnSetFocus(BOOL foreground) {
    if (!foreground && prediction_active_) dismiss_prediction(L"focus_lost");
    forget_committed_text();
    end_translation_run();
    cancel_translation();
    ITfDocumentMgr* focus_document_mgr = nullptr;
    ITfContext* top_context = nullptr;
    HRESULT focus_hr = E_UNEXPECTED;
    HRESULT top_hr = E_UNEXPECTED;
    query_focus_context(thread_mgr_, &focus_document_mgr, &top_context, &focus_hr, &top_hr);
    ActivationFocusDiagnostics focus{};
    focus.thread_focus_set_received = foreground != FALSE;
    focus.thread_focus_kill_received = foreground == FALSE;
    focus.document_focus_changed = true;
    focus.old_document_mgr_identity = document_mgr_identity_for_context(composition_context_);
    focus.new_document_mgr_identity = reinterpret_cast<std::uintptr_t>(focus_document_mgr);
    focus.document_mgr_identity = focus.new_document_mgr_identity;
    focus.context_stack_top_hr = top_hr;
    focus.context_stack_top_present = top_context != nullptr;
    focus.context_identity = reinterpret_cast<std::uintptr_t>(top_context);
    read_open_close_compartment(thread_mgr_, &focus.compartment_read_hr,
                                &focus.compartment_open_close_state);
    std::wstringstream stream;
    stream << L"focus_change_received=true foreground=" << bool_text(foreground != FALSE)
           << L" composition_active=" << bool_text(!composing_.empty())
           << L" candidate_active=" << bool_text(!candidates_.empty())
           << L" pending_edit_session=false"
           << L" composition_context_identity="
           << reinterpret_cast<std::uintptr_t>(composition_context_)
           << L" candidate_context_identity="
           << reinterpret_cast<std::uintptr_t>(candidate_context_)
           << L" old_document_mgr_identity=" << focus.old_document_mgr_identity
           << L" new_document_mgr_identity=" << focus.new_document_mgr_identity
           << L" context_stack_top_hr=" << hresult_hex(top_hr)
           << L" context_stack_top_present=" << bool_text(top_context != nullptr)
           << L" context_identity=" << reinterpret_cast<std::uintptr_t>(top_context);
    log_status(L"compatibility", stream.str());
    log_activation_focus_diagnostics(focus);
    if (prediction_active_ && top_context != composition_context_)
        dismiss_prediction(L"document_focus_changed");
    if (top_context) {
        top_context->Release();
    }
    if (focus_document_mgr) {
        focus_document_mgr->Release();
    }
    return S_OK;
}

HRESULT TextService::OnTestKeyDown(ITfContext* context, WPARAM wparam, LPARAM lparam, BOOL* eaten) {
    if (!eaten) {
        return E_POINTER;
    }
    *eaten = FALSE;
    key_repeat_ = (lparam & (1 << 30)) != 0;
    const HRESULT hr = test_key_down(context, wparam, eaten);
    if (SUCCEEDED(hr) && !*eaten && composing_.empty()) note_key_left_to_application(static_cast<unsigned long>(wparam), lparam);
    return hr;
}

void TextService::note_key_left_to_application(unsigned long key, LPARAM lparam) {
    if (is_modifier_key(key) || key == noted_key_) return;
    noted_key_ = key;
    // Bit 30: the key was already down, a repeat of a held key.
    if ((lparam & (1 << 30)) != 0) {
        end_translation_run();
        end_phrase_run();
    }
    note_unhandled_key(key);
}

HRESULT TextService::test_key_down(ITfContext* context, WPARAM wparam, BOOL* eaten) {
    prediction_passthrough_key_ = 0;
    prepare_context_for_key(context);
    const CtrlSpaceRuntimeState ctrl_space_state =
        current_ctrl_space_runtime_state(key_state_reader_, key_state_reader_context_);
    const ModifierKeyState modifiers = ctrl_space_state.modifiers;
    const unsigned long key = static_cast<unsigned long>(wparam);
    if (wparam != tested_key_) {
        previous_key_digit_ = tested_key_digit_;
        previous_key_letter_ = tested_key_letter_;
        tested_key_ = wparam;
    }
    tested_key_digit_ = composing_.empty() && !modifiers.shift &&
                        ((wparam >= L'0' && wparam <= L'9') || (wparam >= VK_NUMPAD0 && wparam <= VK_NUMPAD9));
    // A letter that goes into the document as typed (CapsLock, Shift), not
    // into a pinyin composition.
    tested_key_letter_ = composing_.empty() && wparam >= L'A' && wparam <= L'Z' && letter_is_literal();
    const bool ctrl_space_relevant_key = is_ctrl_space_key(key);
    if (ctrl_space_relevant_key) {
        ctrl_space_chord_on_key_down(ctrl_space_toggle_, key);
    }
    if (is_ctrl_space(key, modifiers)) {
        ctrl_space_key_down_.accepted_by_test_key_down = true;
        handle_ctrl_space_toggle(context, L"ctrl_space_test_key_down");
        *eaten = TRUE;
        return S_OK;
    }
    ctrl_space_key_down_.accepted_by_test_key_down = false;
    if (!is_modifier_key(key)) cancel_translation();
    if (prediction_active_) {
        if (!prediction_still_valid(context)) dismiss_prediction(L"prediction_caret_changed");
        else if ((wparam == VK_LEFT || wparam == VK_RIGHT) && candidate_page_indices_.size() > 1 &&
                 !modifiers.shift && !modifiers.ctrl && !modifiers.alt && !modifiers.win) {
            *eaten = TRUE; return S_OK;
        } else if (wparam == VK_TAB && !modifiers.shift && !modifiers.ctrl && !modifiers.alt && !modifiers.win) {
            *eaten = TRUE; return S_OK;
        } else if (wparam == VK_ESCAPE && !modifiers.ctrl && !modifiers.alt && !modifiers.win) {
            *eaten = TRUE; return S_OK;
        } else {
            dismiss_prediction(L"ordinary_editing_key");
            // Digits, whitespace, arrows and punctuation keep their ordinary
            // host meaning while dismissing suggestions. Letters start pinyin.
            if (!(wparam >= L'A' && wparam <= L'Z') &&
                !can_start_shift_toggle(key, modifiers, shift_toggle_enabled())) {
                prediction_passthrough_key_ = key; return S_OK;
            }
        }
    }
    if (wparam == VK_TAB && !modifiers.shift && !modifiers.ctrl && !modifiers.alt && !modifiers.win &&
        mode_ == InputMode::Chinese) {
        if (pass_next_tab_ || key_repeat_) return S_OK;
        const auto options = settings_store_.load_input_mode_options();
        if (options.panel_double_tab) {
            *eaten = TRUE;
            return S_OK;
        }
        // While typing pinyin Tab commits it first, then translates.
        if (options.tab_translate && (!composing_.empty() || !translation_source(context).text.empty())) {
            *eaten = TRUE;
            return S_OK;
        }
    }
    if (!composing_.empty() && tool_key_character(wparam, modifiers.shift)) {
        *eaten = TRUE;
        return S_OK;
    }
    if (can_start_shift_toggle(static_cast<unsigned long>(wparam), modifiers, shift_toggle_enabled())) {
        *eaten = TRUE;
        return S_OK;
    }
    if (mode_ == InputMode::English || should_pass_through(wparam)) {
        return S_OK;
    }
    if (code_punctuation_for_key(wparam, modifiers.shift)) {
        // With nothing being typed the code editor types the key itself and
        // keeps its own bracket pairing; during pinyin the IME commits first.
        *eaten = composing_.empty() ? FALSE : TRUE;
        return S_OK;
    }
    if (chinese_punctuation_for_key(wparam, modifiers.shift, !composing_.empty(),
                                    opening_double_quote_, opening_single_quote_)) {
        *eaten = TRUE;
        return S_OK;
    }
    if (wparam >= L'1' && wparam <= L'9') {
        *eaten = can_select_candidate_by_digit(key, current_candidate_page_count(), !composing_.empty(),
            candidate_window_.is_visible() || candidate_ui_element_session_.host_managed());
        return S_OK;
    }
    if (wparam == VK_SPACE || wparam == VK_RETURN) {
        *eaten = should_eat_composition_commit_key(key, candidates_.size(), !composing_.empty());
        return S_OK;
    }
    if (wparam == VK_BACK) {
        *eaten = should_eat_backspace_for_composition(mode_, !composing_.empty(), false);
        return S_OK;
    }
    if (wparam >= L'A' && wparam <= L'Z') {
        // With CapsLock or Shift, a letter outside a composition goes straight
        // to the application, which types it in upper case.
        *eaten = !composing_.empty() || !letter_is_literal();
    } else if ((wparam == VK_OEM_7 && !composing_.empty()) || (wparam == VK_ESCAPE && !composing_.empty())) {
        *eaten = TRUE;
    } else if (wparam == VK_LEFT || wparam == VK_RIGHT) {
        *eaten = should_eat_candidate_navigation_key(
            static_cast<unsigned long>(wparam),
            !composing_.empty(),
            candidates_.size(),
            transient_context_matches(context)) ? TRUE : FALSE;
    }
    return S_OK;
}

HRESULT TextService::OnKeyDown(ITfContext* context, WPARAM wparam, LPARAM lparam, BOOL* eaten) {
    if (!eaten) {
        return E_POINTER;
    }
    *eaten = FALSE;
    key_repeat_ = (lparam & (1 << 30)) != 0;
    // A key while a single Tab waits: the Tab's tab character goes first.
    const unsigned long pressed = static_cast<unsigned long>(wparam);
    if (tab_tap_.pending && pressed != VK_TAB && !is_modifier_key(pressed)) {
        const bool replay = tab_tap_.replay;
        tab_tap_ = TabTap{};
        candidate_window_.cancel_task(kTabTask);
        translation_hold_until_ = 0;
        if (replay && context) composition_.insert_text(context, client_id_, L"\t");
    }
    const HRESULT hr = handle_key_down(context, wparam, eaten);
    if (SUCCEEDED(hr) && !*eaten && composing_.empty()) note_key_left_to_application(static_cast<unsigned long>(wparam), lparam);
    return hr;
}

HRESULT TextService::handle_key_down(ITfContext* context, WPARAM wparam, BOOL* eaten) {
    const auto passthrough_key = prediction_passthrough_key_;
    prediction_passthrough_key_ = 0;
    if (passthrough_key == static_cast<unsigned long>(wparam) &&
        !is_ctrl_space(static_cast<unsigned long>(wparam),
            current_modifier_key_state(key_state_reader_, key_state_reader_context_))) {
        return S_OK;
    }
    // The Tab this service sent after a single tap belongs to the
    // application. Documents that skip the key test (RichEdit) bring it here.
    if (wparam == VK_TAB && pass_next_tab_) {
        pass_next_tab_ = false;
        return S_OK;
    }
    prepare_context_for_key(context);

    const CtrlSpaceRuntimeState ctrl_space_state =
        current_ctrl_space_runtime_state(key_state_reader_, key_state_reader_context_);
    const ModifierKeyState modifiers = ctrl_space_state.modifiers;
    const bool shift_enabled = shift_toggle_enabled();
    const unsigned long key = static_cast<unsigned long>(wparam);
    if (!is_modifier_key(key)) cancel_translation();
    if (prediction_active_) {
        if (!prediction_still_valid(context)) dismiss_prediction(L"prediction_caret_changed");
        else if ((key == VK_LEFT || key == VK_RIGHT) && candidate_page_indices_.size() > 1 &&
                 !modifiers.shift && !modifiers.ctrl && !modifiers.alt && !modifiers.win) {
            change_candidate_page(context, key == VK_LEFT ? -1 : 1);
            *eaten = TRUE; return S_OK;
        } else if (key == VK_TAB && !modifiers.shift && !modifiers.ctrl && !modifiers.alt && !modifiers.win) {
            const HRESULT hr = commit_prediction(context);
            *eaten = SUCCEEDED(hr) && hr != S_FALSE; return S_OK;
        } else if (key == VK_ESCAPE && !modifiers.ctrl && !modifiers.alt && !modifiers.win) {
            dismiss_prediction(L"prediction_escape"); *eaten = TRUE; return S_OK;
        } else {
            dismiss_prediction(L"ordinary_editing_key");
            if (!(key >= L'A' && key <= L'Z') && !is_ctrl_space(key, modifiers) &&
                !can_start_shift_toggle(key, modifiers, shift_enabled)) return S_OK;
        }
    }
    if (key == VK_TAB && !modifiers.shift && !modifiers.ctrl && !modifiers.alt && !modifiers.win &&
        mode_ == InputMode::Chinese && !pass_next_tab_ && !key_repeat_) {
        const auto options = settings_store_.load_input_mode_options();
        const ULONGLONG now = GetTickCount64();
        if (options.panel_double_tab && tab_tap_.pending && now - tab_tap_.tick <= kDoubleTabMs) {
            // The second Tab: the first one's translation or tab is dropped.
            tab_tap_ = TabTap{};
            candidate_window_.cancel_task(kTabTask);
            translation_hold_until_ = 0;
            cancel_translation();
            open_panel(context);
            *eaten = TRUE;
            return S_OK;
        }
        bool handled = false;
        if (options.tab_translate) {
            const bool was_typing = !composing_.empty();
            if (was_typing) {
                const HRESULT hr = commit_whole_composition(context);
                if (FAILED(hr)) return hr;
            }
            const auto source = translation_source(context);
            if (!source.text.empty()) {
                start_translation(context, source);
                if (options.panel_double_tab) translation_hold_until_ = now + kDoubleTabMs;
            }
            handled = was_typing || !source.text.empty();
        }
        if (options.panel_double_tab) {
            tab_tap_ = TabTap{true, now, !handled};
            candidate_window_.schedule_task(kTabTask, static_cast<UINT>(kDoubleTabMs), [this]() { finish_tab_tap(); });
            *eaten = TRUE;
            return S_OK;
        }
        *eaten = handled ? TRUE : FALSE;
        if (*eaten) return S_OK;
    }
    if (is_ctrl_space_key(key)) {
        ctrl_space_chord_on_key_down(ctrl_space_toggle_, key);
    }

    const bool ctrl_space_key_down = should_handle_ctrl_space_key_down(
        key,
        modifiers,
        ctrl_space_key_down_);
    if (ctrl_space_key_down) {
        ctrl_space_key_down_.accepted_by_test_key_down = false;
        const wchar_t* reason = ctrl_space_preserved_ ? L"ctrl_space_keydown" : L"ctrl_space_fallback";
        handle_ctrl_space_toggle(context, reason);
        *eaten = TRUE;
        return S_OK;
    }
    ctrl_space_key_down_.accepted_by_test_key_down = false;

    shift_toggle_on_key_down(shift_toggle_, static_cast<unsigned long>(wparam), modifiers, shift_enabled);
    if (can_start_shift_toggle(static_cast<unsigned long>(wparam), modifiers, shift_enabled)) {
        *eaten = TRUE;
        return S_OK;
    }

    if (mode_ == InputMode::English || should_pass_through(wparam)) {
        return S_OK;
    }
    handle_chinese_key(context, wparam, eaten);
    return S_OK;
}

HRESULT TextService::OnTestKeyUp(ITfContext*, WPARAM wparam, LPARAM, BOOL* eaten) {
    if (!eaten) {
        return E_POINTER;
    }
    if (wparam == tested_key_) tested_key_ = 0;
    if (static_cast<unsigned long>(wparam) == noted_key_) noted_key_ = 0;
    if (wparam == VK_TAB && pass_next_tab_ && !tab_tap_.pending) pass_next_tab_ = false;
    *eaten = is_shift_key(static_cast<unsigned long>(wparam)) && shift_toggle_.pending ? TRUE : FALSE;
    return S_OK;
}

HRESULT TextService::OnKeyUp(ITfContext* context, WPARAM wparam, LPARAM lparam, BOOL* eaten) {
    if (!eaten) {
        return E_POINTER;
    }
    *eaten = FALSE;
    if (wparam == VK_TAB && pass_next_tab_ && !tab_tap_.pending) pass_next_tab_ = false;
    const unsigned long key = static_cast<unsigned long>(wparam);
    if (key == noted_key_) noted_key_ = 0;
    ctrl_space_key_down_.accepted_by_test_key_down = false;
    const CtrlSpaceRuntimeState ctrl_space_state =
        current_ctrl_space_runtime_state(key_state_reader_, key_state_reader_context_);
    const ModifierKeyState modifiers = ctrl_space_state.modifiers;
    if (is_ctrl_key(key)) {
        handle_ctrl_space_ctrl_keyup_latch(key, lparam, L"OnKeyUp");
    }
    if (key == VK_SPACE) {
        if (handle_ctrl_space_keyup_fallback(context, key, lparam, L"OnKeyUp", nullptr)) {
            *eaten = TRUE;
        }
    }
    if (shift_toggle_on_key_up(shift_toggle_,
                               key,
                               modifiers,
                               shift_toggle_enabled())) {
        toggle_input_mode(context, L"shift_toggle");
        *eaten = TRUE;
    }
    if (is_ctrl_space_key(key)) {
        ctrl_space_toggle_on_key_up(ctrl_space_toggle_, key, ctrl_space_state.modifiers.ctrl, ctrl_space_state.space_down);
    }
    return S_OK;
}

HRESULT TextService::OnPreservedKey(ITfContext* context, REFGUID guid, BOOL* eaten) {
    if (!eaten) {
        return E_POINTER;
    }
    *eaten = FALSE;
    if (preserved_key_action_for_guid(guid) == PreservedKeyAction::ToggleInputMode) {
        ctrl_space_key_down_.accepted_by_test_key_down = false;
        handle_ctrl_space_toggle(context, L"ctrl_space_preserved_key");
        *eaten = TRUE;
    }
    return S_OK;
}

HRESULT TextService::OnCompositionTerminated(TfEditCookie, ITfComposition*) {
    reset_transient_input_state(L"composition_terminated");
    return S_OK;
}

void TextService::advise_thread_events() {
    if (!thread_mgr_ || thread_event_cookie_ != TF_INVALID_COOKIE) return;
    ITfSource* source = nullptr;
    if (SUCCEEDED(thread_mgr_->QueryInterface(IID_PPV_ARGS(&source)))) {
        source->AdviseSink(__uuidof(ITfThreadMgrEventSink), static_cast<ITfThreadMgrEventSink*>(this), &thread_event_cookie_);
        source->Release();
    }
}

void TextService::unadvise_thread_events() {
    if (!thread_mgr_ || thread_event_cookie_ == TF_INVALID_COOKIE) return;
    ITfSource* source = nullptr;
    if (SUCCEEDED(thread_mgr_->QueryInterface(IID_PPV_ARGS(&source)))) {
        source->UnadviseSink(thread_event_cookie_); source->Release();
    }
    thread_event_cookie_ = TF_INVALID_COOKIE;
}

void TextService::unadvise_prediction_edits() {
    if (!composition_context_ || prediction_edit_cookie_ == TF_INVALID_COOKIE) return;
    ITfSource* source = nullptr;
    if (SUCCEEDED(composition_context_->QueryInterface(IID_PPV_ARGS(&source)))) {
        source->UnadviseSink(prediction_edit_cookie_); source->Release();
    }
    prediction_edit_cookie_ = TF_INVALID_COOKIE;
}

HRESULT TextService::OnInitDocumentMgr(ITfDocumentMgr*) { return S_OK; }
HRESULT TextService::OnUninitDocumentMgr(ITfDocumentMgr*) {
    if (prediction_active_) dismiss_prediction(L"document_closed");
    return S_OK;
}
HRESULT TextService::OnSetFocus(ITfDocumentMgr* focus, ITfDocumentMgr*) {
    forget_committed_text();
    end_phrase_run();
    end_translation_run();
    cancel_translation();
    if (prediction_active_) {
        ITfDocumentMgr* prediction_document = nullptr;
        if (!composition_context_ || FAILED(composition_context_->GetDocumentMgr(&prediction_document)) ||
            prediction_document != focus) dismiss_prediction(L"document_focus_changed");
        if (prediction_document) prediction_document->Release();
    }
    return S_OK;
}
HRESULT TextService::OnPushContext(ITfContext* context) {
    if (prediction_active_ && context != composition_context_) dismiss_prediction(L"context_pushed");
    return S_OK;
}
HRESULT TextService::OnPopContext(ITfContext*) {
    if (prediction_active_) dismiss_prediction(L"context_popped");
    return S_OK;
}
HRESULT TextService::OnLayoutChange(ITfContext* context, TfLayoutCode code, ITfContextView*) {
    if (context && context == candidate_context_ && code != TF_LC_DESTROY) schedule_anchor_follow(0);
    return S_OK;
}

HRESULT TextService::OnEndEdit(ITfContext* context, TfEditCookie, ITfEditRecord* record) {
    if (!prediction_active_ || context != composition_context_ || !record) return S_OK;
    BOOL changed = FALSE;
    if (FAILED(record->GetSelectionStatus(&changed)) || changed) {
        dismiss_prediction(L"selection_changed"); return S_OK;
    }
    IEnumTfRanges* ranges = nullptr;
    if (FAILED(record->GetTextAndPropertyUpdates(TF_GTP_INCL_TEXT, nullptr, 0, &ranges))) {
        dismiss_prediction(L"document_changed"); return S_OK;
    }
    if (ranges) {
        ITfRange* range = nullptr; ULONG fetched = 0;
        const HRESULT hr = ranges->Next(1, &range, &fetched);
        if (range) range->Release();
        ranges->Release();
        if (FAILED(hr) || fetched) dismiss_prediction(L"document_changed");
    }
    return S_OK;
}

bool TextService::should_pass_through(WPARAM wparam) const {
    const ModifierKeyState modifiers =
        current_modifier_key_state(key_state_reader_, key_state_reader_context_);
    if (modifiers.alt || modifiers.win) {
        return true;
    }
    if (modifiers.ctrl && wparam != VK_SPACE) {
        return true;
    }
    return false;
}

bool TextService::shift_toggle_enabled() const {
    return settings_store_.load_input_mode_options().shift_toggle_enabled;
}

bool TextService::handle_ctrl_space_toggle(ITfContext* context, const wchar_t* reason) {
    if (!ctrl_space_toggle_should_fire(ctrl_space_toggle_, true)) {
        return false;
    }
    toggle_input_mode(context, reason);
    return true;
}

void TextService::publish_runtime_input_mode_status(const wchar_t* source, bool active) {
    RuntimeInputModeStatus status;
    status.schema_version = 1;
    status.active = active;
    status.mode = mode_;
    status.source = source ? source : L"unknown";
    status.process_id = GetCurrentProcessId();
    status.thread_id = GetCurrentThreadId();
    status.sequence = ++runtime_status_sequence_;
    status.updated_at_utc_ms = runtime_status_current_utc_ms();
    status.available = active;
    if (!settings_store_.save_runtime_input_mode_status(status)) {
        log_status(L"mode", L"runtime_status write failed");
    }
}

HRESULT TextService::toggle_input_mode(ITfContext* context, const wchar_t* reason) {
    end_phrase_run();
    if (prediction_active_) dismiss_prediction(reason ? reason : L"mode_toggle");
    if (!composing_.empty()) {
        // Letters typed before noticing the Chinese mode stay as typed
        // (hello stays hello) instead of vanishing with the composition.
        ITfContext* target = context ? context : composition_context_;
        if (!target || FAILED(commit_raw_composition(target))) cancel_composition(context);
    } else {
        candidates_.clear();
        selected_index_ = 0;
        has_last_caret_rect_ = false;
        end_candidate_ui_element(reason ? reason : L"mode_toggle");
        candidate_window_.hide(reason);
    }

    mode_ = toggled_input_mode(mode_);
    language_bar_.set_mode(mode_);
    publish_runtime_input_mode_status(reason, true);
    std::wstringstream stream;
    stream << (mode_ == InputMode::Chinese ? L"Chinese" : L"English")
           << L" reason=" << (reason ? reason : L"unknown");
    log_status(L"mode", stream.str());
    if (mode_ == InputMode::Chinese) {
        update_candidate_window(context);
    }
    return S_OK;
}

HRESULT TextService::toggle_input_mode_from_language_bar(const wchar_t* reason) {
    log_status(L"language-bar", L"event=mode_icon_toggle_enter");
    return toggle_input_mode(nullptr, reason ? reason : L"mode_icon_toggle");
}

HRESULT TextService::open_settings_from_language_bar(const wchar_t* source) {
    const std::wstring settings_path = install_file_path(g_instance, L"LocalPinyinSettings.exe");
    const std::wstring source_text = source ? source : L"unknown";
    if (settings_path.empty()) {
        log_status(L"language-bar", L"event=open_settings source=" + source_text + L" path= result=not_found");
        return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
    }
    const DWORD attributes = GetFileAttributesW(settings_path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        const DWORD error = attributes == INVALID_FILE_ATTRIBUTES ? GetLastError() : ERROR_FILE_NOT_FOUND;
        log_status(L"language-bar",
                   L"event=open_settings source=" + source_text +
                       L" path=" + settings_path +
                       L" result=missing error=" + std::to_wstring(error));
        return HRESULT_FROM_WIN32(error);
    }

    std::wstring command_line = L"\"" + settings_path + L"\"";
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(settings_path.c_str(),
                        command_line.data(),
                        nullptr,
                        nullptr,
                        FALSE,
                        0,
                        nullptr,
                        nullptr,
                        &startup,
                        &process)) {
        const DWORD error = GetLastError();
        log_status(L"language-bar",
                   L"event=open_settings source=" + source_text +
                       L" path=" + settings_path +
                       L" result=failed error=" + std::to_wstring(error));
        return HRESULT_FROM_WIN32(error);
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    log_status(L"language-bar", L"event=open_settings source=" + source_text + L" path=" + settings_path + L" result=S_OK");
    return S_OK;
}

bool TextService::handle_chinese_key(ITfContext* context, WPARAM wparam, BOOL* eaten) {
    if (!context || !eaten) {
        return false;
    }

    const auto modifiers = current_modifier_key_state(key_state_reader_, key_state_reader_context_);
    if (const wchar_t tool = tool_key_character(wparam, modifiers.shift); tool && !composing_.empty()) {
        composing_.push_back(tool);
        selected_index_ = 0;
        refresh_candidates();
        update_composition(context);
        *eaten = TRUE;
        return true;
    }
    if (const wchar_t ascii = code_punctuation_for_key(wparam, modifiers.shift)) {
        if (composing_.empty()) return false;
        const HRESULT hr = commit_punctuation(context, ascii);
        if (FAILED(hr)) log_status(L"composition", L"punctuation failed " + hresult_hex(hr));
        *eaten = TRUE;
        return true;
    }
    wchar_t punctuation = chinese_punctuation_for_key(wparam, modifiers.shift, !composing_.empty(),
        opening_double_quote_, opening_single_quote_);
    // Helpers (辅助功能): after a digit the period (3.14) and , / : (1,000,
    // 1/2, 12:30) are ASCII; after an English letter every punctuation key is
    // (Hello, world.). Each has its own switch.
    if (composing_.empty() && punctuation) {
        const auto options = settings_store_.load_input_mode_options();
        const bool digit_rule = (punctuation == L'。' && options.digit_period) ||
                                (is_digit_punctuation(punctuation) && options.digit_punctuation);
        if (digit_rule || options.letter_punctuation) {
            const auto neighbor = neighbor_before_caret(context);
            if ((digit_rule && neighbor.digit) || (options.letter_punctuation && neighbor.letter)) {
                if (const wchar_t ascii = ascii_punctuation_for_key(wparam, modifiers.shift)) punctuation = ascii;
            }
        }
    }
    if (punctuation) {
        const HRESULT hr = commit_punctuation(context, punctuation);
        if (FAILED(hr)) log_status(L"composition", L"punctuation failed " + hresult_hex(hr));
        *eaten = TRUE;
        return true;
    }

    if (wparam >= L'A' && wparam <= L'Z') {
        const bool literal = letter_is_literal();
        if (literal && composing_.empty()) return false;
        if (composing_.empty()) {
            engine_.refresh_user_lexicon_if_needed(false);
            set_composition_context(context);
            preceding_committed_text_.clear();
            if (!secure_mode_) {
                const HRESULT read = composition_.read_preceding_text(context, client_id_, &preceding_committed_text_);
                if (read != S_OK) preceding_committed_text_.clear();
                // S_FALSE marks a protected or read-only field: no fallback there.
                if (FAILED(read) || (read == S_OK && preceding_committed_text_.empty()))
                    preceding_committed_text_ = recent_committed_text(context);
            }
        }
        if (FAILED(make_composition_room(context))) {
            *eaten = FALSE;
            return false;
        }
        // Inside a composition a literal letter keeps its case (Shift XOR
        // CapsLock); the composition is then committed as typed.
        const bool upper = literal && (modifiers.shift != caps_lock_on(key_state_reader_, key_state_reader_context_));
        composing_.push_back(upper ? static_cast<wchar_t>(wparam)
                                   : static_cast<wchar_t>(std::towlower(static_cast<wint_t>(wparam))));
        selected_index_ = 0;
        refresh_candidates();
        update_composition(context);
        *eaten = TRUE;
        return true;
    }

    if (wparam == VK_OEM_7 && !composing_.empty()) {
        if (FAILED(make_composition_room(context))) {
            *eaten = FALSE;
            return false;
        }
        // A separator at an automatically committed boundary is already fulfilled.
        if (composing_.empty()) { *eaten = TRUE; return true; }
        composing_.push_back(L'\'');
        selected_index_ = 0;
        refresh_candidates();
        update_composition(context);
        *eaten = TRUE;
        return true;
    }

    if (wparam == VK_BACK) {
        if (composing_.empty()) {
            return false;
        }
        composing_.pop_back();
        selected_index_ = 0;
        if (composing_.empty()) {
            cancel_composition(context);
        } else {
            refresh_candidates();
            update_composition(context);
        }
        *eaten = TRUE;
        return true;
    }

    if (wparam == VK_ESCAPE) {
        if (!composing_.empty()) {
            cancel_composition(context);
            *eaten = TRUE;
            return true;
        }
        return false;
    }

    if (wparam == VK_RETURN) {
        if (composing_.empty()) {
            return false;
        }
        commit_raw_composition(context);
        *eaten = TRUE;
        return true;
    }

    if (wparam == VK_SPACE) {
        if (candidate_ui_element_session_.host_managed())
            selected_index_ = candidate_ui_element_session_.selected_index();
        const int commit_index = candidate_commit_index_for_key(static_cast<unsigned long>(wparam), candidates_.size(), !composing_.empty(), selected_index_);
        if (commit_index >= 0) {
            selected_index_ = static_cast<size_t>(commit_index);
            commit_selected(context);
            *eaten = TRUE;
            return true;
        }
        return false;
    }

    if (wparam >= L'1' && wparam <= L'9') {
        if (can_select_candidate_by_digit(static_cast<unsigned long>(wparam), current_candidate_page_count(), !composing_.empty(),
            candidate_window_.is_visible() || candidate_ui_element_session_.host_managed())) {
            const size_t index = static_cast<size_t>(candidate_index_from_digit_key(static_cast<unsigned long>(wparam)));
            selected_index_ = (candidate_page_indices_.empty() ? 0 : candidate_page_indices_[current_candidate_page()]) + index;
            commit_selected(context);
            *eaten = TRUE;
            return true;
        }
        return false;
    }

    if (should_eat_candidate_navigation_key(
            static_cast<unsigned long>(wparam), !composing_.empty(), candidates_.size(),
            transient_context_matches(context)) && wparam == VK_LEFT) {
        change_candidate_page(context, -1);
        *eaten = TRUE;
        return true;
    }

    if (should_eat_candidate_navigation_key(
            static_cast<unsigned long>(wparam), !composing_.empty(), candidates_.size(),
            transient_context_matches(context)) && wparam == VK_RIGHT) {
        change_candidate_page(context, 1);
        *eaten = TRUE;
        return true;
    }

    return false;
}

void TextService::refresh_candidates() {
    candidate_page_indices_.clear();
    host_page_indices_ = false;
    selected_index_ = 0;
    engine_.refresh_user_lexicon_if_needed(!composing_.empty());
    // Fuzzy pinyin, corrections, mixed input and double pinyin follow the
    // saved options on every key, like the candidate window's appearance.
    const auto options = settings_store_.load_input_mode_options().engine;
    engine_.set_input_options(options);
    if (composition_has_literal_letters()) {
        // Shift-typed English: the text as typed first (Space commits it as
        // before), and with mixed input on, the pinyin before it converted
        // as the second choice (woyong + Python -> 我用 Python).
        candidates_ = {Candidate{composing_, 1, 0}};
        const auto first_upper = std::find_if(composing_.begin(), composing_.end(),
                                              [](wchar_t ch) { return ch >= L'A' && ch <= L'Z'; }) - composing_.begin();
        if (options.mixed_english && options.double_pinyin == DoublePinyinScheme::None && first_upper > 0) {
            const auto head = engine_.lookup(composing_.substr(0, static_cast<size_t>(first_upper)), preceding_committed_text_);
            if (!head.empty() && !head.front().consumed_pinyin &&
                std::any_of(head.front().text.begin(), head.front().text.end(), [](wchar_t ch) { return ch >= 0x3400; })) {
                candidates_.push_back(Candidate{head.front().text + L' ' + composing_.substr(static_cast<size_t>(first_upper)), 2, 0});
            }
        }
        return;
    }
    candidates_ = engine_.lookup(composing_, preceding_committed_text_);
}

bool TextService::letter_is_literal() const noexcept {
    const auto modifiers = current_modifier_key_state(key_state_reader_, key_state_reader_context_);
    return modifiers.shift || caps_lock_on(key_state_reader_, key_state_reader_context_);
}

bool TextService::composition_has_literal_letters() const noexcept {
    return std::any_of(composing_.begin(), composing_.end(), [](wchar_t ch) { return ch >= L'A' && ch <= L'Z'; });
}

size_t TextService::current_candidate_page() const noexcept {
    if (candidate_page_indices_.empty()) return 0;
    const auto after = std::upper_bound(candidate_page_indices_.begin(), candidate_page_indices_.end(), selected_index_);
    return after == candidate_page_indices_.begin() ? 0 : static_cast<size_t>(after - candidate_page_indices_.begin() - 1);
}

size_t TextService::current_candidate_page_count() const noexcept {
    if (candidate_page_indices_.empty()) return candidates_.size();
    const size_t page = current_candidate_page();
    const size_t end = page + 1 < candidate_page_indices_.size() ? candidate_page_indices_[page + 1] : candidates_.size();
    return end - candidate_page_indices_[page];
}

void TextService::change_candidate_page(ITfContext* context, int direction) {
    if (candidate_page_indices_.empty() || !transient_context_matches(context)) return;
    const size_t page = current_candidate_page();
    const size_t next = direction < 0 ? (page > 0 ? page - 1 : 0) :
        (std::min)(page + 1, candidate_page_indices_.size() - 1);
    if (next != page) selected_index_ = candidate_page_indices_[next];
    update_candidate_window(context);
}

HRESULT TextService::update_composition(ITfContext* context) {
    HRESULT hr = composition_.set_text(context, client_id_, static_cast<ITfCompositionSink*>(this), composing_);
    if (FAILED(hr)) {
        log_status(L"composition", L"set_text failed " + hresult_hex(hr));
        return hr;
    }
    update_candidate_window(context);
    return S_OK;
}

void TextService::update_candidate_window(ITfContext* context) {
    candidate_window_.set_prediction_mode(prediction_active_);
    if (prediction_active_ && !settings_store_.load_input_mode_options().prediction_enabled) {
        dismiss_prediction(L"prediction_disabled"); return;
    }
    const std::wstring display_label = candidate_display_label();
    const bool logical_show_requested =
        mode_ == InputMode::Chinese && !display_label.empty() && !candidates_.empty();
    if (logical_show_requested) {
        if (candidate_context_ != context) {
            end_candidate_ui_element(L"candidate_context_changed");
            set_candidate_context(context);
            candidate_layout_retry_attempts_ = 0;
            has_last_caret_rect_ = false;
        }
    } else {
        candidate_window_.cancel_layout_retry();
        candidate_layout_retry_attempts_ = 0;
        end_candidate_ui_element(L"candidate_model_inactive");
        set_candidate_context(nullptr);
    }

    RECT anchor_rect{};
    bool anchor_available = false;
    if (logical_show_requested) {
        bool layout_pending = false;
        anchor_available = candidate_anchor_rect(context, &anchor_rect, &layout_pending);
        // The app is still laying out the text just typed: a popup that is
        // not up yet waits briefly for its layout notification instead of
        // appearing at a fallback caret (the retry below shows it at the
        // fallback if the app stays silent).
        if (anchor_available && layout_pending && !candidate_window_.is_visible() &&
            candidate_layout_retry_attempts_ == 0) {
            anchor_available = false;
        }
    }
    // The page model exists before host UI negotiation and without a caret HWND.
    RECT layout_anchor = anchor_rect;
    const CandidateWindowOwnerInfo layout_owner = candidate_owner_info(context);
    if (!anchor_available) {
        if (has_last_caret_rect_) layout_anchor = last_caret_rect_;
        else if (layout_owner.context_valid) GetWindowRect(layout_owner.context_hwnd, &layout_anchor);
        else layout_anchor = RECT{0, 0, 1, 20};
    }
    if (logical_show_requested) {
        const auto layout = candidate_window_.prepare_layout(display_label, candidates_, selected_index_,
            layout_anchor, layout_owner, host_page_indices_ ? candidate_page_indices_ : std::vector<UINT>{});
        candidate_page_indices_ = layout.page_indices;
    }

    CandidateModelSnapshot snapshot =
        make_candidate_model_snapshot(display_label, candidates_, selected_index_, context);
    snapshot.page_indices = candidate_page_indices_;
    if (logical_show_requested) {
        HRESULT ui_element_hr = S_OK;
        if (!candidate_ui_element_session_.active()) {
            ui_element_hr = candidate_ui_element_session_.begin(thread_mgr_, snapshot);
        } else {
            ui_element_hr = candidate_ui_element_session_.update(snapshot);
        }
        if (FAILED(ui_element_hr)) {
            log_status(L"candidate-ui", L"ui_element_begin_or_update_failed hr=" + hresult_hex(ui_element_hr));
        }
    }

    candidate_ui_element_session_.set_anchor_available(anchor_available);

    const CandidateDisplayDecision decision = decide_candidate_display(CandidateDisplayState{
        mode_ == InputMode::English,
        !display_label.empty(),
        candidates_.size(),
        anchor_available
    });

    if (logical_show_requested && candidate_ui_element_session_.active() &&
        candidate_ui_element_session_.host_managed()) {
        candidate_layout_retry_attempts_ = 0;
        candidate_window_.cancel_layout_retry();
        candidate_window_.hide(L"host_managed_ui_element");
        candidate_ui_element_session_.set_self_draw_enabled(false);
        candidate_ui_element_session_.set_window_visible(false);
        candidate_ui_element_session_.set_presentation(
            CandidatePresentationMode::HostManagedUIElement,
            CandidatePresentationFallbackReason::None,
            false);
        release_candidate_model_snapshot(snapshot);
        return;
    }

    if (!decision.request_show) {
        const std::wstring reason = candidate_hide_reason_to_string(decision.hide_reason);
        candidate_window_.hide(reason.c_str());
        candidate_ui_element_session_.set_self_draw_enabled(true);
        candidate_ui_element_session_.set_window_visible(false);
        const bool ui_element_fallback_failed =
            !candidate_ui_element_session_.active() &&
            candidate_ui_element_session_.diagnostics().ui_element_begin_called &&
            FAILED(candidate_ui_element_session_.diagnostics().ui_element_begin_hr);
        const auto fallback_reason = ui_element_fallback_failed
            ? CandidatePresentationFallbackReason::BeginFailed
            : (candidate_ui_element_session_.diagnostics().ui_element_manager_available
                   ? CandidatePresentationFallbackReason::NoSelfDrawAnchor
                   : CandidatePresentationFallbackReason::UIElementManagerUnavailable);
        candidate_ui_element_session_.set_presentation(
            CandidatePresentationMode::None,
            logical_show_requested ? fallback_reason : CandidatePresentationFallbackReason::ModelInactive,
            logical_show_requested);
        const CandidateLayoutRetryDecision retry = decide_candidate_layout_retry(
            logical_show_requested, anchor_available, candidate_layout_retry_attempts_);
        if (retry.schedule && candidate_context_) {
            candidate_layout_retry_attempts_ = retry.next_attempt;
            candidate_window_.schedule_layout_retry(
                [this]() {
                    if (candidate_context_ && mode_ == InputMode::Chinese &&
                        (!composing_.empty() || prediction_active_) && !candidates_.empty()) {
                        update_candidate_window(candidate_context_);
                    }
                },
                retry.next_attempt,
                retry.delay_ms);
        }
        release_candidate_model_snapshot(snapshot);
        return;
    }
    candidate_layout_retry_attempts_ = 0;
    const CandidateWindowOwnerInfo owner_info = candidate_owner_info(context);
    const bool appearing = !candidate_window_.is_visible();
    candidate_window_.show(display_label, candidates_, selected_index_, anchor_rect, owner_info, candidate_page_indices_);
    if (appearing && candidate_window_.is_visible()) {
        anchor_follow_step_ = 0;
        schedule_anchor_follow(kAnchorFollowDelaysMs[0]);
    }
    candidate_ui_element_session_.set_self_draw_enabled(true);
    candidate_ui_element_session_.set_window_visible(candidate_window_.is_visible());
    const bool begin_failed =
        candidate_ui_element_session_.diagnostics().ui_element_begin_called &&
        FAILED(candidate_ui_element_session_.diagnostics().ui_element_begin_hr);
    const auto show_fallback_reason =
        candidate_ui_element_session_.active()
            ? CandidatePresentationFallbackReason::None
            : (begin_failed ? CandidatePresentationFallbackReason::BeginFailed
                            : CandidatePresentationFallbackReason::UIElementManagerUnavailable);
    candidate_ui_element_session_.set_presentation(
        CandidatePresentationMode::SelfDrawnHwnd,
        show_fallback_reason,
        false);
    release_candidate_model_snapshot(snapshot);
}

void TextService::refresh_candidate_window_from_host() {
    // A host can set page boundaries/selection synchronously during Begin or
    // UpdateUIElement. Notify only our HWND here; calling the manager again
    // would recursively negotiate the same UI element with that host.
    if (!candidate_ui_element_session_.active() || candidate_ui_element_session_.host_managed() ||
        !candidate_window_.is_visible() || !candidate_context_) return;
    RECT anchor{};
    if (!candidate_anchor_rect(candidate_context_, &anchor)) return;
    candidate_window_.show(candidate_display_label(), candidates_, selected_index_, anchor,
        candidate_owner_info(candidate_context_), candidate_page_indices_);
}

std::wstring TextService::candidate_display_label() {
    return prediction_active_ ? L"联想 · Tab" :
        composition_has_literal_letters() ? composing_ : engine_.display_input(composing_);
}

void TextService::schedule_anchor_follow(UINT delay_ms) {
    candidate_window_.schedule_anchor_check([this]() {
        follow_candidate_anchor();
        if (++anchor_follow_step_ < kAnchorFollowDelaysMs.size() && candidate_window_.is_visible())
            schedule_anchor_follow(kAnchorFollowDelaysMs[anchor_follow_step_]);
    }, delay_ms);
}

void TextService::follow_candidate_anchor() {
    if (!candidate_context_ || mode_ != InputMode::Chinese || candidates_.empty() ||
        (composing_.empty() && !prediction_active_)) return;
    if (candidate_ui_element_session_.active() && candidate_ui_element_session_.host_managed()) return;
    // Still waiting for a first layout: show it now that there is one.
    if (!candidate_window_.is_visible()) {
        update_candidate_window(candidate_context_);
        return;
    }
    RECT anchor{};
    if (!candidate_anchor_rect(candidate_context_, &anchor) || EqualRect(&anchor, &candidate_window_.anchor_rect())) return;
    log_status(L"candidate-ui", L"anchor_followed left=" + std::to_wstring(anchor.left) + L" top=" + std::to_wstring(anchor.top) +
        L" from_left=" + std::to_wstring(candidate_window_.anchor_rect().left) +
        L" from_top=" + std::to_wstring(candidate_window_.anchor_rect().top));
    candidate_window_.show(candidate_display_label(), candidates_, selected_index_, anchor,
        candidate_owner_info(candidate_context_), candidate_page_indices_);
}

void TextService::end_candidate_ui_element(const wchar_t* reason) {
    if (!candidate_ui_element_session_.active()) {
        return;
    }
    const HRESULT hr = candidate_ui_element_session_.end();
    if (FAILED(hr)) {
        log_status(L"candidate-ui",
                   L"ui_element_end_failed reason=" +
                       std::wstring(reason ? reason : L"unknown") +
                       L" hr=" + hresult_hex(hr));
    }
}

void TextService::dismiss_prediction(const wchar_t* reason) {
    if (prediction_active_) reset_transient_input_state(reason);
}

bool TextService::prediction_still_valid(ITfContext* context) {
    if (!prediction_active_ || !context || context != composition_context_ || secure_mode_ ||
        !settings_store_.load_input_mode_options().prediction_enabled || !prediction_caret_) return false;
    std::wstring before;
    return composition_.read_preceding_text(context, client_id_, &before, nullptr, prediction_caret_) == S_OK &&
           before == prediction_before_;
}

void TextService::start_prediction(ITfContext* context) {
    if (!context || secure_mode_ || mode_ != InputMode::Chinese || composition_.active() || !composing_.empty()) return;
    // With Tab to English on, Tab belongs to translation: no suggestions.
    const auto options = settings_store_.load_input_mode_options();
    if (!options.prediction_enabled || options.tab_translate) return;
    std::wstring before; ITfRange* caret = nullptr;
    if (composition_.read_preceding_text(context, client_id_, &before, &caret) != S_OK) return;
    auto predictions = engine_.predict(before);
    if (predictions.empty()) { if (caret) caret->Release(); return; }
    // Host-managed UI still needs a hidden top-level HWND to receive the
    // existing settings broadcast, so disabling predictions closes it now.
    candidate_window_.create();
    set_composition_context(context);
    ITfSource* source = nullptr;
    HRESULT hr = context->QueryInterface(IID_PPV_ARGS(&source));
    if (SUCCEEDED(hr)) {
        hr = source->AdviseSink(__uuidof(ITfTextEditSink), static_cast<ITfTextEditSink*>(this), &prediction_edit_cookie_);
        source->Release();
    }
    if (FAILED(hr)) {
        if (caret) caret->Release();
        reset_transient_input_state(L"prediction_monitor_unavailable"); return;
    }
    prediction_caret_ = caret;
    prediction_before_ = std::move(before);
    candidate_page_indices_.clear();
    host_page_indices_ = false;
    prediction_active_ = true;
    candidates_ = std::move(predictions);
    selected_index_ = 0;
    update_candidate_window(context);
}

HRESULT TextService::commit_prediction(ITfContext* context) {
    if (!prediction_still_valid(context) || candidates_.empty()) {
        dismiss_prediction(L"prediction_stale"); return S_FALSE;
    }
    if (candidate_ui_element_session_.host_managed())
        selected_index_ = candidate_ui_element_session_.selected_index();
    const std::wstring extension = candidates_[std::min(selected_index_, candidates_.size() - 1)].text;
    context->AddRef();
    // Close the UI and its edit monitor before inserting. Accepted extensions
    // are never treated as typed pinyin, and the surrounding document is never
    // sent to the persistent learning store.
    dismiss_prediction(L"prediction_accepted");
    const HRESULT hr = composition_.insert_text(context, client_id_, extension);
    if (SUCCEEDED(hr)) {
        remember_committed_text(context, extension);
        start_prediction(context);
    }
    context->Release();
    return hr;
}

void TextService::accept_candidate_from_ui(size_t index) {
    if (mode_ != InputMode::Chinese || !candidate_context_ || index >= candidates_.size() ||
        (!prediction_active_ && (composing_.empty() || !composition_.active()))) return;
    ITfContext* context = candidate_context_;
    context->AddRef();
    selected_index_ = index;
    const HRESULT hr = prediction_active_ ? commit_prediction(context) : commit_selected(context);
    if (FAILED(hr)) log_status(L"candidate-ui", L"candidate accept failed " + hresult_hex(hr));
    context->Release();
}

HRESULT TextService::commit_selected(ITfContext* context) {
    if (candidates_.empty()) {
        return S_FALSE;
    }
    selected_index_ = std::min(selected_index_, candidates_.size() - 1);
    const auto selected = candidates_[selected_index_];
    const std::wstring committed = selected.text;
    const size_t consumed = selected.consumed_pinyin
        ? std::min(selected.consumed_pinyin, composing_.size()) : composing_.size();
    const std::wstring learned_pinyin = composing_.substr(0, consumed);
    std::wstring suffix = composing_.substr(consumed);
    const std::wstring preceding = preceding_committed_text_ + committed;
    while (!suffix.empty() && suffix.front() == L'\'') suffix.erase(0, 1);
    const HRESULT hr = composition_.commit_text(context, client_id_, committed);
    if (FAILED(hr)) {
        log_status(L"composition", L"commit failed " + hresult_hex(hr));
        if (!composition_.active()) {
            reset_transient_input_state(L"commit_end_failed_after_clear");
        }
        return hr;
    }
    engine_.learn(learned_pinyin, committed);
    extend_phrase_run(context, learned_pinyin, committed, preceding_committed_text_);
    remember_committed_text(context, committed);
    reset_transient_input_state(L"commit");
    if (!suffix.empty()) {
        set_composition_context(context);
        preceding_committed_text_ = preceding.substr(preceding.size() > 128 ? preceding.size() - 128 : 0);
        composing_ = std::move(suffix);
        refresh_candidates();
        return update_composition(context);
    }
    if (!suppress_prediction_ && std::any_of(committed.begin(), committed.end(),
        [](wchar_t ch) { return ch >= 0x3400 && ch <= 0x9FFF; })) start_prediction(context);
    return S_OK;
}

HRESULT TextService::make_composition_room(ITfContext* context) {
    if (composing_.size() < 256) return S_OK;
    const size_t before = composing_.size();
    const Candidate* selected = candidates_.empty() ? nullptr
        : &candidates_[std::min(selected_index_, candidates_.size() - 1)];
    const bool raw = !selected || std::any_of(selected->text.begin(), selected->text.end(),
        [](wchar_t ch) { return ch >= L'a' && ch <= L'z'; });
    const std::wstring committed = raw ? composing_ : selected->text;
    const std::wstring preceding = preceding_committed_text_ + committed;
    suppress_prediction_ = true;
    const HRESULT hr = raw ? commit_raw_composition(context) : commit_selected(context);
    suppress_prediction_ = false;
    if (FAILED(hr)) return hr;
    if (composing_.size() >= before) return E_UNEXPECTED;
    if (composing_.empty()) {
        set_composition_context(context);
        preceding_committed_text_ = preceding.substr(preceding.size() > 128 ? preceding.size() - 128 : 0);
    }
    return S_OK;
}

wchar_t TextService::code_punctuation_for_key(WPARAM wparam, bool shift) const {
    // An unshifted apostrophe still separates pinyin syllables.
    if (!code_application_ || (wparam == VK_OEM_7 && !shift && !composing_.empty())) return 0;
    const wchar_t ascii = ascii_punctuation_for_key(wparam, shift);
    return ascii && settings_store_.load_input_mode_options().code_punctuation ? ascii : 0;
}

TextService::CaretNeighbor TextService::neighbor_before_caret(ITfContext* context) {
    if (!secure_mode_) {
        std::wstring before;
        if (composition_.read_preceding_text(context, client_id_, &before) == S_OK && !before.empty()) {
            const wchar_t last = before.back();
            return {last >= L'0' && last <= L'9', (last >= L'a' && last <= L'z') || (last >= L'A' && last <= L'Z')};
        }
    }
    // The document cannot be read: go by the key typed before.
    return {previous_key_digit_, previous_key_letter_};
}

void TextService::extend_phrase_run(ITfContext* context, const std::wstring& typed, const std::wstring& committed,
                                    const std::wstring& preceding) {
    constexpr ULONGLONG kGapMs = 20000;
    constexpr size_t kMaxCharacters = 8;
    const bool chinese = !committed.empty() && std::all_of(committed.begin(), committed.end(),
                                                           [](wchar_t ch) { return ch >= 0x3400 && ch <= 0x9FFF; });
    if (secure_mode_ || !chinese || typed.empty()) {
        end_phrase_run();
        return;
    }
    const ULONGLONG now = GetTickCount64();
    const auto identity = reinterpret_cast<std::uintptr_t>(context);
    bool continues = phrase_run_.pieces > 0 && phrase_run_.context == identity && now - phrase_run_.last_tick <= kGapMs &&
                     phrase_run_.text.size() + committed.size() <= kMaxCharacters;
    // The document must still end with the run and this piece: the caret
    // did not move and nothing else was typed in between.
    if (continues) {
        std::wstring before;
        if (composition_.read_preceding_text(context, client_id_, &before) == S_OK && !before.empty()) {
            const std::wstring expected = phrase_run_.text + committed;
            continues = before.size() >= expected.size() &&
                        before.compare(before.size() - expected.size(), expected.size(), expected) == 0;
        }
    }
    if (!continues) {
        phrase_run_ = PhraseRun{typed, committed, preceding, 1, now, identity};
        return;
    }
    phrase_run_.typed += typed;
    phrase_run_.text += committed;
    ++phrase_run_.pieces;
    phrase_run_.last_tick = now;
    if (engine_.learn_phrase(phrase_run_.typed, phrase_run_.text, phrase_run_.preceding))
        log_status(L"learning", L"phrase built from " + std::to_wstring(phrase_run_.pieces) + L" pieces");
}

HRESULT TextService::commit_whole_composition(ITfContext* context) {
    // A prefix candidate may leave an unresolved suffix: commit until nothing
    // of the typed pinyin is left.
    for (size_t attempt = 0; !composing_.empty() && attempt < 256; ++attempt) {
        const size_t before = composing_.size();
        suppress_prediction_ = true;
        const HRESULT hr = candidates_.empty() ? commit_raw_composition(context) : commit_selected(context);
        suppress_prediction_ = false;
        if (FAILED(hr)) return hr;
        if (!composing_.empty() && composing_.size() >= before) return E_UNEXPECTED;
    }
    return composing_.empty() ? S_OK : E_UNEXPECTED;
}

HRESULT TextService::commit_punctuation(ITfContext* context, wchar_t punctuation) {
    end_phrase_run();
    // Finish the typed pinyin before placing punctuation so the document
    // never loses its remainder.
    const HRESULT committed = commit_whole_composition(context);
    if (FAILED(committed)) return committed;
    const HRESULT hr = composition_.insert_text(context, client_id_, std::wstring(1, punctuation));
    if (SUCCEEDED(hr)) {
        remember_committed_text(context, std::wstring(1, punctuation));
        if (punctuation == L'“' || punctuation == L'”') opening_double_quote_ = !opening_double_quote_;
        if (punctuation == L'‘' || punctuation == L'’') opening_single_quote_ = !opening_single_quote_;
    }
    return hr;
}

HRESULT TextService::commit_raw_composition(ITfContext* context) {
    if (composing_.empty()) {
        return S_FALSE;
    }
    end_phrase_run();
    const std::wstring committed = composing_;
    const HRESULT hr = composition_.commit_text(context, client_id_, committed);
    if (FAILED(hr)) {
        log_status(L"composition", L"commit raw failed " + hresult_hex(hr));
        if (!composition_.active()) {
            reset_transient_input_state(L"commit_raw_end_failed_after_clear");
        }
        return hr;
    }
    remember_committed_text(context, committed);
    reset_transient_input_state(L"commit_raw");
    return S_OK;
}

void TextService::remember_committed_text(ITfContext* context, const std::wstring& text) {
    if (secure_mode_ || !context || text.empty()) return;
    extend_translation_run(context, text);
    const auto identity = reinterpret_cast<std::uintptr_t>(context);
    if (identity != recent_committed_context_ || GetTickCount64() - recent_committed_tick_ > kRecentCommitLifetimeMs)
        recent_committed_text_.clear();
    recent_committed_context_ = identity;
    recent_committed_tick_ = GetTickCount64();
    recent_committed_text_ += text;
    if (recent_committed_text_.size() > 32) recent_committed_text_.erase(0, recent_committed_text_.size() - 32);
}

void TextService::note_unhandled_key(unsigned long key) {
    // Any key the application handles itself (a space, Enter, an arrow,
    // Backspace) ends a phrase being built; modifier keys do not.
    switch (key) {
        case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT: case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
        case VK_MENU: case VK_LMENU: case VK_RMENU: case VK_CAPITAL:
            break;
        default:
            end_phrase_run();
            break;
    }
    if (!translation_run_.text.empty() && !is_modifier_key(key)) {
        const auto modifiers = current_modifier_key_state(key_state_reader_, key_state_reader_context_);
        const bool typed = key == VK_SPACE || (key >= L'0' && key <= L'9') || (key >= L'A' && key <= L'Z') ||
                           (key >= VK_OEM_1 && key <= VK_OEM_8);
        if (key == VK_BACK) {
            translation_run_.text.pop_back();
        } else if (typed && !modifiers.ctrl && !modifiers.alt && !modifiers.win) {
            // The character the key typed, from the keyboard layout (without
            // disturbing its dead-key state).
            BYTE state[256]{};
            wchar_t typed_text[4]{};
            int count = 0;
            if (GetKeyboardState(state))
                count = ToUnicodeEx(key, MapVirtualKeyW(key, MAPVK_VK_TO_VSC), state, typed_text, 4, 0x4, GetKeyboardLayout(0));
            translation_run_.text += count == 1 ? typed_text[0] : kAnyCharacter;
            translation_run_.last_tick = GetTickCount64();
        } else {
            end_translation_run();
        }
    }
    if (recent_committed_text_.empty()) return;
    switch (key) {
        case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT: case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
        case VK_MENU: case VK_LMENU: case VK_RMENU: case VK_CAPITAL:
            return;
        case VK_BACK:
            recent_committed_text_.pop_back();
            return;
        default:
            break;
    }
    const auto modifiers = current_modifier_key_state(key_state_reader_, key_state_reader_context_);
    const bool printable = key == VK_SPACE || key == VK_RETURN || (key >= L'0' && key <= L'9') ||
                           (key >= L'A' && key <= L'Z') || (key >= VK_OEM_1 && key <= VK_OEM_8);
    // Typed host text is unknown but is a clause boundary for ranking. Caret
    // movement and editing shortcuts make the remembered text unreliable.
    if (printable && !modifiers.ctrl && !modifiers.alt && !modifiers.win) recent_committed_text_ += L' ';
    else forget_committed_text();
}

void TextService::extend_translation_run(ITfContext* context, const std::wstring& text) {
    if (!tab_translate_enabled()) {
        end_translation_run();
        return;
    }
    const auto identity = reinterpret_cast<std::uintptr_t>(context);
    const ULONGLONG now = GetTickCount64();
    if (translation_run_.context != identity || now - translation_run_.last_tick > kTranslationRunLifetimeMs)
        translation_run_ = TranslationRun{};
    // The run goes on only while the document still has it right before
    // this text: a click elsewhere starts a new one.
    if (!translation_run_.text.empty() && !transitory_context(context)) {
        std::wstring before;
        const std::wstring expected = translation_run_.text + text;
        if (composition_.read_preceding_text(context, client_id_, &before, nullptr, nullptr, expected.size()) != S_OK ||
            !ends_with_run(before, expected))
            translation_run_ = TranslationRun{};
    }
    translation_run_.context = identity;
    translation_run_.last_tick = now;
    translation_run_.text += text;
    if (translation_run_.text.size() > kMaxTranslationUnits)
        translation_run_.text.erase(0, translation_run_.text.size() - kMaxTranslationUnits);
}

bool TextService::transitory_context(ITfContext* context) {
    TF_STATUS status{};
    return context && SUCCEEDED(context->GetStatus(&status)) && (status.dwStaticFlags & TF_SS_TRANSITORY) != 0;
}

bool TextService::tab_translate_enabled() const {
    return settings_store_.load_input_mode_options().tab_translate;
}

TextService::TranslationSource TextService::translation_source(ITfContext* context) {
    const auto& run = translation_run_;
    if (!context || secure_mode_ || run.text.empty() || run.context != reinterpret_cast<std::uintptr_t>(context) ||
        GetTickCount64() - run.last_tick > kTranslationRunLifetimeMs ||
        std::none_of(run.text.begin(), run.text.end(), is_han))
        return {};
    // Many applications show the input method only what is being typed
    // (older Win32 programs through Windows' compatibility layer): go by the
    // run, which a click, a caret key or a shortcut would have ended.
    if (transitory_context(context)) {
        if (run.text.find(kAnyCharacter) != std::wstring::npos) return {};
        return {run.text, false};
    }
    // Elsewhere the document must still end with what was typed: nothing
    // was pasted or deleted in between.
    std::wstring before;
    if (composition_.read_preceding_text(context, client_id_, &before, nullptr, nullptr, run.text.size()) != S_OK ||
        !ends_with_run(before, run.text))
        return {};
    return {before.substr(before.size() - run.text.size()), true};
}

void TextService::start_translation(ITfContext* context, const TranslationSource& source) {
    cancel_translation();
    auto job = std::make_shared<TranslationJob>();
    job->serial = ++translation_serial_;
    job->source = source.text;
    translation_source_read_ = source.read;
    job->translator = translator_ ? translator_ : Translator(translate_with_google);
    job->target = candidate_window_.async_target();
    if (!job->target || !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                                            reinterpret_cast<LPCWSTR>(&translation_thread), &job->module)) {
        log_status(L"translation", L"could not start");
        return;
    }
    auto* parameter = new std::shared_ptr<TranslationJob>(job);
    HANDLE thread = CreateThread(nullptr, 0, translation_thread, parameter, 0, nullptr);
    if (!thread) {
        delete parameter;
        FreeLibrary(job->module);
        log_status(L"translation", L"thread failed " + std::to_wstring(GetLastError()));
        return;
    }
    CloseHandle(thread);
    pending_translation_ = job;
    context->AddRef();
    translation_context_ = context;
    candidate_window_.schedule_anchor_check([this, serial = job->serial]() {
        if (pending_translation_ && pending_translation_->serial == serial)
            show_translation_notice(L"翻译中…", pending_translation_->source, 0);
    }, kTranslationNoticeDelayMs);
}

void TextService::finish_translation(unsigned long long serial) {
    const auto job = pending_translation_;
    if (!job || job->serial != serial) return;
    std::optional<TranslationResult> result;
    {
        std::lock_guard<std::mutex> guard(job->lock);
        result = job->result;
    }
    if (!result) return;
    // A first Tab's translation lands once a second Tab can no longer come.
    if (GetTickCount64() < translation_hold_until_) return;
    pending_translation_.reset();
    candidate_window_.cancel_anchor_check();
    if (!result->ok || result->text.empty()) {
        show_translation_notice(L"翻译失败", result->error.empty() ? L"翻译失败。" : result->error,
                                kTranslationErrorShownMs);
    } else {
        // The spaces around the original stay where they were.
        const std::wstring& source = job->source;
        size_t first = 0, last = source.size();
        while (first < last && is_space(source[first])) ++first;
        while (last > first && is_space(source[last - 1])) --last;
        const std::wstring replacement = source.substr(0, first) + result->text + source.substr(last);
        // In place through the document where the application allows it.
        HRESULT hr = composition_.replace_preceding_text(translation_context_, client_id_, source, replacement);
        bool replaced = hr == S_OK;
        if (!replaced && transitory_context(translation_context_)) {
            // A document that shows only what is being typed (it may have
            // shown the source just after it was committed) gets the
            // translation typed.
            end_translation_run();
            replaced = type_replacement(source, replacement);
        }
        if (replaced) {
            end_translation_run();
            end_phrase_run();
            forget_committed_text();
            if (candidate_window_.showing_notice()) candidate_window_.hide(L"translated");
            log_status(L"translation", L"replaced " + std::to_wstring(source.size()) + L" units" +
                                           (hr == S_OK ? L"" : L" by typing"));
            if (hr == S_OK && replacement != source) {
                // Still the original once the application has settled: it
                // did not take the edit, so type the translation instead.
                Microsoft::WRL::ComPtr<ITfContext> context(translation_context_);
                translation_check_pending_ = true;
                candidate_window_.schedule_anchor_check([this, context, source, replacement]() {
                    if (!translation_check_pending_) return;
                    translation_check_pending_ = false;
                    const auto ends_with = [](const std::wstring& text, const std::wstring& tail) {
                        return text.size() >= tail.size() && text.compare(text.size() - tail.size(), tail.size(), tail) == 0;
                    };
                    std::wstring before;
                    if (composition_.read_preceding_text(context.Get(), client_id_, &before, nullptr, nullptr,
                                                         (std::max)(source.size(), replacement.size())) == S_OK &&
                        ends_with(before, source) && !ends_with(before, replacement) &&
                        type_replacement(source, replacement))
                        log_status(L"translation", L"application kept the original; typed instead");
                }, kTranslationCheckMs);
            }
        } else {
            show_translation_notice(L"没有替换", L"原文已经改动，或者已经离开这个输入框。", kTranslationErrorShownMs);
            log_status(L"translation", L"replace failed " + hresult_hex(hr));
        }
    }
    if (translation_context_) {
        translation_context_->Release();
        translation_context_ = nullptr;
    }
}

bool TextService::type_replacement(const std::wstring& source, const std::wstring& replacement) {
    // Typed keys go to the window with the keyboard focus: it must be this
    // thread's, where the source was typed.
    GUITHREADINFO info{};
    info.cbSize = sizeof(info);
    if (!GetGUIThreadInfo(0, &info) || !info.hwndFocus ||
        GetWindowThreadProcessId(info.hwndFocus, nullptr) != GetCurrentThreadId())
        return false;
    std::vector<INPUT> inputs;
    const auto key = [&inputs](WORD vk, wchar_t ch, bool up) {
        INPUT input{};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vk;
        input.ki.wScan = ch;
        input.ki.dwFlags = (ch ? KEYEVENTF_UNICODE : 0) | (up ? KEYEVENTF_KEYUP : 0);
        inputs.push_back(input);
    };
    // One Backspace per character (a surrogate pair is one).
    for (size_t index = 0; index < source.size(); ++index) {
        if (source[index] >= 0xDC00 && source[index] <= 0xDFFF && index > 0) continue;
        key(VK_BACK, 0, false);
        key(VK_BACK, 0, true);
    }
    for (const wchar_t ch : replacement) {
        key(0, ch, false);
        key(0, ch, true);
    }
    return SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT)) == inputs.size();
}

wchar_t TextService::tool_key_character(WPARAM wparam, bool shift) const {
    // Only after a leading v (and nothing but tool characters since) in full pinyin.
    if (composing_.empty() || composing_[0] != L'v' || engine_.input_options().double_pinyin != DoublePinyinScheme::None)
        return 0;
    if (composing_.size() > 1 && !std::all_of(composing_.begin() + 1, composing_.end(), [](wchar_t ch) {
            return std::wstring(L"0123456789.+-*/()^%").find(ch) != std::wstring::npos;
        })) return 0;
    if (!shift && wparam >= L'0' && wparam <= L'9') return static_cast<wchar_t>(wparam);
    if (wparam >= VK_NUMPAD0 && wparam <= VK_NUMPAD9) return static_cast<wchar_t>(L'0' + (wparam - VK_NUMPAD0));
    if (shift) {
        switch (wparam) {
            case VK_OEM_PLUS: return L'+';
            case L'8': return L'*';
            case L'9': return L'(';
            case L'0': return L')';
            case L'6': return L'^';
            case L'5': return L'%';
            default: return 0;
        }
    }
    switch (wparam) {
        case VK_OEM_PERIOD: case VK_DECIMAL: return L'.';
        case VK_OEM_MINUS: case VK_SUBTRACT: return L'-';
        case VK_ADD: return L'+';
        case VK_MULTIPLY: return L'*';
        case VK_OEM_2: case VK_DIVIDE: return L'/';
        default: return 0;
    }
}

void TextService::finish_tab_tap() {
    const bool replay = tab_tap_.pending && tab_tap_.replay;
    tab_tap_ = TabTap{};
    translation_hold_until_ = 0;
    // Only into this application: the keys go to whatever is in front.
    const HWND foreground = GetForegroundWindow();
    DWORD foreground_process = 0;
    if (foreground) GetWindowThreadProcessId(foreground, &foreground_process);
    if (replay && foreground_process == GetCurrentProcessId()) {
        // The application gets its Tab, passed over by this service.
        pass_next_tab_ = true;
        INPUT keys[2]{};
        keys[0].type = keys[1].type = INPUT_KEYBOARD;
        keys[0].ki.wVk = keys[1].ki.wVk = VK_TAB;
        keys[1].ki.dwFlags = KEYEVENTF_KEYUP;
        if (SendInput(2, keys, sizeof(INPUT)) != 2) pass_next_tab_ = false;
    }
    if (pending_translation_) finish_translation(pending_translation_->serial);
}

void TextService::open_panel(ITfContext* context) {
    if (secure_mode_) return;
    // A fresh token for this opening: only the panel started with it may
    // type through this window.
    std::random_device random;
    wchar_t token[40]{};
    swprintf_s(token, L"%08x%08x%08x%08x", random(), random(), random(), random());
    panel_token_ = token;
    const HWND target = candidate_window_.async_target();
    const auto owner = candidate_owner_info(context);
    const HWND host = owner.root_valid ? owner.root_hwnd : GetForegroundWindow();
    RECT anchor{};
    if (!context || !candidate_anchor_rect(context, &anchor)) {
        POINT cursor{};
        GetCursorPos(&cursor);
        anchor = RECT{cursor.x, cursor.y, cursor.x + 1, cursor.y + 1};
    }
    const std::wstring arguments = L"--panel --ime=" + std::to_wstring(reinterpret_cast<std::uintptr_t>(target)) +
        L" --host=" + std::to_wstring(reinterpret_cast<std::uintptr_t>(host)) + L" --token=" + panel_token_ +
        L" --anchor=" + std::to_wstring(anchor.left) + L"," + std::to_wstring(anchor.top) + L"," + std::to_wstring(anchor.bottom);
    if (!launch_settings_app(arguments)) log_status(L"panel", L"could not start the settings app");
}

bool TextService::accept_panel_message(const COPYDATASTRUCT& data) {
    if (data.dwData != kPanelInsertMessage || !data.lpData || data.cbData < 2 * sizeof(wchar_t) ||
        data.cbData > 64 * 1024 || panel_token_.empty() || secure_mode_) return false;
    std::wstring payload(static_cast<const wchar_t*>(data.lpData), data.cbData / sizeof(wchar_t));
    while (!payload.empty() && payload.back() == L'\0') payload.pop_back();
    const auto line = payload.find(L'\n');
    if (line == std::wstring::npos || payload.compare(0, line, panel_token_) != 0) return false;
    pending_panel_text_ = payload.substr(line + 1);
    panel_insert_attempts_ = 0;
    // Inserted once the application has its focus back, outside this message.
    candidate_window_.schedule_task(kPanelTask, 10, [this]() { insert_panel_text(); });
    return !pending_panel_text_.empty();
}

void TextService::insert_panel_text() {
    if (pending_panel_text_.empty() || !thread_mgr_) return;
    ITfDocumentMgr* document = nullptr;
    ITfContext* context = nullptr;
    const HWND foreground = GetForegroundWindow();
    DWORD foreground_process = 0;
    if (foreground) GetWindowThreadProcessId(foreground, &foreground_process);
    const bool ours = foreground_process == GetCurrentProcessId();
    if (ours && SUCCEEDED(thread_mgr_->GetFocus(&document)) && document) document->GetTop(&context);
    if (document) document->Release();
    if (!context) {
        if (++panel_insert_attempts_ < 60) candidate_window_.schedule_task(kPanelTask, 50, [this]() { insert_panel_text(); });
        else pending_panel_text_.clear();
        return;
    }
    const std::wstring text = std::move(pending_panel_text_);
    pending_panel_text_.clear();
    if (!composing_.empty()) commit_whole_composition(context);
    if (SUCCEEDED(composition_.insert_text(context, client_id_, text))) remember_committed_text(context, text);
    context->Release();
}

void TextService::cancel_translation() {
    translation_check_pending_ = false;
    if (!pending_translation_) return;
    pending_translation_.reset();
    candidate_window_.cancel_anchor_check();
    if (candidate_window_.showing_notice()) candidate_window_.hide(L"translation_cancelled");
    if (translation_context_) {
        translation_context_->Release();
        translation_context_ = nullptr;
    }
}

void TextService::show_translation_notice(const std::wstring& title, const std::wstring& body, UINT hide_after_ms) {
    RECT anchor{};
    if (!translation_context_ || !candidate_anchor_rect(translation_context_, &anchor)) return;
    candidate_window_.show_notice(title, body, anchor, candidate_owner_info(translation_context_), hide_after_ms);
}

void TextService::forget_committed_text() noexcept {
    recent_committed_text_.clear();
    recent_committed_context_ = 0;
    recent_committed_tick_ = 0;
}

std::wstring TextService::recent_committed_text(ITfContext* context) const {
    if (!context || reinterpret_cast<std::uintptr_t>(context) != recent_committed_context_ ||
        GetTickCount64() - recent_committed_tick_ > kRecentCommitLifetimeMs) return {};
    return recent_committed_text_;
}

HRESULT TextService::cancel_composition(ITfContext* context) {
    const HRESULT hr = composition_.cancel(context, client_id_);
    if (FAILED(hr)) {
        log_status(L"composition", L"cancel failed " + hresult_hex(hr));
    }
    reset_transient_input_state(L"cancel");
    return hr;
}

void TextService::set_composition_context(ITfContext* context) {
    if (composition_context_ == context) {
        return;
    }
    if (context) {
        context->AddRef();
    }
    if (composition_context_) {
        composition_context_->Release();
    }
    composition_context_ = context;
}

void TextService::set_candidate_context(ITfContext* context) {
    if (candidate_context_ == context) {
        return;
    }
    if (context) {
        context->AddRef();
    }
    if (candidate_context_) {
        if (layout_sink_cookie_ != TF_INVALID_COOKIE) {
            ITfSource* source = nullptr;
            if (SUCCEEDED(candidate_context_->QueryInterface(IID_PPV_ARGS(&source)))) {
                source->UnadviseSink(layout_sink_cookie_);
                source->Release();
            }
            layout_sink_cookie_ = TF_INVALID_COOKIE;
        }
        candidate_context_->Release();
    }
    candidate_context_ = context;
    candidate_window_.cancel_anchor_check();
    if (context) {
        ITfSource* source = nullptr;
        if (SUCCEEDED(context->QueryInterface(IID_PPV_ARGS(&source)))) {
            if (FAILED(source->AdviseSink(__uuidof(ITfTextLayoutSink), static_cast<ITfTextLayoutSink*>(this),
                                          &layout_sink_cookie_))) {
                layout_sink_cookie_ = TF_INVALID_COOKIE;
            }
            source->Release();
        }
    }
}

void TextService::clear_transient_contexts() {
    set_candidate_context(nullptr);
    set_composition_context(nullptr);
}

void TextService::reset_transient_input_state(const wchar_t* reason) {
    unadvise_prediction_edits();
    prediction_active_ = false;
    prediction_before_.clear();
    if (prediction_caret_) { prediction_caret_->Release(); prediction_caret_ = nullptr; }
    candidate_window_.cancel_layout_retry();
    end_candidate_ui_element(reason ? reason : L"transient_reset");
    composition_.clear();
    composing_.clear();
    preceding_committed_text_.clear();
    candidates_.clear();
    candidate_page_indices_.clear();
    host_page_indices_ = false;
    selected_index_ = 0;
    has_last_caret_rect_ = false;
    candidate_layout_retry_attempts_ = 0;
    candidate_window_.hide(reason ? reason : L"transient_reset");
    clear_transient_contexts();
}

void TextService::log_activation_focus_diagnostics(const ActivationFocusDiagnostics& diagnostics) {
    log_status(L"compatibility", make_activation_focus_diagnostic_log(diagnostics));
}

bool TextService::transient_context_matches(ITfContext* context) const noexcept {
    return !transient_context_changed(
        reinterpret_cast<std::uintptr_t>(composition_context_),
        reinterpret_cast<std::uintptr_t>(context),
        !composing_.empty() || !candidates_.empty() || composition_.active());
}

void TextService::prepare_context_for_key(ITfContext* context) {
    const bool active = !composing_.empty() || !candidates_.empty() || composition_.active();
    if (!transient_context_changed(
            reinterpret_cast<std::uintptr_t>(composition_context_),
            reinterpret_cast<std::uintptr_t>(context), active)) {
        return;
    }

    if (composition_.active() && composition_context_) {
        const HRESULT cancel_hr = composition_.cancel(composition_context_, client_id_);
        if (FAILED(cancel_hr)) {
            log_status(L"composition", L"context change cancel failed " + hresult_hex(cancel_hr));
        }
    }
    reset_transient_input_state(L"context_changed");
}

bool TextService::candidate_anchor_rect(ITfContext* context, RECT* rect, bool* layout_pending) {
    if (layout_pending) *layout_pending = false;
    if (!rect) {
        return false;
    }

    if (context) {
        RECT tsf_rect{};
        const HRESULT hr = composition_.caret_rect(context, client_id_, &tsf_rect, nullptr);
        if (hr == S_OK && usable_anchor_rect(tsf_rect)) {
            *rect = tsf_rect;
            last_caret_rect_ = tsf_rect;
            has_last_caret_rect_ = true;
            last_caret_rect_from_text_ = true;
            return true;
        }
        if (layout_pending) *layout_pending = hr == TF_E_NOLAYOUT;
    }

    // While the app lays out the key just typed, where it laid out this
    // composition before is closer than the system caret, which some apps
    // leave elsewhere (the layout notification then moves the popup on).
    if (has_last_caret_rect_ && last_caret_rect_from_text_ && usable_anchor_rect(last_caret_rect_)) {
        *rect = last_caret_rect_;
        return true;
    }

    RECT gui_rect{};
    if (gui_caret_rect(&gui_rect)) {
        *rect = gui_rect;
        last_caret_rect_ = gui_rect;
        has_last_caret_rect_ = true;
        last_caret_rect_from_text_ = false;
        return true;
    }

    if (has_last_caret_rect_ && usable_anchor_rect(last_caret_rect_)) {
        *rect = last_caret_rect_;
        return true;
    }
    return false;
}

CandidateWindowOwnerInfo TextService::candidate_owner_info(ITfContext* context) const {
    CandidateWindowOwnerInfo owner_info{};
    ITfContextView* view = nullptr;
    if (context && SUCCEEDED(context->GetActiveView(&view)) && view) {
        view->GetWnd(&owner_info.context_hwnd);
        view->Release();
    }

    owner_info.context_valid = owner_info.context_hwnd && IsWindow(owner_info.context_hwnd);
    if (owner_info.context_valid) {
        owner_info.root_hwnd = GetAncestor(owner_info.context_hwnd, GA_ROOT);
        owner_info.root_valid = owner_info.root_hwnd && IsWindow(owner_info.root_hwnd);
    }

    return owner_info;
}

bool TextService::gui_caret_rect(RECT* rect) const {
    if (!rect) {
        return false;
    }

    GUITHREADINFO info{};
    info.cbSize = sizeof(info);
    if (!GetGUIThreadInfo(GetCurrentThreadId(), &info) || !info.hwndCaret) {
        return false;
    }

    POINT top_left{info.rcCaret.left, info.rcCaret.top};
    POINT bottom_right{info.rcCaret.right, info.rcCaret.bottom};
    if (!ClientToScreen(info.hwndCaret, &top_left) || !ClientToScreen(info.hwndCaret, &bottom_right)) {
        return false;
    }

    RECT screen_rect{top_left.x, top_left.y, bottom_right.x, bottom_right.y};
    if (screen_rect.right < screen_rect.left || screen_rect.bottom < screen_rect.top) {
        return false;
    }
    if (screen_rect.left == screen_rect.right) {
        screen_rect.right = screen_rect.left + 1;
    }
    if (screen_rect.top == screen_rect.bottom) {
        screen_rect.bottom = screen_rect.top + 1;
    }
    *rect = screen_rect;
    return usable_anchor_rect(*rect);
}

}  // namespace localpinyin
