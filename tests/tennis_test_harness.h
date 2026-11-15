#pragma once

#include "tennis_b.h"
#include <cstdio>

inline void check_failure(const char* file, int line, const char* expression) {
    std::fprintf(stderr, "%s:%d: %s\n", file, line, expression);
}

#define CHECK(x) do { if (!(x)) { check_failure(__FILE__, __LINE__, #x); return 1; } } while (0)

inline bool same(const ctg::TennisState& a, const ctg::TennisState& b) {
    return a.ball_x == b.ball_x && a.ball_y == b.ball_y &&
           a.ball_vx == b.ball_vx && a.ball_vy == b.ball_vy &&
           a.paddle_y == b.paddle_y && a.doubles == b.doubles && a.hockey == b.hockey && a.volleyball == b.volleyball && a.volleyball_a == b.volleyball_a && a.tennis_a == b.tennis_a && a.ping_pong == b.ping_pong &&
           a.score == b.score && a.winner == b.winner && a.serve_wait == b.serve_wait;
}

namespace tennis_tests {
int paddle_limits_and_visible_misses();
int rebound_history_and_stalemate();
int winning_scores_and_paddle_options();
int ping_pong_singles();
int variant_block_collisions();
int tennis_lifecycle_and_doubles();
int hockey_a_barrier();
int hockey_lifecycle_and_scoring();
int hockey_paddle_geometry();
int volleyball_singles();
int ping_pong_doubles();
int volleyball_doubles();
} // namespace tennis_tests
