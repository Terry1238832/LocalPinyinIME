#include "test_common.h"
#include "../src/engine/neural_model.h"
#include "../src/engine/pinyin_engine.h"
#include "../src/common/utf_utils.h"

#include <array>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::filesystem::path executable_directory() {
    std::array<wchar_t, MAX_PATH> path{};
    const DWORD size = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    return size == 0 || size >= path.size() ? std::filesystem::current_path()
                                            : std::filesystem::path(path.data()).parent_path();
}

}  // namespace

// The packaged neural character model prefers fluent sentences, scores texts
// sharing a beginning exactly as it scores them alone, and rescores the
// sentence candidates of the engine.
int main() {
    const auto root = use_temp_user_lexicon_override();
    const auto resources = executable_directory();

    localpinyin::NeuralModel model;
    REQUIRE_TRUE(model.load((resources / L"dictionary" / L"neural_zh.lm").wstring()));
    REQUIRE_TRUE(!model.empty());
    const std::vector<std::wstring> texts = {L"我们一起去吃饭", L"我门一起去迟饭", L"我们一起", L"他把牛奶倒到杯子里了",
                                             L"他把牛奶倒到被子里了"};
    const auto together = model.score(L"", texts);
    REQUIRE_TRUE(together[0] > together[1]);
    REQUIRE_TRUE(together[3] > together[4]);
    REQUIRE_TRUE(together[2] < 0 && together[2] > together[0]);
    for (size_t i = 0; i < texts.size(); ++i) {
        const auto alone = model.score(L"", {texts[i]});
        REQUIRE_TRUE(std::abs(alone[0] - together[i]) < 1e-6);
    }
    // The clause before the text conditions it.
    REQUIRE_TRUE(model.score(L"打开", {L"窗户"})[0] > model.score(L"", {L"窗户"})[0]);

    localpinyin::NeuralModel missing;
    REQUIRE_TRUE(!missing.load((root / L"absent_neural_zh.lm").wstring()));
    REQUIRE_TRUE(missing.score(L"", {L"你好"})[0] == 0.0);

    localpinyin::PinyinEngine engine(localpinyin::DictionaryLoadMode::Empty);
    REQUIRE_TRUE(engine.load_dictionary_resource_directory(resources.wstring(), (root / L"user_lexicon.tsv").wstring()));
    const auto candidates = engine.lookup(L"zhejiadianshizhuanmenmaichufangyongpinde", L"");
    REQUIRE_TRUE(!candidates.empty());
    REQUIRE_EQ(candidates.front().text, std::wstring(L"这家店是专门卖厨房用品的"));

    std::filesystem::remove_all(root);
    return 0;
}
