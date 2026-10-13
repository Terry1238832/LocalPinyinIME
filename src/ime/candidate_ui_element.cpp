#include "candidate_ui_element.h"

#include "globals.h"
#include "../common/win32_utils.h"

#include <algorithm>
#include <limits>
#include <new>
#include <oleauto.h>
#include <sstream>

namespace localpinyin {
namespace {

constexpr GUID kCandidateListUIElementGuid{
    0x1c6e9f5b, 0x2dbd, 0x4f5f, {0x92, 0x68, 0xa3, 0x7d, 0xf9, 0x0e, 0x4d, 0x9a}};

const wchar_t* bool_text(bool value) noexcept {
    return value ? L"true" : L"false";
}

}  // namespace

const wchar_t* candidate_presentation_mode_text(CandidatePresentationMode mode) noexcept {
    switch (mode) {
        case CandidatePresentationMode::None:
            return L"none";
        case CandidatePresentationMode::SelfDrawnHwnd:
            return L"self_drawn_hwnd";
        case CandidatePresentationMode::HostManagedUIElement:
            return L"host_managed_ui_element";
    }
    return L"unknown";
}

const wchar_t* candidate_presentation_fallback_reason_text(
    CandidatePresentationFallbackReason reason) noexcept {
    switch (reason) {
        case CandidatePresentationFallbackReason::None:
            return L"none";
        case CandidatePresentationFallbackReason::UIElementManagerUnavailable:
            return L"ui_element_manager_unavailable";
        case CandidatePresentationFallbackReason::BeginFailed:
            return L"ui_element_begin_failed";
        case CandidatePresentationFallbackReason::NoSelfDrawAnchor:
            return L"no_self_draw_anchor";
        case CandidatePresentationFallbackReason::NoPresentationPath:
            return L"no_presentation_path";
        case CandidatePresentationFallbackReason::ModelInactive:
            return L"model_inactive";
    }
    return L"unknown";
}

std::wstring make_candidate_presentation_diagnostic_log(
    const CandidatePresentationDiagnostics& diagnostics) {
    std::wstringstream stream;
    stream << L"ui_element_manager_available=" << bool_text(diagnostics.ui_element_manager_available)
           << L" ui_element_manager_hr=" << hresult_hex(diagnostics.ui_element_manager_hr)
           << L" ui_element_begin_called=" << bool_text(diagnostics.ui_element_begin_called)
           << L" ui_element_begin_hr=" << hresult_hex(diagnostics.ui_element_begin_hr)
           << L" ui_element_id=" << diagnostics.ui_element_id
           << L" ui_element_tip_should_show=" << bool_text(diagnostics.ui_element_tip_should_show)
           << L" ui_element_host_managed=" << bool_text(diagnostics.ui_element_host_managed)
           << L" ui_element_update_called=" << bool_text(diagnostics.ui_element_update_called)
           << L" ui_element_update_hr=" << hresult_hex(diagnostics.ui_element_update_hr)
           << L" ui_element_end_called=" << bool_text(diagnostics.ui_element_end_called)
           << L" ui_element_end_hr=" << hresult_hex(diagnostics.ui_element_end_hr)
           << L" candidate_model_active=" << bool_text(diagnostics.candidate_model_active)
           << L" candidate_model_count=" << diagnostics.candidate_model_count
           << L" candidate_model_selection=" << diagnostics.candidate_model_selection
           << L" candidate_window_self_draw_enabled="
           << bool_text(diagnostics.candidate_window_self_draw_enabled)
           << L" candidate_window_anchor_available="
           << bool_text(diagnostics.candidate_window_anchor_available)
           << L" candidate_window_visible=" << bool_text(diagnostics.candidate_window_visible)
           << L" candidate_presentation_mode="
           << candidate_presentation_mode_text(diagnostics.candidate_presentation_mode)
           << L" candidate_presentation_fallback_reason="
           << candidate_presentation_fallback_reason_text(
                  diagnostics.candidate_presentation_fallback_reason)
           << L" no_presentation_path=" << bool_text(diagnostics.no_presentation_path);
    return stream.str();
}

CandidateListUIElement::CandidateListUIElement(const CandidateModelSnapshot& snapshot) {
    dll_add_ref();
    update(snapshot);
}

CandidateListUIElement::~CandidateListUIElement() {
    release_candidate_model_snapshot(snapshot_);
    dll_release();
}

HRESULT CandidateListUIElement::QueryInterface(REFIID riid, void** object) {
    if (!object) {
        return E_POINTER;
    }
    *object = nullptr;
    if (riid == IID_IUnknown) {
        *object = static_cast<ITfCandidateListUIElement*>(this);
    } else if (riid == __uuidof(ITfUIElement)) {
        *object = static_cast<ITfCandidateListUIElement*>(this);
    } else if (riid == __uuidof(ITfCandidateListUIElement)) {
        *object = static_cast<ITfCandidateListUIElement*>(this);
    } else if (riid == __uuidof(ITfCandidateListUIElementBehavior)) {
        *object = static_cast<ITfCandidateListUIElementBehavior*>(this);
    } else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG CandidateListUIElement::AddRef() {
    return static_cast<ULONG>(InterlockedIncrement(&ref_count_));
}

ULONG CandidateListUIElement::Release() {
    const long count = InterlockedDecrement(&ref_count_);
    if (count == 0) {
        delete this;
    }
    return static_cast<ULONG>(count);
}

HRESULT CandidateListUIElement::GetDescription(BSTR* description) {
    if (!description) {
        return E_POINTER;
    }
    *description = SysAllocString(L"LocalPinyinIME Candidate List");
    return *description ? S_OK : E_OUTOFMEMORY;
}

HRESULT CandidateListUIElement::GetGUID(GUID* guid) {
    if (!guid) {
        return E_POINTER;
    }
    *guid = kCandidateListUIElementGuid;
    return S_OK;
}

HRESULT CandidateListUIElement::Show(BOOL show) {
    shown_ = show != FALSE;
    return S_OK;
}

HRESULT CandidateListUIElement::IsShown(BOOL* show) {
    if (!show) {
        return E_POINTER;
    }
    *show = shown_ ? TRUE : FALSE;
    return S_OK;
}

HRESULT CandidateListUIElement::GetUpdatedFlags(DWORD* flags) {
    if (!flags) {
        return E_POINTER;
    }
    *flags = updated_flags_;
    updated_flags_ = 0;
    return S_OK;
}

HRESULT CandidateListUIElement::GetDocumentMgr(ITfDocumentMgr** document_mgr) {
    if (!document_mgr) {
        return E_POINTER;
    }
    *document_mgr = snapshot_.document_mgr;
    if (*document_mgr) {
        (*document_mgr)->AddRef();
    }
    return S_OK;
}

HRESULT CandidateListUIElement::GetCount(UINT* count) {
    if (!count) {
        return E_POINTER;
    }
    *count = static_cast<UINT>((std::min)(snapshot_.candidate_texts.size(),
                                          static_cast<std::size_t>((std::numeric_limits<UINT>::max)())));
    return S_OK;
}

HRESULT CandidateListUIElement::GetSelection(UINT* index) {
    if (!index) {
        return E_POINTER;
    }
    *index = static_cast<UINT>((std::min)(snapshot_.selection,
                                          snapshot_.candidate_texts.empty()
                                              ? std::size_t{0}
                                              : snapshot_.candidate_texts.size() - 1));
    return S_OK;
}

HRESULT CandidateListUIElement::GetString(UINT index, BSTR* value) {
    if (!value) {
        return E_POINTER;
    }
    *value = nullptr;
    if (index >= snapshot_.candidate_texts.size()) {
        return E_INVALIDARG;
    }
    *value = SysAllocString(snapshot_.candidate_texts[index].c_str());
    return *value ? S_OK : E_OUTOFMEMORY;
}

HRESULT CandidateListUIElement::GetPageIndex(UINT* index, UINT size, UINT* page_count) {
    if (!page_count) {
        return E_POINTER;
    }
    if (index && size > 0) {
        index[0] = 0;
    }
    *page_count = snapshot_.candidate_texts.empty() ? 0 : 1;
    return S_OK;
}

HRESULT CandidateListUIElement::SetPageIndex(UINT*, UINT page_count) {
    if (page_count > 1) {
        return E_INVALIDARG;
    }
    updated_flags_ |= TF_CLUIE_PAGEINDEX | TF_CLUIE_CURRENTPAGE;
    return S_OK;
}

HRESULT CandidateListUIElement::GetCurrentPage(UINT* page) {
    if (!page) {
        return E_POINTER;
    }
    *page = current_page_;
    return S_OK;
}

HRESULT CandidateListUIElement::SetSelection(UINT index) {
    if (index >= snapshot_.candidate_texts.size()) {
        return E_INVALIDARG;
    }
    snapshot_.selection = index;
    updated_flags_ |= TF_CLUIE_SELECTION;
    return S_OK;
}

HRESULT CandidateListUIElement::Finalize() {
    const bool notify = !finalized_ && !aborted_;
    finalized_ = true;
    const auto callback = finalize_callback_;
    if (notify && callback) callback(snapshot_.selection);
    return S_OK;
}

HRESULT CandidateListUIElement::Abort() {
    const bool notify = !finalized_ && !aborted_;
    aborted_ = true;
    const auto callback = abort_callback_;
    if (notify && callback) callback();
    return S_OK;
}

void CandidateListUIElement::update(const CandidateModelSnapshot& snapshot) {
    finalized_ = false;
    aborted_ = false;
    release_candidate_model_snapshot(snapshot_);
    snapshot_.active = snapshot.active;
    snapshot_.composition_text = snapshot.composition_text;
    snapshot_.candidate_texts = snapshot.candidate_texts;
    snapshot_.selection = snapshot.selection;
    snapshot_.context_identity = snapshot.context_identity;
    set_document_mgr(snapshot.document_mgr);
    if (!snapshot_.candidate_texts.empty() && snapshot_.selection >= snapshot_.candidate_texts.size()) {
        snapshot_.selection = snapshot_.candidate_texts.size() - 1;
    }
    updated_flags_ |= TF_CLUIE_DOCUMENTMGR | TF_CLUIE_COUNT | TF_CLUIE_SELECTION |
                      TF_CLUIE_STRING | TF_CLUIE_PAGEINDEX | TF_CLUIE_CURRENTPAGE;
}

void CandidateListUIElement::set_document_mgr(ITfDocumentMgr* document_mgr) {
    snapshot_.document_mgr = document_mgr;
    if (snapshot_.document_mgr) {
        snapshot_.document_mgr->AddRef();
    }
}

CandidateUIElementSession::~CandidateUIElementSession() {
    end();
    release_element();
    release_manager();
}

HRESULT CandidateUIElementSession::begin(ITfThreadMgr* thread_mgr,
                                         const CandidateModelSnapshot& snapshot) {
    ITfUIElementMgr* manager = nullptr;
    HRESULT manager_hr = E_POINTER;
    if (thread_mgr) {
        manager_hr = thread_mgr->QueryInterface(IID_PPV_ARGS(&manager));
    }
    const HRESULT hr = begin_with_manager(manager, manager_hr, snapshot);
    if (manager) {
        manager->Release();
    }
    return hr;
}

HRESULT CandidateUIElementSession::begin_with_manager(ITfUIElementMgr* manager,
                                                      HRESULT manager_hr,
                                                      const CandidateModelSnapshot& snapshot) {
    update_model_diagnostics(snapshot);
    diagnostics_.ui_element_manager_hr = manager_hr;
    diagnostics_.ui_element_manager_available = SUCCEEDED(manager_hr) && manager != nullptr;
    if (active_) {
        return update(snapshot);
    }
    if (!snapshot.active) {
        diagnostics_.candidate_presentation_fallback_reason =
            CandidatePresentationFallbackReason::ModelInactive;
        return S_FALSE;
    }
    if (FAILED(manager_hr) || !manager) {
        diagnostics_.candidate_presentation_fallback_reason =
            CandidatePresentationFallbackReason::UIElementManagerUnavailable;
        return FAILED(manager_hr) ? manager_hr : E_NOINTERFACE;
    }

    release_manager();
    manager_ = manager;
    manager_->AddRef();
    release_element();
    element_ = new (std::nothrow) CandidateListUIElement(snapshot);
    if (!element_) {
        return E_OUTOFMEMORY;
    }
    element_->set_finalize_callback(finalize_callback_);
    element_->set_abort_callback(abort_callback_);

    BOOL show = TRUE;
    DWORD id = 0;
    diagnostics_.ui_element_begin_called = true;
    const HRESULT begin_hr = manager_->BeginUIElement(element_, &show, &id);
    diagnostics_.ui_element_begin_hr = begin_hr;
    diagnostics_.ui_element_tip_should_show = show != FALSE;
    diagnostics_.ui_element_host_managed = show == FALSE;
    id_ = SUCCEEDED(begin_hr) ? id : 0;
    tip_should_show_ = show != FALSE;
    if (FAILED(begin_hr)) {
        diagnostics_.candidate_presentation_fallback_reason =
            CandidatePresentationFallbackReason::BeginFailed;
        release_element();
        release_manager();
        return begin_hr;
    }
    active_ = true;
    diagnostics_.ui_element_id = id_;
    diagnostics_.candidate_presentation_mode =
        tip_should_show_ ? CandidatePresentationMode::SelfDrawnHwnd
                         : CandidatePresentationMode::HostManagedUIElement;
    diagnostics_.candidate_presentation_fallback_reason =
        CandidatePresentationFallbackReason::None;
    diagnostics_.no_presentation_path = false;
    return S_OK;
}

void CandidateUIElementSession::set_finalize_callback(std::function<void(size_t)> callback) {
    finalize_callback_ = std::move(callback);
    if (element_) element_->set_finalize_callback(finalize_callback_);
}

void CandidateUIElementSession::set_abort_callback(std::function<void()> callback) {
    abort_callback_ = std::move(callback);
    if (element_) element_->set_abort_callback(abort_callback_);
}

HRESULT CandidateUIElementSession::update(const CandidateModelSnapshot& snapshot) {
    update_model_diagnostics(snapshot);
    if (!active_ || !manager_ || !element_) {
        return S_FALSE;
    }
    element_->update(snapshot);
    diagnostics_.ui_element_update_called = true;
    diagnostics_.ui_element_update_hr = manager_->UpdateUIElement(id_);
    return diagnostics_.ui_element_update_hr;
}

HRESULT CandidateUIElementSession::end() {
    if (!active_) {
        return S_FALSE;
    }
    diagnostics_.ui_element_end_called = true;
    diagnostics_.ui_element_end_hr = manager_ ? manager_->EndUIElement(id_) : E_UNEXPECTED;
    active_ = false;
    id_ = 0;
    tip_should_show_ = true;
    diagnostics_.ui_element_id = 0;
    diagnostics_.ui_element_host_managed = false;
    diagnostics_.candidate_presentation_mode = CandidatePresentationMode::None;
    release_element();
    release_manager();
    return diagnostics_.ui_element_end_hr;
}

void CandidateUIElementSession::reset_without_end() {
    active_ = false;
    id_ = 0;
    tip_should_show_ = true;
    release_element();
    release_manager();
}

void CandidateUIElementSession::set_presentation(CandidatePresentationMode mode,
                                                 CandidatePresentationFallbackReason fallback,
                                                 bool no_presentation_path) noexcept {
    diagnostics_.candidate_presentation_mode = mode;
    diagnostics_.candidate_presentation_fallback_reason = fallback;
    diagnostics_.no_presentation_path = no_presentation_path;
}

void CandidateUIElementSession::release_manager() noexcept {
    if (manager_) {
        manager_->Release();
        manager_ = nullptr;
    }
}

void CandidateUIElementSession::release_element() noexcept {
    if (element_) {
        element_->set_finalize_callback({});
        element_->set_abort_callback({});
        element_->Release();
        element_ = nullptr;
    }
}

void CandidateUIElementSession::update_model_diagnostics(
    const CandidateModelSnapshot& snapshot) noexcept {
    diagnostics_.candidate_model_active = snapshot.active;
    diagnostics_.candidate_model_count = snapshot.count();
    diagnostics_.candidate_model_selection = snapshot.selection;
}

}  // namespace localpinyin
