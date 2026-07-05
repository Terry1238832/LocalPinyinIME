#pragma once

#include <windows.h>
#include <msctf.h>

#include <cstring>

namespace localpinyin {

// Project-local preserved key used only while this TSF text service is active.
inline constexpr GUID GUID_LocalPinyinCtrlSpacePreservedKey =
    {0x1a7e9f54, 0x0e45, 0x43f2, {0x9b, 0x4c, 0x52, 0x9a, 0xa7, 0x23, 0x63, 0x81}};

enum class PreservedKeyAction {
    Ignore,
    ToggleInputMode
};

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
    return same_guid(guid, GUID_LocalPinyinCtrlSpacePreservedKey)
        ? PreservedKeyAction::ToggleInputMode
        : PreservedKeyAction::Ignore;
}

}  // namespace localpinyin
