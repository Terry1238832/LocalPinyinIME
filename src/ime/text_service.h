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
                          public ITfCompositionSink,
                          public ITfThreadMgrEventSink,
                          public ITfTextEditSink {
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
    HRESULT STDMETHODCALLTYPE OnInitDocumentMgr(ITfDocumentMgr*) override;
    HRESULT STDMETHODCALLTYPE OnUninitDocumentMgr(ITfDocumentMgr*) override;
    HRESULT STDMETHODCALLTYPE OnSetFocus(ITfDocumentMgr* focus, ITfDocumentMgr* previous) override;
    HRESULT STDMETHODCALLTYPE OnPushContext(ITfContext* context) override;
    HRESULT STDMETHODCALLTYPE OnPopContext(ITfContext* context) override;
    HRESULT STDMETHODCALLTYPE OnEndEdit(ITfContext* context, TfEditCookie cookie, ITfEditRecord* record) override;

private:
    ~TextService();

    HRESULT advise_key_event_sink();
    void unadvise_key_event_sink();
    void advise_thread_events();
    void unadvise_thread_events();
    void unadvise_prediction_edits();
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
    HRESULT handle_key_down(ITfContext* context, WPARAM wparam, BOOL* eaten);
    bool handle_chinese_key(ITfContext* context, WPARAM wparam, BOOL* eaten);
    // CapsLock on or Shift held: letters are typed as English letters.
    [[nodiscard]] bool letter_is_literal() const noexcept;
    [[nodiscard]] bool composition_has_literal_letters() const noexcept;
    // Text this service committed into a context recently. It is the ranking
    // context for hosts that cannot report the text before the caret; it lives
    // only in memory and is dropped on focus changes and caret movement.
    void remember_committed_text(ITfContext* context, const std::wstring& text);
    void note_unhandled_key(unsigned long key);
    void forget_committed_text() noexcept;
    [[nodiscard]] std::wstring recent_committed_text(ITfContext* context) const;
    bool shift_toggle_enabled() const;
    HRESULT toggle_input_mode(ITfContext* context, const wchar_t* reason);
    HRESULT toggle_input_mode_from_language_bar(const wchar_t* reason);
    HRESULT open_settings_from_language_bar(const wchar_t* source);
    void refresh_candidates();
    void change_candidate_page(ITfContext* context, int direction);
    [[nodiscard]] size_t current_candidate_page() const noexcept;
    [[nodiscard]] size_t current_candidate_page_count() const noexcept;
    HRESULT update_composition(ITfContext* context);
    void update_candidate_window(ITfContext* context);
    void refresh_candidate_window_from_host();
    void end_candidate_ui_element(const wchar_t* reason);
    HRESULT commit_selected(ITfContext* context);
    void start_prediction(ITfContext* context);
    bool prediction_still_valid(ITfContext* context);
    HRESULT commit_prediction(ITfContext* context);
    void dismiss_prediction(const wchar_t* reason);
    void accept_candidate_from_ui(size_t index);
    HRESULT make_composition_room(ITfContext* context);
    HRESULT commit_raw_composition(ITfContext* context);
    HRESULT commit_punctuation(ITfContext* context, wchar_t punctuation);
    // A code-editor punctuation key that types ASCII (not the pinyin separator).
    [[nodiscard]] wchar_t code_punctuation_for_key(WPARAM wparam, bool shift) const;
    // What the document has right before the caret, for the punctuation
    // helpers; when it cannot be read, what the key typed before was.
    struct CaretNeighbor { bool digit = false; bool letter = false; };
    [[nodiscard]] CaretNeighbor neighbor_before_caret(ITfContext* context);
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
    DWORD thread_event_cookie_ = TF_INVALID_COOKIE;
    DWORD prediction_edit_cookie_ = TF_INVALID_COOKIE;
    bool secure_mode_ = false;
    bool prediction_active_ = false;
    bool suppress_prediction_ = false;
    unsigned long prediction_passthrough_key_ = 0;
    ITfRange* prediction_caret_ = nullptr;
    std::wstring prediction_before_;
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
    std::wstring preceding_committed_text_;
    std::wstring recent_committed_text_;
    std::uintptr_t recent_committed_context_ = 0;
    ULONGLONG recent_committed_tick_ = 0;
    // The host is a code editor, IDE or terminal: punctuation is ASCII there.
    bool code_application_ = false;
    // Whether the key before the one being handled was a digit, for 3. when
    // the document's text cannot be read. A host may test one key press more
    // than once, so a key counts as new only after its key-up.
    WPARAM tested_key_ = 0;
    bool tested_key_digit_ = false;
    bool previous_key_letter_ = false;
    bool tested_key_letter_ = false;
    // 造词: picks committed one right after another, with nothing else typed
    // and the caret unmoved, remembered as one phrase from the second on.
    struct PhraseRun {
        std::wstring typed;
        std::wstring text;
        std::wstring preceding;
        size_t pieces = 0;
        ULONGLONG last_tick = 0;
        std::uintptr_t context = 0;
    };
    PhraseRun phrase_run_;
    void extend_phrase_run(ITfContext* context, const std::wstring& typed, const std::wstring& committed,
                           const std::wstring& preceding);
    void end_phrase_run() noexcept { phrase_run_ = PhraseRun{}; }
    bool previous_key_digit_ = false;
    bool opening_double_quote_ = true;
    bool opening_single_quote_ = true;
    std::vector<Candidate> candidates_;
    size_t selected_index_ = 0;
    std::vector<UINT> candidate_page_indices_;
    bool host_page_indices_ = false;
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
