#include "test_common.h"
#include "../src/engine/user_learning.h"
#include "../src/engine/candidate_ranker.h"

#include <windows.h>
#include <fstream>
#include <thread>
#include <atomic>
#include <ctime>
#include <vector>

int main() {
    wchar_t temp_path[MAX_PATH]{};
    wchar_t file_path[MAX_PATH]{};
    GetTempPathW(MAX_PATH, temp_path);
    GetTempFileNameW(temp_path, L"lpi", 0, file_path);

    {
        localpinyin::UserLearning learning;
        REQUIRE_TRUE(learning.open(file_path));
        REQUIRE_TRUE(learning.record_selection(L"nihao", L"\u4F60\u597D"));
        REQUIRE_TRUE(learning.record_selection(L"nihao", L"\u4F60\u597D"));
        REQUIRE_EQ(learning.frequency(L"nihao", L"\u4F60\u597D"), 2);
        REQUIRE_TRUE(learning.clear());
        REQUIRE_EQ(learning.frequency(L"nihao", L"\u4F60\u597D"), 0);
        REQUIRE_TRUE(learning.record_selection(L"nihao", L"\u4F60\u597D"));
    }
    {
        localpinyin::UserLearning learning;
        REQUIRE_TRUE(learning.open(file_path));
        REQUIRE_EQ(learning.frequency(L"nihao", L"\u4F60\u597D"), 1);
    }

    // A truncated or manually edited record must never crash an IME process.
    {
        std::ofstream file(std::filesystem::path(file_path), std::ios::binary | std::ios::trunc);
        file << "nihao\tvalid\t3\t42\n"
             << "nihao\tbad\tnot-a-number\t42\n"
             << "nihao\tnegative\t-7\t42\n"
             << "nihao\toverflow\t99999999999999999\t42\n"
             << "nihao\tbadtime\t7\tnonsense\n"
             << "malformed\n";
    }
    localpinyin::UserLearning first;
    localpinyin::UserLearning second;
    REQUIRE_TRUE(first.open(file_path));
    REQUIRE_TRUE(second.open(file_path));
    REQUIRE_EQ(first.frequency(L"nihao", L"valid"), 3);
    // A record from before stability was saved: as if two of its three
    // choices came after a break (3 days x 2.5 x 2.5).
    REQUIRE_EQ(first.stability(L"nihao", L"valid"), 1620000LL);
    REQUIRE_EQ(first.frequency(L"nihao", L"bad"), 0);
    REQUIRE_EQ(first.frequency(L"nihao", L"negative"), 0);
    REQUIRE_EQ(first.frequency(L"nihao", L"badtime"), 0);
    REQUIRE_TRUE(!first.record_selection(L"nihao", L"bad\trow"));
    REQUIRE_TRUE(first.record_selection(L"nihao", L"valid"));
    REQUIRE_TRUE(second.record_selection(L"nihao", L"valid"));
    first.refresh_if_changed();
    REQUIRE_EQ(first.frequency(L"nihao", L"valid"), 5);

    // Concurrent IME instances merge under the same per-file lock.
    std::atomic<bool> writes_ok{true};
    const auto record_many = [&](localpinyin::UserLearning& learning) {
        for (int index = 0; index < 20; ++index) {
            if (!learning.record_selection(L"nihao", L"valid")) writes_ok = false;
        }
    };
    std::thread first_thread([&] { record_many(first); });
    std::thread second_thread([&] { record_many(second); });
    first_thread.join();
    second_thread.join();
    REQUIRE_TRUE(writes_ok);
    first.refresh_if_changed();
    REQUIRE_EQ(first.frequency(L"nihao", L"valid"), 45);
    REQUIRE_TRUE(second.clear());
    first.refresh_if_changed();
    REQUIRE_EQ(first.frequency(L"nihao", L"valid"), 0);

    // A chosen word fades: first for 3 days, a place lower for each further
    // period, as if never chosen after four. Chosen again in the same
    // sitting it is only renewed; after a break it also lasts 2.5 times as
    // long. The period survives a restart.
    {
        constexpr long long day = 24LL * 3600;
        const long long start = 1800000000LL;
        const std::wstring pinyin = L"liangjiacheng", word = L"量甲成";
        localpinyin::UserLearning learning;
        REQUIRE_TRUE(learning.open(file_path));
        REQUIRE_TRUE(learning.clear());
        REQUIRE_EQ(learning.faded_steps(pinyin, word, start), -1);
        REQUIRE_TRUE(learning.record_selection(pinyin, word, start));
        REQUIRE_EQ(learning.stability(pinyin, word), 3 * day);
        REQUIRE_EQ(learning.faded_steps(pinyin, word, start + 2 * day), 0);
        REQUIRE_EQ(learning.faded_steps(pinyin, word, start + 4 * day), 1);
        REQUIRE_EQ(learning.faded_steps(pinyin, word, start + 11 * day), 3);
        REQUIRE_EQ(learning.faded_steps(pinyin, word, start + 12 * day), -1);
        REQUIRE_TRUE(learning.record_selection(pinyin, word, start + 3600));
        REQUIRE_EQ(learning.stability(pinyin, word), 3 * day);
        const long long again = start + 5 * day;
        REQUIRE_TRUE(learning.record_selection(pinyin, word, again));
        REQUIRE_EQ(learning.stability(pinyin, word), 3 * day * 5 / 2);
        REQUIRE_EQ(learning.faded_steps(pinyin, word, again + 7 * day), 0);
        REQUIRE_EQ(learning.faded_steps(pinyin, word, again + 8 * day), 1);
        REQUIRE_EQ(learning.faded_steps(pinyin, word, again + 29 * day), 3);
        REQUIRE_EQ(learning.faded_steps(pinyin, word, again + 30 * day), -1);
        REQUIRE_TRUE(learning.record_selection(pinyin, word, again + 20 * day));
        REQUIRE_EQ(learning.stability(pinyin, word), 3 * day * 25 / 4);
        localpinyin::UserLearning reopened;
        REQUIRE_TRUE(reopened.open(file_path));
        REQUIRE_EQ(reopened.stability(pinyin, word), 3 * day * 25 / 4);
        REQUIRE_EQ(reopened.frequency(pinyin, word), 4);
        // Half a year at most.
        for (int pick = 1; pick <= 10; ++pick)
            REQUIRE_TRUE(learning.record_selection(L"nihao", L"你好", start + pick * 200 * day));
        REQUIRE_EQ(learning.stability(L"nihao", L"你好"), localpinyin::UserLearning::kMaxStabilitySeconds);
    }

    // The ranker places a fading choice a place lower per period, never
    // below where it would rank anyway, and a faded one where it ranks.
    {
        constexpr long long day = 24LL * 3600;
        const long long now = static_cast<long long>(std::time(nullptr));
        const std::wstring liang = L"两", bright = L"亮", measure = L"量", cool = L"凉";
        const auto order = [&](long long chosen_at) {
            localpinyin::UserLearning learning;
            learning.open(file_path);
            learning.clear();
            learning.record_selection(L"liang", measure, chosen_at);
            std::vector<localpinyin::Candidate> candidates(4);
            const std::wstring words[] = {liang, bright, measure, cool};
            const int scores[] = {9000, 8800, 7000, 6000};
            for (size_t index = 0; index < 4; ++index) {
                candidates[index].text = words[index];
                candidates[index].base_score = scores[index];
            }
            localpinyin::CandidateRanker ranker;
            const auto ranked = ranker.rank(L"liang", candidates, &learning);
            std::vector<std::wstring> texts;
            for (const auto& candidate : ranked) texts.push_back(candidate.text);
            return texts;
        };
        REQUIRE_TRUE(order(now - 60) == std::vector<std::wstring>({measure, liang, bright, cool}));
        REQUIRE_TRUE(order(now - 4 * day) == std::vector<std::wstring>({liang, measure, bright, cool}));
        REQUIRE_TRUE(order(now - 7 * day) == std::vector<std::wstring>({liang, bright, measure, cool}));
        REQUIRE_TRUE(order(now - 10 * day) == std::vector<std::wstring>({liang, bright, measure, cool}));
        REQUIRE_TRUE(order(now - 13 * day) == std::vector<std::wstring>({liang, bright, measure, cool}));
    }

    DeleteFileW(file_path);
    return 0;
}
