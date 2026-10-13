#pragma once

#include <msctf.h>

#include <string>

namespace localpinyin {

struct EditSessionRequestResult {
    HRESULT request_hr = E_UNEXPECTED;
    HRESULT session_hr = E_UNEXPECTED;
    bool session_invoked = false;

    [[nodiscard]] HRESULT effective_hr() const noexcept {
        return FAILED(request_hr) ? request_hr : session_hr;
    }
};

EditSessionRequestResult make_edit_session_request_result(HRESULT request_hr,
                                                          HRESULT session_hr) noexcept;

struct CaretRectDiagnostics {
    HRESULT active_view_hr = E_UNEXPECTED;
    HRESULT get_wnd_hr = E_UNEXPECTED;
    bool context_hwnd_present = false;
    HRESULT get_text_ext_hr = E_UNEXPECTED;
    bool get_text_ext_nolayout = false;
    bool text_ext_clipped = false;
};

class CompositionEditSession;
class CompositionSurroundingTextEditSession;

class CompositionManager {
public:
    ~CompositionManager();

    HRESULT set_text(ITfContext* context, TfClientId client_id, ITfCompositionSink* sink, const std::wstring& text);
    HRESULT commit_text(ITfContext* context, TfClientId client_id, const std::wstring& text);
    HRESULT insert_text(ITfContext* context, TfClientId client_id, const std::wstring& text);
    HRESULT cancel(ITfContext* context, TfClientId client_id);
    HRESULT caret_rect(ITfContext* context, TfClientId client_id, RECT* rect,
                       CaretRectDiagnostics* diagnostics = nullptr);
    // S_FALSE means a selected range, protected input, or moved caret. Reading
    // never changes the document or falls back to text from a previous context.
    HRESULT read_preceding_text(ITfContext* context, TfClientId client_id, std::wstring* text,
                                ITfRange** caret = nullptr, ITfRange* expected_caret = nullptr);
    [[nodiscard]] bool active() const noexcept { return composition_ != nullptr; }
    [[nodiscard]] EditSessionRequestResult last_edit_session_result() const noexcept {
        return last_edit_session_result_;
    }
    void clear();

private:
    friend class CompositionEditSession;
    friend class CompositionCaretRectEditSession;
    friend class CompositionSurroundingTextEditSession;

    HRESULT do_set_text(TfEditCookie cookie, ITfContext* context, ITfCompositionSink* sink, const std::wstring& text);
    HRESULT do_commit_text(TfEditCookie cookie, ITfContext* context, const std::wstring& text);
    HRESULT do_insert_text(TfEditCookie cookie, ITfContext* context, const std::wstring& text);
    HRESULT do_cancel(TfEditCookie cookie, ITfContext* context);
    HRESULT do_caret_rect(TfEditCookie cookie, ITfContext* context, RECT* rect,
                          CaretRectDiagnostics* diagnostics);
    HRESULT do_read_preceding_text(TfEditCookie cookie, ITfContext* context, std::wstring* text,
                                   ITfRange** caret, ITfRange* expected_caret);

    ITfComposition* composition_ = nullptr;
    EditSessionRequestResult last_edit_session_result_{};
};

}  // namespace localpinyin
