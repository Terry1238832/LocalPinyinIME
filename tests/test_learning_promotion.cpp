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
    // 造词: a phrase built by choosing 量, 甲 and 成 one after another is
    // offered first for liangjiacheng, also after a restart and as part of a
    // longer input; a phrase the engine already offers first is not stored.
    {
        localpinyin::PinyinEngine engine(localpinyin::DictionaryLoadMode::Empty);
        REQUIRE_TRUE(engine.load_dictionary_resource_directory(resources.wstring(), user));
        REQUIRE_TRUE(rank_of(engine.lookup(L"liangjiacheng"), L"量甲成") != 1);
        REQUIRE_TRUE(engine.learn_phrase(L"liang'jia'cheng", L"量甲成"));
        REQUIRE_EQ(rank_of(engine.lookup(L"liangjiacheng"), L"量甲成"), 1);
        REQUIRE_EQ(rank_of(engine.lookup(L"liangjiacheng", L"我们找"), L"量甲成"), 1);
        REQUIRE_TRUE(!engine.learn_phrase(L"nihaoshijie", L"你好世界"));
        localpinyin::UserLearning reader;
        REQUIRE_TRUE(reader.open((root / L"user_learning.tsv").wstring()));
        REQUIRE_TRUE(reader.frequency(L"nihaoshijie", L"你好世界") == 0);
        REQUIRE_TRUE(reader.words_for(L"liangjiacheng").size() == 1);
        localpinyin::PinyinEngine reopened(localpinyin::DictionaryLoadMode::Empty);
        REQUIRE_TRUE(reopened.load_dictionary_resource_directory(resources.wstring(), user));
        REQUIRE_EQ(rank_of(reopened.lookup(L"liangjiacheng"), L"量甲成"), 1);
        const auto longer = reopened.lookup(L"liangjiachenggongsi");
        REQUIRE_TRUE(std::any_of(longer.begin(), longer.end(), [](const auto& c) {
            return c.text.rfind(L"量甲成", 0) == 0;
        }));
        REQUIRE_TRUE(reader.clear());
    }
    // A committed first choice teaches nothing; a correction is remembered
    // with the character before it. After other text, a single character
    // chosen elsewhere does not override what that text implies.
    {
        localpinyin::PinyinEngine engine(localpinyin::DictionaryLoadMode::Empty);
        REQUIRE_TRUE(engine.load_dictionary_resource_directory(resources.wstring(), user));
        const auto plain_first = engine.lookup(L"ta").front().text;
        const auto after_he_said = engine.lookup(L"ta", L"他说").front().text;
        engine.learn(L"ta", plain_first, true, L"");
        localpinyin::UserLearning reader;
        REQUIRE_TRUE(reader.open((root / L"user_learning.tsv").wstring()));
        REQUIRE_TRUE(reader.entries().empty());

        const std::wstring chosen = plain_first == L"她" ? L"它" : L"她";
        engine.learn(L"ta", chosen, false, L"我对");
        REQUIRE_EQ(engine.lookup(L"ta", L"我对").front().text, chosen);
        REQUIRE_EQ(engine.lookup(L"ta").front().text, chosen);
        REQUIRE_EQ(engine.lookup(L"ta", L"他说").front().text, after_he_said);
        // The settings list shows the choice once, without its context.
        REQUIRE_TRUE(reader.open((root / L"user_learning.tsv").wstring()));
        REQUIRE_EQ(reader.entries().size(), static_cast<size_t>(1));
        REQUIRE_EQ(reader.frequency(localpinyin::UserLearning::contextual_key(L"ta", L'对'), chosen), 1);
        // Forgetting the choice also forgets it in context.
        REQUIRE_EQ(reader.forget(chosen, L"ta"), static_cast<size_t>(2));
        REQUIRE_TRUE(reader.clear());
    }
    std::filesystem::remove_all(root);
    return passed ? 0 : 1;
}
