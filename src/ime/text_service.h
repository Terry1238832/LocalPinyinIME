#pragma once

#include "candidate_window.h"
#include "candidate_ui_element.h"
#include "composition_manager.h"
#include "input_mode.h"
#include "language_bar.h"
#include "preserved_key.h"
#include "runtime_diagnostics.h"
#include "../engine/pinyin_engine.h"
#include "../settings/settings_store.h"

#include <msctf.h>

#include <string>
#include <vector>

namespace localpinyin {

class TextService final : public ITfTextInputProcessorEx,
                          public ITfKeyEventSink,
                          public ITfCompositionSink {
public:
    using KeyStateReader = bool (*)(int virtual_key, void* context) noexcept;

    TextService();

    // Internal test seam. COM-created instances leave this unset and use Win32 key state.
    void set_key_state_reader_for_testing(KeyStateReader reader, void* context) noexcept;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    HRESULT STDMETHODCALLTYPE Activate(ITfThreadMgr* thread_mgr, TfClientId client_id) override;
    HRESULT STDMETHODCALLTYPE ActivateEx(ITfThreadMgr* thread_mgr, TfClientId client_id, DWORD flags) override;
    HRESULT STDMETHODCALLTYPE Deactivate() override;

    HRESULT STDMETHODCALLTYPE OnSetFocus(BOOL foreground) override;
    HRESULT STDMETHODCALLTYPE OnTestKeyDown(ITfContext* context, WPARAM wparam, LPARAM lparam, BOOL* eaten) override;
    HRESULT STDMETHODCALLTYPE OnKeyDown(ITfContext* context, WPARAM wparam, LPARAM lparam, BOOL* eaten) override;
    HRESULT STDMETHODCALLTYPE OnTestKeyUp(ITfContext* context, WPARAM wparam, LPARAM lparam, BOOL* eaten) override;
    HRESULT STDMETHODCALLTYPE OnKeyUp(ITfContext* context, WPARAM wparam, LPARAM lparam, BOOL* eaten) override;
    HRESULT STDMETHODCALLTYPE OnPreservedKey(ITfContext* context, REFGUID guid, BOOL* eaten) override;

    HRESULT STDMETHODCALLTYPE OnCompositionTerminated(TfEditCookie cookie, ITfComposition* composition) override;

private:
    ~TextService();

    HRESULT advise_key_event_sink();
    void unadvise_key_event_sink();
    HRESULT preserve_ctrl_space_key();
    HRESULT unpreserve_ctrl_space_key();
    static LRESULT CALLBACK thread_keyboard_hook_proc(int code, WPARAM wparam, LPARAM lparam);
    bool install_thread_keyboard_hook();
    void uninstall_thread_keyboard_hook();
    bool handle_thread_keyboard_hook(int code, unsigned long key, LPARAM lparam);
    bool handle_ctrl_space_ctrl_keyup_latch(unsigned long key,
                                            LPARAM lparam,
                                            const wchar_t* source);
    bool handle_ctrl_space_keyup_fallback(ITfContext* context,
                                          unsigned long key,
                                          LPARAM lparam,
                                          const wchar_t* source,
                                          bool* toggled);
    bool should_pass_through(WPARAM wparam) const;
    bool handle_ctrl_space_toggle(ITfContext* context, const wchar_t* reason);
    void publish_runtime_input_mode_status(const wchar_t* source, bool active);
    bool handle_chinese_key(ITfContext* context, WPARAM wparam, BOOL* eaten);
    bool shift_toggle_enabled() const;
    HRESULT toggle_input_mode(ITfContext* context, const wchar_t* reason);
    HRESULT toggle_input_mode_from_language_bar(const wchar_t* reason);
    HRESULT open_settings_from_language_bar(const wchar_t* source);
    void refresh_candidates();
    HRESULT update_composition(ITfContext* context);
    void update_candidate_window(ITfContext* context);
    void end_candidate_ui_element(const wchar_t* reason);
    HRESULT commit_selected(ITfContext* context);
    HRESULT commit_raw_composition(ITfContext* context);
    HRESULT cancel_composition(ITfContext* context);
    bool candidate_anchor_rect(ITfContext* context, RECT* rect);
    CandidateWindowOwnerInfo candidate_owner_info(ITfContext* context) const;
    bool gui_caret_rect(RECT* rect) const;
    void prepare_context_for_key(ITfContext* context);
    void log_activation_focus_diagnostics(const ActivationFocusDiagnostics& diagnostics);
    void set_composition_context(ITfContext* context);
    void set_candidate_context(ITfContext* context);
    void clear_transient_contexts();
    void reset_transient_input_state(const wchar_t* reason);
    [[nodiscard]] bool transient_context_matches(ITfContext* context) const noexcept;

    long ref_count_ = 1;
    ITfThreadMgr* thread_mgr_ = nullptr;
    TfClientId client_id_ = TF_CLIENTID_NULL;
    bool key_sink_advised_ = false;
    bool ctrl_space_preserved_ = false;
    HHOOK keyboard_hook_ = nullptr;
    DWORD keyboard_hook_thread_id_ = 0;
    bool keyboard_hook_uninstall_failed_ = false;
    InputMode mode_ = InputMode::Chinese;
    LanguageBarState language_bar_;
    CtrlSpaceToggleTracker ctrl_space_toggle_;
    CtrlSpaceKeyDownState ctrl_space_key_down_;
    unsigned long long runtime_status_sequence_ = 0;
    ShiftToggleTracker shift_toggle_;
    KeyStateReader key_state_reader_ = nullptr;
    void* key_state_reader_context_ = nullptr;
    std::wstring composing_;
    std::vector<Candidate> candidates_;
    size_t selected_index_ = 0;
    PinyinEngine engine_;
    SettingsStore settings_store_;
    CompositionManager composition_;
    CandidateWindow candidate_window_;
    CandidateUIElementSession candidate_ui_element_session_;
    RECT last_caret_rect_{};
    bool has_last_caret_rect_ = false;
    ITfContext* composition_context_ = nullptr;
    ITfContext* candidate_context_ = nullptr;
    unsigned int candidate_layout_retry_attempts_ = 0;
};

}  // namespace localpinyin
