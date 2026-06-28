#pragma once

#include <cstddef>
#include <string>

namespace localpinyin {

inline constexpr size_t kMaxCandidateCount = 9;

struct Candidate {
    std::wstring text;
    int base_score = 0;
    int user_frequency = 0;
    int exact_match_bonus = 0;
    int complete_segmentation_bonus = 0;
    int segmentation_penalty = 0;

    [[nodiscard]] int total_score() const noexcept {
        return base_score + user_frequency + exact_match_bonus + complete_segmentation_bonus - segmentation_penalty;
    }
};

}  // namespace localpinyin
