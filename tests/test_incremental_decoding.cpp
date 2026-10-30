#include "test_common.h"
#include "../src/engine/pinyin_engine.h"
#include "../src/common/utf_utils.h"

#include <array>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::filesystem::path executable_directory() {
    std::array<wchar_t, MAX_PATH> path{};
    const DWORD size = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    return size == 0 || size >= path.size() ? std::filesystem::current_path()
                                            : std::filesystem::path(path.data()).parent_path();
}

std::vector<std::wstring> texts(const std::vector<localpinyin::Candidate>& candidates) {
    std::vector<std::wstring> result;
    for (const auto& candidate : candidates) result.push_back(candidate.text);
    return result;
}

}  // namespace

// Each keystroke reuses the previous keystroke's sentence lattice. Typing a
// sentence letter by letter, deleting back and typing on, and changing the
// preceding text must give exactly what decoding that input afresh gives.
int main() {
    const auto root = use_temp_user_lexicon_override();
    const auto resources = executable_directory();
    const auto user = (root / L"user_lexicon.tsv").wstring();
    localpinyin::PinyinEngine typing(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(typing.load_dictionary_resource_directory(resources.wstring(), user));

    struct Step { std::wstring input; std::wstring preceding; };
    std::vector<Step> steps;
    const std::wstring sentence = L"woyijingkanguozhebudianyingle";
    for (size_t end = 1; end <= sentence.size(); ++end) steps.push_back({sentence.substr(0, end), L""});
    for (size_t end = sentence.size() - 1; end >= sentence.size() - 9; --end) steps.push_back({sentence.substr(0, end), L""});
    for (const auto* tail : {L"n", L"ni", L"nim", L"nime", L"nimen"})
        steps.push_back({sentence.substr(0, sentence.size() - 9) + tail, L""});
    // Typed boundaries and a change of preceding text.
    steps.push_back({L"xi'an'shi", L""});
    steps.push_back({L"xi'an'shi'de", L""});
    steps.push_back({L"jintiantianqi", L"你好"});
    steps.push_back({L"jintiantianqihen", L"你好"});
    steps.push_back({L"jintiantianqihen", L"昨天"});

    // The reference decodes every input afresh: an unrelated input before
    // each one leaves nothing to reuse.
    localpinyin::PinyinEngine fresh(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(fresh.load_dictionary_resource_directory(resources.wstring(), user));
    bool passed = true;
    for (const auto& step : steps) {
        const auto reused = texts(typing.lookup(step.input, step.preceding));
        (void)fresh.lookup(L"qqqqqq", L"");
        const auto expected = texts(fresh.lookup(step.input, step.preceding));
        if (reused != expected) {
            passed = false;
            std::cout << "differs after typing: " << localpinyin::wide_to_utf8(step.input) << " -> "
                      << (reused.empty() ? std::string() : localpinyin::wide_to_utf8(reused.front())) << " (fresh "
                      << (expected.empty() ? std::string() : localpinyin::wide_to_utf8(expected.front())) << ")\n";
        }
    }
    // Warming the completion indexes (the text service does it after
    // activation) ends after one step per initial and changes no result.
    localpinyin::PinyinEngine warmed(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(warmed.load_dictionary_resource_directory(resources.wstring(), user));
    int warm_steps = 0;
    while (warmed.warm_up_step()) REQUIRE_TRUE(++warm_steps <= 26);
    REQUIRE_TRUE(warm_steps > 0);
    REQUIRE_TRUE(!warmed.warm_up_step());
    for (const auto* input : {L"nihaos", L"woyijingk", L"zhongg", L"q"}) {
        (void)fresh.lookup(L"qqqqqq", L"");
        if (texts(warmed.lookup(input, L"")) != texts(fresh.lookup(input, L""))) {
            passed = false;
            std::cout << "differs after warming: " << localpinyin::wide_to_utf8(input) << "\n";
        }
    }
    std::filesystem::remove_all(root);
    return passed ? 0 : 1;
}
