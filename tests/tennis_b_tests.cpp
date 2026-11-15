#include "tennis_test_harness.h"

int main() {
    if (tennis_tests::paddle_limits_and_visible_misses() != 0) return 1;
    if (tennis_tests::rebound_history_and_stalemate() != 0) return 1;
    if (tennis_tests::winning_scores_and_paddle_options() != 0) return 1;
    if (tennis_tests::ping_pong_singles() != 0) return 1;
    if (tennis_tests::variant_block_collisions() != 0) return 1;
    if (tennis_tests::tennis_lifecycle_and_doubles() != 0) return 1;
    if (tennis_tests::hockey_a_barrier() != 0) return 1;
    if (tennis_tests::hockey_lifecycle_and_scoring() != 0) return 1;
    if (tennis_tests::hockey_paddle_geometry() != 0) return 1;
    if (tennis_tests::volleyball_singles() != 0) return 1;
    if (tennis_tests::ping_pong_doubles() != 0) return 1;
    if (tennis_tests::volleyball_doubles() != 0) return 1;
    return 0;
}
