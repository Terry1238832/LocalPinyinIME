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
    // A word the user chose for exactly this input goes first, the most
    // recently chosen one ahead of others, whatever its frequency or how it
    // was found (sentence, completion or spelling correction). A frequency
    // bonus alone cannot overtake a far more common homophone.
    struct Selection { long long time = 0; int count = 0; };
    std::unordered_map<std::wstring, Selection> selections;
    for (const auto& candidate : candidates) {
        Selection selection;
        if (learning) {
            selection.count = learning->frequency(pinyin, candidate.text);
            selection.time = learning->last_selected(pinyin, candidate.text);
        }
        const auto found = frequency_.find(key_for(pinyin, candidate.text));
        if (found != frequency_.end()) selection.count = std::max(selection.count, found->second);
        if (selection.count > 0) selections.emplace(candidate.text, selection);
    }
    std::stable_sort(candidates.begin(), candidates.end(), [&](const Candidate& left, const Candidate& right) {
        const auto l = selections.find(left.text);
        const auto r = selections.find(right.text);
        const bool left_learned = l != selections.end(), right_learned = r != selections.end();
        if (left_learned != right_learned) return left_learned;
        if (left_learned) {
            if (l->second.time != r->second.time) return l->second.time > r->second.time;
            if (l->second.count != r->second.count) return l->second.count > r->second.count;
        }
        return left.total_score() > right.total_score();
    });
    return candidates;
}

}  // namespace localpinyin
