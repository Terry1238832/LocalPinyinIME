#include "test_common.h"
#include "../src/ime/preserved_key.h"
#include "../src/ime/text_service.h"
#include "../src/ime/tsf_profile_categories.h"

#include <msctf.h>

#include <cstddef>

namespace {

class FakeKeystrokeMgr final : public ITfKeystrokeMgr {
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (!object) {
            return E_POINTER;
        }
        *object = nullptr;
        if (riid == IID_IUnknown || riid == __uuidof(ITfKeystrokeMgr)) {
            *object = static_cast<ITfKeystrokeMgr*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return 2;
    }

    ULONG STDMETHODCALLTYPE Release() override {
        return 1;
    }

    HRESULT STDMETHODCALLTYPE AdviseKeyEventSink(TfClientId tid, ITfKeyEventSink* sink, BOOL foreground) override {
        advised_tid = tid;
        advised_sink = sink;
        advised_foreground = foreground;
        ++advise_count;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE UnadviseKeyEventSink(TfClientId tid) override {
        unadvised_tid = tid;
        ++unadvise_count;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetForeground(CLSID*) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE TestKeyDown(WPARAM, LPARAM, BOOL*) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE TestKeyUp(WPARAM, LPARAM, BOOL*) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE KeyDown(WPARAM, LPARAM, BOOL*) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE KeyUp(WPARAM, LPARAM, BOOL*) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetPreservedKey(ITfContext*, const TF_PRESERVEDKEY*, GUID*) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE IsPreservedKey(REFGUID, const TF_PRESERVEDKEY*, BOOL*) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE PreserveKey(TfClientId tid,
                                          REFGUID guid,
                                          const TF_PRESERVEDKEY* key,
                                          const WCHAR* description,
                                          ULONG description_length) override {
        if (!key || !description) {
            return E_POINTER;
        }
        preserved_tid = tid;
        preserved_guid = guid;
        preserved_key = *key;
        preserved_description_length = description_length;
        ++preserve_count;
        return preserve_result;
    }

    HRESULT STDMETHODCALLTYPE UnpreserveKey(REFGUID guid, const TF_PRESERVEDKEY* key) override {
        if (!key) {
            return E_POINTER;
        }
        unpreserved_guid = guid;
        unpreserved_key = *key;
        ++unpreserve_count;
        return unpreserve_result;
    }

    HRESULT STDMETHODCALLTYPE SetPreservedKeyDescription(REFGUID, const WCHAR*, ULONG) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetPreservedKeyDescription(REFGUID, BSTR*) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE SimulatePreservedKey(ITfContext*, REFGUID, BOOL*) override {
        return E_NOTIMPL;
    }

    int advise_count = 0;
    int unadvise_count = 0;
    int preserve_count = 0;
    int unpreserve_count = 0;
    TfClientId advised_tid = TF_CLIENTID_NULL;
    TfClientId unadvised_tid = TF_CLIENTID_NULL;
    TfClientId preserved_tid = TF_CLIENTID_NULL;
    ITfKeyEventSink* advised_sink = nullptr;
    BOOL advised_foreground = FALSE;
    GUID preserved_guid = GUID_NULL;
    GUID unpreserved_guid = GUID_NULL;
    TF_PRESERVEDKEY preserved_key{};
    TF_PRESERVEDKEY unpreserved_key{};
    ULONG preserved_description_length = 0;
    HRESULT preserve_result = S_OK;
    HRESULT unpreserve_result = S_OK;
};

class FakeThreadMgr final : public ITfThreadMgr {
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (!object) {
            return E_POINTER;
        }
        *object = nullptr;
        if (riid == IID_IUnknown || riid == __uuidof(ITfThreadMgr)) {
            *object = static_cast<ITfThreadMgr*>(this);
            AddRef();
            return S_OK;
        }
        if (riid == __uuidof(ITfKeystrokeMgr)) {
            return keystroke_mgr.QueryInterface(riid, object);
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return 2;
    }

    ULONG STDMETHODCALLTYPE Release() override {
        return 1;
    }

    HRESULT STDMETHODCALLTYPE Activate(TfClientId* tid) override {
        if (!tid) {
            return E_POINTER;
        }
        *tid = 7;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Deactivate() override {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CreateDocumentMgr(ITfDocumentMgr**) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE EnumDocumentMgrs(IEnumTfDocumentMgrs**) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetFocus(ITfDocumentMgr**) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE SetFocus(ITfDocumentMgr*) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE AssociateFocus(HWND, ITfDocumentMgr*, ITfDocumentMgr**) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE IsThreadFocus(BOOL*) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetFunctionProvider(REFCLSID, ITfFunctionProvider**) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE EnumFunctionProviders(IEnumTfFunctionProviders**) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetGlobalCompartment(ITfCompartmentMgr**) override {
        return E_NOTIMPL;
    }

    FakeKeystrokeMgr keystroke_mgr;
};

}  // namespace

int main() {
    FakeThreadMgr thread_mgr;
    auto* service = new localpinyin::TextService();
    constexpr TfClientId kClientId = 123;

    REQUIRE_EQ(service->ActivateEx(&thread_mgr, kClientId, 0), S_OK);
    REQUIRE_EQ(thread_mgr.keystroke_mgr.advise_count, 1);
    REQUIRE_EQ(thread_mgr.keystroke_mgr.advised_tid, kClientId);
    REQUIRE_TRUE(thread_mgr.keystroke_mgr.advised_sink != nullptr);
    REQUIRE_EQ(thread_mgr.keystroke_mgr.advised_foreground, TRUE);
    REQUIRE_EQ(thread_mgr.keystroke_mgr.preserve_count, 1);
    REQUIRE_EQ(thread_mgr.keystroke_mgr.preserved_tid, kClientId);
    REQUIRE_TRUE(localpinyin::same_guid(thread_mgr.keystroke_mgr.preserved_guid,
                                        localpinyin::GUID_LocalPinyinCtrlSpacePreservedKey));
    REQUIRE_TRUE(localpinyin::is_ctrl_space_preserved_key(thread_mgr.keystroke_mgr.preserved_key));
    REQUIRE_TRUE(thread_mgr.keystroke_mgr.preserved_description_length > 0);

    BOOL eaten = FALSE;
    REQUIRE_EQ(service->OnTestKeyDown(nullptr, L'A', 0, &eaten), S_OK);
    REQUIRE_EQ(eaten, TRUE);
    REQUIRE_EQ(service->OnPreservedKey(nullptr, localpinyin::GUID_LocalPinyinCtrlSpacePreservedKey, &eaten), S_OK);
    REQUIRE_EQ(eaten, TRUE);
    REQUIRE_EQ(service->OnTestKeyDown(nullptr, L'A', 0, &eaten), S_OK);
    REQUIRE_EQ(eaten, FALSE);
    REQUIRE_EQ(service->OnPreservedKey(nullptr, localpinyin::GUID_LocalPinyinCtrlSpacePreservedKey, &eaten), S_OK);
    REQUIRE_EQ(eaten, TRUE);
    REQUIRE_EQ(service->OnTestKeyDown(nullptr, L'A', 0, &eaten), S_OK);
    REQUIRE_EQ(eaten, FALSE);
    REQUIRE_EQ(service->OnKeyUp(nullptr, VK_SPACE, 0, &eaten), S_OK);
    REQUIRE_EQ(service->OnPreservedKey(nullptr, GUID_NULL, &eaten), S_OK);
    REQUIRE_EQ(eaten, FALSE);
    REQUIRE_EQ(service->OnTestKeyDown(nullptr, L'A', 0, &eaten), S_OK);
    REQUIRE_EQ(eaten, FALSE);
    REQUIRE_EQ(service->OnPreservedKey(nullptr, localpinyin::GUID_LocalPinyinCtrlSpacePreservedKey, &eaten), S_OK);
    REQUIRE_EQ(eaten, TRUE);
    REQUIRE_EQ(service->OnTestKeyDown(nullptr, L'A', 0, &eaten), S_OK);
    REQUIRE_EQ(eaten, TRUE);

    REQUIRE_EQ(service->OnTestKeyDown(nullptr, VK_BACK, 0, &eaten), S_OK);
    REQUIRE_EQ(eaten, FALSE);

    REQUIRE_EQ(service->Deactivate(), S_OK);
    REQUIRE_EQ(thread_mgr.keystroke_mgr.unpreserve_count, 1);
    REQUIRE_EQ(thread_mgr.keystroke_mgr.unadvise_count, 1);
    REQUIRE_EQ(thread_mgr.keystroke_mgr.unadvised_tid, kClientId);
    REQUIRE_TRUE(localpinyin::same_guid(thread_mgr.keystroke_mgr.unpreserved_guid,
                                        localpinyin::GUID_LocalPinyinCtrlSpacePreservedKey));
    REQUIRE_TRUE(localpinyin::is_ctrl_space_preserved_key(thread_mgr.keystroke_mgr.unpreserved_key));

    service->Release();

    FakeThreadMgr failing_thread_mgr;
    failing_thread_mgr.keystroke_mgr.preserve_result = E_FAIL;
    auto* failing_service = new localpinyin::TextService();
    REQUIRE_EQ(failing_service->ActivateEx(&failing_thread_mgr, kClientId, 0), E_FAIL);
    REQUIRE_EQ(failing_thread_mgr.keystroke_mgr.preserve_count, 1);
    failing_service->Release();

    const auto& capabilities = localpinyin::required_tsf_profile_capabilities();
    REQUIRE_EQ(capabilities.size(), static_cast<size_t>(4));
    bool has_keyboard = false;
    bool has_immersive = false;
    bool has_ui_element = false;
    bool has_systray = false;
    for (const auto& capability : capabilities) {
        REQUIRE_TRUE(capability.category_guid != nullptr);
        REQUIRE_TRUE(capability.item_guid != nullptr);
        has_keyboard = has_keyboard || IsEqualGUID(*capability.category_guid, GUID_TFCAT_TIP_KEYBOARD);
        has_immersive = has_immersive || IsEqualGUID(*capability.category_guid, GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT);
        has_ui_element = has_ui_element || IsEqualGUID(*capability.category_guid, GUID_TFCAT_TIPCAP_UIELEMENTENABLED);
        has_systray = has_systray || IsEqualGUID(*capability.category_guid, GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT);
        REQUIRE_TRUE(!IsEqualGUID(*capability.category_guid, GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT));
        REQUIRE_TRUE(!IsEqualGUID(*capability.category_guid, GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER));
    }
    REQUIRE_TRUE(has_keyboard);
    REQUIRE_TRUE(has_immersive);
    REQUIRE_TRUE(has_ui_element);
    REQUIRE_TRUE(has_systray);
    const auto& registerable_categories = localpinyin::required_tsf_profile_categories();
    REQUIRE_EQ(registerable_categories.size(), static_cast<size_t>(1));
    for (const auto& category : registerable_categories) {
        REQUIRE_TRUE(category.allow_register_category);
        REQUIRE_EQ(category.check_strategy, localpinyin::TsfCapabilityCheckStrategy::CategoryContainsItem);
        REQUIRE_TRUE(!IsEqualGUID(*category.category_guid, GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT));
    }
    REQUIRE_EQ((localpinyin::required_tsf_profile_caps() & TF_IPP_CAPS_IMMERSIVESUPPORT),
               static_cast<DWORD>(TF_IPP_CAPS_IMMERSIVESUPPORT));
    REQUIRE_EQ((localpinyin::required_tsf_profile_caps() & TF_IPP_CAPS_UIELEMENTENABLED),
               static_cast<DWORD>(TF_IPP_CAPS_UIELEMENTENABLED));
    REQUIRE_EQ((localpinyin::required_tsf_profile_caps() & TF_IPP_CAPS_SYSTRAYSUPPORT),
               static_cast<DWORD>(TF_IPP_CAPS_SYSTRAYSUPPORT));
    return 0;
}
