#include "candidate_ranker.h"
#include "user_learning.h"

#include <algorithm>

namespace localpinyin {
namespace {

constexpr int kUserLearningScoreUnit = 1200;
constexpr int kMaxUserLearningScore = 6000;

std::wstring key_for(const std::wstring& pinyin, const std::wstring& word) {
    return pinyin + L"\x1f" + word;
}

}  // namespace

void CandidateRanker::set_frequency(const std::wstring& pinyin, const std::wstring& word, int frequency) {
    frequency_[key_for(pinyin, word)] = std::clamp(frequency, 0, 100000);
}

void CandidateRanker::increment_frequency(const std::wstring& pinyin, const std::wstring& word) {
    auto& count = frequency_[key_for(pinyin, word)];
    count = std::min(count + 1, 100000);
}

void CandidateRanker::apply_learning(const std::wstring& pinyin, std::vector<Candidate>& candidates,
                                     const UserLearning* learning) const {
    for (auto& candidate : candidates) {
        int frequency = learning ? learning->frequency(pinyin, candidate.text) : 0;
        const auto it = frequency_.find(key_for(pinyin, candidate.text));
        if (it != frequency_.end()) {
            frequency = std::max(frequency, it->second);
        }
        candidate.user_frequency = std::min(frequency, kMaxUserLearningScore / kUserLearningScoreUnit) * kUserLearningScoreUnit;
    }
}

std::vector<Candidate> CandidateRanker::rank(const std::wstring& pinyin, std::vector<Candidate> candidates,
                                          const UserLearning* learning) const {
    apply_learning(pinyin, candidates, learning);

    std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) {
        return left.total_score() > right.total_score();
    });
    return candidates;
}

}  // namespace localpinyin
