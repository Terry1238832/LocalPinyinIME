#pragma once

#include <cstddef>

namespace localpinyin {

enum class CompositionCommitAction {
    PassThrough,
    CommitSelectedCandidate,
    CommitRawComposition,
};

inline constexpr int candidate_index_from_digit_key(unsigned long key) noexcept {
    return key >= '1' && key <= '9' ? static_cast<int>(key - '1') : -1;
}

inline constexpr bool can_select_candidate_by_digit(unsigned long key,
                                                    size_t candidate_count,
                                                    bool has_composition,
                                                    bool candidate_window_visible) noexcept {
    const int index = candidate_index_from_digit_key(key);
    return has_composition && candidate_window_visible && index >= 0 &&
           static_cast<size_t>(index) < candidate_count;
}

inline constexpr CompositionCommitAction composition_commit_action_for_key(unsigned long key,
                                                                          size_t candidate_count,
                                                                          bool has_composition) noexcept {
    if (!has_composition || candidate_count == 0) {
        return CompositionCommitAction::PassThrough;
    }
    if (key == ' ') {
        return CompositionCommitAction::CommitSelectedCandidate;
    }
    if (key == 0x0D) {
        return CompositionCommitAction::CommitRawComposition;
    }
    return CompositionCommitAction::PassThrough;
}

inline constexpr bool should_eat_composition_commit_key(unsigned long key,
                                                        size_t candidate_count,
                                                        bool has_composition) noexcept {
    return composition_commit_action_for_key(key, candidate_count, has_composition) !=
           CompositionCommitAction::PassThrough;
}

inline constexpr int candidate_commit_index_for_key(unsigned long key,
                                                    size_t candidate_count,
                                                    bool has_composition,
                                                    size_t selected_index) noexcept {
    switch (composition_commit_action_for_key(key, candidate_count, has_composition)) {
        case CompositionCommitAction::CommitSelectedCandidate:
            return candidate_count == 0 ? -1 : static_cast<int>(selected_index < candidate_count ? selected_index : 0);
        case CompositionCommitAction::CommitRawComposition:
        case CompositionCommitAction::PassThrough:
        default:
            return -1;
    }
}

}  // namespace localpinyin
