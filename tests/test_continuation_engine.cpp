#include "test_common.h"
#include "../src/engine/pinyin_engine.h"
#include "../src/common/utf_utils.h"

#include <algorithm>
#include <fstream>
#include <set>

namespace {
void write_text(const std::filesystem::path& path, const std::wstring& text) {
    std::ofstream file(path, std::ios::binary);
    file << localpinyin::wide_to_utf8(text);
}
bool contains(const std::vector<localpinyin::Candidate>& candidates, const std::wstring& text) {
    return std::any_of(candidates.begin(), candidates.end(), [&](const auto& candidate) { return candidate.text == text; });
}
}

int main() {
    using namespace localpinyin;
    const auto root = use_temp_user_lexicon_override();
    const auto dictionary = root / L"words.tsv";
    const auto rules = root / L"continuations_zh.tsv";
    write_text(dictionary, L"qingshikeji\t青石科技\t6000\nqingshigongsi\t青石公司\t5800\nwo\t我\t9000\n");
    write_text(rules,
        L"# independent test associations, not production ranking answers\n"
        L"我准备\t出发\t9500\n我准备\t休息\t7500\n准备\t吃饭\t8000\n"
        L"我准备\t出发\t3000\n我准备\t出发\t9600\n"
        L"我准备\t负值\t-1\n我准备\t巨值\t10001\n"
        L"我准备\t多列\t6000\tX\n我准备\t非数字\tabc\n"
        L"我\t太短\t7000\n我准备\tASCII\t7000\n");
    PinyinEngine engine(DictionaryLoadMode::Empty);
    REQUIRE_TRUE(engine.load_dictionary(dictionary.wstring()));
    REQUIRE_EQ(engine.predict(L"请问我准备").front().text, std::wstring(L"出发"));
    REQUIRE_TRUE(contains(engine.predict(L"请问我准备"), L"休息"));
    REQUIRE_TRUE(contains(engine.predict(L"我准备"), L"吃饭"));
    REQUIRE_TRUE(!contains(engine.predict(L"我准备"), L"负值"));
    REQUIRE_TRUE(contains(engine.predict(L"青石"), L"科技"));
    REQUIRE_TRUE(contains(engine.predict(L"青石"), L"公司"));
    REQUIRE_EQ(engine.predict(L"我准备", 1).size(), size_t{1});
    REQUIRE_TRUE(engine.predict(L"我准备", 0).empty());
    REQUIRE_TRUE(engine.predict(L"").empty());
    REQUIRE_TRUE(engine.predict(L"我").empty());
    REQUIRE_TRUE(engine.predict(L"我准备。").empty());
    REQUIRE_TRUE(engine.predict(L"我准备\n").empty());
    REQUIRE_TRUE(engine.predict(L"我准备 123").empty());
    REQUIRE_EQ(engine.predict(L"ASCII🙂我准备").front().text, std::wstring(L"出发"));
    REQUIRE_EQ(engine.predict(std::wstring(10000, L'我') + L"我准备").front().text, std::wstring(L"出发"));
    REQUIRE_EQ(ContinuationModel::context_suffix(L"前句。新的句子"), std::wstring(L"新的句子"));
    REQUIRE_EQ(ContinuationModel::context_suffix(std::wstring(100, L'我')).size(), size_t{32});

    // The already-existing selected-phrase storage also supplies completions;
    // prediction itself must neither learn nor persist the surrounding text.
    engine.learn(L"qingshigongzuoshi", L"青石工作室");
    REQUIRE_EQ(engine.predict(L"青石").front().text, std::wstring(L"工作室"));
    const auto learning = root / L"user_learning.tsv";
    std::ifstream before_file(learning, std::ios::binary);
    const std::string before((std::istreambuf_iterator<char>(before_file)), {});
    before_file.close();
    for (int i = 0; i < 20; ++i) (void)engine.predict(L"文档隐私青石");
    std::ifstream after_file(learning, std::ios::binary);
    const std::string after((std::istreambuf_iterator<char>(after_file)), {});
    after_file.close();
    REQUIRE_EQ(after, before);
    PinyinEngine reopened(DictionaryLoadMode::Empty);
    REQUIRE_TRUE(reopened.load_dictionary(dictionary.wstring()));
    REQUIRE_EQ(reopened.predict(L"青石").front().text, std::wstring(L"工作室"));
    UserLearning clearer;
    REQUIRE_TRUE(clearer.open(learning.wstring()));
    REQUIRE_TRUE(clearer.clear());
    REQUIRE_TRUE(!contains(reopened.predict(L"青石"), L"工作室"));

    // Keep at most nine distinct suggestions and never include the old prefix.
    write_text(rules, L"我准备\t一\t6000\n我准备\t二\t6000\n我准备\t三\t6000\n我准备\t四\t6000\n我准备\t五\t6000\n我准备\t六\t6000\n我准备\t七\t6000\n我准备\t八\t6000\n我准备\t九\t6000\n我准备\t十\t6000\n");
    REQUIRE_TRUE(engine.load_continuation_model(rules.wstring()));
    const auto limited = engine.predict(L"我准备", 1000);
    REQUIRE_EQ(limited.size(), kMaxCandidateCount);
    std::set<std::wstring> unique;
    for (const auto& item : limited) {
        REQUIRE_TRUE(unique.insert(item.text).second);
        REQUIRE_EQ(item.consumed_pinyin, size_t{0});
        REQUIRE_TRUE(item.text.find(L"我准备") == std::wstring::npos);
    }
    REQUIRE_TRUE(!engine.load_continuation_model((root / L"missing.tsv").wstring()));
    REQUIRE_TRUE(engine.predict(L"我准备").empty());
    REQUIRE_TRUE(contains(engine.predict(L"青石"), L"科技"));
    // A deliberately lower-scored six-character dictionary expression must
    // remain selectable even when many competing segmented homophones win.
    const auto long_words = root / L"complete-words.tsv";
    write_text(long_words,
        L"qing\t清\t20000\nqing\t轻\t19000\nqing\t情\t18000\nqing\t卿\t17000\nqing\t青\t100\n"
        L"shi\t时\t20000\nshi\t市\t19000\nshi\t事\t18000\nshi\t式\t17000\nshi\t石\t100\n"
        L"keji\t科技\t6000\nfuwu\t服务\t6000\nqingshikejifuwu\t青石科技服务\t5000\n");
    PinyinEngine long_engine(DictionaryLoadMode::Empty);
    REQUIRE_TRUE(long_engine.load_dictionary(long_words.wstring()));
    const auto choices = long_engine.lookup(L"qingshikejifuwu");
    REQUIRE_TRUE(contains(choices, L"青石科技服务"));
    REQUIRE_TRUE(choices.front().text != L"青石科技服务");
    REQUIRE_TRUE(choices.size() <= kMaxCandidateCount);
    REQUIRE_TRUE(std::any_of(choices.begin(), choices.end(), [](const auto& choice) { return choice.consumed_pinyin > 0; }));
    REQUIRE_TRUE(contains(long_engine.lookup(L"qingshikejifuwu", L"请联系"), L"青石科技服务"));
    return 0;
}
