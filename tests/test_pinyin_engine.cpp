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
    // 星期天 is xingqitian. The previous xingtianqi spelling swapped
    // syllables and passed only because a whole-sentence patch hid the error.
    REQUIRE_EQ(engine.lookup(L"jintianxingqitianhenhao").front().text, std::wstring(L"\u4ECA\u5929\u661F\u671F\u5929\u5F88\u597D"));
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
    // A synthetic nine-word dictionary exercises complete-word retention
    // independently of shipped vocabulary or frozen sentence answers. A word
    // already ranked ninth must survive reserving the shorter-prefix slot.
    const auto retention_fixture = user_lexicon_root / L"retention_fixture.tsv";
    const std::vector<std::wstring> retention_words = {L"这不是文子", L"这不是蚊子", L"这不是文资",
        L"这不是文姿", L"这不是文滋", L"这不是文籽", L"这不是文梓", L"这不是文紫", L"这不是文字"};
    {
        std::ofstream file(retention_fixture, std::ios::binary);
        file << localpinyin::wide_to_utf8(L"zhe\t这\t9000\n");
        for (size_t i = 0; i < retention_words.size(); ++i)
            file << localpinyin::wide_to_utf8(L"zhebushiwenzi\t" + retention_words[i] + L"\t" +
                std::to_wstring(6000 - static_cast<int>(i) * 125) + L"\n");
    }
    const auto no_prefix_fixture = user_lexicon_root / L"retention_no_prefix.tsv";
    {
        std::ofstream file(no_prefix_fixture, std::ios::binary);
        for (size_t i = 0; i < retention_words.size(); ++i)
            file << localpinyin::wide_to_utf8(L"zhebushiwenzi\t" + retention_words[i] + L"\t" +
                std::to_wstring(6000 - static_cast<int>(i) * 125) + L"\n");
    }
    localpinyin::PinyinEngine no_prefix(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(no_prefix.load_dictionary(no_prefix_fixture.wstring()));
    const auto original_nine = no_prefix.lookup(L"zhebushiwenzi");
    REQUIRE_EQ(original_nine.size(), localpinyin::kMaxCandidateCount);
    REQUIRE_EQ(original_nine.back().text, retention_words.back());

    localpinyin::PinyinEngine retention(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(retention.load_dictionary(retention_fixture.wstring()));
    const auto retained = retention.lookup(L"zhebushiwenzi");
    REQUIRE_EQ(retained.size(), localpinyin::kMaxCandidateCount);
    REQUIRE_EQ(retained.front().text, retention_words.front());
    REQUIRE_EQ(retained[7].text, retention_words.back());
    REQUIRE_EQ(retained[8].text, std::wstring(L"这"));
    REQUIRE_EQ(retained[8].consumed_pinyin, size_t{3});
    for (size_t i = 0; i < 7; ++i) REQUIRE_EQ(retained[i].text, retention_words[i]);
    REQUIRE_EQ(std::count_if(retained.begin(), retained.end(), [&](const auto& word) {
        return word.text == retention_words.back();
    }), 1);

    std::filesystem::remove_all(user_lexicon_root);
    return 0;
}
