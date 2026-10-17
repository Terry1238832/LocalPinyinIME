#include "test_common.h"
#include "../src/engine/pinyin_engine.h"
#include "../src/engine/double_pinyin.h"
#include "../src/engine/fuzzy_pinyin.h"
#include "../src/engine/typo_correction.h"

#include <algorithm>
#include <string>
#include <vector>

// Spelling correction, fuzzy pinyin, abbreviations, Chinese-English mixed
// input and double pinyin, against the shipped dictionary.
int main() {
    using namespace localpinyin;
    const auto user_lexicon_root = use_temp_user_lexicon_override();
    (void)user_lexicon_root;

    const auto contains = [](const std::vector<Candidate>& candidates, const std::wstring& text) {
        return std::any_of(candidates.begin(), candidates.end(), [&](const auto& candidate) { return candidate.text == text; });
    };
    const auto rank_of = [](const std::vector<Candidate>& candidates, const std::wstring& text) {
        const auto found = std::find_if(candidates.begin(), candidates.end(), [&](const auto& candidate) { return candidate.text == text; });
        return found == candidates.end() ? -1 : static_cast<int>(found - candidates.begin());
    };
    PinyinEngine engine;
    EngineInputOptions defaults;
    REQUIRE_TRUE(defaults.typo_correction && defaults.mixed_english && defaults.fuzzy == 0 &&
                 defaults.double_pinyin == DoublePinyinScheme::None);
    engine.set_input_options(defaults);

    // ── Spelling correction: keyboard slips ───────────────────────────────
    // A neighbor key: nihso (s for a) reads as nothing, so 你好 leads.
    auto corrected = engine.lookup(L"nihso");
    REQUIRE_EQ(corrected.front().text, std::wstring(L"你好"));
    REQUIRE_TRUE(corrected.front().correction);
    // Two keys swapped, a key missing or doubled.
    REQUIRE_EQ(engine.lookup(L"xihaun").front().text, std::wstring(L"喜欢"));
    REQUIRE_EQ(engine.lookup(L"zhdiao").front().text, std::wstring(L"知道"));
    REQUIRE_EQ(engine.lookup(L"shneme").front().text, std::wstring(L"什么"));
    // Valid typing that is probably a slip: zhida reads as 直达, but 知道 is
    // far more common one key away. It comes second, never first.
    auto zhida = engine.lookup(L"zhida");
    REQUIRE_TRUE(!zhida.front().correction);
    REQUIRE_EQ(rank_of(zhida, L"知道"), 1);
    REQUIRE_TRUE(zhida[1].correction);
    // A common word as typed is never second-guessed.
    auto tongshi = engine.lookup(L"tongshi");
    REQUIRE_TRUE(std::none_of(tongshi.begin(), tongshi.end(), [](const auto& candidate) { return candidate.correction; }));
    REQUIRE_EQ(engine.lookup(L"nihao").front().text, std::wstring(L"你好"));
    // A slip inside a sentence is read again around where reading stopped.
    REQUIRE_EQ(engine.lookup(L"jintianwanshnagqutushuguan").front().text, std::wstring(L"今天晚上去图书馆"));
    // Sound confusions are fuzzy pinyin, not typos: without z=zh, zongguo
    // never becomes 中国.
    REQUIRE_TRUE(!contains(engine.lookup(L"zongguo"), L"中国"));
    for (const auto& correction : spelling_corrections(L"zongguo", true)) REQUIRE_TRUE(correction.pinyin != L"zhongguo");
    for (const auto& correction : spelling_corrections(L"lihao", true)) REQUIRE_TRUE(correction.pinyin != L"nihao");
    REQUIRE_TRUE(keyboard_neighbors(L'a', L's') && !keyboard_neighbors(L'a', L'p'));
    // Off: no corrections anywhere.
    auto no_correction = defaults;
    no_correction.typo_correction = false;
    engine.set_input_options(no_correction);
    REQUIRE_TRUE(!contains(engine.lookup(L"nihso"), L"你好") || !engine.lookup(L"nihso").front().correction);
    REQUIRE_TRUE(!contains(engine.lookup(L"zhida"), L"知道"));
    for (const auto& candidate : engine.lookup(L"xihaun")) REQUIRE_TRUE(!candidate.correction);
    engine.set_input_options(defaults);

    // ── Fuzzy pinyin ────────────────────────────────────────────────────────
    REQUIRE_TRUE(fuzzy_syllable_variants(L"zong", 0).empty());
    REQUIRE_EQ(fuzzy_syllable_variants(L"zong", kFuzzyZZh), std::vector<std::wstring>{L"zhong"});
    auto lin = fuzzy_syllable_variants(L"lin", kFuzzyNL | kFuzzyInIng);
    std::sort(lin.begin(), lin.end());
    REQUIRE_EQ(lin, (std::vector<std::wstring>{L"ling", L"nin", L"ning"}));
    REQUIRE_TRUE(fuzzy_syllable_variants(L"zhi", kFuzzyZZh) == std::vector<std::wstring>{L"zi"});
    REQUIRE_EQ(parse_fuzzy_pinyin(fuzzy_pinyin_to_string(kFuzzyAll)), static_cast<unsigned>(kFuzzyAll));
    REQUIRE_EQ(parse_fuzzy_pinyin(L"z-zh,bogus,in-ing"), static_cast<unsigned>(kFuzzyZZh | kFuzzyInIng));
    auto fuzzy = defaults;
    fuzzy.fuzzy = kFuzzyZZh | kFuzzySSh | kFuzzyNL | kFuzzyInIng | kFuzzyEnEng;
    engine.set_input_options(fuzzy);
    REQUIRE_EQ(engine.lookup(L"zongguo").front().text, std::wstring(L"中国"));
    REQUIRE_EQ(engine.lookup(L"sanghai").front().text, std::wstring(L"上海"));
    REQUIRE_EQ(engine.lookup(L"senme").front().text, std::wstring(L"什么"));
    REQUIRE_TRUE(rank_of(engine.lookup(L"xinqi"), L"星期") >= 0);
    REQUIRE_TRUE(rank_of(engine.lookup(L"lihao"), L"你好") >= 0);
    // Correct spellings keep their own reading first.
    REQUIRE_EQ(engine.lookup(L"zhongguo").front().text, std::wstring(L"中国"));
    REQUIRE_EQ(engine.lookup(L"nihao").front().text, std::wstring(L"你好"));
    // Fuzzy readings are readings, not corrections.
    REQUIRE_TRUE(!engine.lookup(L"zongguo").front().correction);
    engine.set_input_options(defaults);

    // ── Abbreviations (简拼) ─────────────────────────────────────────────────
    REQUIRE_EQ(engine.lookup(L"bjdx").front().text, std::wstring(L"北京大学"));
    REQUIRE_EQ(engine.lookup(L"jsj").front().text, std::wstring(L"计算机"));
    // zh/ch/sh typed as two letters: each syllable starts with its initial.
    auto zhq = engine.lookup(L"zhqq");
    REQUIRE_TRUE(!zhq.empty() && zhq.front().text.size() == 3);

    // ── Chinese-English mixed input ─────────────────────────────────────────
    auto python = engine.lookup(L"woyongpythonxiedaima");
    REQUIRE_TRUE(python.front().text.starts_with(L"我用 Python "));
    REQUIRE_TRUE(contains(python, L"我用 Python 写代码"));
    REQUIRE_EQ(engine.lookup(L"woyongappdakai").front().text, std::wstring(L"我用 App 打开"));
    // A term being typed completes from three letters.
    REQUIRE_EQ(engine.lookup(L"woyongpyt").front().text, std::wstring(L"我用 Python"));
    // Whole-input terms and abbreviations; pinyin-like ones after Chinese.
    REQUIRE_EQ(engine.lookup(L"ppt").front().text, std::wstring(L"PPT"));
    REQUIRE_EQ(engine.lookup(L"python").front().text, std::wstring(L"Python"));
    REQUIRE_EQ(engine.lookup(L"github").front().text, std::wstring(L"GitHub"));
    auto ai = engine.lookup(L"ai");
    REQUIRE_TRUE(ai.front().text != L"AI" && contains(ai, L"AI"));
    // Chinese wins any span both can read; words that read as pinyin are
    // never English inside a sentence.
    REQUIRE_EQ(engine.lookup(L"mache").front().text, std::wstring(L"马车"));
    REQUIRE_TRUE(EnglishTerms::reads_as_pinyin(L"change") && EnglishTerms::reads_as_pinyin(L"line"));
    REQUIRE_TRUE(!EnglishTerms::reads_as_pinyin(L"python") && !EnglishTerms::reads_as_pinyin(L"github"));
    REQUIRE_EQ(engine.lookup(L"zhongguo").front().text, std::wstring(L"中国"));
    // The window shows English terms set apart from the syllables.
    REQUIRE_EQ(engine.display_input(L"woyongpythonxiedaima"), std::wstring(L"wo'yong python xie'dai'ma"));
    REQUIRE_EQ(engine.display_input(L"nihaoshijie"), std::wstring(L"ni'hao'shi'jie"));
    auto no_english = defaults;
    no_english.mixed_english = false;
    engine.set_input_options(no_english);
    REQUIRE_TRUE(engine.lookup(L"woyongpythonxiedaima").front().text.find(L"Python") == std::wstring::npos);
    REQUIRE_TRUE(!contains(engine.lookup(L"ppt"), L"PPT"));
    engine.set_input_options(defaults);

    // ── Double pinyin ───────────────────────────────────────────────────────
    REQUIRE_EQ(double_pinyin_syllable(L'n', L'i', DoublePinyinScheme::Xiaohe), std::wstring(L"ni"));
    REQUIRE_EQ(double_pinyin_syllable(L'h', L'c', DoublePinyinScheme::Xiaohe), std::wstring(L"hao"));
    REQUIRE_EQ(double_pinyin_syllable(L'h', L'k', DoublePinyinScheme::Ziranma), std::wstring(L"hao"));
    REQUIRE_EQ(double_pinyin_syllable(L'v', L's', DoublePinyinScheme::Xiaohe), std::wstring(L"zhong"));
    REQUIRE_EQ(double_pinyin_syllable(L'i', L'l', DoublePinyinScheme::Xiaohe), std::wstring(L"chuang"));
    REQUIRE_EQ(double_pinyin_syllable(L'j', L'l', DoublePinyinScheme::Xiaohe), std::wstring(L"jiang"));
    REQUIRE_EQ(double_pinyin_syllable(L'k', L'k', DoublePinyinScheme::Xiaohe), std::wstring(L"kuai"));
    REQUIRE_EQ(double_pinyin_syllable(L'l', L'k', DoublePinyinScheme::Xiaohe), std::wstring(L"ling"));
    REQUIRE_EQ(double_pinyin_syllable(L'l', L'v', DoublePinyinScheme::Xiaohe), std::wstring(L"lv"));
    REQUIRE_EQ(double_pinyin_syllable(L'd', L'v', DoublePinyinScheme::Xiaohe), std::wstring(L"dui"));
    REQUIRE_EQ(double_pinyin_syllable(L'l', L'y', DoublePinyinScheme::Ziranma), std::wstring(L"ling"));
    REQUIRE_EQ(double_pinyin_syllable(L'k', L'y', DoublePinyinScheme::Ziranma), std::wstring(L"kuai"));
    REQUIRE_EQ(double_pinyin_syllable(L'a', L'a', DoublePinyinScheme::Xiaohe), std::wstring(L"a"));
    REQUIRE_EQ(double_pinyin_syllable(L'a', L'h', DoublePinyinScheme::Xiaohe), std::wstring(L"ang"));
    REQUIRE_EQ(double_pinyin_syllable(L'e', L'g', DoublePinyinScheme::Ziranma), std::wstring(L"eng"));
    REQUIRE_EQ(double_pinyin_syllable(L'o', L'u', DoublePinyinScheme::Xiaohe), std::wstring(L"ou"));
    REQUIRE_TRUE(double_pinyin_syllable(L'f', L'x', DoublePinyinScheme::Xiaohe).empty());
    auto conversion = convert_double_pinyin(L"nihcv", DoublePinyinScheme::Xiaohe);
    REQUIRE_EQ(conversion.pinyin, std::wstring(L"ni'hao'zh"));
    REQUIRE_EQ(conversion.raw_end.size(), conversion.pinyin.size() + 1);
    REQUIRE_EQ(conversion.raw_end[2], size_t{2});   // "ni" is two keys
    REQUIRE_EQ(conversion.raw_end[3], size_t{2});   // the separator adds none
    REQUIRE_EQ(conversion.raw_end[6], size_t{4});
    REQUIRE_EQ(conversion.raw_end.back(), size_t{5});
    REQUIRE_EQ(convert_double_pinyin(L"ni'hc", DoublePinyinScheme::Xiaohe).pinyin, std::wstring(L"ni'hao"));

    auto xiaohe = defaults;
    xiaohe.double_pinyin = DoublePinyinScheme::Xiaohe;
    engine.set_input_options(xiaohe);
    REQUIRE_EQ(engine.lookup(L"nihc").front().text, std::wstring(L"你好"));
    REQUIRE_EQ(engine.lookup(L"vsgo").front().text, std::wstring(L"中国"));
    REQUIRE_EQ(engine.lookup(L"nihcuijp").front().text, std::wstring(L"你好世界"));
    // 今天晚上去图书馆: jb tm wj uh qu tu uu gr
    REQUIRE_EQ(engine.lookup(L"jbtmwjuhqutuuugr").front().text, std::wstring(L"今天晚上去图书馆"));
    REQUIRE_EQ(engine.display_input(L"nihcuijp"), std::wstring(L"ni'hao'shi'jie"));
    // A prefix choice consumes whole key pairs of what was typed.
    for (const auto& candidate : engine.lookup(L"nihcuijp")) {
        if (candidate.consumed_pinyin) REQUIRE_TRUE(candidate.consumed_pinyin % 2 == 0 && candidate.consumed_pinyin < 8);
    }
    auto ziranma = defaults;
    ziranma.double_pinyin = DoublePinyinScheme::Ziranma;
    engine.set_input_options(ziranma);
    REQUIRE_EQ(engine.lookup(L"nihk").front().text, std::wstring(L"你好"));
    REQUIRE_EQ(engine.lookup(L"nihkuijx").front().text, std::wstring(L"你好世界"));
    // Fuzzy pinyin applies to the converted syllables (zong -> zhong).
    ziranma.fuzzy = kFuzzyZZh;
    engine.set_input_options(ziranma);
    REQUIRE_EQ(engine.lookup(L"zsgo").front().text, std::wstring(L"中国"));
    REQUIRE_EQ(parse_double_pinyin(double_pinyin_to_string(DoublePinyinScheme::Ziranma)), DoublePinyinScheme::Ziranma);
    REQUIRE_EQ(parse_double_pinyin(L"weird"), DoublePinyinScheme::None);

    // ── Usage scenes (常用场景) ─────────────────────────────────────────────
    REQUIRE_EQ(parse_usage_domains(L"daily,coding,unknown"), static_cast<unsigned>(kDomainCoding | kDomainDaily));
    REQUIRE_EQ(usage_domains_to_string(kDomainDaily | kDomainCoding), std::wstring(L"coding,daily"));
    engine.set_input_options(defaults);
    REQUIRE_TRUE(!engine.lookup(L"haoxiangshi").front().domain);
    // The daily scene lifts everyday words over the old variant 好象是 and,
    // after preceding text, over the news-trained model's 歧视.
    auto daily = defaults;
    daily.domains = kDomainDaily;
    engine.set_input_options(daily);
    REQUIRE_EQ(engine.lookup(L"haoxiangshi").front().text, std::wstring(L"好像是"));
    REQUIRE_TRUE(engine.lookup(L"haoxiangshi").front().domain);
    REQUIRE_EQ(engine.lookup(L"qishi", L"所以整道题").front().text, std::wstring(L"其实"));
    // A scene word the dictionary lacks is offered while its scene is chosen.
    auto coding = defaults;
    coding.domains = kDomainCoding;
    engine.set_input_options(coding);
    REQUIRE_EQ(engine.lookup(L"yilaizhuru").front().text, std::wstring(L"依赖注入"));
    REQUIRE_TRUE(engine.lookup(L"yilaizhuru").front().domain);
    // Dropping the scenes restores the plain order at once.
    engine.set_input_options(defaults);
    REQUIRE_TRUE(!engine.lookup(L"haoxiangshi").front().domain);
    REQUIRE_TRUE(!engine.lookup(L"yilaizhuru").front().domain);
    return 0;
}
