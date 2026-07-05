#include "test_common.h"
#include "../src/engine/pinyin_engine.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct SentenceCase {
    std::wstring pinyin;
    std::wstring expected;
    std::wstring category;
};

std::filesystem::path executable_directory() {
    std::array<wchar_t, MAX_PATH> path{};
    const DWORD size = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (size == 0 || size >= path.size()) {
        return std::filesystem::current_path();
    }
    return std::filesystem::path(path.data()).parent_path();
}

std::wstring first_candidate(localpinyin::PinyinEngine& engine, const std::wstring& pinyin) {
    const auto candidates = engine.lookup(pinyin);
    return candidates.empty() ? L"" : candidates.front().text;
}

std::string codepoints(const std::wstring& value) {
    std::ostringstream output;
    output << "len=" << value.size() << " [";
    for (std::size_t index = 0; index < value.size(); ++index) {
        if (index > 0) {
            output << ' ';
        }
        output << "U+";
        output.width(4);
        output.fill('0');
        output << std::uppercase << std::hex << static_cast<unsigned int>(value[index]);
        output << std::dec;
    }
    output << ']';
    return output.str();
}

bool expect_sentence(localpinyin::PinyinEngine& engine, const SentenceCase& test_case) {
    const std::wstring actual = first_candidate(engine, test_case.pinyin);
    const bool passed = actual == test_case.expected;
    std::wcout << L"sentence_baseline category=" << test_case.category
               << L" pinyin=" << test_case.pinyin
               << L" passed=" << (passed ? L"true" : L"false") << L"\n";
    if (!passed) {
        std::cout << "expected_codepoints=" << codepoints(test_case.expected) << "\n"
                  << "actual_codepoints=" << codepoints(actual) << "\n";
    }
    return passed;
}

bool expect_candidate_contains(localpinyin::PinyinEngine& engine,
                               const std::wstring& pinyin,
                               const std::wstring& expected) {
    const auto candidates = engine.lookup(pinyin);
    const bool passed = std::any_of(candidates.begin(), candidates.end(), [&](const localpinyin::Candidate& candidate) {
        return candidate.text == expected;
    });
    std::wcout << L"candidate_contains pinyin=" << pinyin
               << L" passed=" << (passed ? L"true" : L"false") << L"\n";
    if (!passed) {
        std::cout << "expected_codepoints=" << codepoints(expected) << "\n";
    }
    return passed;
}

}  // namespace

int main() {
    const auto user_lexicon_root = use_temp_user_lexicon_override();
    const auto resource_dir = executable_directory();

    localpinyin::PinyinEngine engine(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(engine.load_dictionary_resource_directory(resource_dir.wstring(),
                                                           (user_lexicon_root / L"user_lexicon.tsv").wstring()));

    const std::vector<SentenceCase> cases = {
        {L"kaihui", L"\u5F00\u4F1A", L"targeted"},
        {L"fangjia", L"\u653E\u5047", L"targeted"},
        {L"biancheng", L"\u53D8\u6210", L"targeted"},
        {L"shenle", L"\u795E\u4E86", L"targeted"},
        {L"zhiyou", L"\u53EA\u6709", L"targeted"},
        {L"bucuo", L"\u4E0D\u9519", L"targeted"},
        {L"bianji", L"\u7F16\u8F91", L"targeted"},
        {L"huode", L"\u83B7\u5F97", L"targeted"},
        {L"an", L"\u6309", L"targeted"},
        {L"anxia", L"\u6309\u4E0B", L"targeted"},
        {L"xiamian", L"\u4E0B\u9762", L"targeted"},
        {L"mian", L"\u9762", L"targeted"},
        {L"mianfei", L"\u514D\u8D39", L"targeted"},
        {L"mianfen", L"\u9762\u7C89", L"targeted"},
        {L"shouxian", L"\u9996\u5148", L"targeted"},
        {L"xian", L"\u5148", L"targeted"},
        {L"xianjiuzheng", L"\u5148\u7EA0\u6B63", L"targeted"},
        {L"yixie", L"\u4E00\u4E9B", L"targeted"},
        {L"qisiwole", L"\u6C14\u6B7B\u6211\u4E86", L"targeted"},
        {L"nihaoshijie", L"\u4F60\u597D\u4E16\u754C", L"required"},
        {L"woxiangqubeijing", L"\u6211\u60F3\u53BB\u5317\u4EAC", L"required"},
        {L"jintiantianqihenhao", L"\u4ECA\u5929\u5929\u6C14\u5F88\u597D", L"required"},
        {L"woxiangchiwanfan", L"\u6211\u60F3\u5403\u665A\u996D", L"required"},
        {L"qingbangwokankan", L"\u8BF7\u5E2E\u6211\u770B\u770B", L"required"},
        {L"zhegeshishenme", L"\u8FD9\u4E2A\u662F\u4EC0\u4E48", L"required"},
        {L"woxianzaiyouyidianmang", L"\u6211\u73B0\u5728\u6709\u4E00\u70B9\u5FD9", L"required"},
        {L"jintianxiawuyouke", L"\u4ECA\u5929\u4E0B\u5348\u6709\u8BFE", L"time"},
        {L"mingtianzaoshangjian", L"\u660E\u5929\u65E9\u4E0A\u89C1", L"time"},
        {L"jintianhuixiayu", L"\u4ECA\u5929\u4F1A\u4E0B\u96E8", L"weather"},
        {L"nixianzaiyoukongma", L"\u4F60\u73B0\u5728\u6709\u7A7A\u5417", L"question"},
        {L"qinggaosuwodizhi", L"\u8BF7\u544A\u8BC9\u6211\u5730\u5740", L"request"},
        {L"woxuyaodakaiwenjian", L"\u6211\u9700\u8981\u6253\u5F00\u6587\u4EF6", L"computer"},
        {L"womenyiqixuexi", L"\u6211\u4EEC\u4E00\u8D77\u5B66\u4E60", L"study"},
        {L"laoshibuzhilezuoye", L"\u8001\u5E08\u5E03\u7F6E\u4E86\u4F5C\u4E1A", L"study"},
        {L"diannaowufalianjiewangluo", L"\u7535\u8111\u65E0\u6CD5\u8FDE\u63A5\u7F51\u7EDC", L"computer"},
        {L"qingbaocunwenjian", L"\u8BF7\u4FDD\u5B58\u6587\u4EF6", L"computer"},
        {L"woxianghedianshui", L"\u6211\u60F3\u559D\u70B9\u6C34", L"life"},
        {L"wanfanxiangchishenme", L"\u665A\u996D\u60F3\u5403\u4EC0\u4E48", L"life"},
        {L"zhegeshezhihenhao", L"\u8FD9\u4E2A\u8BBE\u7F6E\u5F88\u597D", L"computer"},
        {L"keyibangwoyixiama", L"\u53EF\u4EE5\u5E2E\u6211\u4E00\u4E0B\u5417", L"request"},
        {L"wozaikanxinxi", L"\u6211\u5728\u770B\u4FE1\u606F", L"chat"},
        {L"zenmehuishi", L"\u600E\u4E48\u56DE\u4E8B", L"question"},
        {L"qingshaodengyixia", L"\u8BF7\u7A0D\u7B49\u4E00\u4E0B", L"request"},
        {L"womendezuoyewanchengle", L"\u6211\u4EEC\u7684\u4F5C\u4E1A\u5B8C\u6210\u4E86", L"study"},
    };

    bool all_passed = true;
    for (const auto& test_case : cases) {
        all_passed = expect_sentence(engine, test_case) && all_passed;
    }
    all_passed = expect_candidate_contains(engine, L"mian", L"\u514D") && all_passed;

    std::filesystem::remove_all(user_lexicon_root);
    return all_passed ? 0 : 1;
}
