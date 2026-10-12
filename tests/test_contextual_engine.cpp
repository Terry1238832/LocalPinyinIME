#include "test_common.h"
#include "../src/engine/pinyin_engine.h"
#include "../src/common/utf_utils.h"

#include <algorithm>
#include <fstream>

namespace {

void write_text(const std::filesystem::path& path, const std::wstring& text) {
    std::ofstream file(path, std::ios::binary);
    file << localpinyin::wide_to_utf8(text);
}

bool contains(const std::vector<localpinyin::Candidate>& candidates, const std::wstring& text) {
    return std::any_of(candidates.begin(), candidates.end(), [&](const auto& candidate) { return candidate.text == text; });
}

}  // namespace

int main() {
    const auto root = use_temp_user_lexicon_override();
    const auto dictionary = root / L"words.tsv";
    const auto context = root / L"context.tsv";
    // There are deliberately no sentence entries and no 写诗 phrase entry.
    // 写 is seventh in frequency order, beyond the old top-three decoder.
    write_text(dictionary,
        L"wo\t我\t9000\nni\t你\t9000\nni\t泥\t8900\n"
        L"xiang\t想\t9000\nqu\t去\t9000\n"
        L"xie\t谢\t9000\nxie\t鞋\t8500\nxie\t些\t8300\n"
        L"xie\t协\t8100\nxie\t邪\t7900\nxie\t斜\t7800\nxie\t写\t7000\n"
        L"shi\t是\t9000\nshi\t事\t8800\nshi\t时\t8500\nshi\t诗\t8400\n"
        L"pao\t跑\t9000\nde\t的\t9000\nde\t得\t8900\nde\t地\t8800\n"
        L"hen\t很\t9000\nkuai\t快\t9000\n"
        L"beijing\t北京\t9000\nxian\t先\t9000\nxian\t西安\t8900\n"
        L"xi\t西\t7000\nan\t安\t7000\n"
        L"jin\t今\t9000\nnian\t年\t9000\nan\t岸\t9000\n"
        L"yi\t一\t9000\nyue\t月\t9000\nyu\t雨\t9000\ne\t饿\t9000\n"
        L"yong\t用\t9000\nshouji\t手机\t9000\nshoujian\t收件\t9000\n"
        L"xuexi\t学习\t9000\nxuexiao\t学校\t9000\nzhidao\t知道\t9000\n");
    write_text(context,
        L"# Count data, not an input-to-sentence mapping.\n"
        L"^我\t50\n^你\t50\n我想\t50\n你想\t50\n想写\t50\n写诗\t50\n"
        L"想写诗\t50\n我跑\t50\n跑得\t50\n得很\t50\n很快\t50\n"
        L"跑得很\t50\n得很快\t50\n想去\t50\n去北\t50\n北京\t50\n"
        L"去北京\t50\n诗$\t50\n快$\t50\n京$\t50\n"
        L"你岸\t1\n雨饿\t1\n收件\t1000\n学校\t1000\n"
        L"我跑得很\t50\n"
        L"bad\tinvalid\n坏\t-5\n超长非法记录\t42\n");
    localpinyin::PinyinEngine engine(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(engine.load_dictionary(dictionary.wstring()));
    REQUIRE_TRUE(engine.load_context_model(context.wstring()));
    REQUIRE_EQ(engine.lookup(L"nixiangxieshi").front().text, std::wstring(L"你想写诗"));
    REQUIRE_EQ(engine.lookup(L"woxi angxieshi").front().text, std::wstring(L"我想写诗"));
    REQUIRE_EQ(engine.lookup(L"wopaodehenkuai").front().text, std::wstring(L"我跑得很快"));
    REQUIRE_EQ(engine.lookup(L"dehenkuai", L"我跑").front().text, std::wstring(L"得很快"));
    REQUIRE_EQ(engine.lookup(L"woxiangqubeij").front().text, std::wstring(L"我想去北京"));
    REQUIRE_EQ(engine.lookup(L"woxiangqubeijing").front().text, std::wstring(L"我想去北京"));
    REQUIRE_EQ(engine.lookup(L"jinnianyiyue").front().text, std::wstring(L"今年一月"));
    REQUIRE_EQ(engine.lookup(L"nixiangyongshouji").front().text, std::wstring(L"你想用手机"));
    REQUIRE_TRUE(!contains(engine.lookup(L"nixiangyongshouji"), L"你想用收件"));
    REQUIRE_EQ(engine.lookup(L"nixiangxuexi").front().text, std::wstring(L"你想学习"));
    REQUIRE_TRUE(!contains(engine.lookup(L"nixiangxuexi"), L"你想学校"));
    localpinyin::Dictionary source(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(source.load_from_tsv_file(dictionary.wstring()));
    localpinyin::ContextModel model;
    model.reset(source);
    REQUIRE_TRUE(model.load(context.wstring()));
    // The same character sequence has the same ngram score with either word
    // segmentation, including 4grams crossing the different boundaries.
    REQUIRE_EQ(model.extend_score(L"", L"我跑得") + model.extend_score(L"我跑得", L"很快"),
               model.extend_score(L"", L"我") + model.extend_score(L"我", L"跑") + model.extend_score(L"我跑", L"得很快"));
    const auto quad_path = root / L"quad.tsv";
    write_text(quad_path, L"你想写诗\t200\n");
    localpinyin::PinyinEngine quad(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(quad.load_dictionary(dictionary.wstring()));
    REQUIRE_TRUE(quad.load_context_model(quad_path.wstring()));
    REQUIRE_EQ(quad.lookup(L"wozhidaonixiangxieshi").front().text, std::wstring(L"我知道你想写诗"));
    REQUIRE_EQ(engine.lookup(L"xi'an").front().text, std::wstring(L"西安"));
    REQUIRE_EQ(engine.lookup(L"xian").front().text, std::wstring(L"先"));
    const auto incomplete = engine.lookup(L"xi'an'qqqqqq");
    REQUIRE_EQ(incomplete.front().text, std::wstring(L"西安"));
    REQUIRE_EQ(incomplete.front().consumed_pinyin, static_cast<size_t>(6));
    const auto full = engine.lookup(L"nixiangxieshi");
    REQUIRE_TRUE(std::any_of(full.begin(), full.end(), [](const auto& candidate) {
        return candidate.text == L"你" && candidate.consumed_pinyin == 2;
    }));
    std::wstring long_input;
    std::wstring long_expected;
    for (int index = 0; index < 24; ++index) { long_input += L"nixiang"; long_expected += L"你想"; }
    REQUIRE_TRUE(long_input.size() > 128);
    REQUIRE_EQ(engine.lookup(long_input).front().text, long_expected);

    // A selected phrase teaches its character transitions to another sentence,
    // including after restart; the phrase itself is absent from the dictionary.
    localpinyin::PinyinEngine learned(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(learned.load_dictionary(dictionary.wstring()));
    learned.learn(L"xieshi", L"写诗");
    REQUIRE_EQ(learned.lookup(L"nixiangxieshi").front().text, std::wstring(L"你想写诗"));
    localpinyin::PinyinEngine reopened(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(reopened.load_dictionary(dictionary.wstring()));
    REQUIRE_EQ(reopened.lookup(L"nixiangxieshi").front().text, std::wstring(L"你想写诗"));
    REQUIRE_TRUE(contains(reopened.lookup(L"nixiangxieshi"), L"你想写诗"));
    localpinyin::UserLearning clearer;
    REQUIRE_TRUE(clearer.open((root / L"user_learning.tsv").wstring()));
    REQUIRE_TRUE(clearer.clear());
    REQUIRE_TRUE(reopened.lookup(L"nixiangxieshi").front().text != L"你想写诗");

    // A frequent saved expression can compete with several shorter words,
    // including inside an unseen sentence. Equal ngram evidence makes this a
    // lexical-preference check rather than a saved input-to-sentence answer.
    const auto expression_dictionary = root / L"expressions.tsv";
    const auto expression_context = root / L"expression_context.tsv";
    write_text(expression_dictionary,
        L"qiyong\t启用\t9000\nbendi\t本地\t9000\nshurufa\t输入罚\t9000\n"
        L"bendishurufa\t本地输入法\t9000\n");
    std::wstring equal_context;
    for (const std::wstring& text : {std::wstring(L"^启用本地输入法$"), std::wstring(L"^启用本地输入罚$")}) {
        for (size_t offset = 0; offset < text.size(); ++offset) {
            for (size_t length = 1; length <= 4 && offset + length <= text.size(); ++length) {
                equal_context += text.substr(offset, length) + L"\t1000\n";
            }
        }
    }
    write_text(expression_context, equal_context);
    localpinyin::PinyinEngine expressions(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(expressions.load_dictionary(expression_dictionary.wstring()));
    REQUIRE_TRUE(expressions.load_context_model(expression_context.wstring()));
    REQUIRE_EQ(expressions.lookup(L"bendishurufa").front().text, std::wstring(L"本地输入法"));
    REQUIRE_EQ(expressions.lookup(L"qiyongbendishurufa").front().text, std::wstring(L"启用本地输入法"));
    REQUIRE_TRUE(contains(expressions.lookup(L"qiyongbendishurufa"), L"启用本地输入罚"));

    // One rare name observation must not defeat a much more frequent word in
    // a new sentence. No sentence mapping or expected-answer learning is used.
    const auto sparse_dictionary = root / L"sparse_words.tsv";
    const auto sparse_context = root / L"sparse_context.tsv";
    write_text(sparse_dictionary,
        L"xuyao\t需要\t9000\nzhaoming\t照明\t9000\nzhaoming\t赵明\t5000\n");
    write_text(sparse_context, L"赵明\t1\n要赵明\t1\n需要赵明\t1\n");
    localpinyin::PinyinEngine sparse(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(sparse.load_dictionary(sparse_dictionary.wstring()));
    REQUIRE_TRUE(sparse.load_context_model(sparse_context.wstring()));
    REQUIRE_EQ(sparse.lookup(L"xuyaozhaoming").front().text, std::wstring(L"需要照明"));

    std::filesystem::remove_all(root);
    return 0;
}
