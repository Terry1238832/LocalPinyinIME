#pragma once

#include <string>

namespace localpinyin {

// Fuzzy pinyin (模糊音): pairs of sounds the user does not distinguish. Each
// pair is a bit; a typed syllable also matches its partner's spelling.
enum FuzzyPinyin : unsigned {
    kFuzzyZZh = 1u << 0,      // z = zh
    kFuzzyCCh = 1u << 1,      // c = ch
    kFuzzySSh = 1u << 2,      // s = sh
    kFuzzyNL = 1u << 3,       // n = l
    kFuzzyFH = 1u << 4,       // f = h
    kFuzzyRL = 1u << 5,       // r = l
    kFuzzyAnAng = 1u << 6,    // an = ang
    kFuzzyEnEng = 1u << 7,    // en = eng
    kFuzzyInIng = 1u << 8,    // in = ing
    kFuzzyIanIang = 1u << 9,  // ian = iang
    kFuzzyUanUang = 1u << 10, // uan = uang
    kFuzzyAll = (1u << 11) - 1
};

// Double pinyin (双拼): every syllable is two keys, an initial and a final.
enum class DoublePinyinScheme {
    None,      // full pinyin
    Xiaohe,    // 小鹤双拼
    Ziranma    // 自然码
};

// Usage scenes (常用场景) the user types in most. Each is a word pack in
// dictionary/domains/<id>.tsv whose words rank higher while it is chosen.
enum UsageDomain : unsigned {
    kDomainCoding = 1u << 0,  // coding: 编程开发
    kDomainWork = 1u << 1,    // work: 办公职场
    kDomainMath = 1u << 2,    // math: 数学理科
    kDomainStudy = 1u << 3,   // study: 学习考试
    kDomainDaily = 1u << 4,   // daily: 日常聊天
    kDomainGames = 1u << 5,   // games: 游戏娱乐
    kDomainAll = (1u << 6) - 1
};

// How typed letters become candidates. Fuzzy pinyin is off and double pinyin
// unused until the user turns them on; spelling correction and Chinese-English
// mixed input are on by default and can be turned off.
struct EngineInputOptions {
    unsigned fuzzy = 0;
    bool typo_correction = true;
    bool mixed_english = true;
    DoublePinyinScheme double_pinyin = DoublePinyinScheme::None;
    unsigned domains = 0;
    // Emoji used from the tool panel come up third for their names.
    bool emoji_candidates = true;

    friend bool operator==(const EngineInputOptions&, const EngineInputOptions&) = default;
};

// "z-zh,n-l,in-ing": the ids of the enabled pairs; unknown ids are ignored.
std::wstring fuzzy_pinyin_to_string(unsigned mask);
unsigned parse_fuzzy_pinyin(const std::wstring& value) noexcept;
// "none", "xiaohe" or "ziranma"; anything else is full pinyin.
std::wstring double_pinyin_to_string(DoublePinyinScheme scheme);
DoublePinyinScheme parse_double_pinyin(const std::wstring& value) noexcept;

// "coding,daily": the ids of the chosen scenes; unknown ids are ignored.
std::wstring usage_domains_to_string(unsigned mask);
unsigned parse_usage_domains(const std::wstring& value) noexcept;
// The id of one scene bit ("coding"), the file name of its pack.
const wchar_t* usage_domain_id(UsageDomain domain) noexcept;

}  // namespace localpinyin
