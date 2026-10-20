#pragma once

#include <guiddef.h>

namespace localpinyin {

extern const CLSID CLSID_LocalPinyinTextService;
// 本地拼音 and 本地英文: two input methods of the same text service, both
// under Chinese (Simplified) so the taskbar shows their own 中 / 英 icons.
extern const GUID GUID_LocalPinyinProfile;
extern const GUID GUID_LocalPinyinEnglishProfile;

}  // namespace localpinyin
