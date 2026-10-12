#include "test_common.h"
#include "../src/engine/pinyin_engine.h"
#include "../src/common/utf_utils.h"

#include <fstream>

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

    // Fixed homophones keep learning coverage independent of improvements to
    // the shipped dictionary and keep personal selections inside this fixture.
    const auto fixture = user_lexicon_root / L"homophones.tsv";
    {
        std::ofstream file(fixture, std::ios::binary);
        file << localpinyin::wide_to_utf8(L"shishi\t实施\t1000\nshishi\t实时\t900\n");
    }
    localpinyin::PinyinEngine engine(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(engine.load_dictionary(fixture.wstring()));
    const std::wstring target = L"实时";

    auto before = engine.lookup(L"shishi");
    REQUIRE_TRUE(before.size() >= 2);
    REQUIRE_EQ(before.front().text, std::wstring(L"实施"));

    engine.learn(L"shishi", target);
    auto after = engine.lookup(L"shishi");
    REQUIRE_EQ(after.front().text, target);
    localpinyin::PinyinEngine reopened(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(reopened.load_dictionary(fixture.wstring()));
    REQUIRE_EQ(reopened.lookup(L"shishi").front().text, target);
    std::filesystem::remove_all(user_lexicon_root);
    return 0;
}
