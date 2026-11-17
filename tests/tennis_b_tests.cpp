#include "tennis_test_harness.h"

int main() {
    for (int variant = 0; variant < 8; ++variant) for (bool small : {false, true}) {
        ctg::TennisB game(variant & 1, variant & 2, variant & 4, false, false, false, variant == 0);
        game.set_options(false, {small, small});
        const auto start = game.state().paddle_y;
        game.step({-1, 1});
        CHECK(game.state().paddle_y[0] == start[0] - 8);
        CHECK(game.state().paddle_y[1] == start[1] + 8);
        game.step({1, -1, {false, false}});
        CHECK(game.state().paddle_y[0] == start[0] - 4);
        CHECK(game.state().paddle_y[1] == start[1] + 4);
        for (int i = 0; i < 100; ++i) game.step({-1, 1});
        CHECK(game.state().paddle_y[0] == game.paddle_min(0));
        CHECK(game.state().paddle_y[1] == game.paddle_max(1));
        for (int i = 0; i < 100; ++i) game.step({1, -1});
        CHECK(game.state().paddle_y[0] == game.paddle_max(0));
        CHECK(game.state().paddle_y[1] == game.paddle_min(1));
    }
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
