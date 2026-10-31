#include "test_common.h"
#include "../src/engine/phrase_counts.h"
#include "../src/engine/pinyin_engine.h"

#include <array>
#include <string>

namespace {

std::filesystem::path executable_directory() {
    std::array<wchar_t, MAX_PATH> path{};
    const DWORD size = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    return size == 0 || size >= path.size() ? std::filesystem::current_path()
                                            : std::filesystem::path(path.data()).parent_path();
}

}  // namespace

// The packaged chat phrase counts order a short phrase typed on its own: the
// reading people send moves ahead of the lexicon's, but not after other text,
// not for a single character, and not past a deliberately frequent word.
int main() {
    const auto root = use_temp_user_lexicon_override();
    const auto resources = executable_directory();

    localpinyin::PhraseCounts counts;
    REQUIRE_TRUE(counts.load((resources / L"dictionary" / L"phrase_zh.lm").wstring()));
    REQUIRE_TRUE(counts.level(L"你猜") > counts.level(L"你才"));
    REQUIRE_EQ(counts.level(L"囧囧囧囧囧囧"), 0);

    localpinyin::PhraseCounts missing;
    REQUIRE_TRUE(!missing.load((root / L"absent_phrase_zh.lm").wstring()));
    REQUIRE_EQ(missing.level(L"你猜"), 0);

    localpinyin::PinyinEngine engine(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(engine.load_dictionary_resource_directory(resources.wstring(), (root / L"user_lexicon.tsv").wstring()));
    REQUIRE_EQ(engine.lookup(L"nicai", L"").front().text, std::wstring(L"你猜"));
    REQUIRE_EQ(engine.lookup(L"meiguo", L"").front().text, std::wstring(L"美国"));
    REQUIRE_EQ(engine.lookup(L"zhifu", L"").front().text, std::wstring(L"支付"));
    REQUIRE_EQ(engine.lookup(L"ta", L"").front().text, std::wstring(L"他"));

    // Sentences typed as initials read like any other, and a syllable being
    // typed is not mistaken for initials.
    REQUIRE_EQ(engine.lookup(L"wzdl", L"").front().text, std::wstring(L"我知道了"));
    REQUIRE_EQ(engine.lookup(L"wxhn", L"").front().text, std::wstring(L"我喜欢你"));
    REQUIRE_EQ(engine.lookup(L"wok", L"").front().text, std::wstring(L"我看"));
    REQUIRE_EQ(engine.lookup(L"zh", L"").front().text, std::wstring(L"这"));
    REQUIRE_EQ(engine.lookup(L"ppt", L"").front().text, std::wstring(L"PPT"));

    std::filesystem::remove_all(root);
    return 0;
}
