#include "test_common.h"
#include "../src/ime/preserved_key.h"
#include "../src/ime/text_service.h"
#include "../src/ime/tsf_profile_categories.h"

#include <msctf.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

struct FakeKeyState {
    std::array<bool, 256> down{};

    static bool read(int virtual_key, void* context) noexcept {
        const auto* state = static_cast<const FakeKeyState*>(context);
        return state && virtual_key >= 0 &&
               static_cast<std::size_t>(virtual_key) < state->down.size() &&
               state->down[static_cast<std::size_t>(virtual_key)];
    }

    void set(int virtual_key, bool is_down) noexcept {
        if (virtual_key >= 0 && static_cast<std::size_t>(virtual_key) < down.size()) {
            down[static_cast<std::size_t>(virtual_key)] = is_down;
        }
    }

    void release_all() noexcept {
        down.fill(false);
    }
};

const wchar_t* bool_name(BOOL value) noexcept {
    return value == TRUE ? L"TRUE" : L"FALSE";
}

bool require_eaten(const wchar_t* stage, BOOL actual, BOOL expected) {
    if (actual == expected) {
        return true;
    }
    std::wcerr << L"Requirement failed: stage=" << stage
               << L" expected=" << bool_name(expected)
               << L" actual=" << bool_name(actual) << L"\n";
    return false;
}

#define REQUIRE_EATEN(stage, actual, expected) \
    do { if (!require_eaten(L##stage, (actual), (expected))) { return 1; } } while (false)

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
        if (preserve_count == 0) {
            preserved_guid = guid;
            preserved_key = *key;
        }
        preserved_guids.push_back(guid);
        preserved_keys.push_back(*key);
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
    std::vector<GUID> preserved_guids;
    std::vector<TF_PRESERVEDKEY> preserved_keys;
    TF_PRESERVEDKEY preserved_key{};
    TF_PRESERVEDKEY unpreserved_key{};
    ULONG preserved_description_length = 0;
    HRESULT preserve_result = S_OK;
    HRESULT unpreserve_result = S_OK;
};

class FakeThreadMgr final : public ITfThreadMgr, public ITfLangBarItemMgr {
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
        if (riid == __uuidof(ITfLangBarItemMgr)) {
            *object = static_cast<ITfLangBarItemMgr*>(this);
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

    HRESULT STDMETHODCALLTYPE EnumItems(IEnumTfLangBarItems**) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetItem(REFGUID, ITfLangBarItem**) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE AddItem(ITfLangBarItem* item) override {
        added_item = item;
        ++add_item_count;
        return add_item_result;
    }

    HRESULT STDMETHODCALLTYPE RemoveItem(ITfLangBarItem* item) override {
        removed_item = item;
        ++remove_item_count;
        return remove_item_result;
    }

    HRESULT STDMETHODCALLTYPE AdviseItemSink(ITfLangBarItemSink*, DWORD*, REFGUID) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE UnadviseItemSink(DWORD) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetItemFloatingRect(DWORD, REFGUID, RECT*) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetItemsStatus(ULONG, const GUID*, DWORD*) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetItemNum(ULONG*) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetItems(ULONG,
                                       ITfLangBarItem**,
                                       TF_LANGBARITEMINFO*,
                                       DWORD*,
                                       ULONG*) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE AdviseItemsSink(ULONG,
                                             ITfLangBarItemSink**,
                                             const GUID*,
                                             DWORD*) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE UnadviseItemsSink(ULONG, DWORD*) override {
        return E_NOTIMPL;
    }

    FakeKeystrokeMgr keystroke_mgr;
    int add_item_count = 0;
    int remove_item_count = 0;
    HRESULT add_item_result = S_OK;
    HRESULT remove_item_result = S_OK;
    ITfLangBarItem* added_item = nullptr;
    ITfLangBarItem* removed_item = nullptr;
};

}  // namespace

int main() {
    FakeThreadMgr thread_mgr;
    auto* service = new localpinyin::TextService();
    FakeKeyState key_state;
    service->set_key_state_reader_for_testing(&FakeKeyState::read, &key_state);
    constexpr TfClientId kClientId = 123;

    REQUIRE_EQ(service->ActivateEx(&thread_mgr, kClientId, 0), S_OK);
    REQUIRE_EQ(thread_mgr.add_item_count, 1);
    REQUIRE_TRUE(thread_mgr.added_item != nullptr);
    REQUIRE_EQ(thread_mgr.keystroke_mgr.advise_count, 1);
    REQUIRE_EQ(thread_mgr.keystroke_mgr.advised_tid, kClientId);
    REQUIRE_TRUE(thread_mgr.keystroke_mgr.advised_sink != nullptr);
    REQUIRE_EQ(thread_mgr.keystroke_mgr.advised_foreground, TRUE);
    // Ctrl+Space first, then the key that switches to 本地英文 (Alt+CapsLock).
    REQUIRE_EQ(thread_mgr.keystroke_mgr.preserve_count, 2);
    REQUIRE_TRUE(localpinyin::same_guid(thread_mgr.keystroke_mgr.preserved_guids[1],
                                        localpinyin::GUID_LocalPinyinSwitchProfilePreservedKey));
    REQUIRE_TRUE(thread_mgr.keystroke_mgr.preserved_keys[1].uVKey == VK_CAPITAL &&
                 thread_mgr.keystroke_mgr.preserved_keys[1].uModifiers == TF_MOD_ALT);
    REQUIRE_EQ(thread_mgr.keystroke_mgr.preserved_tid, kClientId);
    REQUIRE_TRUE(localpinyin::same_guid(thread_mgr.keystroke_mgr.preserved_guid,
                                        localpinyin::GUID_LocalPinyinCtrlSpacePreservedKey));
    REQUIRE_TRUE(localpinyin::is_ctrl_space_preserved_key(thread_mgr.keystroke_mgr.preserved_key));
    REQUIRE_TRUE(thread_mgr.keystroke_mgr.preserved_description_length > 0);

    const auto left_ctrl_modifiers = localpinyin::modifier_key_state_from_readings(
        localpinyin::ModifierKeyReadings{false, true, false});
    REQUIRE_TRUE(left_ctrl_modifiers.ctrl);
    REQUIRE_TRUE(localpinyin::is_ctrl_space(VK_SPACE, left_ctrl_modifiers));
    const auto right_ctrl_modifiers = localpinyin::modifier_key_state_from_readings(
        localpinyin::ModifierKeyReadings{false, false, true});
    REQUIRE_TRUE(right_ctrl_modifiers.ctrl);
    REQUIRE_TRUE(localpinyin::is_ctrl_space(VK_SPACE, right_ctrl_modifiers));
    const auto generic_ctrl_modifiers = localpinyin::modifier_key_state_from_readings(
        localpinyin::ModifierKeyReadings{true, false, false});
    REQUIRE_TRUE(generic_ctrl_modifiers.ctrl);
    REQUIRE_TRUE(localpinyin::is_ctrl_space(VK_SPACE, generic_ctrl_modifiers));
    auto shifted_ctrl = left_ctrl_modifiers;
    shifted_ctrl.shift = true;
    REQUIRE_TRUE(!localpinyin::is_ctrl_space(VK_SPACE, shifted_ctrl));
    localpinyin::CtrlSpaceKeyDownState accepted_by_test;
    accepted_by_test.accepted_by_test_key_down = true;
    REQUIRE_TRUE(localpinyin::should_handle_ctrl_space_key_down(VK_SPACE,
                                                                localpinyin::ModifierKeyState{},
                                                                accepted_by_test));
    REQUIRE_TRUE(!localpinyin::should_handle_ctrl_space_key_down(L'A',
                                                                 localpinyin::ModifierKeyState{},
                                                                 accepted_by_test));

    localpinyin::CtrlSpaceToggleTracker tracker;
    REQUIRE_TRUE(localpinyin::ctrl_space_toggle_should_fire(tracker, true));
    REQUIRE_TRUE(!localpinyin::ctrl_space_toggle_should_fire(tracker, true));
    localpinyin::ctrl_space_toggle_on_key_up(tracker, VK_CONTROL, false, false);
    REQUIRE_TRUE(localpinyin::ctrl_space_toggle_should_fire(tracker, true));
    REQUIRE_TRUE(!localpinyin::ctrl_space_toggle_should_fire(tracker, true));
    localpinyin::ctrl_space_toggle_on_key_up(tracker, VK_SPACE, false, false);
    REQUIRE_TRUE(localpinyin::ctrl_space_toggle_should_fire(tracker, true));

    BOOL eaten = FALSE;
    key_state.release_all();
    REQUIRE_EQ(service->OnTestKeyDown(nullptr, L'A', 0, &eaten), S_OK);
    REQUIRE_EATEN("initial_chinese_letter", eaten, TRUE);

    key_state.set(VK_CONTROL, true);
    REQUIRE_EQ(service->OnTestKeyDown(nullptr, L'A', 0, &eaten), S_OK);
    REQUIRE_EATEN("ctrl_letter_pass_through", eaten, FALSE);
    key_state.release_all();
    key_state.set(VK_MENU, true);
    REQUIRE_EQ(service->OnTestKeyDown(nullptr, L'A', 0, &eaten), S_OK);
    REQUIRE_EATEN("alt_letter_pass_through", eaten, FALSE);
    key_state.release_all();
    key_state.set(VK_LWIN, true);
    REQUIRE_EQ(service->OnTestKeyDown(nullptr, L'A', 0, &eaten), S_OK);
    REQUIRE_EATEN("win_letter_pass_through", eaten, FALSE);

    key_state.release_all();
    key_state.set(VK_CONTROL, true);
    key_state.set(VK_SPACE, true);
    REQUIRE_EQ(service->OnPreservedKey(nullptr, localpinyin::GUID_LocalPinyinCtrlSpacePreservedKey, &eaten), S_OK);
    REQUIRE_EATEN("first_preserved_key_consumed", eaten, TRUE);
    REQUIRE_EQ(service->OnPreservedKey(nullptr, localpinyin::GUID_LocalPinyinCtrlSpacePreservedKey, &eaten), S_OK);
    REQUIRE_EATEN("repeated_preserved_key_consumed", eaten, TRUE);

    key_state.release_all();
    REQUIRE_EQ(service->OnTestKeyDown(nullptr, L'A', 0, &eaten), S_OK);
    REQUIRE_EATEN("english_letter_after_first_toggle", eaten, FALSE);

    key_state.set(VK_CONTROL, true);
    key_state.set(VK_SPACE, true);
    REQUIRE_EQ(service->OnKeyUp(nullptr, VK_SPACE, 0, &eaten), S_OK);
    REQUIRE_EATEN("retained_ctrl_space_key_up", eaten, FALSE);
    key_state.release_all();
    REQUIRE_EQ(service->OnPreservedKey(nullptr, localpinyin::GUID_LocalPinyinCtrlSpacePreservedKey, &eaten), S_OK);
    REQUIRE_EATEN("retained_state_preserved_key_consumed", eaten, TRUE);
    REQUIRE_EQ(service->OnTestKeyDown(nullptr, L'A', 0, &eaten), S_OK);
    REQUIRE_EATEN("retained_state_still_english", eaten, FALSE);

    key_state.release_all();
    REQUIRE_EQ(service->OnKeyUp(nullptr, VK_SPACE, 0, &eaten), S_OK);
    REQUIRE_EATEN("released_ctrl_space_key_up", eaten, FALSE);
    REQUIRE_EQ(service->OnPreservedKey(nullptr, GUID_NULL, &eaten), S_OK);
    REQUIRE_EATEN("unknown_preserved_key_ignored", eaten, FALSE);
    REQUIRE_EQ(service->OnTestKeyDown(nullptr, L'A', 0, &eaten), S_OK);
    REQUIRE_EATEN("english_letter_before_third_toggle", eaten, FALSE);

    key_state.set(VK_CONTROL, true);
    key_state.set(VK_SPACE, true);
    REQUIRE_EQ(service->OnPreservedKey(nullptr, localpinyin::GUID_LocalPinyinCtrlSpacePreservedKey, &eaten), S_OK);
    REQUIRE_EATEN("third_preserved_key_consumed", eaten, TRUE);
    key_state.release_all();
    REQUIRE_EQ(service->OnTestKeyDown(nullptr, L'A', 0, &eaten), S_OK);
    REQUIRE_EATEN("final_chinese_letter", eaten, TRUE);

    REQUIRE_EQ(service->OnTestKeyDown(nullptr, VK_BACK, 0, &eaten), S_OK);
    REQUIRE_EATEN("backspace_without_composition", eaten, FALSE);
    REQUIRE_EQ(service->OnTestKeyDown(nullptr, VK_LEFT, 0, &eaten), S_OK);
    REQUIRE_EATEN("left_test_without_candidates", eaten, FALSE);
    REQUIRE_EQ(service->OnKeyDown(nullptr, VK_LEFT, 0, &eaten), S_OK);
    REQUIRE_EATEN("left_key_without_candidates", eaten, FALSE);
    REQUIRE_EQ(service->OnTestKeyDown(nullptr, VK_RIGHT, 0, &eaten), S_OK);
    REQUIRE_EATEN("right_test_without_candidates", eaten, FALSE);
    REQUIRE_EQ(service->OnKeyDown(nullptr, VK_RIGHT, 0, &eaten), S_OK);
    REQUIRE_EATEN("right_key_without_candidates", eaten, FALSE);

    REQUIRE_EQ(service->Deactivate(), S_OK);
    REQUIRE_EQ(thread_mgr.remove_item_count, 1);
    REQUIRE_EQ(thread_mgr.removed_item, thread_mgr.added_item);
    REQUIRE_EQ(thread_mgr.keystroke_mgr.unpreserve_count, 2);
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

    // With 本地英文 turned off only Ctrl+Space is taken: Alt+CapsLock stays
    // with the application.
    {
        wchar_t temp[MAX_PATH]{};
        GetTempPathW(MAX_PATH, temp);
        const std::filesystem::path root =
            std::filesystem::path(temp) / (L"lp-english-off-" + std::to_wstring(GetCurrentProcessId()));
        std::filesystem::create_directories(root / L"LocalPinyinIME");
        std::ofstream(root / L"LocalPinyinIME" / L"settings.json") << "{\"inputMode\": {\"englishIme\": false}}\n";
        SetEnvironmentVariableW(L"LOCALPINYINIME_LOCALAPPDATA", root.c_str());
        FakeThreadMgr off_thread_mgr;
        auto* off_service = new localpinyin::TextService();
        REQUIRE_EQ(off_service->ActivateEx(&off_thread_mgr, kClientId, 0), S_OK);
        REQUIRE_EQ(off_thread_mgr.keystroke_mgr.preserve_count, 1);
        REQUIRE_TRUE(localpinyin::same_guid(off_thread_mgr.keystroke_mgr.preserved_guids[0],
                                            localpinyin::GUID_LocalPinyinCtrlSpacePreservedKey));
        REQUIRE_EQ(off_service->Deactivate(), S_OK);
        REQUIRE_EQ(off_thread_mgr.keystroke_mgr.unpreserve_count, 1);
        off_service->Release();
        std::error_code ignored;
        std::filesystem::remove_all(root, ignored);
    }

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
    bool registerable_has_keyboard = false;
    bool registerable_has_immersive = false;
    for (const auto& category : registerable_categories) {
        REQUIRE_TRUE(category.allow_register_category);
        REQUIRE_EQ(category.check_strategy, localpinyin::TsfCapabilityCheckStrategy::CategoryContainsItem);
        registerable_has_keyboard = registerable_has_keyboard ||
            IsEqualGUID(*category.category_guid, GUID_TFCAT_TIP_KEYBOARD);
        registerable_has_immersive = registerable_has_immersive ||
            IsEqualGUID(*category.category_guid, GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT);
        REQUIRE_TRUE(!IsEqualGUID(*category.category_guid, GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT));
    }
    REQUIRE_TRUE(registerable_has_keyboard);
    REQUIRE_TRUE(!registerable_has_immersive);
    const auto& compatibility_categories = localpinyin::compatibility_tsf_profile_categories();
    REQUIRE_EQ(compatibility_categories.size(), static_cast<size_t>(1));
    REQUIRE_TRUE(IsEqualGUID(*compatibility_categories[0].category_guid,
                             GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT));
    REQUIRE_EQ((localpinyin::required_tsf_profile_caps() & TF_IPP_CAPS_IMMERSIVESUPPORT),
               static_cast<DWORD>(TF_IPP_CAPS_IMMERSIVESUPPORT));
    REQUIRE_EQ((localpinyin::required_tsf_profile_caps() & TF_IPP_CAPS_UIELEMENTENABLED),
               static_cast<DWORD>(TF_IPP_CAPS_UIELEMENTENABLED));
    REQUIRE_EQ((localpinyin::required_tsf_profile_caps() & TF_IPP_CAPS_SYSTRAYSUPPORT),
               static_cast<DWORD>(TF_IPP_CAPS_SYSTRAYSUPPORT));
    return 0;
}
