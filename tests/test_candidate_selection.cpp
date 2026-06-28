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
    return 0;
}
