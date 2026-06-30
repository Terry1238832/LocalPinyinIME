#pragma once

#include <msctf.h>

#include <string>

namespace localpinyin {

class CompositionEditSession;

class CompositionManager {
public:
    ~CompositionManager();

    HRESULT set_text(ITfContext* context, TfClientId client_id, ITfCompositionSink* sink, const std::wstring& text);
    HRESULT commit_text(ITfContext* context, TfClientId client_id, const std::wstring& text);
    HRESULT cancel(ITfContext* context, TfClientId client_id);
    HRESULT caret_rect(ITfContext* context, TfClientId client_id, RECT* rect);
    void clear();

private:
    friend class CompositionEditSession;
    friend class CompositionCaretRectEditSession;

    HRESULT do_set_text(TfEditCookie cookie, ITfContext* context, ITfCompositionSink* sink, const std::wstring& text);
    HRESULT do_commit_text(TfEditCookie cookie, ITfContext* context, const std::wstring& text);
    HRESULT do_cancel(TfEditCookie cookie, ITfContext* context);
    HRESULT do_caret_rect(TfEditCookie cookie, ITfContext* context, RECT* rect);

    ITfComposition* composition_ = nullptr;
};

}  // namespace localpinyin
