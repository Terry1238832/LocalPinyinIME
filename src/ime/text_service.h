#pragma once

#include "candidate_window.h"
#include "candidate_ui_element.h"
#include "composition_manager.h"
#include "input_mode.h"
#include "language_bar.h"
#include "online_translation.h"
#include "preserved_key.h"
#include "runtime_diagnostics.h"
#include "tool_panel.h"
#include "tool_panel_window.h"
#include "../engine/english_speller.h"
#include "../engine/pinyin_engine.h"
#include "../settings/settings_store.h"

#include <msctf.h>

#include <memory>
#include <string>
#include <vector>

namespace localpinyin {

struct TranslationJob;

class TextService final : public ITfTextInputProcessorEx,
                          public ITfKeyEventSink,
                          public ITfCompositionSink,
                          public ITfThreadMgrEventSink,
                          public ITfTextEditSink,
                          public ITfTextLayoutSink,
                          public ITfInputProcessorProfileActivationSink {
public:
    using KeyStateReader = bool (*)(int virtual_key, void* context) noexcept;

    TextService();

    // Internal test seam. COM-created instances leave this unset and use Win32 key state.
    void set_key_state_reader_for_testing(KeyStateReader reader, void* context) noexcept;
    // Internal test seam: Tab translations come from here, not the network.
    void set_translator_for_testing(Translator translator) { translator_ = std::move(translator); }
    // As if this service had just sent the application a single tap's Tab.
    void pass_next_tab_for_testing() { pass_next_tab_ = true; }

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
    HRESULT STDMETHODCALLTYPE OnLayoutChange(ITfContext* context, TfLayoutCode code, ITfContextView* view) override;
    // Switching between 本地拼音 and 本地英文, two profiles of this service.
    HRESULT STDMETHODCALLTYPE OnActivated(DWORD profile_type, LANGID langid, REFCLSID clsid, REFGUID category,
                                          REFGUID profile, HKL layout, DWORD flags) override;
    // Internal test seam: as if 本地英文 (or 本地拼音) had just become active.
    void use_english_profile_for_testing(bool english) { use_profile(nullptr, english, L"test"); }

private:
    ~TextService();

    HRESULT advise_key_event_sink();
    void unadvise_key_event_sink();
    void advise_thread_events();
    void unadvise_thread_events();
    void advise_profile_activation();
    void unadvise_profile_activation();
    // Which of this service's profiles the thread has active (本地英文 or not).
    [[nodiscard]] bool active_profile_is_english() const;
    // 本地英文 types English words with spelling help (or plainly, after
    // Shift); 本地拼音 types Chinese (or plain English). Each keeps the mode
    // it was left in.
    void use_profile(ITfContext* context, bool english, const wchar_t* reason);
    void unadvise_prediction_edits();
    HRESULT preserve_ctrl_space_key();
    HRESULT unpreserve_ctrl_space_key();
    // The switch key (imeSwitchKey): preserved as set, again when it changes.
    HRESULT preserve_switch_key();
    HRESULT unpreserve_switch_key();
    // To the other of this service's two input methods, for the session as
    // Win + Space does; a CapsLock in the key is put back once released.
    void switch_profile(ITfContext* context);
    void restore_caps_after_switch();
    // Prepares the engine's indexes in idle moments after activation, one
    // letter at a time, so the first keystroke of each initial does not stall.
    void schedule_warm_up(UINT delay_ms);
    static LRESULT CALLBACK thread_keyboard_hook_proc(int code, WPARAM wparam, LPARAM lparam);
    // A click in the application ends the run Tab would translate: the caret
    // may have moved where the input method cannot see it.
    static LRESULT CALLBACK thread_mouse_hook_proc(int code, WPARAM wparam, LPARAM lparam);
    void install_thread_mouse_hook();
    // Tool panel. Tab twice opens it: the first Tab waits kDoubleTabMs for a
    // second. A translation it starts runs meanwhile but lands after the
    // wait; a plain Tab is played back to the application after it (another
    // key in between gets a tab character first, keeping the order).
    struct TabTap {
        bool pending = false;
        ULONGLONG tick = 0;
        bool replay = false;
    };
    TabTap tab_tap_;
    bool pass_next_tab_ = false;
    bool key_repeat_ = false;
    ULONGLONG translation_hold_until_ = 0;
    std::wstring panel_token_;
    std::wstring pending_panel_text_;
    int panel_insert_attempts_ = 0;
    void finish_tab_tap();
    // The settings app's large panel, on `page` (the small one's ⤢).
    void open_panel(ITfContext* context, const wchar_t* page = nullptr);
    // The small panel Tab twice opens at the caret; keys go to it while up.
    void show_tool_panel(ITfContext* context);
    void hide_tool_panel();
    [[nodiscard]] bool tool_panel_open() const noexcept;
    void handle_tool_panel_key(ITfContext* context, unsigned long key, bool shift, bool ctrl);
    // Ctrl+Delete while typing: the highlighted candidate's learned choices
    // are forgotten and the candidates shown again.
    void forget_selected_candidate(ITfContext* context);
    // CapsLock while the panel is open is the panel's (it walks the
    // categories): eaten, and its on/off put back on release. True when the
    // key is dealt with; `route` sends it to the panel (the key-down, not the test).
    bool tool_panel_caps_down(ITfContext* context, const ModifierKeyState& modifiers, bool route, BOOL* eaten);
    void tool_panel_caps_up();
    void handle_tool_panel_action(const PanelAction& action, ITfContext* context);
    ToolPanel& tool_panel();
    // Runs the settings bridge beside this DLL without waiting (pastes, emoji).
    void run_bridge(const std::vector<std::wstring>& arguments);
    // A record copied for pasting elsewhere: kept out of clipboard history and
    // cleared after a while.
    void copy_secret(const std::wstring& text);
    std::unique_ptr<ToolPanel> tool_panel_;
    ToolPanelWindow tool_panel_window_;
    HWND tool_panel_host_ = nullptr;
    DWORD secret_clipboard_sequence_ = 0;
    // CapsLock's on/off when the panel opened, and whether the panel holds a
    // CapsLock press. The press sent to put it back: 1 on its way, 2 down.
    bool caps_expected_on_ = false;
    bool caps_panel_down_ = false;
    int caps_restore_pending_ = 0;
    bool accept_panel_message(const COPYDATASTRUCT& data);
    void insert_panel_text();
    // v and a sum, number or date: the typing tools (full pinyin only).
    [[nodiscard]] wchar_t tool_key_character(WPARAM wparam, bool shift) const;
    void uninstall_thread_mouse_hook();
    void note_mouse_press(HWND window);
    HHOOK mouse_hook_ = nullptr;
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
    // Many hosts (older programs through Windows' compatibility layer) only
    // test a key they will type themselves and never send OnKeyDown for it,
    // and some test one press more than once: a press is noted once, from
    // either call, until its key-up. A held key's repeats cannot be counted.
    void note_key_left_to_application(unsigned long key, LPARAM lparam);
    unsigned long noted_key_ = 0;
    HRESULT test_key_down(ITfContext* context, WPARAM wparam, BOOL* eaten);
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
    // Apps often lay out new composition text only after the key that typed
    // it, so a popup can first be placed from a fallback caret. It follows
    // the text once the app reports its layout, and checks a few times
    // right after it appears for apps that never report.
    void follow_candidate_anchor();
    void schedule_anchor_follow(UINT delay_ms);
    [[nodiscard]] std::wstring candidate_display_label();
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
    // `layout_pending` is set when the app reported that its layout is not
    // ready yet and the rect is a fallback (the system caret or the last one).
    bool candidate_anchor_rect(ITfContext* context, RECT* rect, bool* layout_pending = nullptr);
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
    bool switch_key_preserved_ = false;
    TF_PRESERVEDKEY switch_key_{};
    std::wstring switch_key_setting_;
    // CapsLock's on/off before the switch key's press, to put back.
    bool switch_caps_pending_ = false;
    bool switch_caps_before_ = false;
    HHOOK keyboard_hook_ = nullptr;
    DWORD keyboard_hook_thread_id_ = 0;
    bool keyboard_hook_uninstall_failed_ = false;
    InputMode mode_ = InputMode::Chinese;
    bool english_profile_ = false;
    InputMode pinyin_mode_ = InputMode::Chinese;
    InputMode english_mode_ = InputMode::Assisted;
    DWORD profile_sink_cookie_ = TF_INVALID_COOKIE;
    // ── 本地英文 ─────────────────────────────────────────────────────────
    // Spelling help applies outside password fields, code editors and
    // terminals; there 辅 types plainly.
    [[nodiscard]] bool english_assist_active() const noexcept;
    // What a key does to the English word being typed. Decided the same way
    // when the key is tested and when it is pressed.
    enum class EnglishKey {
        PassThrough,  // the application's
        Letter,       // into the word (`character` has its case)
        Apostrophe,   // into the word (don't)
        Back,         // one letter off
        Undo,         // right after Space corrected a word: put back what was typed
        Cancel,       // Esc: the word is dropped
        Raw,          // Enter: the word as typed
        Space,        // the first candidate and a space
        Select,       // a digit: that candidate and a space
        Punctuation,  // the first candidate and `character` (, . ! ?)
        RawThen,      // the word as typed and `character` (0, a keypad digit)
        Commit,       // Tab: the first candidate
        Page,         // ← →
        SentenceSpace // sentence mode: a space; the second in a row puts the sentence in
    };
    // Sentence mode (englishSentence): the whole sentence is typed before it
    // goes in, corrected as a whole.
    [[nodiscard]] bool english_sentence_mode() const;
    // The words the first candidate's sentence spells differently, as typed.
    std::vector<std::wstring> english_sentence_spelling_;
    // Whether the text before the word ends a sentence (or is empty), and
    // whether the word is joined to it (an address): from the document, or
    // where it cannot be read, from what this service put in last.
    bool english_sentence_start_ = true;
    bool english_joined_ = false;
    [[nodiscard]] EnglishKey english_key(ITfContext* context, WPARAM wparam, const ModifierKeyState& modifiers,
                                         wchar_t* character);
    bool handle_english_key(ITfContext* context, WPARAM wparam, BOOL* eaten);
    void refresh_english_candidates();
    // Commits candidate `index` followed by `suffix` (a space, punctuation).
    // `automatic`: the first candidate, put in by Space or punctuation, so a
    // correction can be taken back with Backspace.
    HRESULT commit_english(ITfContext* context, size_t index, const std::wstring& suffix, bool automatic = false);
    HRESULT commit_english_raw(ITfContext* context, const std::wstring& suffix);
    // The word Space (or punctuation) just corrected, until another key.
    struct EnglishCorrection {
        std::uintptr_t context = 0;
        std::wstring typed;
        std::wstring committed;
        ULONGLONG tick = 0;
        // A sentence: taken back, these words (spelled differently) are learned.
        bool sentence = false;
        std::vector<std::wstring> learn;
    };
    EnglishCorrection english_correction_;
    // Whether the document still ends with the correction just made.
    [[nodiscard]] bool english_undo_available(ITfContext* context);
    HRESULT undo_english_correction(ITfContext* context);
    // Whether Space puts in corrections (otherwise the word as typed).
    [[nodiscard]] bool english_autocorrect_enabled() const;
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
    // A field that showed its committed text when read: an empty read there
    // means the field really is empty (a message just sent), not unreadable.
    std::uintptr_t text_exposing_context_ = 0;
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
    // Tab to English (online, opt-in): what was typed since the caret was
    // last placed, translated and put in its place. The run holds this
    // service's text as committed and the characters of keys the application
    // typed itself (a digit, a space; kAnyCharacter when unknown); Backspace
    // takes one off, anything else that may move the caret ends it.
    struct TranslationRun {
        std::uintptr_t context = 0;
        std::wstring text;
        ULONGLONG last_tick = 0;
    };
    TranslationRun translation_run_;
    std::shared_ptr<TranslationJob> pending_translation_;
    ITfContext* translation_context_ = nullptr;
    unsigned long long translation_serial_ = 0;
    Translator translator_;
    void extend_translation_run(ITfContext* context, const std::wstring& text);
    void end_translation_run() noexcept { translation_run_ = TranslationRun{}; }
    [[nodiscard]] bool tab_translate_enabled() const;
    // The text Tab would translate. When the application shows its text,
    // as the document has it (`read`), and nothing when the document no
    // longer ends with what was typed; when it does not, the run itself.
    struct TranslationSource {
        std::wstring text;
        bool read = false;
    };
    [[nodiscard]] TranslationSource translation_source(ITfContext* context);
    // A document that shows only what is being typed (TF_SS_TRANSITORY: older
    // programs through Windows' compatibility layer).
    [[nodiscard]] static bool transitory_context(ITfContext* context);
    bool translation_source_read_ = false;
    // A replacement through the document waits to be checked on screen.
    bool translation_check_pending_ = false;
    void start_translation(ITfContext* context, const TranslationSource& source);
    // Replaces the source by typing: Backspace for each of its characters,
    // then the translation. Only while this thread has the keyboard.
    bool type_replacement(const std::wstring& source, const std::wstring& replacement);
    void finish_translation(unsigned long long serial);
    void cancel_translation();
    void show_translation_notice(const std::wstring& title, const std::wstring& body, UINT hide_after_ms);
    // Commits everything being typed, the selected candidates in turn.
    HRESULT commit_whole_composition(ITfContext* context);
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
    // The last rect came from the app's text layout, not the system caret.
    bool last_caret_rect_from_text_ = false;
    ITfContext* composition_context_ = nullptr;
    ITfContext* candidate_context_ = nullptr;
    unsigned int candidate_layout_retry_attempts_ = 0;
    // The text layout sink is advised on the candidate context.
    DWORD layout_sink_cookie_ = TF_INVALID_COOKIE;
    unsigned int anchor_follow_step_ = 0;
};

}  // namespace localpinyin
