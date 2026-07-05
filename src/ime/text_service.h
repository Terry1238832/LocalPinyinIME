#pragma once

#include "candidate_window.h"
#include "composition_manager.h"
#include "input_mode.h"
#include "language_bar.h"
#include "preserved_key.h"
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
    TextService();

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
    bool should_pass_through(WPARAM wparam) const;
    bool handle_ctrl_space_toggle(ITfContext* context, const wchar_t* reason);
    bool handle_chinese_key(ITfContext* context, WPARAM wparam, BOOL* eaten);
    bool shift_toggle_enabled() const;
    HRESULT toggle_input_mode(ITfContext* context, const wchar_t* reason);
    void refresh_candidates();
    HRESULT update_composition(ITfContext* context);
    void update_candidate_window(ITfContext* context);
    HRESULT commit_selected(ITfContext* context);
    HRESULT commit_raw_composition(ITfContext* context);
    HRESULT cancel_composition(ITfContext* context);
    bool candidate_anchor_rect(ITfContext* context, RECT* rect);
    bool gui_caret_rect(RECT* rect) const;

    long ref_count_ = 1;
    ITfThreadMgr* thread_mgr_ = nullptr;
    TfClientId client_id_ = TF_CLIENTID_NULL;
    bool key_sink_advised_ = false;
    bool ctrl_space_preserved_ = false;
    InputMode mode_ = InputMode::Chinese;
    LanguageBarState language_bar_;
    CtrlSpaceToggleTracker ctrl_space_toggle_;
    ShiftToggleTracker shift_toggle_;
    std::wstring composing_;
    std::vector<Candidate> candidates_;
    size_t selected_index_ = 0;
    PinyinEngine engine_;
    SettingsStore settings_store_;
    CompositionManager composition_;
    CandidateWindow candidate_window_;
    RECT last_caret_rect_{};
    bool has_last_caret_rect_ = false;
};

}  // namespace localpinyin
