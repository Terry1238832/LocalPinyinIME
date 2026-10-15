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

    // Independently bias sentence composition toward other homophones. Both
    // complete dictionary alternatives must remain selectable, without raising
    // their frequencies or replacing the contextual first choice.
    const auto plural_fixture = user_lexicon_root / L"plural_retention.tsv";
    const auto plural_context = user_lexicon_root / L"plural_context.tsv";
    {
        std::ofstream file(plural_fixture, std::ios::binary);
        file << localpinyin::wide_to_utf8(
            L"wo\t我\t10000\nxiang\t想\t10000\n"
            L"xie\t谢\t10000\nxie\t鞋\t10000\nxie\t协\t10000\nxie\t写\t9000\n"
            L"shi\t是\t10000\nshi\t市\t10000\nshi\t诗\t9000\nshi\t事\t9000\n"
            L"le\t乐\t10000\nle\t了\t9000\n"
            L"woxiangxieshile\t我想写诗了\t6000\n"
            L"woxiangxieshile\t我想写事了\t6000\n");
        std::ofstream context_file(plural_context, std::ios::binary);
        for (const auto& sequence : {L"我想", L"想鞋", L"想谢", L"想协", L"鞋市", L"谢市", L"协市",
                                     L"鞋是", L"谢是", L"协是", L"市乐", L"是乐", L"市了", L"是了"})
            context_file << localpinyin::wide_to_utf8(std::wstring(sequence) + L"\t10000\n");
    }
    localpinyin::PinyinEngine plural(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(plural.load_dictionary(plural_fixture.wstring()));
    REQUIRE_TRUE(plural.load_context_model(plural_context.wstring()));
    const auto plural_choices = plural.lookup(L"woxiangxieshile");
    REQUIRE_EQ(plural_choices.size(), localpinyin::kMaxCandidateCount);
    REQUIRE_EQ(plural_choices.front().text, std::wstring(L"我想谢是乐"));
    for (const auto& word : {L"我想写诗了", L"我想写事了"}) {
        const auto choice = std::find_if(plural_choices.begin(), plural_choices.end(), [&](const auto& item) {
            return item.text == word;
        });
        REQUIRE_TRUE(choice != plural_choices.end());
        REQUIRE_EQ(choice->consumed_pinyin, size_t{0});
        REQUIRE_EQ(std::count_if(plural_choices.begin(), plural_choices.end(), [&](const auto& item) {
            return item.text == word;
        }), 1);
    }
    REQUIRE_EQ(plural_choices.back().text, std::wstring(L"我"));
    REQUIRE_EQ(plural_choices.back().consumed_pinyin, size_t{2});

    // High-frequency spelling keys may coincide with pinyin. These synthetic
    // names remain directly selectable, but must not form Chinese sentences
    // unless the user explicitly selected or added that token.
    const auto mixed_fixture = user_lexicon_root / L"mixed_fixture.tsv";
    {
        std::ofstream file(mixed_fixture, std::ios::binary);
        file << localpinyin::wide_to_utf8(
            L"yu\t雨\t9000\n"
            L"yu\tYu\t20000\n"
            L"xue\t雪\t9000\n"
            L"xue\tXue\t20000\n"
            L"yutian\t雨天\t9000\n"
            L"yutian\tYu天\t20000\n"
            L"mingcheng\tVeryLongLatinCompanyName\t5500\n");
    }
    localpinyin::PinyinEngine mixed(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(mixed.load_dictionary(mixed_fixture.wstring()));
    REQUIRE_TRUE(contains(mixed.lookup(L"yu"), L"Yu"));
    REQUIRE_TRUE(contains(mixed.lookup(L"yu", L"今天"), L"Yu"));
    REQUIRE_TRUE(contains(mixed.lookup(L"yutian"), L"Yu天"));
    REQUIRE_TRUE(contains(mixed.lookup(L"mingcheng"), L"VeryLongLatinCompanyName"));
    for (const auto& key : {L"yuxue", L"yutianxue", L"yux"}) {
        const auto choices = mixed.lookup(key);
        REQUIRE_TRUE(!choices.empty());
        for (const auto& choice : choices) {
            if (choice.consumed_pinyin != 0) continue;
            REQUIRE_TRUE(std::none_of(choice.text.begin(), choice.text.end(), [](wchar_t ch) {
                return (ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z');
            }));
        }
    }
    REQUIRE_TRUE(contains(mixed.lookup(L"yuxue"), L"雨雪"));
    REQUIRE_TRUE(contains(mixed.lookup(L"yutianxue"), L"雨天雪"));
    mixed.learn(L"yu", L"Yu");
    REQUIRE_TRUE(contains(mixed.lookup(L"yuxue"), L"Yu雪"));
    REQUIRE_TRUE(cleaner.clear());

    const auto personal_resources = user_lexicon_root / L"mixed_resources";
    std::filesystem::create_directories(personal_resources / L"dictionary");
    std::filesystem::copy_file(mixed_fixture, personal_resources / L"dictionary/core_zh_pinyin.tsv");
    const auto personal_file = user_lexicon_root / L"mixed_personal.tsv";
    {
        std::ofstream file(personal_file, std::ios::binary);
        file << localpinyin::wide_to_utf8(L"yu\tYu\t20000\n");
    }
    localpinyin::PinyinEngine personal_mixed(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(personal_mixed.load_dictionary_resource_directory(personal_resources.wstring(), personal_file.wstring()));
    REQUIRE_TRUE(contains(personal_mixed.lookup(L"yuxue"), L"Yu雪"));

    std::filesystem::remove_all(user_lexicon_root);
    return 0;
}
