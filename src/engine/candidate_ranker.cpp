#include "candidate_ranker.h"

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
    frequency_[key_for(pinyin, word)] = frequency;
}

void CandidateRanker::increment_frequency(const std::wstring& pinyin, const std::wstring& word) {
    ++frequency_[key_for(pinyin, word)];
}

std::vector<Candidate> CandidateRanker::rank(const std::wstring& pinyin, std::vector<Candidate> candidates) const {
    for (auto& candidate : candidates) {
        const auto it = frequency_.find(key_for(pinyin, candidate.text));
        if (it != frequency_.end()) {
            candidate.user_frequency = std::min(it->second * kUserLearningScoreUnit, kMaxUserLearningScore);
        }
    }

    std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) {
        return left.total_score() > right.total_score();
    });
    return candidates;
}

}  // namespace localpinyin
