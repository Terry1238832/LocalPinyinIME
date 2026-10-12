#pragma once

#include <cstddef>
#include <string>
#include <algorithm>
#include <limits>

namespace localpinyin {

inline constexpr size_t kMaxCandidateCount = 9;

struct Candidate {
    std::wstring text;
    int base_score = 0;
    int user_frequency = 0;
    int exact_match_bonus = 0;
    int complete_segmentation_bonus = 0;
    int segmentation_penalty = 0;
    // Zero consumes the entire composition. Prefix candidates specify a raw
    // character count (including apostrophes) so the TSF host retains the rest.
    size_t consumed_pinyin = 0;
    // Reference unigram prior used by sentence decoding; the visible default
    // word order remains governed by the project/user base_score.
    int decoding_frequency = 0;

    [[nodiscard]] int total_score() const noexcept {
        const long long score = static_cast<long long>(base_score) + user_frequency + exact_match_bonus + complete_segmentation_bonus - segmentation_penalty;
        return static_cast<int>(std::clamp(score, static_cast<long long>(std::numeric_limits<int>::min()), static_cast<long long>(std::numeric_limits<int>::max())));
    }
};

}  // namespace localpinyin
