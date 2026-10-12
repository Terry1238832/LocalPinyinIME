#include "test_common.h"
#include "../src/engine/pinyin_engine.h"
#include "../src/common/utf_utils.h"

#include <algorithm>
#include <fstream>

int main() {
    const auto user_lexicon_root = use_temp_user_lexicon_override();

    localpinyin::PinyinEngine engine;
    REQUIRE_EQ(engine.lookup(L"nihao").front().text, std::wstring(L"\u4F60\u597D"));
    REQUIRE_EQ(engine.lookup(L"henbang").front().text, std::wstring(L"\u5F88\u68D2"));
    auto nihao_world = engine.lookup(L"nihaoshijie");
    REQUIRE_EQ(nihao_world.front().text, std::wstring(L"\u4F60\u597D\u4E16\u754C"));
    REQUIRE_EQ(engine.lookup(L"zhongguo").front().text, std::wstring(L"\u4E2D\u56FD"));
    REQUIRE_EQ(engine.lookup(L"beijing").front().text, std::wstring(L"\u5317\u4EAC"));
    REQUIRE_EQ(engine.lookup(L"woxiangqubeijing").front().text, std::wstring(L"\u6211\u60F3\u53BB\u5317\u4EAC"));
    REQUIRE_EQ(engine.lookup(L"jintianxingtianqihenhao").front().text, std::wstring(L"\u4ECA\u5929\u661F\u671F\u5929\u5F88\u597D"));
    REQUIRE_EQ(engine.lookup(L"xuesheng").front().text, std::wstring(L"\u5B66\u751F"));
    REQUIRE_EQ(engine.lookup(L"wo").front().text, std::wstring(L"\u6211"));

    auto unknown = engine.lookup(L"zzzzunknown");
    REQUIRE_TRUE(!unknown.empty());
    REQUIRE_EQ(unknown.front().text, std::wstring(L"zzzzunknown"));

    REQUIRE_EQ(engine.lookup(L"ni'hao").front().text, std::wstring(L"\u4F60\u597D"));

    auto deduped = engine.lookup(L"nihaoshijie");
    int nihao_world_count = 0;
    for (const auto& candidate : deduped) {
        if (candidate.text == L"\u4F60\u597D\u4E16\u754C") {
            ++nihao_world_count;
        }
    }
    REQUIRE_EQ(nihao_world_count, 1);

    // A small controlled dictionary catches behavior independently of shipped
    // frequencies and verifies that learning survives a new engine instance.
    const auto fixture = user_lexicon_root / L"smart_fixture.tsv";
    {
        std::ofstream file(fixture, std::ios::binary);
        file << localpinyin::wide_to_utf8(
            L"ni\t\u4F60\t9000\n"
            L"hao\t\u597D\t9000\n"
            L"nihao\t\u4F60\u597D\t9000\n"
            L"nihao\t\u62DF\u597D\t8900\n"
            L"zhongguo\t\u4E2D\u56FD\t9000\n"
            L"shijie\t\u4E16\u754C\t8500\n"
            L"shijie\t\u89C6\u754C\t8400\n"
            L"changan\t\u957F\u5B89\t8000\n"
            L"xian\t\u5148\t7900\n"
            L"xian\t\u897F\u5B89\t7800\n");
    }
    const auto contains = [](const auto& candidates, const std::wstring& text) {
        return std::any_of(candidates.begin(), candidates.end(), [&](const auto& candidate) { return candidate.text == text; });
    };
    localpinyin::PinyinEngine smart(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(smart.load_dictionary(fixture.wstring()));
    REQUIRE_EQ(smart.lookup(L"NH").front().text, std::wstring(L"\u4F60\u597D"));
    REQUIRE_EQ(smart.lookup(L"zg").front().text, std::wstring(L"\u4E2D\u56FD"));
    REQUIRE_EQ(smart.lookup(L"ca").front().text, std::wstring(L"\u957F\u5B89"));
    REQUIRE_EQ(smart.lookup(L"xa").front().text, std::wstring(L"\u897F\u5B89"));
    REQUIRE_EQ(smart.lookup(L"niha").front().text, std::wstring(L"\u4F60\u597D"));
    REQUIRE_EQ(smart.lookup(L"nihaoo").front().text, std::wstring(L"\u4F60\u597D"));
    REQUIRE_EQ(smart.lookup(L"nihoa").front().text, std::wstring(L"\u4F60\u597D"));
    REQUIRE_EQ(smart.lookup(L"nihao").front().text, std::wstring(L"\u4F60\u597D"));
    const auto sentence = smart.lookup(L"nihaoshijie");
    REQUIRE_EQ(sentence.front().text, std::wstring(L"\u4F60\u597D\u4E16\u754C"));
    REQUIRE_TRUE(contains(sentence, L"\u4F60\u597D\u89C6\u754C"));
    REQUIRE_TRUE(sentence.size() <= localpinyin::kMaxCandidateCount);
    const std::wstring unknown_long(256, L'z');
    REQUIRE_EQ(smart.lookup(unknown_long).front().text, unknown_long);
    smart.learn(L"nihao", L"\u62DF\u597D");
    REQUIRE_EQ(smart.lookup(L"nihao").front().text, std::wstring(L"\u62DF\u597D"));
    localpinyin::PinyinEngine reopened(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(reopened.load_dictionary(fixture.wstring()));
    REQUIRE_EQ(reopened.lookup(L"nihao").front().text, std::wstring(L"\u62DF\u597D"));
    localpinyin::UserLearning cleaner;
    REQUIRE_TRUE(cleaner.open((user_lexicon_root / L"user_learning.tsv").wstring()));
    REQUIRE_TRUE(cleaner.clear());
    REQUIRE_EQ(smart.lookup(L"nihao").front().text, std::wstring(L"\u4F60\u597D"));
    std::filesystem::remove_all(user_lexicon_root);
    return 0;
}
