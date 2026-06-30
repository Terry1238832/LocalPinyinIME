#include "text_service.h"

#include "globals.h"
#include "../common/logging.h"
#include "../common/win32_utils.h"
#include "../engine/candidate_selection.h"

#include <algorithm>
#include <cwctype>
#include <sstream>

namespace localpinyin {
namespace {

const wchar_t* bool_text(bool value) noexcept {
    return value ? L"true" : L"false";
}

bool usable_anchor_rect(const RECT& rect) noexcept {
    return rect.right >= rect.left && rect.bottom > rect.top && !(rect.left == rect.right && rect.top == rect.bottom);
}

std::wstring caret_rect_failure_reason(HRESULT hr) {
    if (hr == TF_E_NOLAYOUT) {
        return L"tf_e_nolayout";
    }
    if (hr == S_FALSE) {
        return L"no_active_composition";
    }
    return L"get_text_ext_failed";
}

}  // namespace

TextService::TextService() {
    dll_add_ref();
}

TextService::~TextService() {
    unadvise_key_event_sink();
    if (thread_mgr_) {
        thread_mgr_->Release();
    }
    dll_release();
}

HRESULT TextService::QueryInterface(REFIID riid, void** object) {
    if (!object) {
        return E_POINTER;
    }
    *object = nullptr;
    if (riid == IID_IUnknown || riid == __uuidof(ITfTextInputProcessor) || riid == __uuidof(ITfTextInputProcessorEx)) {
        *object = static_cast<ITfTextInputProcessorEx*>(this);
    } else if (riid == __uuidof(ITfKeyEventSink)) {
        *object = static_cast<ITfKeyEventSink*>(this);
    } else if (riid == __uuidof(ITfCompositionSink)) {
        *object = static_cast<ITfCompositionSink*>(this);
    } else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG TextService::AddRef() {
    return static_cast<ULONG>(InterlockedIncrement(&ref_count_));
}

ULONG TextService::Release() {
    const long count = InterlockedDecrement(&ref_count_);
    if (count == 0) {
        delete this;
    }
    return static_cast<ULONG>(count);
}

HRESULT TextService::Activate(ITfThreadMgr* thread_mgr, TfClientId client_id) {
    return ActivateEx(thread_mgr, client_id, 0);
}

HRESULT TextService::ActivateEx(ITfThreadMgr* thread_mgr, TfClientId client_id, DWORD) {
    if (!thread_mgr) {
        return E_POINTER;
    }
    thread_mgr_ = thread_mgr;
    thread_mgr_->AddRef();
    client_id_ = client_id;

    const HRESULT hr = advise_key_event_sink();
    log_status(L"tsf", FAILED(hr) ? L"ActivateEx failed " + hresult_hex(hr) : L"ActivateEx succeeded");
    return hr;
}

HRESULT TextService::Deactivate() {
    unadvise_key_event_sink();
    candidate_window_.hide(L"deactivate");
    composition_.clear();
    composing_.clear();
    candidates_.clear();
    has_last_caret_rect_ = false;
    if (thread_mgr_) {
        thread_mgr_->Release();
        thread_mgr_ = nullptr;
    }
    client_id_ = TF_CLIENTID_NULL;
    log_status(L"tsf", L"Deactivate");
    return S_OK;
}

HRESULT TextService::advise_key_event_sink() {
    if (!thread_mgr_ || key_sink_advised_) {
        return S_OK;
    }
    ITfKeystrokeMgr* keystroke_mgr = nullptr;
    HRESULT hr = thread_mgr_->QueryInterface(IID_PPV_ARGS(&keystroke_mgr));
    if (FAILED(hr)) {
        return hr;
    }
    hr = keystroke_mgr->AdviseKeyEventSink(client_id_, static_cast<ITfKeyEventSink*>(this), TRUE);
    if (SUCCEEDED(hr)) {
        key_sink_advised_ = true;
    }
    keystroke_mgr->Release();
    return hr;
}

void TextService::unadvise_key_event_sink() {
    if (!thread_mgr_ || !key_sink_advised_) {
        return;
    }
    ITfKeystrokeMgr* keystroke_mgr = nullptr;
    if (SUCCEEDED(thread_mgr_->QueryInterface(IID_PPV_ARGS(&keystroke_mgr)))) {
        keystroke_mgr->UnadviseKeyEventSink(client_id_);
        keystroke_mgr->Release();
    }
    key_sink_advised_ = false;
}

HRESULT TextService::OnSetFocus(BOOL) {
    return S_OK;
}

HRESULT TextService::OnTestKeyDown(ITfContext*, WPARAM wparam, LPARAM, BOOL* eaten) {
    if (!eaten) {
        return E_POINTER;
    }
    *eaten = FALSE;
    if ((GetKeyState(VK_CONTROL) & 0x8000) && wparam == VK_SPACE) {
        *eaten = TRUE;
        return S_OK;
    }
    if (mode_ == InputMode::English || should_pass_through(wparam)) {
        return S_OK;
    }
    if (wparam >= L'1' && wparam <= L'9') {
        *eaten = can_select_candidate_by_digit(wparam, candidates_.size(), !composing_.empty(), candidate_window_.is_visible());
        return S_OK;
    }
    if ((wparam >= L'A' && wparam <= L'Z') || (wparam == VK_OEM_7 && !composing_.empty()) ||
        wparam == VK_BACK || wparam == VK_ESCAPE || wparam == VK_SPACE || wparam == VK_RETURN ||
        wparam == VK_LEFT || wparam == VK_RIGHT) {
        *eaten = TRUE;
    }
    return S_OK;
}

HRESULT TextService::OnKeyDown(ITfContext* context, WPARAM wparam, LPARAM, BOOL* eaten) {
    if (!eaten) {
        return E_POINTER;
    }
    *eaten = FALSE;

    if ((GetKeyState(VK_CONTROL) & 0x8000) && wparam == VK_SPACE) {
        mode_ = mode_ == InputMode::Chinese ? InputMode::English : InputMode::Chinese;
        *eaten = TRUE;
        if (mode_ == InputMode::English) {
            candidate_window_.hide(L"english_mode");
        } else {
            update_candidate_window(context);
        }
        log_status(L"mode", mode_ == InputMode::Chinese ? L"Chinese" : L"English");
        return S_OK;
    }

    if (mode_ == InputMode::English || should_pass_through(wparam)) {
        return S_OK;
    }
    return handle_chinese_key(context, wparam, eaten) ? S_OK : S_OK;
}

HRESULT TextService::OnTestKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* eaten) {
    if (!eaten) {
        return E_POINTER;
    }
    *eaten = FALSE;
    return S_OK;
}

HRESULT TextService::OnKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* eaten) {
    if (!eaten) {
        return E_POINTER;
    }
    *eaten = FALSE;
    return S_OK;
}

HRESULT TextService::OnPreservedKey(ITfContext*, REFGUID, BOOL* eaten) {
    if (!eaten) {
        return E_POINTER;
    }
    *eaten = FALSE;
    return S_OK;
}

HRESULT TextService::OnCompositionTerminated(TfEditCookie, ITfComposition*) {
    composition_.clear();
    composing_.clear();
    candidates_.clear();
    has_last_caret_rect_ = false;
    candidate_window_.hide(L"composition_terminated");
    return S_OK;
}

bool TextService::should_pass_through(WPARAM wparam) const {
    const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    const bool win = (GetKeyState(VK_LWIN) & 0x8000) != 0 || (GetKeyState(VK_RWIN) & 0x8000) != 0;
    if (alt || win) {
        return true;
    }
    if (ctrl && wparam != VK_SPACE) {
        return true;
    }
    return false;
}

bool TextService::handle_chinese_key(ITfContext* context, WPARAM wparam, BOOL* eaten) {
    if (!context || !eaten) {
        return false;
    }

    if (wparam >= L'A' && wparam <= L'Z') {
        composing_.push_back(static_cast<wchar_t>(std::towlower(static_cast<wint_t>(wparam))));
        selected_index_ = 0;
        refresh_candidates();
        update_composition(context);
        *eaten = TRUE;
        return true;
    }

    if (wparam == VK_OEM_7 && !composing_.empty()) {
        composing_.push_back(L'\'');
        selected_index_ = 0;
        refresh_candidates();
        update_composition(context);
        *eaten = TRUE;
        return true;
    }

    if (wparam == VK_BACK) {
        if (composing_.empty()) {
            return false;
        }
        composing_.pop_back();
        selected_index_ = 0;
        if (composing_.empty()) {
            cancel_composition(context);
        } else {
            refresh_candidates();
            update_composition(context);
        }
        *eaten = TRUE;
        return true;
    }

    if (wparam == VK_ESCAPE) {
        if (!composing_.empty()) {
            cancel_composition(context);
            *eaten = TRUE;
            return true;
        }
        return false;
    }

    if (wparam == VK_SPACE || wparam == VK_RETURN) {
        if (!composing_.empty() && !candidates_.empty()) {
            if (wparam == VK_RETURN) {
                selected_index_ = 0;
            }
            commit_selected(context);
            *eaten = TRUE;
            return true;
        }
        return false;
    }

    if (wparam >= L'1' && wparam <= L'9') {
        if (can_select_candidate_by_digit(wparam, candidates_.size(), !composing_.empty(), candidate_window_.is_visible())) {
            const size_t index = static_cast<size_t>(candidate_index_from_digit_key(wparam));
            selected_index_ = index;
            commit_selected(context);
            *eaten = TRUE;
            return true;
        }
        return false;
    }

    if (wparam == VK_LEFT && !candidates_.empty()) {
        selected_index_ = selected_index_ == 0 ? candidates_.size() - 1 : selected_index_ - 1;
        update_candidate_window(context);
        *eaten = TRUE;
        return true;
    }

    if (wparam == VK_RIGHT && !candidates_.empty()) {
        selected_index_ = (selected_index_ + 1) % candidates_.size();
        update_candidate_window(context);
        *eaten = TRUE;
        return true;
    }

    return false;
}

void TextService::refresh_candidates() {
    candidates_ = engine_.lookup(composing_);
}

HRESULT TextService::update_composition(ITfContext* context) {
    HRESULT hr = composition_.set_text(context, client_id_, static_cast<ITfCompositionSink*>(this), composing_);
    if (FAILED(hr)) {
        log_status(L"composition", L"set_text failed " + hresult_hex(hr));
        return hr;
    }
    update_candidate_window(context);
    return S_OK;
}

void TextService::update_candidate_window(ITfContext* context) {
    RECT anchor_rect{};
    bool anchor_available = false;
    if (mode_ == InputMode::Chinese && !composing_.empty() && !candidates_.empty()) {
        anchor_available = candidate_anchor_rect(context, &anchor_rect);
    }

    const CandidateDisplayDecision decision = decide_candidate_display(CandidateDisplayState{
        mode_ == InputMode::English,
        !composing_.empty(),
        candidates_.size(),
        anchor_available
    });

    std::wstringstream stream;
    stream << L"request_show=" << bool_text(decision.request_show)
           << L" composition_active=" << bool_text(!composing_.empty())
           << L" candidate_count=" << candidates_.size()
           << L" caret_rect_available=" << bool_text(anchor_available)
           << L" hide_reason=" << candidate_hide_reason_to_string(decision.hide_reason);
    log_status(L"candidate-ui", stream.str());

    if (!decision.request_show) {
        const std::wstring reason = candidate_hide_reason_to_string(decision.hide_reason);
        candidate_window_.hide(reason.c_str());
        return;
    }
    candidate_window_.show(composing_, candidates_, selected_index_, anchor_rect);
}

HRESULT TextService::commit_selected(ITfContext* context) {
    if (candidates_.empty()) {
        return S_FALSE;
    }
    const std::wstring committed = candidates_[selected_index_].text;
    const HRESULT hr = composition_.commit_text(context, client_id_, committed);
    if (FAILED(hr)) {
        log_status(L"composition", L"commit failed " + hresult_hex(hr));
        return hr;
    }
    engine_.learn(composing_, committed);
    composing_.clear();
    candidates_.clear();
    selected_index_ = 0;
    has_last_caret_rect_ = false;
    candidate_window_.hide(L"commit");
    return S_OK;
}

HRESULT TextService::cancel_composition(ITfContext* context) {
    const HRESULT hr = composition_.cancel(context, client_id_);
    if (FAILED(hr)) {
        log_status(L"composition", L"cancel failed " + hresult_hex(hr));
    }
    composing_.clear();
    candidates_.clear();
    selected_index_ = 0;
    has_last_caret_rect_ = false;
    candidate_window_.hide(L"cancel");
    return hr;
}

bool TextService::candidate_anchor_rect(ITfContext* context, RECT* rect) {
    if (!rect) {
        return false;
    }

    if (context) {
        RECT tsf_rect{};
        const HRESULT hr = composition_.caret_rect(context, client_id_, &tsf_rect);
        std::wstringstream stream;
        stream << L"get_text_ext_hr=" << hresult_hex(hr);
        if (hr == S_OK && usable_anchor_rect(tsf_rect)) {
            *rect = tsf_rect;
            last_caret_rect_ = tsf_rect;
            has_last_caret_rect_ = true;
            stream << L" caret_rect_valid=true caret_source=tsf";
            log_status(L"candidate-ui", stream.str());
            return true;
        }

        stream << L" caret_rect_valid=false caret_source=tsf";
        if (hr == S_OK) {
            stream << L" reason=empty_rect";
        } else {
            stream << L" reason=" << caret_rect_failure_reason(hr);
        }
        log_status(L"candidate-ui", stream.str());
    } else {
        log_status(L"candidate-ui", L"get_text_ext_hr=not_called caret_rect_valid=false caret_source=tsf reason=null_context");
    }

    RECT gui_rect{};
    if (gui_caret_rect(&gui_rect)) {
        *rect = gui_rect;
        last_caret_rect_ = gui_rect;
        has_last_caret_rect_ = true;
        log_status(L"candidate-ui", L"caret_rect_valid=true caret_source=gui_caret fallback=true");
        return true;
    }

    if (has_last_caret_rect_ && usable_anchor_rect(last_caret_rect_)) {
        *rect = last_caret_rect_;
        log_status(L"candidate-ui", L"caret_rect_valid=true caret_source=last_valid fallback=true");
        return true;
    }

    log_status(L"candidate-ui", L"caret_rect_valid=false caret_source=none fallback=false");
    return false;
}

bool TextService::gui_caret_rect(RECT* rect) const {
    if (!rect) {
        return false;
    }

    GUITHREADINFO info{};
    info.cbSize = sizeof(info);
    if (!GetGUIThreadInfo(0, &info) || !info.hwndCaret) {
        return false;
    }

    POINT top_left{info.rcCaret.left, info.rcCaret.top};
    POINT bottom_right{info.rcCaret.right, info.rcCaret.bottom};
    if (!ClientToScreen(info.hwndCaret, &top_left) || !ClientToScreen(info.hwndCaret, &bottom_right)) {
        return false;
    }

    RECT screen_rect{top_left.x, top_left.y, bottom_right.x, bottom_right.y};
    if (screen_rect.right < screen_rect.left || screen_rect.bottom < screen_rect.top) {
        return false;
    }
    if (screen_rect.left == screen_rect.right) {
        screen_rect.right = screen_rect.left + 1;
    }
    if (screen_rect.top == screen_rect.bottom) {
        screen_rect.bottom = screen_rect.top + 1;
    }
    *rect = screen_rect;
    return usable_anchor_rect(*rect);
}

}  // namespace localpinyin
