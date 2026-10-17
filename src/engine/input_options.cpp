#include "input_options.h"

#include <array>
#include <utility>

namespace localpinyin {
namespace {

constexpr std::array<std::pair<FuzzyPinyin, const wchar_t*>, 11> kFuzzyIds{{
    {kFuzzyZZh, L"z-zh"}, {kFuzzyCCh, L"c-ch"}, {kFuzzySSh, L"s-sh"}, {kFuzzyNL, L"n-l"},
    {kFuzzyFH, L"f-h"}, {kFuzzyRL, L"r-l"}, {kFuzzyAnAng, L"an-ang"}, {kFuzzyEnEng, L"en-eng"},
    {kFuzzyInIng, L"in-ing"}, {kFuzzyIanIang, L"ian-iang"}, {kFuzzyUanUang, L"uan-uang"},
}};

constexpr std::array<std::pair<UsageDomain, const wchar_t*>, 6> kDomainIds{{
    {kDomainCoding, L"coding"}, {kDomainWork, L"work"}, {kDomainMath, L"math"},
    {kDomainStudy, L"study"}, {kDomainDaily, L"daily"}, {kDomainGames, L"games"},
}};

template <typename Ids>
std::wstring ids_to_string(const Ids& ids, unsigned mask) {
    std::wstring result;
    for (const auto& [bit, id] : ids) {
        if (!(mask & bit)) continue;
        if (!result.empty()) result.push_back(L',');
        result += id;
    }
    return result;
}

template <typename Ids>
unsigned parse_ids(const Ids& ids, const std::wstring& value) noexcept {
    unsigned mask = 0;
    size_t start = 0;
    while (start <= value.size()) {
        const size_t comma = value.find(L',', start);
        const std::wstring item = value.substr(start, comma == std::wstring::npos ? std::wstring::npos : comma - start);
        for (const auto& [bit, id] : ids) {
            if (item == id) mask |= bit;
        }
        if (comma == std::wstring::npos) break;
        start = comma + 1;
    }
    return mask;
}

}  // namespace

std::wstring fuzzy_pinyin_to_string(unsigned mask) { return ids_to_string(kFuzzyIds, mask); }

unsigned parse_fuzzy_pinyin(const std::wstring& value) noexcept { return parse_ids(kFuzzyIds, value); }

std::wstring usage_domains_to_string(unsigned mask) { return ids_to_string(kDomainIds, mask); }

unsigned parse_usage_domains(const std::wstring& value) noexcept { return parse_ids(kDomainIds, value); }

const wchar_t* usage_domain_id(UsageDomain domain) noexcept {
    for (const auto& [bit, id] : kDomainIds) {
        if (bit == domain) return id;
    }
    return L"";
}

std::wstring double_pinyin_to_string(DoublePinyinScheme scheme) {
    switch (scheme) {
        case DoublePinyinScheme::Xiaohe: return L"xiaohe";
        case DoublePinyinScheme::Ziranma: return L"ziranma";
        case DoublePinyinScheme::None:
        default: return L"none";
    }
}

DoublePinyinScheme parse_double_pinyin(const std::wstring& value) noexcept {
    if (value == L"xiaohe") return DoublePinyinScheme::Xiaohe;
    if (value == L"ziranma") return DoublePinyinScheme::Ziranma;
    return DoublePinyinScheme::None;
}

}  // namespace localpinyin
