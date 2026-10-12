#pragma once

#include "input_mode.h"

#include <ctfutb.h>
#include <msctf.h>
#include <windows.h>

#include <functional>

namespace localpinyin {

inline constexpr GUID GUID_LocalPinyinInputModeLangBarItem =
    {0x6d95afad, 0x29c5, 0x4d1d, {0x97, 0x73, 0x1f, 0xd8, 0x4c, 0x8e, 0x9f, 0x21}};

[[nodiscard]] const wchar_t* input_mode_label(InputMode mode) noexcept;
[[nodiscard]] const wchar_t* input_mode_tooltip(InputMode mode) noexcept;

enum class ModeIconPopupCommand {
    Cancel,
    Toggle,
    Settings,
    Error
};

class LanguageBarState final : public ITfLangBarItemButton, public ITfSource {
public:
    LanguageBarState() = default;
    ~LanguageBarState();

    LanguageBarState(const LanguageBarState&) = delete;
    LanguageBarState& operator=(const LanguageBarState&) = delete;

    void set_mode(InputMode mode) noexcept;
    [[nodiscard]] InputMode mode() const noexcept { return mode_; }
    void set_toggle_callback(std::function<HRESULT(const wchar_t*)> callback);
    void set_settings_callback(std::function<HRESULT(const wchar_t*)> callback);
    void set_popup_menu_callback(std::function<ModeIconPopupCommand()> callback);
    void clear_callbacks();
    [[nodiscard]] bool pending_initial_update() const noexcept { return pending_initial_update_; }
    [[nodiscard]] unsigned initial_update_retry_count() const noexcept { return initial_update_retry_count_; }
    HRESULT register_item(ITfThreadMgr* thread_mgr);
    HRESULT unregister_item();
    void notify_mode_changed();
    void publish_initial_mode();
    HRESULT retry_pending_initial_update(const wchar_t* reason);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    HRESULT STDMETHODCALLTYPE GetInfo(TF_LANGBARITEMINFO* info) override;
    HRESULT STDMETHODCALLTYPE GetStatus(DWORD* status) override;
    HRESULT STDMETHODCALLTYPE Show(BOOL show) override;
    HRESULT STDMETHODCALLTYPE GetTooltipString(BSTR* tooltip) override;

    HRESULT STDMETHODCALLTYPE OnClick(TfLBIClick click, POINT point, const RECT* area) override;
    HRESULT STDMETHODCALLTYPE InitMenu(ITfMenu* menu) override;
    HRESULT STDMETHODCALLTYPE OnMenuSelect(UINT id) override;
    HRESULT STDMETHODCALLTYPE GetIcon(HICON* icon) override;
    HRESULT STDMETHODCALLTYPE GetText(BSTR* text) override;

    HRESULT STDMETHODCALLTYPE AdviseSink(REFIID riid, IUnknown* unknown, DWORD* cookie) override;
    HRESULT STDMETHODCALLTYPE UnadviseSink(DWORD cookie) override;

private:
    long ref_count_ = 1;
    ITfLangBarItemMgr* item_mgr_ = nullptr;
    ITfLangBarItemSink* sink_ = nullptr;
    DWORD sink_cookie_ = TF_INVALID_COOKIE;
    bool item_added_ = false;
    bool visible_ = true;
    bool logged_get_icon_chinese_ = false;
    bool logged_get_icon_english_ = false;
    InputMode mode_ = InputMode::Chinese;
    bool pending_initial_update_ = false;
    unsigned initial_update_retry_count_ = 0;
    HWND popup_owner_hwnd_ = nullptr;
    std::function<HRESULT(const wchar_t*)> toggle_callback_;
    std::function<HRESULT(const wchar_t*)> settings_callback_;
    std::function<ModeIconPopupCommand()> popup_menu_callback_;

    HRESULT send_update(DWORD flags, const wchar_t* reason, bool defer_on_failure);
    HRESULT handle_toggle_request(const wchar_t* source);
    HRESULT handle_settings_request(const wchar_t* source);
    ModeIconPopupCommand show_mode_icon_context_menu();
    HWND ensure_popup_owner_window();
    void destroy_popup_owner_window();
};

}  // namespace localpinyin
