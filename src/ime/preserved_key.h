#pragma once

#include <windows.h>
#include <msctf.h>

#include <cstring>
#include <string_view>

namespace localpinyin {

// Project-local preserved key used only while this TSF text service is active.
inline constexpr GUID GUID_LocalPinyinCtrlSpacePreservedKey =
    {0x1a7e9f54, 0x0e45, 0x43f2, {0x9b, 0x4c, 0x52, 0x9a, 0xa7, 0x23, 0x63, 0x81}};

// The key that switches between 本地拼音 and 本地英文 (setting imeSwitchKey).
inline constexpr GUID GUID_LocalPinyinSwitchProfilePreservedKey =
    {0x6b2c41d8, 0x3f7a, 0x4e19, {0x8d, 0x52, 0x1c, 0xe0, 0x9a, 0x47, 0xb3, 0x6e}};

enum class PreservedKeyAction {
    Ignore,
    ToggleInputMode,
    SwitchProfile
};

// The switch key a setting names; false for off (or an unknown setting).
[[nodiscard]] inline bool ime_switch_preserved_key(std::wstring_view setting, TF_PRESERVEDKEY* key) noexcept {
    if (setting == L"alt+capslock") { *key = TF_PRESERVEDKEY{VK_CAPITAL, TF_MOD_ALT}; return true; }
    if (setting == L"ctrl+capslock") { *key = TF_PRESERVEDKEY{VK_CAPITAL, TF_MOD_CONTROL}; return true; }
    if (setting == L"ctrl+alt+space") { *key = TF_PRESERVEDKEY{VK_SPACE, TF_MOD_CONTROL | TF_MOD_ALT}; return true; }
    return false;
}

[[nodiscard]] constexpr TF_PRESERVEDKEY ctrl_space_preserved_key() noexcept {
    return TF_PRESERVEDKEY{VK_SPACE, TF_MOD_CONTROL};
}

[[nodiscard]] constexpr const wchar_t* ctrl_space_preserved_key_description() noexcept {
    return L"LocalPinyinIME Ctrl+Space input mode toggle";
}

[[nodiscard]] inline bool same_guid(REFGUID left, REFGUID right) noexcept {
    return left.Data1 == right.Data1 &&
           left.Data2 == right.Data2 &&
           left.Data3 == right.Data3 &&
           std::memcmp(left.Data4, right.Data4, sizeof(left.Data4)) == 0;
}

[[nodiscard]] inline bool is_ctrl_space_preserved_key(const TF_PRESERVEDKEY& key) noexcept {
    const TF_PRESERVEDKEY expected = ctrl_space_preserved_key();
    return key.uVKey == expected.uVKey && key.uModifiers == expected.uModifiers;
}

[[nodiscard]] inline PreservedKeyAction preserved_key_action_for_guid(REFGUID guid) noexcept {
    if (same_guid(guid, GUID_LocalPinyinCtrlSpacePreservedKey)) return PreservedKeyAction::ToggleInputMode;
    if (same_guid(guid, GUID_LocalPinyinSwitchProfilePreservedKey)) return PreservedKeyAction::SwitchProfile;
    return PreservedKeyAction::Ignore;
}

}  // namespace localpinyin
