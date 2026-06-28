#include "test_common.h"
#include "../src/engine/pinyin_engine.h"

int main() {
    localpinyin::PinyinEngine engine;
    REQUIRE_EQ(engine.lookup(L"nihao").front().text, std::wstring(L"\u4F60\u597D"));
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
    return 0;
}
