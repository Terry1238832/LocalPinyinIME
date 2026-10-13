#include "composition_manager.h"

#include "globals.h"
#include "../common/logging.h"
#include "../common/win32_utils.h"

#include <new>
#include <utility>
#include <initguid.h>
#include <inputscope.h>
#include <oleauto.h>
#include <wrl/client.h>

namespace localpinyin {

enum class EditAction {
    SetText,
    CommitText,
    InsertText,
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
            case EditAction::InsertText:
                return manager_->do_insert_text(cookie, context_, text_);
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
    CompositionCaretRectEditSession(CompositionManager* manager, ITfContext* context, RECT* rect,
                                    CaretRectDiagnostics* diagnostics)
        : manager_(manager), context_(context), rect_(rect), diagnostics_(diagnostics) {
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
        return manager_->do_caret_rect(cookie, context_, rect_, diagnostics_);
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
    CaretRectDiagnostics* diagnostics_ = nullptr;
};

class CompositionSurroundingTextEditSession final : public ITfEditSession {
public:
    CompositionSurroundingTextEditSession(CompositionManager* manager, ITfContext* context,
        std::wstring* text, ITfRange** caret, ITfRange* expected)
        : manager_(manager), context_(context), text_(text), caret_(caret), expected_(expected) { dll_add_ref(); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (iid != IID_IUnknown && iid != __uuidof(ITfEditSession)) return E_NOINTERFACE;
        *out = static_cast<ITfEditSession*>(this); AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&refs_); }
    ULONG STDMETHODCALLTYPE Release() override { const long n = InterlockedDecrement(&refs_); if (!n) delete this; return n; }
    HRESULT STDMETHODCALLTYPE DoEditSession(TfEditCookie cookie) override {
        return manager_->do_read_preceding_text(cookie, context_.Get(), text_, caret_, expected_.Get());
    }
private:
    ~CompositionSurroundingTextEditSession() { dll_release(); }
    long refs_ = 1;
    CompositionManager* manager_;
    Microsoft::WRL::ComPtr<ITfContext> context_;
    std::wstring* text_;
    ITfRange** caret_;
    Microsoft::WRL::ComPtr<ITfRange> expected_;
};

namespace {

EditSessionRequestResult request_session(ITfContext* context, TfClientId client_id, ITfEditSession* session) {
    HRESULT session_result = E_FAIL;
    const HRESULT hr = context->RequestEditSession(client_id, session, TF_ES_SYNC | TF_ES_READWRITE, &session_result);
    return make_edit_session_request_result(hr, session_result);
}

EditSessionRequestResult request_readonly_session(ITfContext* context, TfClientId client_id, ITfEditSession* session) {
    HRESULT session_result = E_FAIL;
    const HRESULT hr = context->RequestEditSession(client_id, session, TF_ES_SYNC | TF_ES_READ, &session_result);
    return make_edit_session_request_result(hr, session_result);
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
        if (composition_range) composition_range->Release();
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

EditSessionRequestResult make_edit_session_request_result(HRESULT request_hr,
                                                          HRESULT session_hr) noexcept {
    return EditSessionRequestResult{request_hr, session_hr, SUCCEEDED(request_hr)};
}

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
    last_edit_session_result_ = request_session(context, client_id, session);
    session->Release();
    return last_edit_session_result_.effective_hr();
}

HRESULT CompositionManager::commit_text(ITfContext* context, TfClientId client_id, const std::wstring& text) {
    if (!context) {
        return E_POINTER;
    }
    auto* session = new (std::nothrow) CompositionEditSession(this, context, nullptr, text, EditAction::CommitText);
    if (!session) {
        return E_OUTOFMEMORY;
    }
    last_edit_session_result_ = request_session(context, client_id, session);
    session->Release();
    return last_edit_session_result_.effective_hr();
}

HRESULT CompositionManager::insert_text(ITfContext* context, TfClientId client_id, const std::wstring& text) {
    if (!context) return E_POINTER;
    if (composition_) return E_UNEXPECTED;
    auto* session = new (std::nothrow) CompositionEditSession(this, context, nullptr, text, EditAction::InsertText);
    if (!session) return E_OUTOFMEMORY;
    last_edit_session_result_ = request_session(context, client_id, session);
    session->Release();
    return last_edit_session_result_.effective_hr();
}

HRESULT CompositionManager::cancel(ITfContext* context, TfClientId client_id) {
    if (!context) {
        return E_POINTER;
    }
    auto* session = new (std::nothrow) CompositionEditSession(this, context, nullptr, L"", EditAction::Cancel);
    if (!session) {
        return E_OUTOFMEMORY;
    }
    last_edit_session_result_ = request_session(context, client_id, session);
    session->Release();
    return last_edit_session_result_.effective_hr();
}

HRESULT CompositionManager::caret_rect(ITfContext* context, TfClientId client_id, RECT* rect,
                                       CaretRectDiagnostics* diagnostics) {
    if (!context || !rect) {
        return E_POINTER;
    }
    auto* session = new (std::nothrow) CompositionCaretRectEditSession(this, context, rect, diagnostics);
    if (!session) {
        return E_OUTOFMEMORY;
    }
    last_edit_session_result_ = request_readonly_session(context, client_id, session);
    session->Release();
    return last_edit_session_result_.effective_hr();
}

HRESULT CompositionManager::read_preceding_text(ITfContext* context, TfClientId client_id,
    std::wstring* text, ITfRange** caret, ITfRange* expected_caret) {
    if (!context || !text) return E_POINTER;
    text->clear();
    if (caret) *caret = nullptr;
    auto* session = new (std::nothrow) CompositionSurroundingTextEditSession(this, context, text, caret, expected_caret);
    if (!session) return E_OUTOFMEMORY;
    last_edit_session_result_ = request_readonly_session(context, client_id, session);
    session->Release();
    return last_edit_session_result_.effective_hr();
}

HRESULT CompositionManager::do_read_preceding_text(TfEditCookie cookie, ITfContext* context,
    std::wstring* text, ITfRange** caret, ITfRange* expected_caret) {
    using Microsoft::WRL::ComPtr;
    TF_STATUS status{};
    HRESULT hr = context->GetStatus(&status);
    if (FAILED(hr)) return hr;
    if (status.dwDynamicFlags & TF_SD_READONLY) return S_FALSE;
    TF_SELECTION selection{};
    ULONG fetched = 0;
    hr = context->GetSelection(cookie, TF_DEFAULT_SELECTION, 1, &selection, &fetched);
    ComPtr<ITfRange> range;
    range.Attach(selection.range);
    if (FAILED(hr)) return hr;
    if (fetched != 1 || !range || selection.style.fInterimChar) return S_FALSE;
    BOOL empty = FALSE;
    hr = range->IsEmpty(cookie, &empty);
    if (FAILED(hr)) return hr;
    if (!empty) return S_FALSE;
    if (expected_caret) {
        LONG comparison = 1;
        hr = range->CompareStart(cookie, expected_caret, TF_ANCHOR_START, &comparison);
        if (FAILED(hr)) return hr;
        if (comparison != 0) return S_FALSE;
    }
    // Input scope is supplied by the host through the real TSF property. Do
    // not inspect preceding characters before checking for protected fields.
    ComPtr<ITfReadOnlyProperty> property;
    hr = context->GetAppProperty(GUID_PROP_INPUTSCOPE, &property);
    if (FAILED(hr)) return hr;
    if (property) {
        VARIANT value; VariantInit(&value);
        hr = property->GetValue(cookie, range.Get(), &value);
        bool protected_input = false;
        if (SUCCEEDED(hr) && value.vt == VT_UNKNOWN && value.punkVal) {
            ComPtr<ITfInputScope> scope;
            hr = value.punkVal->QueryInterface(IID_PPV_ARGS(&scope));
            if (SUCCEEDED(hr)) {
                InputScope* values = nullptr; UINT count = 0;
                hr = scope->GetInputScopes(&values, &count);
                if (SUCCEEDED(hr)) for (UINT i = 0; i < count; ++i)
                    if (values[i] == IS_PASSWORD || values[i] == IS_NUMERIC_PASSWORD) protected_input = true;
                CoTaskMemFree(values);
            }
        }
        VariantClear(&value);
        if (FAILED(hr)) return hr;
        if (protected_input) return S_FALSE;
    }
    ComPtr<ITfRange> caret_copy;
    if (caret) { hr = range->Clone(&caret_copy); if (FAILED(hr)) return hr; }
    LONG shifted = 0;
    hr = range->ShiftStart(cookie, -32, &shifted, nullptr);
    if (FAILED(hr)) return hr;
    wchar_t buffer[32]{}; ULONG copied = 0;
    hr = range->GetText(cookie, 0, buffer, 32, &copied);
    if (FAILED(hr)) return hr;
    text->assign(buffer, copied);
    // A 32-unit range may start halfway through an emoji's surrogate pair.
    if (!text->empty() && text->front() >= 0xDC00 && text->front() <= 0xDFFF) text->erase(0, 1);
    if (caret) *caret = caret_copy.Detach();
    return S_OK;
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

HRESULT CompositionManager::do_insert_text(TfEditCookie cookie, ITfContext* context, const std::wstring& text) {
    ITfInsertAtSelection* insert = nullptr;
    HRESULT hr = context->QueryInterface(IID_PPV_ARGS(&insert));
    if (FAILED(hr)) return hr;
    ITfRange* range = nullptr;
    hr = insert->InsertTextAtSelection(cookie, 0, text.c_str(), static_cast<LONG>(text.size()), &range);
    insert->Release();
    if (FAILED(hr)) return hr;
    hr = range->Collapse(cookie, TF_ANCHOR_END);
    if (SUCCEEDED(hr)) {
        TF_SELECTION selection{};
        selection.range = range;
        selection.style.ase = TF_AE_NONE;
        hr = context->SetSelection(cookie, 1, &selection);
    }
    range->Release();
    return hr;
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

HRESULT CompositionManager::do_caret_rect(TfEditCookie cookie, ITfContext* context, RECT* rect,
                                          CaretRectDiagnostics* diagnostics) {
    if (!context || !rect) {
        return E_POINTER;
    }

    ITfRange* composition_range = nullptr;
    HRESULT hr = S_OK;
    if (composition_) hr = composition_->GetRange(&composition_range);
    else {
        TF_SELECTION selection{}; ULONG fetched = 0;
        hr = context->GetSelection(cookie, TF_DEFAULT_SELECTION, 1, &selection, &fetched);
        composition_range = selection.range;
        if (SUCCEEDED(hr) && (fetched != 1 || !composition_range)) hr = E_FAIL;
    }
    if (FAILED(hr)) {
        if (composition_range) composition_range->Release();
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
    if (diagnostics) {
        diagnostics->active_view_hr = hr;
    }
    if (FAILED(hr)) {
        caret_range->Release();
        return hr;
    }

    HWND context_hwnd = nullptr;
    const HRESULT get_wnd_hr = view->GetWnd(&context_hwnd);
    if (diagnostics) {
        diagnostics->get_wnd_hr = get_wnd_hr;
        diagnostics->context_hwnd_present = SUCCEEDED(get_wnd_hr) && context_hwnd != nullptr;
    }

    BOOL clipped = FALSE;
    hr = view->GetTextExt(cookie, caret_range, rect, &clipped);
    if (diagnostics) {
        diagnostics->get_text_ext_hr = hr;
        diagnostics->get_text_ext_nolayout = hr == TF_E_NOLAYOUT;
        diagnostics->text_ext_clipped = clipped != FALSE;
    }
    view->Release();
    caret_range->Release();
    return hr;
}

}  // namespace localpinyin
