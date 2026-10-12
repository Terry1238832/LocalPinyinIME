#include "test_common.h"
#include "../src/common/utf_utils.h"
#include "../src/engine/dictionary.h"

#include <algorithm>
#include <chrono>
#include <fstream>

namespace {

void write_utf8_text(const std::filesystem::path& path, const std::wstring& text) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file << localpinyin::wide_to_utf8(text);
}

bool same_predictions(const std::vector<localpinyin::Candidate>& left,
                      const std::vector<localpinyin::Candidate>& right) {
    return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin(),
        [](const auto& a, const auto& b) {
            return a.text == b.text && a.base_score == b.base_score &&
                   a.decoding_frequency == b.decoding_frequency;
        });
}

}  // namespace

int main() {
    const auto user_lexicon_root = use_temp_user_lexicon_override();

    localpinyin::Dictionary dictionary;
    REQUIRE_TRUE(dictionary.entry_count() >= 300);
    REQUIRE_TRUE(dictionary.stats().source_rows >= static_cast<size_t>(404));
    REQUIRE_TRUE(dictionary.stats().duplicate_rows >= static_cast<size_t>(1));
    REQUIRE_EQ(dictionary.stats().invalid_rows, static_cast<size_t>(0));
    REQUIRE_EQ(dictionary.stats().valid_entries, dictionary.entry_count());
    REQUIRE_EQ(dictionary.layer_stats().size(), static_cast<size_t>(4));
    REQUIRE_TRUE(std::filesystem::exists(user_lexicon_root / L"user_lexicon.tsv"));

    auto candidates = dictionary.lookup(L"nihao");
    REQUIRE_TRUE(!candidates.empty());
    REQUIRE_EQ(candidates.front().text, std::wstring(L"\u4F60\u597D"));
    REQUIRE_EQ(dictionary.lookup(L"henbang").front().text, std::wstring(L"\u5F88\u68D2"));
    REQUIRE_EQ(dictionary.lookup(L"zhongguo").front().text, std::wstring(L"\u4E2D\u56FD"));
    REQUIRE_EQ(dictionary.lookup(L"beijing").front().text, std::wstring(L"\u5317\u4EAC"));
    REQUIRE_TRUE(dictionary.lookup(L"missing").empty());

    const auto core_path = user_lexicon_root / L"prediction-core.tsv";
    const auto local_path = user_lexicon_root / L"prediction-local.tsv";
    const auto user_path = user_lexicon_root / L"prediction-user.tsv";
    write_utf8_text(core_path,
        L"ni\t你\t9000\nnihao\t你好\t3000\nnihao\t尼豪\t2900\n"
        L"nihao\t倪豪\t2800\nnihao\t泥豪\t2700\nnanjing\t南京\t5000\n"
        L"beifang\t北方\t8000\nbeijing\t北京\t7000\n"
        L"xian\t先\t6500\nxian\t西安\t6400\n"
        L"zhongguo\t中国\t6000\nzhongwen\t中文\t6000\n");
    write_utf8_text(local_path, L"# empty\n");
    write_utf8_text(user_path, L"nanchang\t南昌\t5500\n");
    localpinyin::Dictionary predictions(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(predictions.load_from_layered_files(core_path.wstring(), local_path.wstring(), user_path.wstring()));

    // Letter partitions are shared by completion and initials, while their
    // creation order must not change candidates, scores, or per-key limits.
    const auto n_completion = predictions.lookup_completion(L"n");
    REQUIRE_EQ(n_completion.size(), static_cast<size_t>(5));
    REQUIRE_EQ(n_completion.front().text, std::wstring(L"你"));
    const auto nh = predictions.lookup_abbreviation(L"nh");
    REQUIRE_EQ(nh.size(), static_cast<size_t>(3));
    REQUIRE_EQ(nh.front().text, std::wstring(L"你好"));
    const auto b_completion = predictions.lookup_completion(L"b");
    REQUIRE_EQ(b_completion.size(), static_cast<size_t>(2));
    REQUIRE_EQ(b_completion.front().text, std::wstring(L"北方"));
    const auto bj = predictions.lookup_abbreviation(L"bj");
    REQUIRE_EQ(bj.front().text, std::wstring(L"北京"));
    const auto x_completion = predictions.lookup_completion(L"x", true);
    REQUIRE_EQ(x_completion.size(), static_cast<size_t>(1));
    REQUIRE_EQ(x_completion.front().text, std::wstring(L"先"));
    const auto xa = predictions.lookup_abbreviation(L"xa");
    REQUIRE_EQ(xa.front().text, std::wstring(L"西安"));
    const auto zh_completion = predictions.lookup_completion(L"zh");
    REQUIRE_EQ(zh_completion.size(), static_cast<size_t>(2));
    REQUIRE_EQ(zh_completion.front().text, std::wstring(L"中国"));

    localpinyin::Dictionary reverse_order(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(reverse_order.load_from_layered_files(core_path.wstring(), local_path.wstring(), user_path.wstring()));
    REQUIRE_TRUE(same_predictions(reverse_order.lookup_completion(L"zh"), zh_completion));
    REQUIRE_TRUE(same_predictions(reverse_order.lookup_abbreviation(L"xa"), xa));
    REQUIRE_TRUE(same_predictions(reverse_order.lookup_completion(L"x", true), x_completion));
    REQUIRE_TRUE(same_predictions(reverse_order.lookup_abbreviation(L"bj"), bj));
    REQUIRE_TRUE(same_predictions(reverse_order.lookup_completion(L"b"), b_completion));
    REQUIRE_TRUE(same_predictions(reverse_order.lookup_abbreviation(L"nh"), nh));
    REQUIRE_TRUE(same_predictions(reverse_order.lookup_completion(L"n"), n_completion));

    // Cache both existing and missing readings, including an empty partition.
    REQUIRE_EQ(predictions.lookup_abbreviation(L"nc").front().text, std::wstring(L"南昌"));
    REQUIRE_TRUE(predictions.lookup_abbreviation(L"nl").empty());
    REQUIRE_TRUE(predictions.lookup_completion(L"nanl", true).empty());
    REQUIRE_TRUE(predictions.lookup_abbreviation(L"dl").empty());
    REQUIRE_TRUE(predictions.lookup_completion(L"d").empty());
    auto user_time = std::filesystem::last_write_time(user_path);
    write_utf8_text(user_path,
        L"nanling\t南陵\t6500\nnihao\t你好\t10000\n"
        L"beijing\t北京\t9000\ndalian\t大连\t8800\n");
    std::filesystem::last_write_time(user_path, user_time + std::chrono::seconds(1));
    REQUIRE_TRUE(predictions.refresh_user_lexicon_if_changed(false).reloaded);
    REQUIRE_TRUE(predictions.lookup_abbreviation(L"nc").empty());
    REQUIRE_TRUE(predictions.lookup_completion(L"nanch", true).empty());
    REQUIRE_EQ(predictions.lookup_abbreviation(L"nl").front().text, std::wstring(L"南陵"));
    REQUIRE_EQ(predictions.lookup_completion(L"nanl", true).front().text, std::wstring(L"南陵"));
    REQUIRE_EQ(predictions.lookup_completion(L"n").front().text, std::wstring(L"你好"));
    REQUIRE_EQ(predictions.lookup_abbreviation(L"nh").front().base_score, 10000);
    REQUIRE_EQ(predictions.lookup_completion(L"b").front().text, std::wstring(L"北京"));
    REQUIRE_EQ(predictions.lookup_abbreviation(L"bj").front().base_score, 9000);
    REQUIRE_EQ(predictions.lookup_completion(L"d").front().text, std::wstring(L"大连"));
    REQUIRE_EQ(predictions.lookup_abbreviation(L"dl").front().text, std::wstring(L"大连"));

    user_time = std::filesystem::last_write_time(user_path);
    write_utf8_text(user_path, L"# removed\n");
    std::filesystem::last_write_time(user_path, user_time + std::chrono::seconds(1));
    REQUIRE_TRUE(predictions.refresh_user_lexicon_if_changed(false).reloaded);
    REQUIRE_TRUE(predictions.lookup_abbreviation(L"nl").empty());
    REQUIRE_TRUE(predictions.lookup_completion(L"nanl", true).empty());
    REQUIRE_TRUE(predictions.lookup_abbreviation(L"dl").empty());
    REQUIRE_TRUE(predictions.lookup_completion(L"d").empty());
    REQUIRE_EQ(predictions.lookup_completion(L"n").front().text, std::wstring(L"你"));
    REQUIRE_EQ(predictions.lookup_completion(L"b").front().text, std::wstring(L"北方"));
    REQUIRE_EQ(predictions.lookup_abbreviation(L"nh").front().base_score, 3000);

    // A replacement dictionary must discard every previously built partition.
    write_utf8_text(core_path, L"qita\t其他\t9000\n");
    REQUIRE_TRUE(predictions.load_from_tsv_file(core_path.wstring()));
    REQUIRE_TRUE(predictions.lookup_completion(L"n").empty());
    REQUIRE_TRUE(predictions.lookup_abbreviation(L"bj").empty());
    REQUIRE_EQ(predictions.lookup_abbreviation(L"qt").front().text, std::wstring(L"其他"));

    std::filesystem::remove_all(user_lexicon_root);
    return 0;
}
