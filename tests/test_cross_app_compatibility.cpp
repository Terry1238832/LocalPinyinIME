#include "ime/candidate_ui.h"
#include "ime/input_mode.h"

#include <windows.h>

#include <cstdlib>
#include <iostream>

#define REQUIRE_TRUE(value) do { if (!(value)) { std::cerr << "requirement failed: " #value "\n"; return 1; } } while (0)
#define REQUIRE_EQ(actual, expected) REQUIRE_TRUE((actual) == (expected))

int main() {
    using namespace localpinyin;

    REQUIRE_TRUE(!should_eat_candidate_navigation_key(VK_LEFT, false, 0));
    REQUIRE_TRUE(!should_eat_candidate_navigation_key(VK_RIGHT, false, 0));
    REQUIRE_TRUE(!should_eat_candidate_navigation_key(VK_LEFT, true, 0));
    REQUIRE_TRUE(should_eat_candidate_navigation_key(VK_LEFT, true, 1));
    REQUIRE_TRUE(should_eat_candidate_navigation_key(VK_RIGHT, true, 9));
    REQUIRE_TRUE(!should_eat_candidate_navigation_key(VK_LEFT, true, 1, false));

    REQUIRE_TRUE(!transient_context_changed(0, 2, true));
    REQUIRE_TRUE(!transient_context_changed(1, 1, true));
    REQUIRE_TRUE(!transient_context_changed(1, 2, false));
    REQUIRE_TRUE(transient_context_changed(1, 2, true));

    const auto retry1 = decide_candidate_layout_retry(true, false, 0);
    REQUIRE_TRUE(retry1.schedule);
    REQUIRE_EQ(retry1.next_attempt, 1U);
    REQUIRE_EQ(retry1.delay_ms, 50U);
    const auto retry3 = decide_candidate_layout_retry(true, false, 2);
    REQUIRE_TRUE(retry3.schedule);
    REQUIRE_EQ(retry3.next_attempt, 3U);
    REQUIRE_EQ(retry3.delay_ms, 300U);
    REQUIRE_TRUE(!decide_candidate_layout_retry(true, false, 3).schedule);
    REQUIRE_TRUE(!decide_candidate_layout_retry(true, true, 0).schedule);
    REQUIRE_TRUE(!decide_candidate_layout_retry(false, false, 0).schedule);

    return 0;
}
