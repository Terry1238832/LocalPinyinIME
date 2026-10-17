#include "double_pinyin.h"

#include "dictionary.h"

#include <vector>

namespace localpinyin {
namespace {

// A vector, not an initializer_list: a returned initializer_list's array
// would not outlive the return statement.
using Finals = std::vector<const wchar_t*>;

// The final(s) each key stands for after an initial. Where a key carries two
// finals, the one that forms a legal syllable with the initial is meant.
Finals xiaohe_finals(wchar_t key) {
    switch (key) {
        case L'q': return {L"iu"};
        case L'w': return {L"ei"};
        case L'e': return {L"e"};
        case L'r': return {L"uan"};
        case L't': return {L"ue", L"ve"};
        case L'y': return {L"un"};
        case L'u': return {L"u"};
        case L'i': return {L"i"};
        case L'o': return {L"uo", L"o"};
        case L'p': return {L"ie"};
        case L'a': return {L"a"};
        case L's': return {L"ong", L"iong"};
        case L'd': return {L"ai"};
        case L'f': return {L"en"};
        case L'g': return {L"eng"};
        case L'h': return {L"ang"};
        case L'j': return {L"an"};
        case L'k': return {L"ing", L"uai"};
        case L'l': return {L"iang", L"uang"};
        case L'z': return {L"ou"};
        case L'x': return {L"ia", L"ua"};
        case L'c': return {L"ao"};
        case L'v': return {L"ui", L"v"};
        case L'b': return {L"in"};
        case L'n': return {L"iao"};
        case L'm': return {L"ian"};
        default: return {};
    }
}

Finals ziranma_finals(wchar_t key) {
    switch (key) {
        case L'q': return {L"iu"};
        case L'w': return {L"ia", L"ua"};
        case L'e': return {L"e"};
        case L'r': return {L"uan"};
        case L't': return {L"ue", L"ve"};
        case L'y': return {L"ing", L"uai"};
        case L'u': return {L"u"};
        case L'i': return {L"i"};
        case L'o': return {L"uo", L"o"};
        case L'p': return {L"un"};
        case L'a': return {L"a"};
        case L's': return {L"ong", L"iong"};
        case L'd': return {L"iang", L"uang"};
        case L'f': return {L"en"};
        case L'g': return {L"eng"};
        case L'h': return {L"ang"};
        case L'j': return {L"an"};
        case L'k': return {L"ao"};
        case L'l': return {L"ai"};
        case L'z': return {L"ei"};
        case L'x': return {L"ie"};
        case L'c': return {L"iao"};
        case L'v': return {L"ui", L"v"};
        case L'b': return {L"ou"};
        case L'n': return {L"in"};
        case L'm': return {L"ian"};
        default: return {};
    }
}

Finals finals_for(wchar_t key, DoublePinyinScheme scheme) {
    return scheme == DoublePinyinScheme::Ziranma ? ziranma_finals(key) : xiaohe_finals(key);
}

// Both schemes: v, i and u as the first key are zh, ch and sh.
std::wstring initial_for(wchar_t key) {
    switch (key) {
        case L'v': return L"zh";
        case L'i': return L"ch";
        case L'u': return L"sh";
        default: return std::wstring(1, key);
    }
}

bool zero_initial(wchar_t key) {
    return key == L'a' || key == L'e' || key == L'o';
}

wchar_t lower(wchar_t ch) {
    return ch >= L'A' && ch <= L'Z' ? static_cast<wchar_t>(ch - L'A' + L'a') : ch;
}

bool letter(wchar_t ch) {
    return ch >= L'a' && ch <= L'z';
}

}  // namespace

std::wstring double_pinyin_syllable(wchar_t first, wchar_t second, DoublePinyinScheme scheme) {
    first = lower(first);
    second = lower(second);
    if (!letter(first) || !letter(second) || scheme == DoublePinyinScheme::None) return {};
    if (zero_initial(first)) {
        // Syllables without an initial: aa a, ai ai, an an, ah ang, ao ao,
        // ee e, ei ei, en en, eg eng, er er, oo o, ou ou.
        if (second == first) return std::wstring(1, first);
        const std::wstring spelled{first, second};
        if (Dictionary::is_syllable(spelled)) return spelled;
        for (const wchar_t* final : finals_for(second, scheme)) {
            if (final[0] == first && Dictionary::is_syllable(final)) return final;
        }
        return {};
    }
    const std::wstring initial = initial_for(first);
    for (const wchar_t* final : finals_for(second, scheme)) {
        const std::wstring syllable = initial + final;
        if (Dictionary::is_syllable(syllable)) return syllable;
    }
    return {};
}

DoublePinyinConversion convert_double_pinyin(const std::wstring& typed, DoublePinyinScheme scheme) {
    DoublePinyinConversion result;
    result.raw_end.push_back(0);
    const auto append = [&](const std::wstring& text, size_t raw_after) {
        for (size_t index = 0; index < text.size(); ++index) {
            result.pinyin.push_back(text[index]);
            result.raw_end.push_back(raw_after);
        }
    };
    const auto separate = [&](size_t raw_position) {
        if (!result.pinyin.empty() && result.pinyin.back() != L'\'') append(L"'", raw_position);
    };
    for (size_t index = 0; index < typed.size();) {
        const wchar_t key = lower(typed[index]);
        if (key == L'\'') {
            separate(index + 1);
            // The typed apostrophe belongs to the separator just written.
            if (!result.raw_end.empty()) result.raw_end.back() = index + 1;
            ++index;
            continue;
        }
        if (!letter(key)) {
            ++index;
            continue;
        }
        const bool pair = index + 1 < typed.size() && letter(lower(typed[index + 1]));
        const std::wstring syllable = pair ? double_pinyin_syllable(key, typed[index + 1], scheme) : std::wstring{};
        separate(index);
        if (!syllable.empty()) {
            append(syllable, index + 2);
            index += 2;
        } else {
            // A single key, or a pair that forms no syllable: the key's
            // initial, which the engine completes like unfinished pinyin.
            append(zero_initial(key) ? std::wstring(1, key) : initial_for(key), index + 1);
            ++index;
        }
    }
    return result;
}

}  // namespace localpinyin
