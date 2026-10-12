#include "test_common.h"
#include "../src/common/utf_utils.h"
#include "../src/engine/dictionary.h"

#include <fstream>
#include <chrono>
#include <algorithm>

namespace {

void write_utf8_text(const std::filesystem::path& path, const std::wstring& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file << localpinyin::wide_to_utf8(text);
}

const localpinyin::DictionaryLayerStats& layer_named(const localpinyin::Dictionary& dictionary,
                                                     const std::wstring& name) {
    for (const auto& layer : dictionary.layer_stats()) {
        if (layer.layer_name == name) {
            return layer;
        }
    }
    static const localpinyin::DictionaryLayerStats empty{};
    return empty;
}

}  // namespace

int main() {
    const auto root = make_test_temp_directory();
    const auto core_path = root / L"core.tsv";
    const auto local_core_path = root / L"local_core.tsv";
    const auto user_path = root / L"user" / L"user_lexicon.tsv";
    const std::wstring sensitive_pinyin = L"secretpinyin";
    const std::wstring sensitive_word = L"\u79C1\u5BC6\u8BCD";

    write_utf8_text(core_path,
                    L"# core\n"
                    L"shared\t\u5171\u4EAB\t10\n"
                    L"userwins\t\u79C1\u6709\t100\n"
                    L"xing\t\u884C\t300\n"
                    L"hang\t\u884C\t200\n");
    write_utf8_text(local_core_path,
                    L"# local core\n"
                    L"shared\t\u5171\u4EAB\t800\n"
                    L"userwins\t\u79C1\u6709\t200\n"
                    L"localonly\t\u5185\u7F6E\t700\n");
    write_utf8_text(user_path,
                    L"# user\n"
                    L"userwins\t\u79C1\u6709\t5000\n"
                    L"csc\tCSC108\t5000\n"
                    L"secretpinyin\t\u79C1\u5BC6\u8BCD\t321\n"
                    L"\t\u7A7A\u62FC\u97F3\t1\n"
                    L"emptyword\t\t1\n"
                    L"negative\t\u574F\t-1\n"
                    L"badfreq\t\u574F\tabc\n"
                    L"tabinject\tbad\tfield\t1\n");

    localpinyin::Dictionary dictionary(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(dictionary.load_from_layered_files(core_path.wstring(),
                                                    local_core_path.wstring(),
                                                    user_path.wstring(),
                                                    localpinyin::UserLexiconCreateMode::CreateIfMissing));
    REQUIRE_EQ(dictionary.layer_stats().size(), static_cast<size_t>(3));
    REQUIRE_EQ(dictionary.lookup(L"shared").front().text, std::wstring(L"\u5171\u4EAB"));
    REQUIRE_EQ(dictionary.lookup(L"shared").front().base_score, 800);
    REQUIRE_EQ(dictionary.lookup(L"userwins").front().text, std::wstring(L"\u79C1\u6709"));
    REQUIRE_EQ(dictionary.lookup(L"userwins").front().base_score, 5000);
    REQUIRE_EQ(dictionary.lookup(L"localonly").front().text, std::wstring(L"\u5185\u7F6E"));
    REQUIRE_EQ(dictionary.lookup(L"xing").front().text, std::wstring(L"\u884C"));
    REQUIRE_EQ(dictionary.lookup(L"hang").front().text, std::wstring(L"\u884C"));
    REQUIRE_EQ(dictionary.lookup(L"csc").front().text, std::wstring(L"CSC108"));
    REQUIRE_EQ(dictionary.lookup(sensitive_pinyin).front().text, sensitive_word);

    const auto& user_layer = layer_named(dictionary, L"local_user");
    REQUIRE_TRUE(user_layer.stats.invalid_rows >= static_cast<size_t>(5));
    for (const auto& message : dictionary.layer_log_messages()) {
        REQUIRE_TRUE(message.find(sensitive_pinyin) == std::wstring::npos);
        REQUIRE_TRUE(message.find(sensitive_word) == std::wstring::npos);
    }

    const auto missing_root = make_test_temp_directory();
    const auto missing_user_path = missing_root / L"user" / L"user_lexicon.tsv";
    localpinyin::Dictionary missing_user(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(missing_user.load_from_layered_files(core_path.wstring(),
                                                      local_core_path.wstring(),
                                                      missing_user_path.wstring(),
                                                      localpinyin::UserLexiconCreateMode::CreateIfMissing));
    REQUIRE_TRUE(std::filesystem::exists(missing_user_path));
    REQUIRE_TRUE(layer_named(missing_user, L"local_user").created);

    // A reference layer supplies new homophones and sentence priors without
    // overwriting project scores; an explicit user score overrides both.
    const auto standard_path = root / L"standard.tsv";
    const auto standard_user_path = root / L"standard_user.tsv";
    write_utf8_text(standard_path,
                    L"shared\t\u5171\u4EAB\t6200\n"
                    L"shared\t\u5206\u4EAB\t6900\n"
                    L"xin\t\u65B0\t6600\n"
                    L"bad\tinvalid\tbroken\n");
    write_utf8_text(standard_user_path, L"shared\t\u5171\u4EAB\t9100\n");
    localpinyin::Dictionary reference(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(reference.load_from_layered_files(core_path.wstring(), local_core_path.wstring(),
                                                   standard_user_path.wstring(),
                                                   localpinyin::UserLexiconCreateMode::CreateIfMissing,
                                                   standard_path.wstring()));
    REQUIRE_EQ(reference.layer_stats().size(), static_cast<size_t>(4));
    REQUIRE_TRUE(layer_named(reference, L"standard").loaded);
    REQUIRE_EQ(layer_named(reference, L"standard").stats.invalid_rows, static_cast<size_t>(1));
    REQUIRE_EQ(reference.lookup(L"xin").front().base_score, 6600);
    REQUIRE_EQ(reference.lookup(L"xin").front().decoding_frequency, 6600);
    REQUIRE_EQ(reference.lookup(L"shared").front().base_score, 9100);
    REQUIRE_EQ(reference.lookup(L"shared").front().decoding_frequency, 9100);
    const auto old_time = std::filesystem::last_write_time(standard_user_path);
    write_utf8_text(standard_user_path, L"# cleared\n");
    std::filesystem::last_write_time(standard_user_path, old_time + std::chrono::seconds(1));
    REQUIRE_TRUE(reference.refresh_user_lexicon_if_changed(false).reloaded);
    const auto restored = reference.lookup(L"shared");
    const auto original = std::find_if(restored.begin(), restored.end(), [](const auto& candidate) {
        return candidate.text == L"\u5171\u4EAB";
    });
    REQUIRE_TRUE(original != restored.end());
    REQUIRE_EQ(original->base_score, 800);
    REQUIRE_EQ(original->decoding_frequency, 6200);
    REQUIRE_EQ(reference.lookup(L"xin").front().decoding_frequency, 6600);

    // Lazy readings cover ambiguous syllable counts; cached positive and
    // negative predictions must refresh when the user edits their lexicon.
    const auto prediction_core = root / L"prediction_core.tsv";
    const auto prediction_local = root / L"prediction_local.tsv";
    const auto prediction_user = root / L"prediction_user.tsv";
    write_utf8_text(prediction_core,
                    L"ni\t你\t9000\nnihao\t你好\t9000\nbeijing\t北京\t9000\n"
                    L"xian\t先\t9000\nxian\t西安\t8900\n");
    write_utf8_text(prediction_local, L"# empty\n");
    write_utf8_text(prediction_user, L"nanjing\t南京\t9000\n");
    localpinyin::Dictionary predictions(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(predictions.load_from_layered_files(prediction_core.wstring(), prediction_local.wstring(),
                                                     prediction_user.wstring()));
    REQUIRE_EQ(predictions.lookup_completion(L"n", true).front().text, std::wstring(L"你"));
    REQUIRE_EQ(predictions.lookup_completion(L"beij", true).front().text, std::wstring(L"北京"));
    REQUIRE_EQ(predictions.lookup_abbreviation(L"nh").front().text, std::wstring(L"你好"));
    REQUIRE_EQ(predictions.lookup_abbreviation(L"xa").front().text, std::wstring(L"西安"));
    REQUIRE_EQ(predictions.lookup_abbreviation(L"nj").front().text, std::wstring(L"南京"));
    REQUIRE_TRUE(predictions.lookup_abbreviation(L"ny").empty());
    const auto prediction_time = std::filesystem::last_write_time(prediction_user);
    write_utf8_text(prediction_user, L"nanyue\t南岳\t9000\n");
    std::filesystem::last_write_time(prediction_user, prediction_time + std::chrono::seconds(1));
    REQUIRE_TRUE(predictions.refresh_user_lexicon_if_changed(false).reloaded);
    REQUIRE_TRUE(predictions.lookup_abbreviation(L"nj").empty());
    REQUIRE_TRUE(predictions.lookup_completion(L"nanj", true).empty());
    REQUIRE_EQ(predictions.lookup_abbreviation(L"ny").front().text, std::wstring(L"南岳"));
    REQUIRE_EQ(predictions.lookup_completion(L"nany", true).front().text, std::wstring(L"南岳"));

    std::filesystem::remove_all(root);
    std::filesystem::remove_all(missing_root);
    return 0;
}
