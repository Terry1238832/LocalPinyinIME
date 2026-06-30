#include "composition_manager.h"

#include "globals.h"
#include "../common/logging.h"
#include "../common/win32_utils.h"

#include <new>
#include <utility>

namespace localpinyin {

enum class EditAction {
    SetText,
    CommitText,
    Cancel
};

class CompositionEditSession final : public ITfEditSession {
public:
    CompositionEditSession(CompositionManager* manager, ITfContext* context, ITfCompositionSink* sink,
                           std::wstring text, EditAction action)
        : manager_(manager), context_(context), sink_(sink), text_(std::move(text)), action_(action) {
        dll_add_ref();
        if (context_) {
            context_->AddRef();
        }
        if (sink_) {
            sink_->AddRef();
        }
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (!object) {
            return E_POINTER;
        }
        *object = nullptr;
        if (riid == IID_IUnknown || riid == __uuidof(ITfEditSession)) {
            *object = static_cast<ITfEditSession*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return static_cast<ULONG>(InterlockedIncrement(&ref_count_));
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const long count = InterlockedDecrement(&ref_count_);
        if (count == 0) {
            delete this;
        }
        return static_cast<ULONG>(count);
    }

    HRESULT STDMETHODCALLTYPE DoEditSession(TfEditCookie cookie) override {
        if (!manager_) {
            return E_UNEXPECTED;
        }
        switch (action_) {
            case EditAction::SetText:
                return manager_->do_set_text(cookie, context_, sink_, text_);
            case EditAction::CommitText:
                return manager_->do_commit_text(cookie, context_, text_);
            case EditAction::Cancel:
                return manager_->do_cancel(cookie, context_);
        }
        return E_UNEXPECTED;
    }

private:
    ~CompositionEditSession() {
        if (sink_) {
            sink_->Release();
        }
        if (context_) {
            context_->Release();
        }
        dll_release();
    }

    long ref_count_ = 1;
    CompositionManager* manager_ = nullptr;
    ITfContext* context_ = nullptr;
    ITfCompositionSink* sink_ = nullptr;
    std::wstring text_;
    EditAction action_;
};

class CompositionCaretRectEditSession final : public ITfEditSession {
public:
    CompositionCaretRectEditSession(CompositionManager* manager, ITfContext* context, RECT* rect)
        : manager_(manager), context_(context), rect_(rect) {
        dll_add_ref();
        if (context_) {
            context_->AddRef();
        }
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (!object) {
            return E_POINTER;
        }
        *object = nullptr;
        if (riid == IID_IUnknown || riid == __uuidof(ITfEditSession)) {
            *object = static_cast<ITfEditSession*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return static_cast<ULONG>(InterlockedIncrement(&ref_count_));
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const long count = InterlockedDecrement(&ref_count_);
        if (count == 0) {
            delete this;
        }
        return static_cast<ULONG>(count);
    }

    HRESULT STDMETHODCALLTYPE DoEditSession(TfEditCookie cookie) override {
        if (!manager_) {
            return E_UNEXPECTED;
        }
        return manager_->do_caret_rect(cookie, context_, rect_);
    }

private:
    ~CompositionCaretRectEditSession() {
        if (context_) {
            context_->Release();
        }
        dll_release();
    }

    long ref_count_ = 1;
    CompositionManager* manager_ = nullptr;
    ITfContext* context_ = nullptr;
    RECT* rect_ = nullptr;
};

namespace {

HRESULT request_session(ITfContext* context, TfClientId client_id, ITfEditSession* session) {
    HRESULT session_result = E_FAIL;
    const HRESULT hr = context->RequestEditSession(client_id, session, TF_ES_SYNC | TF_ES_READWRITE, &session_result);
    return FAILED(hr) ? hr : session_result;
}

HRESULT request_readonly_session(ITfContext* context, TfClientId client_id, ITfEditSession* session) {
    HRESULT session_result = E_FAIL;
    const HRESULT hr = context->RequestEditSession(client_id, session, TF_ES_SYNC | TF_ES_READ, &session_result);
    return FAILED(hr) ? hr : session_result;
}

void log_composition_hr(const wchar_t* message, HRESULT hr) {
    log_status(L"composition", std::wstring(message) + hresult_hex(hr));
}

HRESULT set_selection_to_composition(ITfContext* context, ITfComposition* composition, TfEditCookie cookie,
                                     TfAnchor anchor, const wchar_t* collapse_log, const wchar_t* selection_log) {
    if (!context || !composition) {
        return E_POINTER;
    }

    ITfRange* composition_range = nullptr;
    HRESULT hr = composition->GetRange(&composition_range);
    if (FAILED(hr)) {
        return hr;
    }

    ITfRange* caret_range = nullptr;
    hr = composition_range->Clone(&caret_range);
    composition_range->Release();
    if (FAILED(hr)) {
        return hr;
    }

    hr = caret_range->Collapse(cookie, anchor);
    if (collapse_log) {
        log_composition_hr(collapse_log, hr);
    }
    if (FAILED(hr)) {
        caret_range->Release();
        return hr;
    }

    TF_SELECTION selection{};
    selection.range = caret_range;
    selection.style.ase = TF_AE_NONE;
    selection.style.fInterimChar = FALSE;

    hr = context->SetSelection(cookie, 1, &selection);
    if (selection_log) {
        log_composition_hr(selection_log, hr);
    }
    caret_range->Release();
    return hr;
}

}  // namespace

CompositionManager::~CompositionManager() {
    clear();
}

void CompositionManager::clear() {
    if (composition_) {
        composition_->Release();
        composition_ = nullptr;
    }
}

HRESULT CompositionManager::set_text(ITfContext* context, TfClientId client_id, ITfCompositionSink* sink, const std::wstring& text) {
    if (!context || !sink) {
        return E_POINTER;
    }
    auto* session = new (std::nothrow) CompositionEditSession(this, context, sink, text, EditAction::SetText);
    if (!session) {
        return E_OUTOFMEMORY;
    }
    const HRESULT hr = request_session(context, client_id, session);
    session->Release();
    return hr;
}

HRESULT CompositionManager::commit_text(ITfContext* context, TfClientId client_id, const std::wstring& text) {
    if (!context) {
        return E_POINTER;
    }
    auto* session = new (std::nothrow) CompositionEditSession(this, context, nullptr, text, EditAction::CommitText);
    if (!session) {
        return E_OUTOFMEMORY;
    }
    const HRESULT hr = request_session(context, client_id, session);
    session->Release();
    return hr;
}

HRESULT CompositionManager::cancel(ITfContext* context, TfClientId client_id) {
    if (!context) {
        return E_POINTER;
    }
    auto* session = new (std::nothrow) CompositionEditSession(this, context, nullptr, L"", EditAction::Cancel);
    if (!session) {
        return E_OUTOFMEMORY;
    }
    const HRESULT hr = request_session(context, client_id, session);
    session->Release();
    return hr;
}

HRESULT CompositionManager::caret_rect(ITfContext* context, TfClientId client_id, RECT* rect) {
    if (!context || !rect) {
        return E_POINTER;
    }
    if (!composition_) {
        return S_FALSE;
    }
    auto* session = new (std::nothrow) CompositionCaretRectEditSession(this, context, rect);
    if (!session) {
        return E_OUTOFMEMORY;
    }
    const HRESULT hr = request_readonly_session(context, client_id, session);
    session->Release();
    return hr;
}

HRESULT CompositionManager::do_set_text(TfEditCookie cookie, ITfContext* context, ITfCompositionSink* sink, const std::wstring& text) {
    ITfRange* range = nullptr;
    if (!composition_) {
        ITfInsertAtSelection* insert = nullptr;
        HRESULT hr = context->QueryInterface(IID_PPV_ARGS(&insert));
        if (FAILED(hr)) {
            return hr;
        }
        hr = insert->InsertTextAtSelection(cookie, 0, text.c_str(), static_cast<LONG>(text.size()), &range);
        insert->Release();
        log_composition_hr(L"update: SetText hr=", hr);
        if (FAILED(hr)) {
            return hr;
        }

        ITfContextComposition* context_composition = nullptr;
        hr = context->QueryInterface(IID_PPV_ARGS(&context_composition));
        if (FAILED(hr)) {
            range->Release();
            return hr;
        }
        hr = context_composition->StartComposition(cookie, range, sink, &composition_);
        context_composition->Release();
        range->Release();
        if (FAILED(hr)) {
            return hr;
        }
        return set_selection_to_composition(context, composition_, cookie, TF_ANCHOR_END,
                                            L"caret: CollapseEnd hr=", L"caret: SetSelection hr=");
    }

    HRESULT hr = composition_->GetRange(&range);
    if (FAILED(hr)) {
        return hr;
    }
    hr = range->SetText(cookie, 0, text.c_str(), static_cast<LONG>(text.size()));
    range->Release();
    log_composition_hr(L"update: SetText hr=", hr);
    if (FAILED(hr)) {
        return hr;
    }
    return set_selection_to_composition(context, composition_, cookie, TF_ANCHOR_END,
                                        L"caret: CollapseEnd hr=", L"caret: SetSelection hr=");
}

HRESULT CompositionManager::do_commit_text(TfEditCookie cookie, ITfContext* context, const std::wstring& text) {
    if (!composition_) {
        return S_FALSE;
    }
    ITfRange* range = nullptr;
    HRESULT hr = composition_->GetRange(&range);
    if (SUCCEEDED(hr)) {
        hr = range->SetText(cookie, 0, text.c_str(), static_cast<LONG>(text.size()));
        range->Release();
    }
    HRESULT selection_hr = S_OK;
    if (SUCCEEDED(hr)) {
        selection_hr = set_selection_to_composition(context, composition_, cookie, TF_ANCHOR_END,
                                                    nullptr, L"commit: SetSelection hr=");
    }
    const HRESULT end_hr = composition_->EndComposition(cookie);
    clear();
    if (FAILED(hr)) {
        return hr;
    }
    if (FAILED(selection_hr)) {
        return selection_hr;
    }
    return end_hr;
}

HRESULT CompositionManager::do_cancel(TfEditCookie cookie, ITfContext* context) {
    if (!composition_) {
        return S_FALSE;
    }
    ITfRange* range = nullptr;
    HRESULT hr = composition_->GetRange(&range);
    if (SUCCEEDED(hr)) {
        hr = range->SetText(cookie, 0, L"", 0);
        range->Release();
    }
    HRESULT selection_hr = S_OK;
    if (SUCCEEDED(hr)) {
        selection_hr = set_selection_to_composition(context, composition_, cookie, TF_ANCHOR_START,
                                                    nullptr, L"cancel: selection restored / hr=");
    }
    const HRESULT end_hr = composition_->EndComposition(cookie);
    clear();
    if (FAILED(hr)) {
        return hr;
    }
    if (FAILED(selection_hr)) {
        return selection_hr;
    }
    return end_hr;
}

HRESULT CompositionManager::do_caret_rect(TfEditCookie cookie, ITfContext* context, RECT* rect) {
    if (!context || !rect || !composition_) {
        return E_POINTER;
    }

    ITfRange* composition_range = nullptr;
    HRESULT hr = composition_->GetRange(&composition_range);
    if (FAILED(hr)) {
        return hr;
    }

    ITfRange* caret_range = nullptr;
    hr = composition_range->Clone(&caret_range);
    composition_range->Release();
    if (FAILED(hr)) {
        return hr;
    }

    hr = caret_range->Collapse(cookie, TF_ANCHOR_END);
    if (FAILED(hr)) {
        caret_range->Release();
        return hr;
    }

    ITfContextView* view = nullptr;
    hr = context->GetActiveView(&view);
    if (FAILED(hr)) {
        caret_range->Release();
        return hr;
    }

    BOOL clipped = FALSE;
    hr = view->GetTextExt(cookie, caret_range, rect, &clipped);
    view->Release();
    caret_range->Release();
    return hr;
}

}  // namespace localpinyin
