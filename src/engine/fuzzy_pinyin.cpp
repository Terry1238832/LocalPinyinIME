#include "fuzzy_pinyin.h"

#include "dictionary.h"
#include "input_options.h"

#include <algorithm>

namespace localpinyin {
namespace {

// Splits a syllable into its initial (possibly empty) and final.
std::pair<std::wstring, std::wstring> split(const std::wstring& syllable) {
    if (syllable.size() >= 2 && (syllable[0] == L'z' || syllable[0] == L'c' || syllable[0] == L's') && syllable[1] == L'h')
        return {syllable.substr(0, 2), syllable.substr(2)};
    if (!syllable.empty() && std::wstring(L"bpmfdtnlgkhjqxrzcsyw").find(syllable[0]) != std::wstring::npos)
        return {syllable.substr(0, 1), syllable.substr(1)};
    return {L"", syllable};
}

std::vector<std::wstring> initial_variants(const std::wstring& initial, unsigned mask) {
    std::vector<std::wstring> result{initial};
    const auto pair = [&](unsigned bit, const wchar_t* a, const wchar_t* b) {
        if (!(mask & bit)) return;
        if (initial == a) result.push_back(b);
        else if (initial == b) result.push_back(a);
    };
    pair(kFuzzyZZh, L"z", L"zh");
    pair(kFuzzyCCh, L"c", L"ch");
    pair(kFuzzySSh, L"s", L"sh");
    pair(kFuzzyNL, L"n", L"l");
    pair(kFuzzyFH, L"f", L"h");
    pair(kFuzzyRL, L"r", L"l");
    return result;
}

std::vector<std::wstring> final_variants(const std::wstring& final, unsigned mask) {
    std::vector<std::wstring> result{final};
    const auto pair = [&](unsigned bit, const wchar_t* a, const wchar_t* b) {
        if (!(mask & bit)) return;
        if (final == a) result.push_back(b);
        else if (final == b) result.push_back(a);
    };
    pair(kFuzzyAnAng, L"an", L"ang");
    pair(kFuzzyEnEng, L"en", L"eng");
    pair(kFuzzyInIng, L"in", L"ing");
    pair(kFuzzyIanIang, L"ian", L"iang");
    pair(kFuzzyUanUang, L"uan", L"uang");
    return result;
}

}  // namespace

std::vector<std::wstring> fuzzy_syllable_variants(const std::wstring& syllable, unsigned mask) {
    std::vector<std::wstring> result;
    if (!mask || !Dictionary::is_syllable(syllable)) return result;
    const auto [initial, final] = split(syllable);
    for (const auto& i : initial_variants(initial, mask)) {
        for (const auto& f : final_variants(final, mask)) {
            const std::wstring variant = i + f;
            if (variant != syllable && Dictionary::is_syllable(variant) &&
                std::find(result.begin(), result.end(), variant) == result.end()) result.push_back(variant);
        }
    }
    return result;
}

}  // namespace localpinyin
