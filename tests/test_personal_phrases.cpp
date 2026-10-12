#include "test_common.h"
#include "../src/common/utf_utils.h"
#include "../src/common/win32_utils.h"
#include "../src/engine/pinyin_engine.h"

#include <algorithm>
#include <chrono>
#include <fstream>

namespace {

struct Phrase { std::wstring pinyin; std::wstring text; };

const std::vector<Phrase>& phrases() {
    static const std::vector<Phrase> values = {
        {L"qingfasongfujian", L"请发送附件"},
        {L"zhouwuzhiqianhuifuyoujian", L"周五之前回复邮件"},
        {L"qingshikejifazhanyouxiangongsi", L"青石科技发展有限公司"},
        {L"shoudaowenjianhouqingjinkuaiquerenbinghuifuxiexie", L"收到文件后请尽快确认并回复谢谢"},
        {L"qingshikejifazhanyouxiangongsishujuanquanyanjiuzhongxin", L"青石科技发展有限公司数据安全研究中心"},
        {L"qingshikejifazhanyouxiangongsishujuanquanyanjiuzhongxinzongbu", L"青石科技发展有限公司数据安全研究中心总部"},
    };
    return values;
}

void write_text(const std::filesystem::path& path, const std::wstring& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file << localpinyin::wide_to_utf8(text);
}

bool save_personal(const std::filesystem::path& path, const std::vector<localpinyin::UserLexiconEntry>& entries) {
    const bool existed = std::filesystem::exists(path);
    const auto previous_time = existed ? std::filesystem::last_write_time(path) : std::filesystem::file_time_type{};
    if (!localpinyin::save_user_lexicon_file_atomic(path.wstring(), entries).saved) return false;
    // A filesystem's timestamp resolution must not make refresh tests flaky.
    if (existed) std::filesystem::last_write_time(path, previous_time + std::chrono::seconds(1));
    return true;
}

bool load(localpinyin::PinyinEngine& engine, const std::filesystem::path& root) {
    return engine.load_dictionary_resource_directory((root / L"resources").wstring(), (root / L"user.tsv").wstring());
}

bool is_direct_first(const std::vector<localpinyin::Candidate>& candidates, const std::wstring& text) {
    return !candidates.empty() && candidates.front().text == text &&
           candidates.front().is_user_lexicon_entry && candidates.front().exact_match_bonus > 0 &&
           candidates.front().consumed_pinyin == 0;
}

bool has_no_long_direct_match(const std::vector<localpinyin::Candidate>& candidates) {
    return std::none_of(candidates.begin(), candidates.end(), [](const auto& candidate) {
        return candidate.text.size() > 4 && candidate.exact_match_bonus > 0;
    });
}

bool check_preferred_phrases(const std::filesystem::path& root) {
    localpinyin::PinyinEngine engine(localpinyin::DictionaryLoadMode::Empty);
    if (!load(engine, root)) return false;
    for (const auto& phrase : phrases()) {
        if (!is_direct_first(engine.lookup(phrase.pinyin), phrase.text)) return false;
    }
    return true;
}

bool run_child(const std::filesystem::path& root, const std::wstring& mode) {
    const auto executable = localpinyin::module_path(nullptr);
    std::wstring arguments = L"\"" + executable + L"\" " + mode + L" \"" + root.wstring() + L"\"";
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(executable.c_str(), arguments.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process)) return false;
    const DWORD waited = WaitForSingleObject(process.hProcess, 10000);
    DWORD exit_code = 1;
    if (waited == WAIT_OBJECT_0) GetExitCodeProcess(process.hProcess, &exit_code);
    else TerminateProcess(process.hProcess, 99);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return waited == WAIT_OBJECT_0 && exit_code == 0;
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc == 3 && std::wstring(argv[1]) == L"--reopen") return check_preferred_phrases(argv[2]) ? 0 : 1;
    if (argc == 3 && std::wstring(argv[1]) == L"--learn-reopen") {
        localpinyin::PinyinEngine engine(localpinyin::DictionaryLoadMode::Empty);
        if (!load(engine, argv[2])) return 1;
        return is_direct_first(engine.lookup(phrases()[2].pinyin), phrases()[2].text) ? 0 : 1;
    }
    REQUIRE_EQ(argc, 1);
    const auto root = make_test_temp_directory();
    const auto core = root / L"resources" / L"dictionary" / L"core_zh_pinyin.tsv";
    const auto local = core.parent_path() / L"local_core_zh_pinyin.tsv";
    const auto user = root / L"user.tsv";
    SetEnvironmentVariableW(L"LOCALPINYINIME_USER_LEXICON_PATH", user.c_str());
    SetEnvironmentVariableW(L"LOCALPINYINIME_LOCALAPPDATA", root.c_str());

    // Fixed, small homophone competitors make personal-entry semantics
    // independent of future corpus or bundled dictionary improvements.
    write_text(core,
        L"qing\t情\t9500\nqing\t请\t6000\nfasong\t发送\t9000\n"
        L"fujian\t福建\t9500\nfujian\t附件\t5500\n"
        L"zhouwu\t周五\t9000\nzhiqian\t之前\t9000\nhuifu\t回复\t9000\n"
        L"youjian\t右肩\t9500\nyoujian\t邮件\t5000\n"
        L"qingshi\t清史\t9500\nqingshi\t青石\t1000\nkeji\t科技\t9000\n"
        L"fazhan\t发展\t9000\nyouxian\t有限\t9000\ngongsi\t公司\t9000\n"
        L"shuju\t数据\t9000\nanquan\t安全\t9000\nyanjiu\t研究\t9000\n"
        L"zhongxin\t中心\t9000\nzongbu\t总部\t9000\n"
        L"shoudao\t收到\t9000\nwenjian\t文件\t9000\nhou\t后\t9000\n"
        L"jinkuai\t尽快\t9000\nqueren\t确认\t9000\nbing\t并\t9000\nxiexie\t谢谢\t9000\n"
        L"qingshikejifazhanyouxiangongsi\t青石科技发展有限公司\t9000\n");
    write_text(local, L"# empty\n");
    std::vector<localpinyin::UserLexiconEntry> entries;
    for (const auto& phrase : phrases()) {
        REQUIRE_TRUE(phrase.text.size() >= 5 && phrase.text.size() <= 20);
        entries.push_back({phrase.pinyin, phrase.text, localpinyin::kPreferredUserLexiconFrequency});
    }
    REQUIRE_TRUE(save_personal(user, entries));

    localpinyin::PinyinEngine engine(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(load(engine, root));
    for (const auto& phrase : phrases()) {
        REQUIRE_TRUE(is_direct_first(engine.lookup(phrase.pinyin), phrase.text));
        REQUIRE_TRUE(is_direct_first(engine.lookup(phrase.pinyin, L"单位名称是"), phrase.text));
        REQUIRE_TRUE(has_no_long_direct_match(engine.lookup(phrase.pinyin.substr(0, phrase.pinyin.size() - 1))));
    }
    REQUIRE_TRUE(is_direct_first(engine.lookup(L"QINGFASONGFUJIAN"), phrases()[0].text));
    REQUIRE_TRUE(is_direct_first(engine.lookup(L"qing'fa'song'fu'jian"), phrases()[0].text));
    REQUIRE_TRUE(has_no_long_direct_match(engine.lookup(L"qi'ngfasongfujian")));
    REQUIRE_TRUE(run_child(root, L"--reopen"));
    REQUIRE_TRUE(check_preferred_phrases(root));

    localpinyin::Dictionary dictionary(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(dictionary.load_from_layered_files(core.wstring(), local.wstring(), user.wstring()));
    auto overridden = dictionary.lookup(phrases()[2].pinyin);
    REQUIRE_EQ(overridden.size(), static_cast<size_t>(1));
    REQUIRE_TRUE(overridden.front().is_user_lexicon_entry);
    REQUIRE_EQ(overridden.front().base_score, 5000);

    // A deliberately low personal frequency retains contextual ranking, even
    // though its provenance remains personal. Active composition defers edits.
    for (int frequency : {0, 1000}) {
        for (auto& entry : entries) entry.frequency = frequency;
        REQUIRE_TRUE(save_personal(user, entries));
        REQUIRE_TRUE(engine.refresh_user_lexicon_if_needed(true).skipped_active_composition);
        REQUIRE_TRUE(engine.refresh_user_lexicon_if_needed(false).reloaded);
        REQUIRE_TRUE(dictionary.refresh_user_lexicon_if_changed(false).reloaded);
        for (const auto& phrase : phrases()) {
            REQUIRE_TRUE(has_no_long_direct_match(engine.lookup(phrase.pinyin)));
            const auto exact = dictionary.lookup(phrase.pinyin);
            REQUIRE_TRUE(exact.front().is_user_lexicon_entry);
            REQUIRE_EQ(exact.front().base_score, frequency);
        }
    }

    // Removing the personal layer restores the built-in score and provenance;
    // wholly personal entries disappear, including cached direct matches.
    REQUIRE_TRUE(save_personal(user, {}));
    REQUIRE_TRUE(engine.refresh_user_lexicon_if_needed(false).reloaded);
    REQUIRE_TRUE(dictionary.refresh_user_lexicon_if_changed(false).reloaded);
    const auto restored = dictionary.lookup(phrases()[2].pinyin);
    REQUIRE_EQ(restored.size(), static_cast<size_t>(1));
    REQUIRE_EQ(restored.front().base_score, 9000);
    REQUIRE_TRUE(!restored.front().is_user_lexicon_entry);
    REQUIRE_TRUE(dictionary.lookup(phrases()[0].pinyin).empty());
    REQUIRE_TRUE(has_no_long_direct_match(engine.lookup(phrases()[2].pinyin)));

    // The same full key can name two personal entries. Their explicit
    // frequencies still order them, and a selection can change that order.
    const std::wstring other = L"清史科技发展有限公司";
    REQUIRE_TRUE(save_personal(user, {{phrases()[2].pinyin, phrases()[2].text, 5000},
                                     {phrases()[2].pinyin, other, 6000}}));
    REQUIRE_TRUE(engine.refresh_user_lexicon_if_needed(false).reloaded);
    REQUIRE_TRUE(dictionary.refresh_user_lexicon_if_changed(false).reloaded);
    auto competing = engine.lookup(phrases()[2].pinyin);
    REQUIRE_TRUE(is_direct_first(competing, other));
    REQUIRE_TRUE(competing.size() >= 2);
    REQUIRE_EQ(competing[1].text, phrases()[2].text);
    REQUIRE_TRUE(competing[1].is_user_lexicon_entry);
    engine.learn(phrases()[2].pinyin, phrases()[2].text);
    REQUIRE_TRUE(is_direct_first(engine.lookup(phrases()[2].pinyin), phrases()[2].text));
    REQUIRE_TRUE(run_child(root, L"--learn-reopen"));
    localpinyin::PinyinEngine reopened(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(load(reopened, root));
    REQUIRE_TRUE(is_direct_first(reopened.lookup(phrases()[2].pinyin), phrases()[2].text));

    REQUIRE_TRUE(save_personal(user, {}));
    REQUIRE_TRUE(dictionary.refresh_user_lexicon_if_changed(false).reloaded);
    const auto final_restored = dictionary.lookup(phrases()[2].pinyin);
    REQUIRE_EQ(final_restored.size(), static_cast<size_t>(1));
    REQUIRE_TRUE(!final_restored.front().is_user_lexicon_entry);
    REQUIRE_TRUE(dictionary.load_from_tsv_file(core.wstring()));
    REQUIRE_TRUE(!dictionary.lookup(phrases()[2].pinyin).front().is_user_lexicon_entry);

    std::filesystem::remove_all(root);
    return 0;
}
