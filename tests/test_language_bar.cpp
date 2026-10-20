#include "test_common.h"
#include "../src/ime/language_bar.h"
#include "../src/ime/guids.h"
#include "../src/ime/resource.h"

#include <ctffunc.h>
#include <oleauto.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace {

std::vector<std::uint8_t> render_icon_pixels(HICON icon, int size) {
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = size;
    info.bmiHeader.biHeight = -size;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    void* raw_pixels = nullptr;
    HDC dc = CreateCompatibleDC(nullptr);
    if (!dc) {
        return {};
    }
    HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &raw_pixels, nullptr, 0);
    if (!bitmap || !raw_pixels) {
        if (bitmap) {
            DeleteObject(bitmap);
        }
        DeleteDC(dc);
        return {};
    }
    HGDIOBJ old_bitmap = SelectObject(dc, bitmap);
    if (!old_bitmap) {
        DeleteObject(bitmap);
        DeleteDC(dc);
        return {};
    }

    const size_t byte_count = static_cast<size_t>(size) * static_cast<size_t>(size) * 4;
    std::fill_n(static_cast<std::uint8_t*>(raw_pixels), byte_count, 0);
    if (!DrawIconEx(dc, 0, 0, icon, size, size, 0, nullptr, DI_NORMAL)) {
        SelectObject(dc, old_bitmap);
        DeleteObject(bitmap);
        DeleteDC(dc);
        return {};
    }
    GdiFlush();

    std::vector<std::uint8_t> pixels(static_cast<std::uint8_t*>(raw_pixels),
                                     static_cast<std::uint8_t*>(raw_pixels) + byte_count);
    SelectObject(dc, old_bitmap);
    DeleteObject(bitmap);
    DeleteDC(dc);
    return pixels;
}

size_t nontransparent_pixel_count(const std::vector<std::uint8_t>& pixels) {
    size_t count = 0;
    for (size_t index = 3; index < pixels.size(); index += 4) {
        if (pixels[index] != 0) {
            ++count;
        }
    }
    return count;
}

}  // namespace

class LangBarSink final : public ITfLangBarItemSink {
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (!object) {
            return E_POINTER;
        }
        *object = nullptr;
        if (riid == IID_IUnknown || riid == __uuidof(ITfLangBarItemSink)) {
            *object = static_cast<ITfLangBarItemSink*>(this);
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

    HRESULT STDMETHODCALLTYPE OnUpdate(DWORD flags) override {
        last_flags = flags;
        update_count += 1;
        if (fail_next_update) {
            fail_next_update = false;
            return E_FAIL;
        }
        return S_OK;
    }

    DWORD last_flags = 0;
    int update_count = 0;
    bool fail_next_update = false;
};

class FakeMenu final : public ITfMenu {
public:
    struct Item {
        UINT id = 0;
        DWORD flags = 0;
        std::wstring text;
    };

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (!object) {
            return E_POINTER;
        }
        *object = nullptr;
        if (riid == IID_IUnknown || riid == __uuidof(ITfMenu)) {
            *object = static_cast<ITfMenu*>(this);
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

    HRESULT STDMETHODCALLTYPE AddMenuItem(UINT id,
                                         DWORD flags,
                                         HBITMAP,
                                         HBITMAP,
                                         const WCHAR* text,
                                         ULONG text_length,
                                         ITfMenu**) override {
        items.push_back(Item{id, flags, std::wstring(text, text + text_length)});
        return S_OK;
    }

    std::vector<Item> items;
};

int main() {
    using namespace localpinyin;

    HMODULE test_module = GetModuleHandleW(nullptr);
    REQUIRE_TRUE(test_module != nullptr);
    REQUIRE_TRUE(FindResourceW(test_module, MAKEINTRESOURCEW(IDI_MODE_ZH), RT_GROUP_ICON) != nullptr);
    REQUIRE_TRUE(FindResourceW(test_module, MAKEINTRESOURCEW(IDI_MODE_EN), RT_GROUP_ICON) != nullptr);

    REQUIRE_EQ(std::wstring(input_mode_label(InputMode::Chinese)), std::wstring(L"\u4E2D"));
    REQUIRE_EQ(std::wstring(input_mode_label(InputMode::English)), std::wstring(L"\u82F1"));
    REQUIRE_EQ(std::wstring(input_mode_label(InputMode::Assisted)), std::wstring(L"\u8F85"));
    REQUIRE_TRUE(std::wstring(input_mode_tooltip(InputMode::Assisted)).find(L"\u8F85\u52A9\u62FC\u5199") != std::wstring::npos);
    REQUIRE_TRUE(std::wstring(input_mode_tooltip(InputMode::Chinese)).find(L"\u4E2D\u6587\u6A21\u5F0F") != std::wstring::npos);
    REQUIRE_TRUE(std::wstring(input_mode_tooltip(InputMode::English)).find(L"\u82F1\u6587\u76F4\u8F93\u6A21\u5F0F") != std::wstring::npos);

    LanguageBarState state;
    TF_LANGBARITEMINFO info{};
    REQUIRE_EQ(state.GetInfo(&info), S_OK);
    REQUIRE_TRUE(IsEqualGUID(info.clsidService, CLSID_LocalPinyinTextService));
    REQUIRE_TRUE(IsEqualGUID(info.guidItem, GUID_LBI_INPUTMODE));
    REQUIRE_TRUE((info.dwStyle & TF_LBI_STYLE_SHOWNINTRAY) == TF_LBI_STYLE_SHOWNINTRAY);
    REQUIRE_TRUE((info.dwStyle & TF_LBI_STYLE_TEXTCOLORICON) == TF_LBI_STYLE_TEXTCOLORICON);
    REQUIRE_TRUE((info.dwStyle & TF_LBI_STYLE_BTN_BUTTON) == TF_LBI_STYLE_BTN_BUTTON);
    REQUIRE_TRUE((info.dwStyle & TF_LBI_STYLE_BTN_MENU) == TF_LBI_STYLE_BTN_MENU);
    REQUIRE_TRUE((info.dwStyle & TF_LBI_STYLE_BTN_TOGGLE) == TF_LBI_STYLE_BTN_TOGGLE);

    LangBarSink sink;
    DWORD cookie = 0;
    REQUIRE_EQ(state.AdviseSink(__uuidof(ITfLangBarItemSink), &sink, &cookie), S_OK);
    REQUIRE_EQ(sink.update_count, 1);
    REQUIRE_TRUE((sink.last_flags & TF_LBI_ICON) == TF_LBI_ICON);

    DWORD status = 0;
    REQUIRE_EQ(state.GetStatus(&status), S_OK);
    REQUIRE_TRUE((status & TF_LBI_STATUS_HIDDEN) == 0);
    REQUIRE_TRUE((status & TF_LBI_STATUS_BTN_TOGGLED) == 0);

    BSTR text = nullptr;
    REQUIRE_EQ(state.GetText(&text), S_OK);
    REQUIRE_TRUE(text != nullptr);
    REQUIRE_EQ(std::wstring(text), std::wstring(L"\u4E2D"));
    SysFreeString(text);

    HICON chinese_icon = nullptr;
    REQUIRE_EQ(state.GetIcon(&chinese_icon), S_OK);
    REQUIRE_TRUE(chinese_icon != nullptr);
    const auto chinese_pixels = render_icon_pixels(chinese_icon, 32);
    REQUIRE_TRUE(nontransparent_pixel_count(chinese_pixels) > 32);

    BSTR tooltip = nullptr;
    REQUIRE_EQ(state.GetTooltipString(&tooltip), S_OK);
    REQUIRE_TRUE(tooltip != nullptr);
    REQUIRE_TRUE(std::wstring(tooltip).find(L"\u4E2D\u6587") != std::wstring::npos);
    SysFreeString(tooltip);

    state.set_mode(InputMode::English);
    REQUIRE_EQ(sink.update_count, 2);
    REQUIRE_TRUE((sink.last_flags & TF_LBI_ICON) == TF_LBI_ICON);
    REQUIRE_TRUE((sink.last_flags & TF_LBI_TEXT) == TF_LBI_TEXT);
    REQUIRE_TRUE((sink.last_flags & TF_LBI_STATUS) == TF_LBI_STATUS);
    REQUIRE_TRUE((sink.last_flags & TF_LBI_TOOLTIP) == TF_LBI_TOOLTIP);
    REQUIRE_EQ(state.GetStatus(&status), S_OK);
    REQUIRE_TRUE((status & TF_LBI_STATUS_BTN_TOGGLED) == TF_LBI_STATUS_BTN_TOGGLED);
    REQUIRE_EQ(state.GetText(&text), S_OK);
    REQUIRE_EQ(std::wstring(text), std::wstring(L"\u82F1"));
    SysFreeString(text);

    HICON english_icon = nullptr;
    REQUIRE_EQ(state.GetIcon(&english_icon), S_OK);
    REQUIRE_TRUE(english_icon != nullptr);
    REQUIRE_TRUE(chinese_icon != english_icon);
    const auto english_pixels = render_icon_pixels(english_icon, 32);
    REQUIRE_TRUE(nontransparent_pixel_count(english_pixels) > 32);
    REQUIRE_TRUE(chinese_pixels != english_pixels);
    DestroyIcon(chinese_icon);
    DestroyIcon(english_icon);

    REQUIRE_EQ(state.GetTooltipString(&tooltip), S_OK);
    REQUIRE_TRUE(tooltip != nullptr);
    REQUIRE_TRUE(std::wstring(tooltip).find(L"\u82F1\u6587") != std::wstring::npos);
    SysFreeString(tooltip);

    state.set_mode(InputMode::Chinese);
    REQUIRE_EQ(sink.update_count, 3);
    REQUIRE_TRUE((sink.last_flags & TF_LBI_ICON) == TF_LBI_ICON);
    REQUIRE_EQ(state.GetText(&text), S_OK);
    REQUIRE_EQ(std::wstring(text), std::wstring(L"\u4E2D"));
    SysFreeString(text);

    LanguageBarState no_sink_state;
    no_sink_state.set_mode(InputMode::English);
    REQUIRE_EQ(no_sink_state.GetText(&text), S_OK);
    REQUIRE_EQ(std::wstring(text), std::wstring(L"\u82F1"));
    SysFreeString(text);

    int toggle_count = 0;
    int settings_count = 0;
    state.set_toggle_callback([&toggle_count](const wchar_t*) {
        ++toggle_count;
        return S_OK;
    });
    state.set_settings_callback([&settings_count](const wchar_t*) {
        ++settings_count;
        return S_OK;
    });
    POINT click_point{};
    RECT click_rect{};
    REQUIRE_EQ(state.OnClick(TF_LBI_CLK_LEFT, click_point, &click_rect), S_OK);
    REQUIRE_EQ(toggle_count, 1);
    REQUIRE_EQ(settings_count, 0);
    state.set_popup_menu_callback([]() {
        return ModeIconPopupCommand::Cancel;
    });
    REQUIRE_EQ(state.OnClick(TF_LBI_CLK_RIGHT, click_point, &click_rect), S_OK);
    REQUIRE_EQ(toggle_count, 1);
    state.set_popup_menu_callback([]() {
        return ModeIconPopupCommand::Toggle;
    });
    REQUIRE_EQ(state.OnClick(TF_LBI_CLK_RIGHT, click_point, &click_rect), S_OK);
    REQUIRE_EQ(toggle_count, 2);
    state.set_popup_menu_callback([]() {
        return ModeIconPopupCommand::Settings;
    });
    REQUIRE_EQ(state.OnClick(TF_LBI_CLK_RIGHT, click_point, &click_rect), S_OK);
    REQUIRE_EQ(settings_count, 1);

    FakeMenu menu;
    REQUIRE_EQ(state.InitMenu(&menu), S_OK);
    REQUIRE_EQ(static_cast<int>(menu.items.size()), 2);
    REQUIRE_EQ(menu.items[0].id, 1001u);
    REQUIRE_EQ(menu.items[0].text, std::wstring(L"Switch Chinese/English"));
    REQUIRE_EQ(menu.items[1].id, 1002u);
    REQUIRE_EQ(menu.items[1].text, std::wstring(L"Open Settings"));
    REQUIRE_EQ(state.OnMenuSelect(1001), S_OK);
    REQUIRE_EQ(toggle_count, 3);
    REQUIRE_EQ(state.OnMenuSelect(1002), S_OK);
    REQUIRE_EQ(settings_count, 2);
    state.clear_callbacks();
    REQUIRE_EQ(state.OnClick(TF_LBI_CLK_LEFT, click_point, &click_rect), E_NOTIMPL);
    state.set_popup_menu_callback([]() {
        return ModeIconPopupCommand::Toggle;
    });
    REQUIRE_EQ(state.OnClick(TF_LBI_CLK_RIGHT, click_point, &click_rect), E_NOTIMPL);
    state.set_popup_menu_callback([]() {
        return ModeIconPopupCommand::Settings;
    });
    REQUIRE_EQ(state.OnClick(TF_LBI_CLK_RIGHT, click_point, &click_rect), E_NOTIMPL);
    REQUIRE_EQ(state.OnMenuSelect(1001), E_NOTIMPL);
    REQUIRE_EQ(state.OnMenuSelect(1002), E_NOTIMPL);

    LanguageBarState retry_state;
    LangBarSink flaky_sink;
    flaky_sink.fail_next_update = true;
    DWORD flaky_cookie = 0;
    REQUIRE_EQ(retry_state.AdviseSink(__uuidof(ITfLangBarItemSink), &flaky_sink, &flaky_cookie), S_OK);
    REQUIRE_TRUE(retry_state.pending_initial_update());
    REQUIRE_EQ(flaky_sink.update_count, 1);
    REQUIRE_EQ(retry_state.retry_pending_initial_update(L"test_retry"), S_OK);
    REQUIRE_TRUE(!retry_state.pending_initial_update());
    REQUIRE_EQ(retry_state.initial_update_retry_count(), 0u);
    REQUIRE_EQ(flaky_sink.update_count, 2);
    REQUIRE_TRUE((flaky_sink.last_flags & TF_LBI_ICON) == TF_LBI_ICON);
    REQUIRE_EQ(retry_state.UnadviseSink(flaky_cookie), S_OK);

    REQUIRE_EQ(state.Show(FALSE), S_OK);
    REQUIRE_EQ(state.GetStatus(&status), S_OK);
    REQUIRE_TRUE((status & TF_LBI_STATUS_HIDDEN) == TF_LBI_STATUS_HIDDEN);

    REQUIRE_EQ(state.GetTooltipString(&tooltip), S_OK);
    REQUIRE_TRUE(tooltip != nullptr);
    REQUIRE_TRUE(std::wstring(tooltip).find(L"LocalPinyinIME") != std::wstring::npos);
    SysFreeString(tooltip);

    REQUIRE_EQ(state.GetText(nullptr), E_POINTER);
    REQUIRE_EQ(state.GetTooltipString(nullptr), E_POINTER);
    REQUIRE_EQ(state.GetStatus(nullptr), E_POINTER);
    REQUIRE_EQ(state.GetInfo(nullptr), E_POINTER);
    REQUIRE_EQ(state.UnadviseSink(cookie), S_OK);

    return 0;
}
