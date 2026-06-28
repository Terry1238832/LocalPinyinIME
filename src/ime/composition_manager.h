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
    void clear();

private:
    friend class CompositionEditSession;

    HRESULT do_set_text(TfEditCookie cookie, ITfContext* context, ITfCompositionSink* sink, const std::wstring& text);
    HRESULT do_commit_text(TfEditCookie cookie, ITfContext* context, const std::wstring& text);
    HRESULT do_cancel(TfEditCookie cookie, ITfContext* context);

    ITfComposition* composition_ = nullptr;
};

}  // namespace localpinyin
