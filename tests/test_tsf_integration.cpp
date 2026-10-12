#include <windows.h>
#include <msctf.h>
#include <textstor.h>
#include <wrl/client.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

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
        status->dwDynamicFlags = 0;
        status->dwStaticFlags = TS_SS_NOHIDDENTEXT;
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
    HRESULT STDMETHODCALLTYPE RequestSupportedAttrs(DWORD, ULONG, const TS_ATTRID*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE RequestAttrsAtPosition(LONG, ULONG, const TS_ATTRID*, DWORD) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE RequestAttrsTransitioningAtPosition(LONG, ULONG, const TS_ATTRID*, DWORD) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE FindNextAttrTransition(LONG, LONG halt, ULONG, const TS_ATTRID*, DWORD,
        LONG* next, BOOL* found, LONG* offset) override {
        if (!next || !found || !offset) return E_POINTER;
        *next = halt; *found = FALSE; *offset = 0;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RetrieveRequestedAttrs(ULONG, TS_ATTRVAL*, ULONG* fetched) override {
        if (!fetched) return E_POINTER;
        *fetched = 0;
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
    // This is a hidden test host; presentation requires separate desktop acceptance.
    HRESULT STDMETHODCALLTYPE GetTextExt(TsViewCookie, LONG, LONG, RECT*, BOOL*) override { return TS_E_NOLAYOUT; }
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
    unsigned composition_starts = 0;
    unsigned composition_ends = 0;
private:
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
class ManagerAdapter final : public ITfThreadMgr, public ITfKeystrokeMgr {
public:
    explicit ManagerAdapter(ITfThreadMgr* system) : system_(system) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (iid == IID_IUnknown || iid == __uuidof(ITfThreadMgr)) *out = static_cast<ITfThreadMgr*>(this);
        else if (iid == __uuidof(ITfKeystrokeMgr)) *out = static_cast<ITfKeystrokeMgr*>(this);
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
private:
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
            s.module = LoadLibraryExW(argv[1], nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
            require(s.module != nullptr, "load IME DLL");
            using GetFactory = HRESULT (STDAPICALLTYPE*)(REFCLSID, REFIID, void**);
            const auto get_factory = reinterpret_cast<GetFactory>(GetProcAddress(s.module, "DllGetClassObject"));
            require(get_factory != nullptr, "DllGetClassObject export");
            ComPtr<IClassFactory> factory;
            check(get_factory(kService, IID_PPV_ARGS(&factory)), "DLL COM factory");
            check(factory->CreateInstance(nullptr, IID_PPV_ARGS(&s.service)), "create COM text service");
            factory.Reset();
            check(s.service.As(&s.keys), "key sink COM interface");
            s.adapter.Attach(new ManagerAdapter(s.manager.Get()));
            check(s.service->Activate(static_cast<ITfThreadMgr*>(s.adapter.Get()), service_client), "text service activation");
            s.service_active = true;
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
            const std::wstring before_english = s.store->text;
            BOOL eaten = FALSE;
            check(s.keys->OnPreservedKey(s.context.Get(), kCtrlSpace, &eaten), "English mode toggle");
            require(eaten != FALSE, "preserved key consumed");
            s.key(L'A', false);
            s.key(VK_OEM_PERIOD, false);
            require(s.store->text == before_english, "English keys pass through without composition");
            require(s.store->composition_starts == s.store->composition_ends,
                "all real TSF compositions ended");
            check(s.service->Deactivate(), "text service deactivation");
            s.service_active = false;
            std::cout << "PASS: DLL COM creation, real TSF edit sessions, Chinese/prefix commit, retained suffix, punctuation, Backspace, Esc, Enter and English passthrough.\n"
                << "Scope: key-sink registration is in memory; OS profile loading/key routing and candidate presentation require desktop acceptance.\n";
        }
        std::filesystem::remove_all(isolated);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "TSF integration failed: " << error.what() << '\n';
        return 1;
    }
}
