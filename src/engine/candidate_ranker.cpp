#include "candidate_ranker.h"
#include "user_learning.h"

#include <algorithm>
#include <ctime>

namespace localpinyin {
namespace {

constexpr int kUserLearningScoreUnit = 1200;
constexpr int kMaxUserLearningScore = 6000;
constexpr int kOutOfContextDivisor = 4;

bool is_single_character(const std::wstring& text) noexcept {
    return text.size() == 1 || (text.size() == 2 && text[0] >= 0xD800 && text[0] <= 0xDBFF);
}

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
    const long long now = static_cast<long long>(std::time(nullptr));
    for (auto& candidate : candidates) {
        int frequency = 0;
        int faded = 0;
        if (learning) {
            // A fading choice weighs less, a faded one not at all.
            faded = learning->faded_steps(pinyin, candidate.text, now);
            if (faded >= 0) frequency = learning->frequency(pinyin, candidate.text);
        }
        const auto it = frequency_.find(key_for(pinyin, candidate.text));
        if (it != frequency_.end() && it->second > frequency) {
            frequency = it->second;
            faded = 0;
        }
        const int score = std::min(frequency, kMaxUserLearningScore / kUserLearningScoreUnit) * kUserLearningScoreUnit;
        candidate.user_frequency = faded < 0 ? 0 : score * (UserLearning::kFadeSteps - faded) / UserLearning::kFadeSteps;
    }
}

std::vector<Candidate> CandidateRanker::rank(const std::wstring& pinyin, std::vector<Candidate> candidates,
                                          const UserLearning* learning, wchar_t before) const {
    apply_learning(pinyin, candidates, learning);
    const auto in_context = before ? UserLearning::contextual_key(pinyin, before) : std::wstring();
    const long long now = static_cast<long long>(std::time(nullptr));
    if (before && learning) {
        // After text, a single character chosen elsewhere keeps a quarter of
        // its bonus: enough to break a near tie, not to override the text.
        for (auto& candidate : candidates) {
            if (is_single_character(candidate.text) && learning->faded_steps(in_context, candidate.text, now) < 0)
                candidate.user_frequency /= kOutOfContextDivisor;
        }
    }
    // A word the user chose for exactly this input is placed by how much of
    // it is remembered, whatever its frequency or how it was found
    // (sentence, completion or spelling correction): first while fresh, a
    // place lower for each period it has faded, never below where it would
    // rank anyway. A frequency bonus alone cannot overtake a far more
    // common homophone, so placement is by position.
    std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) {
        return left.total_score() > right.total_score();
    });
    struct Learned { size_t natural = 0; size_t place = 0; long long time = 0; int count = 0; };
    std::vector<std::pair<Learned, Candidate>> learned;
    std::vector<Candidate> others;
    others.reserve(candidates.size());
    for (size_t index = 0; index < candidates.size(); ++index) {
        Learned entry;
        entry.natural = index;
        int faded = -1;
        bool chosen_here = false;
        if (learning) {
            if (before) {
                faded = learning->faded_steps(in_context, candidates[index].text, now);
                chosen_here = faded >= 0;
            }
            const auto& key = chosen_here ? in_context : pinyin;
            if (!chosen_here) faded = learning->faded_steps(pinyin, candidates[index].text, now);
            if (faded >= 0) {
                entry.count = learning->frequency(key, candidates[index].text);
                entry.time = learning->last_selected(key, candidates[index].text);
            }
        }
        const auto found = frequency_.find(key_for(pinyin, candidates[index].text));
        if (found != frequency_.end() && found->second > 0) {
            // Chosen this session, before it could be saved.
            faded = 0;
            entry.count = std::max(entry.count, found->second);
        }
        if (faded < 0 || (before && is_single_character(candidates[index].text) && !chosen_here)) {
            others.push_back(std::move(candidates[index]));
            continue;
        }
        entry.place = std::min(static_cast<size_t>(faded), index);
        learned.emplace_back(entry, std::move(candidates[index]));
    }
    std::stable_sort(learned.begin(), learned.end(), [](const auto& left, const auto& right) {
        if (left.first.place != right.first.place) return left.first.place < right.first.place;
        if (left.first.time != right.first.time) return left.first.time > right.first.time;
        if (left.first.count != right.first.count) return left.first.count > right.first.count;
        return left.first.natural < right.first.natural;
    });
    size_t next = 0;
    for (auto& [entry, candidate] : learned) {
        const size_t at = std::min(std::max(entry.place, next), others.size());
        others.insert(others.begin() + static_cast<std::ptrdiff_t>(at), std::move(candidate));
        next = at + 1;
    }
    return others;
}

}  // namespace localpinyin
