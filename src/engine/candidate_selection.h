#pragma once

#include <cstddef>

namespace localpinyin {

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

}  // namespace localpinyin
