#include "test_common.h"
#include "../src/engine/pinyin_engine.h"

int main() {
    const auto user_lexicon_root = use_temp_user_lexicon_override();

    {
        localpinyin::CandidateRanker ranker;
        std::vector<localpinyin::Candidate> candidates = {
            {L"\u4E59", 1000, 0},
            {L"\u7532", 1000, 0},
        };
        auto ranked = ranker.rank(L"yi", candidates);
        REQUIRE_EQ(ranked[0].text, std::wstring(L"\u4E59"));
        REQUIRE_EQ(ranked[1].text, std::wstring(L"\u7532"));
    }

    {
        localpinyin::CandidateRanker ranker;
        for (int i = 0; i < 100; ++i) {
            ranker.increment_frequency(L"yi", L"\u4E59");
        }
        auto ranked = ranker.rank(L"yi", {{L"\u4E59", 1000, 0}});
        REQUIRE_TRUE(!ranked.empty());
        REQUIRE_TRUE(ranked.front().user_frequency <= 6000);
    }

    localpinyin::PinyinEngine engine;
    const std::wstring target = L"\u9996\u90FD\u7ECF\u8D38\u5927\u5B66";

    auto before = engine.lookup(L"shoudujingmaodaxue");
    REQUIRE_TRUE(before.size() >= 2);
    REQUIRE_TRUE(before.front().text != target);

    engine.learn(L"shoudujingmaodaxue", target);
    auto after = engine.lookup(L"shoudujingmaodaxue");
    REQUIRE_EQ(after.front().text, target);
    std::filesystem::remove_all(user_lexicon_root);
    return 0;
}
