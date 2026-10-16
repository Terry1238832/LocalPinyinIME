#include "test_common.h"
#include "../src/engine/pinyin_engine.h"
#include "../src/common/utf_utils.h"

#include <algorithm>
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

int rank_of(const std::vector<localpinyin::Candidate>& candidates, const std::wstring& word) {
    const auto found = std::find_if(candidates.begin(), candidates.end(), [&](const auto& c) { return c.text == word; });
    return found == candidates.end() ? 0 : static_cast<int>(found - candidates.begin()) + 1;
}

struct Case {
    const wchar_t* pinyin;
    const wchar_t* word;
    const wchar_t* preceding;
};

}  // namespace

// A word the user keeps choosing must move to the first position, with and
// without surrounding text, and stay there after the engine is reopened.
int main() {
    const auto root = use_temp_user_lexicon_override();
    const auto resources = executable_directory();
    const auto user = (root / L"user_lexicon.tsv").wstring();
    const std::vector<Case> cases = {
        {L"anniu", L"按钮", L""},
        // A transposed spelling corrected by the engine must also learn.
        {L"annui", L"按钮", L""},
        {L"anniu", L"按钮", L"点击这个"},
        {L"shijian", L"事件", L"没有"},
        {L"xiangmu", L"项目", L""},
    };
    bool passed = true;
    for (const auto& item : cases) {
        localpinyin::PinyinEngine engine(localpinyin::DictionaryLoadMode::Empty);
        REQUIRE_TRUE(engine.load_dictionary_resource_directory(resources.wstring(), user));
        const int before = rank_of(engine.lookup(item.pinyin, item.preceding), item.word);
        int promoted_after = 0;
        for (int selection = 1; selection <= 3 && !promoted_after; ++selection) {
            engine.learn(item.pinyin, item.word);
            if (rank_of(engine.lookup(item.pinyin, item.preceding), item.word) == 1) promoted_after = selection;
        }
        localpinyin::PinyinEngine reopened(localpinyin::DictionaryLoadMode::Empty);
        REQUIRE_TRUE(reopened.load_dictionary_resource_directory(resources.wstring(), user));
        const int after_restart = rank_of(reopened.lookup(item.pinyin, item.preceding), item.word);
        std::cout << localpinyin::wide_to_utf8(item.pinyin) << " " << localpinyin::wide_to_utf8(item.word)
                  << " context=" << localpinyin::wide_to_utf8(item.preceding) << " before=" << before
                  << " first_after_selections=" << promoted_after << " after_restart=" << after_restart << "\n";
        passed = passed && before > 0 && promoted_after > 0 && promoted_after <= 2 && after_restart == 1;
        localpinyin::UserLearning clearer;
        REQUIRE_TRUE(clearer.open((root / L"user_learning.tsv").wstring()));
        REQUIRE_TRUE(clearer.clear());
    }
    std::filesystem::remove_all(root);
    return passed ? 0 : 1;
}
