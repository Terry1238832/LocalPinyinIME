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

struct ContextCase {
    const wchar_t* preceding;
    const wchar_t* pinyin;
    const wchar_t* expected;
};

}  // namespace

// The packaged character model ranks the candidates of newly typed pinyin by
// the text already before the caret. Every case below is ambiguous without
// that text; none of these sentences is part of the model's training data.
int main() {
    const auto root = use_temp_user_lexicon_override();
    const auto resources = executable_directory();

    localpinyin::ContextModel model;
    REQUIRE_TRUE(model.load((resources / L"dictionary" / L"context_zh.lm").wstring()));
    REQUIRE_TRUE(model.entry_count() > 1000000);
    // Kneser-Ney backoff probabilities: a frequent continuation beats a rare one.
    REQUIRE_TRUE(model.log_probability(L"我", L'们') > model.log_probability(L"我", L'门'));
    REQUIRE_TRUE(model.log_probability(L"^", L'请') > model.log_probability(L"^", L'晴'));
    REQUIRE_TRUE(model.log_probability(L"天气很", L'好') > model.log_probability(L"天气很", L'号'));
    // Scores are additive across any segmentation of the same text.
    REQUIRE_EQ(model.extend_score(L"", L"我们") + model.extend_score(L"我们", L"一起去"),
               model.extend_score(L"", L"我") + model.extend_score(L"我", L"们一起") + model.extend_score(L"我们一起", L"去"));

    localpinyin::PinyinEngine engine(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(engine.load_dictionary_resource_directory(resources.wstring(), (root / L"user_lexicon.tsv").wstring()));
    const std::vector<ContextCase> cases = {
        {L"他跑", L"de", L"得"},
        {L"他慢慢", L"de", L"地"},
        {L"这件", L"shi", L"事"},
        {L"小", L"shi", L"时"},
        {L"这起", L"shijian", L"事件"},
        {L"没有", L"shijian", L"时间"},
        {L"一幅", L"hua", L"画"},
        {L"说", L"hua", L"话"},
        {L"闭上", L"yanjing", L"眼睛"},
        {L"天", L"qing", L"晴"},
        {L"我们", L"yiqi", L"一起"},
        {L"今天天气", L"henhao", L"很好"},
    };
    bool passed = true;
    for (const auto& item : cases) {
        const auto candidates = engine.lookup(item.pinyin, item.preceding);
        const auto actual = candidates.empty() ? std::wstring() : candidates.front().text;
        if (actual != item.expected) {
            passed = false;
            std::cout << "context case failed: " << localpinyin::wide_to_utf8(item.preceding) << " + "
                      << localpinyin::wide_to_utf8(item.pinyin) << " -> " << localpinyin::wide_to_utf8(actual)
                      << " (expected " << localpinyin::wide_to_utf8(item.expected) << ")\n";
        }
    }
    // The same pinyin follows its own context in both directions.
    REQUIRE_TRUE(engine.lookup(L"shijian", L"这起").front().text != engine.lookup(L"shijian", L"没有").front().text);
    // Without surrounding text, a standalone word keeps the lexicon order and
    // the clause-start model only breaks the 请/晴 frequency tie.
    REQUIRE_EQ(engine.lookup(L"qing").front().text, std::wstring(L"请"));

    std::filesystem::remove_all(root);
    return passed ? 0 : 1;
}
