#include <windows.h>
#include <msctf.h>
#include <textstor.h>
#include <initguid.h>
#include <inputscope.h>
#include "../src/ime/composition_manager.h"
#include "../src/ime/text_service.h"
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

// This host uses Windows' real TSF context, ranges, compositions and edit sessions.
// The DLL is loaded directly: no installation, registry writes or input injection.
namespace {
using Microsoft::WRL::ComPtr;
constexpr CLSID kService =
    {0x7c0b4b75, 0x80b0, 0x4e1f, {0xa4, 0xa5, 0x4d, 0x49, 0xa5, 0x44, 0x0d, 0x8a}};
constexpr GUID kCtrlSpace =
    {0x1a7e9f54, 0x0e45, 0x43f2, {0x9b, 0x4c, 0x52, 0x9a, 0xa7, 0x23, 0x63, 0x81}};

void check(HRESULT hr, const char* stage) {
    if (FAILED(hr)) {
        std::cerr << stage << " failed: 0x" << std::hex << static_cast<unsigned long>(hr) << '\n';
        throw std::runtime_error(stage);
    }
}
void require(bool value, const char* stage) {
    if (!value) throw std::runtime_error(stage);
}

class HostInputScope final : public ITfInputScope {
public:
    explicit HostInputScope(InputScope scope) : scope_(scope) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (!out) return E_POINTER; *out = nullptr;
        if (iid != IID_IUnknown && iid != __uuidof(ITfInputScope)) return E_NOINTERFACE;
        *out = static_cast<ITfInputScope*>(this); AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs_; }
    ULONG STDMETHODCALLTYPE Release() override { const auto n = --refs_; if (!n) delete this; return n; }
    HRESULT STDMETHODCALLTYPE GetInputScopes(InputScope** values, UINT* count) override {
        if (!values || !count) return E_POINTER;
        *values = static_cast<InputScope*>(CoTaskMemAlloc(sizeof(InputScope)));
        if (!*values) return E_OUTOFMEMORY;
        **values = scope_; *count = 1; return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetPhrase(BSTR** values, UINT* count) override {
        if (!values || !count) return E_POINTER; *values = nullptr; *count = 0; return S_FALSE;
    }
    HRESULT STDMETHODCALLTYPE GetRegularExpression(BSTR* out) override { if (out) *out = nullptr; return S_FALSE; }
    HRESULT STDMETHODCALLTYPE GetSRGS(BSTR* out) override { if (out) *out = nullptr; return S_FALSE; }
    HRESULT STDMETHODCALLTYPE GetXML(BSTR* out) override { if (out) *out = nullptr; return S_FALSE; }
private:
    ULONG refs_ = 1;
    InputScope scope_;
};

class TextStore final : public ITextStoreACP, public ITfContextOwnerCompositionSink {
public:
    explicit TextStore(HWND window) : window_(window) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (iid == IID_IUnknown || iid == __uuidof(ITextStoreACP))
            *out = static_cast<ITextStoreACP*>(this);
        else if (iid == __uuidof(ITfContextOwnerCompositionSink))
            *out = static_cast<ITfContextOwnerCompositionSink*>(this);
        else return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs_; }
    ULONG STDMETHODCALLTYPE Release() override {
        const auto count = --refs_;
        if (!count) delete this;
        return count;
    }
    HRESULT STDMETHODCALLTYPE AdviseSink(REFIID iid, IUnknown* sink, DWORD) override {
        if (iid != __uuidof(ITextStoreACPSink)) return E_INVALIDARG;
        if (!sink) return E_POINTER;
        return sink->QueryInterface(IID_PPV_ARGS(&sink_));
    }
    HRESULT STDMETHODCALLTYPE UnadviseSink(IUnknown*) override { sink_.Reset(); return S_OK; }
    HRESULT STDMETHODCALLTYPE RequestLock(DWORD flags, HRESULT* result) override {
        if (!result || !sink_) return E_INVALIDARG;
        if (reject_readonly && (flags & TS_LF_READWRITE) == TS_LF_READ) { *result = TS_E_NOLOCK; return S_OK; }
        if (lock_) {
            if (flags & TS_LF_SYNC) *result = TS_E_SYNCHRONOUS;
            else { pending_lock_ |= flags; *result = TS_S_ASYNC; }
            return S_OK;
        }
        lock_ = flags;
        *result = sink_->OnLockGranted(flags);
        lock_ = 0;
        while (pending_lock_) {
            const auto pending = pending_lock_;
            pending_lock_ = 0;
            lock_ = pending;
            sink_->OnLockGranted(pending);
            lock_ = 0;
        }
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetStatus(TS_STATUS* status) override {
        if (!status) return E_POINTER;
        status->dwDynamicFlags = read_only ? TS_SD_READONLY : 0;
        status->dwStaticFlags = TS_SS_NOHIDDENTEXT | (opaque ? TS_SS_TRANSITORY : 0);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE QueryInsert(LONG start, LONG end, ULONG count, LONG* out_start, LONG* out_end) override {
        if (!valid(start, end) || !out_start || !out_end) return E_INVALIDARG;
        *out_start = start;
        *out_end = start + static_cast<LONG>(count);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetSelection(ULONG index, ULONG count, TS_SELECTION_ACP* out, ULONG* fetched) override {
        if (!fetched || !out) return E_POINTER;
        *fetched = 0;
        if (index != TS_DEFAULT_SELECTION && index != 0) return E_INVALIDARG;
        if (count) { out[0] = selection_; *fetched = 1; }
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE SetSelection(ULONG count, const TS_SELECTION_ACP* selection) override {
        if (count != 1 || !selection || !valid(selection->acpStart, selection->acpEnd)) return E_INVALIDARG;
        selection_ = *selection;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetText(LONG start, LONG end, WCHAR* plain, ULONG capacity,
        ULONG* copied, TS_RUNINFO* runs, ULONG run_capacity, ULONG* run_count, LONG* next) override {
        if (!copied || !run_count || !next) return E_POINTER;
        if (opaque) {
            // Shows the input method nothing, as older programs do.
            *copied = 0; *run_count = 0; *next = start;
            return S_OK;
        }
        if (plain && capacity) ++plain_text_reads;
        if (end == -1) end = static_cast<LONG>(text.size());
        if (!valid(start, end)) return TS_E_INVALIDPOS;
        const auto length = static_cast<ULONG>(end - start);
        const auto count = plain ? std::min(length, capacity) : length;
        if (plain && count) std::copy_n(text.data() + start, count, plain);
        *copied = plain ? count : 0;
        *run_count = 0;
        if (runs && run_capacity && count) {
            runs[0].uCount = count;
            runs[0].type = TS_RT_PLAIN;
            *run_count = 1;
        }
        *next = start + static_cast<LONG>(count);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE SetText(DWORD, LONG start, LONG end, const WCHAR* value,
        ULONG count, TS_TEXTCHANGE* change) override {
        if (!valid(start, end) || !change || (!value && count)) return E_INVALIDARG;
        text.replace(static_cast<size_t>(start), static_cast<size_t>(end - start), value ? value : L"", count);
        *change = {start, end, start + static_cast<LONG>(count)};
        selection_.acpStart = selection_.acpEnd = change->acpNewEnd;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetFormattedText(LONG, LONG, IDataObject** out) override {
        if (out) *out = nullptr;
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE GetEmbedded(LONG, REFGUID, REFIID, IUnknown** out) override {
        if (out) *out = nullptr;
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE QueryInsertEmbedded(const GUID*, const FORMATETC*, BOOL* allowed) override {
        if (!allowed) return E_POINTER;
        *allowed = FALSE;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE InsertEmbedded(DWORD, LONG, LONG, IDataObject*, TS_TEXTCHANGE*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE InsertTextAtSelection(DWORD flags, const WCHAR* value, ULONG count,
        LONG* start, LONG* end, TS_TEXTCHANGE* change) override {
        const auto insert_start = selection_.acpStart;
        if (start) *start = insert_start;
        if (end) *end = (flags & TS_IAS_QUERYONLY) ? selection_.acpEnd : insert_start + static_cast<LONG>(count);
        if (flags & TS_IAS_QUERYONLY) return S_OK;
        return SetText(0, insert_start, selection_.acpEnd, value, count, change);
    }
    HRESULT STDMETHODCALLTYPE InsertEmbeddedAtSelection(DWORD, IDataObject*, LONG*, LONG*, TS_TEXTCHANGE*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE RequestSupportedAttrs(DWORD, ULONG count, const TS_ATTRID* attributes) override { return request_attributes(count, attributes); }
    HRESULT STDMETHODCALLTYPE RequestAttrsAtPosition(LONG, ULONG count, const TS_ATTRID* attributes, DWORD) override { return request_attributes(count, attributes); }
    HRESULT STDMETHODCALLTYPE RequestAttrsTransitioningAtPosition(LONG, ULONG, const TS_ATTRID*, DWORD) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE FindNextAttrTransition(LONG, LONG halt, ULONG, const TS_ATTRID*, DWORD,
        LONG* next, BOOL* found, LONG* offset) override {
        if (!next || !found || !offset) return E_POINTER;
        *next = halt; *found = FALSE; *offset = 0;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RetrieveRequestedAttrs(ULONG count, TS_ATTRVAL* values, ULONG* fetched) override {
        if (!fetched) return E_POINTER;
        *fetched = 0;
        if (count && values && scope_requested_) {
            values[0].idAttr = GUID_PROP_INPUTSCOPE;
            values[0].dwOverlapId = 0;
            VariantInit(&values[0].varValue);
            values[0].varValue.vt = VT_UNKNOWN;
            values[0].varValue.punkVal = new HostInputScope(password ? IS_PASSWORD : IS_DEFAULT);
            *fetched = 1;
        }
        scope_requested_ = false;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetEndACP(LONG* end) override {
        if (!end) return E_POINTER;
        *end = static_cast<LONG>(text.size());
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetActiveView(TsViewCookie* view) override {
        if (!view) return E_POINTER;
        *view = 1;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetACPFromPoint(TsViewCookie, const POINT*, DWORD, LONG* position) override {
        if (!position) return E_POINTER;
        *position = selection_.acpEnd;
        return S_OK;
    }
    // This hidden test host has no layout unless a test gives it one.
    HRESULT STDMETHODCALLTYPE GetTextExt(TsViewCookie, LONG, LONG, RECT* rect, BOOL* clipped) override {
        if (!layout) return TS_E_NOLAYOUT;
        if (!rect || !clipped) return E_INVALIDARG;
        *rect = *layout;
        *clipped = FALSE;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetScreenExt(TsViewCookie, RECT* rect) override {
        return rect && GetWindowRect(window_, rect) ? S_OK : E_INVALIDARG;
    }
    HRESULT STDMETHODCALLTYPE GetWnd(TsViewCookie, HWND* window) override {
        if (!window) return E_POINTER;
        *window = window_;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnStartComposition(ITfCompositionView*, BOOL* accepted) override {
        if (!accepted) return E_POINTER;
        *accepted = TRUE;
        ++composition_starts;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnUpdateComposition(ITfCompositionView*, ITfRange*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnEndComposition(ITfCompositionView*) override { ++composition_ends; return S_OK; }
    std::wstring text;
    void move_selection(LONG start, LONG end) {
        require(valid(start, end), "test selection bounds");
        selection_.acpStart = start; selection_.acpEnd = end;
        if (sink_) check(sink_->OnSelectionChange(), "host selection change notification");
    }
    void host_replace(const std::wstring& value) {
        const LONG previous = static_cast<LONG>(text.size());
        text = value;
        selection_.acpStart = selection_.acpEnd = static_cast<LONG>(text.size());
        TS_TEXTCHANGE change{0, previous, static_cast<LONG>(text.size())};
        if (sink_) {
            check(sink_->OnTextChange(0, &change), "host text change notification");
            check(sink_->OnSelectionChange(), "host selection notification");
        }
    }
    bool read_only = false;
    bool reject_readonly = false;
    ULONG plain_text_reads = 0;
    bool password = false;
    void set_password(bool value) {
        password = value;
        const TS_ATTRID attribute = GUID_PROP_INPUTSCOPE;
        if (sink_) check(sink_->OnAttrsChange(0, static_cast<LONG>(text.size()), 1, &attribute), "host input scope change");
    }
    std::optional<RECT> layout;
    bool opaque = false;
    void layout_changed() {
        if (sink_) check(sink_->OnLayoutChange(TS_LC_CHANGE, 1), "host layout change notification");
    }
    void set_read_only(bool value) {
        read_only = value;
        if (sink_) check(sink_->OnStatusChange(value ? TS_SD_READONLY : 0), "host readonly status change");
    }
    unsigned composition_starts = 0;
    unsigned composition_ends = 0;
private:
    HRESULT request_attributes(ULONG count, const TS_ATTRID* attributes) {
        scope_requested_ = false;
        for (ULONG i = 0; i < count; ++i) if (attributes[i] == GUID_PROP_INPUTSCOPE) scope_requested_ = true;
        return S_OK;
    }
    bool scope_requested_ = false;
    bool valid(LONG start, LONG end) const {
        return start >= 0 && end >= start && end <= static_cast<LONG>(text.size());
    }
    ULONG refs_ = 1;
    HWND window_ = nullptr;
    DWORD lock_ = 0;
    DWORD pending_lock_ = 0;
    TS_SELECTION_ACP selection_{0, 0, {TS_AE_NONE, FALSE}};
    ComPtr<ITextStoreACPSink> sink_;
};

// Windows rejects AdviseKeyEventSink for a TIP not loaded through its registered
// profile. This adapter supplies only that registration bookkeeping in memory;
// document/context/range/composition/edit-session operations remain system TSF.
class ManagerAdapter final : public ITfThreadMgr, public ITfKeystrokeMgr, public ITfUIElementMgr {
public:
    explicit ManagerAdapter(ITfThreadMgr* system) : system_(system) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (iid == IID_IUnknown || iid == __uuidof(ITfThreadMgr)) *out = static_cast<ITfThreadMgr*>(this);
        else if (iid == __uuidof(ITfKeystrokeMgr)) *out = static_cast<ITfKeystrokeMgr*>(this);
        else if (iid == __uuidof(ITfUIElementMgr)) *out = static_cast<ITfUIElementMgr*>(this);
        else return system_->QueryInterface(iid, out);
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs_; }
    ULONG STDMETHODCALLTYPE Release() override { const auto n = --refs_; if (!n) delete this; return n; }
    HRESULT STDMETHODCALLTYPE Activate(TfClientId* id) override { return system_->Activate(id); }
    HRESULT STDMETHODCALLTYPE Deactivate() override { return system_->Deactivate(); }
    HRESULT STDMETHODCALLTYPE CreateDocumentMgr(ITfDocumentMgr** out) override { return system_->CreateDocumentMgr(out); }
    HRESULT STDMETHODCALLTYPE EnumDocumentMgrs(IEnumTfDocumentMgrs** out) override { return system_->EnumDocumentMgrs(out); }
    HRESULT STDMETHODCALLTYPE GetFocus(ITfDocumentMgr** out) override { return system_->GetFocus(out); }
    HRESULT STDMETHODCALLTYPE SetFocus(ITfDocumentMgr* doc) override { return system_->SetFocus(doc); }
    HRESULT STDMETHODCALLTYPE AssociateFocus(HWND hwnd, ITfDocumentMgr* doc, ITfDocumentMgr** old) override {
        return system_->AssociateFocus(hwnd, doc, old);
    }
    HRESULT STDMETHODCALLTYPE IsThreadFocus(BOOL* out) override { return system_->IsThreadFocus(out); }
    HRESULT STDMETHODCALLTYPE GetFunctionProvider(REFCLSID id, ITfFunctionProvider** out) override {
        return system_->GetFunctionProvider(id, out);
    }
    HRESULT STDMETHODCALLTYPE EnumFunctionProviders(IEnumTfFunctionProviders** out) override {
        return system_->EnumFunctionProviders(out);
    }
    HRESULT STDMETHODCALLTYPE GetGlobalCompartment(ITfCompartmentMgr** out) override { return system_->GetGlobalCompartment(out); }
    HRESULT STDMETHODCALLTYPE AdviseKeyEventSink(TfClientId, ITfKeyEventSink* sink, BOOL) override {
        if (!sink) return E_POINTER;
        sink_ = sink;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE UnadviseKeyEventSink(TfClientId) override { sink_.Reset(); return S_OK; }
    HRESULT STDMETHODCALLTYPE GetForeground(CLSID* out) override { if (!out) return E_POINTER; *out = kService; return S_OK; }
    HRESULT STDMETHODCALLTYPE TestKeyDown(WPARAM, LPARAM, BOOL*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE TestKeyUp(WPARAM, LPARAM, BOOL*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE KeyDown(WPARAM, LPARAM, BOOL*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE KeyUp(WPARAM, LPARAM, BOOL*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetPreservedKey(ITfContext*, const TF_PRESERVEDKEY*, GUID*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE IsPreservedKey(REFGUID, const TF_PRESERVEDKEY*, BOOL*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE PreserveKey(TfClientId, REFGUID, const TF_PRESERVEDKEY*, const WCHAR*, ULONG) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE UnpreserveKey(REFGUID, const TF_PRESERVEDKEY*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE SetPreservedKeyDescription(REFGUID, const WCHAR*, ULONG) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetPreservedKeyDescription(REFGUID, BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SimulatePreservedKey(ITfContext*, REFGUID, BOOL*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE BeginUIElement(ITfUIElement* element, BOOL* show, DWORD* id) override {
        if (!element || !show || !id) return E_POINTER;
        current_element = element; *show = self_draw ? TRUE : FALSE; *id = ++element_id_;
        if (synchronous_page_callback) {
            ComPtr<ITfCandidateListUIElementBehavior> behavior;
            if (SUCCEEDED(current_element.As(&behavior))) {
                UINT page = 0;
                check(behavior->SetPageIndex(&page, 1), "synchronous Begin host pages");
                check(behavior->SetSelection(0), "synchronous Begin host selection");
            }
        }
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE UpdateUIElement(DWORD) override {
        ++updates;
        if (synchronous_page_callback && current_element) {
            ComPtr<ITfCandidateListUIElementBehavior> behavior;
            if (SUCCEEDED(current_element.As(&behavior))) {
                UINT page = 0;
                check(behavior->SetPageIndex(&page, 1), "synchronous Update host pages");
                check(behavior->SetSelection(0), "synchronous Update host selection");
            }
        }
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE EndUIElement(DWORD id) override {
        if (id == element_id_) current_element.Reset();
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetUIElement(DWORD id, ITfUIElement** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (id != element_id_ || !current_element) return E_INVALIDARG;
        *out = current_element.Get(); (*out)->AddRef(); return S_OK;
    }
    HRESULT STDMETHODCALLTYPE EnumUIElements(IEnumTfUIElements** out) override {
        if (out) *out = nullptr; return E_NOTIMPL;
    }
    ComPtr<ITfUIElement> current_element;
    bool synchronous_page_callback = false;
    // The host lets the service draw its own candidate popup.
    bool self_draw = false;
    unsigned int updates = 0;
private:
    DWORD element_id_ = 0;
    ULONG refs_ = 1;
    ComPtr<ITfThreadMgr> system_;
    ComPtr<ITfKeyEventSink> sink_;
};

struct Session {
    HMODULE module = nullptr;
    HWND window = nullptr;
    bool com_initialized = false;
    bool manager_active = false;
    bool service_active = false;
    ComPtr<ITfThreadMgrEx> manager;
    ComPtr<ManagerAdapter> adapter;
    ComPtr<ITfDocumentMgr> document;
    ComPtr<ITfContext> context;
    ComPtr<ITfTextInputProcessor> service;
    ComPtr<ITfKeyEventSink> keys;
    ComPtr<TextStore> store;
    std::array<bool, 256> modifier_keys{};
    static bool test_key_state(int key, void* context) noexcept {
        return key >= 0 && key < 256 && static_cast<Session*>(context)->modifier_keys[static_cast<size_t>(key)];
    }
    std::vector<UINT> pages() {
        ComPtr<ITfCandidateListUIElement> list;
        if (!adapter->current_element) std::wcerr << L"Missing page model; host text=" << store->text << L"\n";
        require(adapter->current_element != nullptr, "page model exists without self-drawn window");
        check(adapter->current_element.As(&list), "paged list");
        UINT count = 0;
        check(list->GetPageIndex(nullptr, 0, &count), "page count");
        std::vector<UINT> result(count);
        check(list->GetPageIndex(result.data(), count, &count), "page boundaries");
        return result;
    }
    UINT current_page() {
        require(adapter->current_element != nullptr, "current page model exists");
        ComPtr<ITfCandidateListUIElement> list;
        check(adapter->current_element.As(&list), "current page list");
        UINT result = 0; check(list->GetCurrentPage(&result), "current page"); return result;
    }
    std::wstring candidate(UINT index) {
        ComPtr<ITfCandidateListUIElement> list;
        check(adapter->current_element.As(&list), "candidate text list");
        BSTR word = nullptr; check(list->GetString(index, &word), "candidate text");
        std::wstring result(word, SysStringLen(word)); SysFreeString(word); return result;
    }
    std::wstring first_candidate() {
        require(adapter->current_element != nullptr, "candidate UI element exists");
        ComPtr<ITfCandidateListUIElement> list;
        check(adapter->current_element.As(&list), "real candidate UI element interface");
        BSTR word = nullptr;
        check(list->GetString(0, &word), "candidate UI first word");
        std::wstring result(word, SysStringLen(word)); SysFreeString(word);
        return result;
    }
    ~Session() {
        if (service_active) service->Deactivate();
        keys.Reset(); service.Reset();
        adapter.Reset();
        if (manager) manager->SetFocus(nullptr);
        if (document) document->Pop(TF_POPF_ALL);
        context.Reset(); document.Reset(); store.Reset();
        if (manager_active) manager->Deactivate();
        manager.Reset();
        if (window) DestroyWindow(window);
        if (module) FreeLibrary(module);
        if (com_initialized) CoUninitialize();
    }
    void key(WPARAM value, bool expected_eaten = true) {
        BOOL tested = FALSE, eaten = FALSE;
        check(keys->OnTestKeyDown(context.Get(), value, 0, &tested), "OnTestKeyDown");
        if ((tested != FALSE) != expected_eaten) {
            std::cerr << "Unexpected key test: vk=" << value << " expected=" << expected_eaten
                      << " eaten=" << tested << " modifiers(ctrl/alt/shift/win)="
                      << GetAsyncKeyState(VK_CONTROL) << '/' << GetAsyncKeyState(VK_MENU) << '/'
                      << GetAsyncKeyState(VK_SHIFT) << '/' << GetAsyncKeyState(VK_LWIN) << '/'
                      << GetAsyncKeyState(VK_RWIN) << '\n';
        }
        require((tested != FALSE) == expected_eaten, "test key consumption");
        check(keys->OnKeyDown(context.Get(), value, 0, &eaten), "OnKeyDown");
        require((eaten != FALSE) == expected_eaten, "key consumption");
        check(keys->OnKeyUp(context.Get(), value, 0, &eaten), "OnKeyUp");
    }
};
} // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc != 2) { std::cerr << "Usage: test_tsf_integration <absolute LocalPinyinIME.dll>\n"; return 2; }
    try {
        wchar_t temporary[MAX_PATH]{};
        require(GetTempPathW(MAX_PATH, temporary) != 0, "test data directory");
        const auto isolated = std::filesystem::path(temporary) / (L"LocalPinyinTsfTest-" + std::to_wstring(GetCurrentProcessId()));
        std::filesystem::create_directories(isolated);
        // Local learning/status written by the service stays out of the user's data.
        require(SetEnvironmentVariableW(L"LOCALAPPDATA", isolated.c_str()) != FALSE, "isolate service data");
        require(SetEnvironmentVariableW(L"LOCALPINYINIME_LOCALAPPDATA", isolated.c_str()) != FALSE, "isolate known-folder data");
        require(SetEnvironmentVariableW(L"LOCALPINYINIME_USER_LEXICON_PATH", (isolated / L"LocalPinyinIME" / L"user_lexicon.tsv").c_str()) != FALSE, "isolate user lexicon");
        const auto native = isolated / L"native";
        std::filesystem::create_directories(native / L"dictionary");
        const auto original = std::filesystem::path(argv[1]);
        const auto isolated_dll = native / L"LocalPinyinIME.dll";
        std::filesystem::copy_file(original, isolated_dll, std::filesystem::copy_options::overwrite_existing);
        {
            // Pin lexical behavior for the host integration contract. Full
            // production text quality is measured by the separate evaluations.
            std::ofstream core(native / L"dictionary" / L"core_zh_pinyin.tsv", std::ios::binary | std::ios::trunc);
            core << "nihao\t你好\t9000\nsuibian\t随便\t9000\npengyou\t朋友\t9000\nni\t你\t9000\ntianqi\t天气\t9000\nweixing\t卫星\t9100\nweixing\t微信\t9000\n";
            for (int i = 0; i < 9; ++i)
                core << "ceshi\t这是用于单行分页完整上屏验证的很长候选文字" << i + 1 << "\t" << 9000 - i * 100 << "\n";
            core << "fenye\t分页\t9000\n";
            // Pieces for building a phrase one pick at a time, each behind a
            // more common homophone.
            core << "liang\t两\t9000\nliang\t量\t8000\njia\t家\t9000\njia\t甲\t8000\n"
                    "cheng\t称\t9000\ncheng\t成\t8000\nyi\t一\t9000\nyi\t以\t8000\n";
            require(static_cast<bool>(core), "write isolated core fixture");
            for (const auto* name : {L"local_core_zh_pinyin.tsv", L"standard_zh_pinyin.tsv", L"context_zh.tsv"}) {
                std::ofstream empty(native / L"dictionary" / name, std::ios::binary | std::ios::trunc);
                empty << "# Isolated TSF fixture\n";
            }
            std::ofstream context(native / L"dictionary" / L"context_zh.tsv", std::ios::binary | std::ios::trunc);
            context << "发送微信\t20000\n气象卫星\t20000\n";
            std::ofstream file(native / L"dictionary" / L"continuations_zh.tsv", std::ios::binary);
            file << "你好\t世界\t10000\n你好\t朋友\t9000\n天气\t不错\t10000\n";
            const char* ordinal[] = {"一", "二", "三", "四", "五", "六", "七", "八", "九"};
            for (int i = 0; i < 9; ++i)
                file << "分页\t这是跨页联想接续完整文字候选" << ordinal[i] << "\t" << 10000 - i * 100 << "\n";
            require(static_cast<bool>(file), "write isolated continuation fixture");
        }
        {
            // Tab twice (the tool panel) has its own section; elsewhere Tab is
            // tested on its own.
            std::filesystem::create_directories(isolated / L"LocalPinyinIME");
            std::ofstream(isolated / L"LocalPinyinIME" / L"settings.json", std::ios::binary | std::ios::trunc)
                << "{\"inputMode\":{\"panelDoubleTab\":false}}";
        }
        {
            Session s;
            check(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED), "CoInitializeEx");
            s.com_initialized = true;
            s.window = CreateWindowExW(0, L"STATIC", L"TSF integration host", WS_POPUP,
                0, 0, 200, 100, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
            require(s.window != nullptr, "hidden host window");
            check(CoCreateInstance(CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&s.manager)), "system TSF manager");
            TfClientId client = TF_CLIENTID_NULL;
            check(s.manager->ActivateEx(&client, TF_TMAE_NOACTIVATETIP), "TSF manager activation");
            s.manager_active = true;
            ComPtr<ITfClientId> client_ids;
            check(s.manager.As(&client_ids), "TSF text service client IDs");
            TfClientId service_client = TF_CLIENTID_NULL;
            check(client_ids->GetClientId(kService, &service_client), "text service client ID");
            check(s.manager->CreateDocumentMgr(&s.document), "real TSF document");
            s.store.Attach(new TextStore(s.window));
            TfEditCookie cookie = 0;
            check(s.document->CreateContext(client, 0, static_cast<ITextStoreACP*>(s.store.Get()),
                &s.context, &cookie), "real TSF context");
            check(s.document->Push(s.context.Get()), "push context");
            check(s.manager->SetFocus(s.document.Get()), "focus test document");
            s.module = LoadLibraryExW(isolated_dll.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
            require(s.module != nullptr, "load IME DLL");
            using GetFactory = HRESULT (STDAPICALLTYPE*)(REFCLSID, REFIID, void**);
            const auto get_factory = reinterpret_cast<GetFactory>(GetProcAddress(s.module, "DllGetClassObject"));
            require(get_factory != nullptr, "DllGetClassObject export");
            ComPtr<IClassFactory> factory;
            check(get_factory(kService, IID_PPV_ARGS(&factory)), "DLL COM factory");
            check(factory->CreateInstance(nullptr, IID_PPV_ARGS(&s.service)), "create COM text service");
            // The in-process object is this repository's TextService. Its
            // existing internal seam keeps modifier tests deterministic while
            // contexts, locks, ranges and compositions remain Windows TSF.
            static_cast<localpinyin::TextService*>(s.service.Get())->set_key_state_reader_for_testing(Session::test_key_state, &s);
            factory.Reset();
            check(s.service.As(&s.keys), "key sink COM interface");
            s.adapter.Attach(new ManagerAdapter(s.manager.Get()));
            check(s.service->Activate(static_cast<ITfThreadMgr*>(s.adapter.Get()), service_client), "text service activation");
            s.service_active = true;
            s.adapter->synchronous_page_callback = true;
            const auto updates_before = s.adapter->updates;
            for (const auto letter : std::wstring(L"CESHI")) s.key(letter);
            require(s.pages() == std::vector<UINT>({0}), "Begin/Update host callbacks preserve supplied pages");
            require(s.adapter->updates - updates_before < 10, "synchronous host page/selection callbacks never recurse");
            s.key(VK_ESCAPE);
            s.adapter->synchronous_page_callback = false;

            // Real TSF paging through the same model while the host owns UI.
            for (const auto letter : std::wstring(L"CESHI")) s.key(letter);
            const auto default_pages = s.pages();
            require(default_pages.size() > 1, "long candidate fixture has multiple measured pages");
            s.key(VK_LEFT);
            require(s.current_page() == 0 && s.store->text == L"ceshi", "left boundary clamps and never moves host caret");
            s.key(VK_RIGHT);
            require(s.current_page() == 1, "right changes candidate page");
            for (size_t page = 2; page < default_pages.size(); ++page) s.key(VK_RIGHT);
            s.key(VK_RIGHT);
            require(s.current_page() == default_pages.size() - 1 && s.store->text == L"ceshi", "right tail boundary clamps without document movement");
            for (size_t page = default_pages.size() - 1; page > 1; --page) s.key(VK_LEFT);
            const auto second_first = s.candidate(default_pages[1]);
            s.key(L'1');
            require(s.store->text == second_first, "page two digit one commits the full original model candidate");
            s.store->host_replace(L"");
            for (const auto letter : std::wstring(L"CESHI")) s.key(letter);
            s.key(VK_RIGHT);
            const auto space_word = s.candidate(s.pages()[1]);
            s.key(VK_SPACE);
            require(s.store->text == space_word, "Space commits the first selection after page change");
            s.store->host_replace(L"");
            for (const auto letter : std::wstring(L"CESHI")) s.key(letter);
            s.key(VK_RIGHT); s.key(VK_LEFT);
            require(s.current_page() == 0, "left returns to previous page");
            s.key(VK_RIGHT); s.key(L'A');
            require(s.current_page() == 0, "new input resets page one");
            s.key(VK_ESCAPE);
            for (const auto letter : std::wstring(L"CESHI")) s.key(letter);
            s.key(VK_RIGHT); s.key(VK_BACK);
            require(s.current_page() == 0, "deleting input resets page one");
            s.key(VK_ESCAPE);
            for (const auto letter : std::wstring(L"CESHI")) s.key(letter);
            ComPtr<ITfCandidateListUIElementBehavior> paged_behavior;
            check(s.adapter->current_element.As(&paged_behavior), "host page behavior");
            UINT host_pages[] = {0, 2, 5};
            check(paged_behavior->SetPageIndex(host_pages, 3), "host page boundaries");
            require(s.pages() == std::vector<UINT>({0, 2, 5}), "host page model matches keyboard pages");
            check(paged_behavior->SetSelection(4), "host selects page two item");
            require(s.current_page() == 1, "host selection changes visible current page");
            const auto host_word = s.candidate(4);
            s.key(VK_SPACE);
            require(s.store->text == host_word, "Space honors host selection");
            paged_behavior.Reset();
            s.store->host_replace(L"");
            for (const auto letter : std::wstring(L"CESHI")) s.key(letter);
            check(s.adapter->current_element.As(&paged_behavior), "host page behavior for digit");
            check(paged_behavior->SetPageIndex(host_pages, 3), "host pages for digit");
            check(paged_behavior->SetSelection(4), "host selects later page two item before digit");
            const auto digit_word = s.candidate(2);
            s.key(L'1');
            require(s.store->text == digit_word, "digit overrides host selection using current page slot");
            paged_behavior.Reset();

            // CapsLock or Shift: a letter outside a composition passes through
            // to the application (which types it in upper case); inside one it
            // keeps its case and the composition is offered as typed text.
            s.store->host_replace(L"");
            for (const auto lock : {VK_CAPITAL, VK_SHIFT}) {
                s.modifier_keys[lock] = true;
                s.key(L'A', false);
                s.modifier_keys[lock] = false;
                require(s.store->text.empty() && !s.adapter->current_element, "literal letter passes through");
            }
            for (const auto letter : std::wstring(L"NI")) s.key(letter);
            s.modifier_keys[VK_SHIFT] = true;
            s.key(L'H');
            s.modifier_keys[VK_SHIFT] = false;
            require(s.first_candidate() == L"niH", "Shift letter inside a composition keeps its case");
            s.key(VK_SPACE);
            require(s.store->text == L"niH", "mixed composition commits as typed");
            s.store->host_replace(L"");

            const auto write_settings = [&](const char* json) {
                std::filesystem::create_directories(isolated / L"LocalPinyinIME");
                std::ofstream options(isolated / L"LocalPinyinIME" / L"settings.json", std::ios::binary | std::ios::trunc);
                options << json;
            };
            // Next-word suggestions are opt-in; enable them for this section.
            write_settings("{\"inputMode\":{\"panelDoubleTab\":false,\"nextWordPrediction\":true}}");
            s.store->host_replace(L"");
            for (const auto letter : std::wstring(L"FENYE")) s.key(letter);
            s.key(VK_SPACE);
            const auto prediction_pages = s.pages();
            require(prediction_pages.size() > 1, "prediction has multiple single-line pages");
            s.key(VK_LEFT);
            require(s.current_page() == 0 && s.store->text == L"分页", "prediction left boundary consumes arrow without document editing");
            for (size_t page = 1; page < prediction_pages.size(); ++page) s.key(VK_RIGHT);
            s.key(VK_RIGHT);
            require(s.current_page() == prediction_pages.size() - 1 && s.store->text == L"分页", "prediction last boundary clamps without document editing");
            for (size_t page = prediction_pages.size() - 1; page > 1; --page) s.key(VK_LEFT);
            const auto extension = s.candidate(prediction_pages[1]);
            require(s.current_page() == 1 && s.store->text == L"分页", "prediction arrows page without inserting or moving caret");
            s.key(VK_TAB);
            require(s.store->text == L"分页" + extension, "Tab inserts full selected continuation across pages");
            s.store->host_replace(L"");

            for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
            require(s.store->text == L"nihao", "real edit sessions update pinyin");
            s.key(VK_SPACE);
            require(s.store->text == L"你好", "real TSF Chinese candidate commit");
            require(s.store->composition_starts >= 1 && s.store->composition_ends >= 1,
                "real TSF composition lifecycle");
            s.key(L'N'); s.key(L'I'); s.key(VK_BACK);
            require(s.store->text == L"你好n", "backspace updates composition");
            s.key(VK_ESCAPE);
            require(s.store->text == L"你好", "Esc removes uncommitted text");
            s.key(L'N'); s.key(L'I'); s.key(VK_RETURN);
            require(s.store->text == L"你好ni", "Enter commits raw pinyin");
            // Unknown trailing syllables stay editable after a prefix selection.
            for (const auto letter : std::wstring(L"NIHAOZZZZ")) s.key(letter);
            s.key(VK_SPACE);
            require(s.store->text == L"你好ni你好zzzz", "partial commit retains unresolved pinyin");
            s.key(VK_ESCAPE);
            require(s.store->text == L"你好ni你好", "Esc only removes retained suffix");
            // Punctuation commits a pending Chinese candidate before insertion,
            // and inserts directly at the caret when no composition is active.
            for (const auto letter : std::wstring(L"SUIBIAN")) s.key(letter);
            s.key(VK_OEM_COMMA);
            require(s.store->text == L"你好ni你好随便，", "punctuation converts then inserts");
            s.key(VK_OEM_PERIOD);
            require(s.store->text == L"你好ni你好随便，。", "idle punctuation insertion");
            // After a digit the application typed, the period key is a
            // decimal point (3.14); after anything else it stays 。.
            const std::wstring before_digit = s.store->text;
            s.store->host_replace(before_digit + L"3");
            s.key(VK_OEM_PERIOD);
            require(s.store->text == before_digit + L"3.", "a period after a digit is a decimal point");
            s.key(VK_OEM_PERIOD);
            require(s.store->text == before_digit + L"3.。", "a period after a period stays Chinese");
            // A comma and a slash after a digit are ASCII too: 1,000 and 1/2.
            s.store->host_replace(before_digit + L"1");
            s.key(VK_OEM_COMMA);
            require(s.store->text == before_digit + L"1,", "a comma after a digit is ASCII");
            s.store->host_replace(before_digit + L"1");
            s.key(VK_OEM_2);
            require(s.store->text == before_digit + L"1/", "a slash after a digit is ASCII");
            // The three punctuation helpers switch separately.
            write_settings("{\"inputMode\":{\"panelDoubleTab\":false,\"nextWordPrediction\":true,\"digitPeriod\":false,\"digitPunctuation\":true,\"letterPunctuation\":true}}");
            s.store->host_replace(L"3");
            s.key(VK_OEM_PERIOD);
            require(s.store->text == L"3\u3002", "with its switch off, the period after a digit stays Chinese");
            s.store->host_replace(L"1");
            s.key(VK_OEM_COMMA);
            require(s.store->text == L"1,", "the comma after a digit follows its own switch");
            s.store->host_replace(L"Hello");
            s.key(VK_OEM_COMMA);
            require(s.store->text == L"Hello,", "after a letter the comma is ASCII");
            s.key(VK_OEM_COMMA);
            require(s.store->text == L"Hello,\uFF0C", "after ASCII punctuation the next one is Chinese again");
            s.store->host_replace(L"OK");
            s.modifier_keys[VK_SHIFT] = true;
            s.key(L'1');
            s.modifier_keys[VK_SHIFT] = false;
            require(s.store->text == L"OK!", "after a letter the exclamation mark is ASCII");
            s.store->host_replace(L"wait");
            s.key(VK_OEM_PERIOD);
            require(s.store->text == L"wait.", "after a letter the period is ASCII");
            // Defaults: the digit helpers on, letters off; an old file with only
            // digitPeriod=false keeps , / : Chinese as well.
            write_settings("{\"inputMode\":{\"panelDoubleTab\":false,\"nextWordPrediction\":true}}");
            s.store->host_replace(L"Hello");
            s.key(VK_OEM_COMMA);
            require(s.store->text == L"Hello\uFF0C", "letters are off by default");
            s.store->host_replace(L"3");
            s.key(VK_OEM_PERIOD);
            require(s.store->text == L"3.", "the digit period is on by default");
            write_settings("{\"inputMode\":{\"panelDoubleTab\":false,\"nextWordPrediction\":true,\"digitPeriod\":false}}");
            s.store->host_replace(L"1");
            s.key(VK_OEM_COMMA);
            require(s.store->text == L"1\uFF0C", "an old combined switch turned off keeps , after a digit Chinese");
            write_settings("{\"inputMode\":{\"panelDoubleTab\":false,\"nextWordPrediction\":true}}");
            s.store->host_replace(before_digit);
            // 造词: 量, 甲 and 成 picked one at a time, with nothing in between,
            // come back as one phrase for liangjiacheng; a punctuation mark
            // between two picks keeps them apart.
            {
                const auto pick = [&](const std::wstring& keys, const std::wstring& word) {
                    for (const auto letter : keys) s.key(letter);
                    const auto pages = s.pages();
                    ComPtr<ITfCandidateListUIElement> list;
                    check(s.adapter->current_element.As(&list), "candidate list for a pick");
                    UINT count = 0;
                    check(list->GetCount(&count), "candidate count for a pick");
                    const UINT first_page = pages.size() > 1 ? pages[1] : count;
                    for (UINT slot = 0; slot < first_page; ++slot) {
                        if (s.candidate(slot) == word) {
                            s.key(static_cast<WPARAM>(L'1' + slot));
                            return;
                        }
                    }
                    std::wcerr << L"pick " << keys << L" text=" << s.store->text << L" page:";
                    for (UINT slot = 0; slot < first_page; ++slot) std::wcerr << L' ' << s.candidate(slot);
                    std::wcerr << std::endl;
                    require(false, "the piece is on the first page");
                };
                s.store->host_replace(L"");
                pick(L"LIANG", L"\u91CF");
                pick(L"JIA", L"\u7532");
                pick(L"CHENG", L"\u6210");
                require(s.store->text == L"\u91CF\u7532\u6210", "pieces picked one at a time");
                s.store->host_replace(L"");
                for (const auto letter : std::wstring(L"LIANGJIACHENG")) s.key(letter);
                require(s.first_candidate() == L"\u91CF\u7532\u6210", "the built phrase comes first next time");
                s.key(VK_ESCAPE);
                s.store->host_replace(L"");
                pick(L"LIANG", L"\u91CF");
                s.key(VK_OEM_COMMA);
                pick(L"YI", L"\u4EE5");
                s.store->host_replace(L"");
                for (const auto letter : std::wstring(L"LIANGYI")) s.key(letter);
                require(s.first_candidate() != L"\u91CF\u4EE5", "punctuation between picks keeps them apart");
                s.key(VK_ESCAPE);
                s.store->host_replace(before_digit);
            }
            // Switching to English keeps the letters typed so far.
            BOOL toggle_eaten = FALSE;
            for (const auto letter : std::wstring(L"HELLO")) s.key(letter);
            check(s.keys->OnPreservedKey(s.context.Get(), kCtrlSpace, &toggle_eaten), "English mode during a composition");
            require(s.store->text == before_digit + L"hello", "mode switch keeps typed letters");
            check(s.keys->OnKeyUp(s.context.Get(), VK_CONTROL, 0, &toggle_eaten), "release the Ctrl mode chord");
            check(s.keys->OnPreservedKey(s.context.Get(), kCtrlSpace, &toggle_eaten), "back to Chinese mode");
            s.store->host_replace(before_digit);
            s.key(VK_ESCAPE, false);
            // A long run must keep every key when the composition buffer rolls over.
            const std::wstring before_long_input = s.store->text;
            for (size_t index = 0; index < 257; ++index) s.key(L'Z');
            require(s.store->text == before_long_input + std::wstring(257, L'z'),
                "composition length boundary preserves the next letter");
            s.key(VK_RETURN);
            const std::wstring before_separator_limit = s.store->text;
            for (size_t index = 0; index < 255; ++index) s.key(L'Z');
            s.key(VK_OEM_7);
            s.key(L'Z');
            require(s.store->text == before_separator_limit + std::wstring(255, L'z') + L"'z",
                "composition length boundary includes separators and preserves raw text");
            s.key(VK_RETURN);
            // Suggestions use document text and a live caret range, never a
            // cached string from the previous composition or another editor.
            s.store->host_replace(L"");
            for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
            s.key(VK_SPACE);
            require(s.store->text == L"你好", "prediction never inserts without acceptance");
            require(s.first_candidate() == L"世界", "stable isolated next-word prediction");
            s.key(VK_TAB);
            require(s.store->text == L"你好世界", "Tab inserts only the extension once");
            s.key(VK_TAB, false);

            s.store->host_replace(std::wstring(48, L'前') + L"你好");
            localpinyin::CompositionManager surrounding_reader;
            std::wstring actual_before;
            check(surrounding_reader.read_preceding_text(s.context.Get(), service_client, &actual_before), "real TSF preceding text API");
            require(actual_before == std::wstring(30, L'前') + L"你好", "preceding text API limits real text to 32 UTF16 units");
            s.key(L'T');
            s.key(VK_ESCAPE);
            s.store->move_selection(0, 0);

            s.store->host_replace(L"发送");
            for (const auto letter : std::wstring(L"WEIXING")) s.key(letter);
            require(s.first_candidate() == L"微信", "lookup ranks homophone using real caret preceding text");
            s.key(VK_ESCAPE);
            s.store->host_replace(L"气象");
            for (const auto letter : std::wstring(L"WEIXING")) s.key(letter);
            require(s.first_candidate() == L"卫星", "new document prefix replaces previous lookup context");
            s.key(VK_ESCAPE);
            s.store->host_replace(L"发送");
            s.store->move_selection(0, 0);
            for (const auto letter : std::wstring(L"WEIXING")) s.key(letter);
            require(s.first_candidate() == L"卫星", "lookup after caret movement does not use stale preceding text");
            s.key(VK_ESCAPE);
            check(surrounding_reader.read_preceding_text(s.context.Get(), service_client, &actual_before), "read moved real caret");
            require(actual_before.empty(), "moving caret reads fresh front-of-document context");
            s.key(L'N');
            s.key(VK_ESCAPE);
            s.store->host_replace(L"此处有密码前文");
            s.store->set_password(true);
            const auto private_reads = s.store->plain_text_reads;
            actual_before = L"previous stale value";
            require(surrounding_reader.read_preceding_text(s.context.Get(), service_client, &actual_before) == S_FALSE && actual_before.empty(),
                "password input scope blocks preceding text and clears stale value");
            require(s.store->plain_text_reads == private_reads, "password scope is checked before private plaintext is read");
            for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
            s.key(VK_SPACE);
            require(!s.adapter->current_element, "password field never exposes automatic predictions");
            s.key(VK_TAB, false);
            s.store->set_password(false);
            s.store->move_selection(0, 3);
            actual_before = L"previous stale value";
            require(surrounding_reader.read_preceding_text(s.context.Get(), service_client, &actual_before) == S_FALSE && actual_before.empty(),
                "nonempty selection never guesses preceding text");
            s.store->move_selection(0, 0);
            s.store->set_read_only(true);
            actual_before = L"previous stale value";
            require(surrounding_reader.read_preceding_text(s.context.Get(), service_client, &actual_before) == S_FALSE && actual_before.empty(),
                "readonly context blocks preceding text");
            s.store->set_read_only(false);
            s.store->reject_readonly = true;
            actual_before = L"previous stale value";
            require(FAILED(surrounding_reader.read_preceding_text(s.context.Get(), service_client, &actual_before)) && actual_before.empty(),
                "failed readonly edit session never uses stale preceding text");
            s.store->reject_readonly = false;
            s.store->host_replace(std::wstring(L"🙂") + std::wstring(31, L'前'));
            check(surrounding_reader.read_preceding_text(s.context.Get(), service_client, &actual_before), "read surrogate boundary");
            require(actual_before == std::wstring(31, L'前'), "32 UTF16 boundary drops an isolated low surrogate");

            s.store->host_replace(L"");
            for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
            s.key(VK_SPACE);
            s.key(VK_ESCAPE);
            require(s.store->text == L"你好", "prediction Escape keeps committed text");
            require(!s.adapter->current_element, "prediction Escape ends candidate UI element");
            s.key(VK_ESCAPE, false);

            for (const auto ordinary : {L'1', static_cast<wchar_t>(VK_SPACE), static_cast<wchar_t>(VK_RETURN),
                    static_cast<wchar_t>(VK_BACK), static_cast<wchar_t>(VK_LEFT), static_cast<wchar_t>(VK_RIGHT),
                    static_cast<wchar_t>(VK_OEM_COMMA)}) {
                s.store->host_replace(L"");
                for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
                s.key(VK_SPACE);
                s.key(ordinary, false);
                require(s.store->text == L"你好", "ordinary editing key is not converted or predicted");
                require(!s.adapter->current_element, "ordinary editing closes prediction UI");
            }
            s.store->host_replace(L"");
            for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
            s.key(VK_SPACE);
            for (const auto letter : std::wstring(L"TIANQI")) s.key(letter);
            require(s.store->text == L"你好tianqi", "letter after suggestion starts ordinary pinyin");
            s.key(VK_SPACE);
            require(s.store->text == L"你好天气" && s.first_candidate() == L"不错", "new composition uses current actual context");
            s.store->move_selection(0, 0);
            require(!s.adapter->current_element, "host caret movement immediately closes prediction");
            s.key(VK_TAB, false);
            require(s.store->text == L"你好天气", "moved caret cannot accept an old suggestion");

            s.store->host_replace(L"");
            for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
            s.key(VK_SPACE);
            s.store->host_replace(L"外部编辑");
            require(!s.adapter->current_element, "external host edit immediately closes prediction");
            s.key(VK_TAB, false);

            s.store->host_replace(L"");
            for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
            s.key(VK_SPACE);
            ComPtr<ITfCandidateListUIElementBehavior> behavior;
            check(s.adapter->current_element.As(&behavior), "host-managed candidate behavior");
            check(behavior->SetSelection(1), "host-managed changes selected prediction");
            require(s.store->text == L"你好", "host selection alone never inserts text");
            check(behavior->Finalize(), "host-managed finalizes prediction");
            require(s.store->text == L"你好朋友", "host-managed accepts selected extension");
            behavior.Reset();

            s.store->host_replace(L"");
            for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
            s.key(VK_SPACE);
            check(s.adapter->current_element.As(&behavior), "host-managed selected prediction for Tab");
            check(behavior->SetSelection(1), "host changes selection before Tab");
            s.key(VK_TAB);
            require(s.store->text == L"你好朋友", "Tab accepts host-selected prediction instead of stale first item");
            behavior.Reset();

            s.store->host_replace(L"");
            for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
            check(s.adapter->current_element.As(&behavior), "host-managed ordinary pinyin selection");
            check(behavior->SetSelection(1), "host selects pinyin prefix candidate");
            s.key(VK_SPACE);
            require(s.store->text == L"你hao", "Space accepts host-selected pinyin and keeps remaining suffix");
            s.key(VK_ESCAPE);
            behavior.Reset();
            s.store->host_replace(L"");
            for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
            s.key(L'2');
            require(s.store->text == L"你hao", "explicit digit selection still overrides host default selection");
            s.key(VK_ESCAPE);

            s.store->host_replace(L"");
            for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
            s.key(VK_SPACE);
            write_settings("{\"inputMode\":{\"panelDoubleTab\":false,\"nextWordPrediction\":false}}");
            const UINT changed_message = RegisterWindowMessageW(L"LocalPinyinIME.CandidateOptionsChanged");
            EnumThreadWindows(GetCurrentThreadId(), [](HWND hwnd, LPARAM message) -> BOOL {
                wchar_t name[64]{}; GetClassNameW(hwnd, name, 64);
                if (std::wstring(name) == L"LocalPinyinCandidateWindow") SendMessageW(hwnd, static_cast<UINT>(message), 0, 0);
                return TRUE;
            }, changed_message);
            require(!s.adapter->current_element, "settings disable closes host-managed prediction immediately");
            s.key(VK_TAB, false);
            s.store->host_replace(L"");
            for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
            s.key(VK_SPACE);
            require(!s.adapter->current_element && s.store->text == L"你好", "disabled prediction preserves normal Chinese commit");
            s.key(VK_TAB, false);
            write_settings("{\"inputMode\":{\"panelDoubleTab\":false,\"nextWordPrediction\":true}}");

            for (const auto modifier : {VK_SHIFT, VK_CONTROL, VK_MENU, VK_LWIN}) {
                s.store->host_replace(L"");
                for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
                s.key(VK_SPACE);
                s.modifier_keys[modifier] = true;
                s.key(VK_TAB, false);
                s.modifier_keys[modifier] = false;
                require(s.store->text == L"你好" && !s.adapter->current_element,
                    "modified Tab passes through and closes prediction without inserting");
            }
            write_settings("{\"inputMode\":{\"panelDoubleTab\":false,\"nextWordPrediction\":true,\"shiftToggleEnabled\":true}}");
            s.store->host_replace(L"");
            for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
            s.key(VK_SPACE);
            require(s.first_candidate() == L"世界", "prediction active before Shift mode toggle");
            s.modifier_keys[VK_SHIFT] = true;
            s.key(VK_SHIFT);
            s.modifier_keys[VK_SHIFT] = false;
            require(!s.adapter->current_element, "enabled Shift still closes prediction and toggles English");
            s.key(L'A', false);
            s.modifier_keys[VK_SHIFT] = true;
            s.key(VK_SHIFT);
            s.modifier_keys[VK_SHIFT] = false;
            write_settings("{\"inputMode\":{\"panelDoubleTab\":false,\"nextWordPrediction\":true}}");

            s.store->host_replace(L"");
            for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
            s.key(VK_SPACE);
            s.modifier_keys[VK_CONTROL] = true;
            s.key(VK_SPACE);
            s.modifier_keys[VK_CONTROL] = false;
            BOOL control_eaten = FALSE;
            check(s.keys->OnKeyUp(s.context.Get(), VK_CONTROL, 0, &control_eaten), "release Ctrl mode chord");
            require(!s.adapter->current_element, "CtrlSpace closes active prediction");
            s.key(L'A', false);
            check(s.keys->OnPreservedKey(s.context.Get(), kCtrlSpace, &control_eaten), "restore Chinese after CtrlSpace");

            s.store->host_replace(L"");
            for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
            s.key(VK_SPACE);
            check(s.keys->OnSetFocus(FALSE), "prediction loses keyboard focus");
            require(!s.adapter->current_element, "lost focus ends prediction candidate UI");
            s.key(VK_TAB, false);

            // A different real TSF context cannot reuse the first editor's
            // pending candidate or preceding text even within the same TIP.
            s.store->host_replace(L"");
            for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
            s.key(VK_SPACE);
            ComPtr<ITfDocumentMgr> other_document;
            ComPtr<ITfContext> other_context;
            ComPtr<TextStore> other_store; other_store.Attach(new TextStore(s.window));
            check(s.manager->CreateDocumentMgr(&other_document), "second real TSF document");
            check(other_document->CreateContext(client, 0, static_cast<ITextStoreACP*>(other_store.Get()), &other_context, &cookie), "second real context");
            check(other_document->Push(other_context.Get()), "second context push");
            check(s.manager->SetFocus(other_document.Get()), "second document focus");
            require(!s.adapter->current_element, "new document immediately closes old prediction");
            auto first_context = s.context;
            s.context = other_context;
            s.key(VK_TAB, false);
            s.key(L'N'); s.key(VK_ESCAPE);
            require(other_store->text.empty(), "no prediction crosses real contexts");
            check(surrounding_reader.read_preceding_text(other_context.Get(), service_client, &actual_before), "read other real context");
            require(actual_before.empty(), "no preceding text crosses real contexts");
            s.context = first_context;
            check(s.manager->SetFocus(s.document.Get()), "restore first document focus");
            check(other_document->Pop(TF_POPF_ALL), "pop second context");

            // The self-drawn popup follows the app's text layout: an app that
            // has not laid out the first key yet, its layout notification, a
            // move it does not report, and the next key typed before layout.
            {
                s.adapter->self_draw = true;
                const auto candidate_window = []() {
                    HWND found = nullptr;
                    EnumThreadWindows(GetCurrentThreadId(), [](HWND hwnd, LPARAM out) -> BOOL {
                        wchar_t name[64]{}; GetClassNameW(hwnd, name, 64);
                        if (std::wstring(name) != L"LocalPinyinCandidateWindow") return TRUE;
                        *reinterpret_cast<HWND*>(out) = hwnd;
                        return FALSE;
                    }, reinterpret_cast<LPARAM>(&found));
                    return found;
                };
                const auto pump = [](ULONGLONG ms) {
                    const ULONGLONG until = GetTickCount64() + ms;
                    MSG message{};
                    while (GetTickCount64() < until) {
                        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                            TranslateMessage(&message);
                            DispatchMessageW(&message);
                        }
                        Sleep(2);
                    }
                };
                const auto shown = [&]() {
                    RECT rect{};
                    const HWND popup = candidate_window();
                    if (popup && IsWindowVisible(popup)) GetWindowRect(popup, &rect);
                    return rect;
                };
                const auto hidden = [&]() { const RECT rect = shown(); return IsRectEmpty(&rect) != FALSE; };
                RECT work{};
                require(SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0) != FALSE, "work area");
                const LONG x = work.left + 240, y = work.top + 200;
                s.store->host_replace(L"");
                s.store->layout.reset();
                s.key(L'N'); s.key(L'I');
                pump(20);
                require(hidden(), "no popup at a guessed place before the app's layout");
                s.store->layout = RECT{x, y, x + 1, y + 20};
                s.store->layout_changed();
                pump(40);
                const RECT first = shown();
                require(!IsRectEmpty(&first), "the layout notification brings up the popup");
                s.store->layout = RECT{x + 150, y + 90, x + 151, y + 110};
                pump(600);
                const RECT moved = shown();
                if (moved.left - first.left != 150 || moved.top - first.top != 90)
                    std::wcerr << L"popup at " << moved.left << L"," << moved.top << L" first " << first.left << L"," << first.top << L"\n";
                require(moved.left - first.left == 150 && moved.top - first.top == 90, "the popup follows text the app moved silently");
                s.store->layout = RECT{x + 40, y + 30, x + 41, y + 50};
                s.store->layout_changed();
                pump(40);
                const RECT notified = shown();
                require(notified.left - first.left == 40 && notified.top - first.top == 30, "a reported layout moves the popup at once");
                s.store->layout.reset();
                s.key(L'H');
                const RECT typing = shown();
                require(typing.left == notified.left && typing.top == notified.top,
                    "the next key keeps the popup where the app laid out the text");
                s.key(VK_ESCAPE);
                require(hidden(), "Escape hides the popup");
                s.adapter->self_draw = false;
            }

            // Tab to English: what was just typed is translated (here by a
            // stand-in for the network) and put in its place.
            {
                auto* service = static_cast<localpinyin::TextService*>(s.service.Get());
                std::mutex asked_lock;
                std::vector<std::wstring> asked;
                std::atomic<int> delay_ms{0};
                service->set_translator_for_testing([&](const std::wstring& chinese) {
                    Sleep(delay_ms.load());
                    {
                        std::lock_guard<std::mutex> guard(asked_lock);
                        asked.push_back(chinese);
                    }
                    if (chinese == L"你好") return localpinyin::TranslationResult{true, L"Hello", {}};
                    if (chinese == L"你好，天气") return localpinyin::TranslationResult{true, L"Hello, weather", {}};
                    if (chinese == L"你好3") return localpinyin::TranslationResult{true, L"Hello 3", {}};
                    return localpinyin::TranslationResult{false, {}, L"offline"};
                });
                const auto pump = [](ULONGLONG ms) {
                    const ULONGLONG until = GetTickCount64() + ms;
                    MSG message{};
                    while (GetTickCount64() < until) {
                        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                            TranslateMessage(&message);
                            DispatchMessageW(&message);
                        }
                        Sleep(2);
                    }
                };
                const auto becomes = [&](const std::wstring& text) {
                    const ULONGLONG until = GetTickCount64() + 3000;
                    while (s.store->text != text && GetTickCount64() < until) pump(10);
                    if (s.store->text != text) std::wcerr << L"Host text: " << s.store->text << L"\n";
                    return s.store->text == text;
                };
                const auto asked_count = [&]() {
                    std::lock_guard<std::mutex> guard(asked_lock);
                    return asked.size();
                };
                // Suggestions are on, but Tab now belongs to translation.
                write_settings("{\"inputMode\":{\"panelDoubleTab\":false,\"nextWordPrediction\":true,\"tabTranslate\":true}}");
                s.store->host_replace(L"");
                s.key(VK_TAB, false);
                require(asked_count() == 0, "with nothing typed Tab is the application's");
                for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
                s.key(VK_SPACE);
                require(s.store->text == L"你好" && !s.adapter->current_element, "no suggestions while Tab translates");
                s.key(VK_TAB);
                require(becomes(L"Hello"), "Tab replaces what was typed with its translation");
                // Translated text is not translated again: Tab goes on to the application.
                s.key(VK_TAB, false);
                require(asked_count() == 1, "the translation itself is not sent again");

                // While typing pinyin, Tab commits it and translates.
                s.store->host_replace(L"");
                for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
                s.key(VK_TAB);
                require(becomes(L"Hello"), "Tab during pinyin commits and translates");

                // Several commits and punctuation make one run.
                s.store->host_replace(L"");
                for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
                s.key(VK_SPACE);
                s.key(VK_OEM_COMMA);
                for (const auto letter : std::wstring(L"TIANQI")) s.key(letter);
                s.key(VK_TAB);
                require(becomes(L"Hello, weather"), "a run of commits and punctuation is translated whole");

                // A key the application typed itself belongs to the run.
                s.store->host_replace(L"");
                for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
                s.key(VK_SPACE);
                s.key(L'3', false);
                s.store->host_replace(s.store->text + L"3");
                s.key(VK_TAB);
                require(becomes(L"Hello 3"), "characters the application typed are part of the run");

                // Text changed behind the input method's back is left alone.
                s.store->host_replace(L"");
                for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
                s.key(VK_SPACE);
                s.store->host_replace(L"别的");
                const auto before_moved = asked_count();
                s.key(VK_TAB, false);
                require(asked_count() == before_moved && s.store->text == L"别的", "a changed document is not translated");

                // A failed translation leaves the text as it was.
                s.store->host_replace(L"");
                for (const auto letter : std::wstring(L"TIANQI")) s.key(letter);
                s.key(VK_SPACE);
                const auto before_failure = asked_count();
                s.key(VK_TAB);
                const ULONGLONG until = GetTickCount64() + 3000;
                while (asked_count() == before_failure && GetTickCount64() < until) pump(10);
                pump(50);
                require(asked_count() == before_failure + 1 && s.store->text == L"天气", "a failed translation changes nothing");

                // An application that shows the input method none of its text:
                // Tab goes by what was typed (here the typed replacement cannot
                // reach a window, so the text stays).
                s.store->opaque = true;
                s.store->host_replace(L"");
                for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
                s.key(VK_SPACE);
                s.store->opaque = false;
                require(s.store->text == L"你好", "typing works in an opaque application");
                s.store->opaque = true;
                const auto before_opaque = asked_count();
                s.key(VK_TAB);
                {
                    const ULONGLONG until_opaque = GetTickCount64() + 3000;
                    while (asked_count() == before_opaque && GetTickCount64() < until_opaque) pump(10);
                }
                pump(50);
                s.store->opaque = false;
                require(asked_count() == before_opaque + 1, "an application that shows no text is translated from what was typed");

                // The next key cancels a translation still on its way.
                delay_ms = 300;
                s.store->host_replace(L"");
                for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
                s.key(VK_SPACE);
                s.key(VK_TAB);
                s.key(L'N');
                s.key(VK_ESCAPE);
                pump(700);
                require(s.store->text == L"你好", "a key typed meanwhile cancels the translation");
                delay_ms = 0;

                // Turned off, Tab is the application's again.
                write_settings("{\"inputMode\":{\"panelDoubleTab\":false,\"nextWordPrediction\":true}}");
                const auto before_off = asked_count();
                s.store->host_replace(L"");
                for (const auto letter : std::wstring(L"SUIBIAN")) s.key(letter);
                s.key(VK_SPACE);
                s.key(VK_TAB, false);
                require(asked_count() == before_off, "Tab does not translate when the option is off");
                service->set_translator_for_testing({});
            }

            // Tab twice opens the tool panel: a single Tab's translation lands
            // only once a second Tab can no longer come, two Tabs drop it, and
            // a key typed during the wait gets the Tab's tab character first.
            {
                auto* service = static_cast<localpinyin::TextService*>(s.service.Get());
                service->set_translator_for_testing([](const std::wstring& chinese) {
                    return chinese == L"你好" ? localpinyin::TranslationResult{true, L"Hello", {}}
                                              : localpinyin::TranslationResult{false, {}, L"offline"};
                });
                const auto pump = [](ULONGLONG ms) {
                    const ULONGLONG until = GetTickCount64() + ms;
                    MSG message{};
                    while (GetTickCount64() < until) {
                        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                            TranslateMessage(&message);
                            DispatchMessageW(&message);
                        }
                        Sleep(2);
                    }
                };
                write_settings("{\"inputMode\":{\"panelDoubleTab\":true,\"tabTranslate\":true}}");
                s.store->host_replace(L"");
                for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
                s.key(VK_SPACE);
                s.key(VK_TAB);
                pump(120);
                require(s.store->text == L"你好", "a single Tab's translation waits for a second Tab");
                pump(600);
                require(s.store->text == L"Hello", "then the translation lands");

                s.store->host_replace(L"");
                for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
                s.key(VK_SPACE);
                s.key(VK_TAB);
                s.key(VK_TAB);
                pump(700);
                require(s.store->text == L"你好", "Tab twice opens the panel and drops the translation");
                // The small panel is up: keys go to it (a letter searches),
                // Esc clears the search, a second Esc puts the panel away.
                s.key(L'N');
                require(s.store->text == L"你好", "keys go to the panel while it is up");
                s.key(VK_ESCAPE);
                s.key(VK_ESCAPE);
                s.key(L'N');
                require(!s.first_candidate().empty(), "after Esc keys are typing again");
                s.key(VK_ESCAPE);
                require(s.store->text == L"你好", "and nothing went into the text");

                s.store->host_replace(L"");
                s.key(VK_TAB);
                s.key(L'N');
                require(s.store->text.size() >= 1 && s.store->text.front() == L'	', "a key during the wait comes after the Tab");
                s.key(VK_ESCAPE);
                require(s.store->text == L"	", "the Tab's tab character stays");
                pump(400);
                require(s.store->text == L"	", "and is not typed twice");

                // The Tab sent after a single tap reaches documents that skip
                // the key test (RichEdit) as OnKeyDown alone: the application's,
                // once; later Tabs are the input method's again.
                s.store->host_replace(L"");
                service->pass_next_tab_for_testing();
                BOOL replayed_eaten = TRUE;
                check(s.keys->OnKeyDown(s.context.Get(), VK_TAB, 0, &replayed_eaten), "replayed Tab");
                require(replayed_eaten == FALSE, "the replayed Tab goes to the application");
                check(s.keys->OnKeyUp(s.context.Get(), VK_TAB, 0, &replayed_eaten), "replayed Tab up");
                s.key(VK_TAB);
                s.key(VK_TAB);
                pump(400);
                require(s.store->text.empty(), "two Tabs after it open the panel again");
                s.key(VK_ESCAPE);

                write_settings("{\"inputMode\":{\"panelDoubleTab\":false,\"nextWordPrediction\":true}}");
                service->set_translator_for_testing({});
            }

            // Typing tools: v and a sum, number or date; sj gives the time second.
            {
                s.store->host_replace(L"");
                for (const auto key : std::vector<WPARAM>{L'V', L'5', L'5'}) s.key(key);
                s.modifier_keys[VK_SHIFT] = true;
                s.key(VK_OEM_PLUS);
                s.modifier_keys[VK_SHIFT] = false;
                for (const auto key : std::vector<WPARAM>{L'1', L'5'}) s.key(key);
                require(s.first_candidate() == L"70", "v55+15 offers 70");
                s.key(VK_SPACE);
                require(s.store->text == L"70", "Space types the result");
                s.store->host_replace(L"");
                for (const auto key : std::vector<WPARAM>{L'V', L'1', L'2', L'3'}) s.key(key);
                require(s.first_candidate() == L"壹佰贰拾叁", "v123 offers the amount in words");
                s.key(VK_ESCAPE);
                s.store->host_replace(L"");
            }

            s.store->host_replace(L"");
            for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
            s.key(VK_SPACE);
            require(s.first_candidate() == L"世界", "prediction active before mode change");

            const std::wstring before_english = s.store->text;
            BOOL eaten = FALSE;
            check(s.keys->OnPreservedKey(s.context.Get(), kCtrlSpace, &eaten), "English mode toggle");
            require(eaten != FALSE, "preserved key consumed");
            require(!s.adapter->current_element, "English mode closes active prediction");
            s.key(VK_TAB, false);
            s.key(L'A', false);
            s.key(VK_OEM_PERIOD, false);
            require(s.store->text == before_english, "English keys pass through without composition");
            require(s.store->composition_starts == s.store->composition_ends,
                "all real TSF compositions ended");
            {
                std::ifstream learning(isolated / L"LocalPinyinIME" / L"user_learning.tsv", std::ios::binary);
                const std::string records((std::istreambuf_iterator<char>(learning)), std::istreambuf_iterator<char>());
                for (const auto* private_text : {"发送", "气象", "此处有密码前文", "外部编辑", "世界", "朋友"})
                    require(records.find(private_text) == std::string::npos,
                        "surrounding document and prediction acceptance are not persisted as pinyin learning");
            }
            check(s.service->Deactivate(), "text service deactivation");
            s.service_active = false;
            // In a code editor (named for this test) punctuation is ASCII: the
            // editor types it itself, and after pinyin the candidate comes first.
            require(SetEnvironmentVariableW(L"LOCALPINYINIME_TEST_HOST_EXECUTABLE", L"Code.exe") != FALSE,
                "name a code editor host");
            check(s.service->Activate(static_cast<ITfThreadMgr*>(s.adapter.Get()), service_client),
                "text service activation in a code editor");
            s.service_active = true;
            check(s.keys->OnPreservedKey(s.context.Get(), kCtrlSpace, &eaten), "Chinese mode in the code editor");
            const std::wstring before_code = s.store->text;
            s.key(VK_OEM_COMMA, false);
            require(s.store->text == before_code, "code editor punctuation goes to the editor");
            for (const auto letter : std::wstring(L"NIHAO")) s.key(letter);
            s.key(VK_OEM_COMMA);
            require(s.store->text == before_code + L"你好,", "code editor punctuation after pinyin is ASCII");
            check(s.service->Deactivate(), "text service deactivation in a code editor");
            s.service_active = false;
            SetEnvironmentVariableW(L"LOCALPINYINIME_TEST_HOST_EXECUTABLE", nullptr);
            std::cout << "PASS: single-line candidate pages, page-relative digit/Space commits, host page boundaries/selection, cross-page prediction Tab, DLL COM creation, real TSF edit sessions, context-ranked pinyin, bounded/private preceding text, Tab prediction, host-managed selection, edit/focus/context invalidation, ordinary key passthrough and composition regressions.\n"
                << "Scope: key-sink registration is in memory; OS profile loading/key routing and candidate presentation require desktop acceptance.\n";
        }
        std::filesystem::remove_all(isolated);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "TSF integration failed: " << error.what() << '\n';
        return 1;
    }
}
