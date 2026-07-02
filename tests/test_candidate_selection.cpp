#include "test_common.h"
#include "../src/engine/candidate_selection.h"

int main() {
    REQUIRE_EQ(localpinyin::candidate_index_from_digit_key('1'), 0);
    REQUIRE_EQ(localpinyin::candidate_index_from_digit_key('9'), 8);
    REQUIRE_EQ(localpinyin::candidate_index_from_digit_key('0'), -1);

    REQUIRE_TRUE(localpinyin::can_select_candidate_by_digit('3', 5, true, true));
    REQUIRE_TRUE(!localpinyin::can_select_candidate_by_digit('6', 5, true, true));
    REQUIRE_TRUE(!localpinyin::can_select_candidate_by_digit('1', 5, false, true));
    REQUIRE_TRUE(!localpinyin::can_select_candidate_by_digit('1', 5, true, false));

    REQUIRE_EQ(localpinyin::composition_commit_action_for_key(' ', 3, true),
               localpinyin::CompositionCommitAction::CommitSelectedCandidate);
    REQUIRE_EQ(localpinyin::composition_commit_action_for_key(0x0D, 3, true),
               localpinyin::CompositionCommitAction::CommitFirstCandidate);
    REQUIRE_EQ(localpinyin::composition_commit_action_for_key(0x0D, 0, true),
               localpinyin::CompositionCommitAction::PassThrough);
    REQUIRE_EQ(localpinyin::composition_commit_action_for_key(0x0D, 3, false),
               localpinyin::CompositionCommitAction::PassThrough);
    REQUIRE_TRUE(localpinyin::should_eat_composition_commit_key(' ', 3, true));
    REQUIRE_TRUE(localpinyin::should_eat_composition_commit_key(0x0D, 3, true));
    REQUIRE_TRUE(!localpinyin::should_eat_composition_commit_key(0x0D, 0, true));
    REQUIRE_TRUE(!localpinyin::should_eat_composition_commit_key(0x0D, 3, false));
    REQUIRE_EQ(localpinyin::candidate_commit_index_for_key(' ', 5, true, 2), 2);
    REQUIRE_EQ(localpinyin::candidate_commit_index_for_key(' ', 5, true, 99), 0);
    REQUIRE_EQ(localpinyin::candidate_commit_index_for_key(0x0D, 5, true, 2), 0);
    REQUIRE_EQ(localpinyin::candidate_commit_index_for_key(0x0D, 0, true, 0), -1);
    REQUIRE_EQ(localpinyin::candidate_commit_index_for_key(0x0D, 5, false, 0), -1);
    return 0;
}
