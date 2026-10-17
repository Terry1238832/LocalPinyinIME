#include "typo_correction.h"

#include <algorithm>
#include <unordered_map>

namespace localpinyin {
namespace {

// Keys that touch each key on a QWERTY layout (same row and staggered rows).
const wchar_t* neighbors_of(wchar_t key) {
    switch (key) {
        case L'q': return L"wa";       case L'w': return L"qeas";     case L'e': return L"wrsd";
        case L'r': return L"etdf";     case L't': return L"ryfg";     case L'y': return L"tugh";
        case L'u': return L"yihj";     case L'i': return L"uojk";     case L'o': return L"ipkl";
        case L'p': return L"ol";       case L'a': return L"qwsz";     case L's': return L"adwezx";
        case L'd': return L"sferxc";   case L'f': return L"dgrtcv";   case L'g': return L"fhtyvb";
        case L'h': return L"gjyubn";   case L'j': return L"hkuinm";   case L'k': return L"jliom";
        case L'l': return L"kop";      case L'z': return L"asx";      case L'x': return L"zcsd";
        case L'c': return L"xvdf";     case L'v': return L"cbfg";     case L'b': return L"vngh";
        case L'n': return L"bmhj";     case L'm': return L"njk";
        default: return L"";
    }
}

constexpr int kSwapCost = 300;
constexpr int kDoubledKeyCost = 250;
constexpr int kNeighborExtraCost = 300;
constexpr int kExtraCost = 450;
constexpr int kMissingVowelCost = 450;
constexpr int kMissingCost = 520;
constexpr int kNeighborCost = 350;
constexpr int kDistantCost = 800;

bool vowel(wchar_t ch) {
    return ch == L'a' || ch == L'e' || ch == L'i' || ch == L'o' || ch == L'u' || ch == L'v';
}

// Edits that are sound confusions handled by fuzzy pinyin instead.
bool fuzzy_insertion_or_deletion(const std::wstring& input, size_t position, wchar_t letter) {
    const wchar_t before = position > 0 ? input[position - 1] : 0;
    if (letter == L'h' && (before == L'z' || before == L'c' || before == L's')) return true;
    if (letter == L'g' && before == L'n') return true;
    return false;
}

bool fuzzy_substitution(wchar_t from, wchar_t to) {
    const auto pair = [&](wchar_t a, wchar_t b) { return (from == a && to == b) || (from == b && to == a); };
    return pair(L'n', L'l') || pair(L'l', L'r') || pair(L'f', L'h');
}

}  // namespace

bool keyboard_neighbors(wchar_t a, wchar_t b) noexcept {
    const wchar_t* list = neighbors_of(a);
    for (; *list; ++list) if (*list == b) return true;
    return false;
}

std::vector<SpellingCorrection> spelling_corrections_near(const std::wstring& input, size_t first, size_t last,
                                                          bool broad) {
    std::unordered_map<std::wstring, int> best;
    const auto add = [&](std::wstring pinyin, int cost) {
        if (pinyin.empty() || pinyin == input) return;
        auto [it, inserted] = best.try_emplace(std::move(pinyin), cost);
        if (!inserted) it->second = std::min(it->second, cost);
    };
    last = std::min(last, input.size());
    for (size_t index = first; index < last; ++index) {
        const wchar_t key = input[index];
        // An extra key: doubled, a neighbor of the key before or after, or stray.
        if (!fuzzy_insertion_or_deletion(input, index, key)) {
            const wchar_t before = index > 0 ? input[index - 1] : 0;
            const wchar_t after = index + 1 < input.size() ? input[index + 1] : 0;
            const int cost = key == before || key == after ? kDoubledKeyCost
                           : keyboard_neighbors(key, before) || keyboard_neighbors(key, after) ? kNeighborExtraCost
                           : kExtraCost;
            add(input.substr(0, index) + input.substr(index + 1), cost);
        }
        // Two keys swapped.
        if (index + 1 < input.size() && input[index] != input[index + 1]) {
            auto swapped = input;
            std::swap(swapped[index], swapped[index + 1]);
            add(std::move(swapped), kSwapCost);
        }
        // A neighbor hit instead (or, broadly, any other key).
        for (wchar_t letter = L'a'; letter <= L'z'; ++letter) {
            if (letter == key || fuzzy_substitution(key, letter)) continue;
            const bool neighbor = keyboard_neighbors(key, letter);
            if (!neighbor && !broad) continue;
            auto replaced = input;
            replaced[index] = letter;
            add(std::move(replaced), neighbor ? kNeighborCost : kDistantCost);
        }
    }
    // A missing key, before each position in range and at its end.
    for (size_t index = first; index <= last; ++index) {
        for (wchar_t letter = L'a'; letter <= L'z'; ++letter) {
            if (fuzzy_insertion_or_deletion(input, index, letter)) continue;
            add(input.substr(0, index) + letter + input.substr(index), vowel(letter) ? kMissingVowelCost : kMissingCost);
        }
    }
    std::vector<SpellingCorrection> result;
    result.reserve(best.size());
    for (auto& [pinyin, cost] : best) result.push_back({pinyin, cost});
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
        return left.cost != right.cost ? left.cost < right.cost : left.pinyin < right.pinyin;
    });
    return result;
}

std::vector<SpellingCorrection> spelling_corrections(const std::wstring& input, bool broad) {
    return spelling_corrections_near(input, 0, input.size(), broad);
}

}  // namespace localpinyin
