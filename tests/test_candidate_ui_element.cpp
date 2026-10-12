#include "test_common.h"
#include "../src/ime/candidate_ui_element.h"

#include <ctffunc.h>
#include <msctf.h>

#include <new>
#include <string>

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
    REQUIRE_TRUE(sizeof(CandidateListUIElement) < 200U);

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
