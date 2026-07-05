#pragma once

#include "guids.h"

#include <msctf.h>

#include <array>

namespace localpinyin {

enum class TsfCapabilityCheckStrategy {
    CategoryContainsItem,
    ProfileCaps,
};

struct TsfProfileCategory {
    const wchar_t* name;
    const GUID* category_guid;
    const GUID* item_guid;
    bool allow_register_category;
    TsfCapabilityCheckStrategy check_strategy;
    DWORD profile_caps;
};

inline const std::array<TsfProfileCategory, 4>& required_tsf_profile_capabilities() noexcept {
    static const std::array<TsfProfileCategory, 4> categories{{
        {L"GUID_TFCAT_TIP_KEYBOARD",
         &GUID_TFCAT_TIP_KEYBOARD,
         &CLSID_LocalPinyinTextService,
         true,
         TsfCapabilityCheckStrategy::CategoryContainsItem,
         0},
        {L"GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT",
         &GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT,
         &CLSID_LocalPinyinTextService,
         false,
         TsfCapabilityCheckStrategy::ProfileCaps,
         TF_IPP_CAPS_IMMERSIVESUPPORT},
        {L"GUID_TFCAT_TIPCAP_UIELEMENTENABLED",
         &GUID_TFCAT_TIPCAP_UIELEMENTENABLED,
         &CLSID_LocalPinyinTextService,
         false,
         TsfCapabilityCheckStrategy::ProfileCaps,
         TF_IPP_CAPS_UIELEMENTENABLED},
        {L"GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT",
         &GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT,
         &CLSID_LocalPinyinTextService,
         false,
         TsfCapabilityCheckStrategy::ProfileCaps,
         TF_IPP_CAPS_SYSTRAYSUPPORT},
    }};
    return categories;
}

inline const std::array<TsfProfileCategory, 1>& required_tsf_profile_categories() noexcept {
    static const std::array<TsfProfileCategory, 1> categories{{
        required_tsf_profile_capabilities()[0],
    }};
    return categories;
}

[[nodiscard]] constexpr DWORD required_tsf_profile_caps() noexcept {
    return TF_IPP_CAPS_IMMERSIVESUPPORT | TF_IPP_CAPS_UIELEMENTENABLED | TF_IPP_CAPS_SYSTRAYSUPPORT;
}

}  // namespace localpinyin
