#include "test_common.h"
#include "../src/engine/pinyin_engine.h"
#include "../src/engine/word_model.h"
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

struct SentenceCase {
    const wchar_t* pinyin;
    const wchar_t* expected;
};

}  // namespace

// The packaged word association model rewards a word that often follows the
// previous word, beyond the three characters the character model sees.
int main() {
    const auto root = use_temp_user_lexicon_override();
    const auto resources = executable_directory();

    localpinyin::WordModel model;
    REQUIRE_TRUE(model.load((resources / L"dictionary" / L"word_zh.lm").wstring()));
    REQUIRE_TRUE(!model.empty());
    // Stored pairs score their association; the likelier continuation wins.
    REQUIRE_TRUE(model.pair_score(L"专门", L"卖") > model.pair_score(L"专门", L"买"));
    REQUIRE_TRUE(model.pair_score(L"严密", L"监视") > 0);
    REQUIRE_EQ(model.pair_score(L"", L"监视"), 0);
    // The previous word of committed text is its longest suffix that begins a pair.
    REQUIRE_EQ(model.previous_word(L"他们非常严密"), std::wstring(L"严密"));
    REQUIRE_EQ(model.previous_word(L""), std::wstring());

    localpinyin::WordModel missing;
    REQUIRE_TRUE(!missing.load((root / L"absent_word_zh.lm").wstring()));
    REQUIRE_TRUE(missing.empty());
    REQUIRE_EQ(missing.pair_score(L"专门", L"卖"), 0);

    // Held-out sentences (tests/data/heldout_sentences.tsv) that the character
    // model alone read wrongly: 专门买, 眼睛掉在地上, 十点钟水饺, 倒到被子里.
    localpinyin::PinyinEngine engine(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(engine.load_dictionary_resource_directory(resources.wstring(), (root / L"user_lexicon.tsv").wstring()));
    const std::vector<SentenceCase> cases = {
        {L"zhejiadianshizhuanmenmaichufangyongpinde", L"这家店是专门卖厨房用品的"},
        {L"nideyanjingdiaozaidishangle", L"你的眼镜掉在地上了"},
        {L"woshidianzhongshuijiao", L"我十点钟睡觉"},
        {L"tabaniunaidaodaobeizilile", L"他把牛奶倒到杯子里了"},
    };
    bool passed = true;
    for (const auto& item : cases) {
        const auto candidates = engine.lookup(item.pinyin, L"");
        const auto actual = candidates.empty() ? std::wstring() : candidates.front().text;
        if (actual != item.expected) {
            passed = false;
            std::cout << "sentence failed: " << localpinyin::wide_to_utf8(item.pinyin) << " -> "
                      << localpinyin::wide_to_utf8(actual) << " (expected " << localpinyin::wide_to_utf8(item.expected) << ")\n";
        }
    }

    std::filesystem::remove_all(root);
    return passed ? 0 : 1;
}
