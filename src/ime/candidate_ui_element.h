#pragma once

#include "candidate_model.h"

#include <msctf.h>
#include <windows.h>

#include <cstddef>
#include <functional>
#include <string>
#include <utility>

namespace localpinyin {

enum class CandidatePresentationMode {
    None,
    SelfDrawnHwnd,
    HostManagedUIElement
};

enum class CandidatePresentationFallbackReason {
    None,
    UIElementManagerUnavailable,
    BeginFailed,
    NoSelfDrawAnchor,
    NoPresentationPath,
    ModelInactive
};

struct CandidatePresentationDiagnostics {
    bool ui_element_manager_available = false;
    HRESULT ui_element_manager_hr = E_NOINTERFACE;
    bool ui_element_begin_called = false;
    HRESULT ui_element_begin_hr = E_UNEXPECTED;
    DWORD ui_element_id = 0;
    bool ui_element_tip_should_show = true;
    bool ui_element_host_managed = false;
    bool ui_element_update_called = false;
    HRESULT ui_element_update_hr = E_UNEXPECTED;
    bool ui_element_end_called = false;
    HRESULT ui_element_end_hr = E_UNEXPECTED;
    bool candidate_model_active = false;
    std::size_t candidate_model_count = 0;
    std::size_t candidate_model_selection = 0;
    bool candidate_window_self_draw_enabled = true;
    bool candidate_window_anchor_available = false;
    bool candidate_window_visible = false;
    CandidatePresentationMode candidate_presentation_mode = CandidatePresentationMode::None;
    CandidatePresentationFallbackReason candidate_presentation_fallback_reason =
        CandidatePresentationFallbackReason::None;
    bool no_presentation_path = false;
};

const wchar_t* candidate_presentation_mode_text(CandidatePresentationMode mode) noexcept;
const wchar_t* candidate_presentation_fallback_reason_text(
    CandidatePresentationFallbackReason reason) noexcept;
std::wstring make_candidate_presentation_diagnostic_log(
    const CandidatePresentationDiagnostics& diagnostics);

class CandidateListUIElement final : public ITfCandidateListUIElementBehavior {
public:
    explicit CandidateListUIElement(const CandidateModelSnapshot& snapshot);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    HRESULT STDMETHODCALLTYPE GetDescription(BSTR* description) override;
    HRESULT STDMETHODCALLTYPE GetGUID(GUID* guid) override;
    HRESULT STDMETHODCALLTYPE Show(BOOL show) override;
    HRESULT STDMETHODCALLTYPE IsShown(BOOL* show) override;

    HRESULT STDMETHODCALLTYPE GetUpdatedFlags(DWORD* flags) override;
    HRESULT STDMETHODCALLTYPE GetDocumentMgr(ITfDocumentMgr** document_mgr) override;
    HRESULT STDMETHODCALLTYPE GetCount(UINT* count) override;
    HRESULT STDMETHODCALLTYPE GetSelection(UINT* index) override;
    HRESULT STDMETHODCALLTYPE GetString(UINT index, BSTR* value) override;
    HRESULT STDMETHODCALLTYPE GetPageIndex(UINT* index, UINT size, UINT* page_count) override;
    HRESULT STDMETHODCALLTYPE SetPageIndex(UINT* index, UINT page_count) override;
    HRESULT STDMETHODCALLTYPE GetCurrentPage(UINT* page) override;

    HRESULT STDMETHODCALLTYPE SetSelection(UINT index) override;
    HRESULT STDMETHODCALLTYPE Finalize() override;
    HRESULT STDMETHODCALLTYPE Abort() override;

    void update(const CandidateModelSnapshot& snapshot);
    void set_finalize_callback(std::function<void(size_t)> callback) { finalize_callback_ = std::move(callback); }
    void set_abort_callback(std::function<void()> callback) { abort_callback_ = std::move(callback); }
    [[nodiscard]] bool finalized() const noexcept { return finalized_; }
    [[nodiscard]] bool aborted() const noexcept { return aborted_; }

private:
    ~CandidateListUIElement();
    void set_document_mgr(ITfDocumentMgr* document_mgr);

    long ref_count_ = 1;
    CandidateModelSnapshot snapshot_;
    bool shown_ = true;
    DWORD updated_flags_ = TF_CLUIE_DOCUMENTMGR | TF_CLUIE_COUNT | TF_CLUIE_SELECTION |
                           TF_CLUIE_STRING | TF_CLUIE_PAGEINDEX | TF_CLUIE_CURRENTPAGE;
    UINT current_page_ = 0;
    bool finalized_ = false;
    bool aborted_ = false;
    std::function<void(size_t)> finalize_callback_;
    std::function<void()> abort_callback_;
};

class CandidateUIElementSession {
public:
    CandidateUIElementSession() = default;
    ~CandidateUIElementSession();

    CandidateUIElementSession(const CandidateUIElementSession&) = delete;
    CandidateUIElementSession& operator=(const CandidateUIElementSession&) = delete;

    HRESULT begin(ITfThreadMgr* thread_mgr, const CandidateModelSnapshot& snapshot);
    HRESULT begin_with_manager(ITfUIElementMgr* manager,
                               HRESULT manager_hr,
                               const CandidateModelSnapshot& snapshot);
    HRESULT update(const CandidateModelSnapshot& snapshot);
    HRESULT end();
    void reset_without_end();
    void set_finalize_callback(std::function<void(size_t)> callback);
    void set_abort_callback(std::function<void()> callback);

    [[nodiscard]] bool active() const noexcept { return active_; }
    [[nodiscard]] DWORD id() const noexcept { return id_; }
    [[nodiscard]] bool host_managed() const noexcept { return active_ && !tip_should_show_; }
    [[nodiscard]] size_t selected_index() const noexcept {
        UINT index = 0;
        if (element_) element_->GetSelection(&index);
        return index;
    }
    [[nodiscard]] bool tip_should_show() const noexcept { return !active_ || tip_should_show_; }
    [[nodiscard]] CandidateListUIElement* element_for_testing() const noexcept { return element_; }
    [[nodiscard]] const CandidatePresentationDiagnostics& diagnostics() const noexcept {
        return diagnostics_;
    }

    void set_anchor_available(bool available) noexcept {
        diagnostics_.candidate_window_anchor_available = available;
    }
    void set_window_visible(bool visible) noexcept {
        diagnostics_.candidate_window_visible = visible;
    }
    void set_self_draw_enabled(bool enabled) noexcept {
        diagnostics_.candidate_window_self_draw_enabled = enabled;
    }
    void set_presentation(CandidatePresentationMode mode,
                          CandidatePresentationFallbackReason fallback,
                          bool no_presentation_path) noexcept;

private:
    void release_manager() noexcept;
    void release_element() noexcept;
    void update_model_diagnostics(const CandidateModelSnapshot& snapshot) noexcept;

    ITfUIElementMgr* manager_ = nullptr;
    CandidateListUIElement* element_ = nullptr;
    bool active_ = false;
    bool tip_should_show_ = true;
    DWORD id_ = 0;
    CandidatePresentationDiagnostics diagnostics_{};
    std::function<void(size_t)> finalize_callback_;
    std::function<void()> abort_callback_;
};

}  // namespace localpinyin
