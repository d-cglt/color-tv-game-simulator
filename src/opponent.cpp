#include "opponent.h"
#include "hockey_ball.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>

namespace ctg {
namespace {

int predict_y(const TennisState& state, int frames) {
    // Fold the unbounded path into the interval between ball-center walls.
    constexpr std::int64_t low = TennisB::ball_radius;
    constexpr std::int64_t span = TennisB::height - 2 * low;
    std::int64_t y = (static_cast<std::int64_t>(state.ball_y) - low +
                      static_cast<std::int64_t>(state.ball_vy) * frames) % (2 * span);
    if (y < 0) y += 2 * span;
    return static_cast<int>(low + (y <= span ? y : 2 * span - y));
}

int intercept(const TennisState& state) {
    if (state.ball_vx == 0) return TennisB::height / 2;
    const bool right = state.ball_vx > 0;
    int face = right ? (state.hockey ? hockey_paddle_face(hockey_outer_right, true) : TennisB::paddle_right - TennisB::ball_radius) : -1;
    if (state.doubles) {
        const int inner = state.hockey ? hockey_paddle_face(hockey_inner_left, right)
                                       : TennisB::inner_right - TennisB::ball_radius;
        if ((state.hockey || right) && (right ? state.ball_x <= inner : state.ball_x >= inner))
            face = inner;
    }
    if (face < 0 || (right ? state.ball_x > face : state.ball_x < face))
        return TennisB::height / 2;
    const std::int64_t distance = std::abs(static_cast<std::int64_t>(face) - state.ball_x);
    const std::int64_t speed = std::abs(static_cast<std::int64_t>(state.ball_vx));
    const int frames = static_cast<int>(std::min<std::int64_t>((distance + speed - 1) / speed, 10000));
    return predict_y(state, frames);
}
}

void Opponent::reset() {
    countdown_ = 0;
    difficulty_ = 0;
    hockey_easy_ = false;
    random_ = 1;
    target_ = TennisB::height / 2;
}

int Opponent::step(const TennisState& state, unsigned difficulty, bool explicitly_selected_easy) {
    difficulty = std::clamp(difficulty, 1u, 3u);
    if (state.winner != -1) return 0;
    const bool hockey_easy = state.hockey && difficulty == 1 && explicitly_selected_easy;
    if (difficulty != difficulty_ || hockey_easy != hockey_easy_) {
        difficulty_ = difficulty;
        hockey_easy_ = hockey_easy;
        countdown_ = 0;
    }
    if (countdown_ == 0) {
        static constexpr unsigned intervals[] = {12, 7, 3};
        static constexpr int errors[] = {30, 16, 6};
        countdown_ = intervals[difficulty - 1];
        target_ = state.serve_wait ? TennisB::height / 2 : intercept(state);
        if (!state.serve_wait && errors[difficulty - 1]) {
            random_ = random_ * 1664525u + 1013904223u;
            const int error = hockey_easy ? 70 : errors[difficulty - 1];
            target_ += static_cast<int>((random_ >> 8) % (2 * error + 1)) - error;
        }
        target_ = std::clamp(target_, TennisB::paddle_min_for_size(state.small_paddle[1]),
                             TennisB::paddle_max_for_size(state.small_paddle[1]));
    }
    --countdown_;
    const int delta = target_ - state.paddle_y[1];
    // A dead zone avoids oscillating around targets between movement increments.
    return delta > TennisB::paddle_speed / 2 ? 1 : delta < -TennisB::paddle_speed / 2 ? -1 : 0;
}
}
