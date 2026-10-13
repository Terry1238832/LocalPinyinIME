#include "test_common.h"
#include "../src/ime/candidate_ui_element.h"

#include <ctffunc.h>
#include <msctf.h>

#include <new>
#include <string>
#include <vector>

namespace {

class FakeUIElementMgr final : public ITfUIElementMgr {
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (!object) {
            return E_POINTER;
        }
        *object = nullptr;
        if (riid == IID_IUnknown || riid == __uuidof(ITfUIElementMgr)) {
            *object = static_cast<ITfUIElementMgr*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        ++ref_count;
        return ref_count;
    }

    ULONG STDMETHODCALLTYPE Release() override {
        --ref_count;
        return ref_count;
    }

    HRESULT STDMETHODCALLTYPE BeginUIElement(ITfUIElement* element, BOOL* show, DWORD* id) override {
        ++begin_count;
        if (!element || !show || !id) {
            return E_POINTER;
        }
        if (FAILED(begin_result)) {
            return begin_result;
        }
        last_element = element;
        last_element->AddRef();
        *show = begin_show;
        *id = next_id;
        last_id = *id;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE UpdateUIElement(DWORD id) override {
        ++update_count;
        last_update_id = id;
        return update_result;
    }

    HRESULT STDMETHODCALLTYPE EndUIElement(DWORD id) override {
        ++end_count;
        last_end_id = id;
        if (last_element) {
            last_element->Release();
            last_element = nullptr;
        }
        return end_result;
    }

    HRESULT STDMETHODCALLTYPE GetUIElement(DWORD, ITfUIElement**) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE EnumUIElements(IEnumTfUIElements**) override {
        return E_NOTIMPL;
    }

    ~FakeUIElementMgr() {
        if (last_element) {
            last_element->Release();
        }
    }

    ULONG ref_count = 1;
    int begin_count = 0;
    int update_count = 0;
    int end_count = 0;
    BOOL begin_show = TRUE;
    HRESULT begin_result = S_OK;
    HRESULT update_result = S_OK;
    HRESULT end_result = S_OK;
    DWORD next_id = 42;
    DWORD last_id = 0;
    DWORD last_update_id = 0;
    DWORD last_end_id = 0;
    ITfUIElement* last_element = nullptr;
};

localpinyin::CandidateModelSnapshot snapshot(size_t selection = 0) {
    localpinyin::CandidateModelSnapshot value;
    value.active = true;
    value.composition_text = L"hidden-input";
    value.candidate_texts = {L"\u4F60\u597D", L"\u4E16\u754C"};
    value.selection = selection;
    value.context_identity = 0x1234;
    return value;
}

localpinyin::CandidateModelSnapshot paged_snapshot(size_t selection = 0) {
    auto value = snapshot(selection);
    value.candidate_texts = {L"zero", L"one", L"two", L"three", L"four", L"five",
                             L"six", L"seven", L"eight", L"nine", L"ten"};
    value.page_indices = {0, 4, 8};
    return value;
}

constexpr GUID kUnsupportedInterfaceId{
    0x7db67213, 0x786b, 0x4f33, {0x91, 0x67, 0x7f, 0xa4, 0xd2, 0x91, 0x4e, 0x57}};

int stress_failure(std::size_t iteration, const wchar_t* stage, HRESULT hr) {
    std::wcerr << L"CandidateListUIElement lifecycle stress failed: iteration=" << iteration
               << L" stage=" << stage << L" hr=0x" << std::hex
               << static_cast<unsigned long>(hr) << std::dec << L"\n";
    return 1;
}

}  // namespace

int main() {
    using namespace localpinyin;

    CandidateUIElementSession unavailable;
    REQUIRE_TRUE(FAILED(unavailable.begin_with_manager(nullptr, E_NOINTERFACE, snapshot())));
    REQUIRE_TRUE(!unavailable.active());
    REQUIRE_TRUE(!unavailable.diagnostics().ui_element_manager_available);
    REQUIRE_EQ(unavailable.diagnostics().candidate_presentation_fallback_reason,
               CandidatePresentationFallbackReason::UIElementManagerUnavailable);

    FakeUIElementMgr failing_mgr;
    failing_mgr.begin_result = E_FAIL;
    CandidateUIElementSession begin_failure;
    REQUIRE_EQ(begin_failure.begin_with_manager(&failing_mgr, S_OK, snapshot()), E_FAIL);
    REQUIRE_TRUE(!begin_failure.active());
    REQUIRE_EQ(failing_mgr.begin_count, 1);
    REQUIRE_EQ(begin_failure.diagnostics().candidate_presentation_fallback_reason,
               CandidatePresentationFallbackReason::BeginFailed);

    FakeUIElementMgr self_draw_mgr;
    self_draw_mgr.begin_show = TRUE;
    CandidateUIElementSession self_draw;
    REQUIRE_EQ(self_draw.begin_with_manager(&self_draw_mgr, S_OK, snapshot()), S_OK);
    REQUIRE_TRUE(self_draw.active());
    REQUIRE_TRUE(self_draw.tip_should_show());
    REQUIRE_TRUE(!self_draw.host_managed());
    REQUIRE_EQ(self_draw.id(), 42U);
    REQUIRE_EQ(self_draw.diagnostics().candidate_presentation_mode,
               CandidatePresentationMode::SelfDrawnHwnd);

    std::wcout << L"CandidateListUIElement sizeof=" << sizeof(CandidateListUIElement) << L"\n";
    REQUIRE_TRUE(sizeof(CandidateListUIElement) < 512U);

    CandidateListUIElement* element = self_draw.element_for_testing();
    REQUIRE_TRUE(element != nullptr);

    REQUIRE_EQ(element->QueryInterface(IID_IUnknown, nullptr), E_POINTER);

    void* queried = nullptr;
    REQUIRE_EQ(element->QueryInterface(IID_IUnknown, &queried), S_OK);
    IUnknown* unknown = static_cast<IUnknown*>(queried);
    REQUIRE_TRUE(unknown != nullptr);

    queried = nullptr;
    REQUIRE_EQ(element->QueryInterface(__uuidof(ITfUIElement), &queried), S_OK);
    ITfUIElement* ui_element = static_cast<ITfUIElement*>(queried);
    REQUIRE_TRUE(ui_element != nullptr);

    queried = nullptr;
    REQUIRE_EQ(element->QueryInterface(__uuidof(ITfCandidateListUIElement), &queried), S_OK);
    ITfCandidateListUIElement* candidate_list =
        static_cast<ITfCandidateListUIElement*>(queried);
    REQUIRE_TRUE(candidate_list != nullptr);

    queried = nullptr;
    REQUIRE_EQ(element->QueryInterface(__uuidof(ITfCandidateListUIElementBehavior), &queried),
               S_OK);
    ITfCandidateListUIElementBehavior* behavior =
        static_cast<ITfCandidateListUIElementBehavior*>(queried);
    REQUIRE_TRUE(behavior != nullptr);

    const auto require_same_identity = [&](IUnknown* interface_pointer) -> int {
        void* identity = nullptr;
        REQUIRE_EQ(interface_pointer->QueryInterface(IID_IUnknown, &identity), S_OK);
        REQUIRE_TRUE(identity == unknown);
        static_cast<IUnknown*>(identity)->Release();
        return 0;
    };
    REQUIRE_EQ(require_same_identity(ui_element), 0);
    REQUIRE_EQ(require_same_identity(candidate_list), 0);
    REQUIRE_EQ(require_same_identity(behavior), 0);

    queried = reinterpret_cast<void*>(1);
    REQUIRE_EQ(element->QueryInterface(__uuidof(ITfIntegratableCandidateListUIElement), &queried),
               E_NOINTERFACE);
    REQUIRE_TRUE(queried == nullptr);
    queried = reinterpret_cast<void*>(1);
    REQUIRE_EQ(element->QueryInterface(IID_ITfReadingInformationUIElement, &queried),
               E_NOINTERFACE);
    REQUIRE_TRUE(queried == nullptr);
    queried = reinterpret_cast<void*>(1);
    REQUIRE_EQ(element->QueryInterface(kUnsupportedInterfaceId, &queried), E_NOINTERFACE);
    REQUIRE_TRUE(queried == nullptr);

    behavior->Release();
    candidate_list->Release();
    ui_element->Release();
    unknown->Release();

    const ULONG ref_count_after_add = element->AddRef();
    REQUIRE_TRUE(ref_count_after_add > 1U);
    REQUIRE_EQ(element->Release(), ref_count_after_add - 1U);
    UINT count = 0;
    REQUIRE_EQ(element->GetCount(&count), S_OK);
    REQUIRE_EQ(count, 2U);
    UINT selected = 0;
    REQUIRE_EQ(element->GetSelection(&selected), S_OK);
    REQUIRE_EQ(selected, 0U);
    BSTR value = nullptr;
    REQUIRE_EQ(element->GetString(1, &value), S_OK);
    REQUIRE_TRUE(std::wstring(value) == L"\u4E16\u754C");
    SysFreeString(value);
    REQUIRE_EQ(element->SetSelection(1), S_OK);
    REQUIRE_EQ(element->GetSelection(&selected), S_OK);
    REQUIRE_EQ(selected, 1U);
    REQUIRE_EQ(element->SetSelection(2), E_INVALIDARG);
    REQUIRE_EQ(element->GetSelection(&selected), S_OK);
    REQUIRE_EQ(selected, 1U);
    REQUIRE_EQ(element->Finalize(), S_OK);
    REQUIRE_TRUE(element->finalized());
    REQUIRE_EQ(element->Abort(), S_OK);
    REQUIRE_TRUE(element->aborted());

    REQUIRE_EQ(self_draw.update(snapshot(1)), S_OK);
    REQUIRE_EQ(self_draw_mgr.update_count, 1);
    REQUIRE_EQ(self_draw_mgr.last_update_id, 42U);
    REQUIRE_EQ(element->GetSelection(&selected), S_OK);
    REQUIRE_EQ(selected, 1U);
    REQUIRE_EQ(self_draw.update(snapshot(1)), S_OK);
    REQUIRE_EQ(self_draw_mgr.update_count, 2);
    REQUIRE_EQ(self_draw.end(), S_OK);
    REQUIRE_EQ(self_draw_mgr.end_count, 1);
    REQUIRE_EQ(self_draw_mgr.last_end_id, 42U);
    REQUIRE_TRUE(!self_draw.active());
    REQUIRE_EQ(self_draw.update(snapshot()), S_FALSE);
    REQUIRE_EQ(self_draw_mgr.update_count, 2);
    REQUIRE_EQ(self_draw.end(), S_FALSE);
    REQUIRE_EQ(self_draw_mgr.end_count, 1);

    FakeUIElementMgr host_mgr;
    host_mgr.begin_show = FALSE;
    CandidateUIElementSession host_managed;
    REQUIRE_EQ(host_managed.begin_with_manager(&host_mgr, S_OK, snapshot()), S_OK);
    REQUIRE_TRUE(host_managed.active());
    REQUIRE_TRUE(host_managed.host_managed());
    REQUIRE_TRUE(!host_managed.tip_should_show());
    REQUIRE_EQ(host_managed.diagnostics().candidate_presentation_mode,
               CandidatePresentationMode::HostManagedUIElement);
    host_managed.set_anchor_available(false);
    host_managed.set_window_visible(false);
    host_managed.set_self_draw_enabled(false);
    host_managed.set_presentation(CandidatePresentationMode::HostManagedUIElement,
                                  CandidatePresentationFallbackReason::None,
                                  false);
    const std::wstring diag = make_candidate_presentation_diagnostic_log(host_managed.diagnostics());
    REQUIRE_TRUE(diag.find(L"ui_element_host_managed=true") != std::wstring::npos);
    REQUIRE_TRUE(diag.find(L"candidate_window_self_draw_enabled=false") != std::wstring::npos);
    REQUIRE_TRUE(diag.find(L"hidden-input") == std::wstring::npos);
    REQUIRE_TRUE(diag.find(L"\u4F60\u597D") == std::wstring::npos);
    REQUIRE_EQ(host_managed.end(), S_OK);
    REQUIRE_EQ(host_mgr.end_count, 1);

    // Selection only highlights; explicit finalize commits once. Update arms
    // the next model, and retained host references cannot act after session end.
    FakeUIElementMgr callback_mgr;
    CandidateUIElementSession callback_session;
    int finalized = 0, aborted = 0;
    size_t committed_index = 0;
    callback_session.set_finalize_callback([&](size_t index) { ++finalized; committed_index = index; });
    callback_session.set_abort_callback([&]() { ++aborted; });
    REQUIRE_EQ(callback_session.begin_with_manager(&callback_mgr, S_OK, snapshot()), S_OK);
    auto* callback_element = callback_session.element_for_testing();
    REQUIRE_EQ(callback_element->SetSelection(1), S_OK);
    REQUIRE_EQ(finalized, 0);
    REQUIRE_EQ(callback_element->Finalize(), S_OK);
    REQUIRE_EQ(finalized, 1);
    REQUIRE_EQ(committed_index, size_t{1});
    REQUIRE_EQ(callback_element->Finalize(), S_OK);
    REQUIRE_EQ(finalized, 1);
    REQUIRE_EQ(callback_session.update(snapshot()), S_OK);
    REQUIRE_EQ(callback_element->Abort(), S_OK);
    REQUIRE_EQ(aborted, 1);
    REQUIRE_EQ(callback_element->Abort(), S_OK);
    REQUIRE_EQ(aborted, 1);
    REQUIRE_EQ(callback_session.update(snapshot(1)), S_OK);
    callback_element->AddRef();
    REQUIRE_EQ(callback_session.end(), S_OK);
    REQUIRE_EQ(callback_element->Finalize(), S_OK);
    REQUIRE_EQ(finalized, 1);
    callback_element->Release();

    FakeUIElementMgr paging_mgr;
    CandidateUIElementSession paging_session;
    int page_changes = 0, selection_changes = 0, paging_finalized = 0;
    std::vector<UINT> reported_pages;
    size_t reported_selection = 0, paging_committed_index = 0;
    paging_session.set_page_indices_changed_callback([&](const std::vector<UINT>& pages) {
        ++page_changes;
        reported_pages = pages;
    });
    paging_session.set_selection_changed_callback([&](size_t index) {
        ++selection_changes;
        reported_selection = index;
    });
    paging_session.set_finalize_callback([&](size_t index) {
        ++paging_finalized;
        paging_committed_index = index;
    });
    REQUIRE_EQ(paging_session.begin_with_manager(&paging_mgr, S_OK, paged_snapshot(4)), S_OK);
    auto* paging_element = paging_session.element_for_testing();
    UINT page_count = 0, current_page = 0;
    UINT page_indices[4] = {99, 99, 99, 99};
    REQUIRE_EQ(paging_element->GetPageIndex(page_indices, 4, &page_count), S_OK);
    REQUIRE_EQ(page_count, 3U);
    REQUIRE_EQ(page_indices[0], 0U);
    REQUIRE_EQ(page_indices[1], 4U);
    REQUIRE_EQ(page_indices[2], 8U);
    REQUIRE_EQ(page_indices[3], 99U);
    REQUIRE_EQ(paging_element->GetPageIndex(nullptr, 0, &page_count), S_OK);
    REQUIRE_EQ(page_count, 3U);
    REQUIRE_EQ(paging_element->GetPageIndex(page_indices, 4, nullptr), E_POINTER);
    page_indices[1] = 99;
    REQUIRE_EQ(paging_element->GetPageIndex(page_indices, 1, &page_count), S_OK);
    REQUIRE_EQ(page_count, 3U);
    REQUIRE_EQ(page_indices[0], 0U);
    REQUIRE_EQ(page_indices[1], 99U);
    REQUIRE_EQ(paging_element->GetCurrentPage(&current_page), S_OK);
    REQUIRE_EQ(current_page, 1U);
    REQUIRE_EQ(paging_element->GetCurrentPage(nullptr), E_POINTER);
    REQUIRE_EQ(page_changes, 0);
    REQUIRE_EQ(selection_changes, 0);

    DWORD updated_flags = 0;
    REQUIRE_EQ(paging_element->GetUpdatedFlags(&updated_flags), S_OK);
    UINT nonzero_start[] = {1, 4, 8};
    UINT duplicate_pages[] = {0, 4, 4};
    UINT descending_pages[] = {0, 8, 4};
    UINT outside_model[] = {0, 4, 11};
    REQUIRE_EQ(paging_element->SetPageIndex(nullptr, 3), E_INVALIDARG);
    REQUIRE_EQ(paging_element->SetPageIndex(nullptr, 0), E_INVALIDARG);
    REQUIRE_EQ(paging_element->SetPageIndex(duplicate_pages, 12), E_INVALIDARG);
    REQUIRE_EQ(paging_element->SetPageIndex(nonzero_start, 3), E_INVALIDARG);
    REQUIRE_EQ(paging_element->SetPageIndex(duplicate_pages, 3), E_INVALIDARG);
    REQUIRE_EQ(paging_element->SetPageIndex(descending_pages, 3), E_INVALIDARG);
    REQUIRE_EQ(paging_element->SetPageIndex(outside_model, 3), E_INVALIDARG);
    REQUIRE_EQ(paging_element->GetUpdatedFlags(&updated_flags), S_OK);
    REQUIRE_EQ(updated_flags, 0U);
    REQUIRE_EQ(page_changes, 0);
    REQUIRE_EQ(paging_element->GetPageIndex(page_indices, 4, &page_count), S_OK);
    REQUIRE_EQ(page_count, 3U);
    REQUIRE_EQ(page_indices[1], 4U);
    REQUIRE_EQ(page_indices[2], 8U);

    UINT host_pages[] = {0, 6, 9};
    REQUIRE_EQ(paging_element->SetPageIndex(host_pages, 3), S_OK);
    REQUIRE_EQ(page_changes, 1);
    REQUIRE_TRUE(reported_pages == std::vector<UINT>({0, 6, 9}));
    REQUIRE_EQ(paging_element->GetCurrentPage(&current_page), S_OK);
    REQUIRE_EQ(current_page, 0U);
    REQUIRE_EQ(paging_element->GetUpdatedFlags(&updated_flags), S_OK);
    REQUIRE_EQ(updated_flags, DWORD{TF_CLUIE_PAGEINDEX | TF_CLUIE_CURRENTPAGE});
    REQUIRE_EQ(paging_element->SetSelection(9), S_OK);
    REQUIRE_EQ(selection_changes, 1);
    REQUIRE_EQ(reported_selection, size_t{9});
    REQUIRE_EQ(paging_element->GetSelection(&selected), S_OK);
    REQUIRE_EQ(selected, 9U);
    REQUIRE_EQ(paging_element->GetCurrentPage(&current_page), S_OK);
    REQUIRE_EQ(current_page, 2U);
    REQUIRE_EQ(paging_element->GetUpdatedFlags(&updated_flags), S_OK);
    REQUIRE_EQ(updated_flags, DWORD{TF_CLUIE_SELECTION | TF_CLUIE_CURRENTPAGE});
    REQUIRE_EQ(paging_element->GetString(9, &value), S_OK);
    REQUIRE_TRUE(std::wstring(value) == L"nine");
    SysFreeString(value);
    REQUIRE_EQ(paging_element->Finalize(), S_OK);
    REQUIRE_EQ(paging_finalized, 1);
    REQUIRE_EQ(paging_committed_index, size_t{9});
    REQUIRE_EQ(paging_element->SetSelection(11), E_INVALIDARG);
    REQUIRE_EQ(selection_changes, 1);
    REQUIRE_EQ(paging_element->GetUpdatedFlags(&updated_flags), S_OK);
    REQUIRE_EQ(updated_flags, 0U);

    // Model updates replace host pagination without reporting a host action.
    REQUIRE_EQ(paging_session.update(paged_snapshot(8)), S_OK);
    REQUIRE_EQ(page_changes, 1);
    REQUIRE_EQ(selection_changes, 1);
    REQUIRE_EQ(paging_element->GetPageIndex(page_indices, 4, &page_count), S_OK);
    REQUIRE_EQ(page_indices[1], 4U);
    REQUIRE_EQ(page_indices[2], 8U);
    REQUIRE_EQ(paging_element->GetCurrentPage(&current_page), S_OK);
    REQUIRE_EQ(current_page, 2U);
    auto invalid_snapshot = paged_snapshot(8);
    invalid_snapshot.page_indices = {0, 5, 5};
    REQUIRE_EQ(paging_session.update(invalid_snapshot), S_OK);
    REQUIRE_EQ(paging_element->GetPageIndex(page_indices, 4, &page_count), S_OK);
    REQUIRE_EQ(page_count, 1U);
    REQUIRE_EQ(page_indices[0], 0U);
    REQUIRE_EQ(paging_element->GetCurrentPage(&current_page), S_OK);
    REQUIRE_EQ(current_page, 0U);
    invalid_snapshot.page_indices.clear();
    REQUIRE_EQ(paging_session.update(invalid_snapshot), S_OK);
    REQUIRE_EQ(paging_element->GetPageIndex(page_indices, 4, &page_count), S_OK);
    REQUIRE_EQ(page_count, 1U);

    // Setters also replace callbacks on an element that is already active.
    paging_session.set_page_indices_changed_callback([&](const std::vector<UINT>& pages) {
        page_changes += 10;
        reported_pages = pages;
    });
    paging_session.set_selection_changed_callback([&](size_t index) {
        selection_changes += 10;
        reported_selection = index;
    });
    REQUIRE_EQ(paging_element->SetPageIndex(host_pages, 3), S_OK);
    REQUIRE_EQ(page_changes, 11);
    REQUIRE_EQ(paging_element->SetSelection(10), S_OK);
    REQUIRE_EQ(selection_changes, 11);
    REQUIRE_EQ(reported_selection, size_t{10});
    paging_element->AddRef();
    REQUIRE_EQ(paging_session.end(), S_OK);
    REQUIRE_EQ(paging_element->SetPageIndex(host_pages, 3), S_OK);
    REQUIRE_EQ(paging_element->SetSelection(9), S_OK);
    REQUIRE_EQ(page_changes, 11);
    REQUIRE_EQ(selection_changes, 11);
    paging_element->Release();

    CandidateModelSnapshot empty_snapshot;
    auto* empty_element = new CandidateListUIElement(empty_snapshot);
    REQUIRE_EQ(empty_element->GetPageIndex(page_indices, 4, &page_count), S_OK);
    REQUIRE_EQ(page_count, 0U);
    REQUIRE_EQ(empty_element->SetPageIndex(nullptr, 0), S_OK);
    REQUIRE_EQ(empty_element->SetPageIndex(host_pages, 1), E_INVALIDARG);
    REQUIRE_EQ(empty_element->GetCurrentPage(&current_page), S_OK);
    REQUIRE_EQ(current_page, 0U);
    empty_element->Release();

    constexpr std::size_t kLifecycleStressIterations = 10000;
    for (std::size_t iteration = 0; iteration < kLifecycleStressIterations; ++iteration) {
        auto* stress_element = new (std::nothrow) CandidateListUIElement(snapshot(iteration % 2));
        if (!stress_element) {
            return stress_failure(iteration, L"create", E_OUTOFMEMORY);
        }

        IUnknown* stress_unknown = nullptr;
        ITfUIElement* stress_ui_element = nullptr;
        ITfCandidateListUIElement* stress_candidate_list = nullptr;
        ITfCandidateListUIElementBehavior* stress_behavior = nullptr;
        const auto release_stress_interfaces = [&]() noexcept {
            if (stress_behavior) {
                stress_behavior->Release();
            }
            if (stress_candidate_list) {
                stress_candidate_list->Release();
            }
            if (stress_ui_element) {
                stress_ui_element->Release();
            }
            if (stress_unknown) {
                stress_unknown->Release();
            }
        };
        const auto fail_stress = [&](const wchar_t* stage, HRESULT hr) -> int {
            release_stress_interfaces();
            stress_element->Release();
            return stress_failure(iteration, stage, hr);
        };

        void* stress_queried = nullptr;
        HRESULT hr = stress_element->QueryInterface(IID_IUnknown, &stress_queried);
        if (hr != S_OK || !stress_queried) {
            return fail_stress(L"QI IUnknown", hr);
        }
        stress_unknown = static_cast<IUnknown*>(stress_queried);

        stress_queried = nullptr;
        hr = stress_element->QueryInterface(__uuidof(ITfUIElement), &stress_queried);
        if (hr != S_OK || !stress_queried) {
            return fail_stress(L"QI ITfUIElement", hr);
        }
        stress_ui_element = static_cast<ITfUIElement*>(stress_queried);

        stress_queried = nullptr;
        hr = stress_element->QueryInterface(__uuidof(ITfCandidateListUIElement),
                                            &stress_queried);
        if (hr != S_OK || !stress_queried) {
            return fail_stress(L"QI ITfCandidateListUIElement", hr);
        }
        stress_candidate_list = static_cast<ITfCandidateListUIElement*>(stress_queried);

        stress_queried = nullptr;
        hr = stress_element->QueryInterface(__uuidof(ITfCandidateListUIElementBehavior),
                                            &stress_queried);
        if (hr != S_OK || !stress_queried) {
            return fail_stress(L"QI ITfCandidateListUIElementBehavior", hr);
        }
        stress_behavior = static_cast<ITfCandidateListUIElementBehavior*>(stress_queried);

        stress_queried = reinterpret_cast<void*>(1);
        hr = stress_element->QueryInterface(__uuidof(ITfIntegratableCandidateListUIElement),
                                            &stress_queried);
        if (hr != E_NOINTERFACE || stress_queried != nullptr) {
            return fail_stress(L"QI unsupported Integratable", hr);
        }
        stress_queried = reinterpret_cast<void*>(1);
        hr = stress_element->QueryInterface(IID_ITfReadingInformationUIElement,
                                            &stress_queried);
        if (hr != E_NOINTERFACE || stress_queried != nullptr) {
            return fail_stress(L"QI unsupported ReadingInformation", hr);
        }
        stress_queried = reinterpret_cast<void*>(1);
        hr = stress_element->QueryInterface(kUnsupportedInterfaceId, &stress_queried);
        if (hr != E_NOINTERFACE || stress_queried != nullptr) {
            return fail_stress(L"QI arbitrary unsupported IID", hr);
        }

        release_stress_interfaces();
        if (stress_element->Release() != 0U) {
            return stress_failure(iteration, L"final Release", E_FAIL);
        }
    }
    std::wcout << L"CandidateListUIElement lifecycle stress iterations="
               << kLifecycleStressIterations << L"\n";

    return 0;
}
