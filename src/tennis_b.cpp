#include "tennis_b.h"
#include "hockey_ball.h"
#include "tennis_a_blocks.h"
#include <algorithm>
#include <cstdlib>
#include <cmath>

namespace ctg {
TennisB::TennisB(bool doubles, bool hockey, bool volleyball, bool tennis_a, bool hockey_a, bool volleyball_a, bool ping_pong) { state_.ping_pong = ping_pong; state_.doubles = doubles; state_.hockey = hockey; state_.volleyball = volleyball; state_.tennis_a = tennis_a; state_.hockey_a = hockey_a; state_.volleyball_a = volleyball_a; reset(); }


// Independent deterministic approximation, not the reference's random sequence.
void TennisB::hockey_serve() {
    // Map the reference's display coordinates through the current renderer;
    // clamp the top to the playable ball-center range.
    const auto next_random = [this]() {
        random_state_ = random_state_ * 1664525u + 1013904223u;
        return random_state_ >> 8;
    };
    state_.ball_x = missed_side_ == 0 ? 13 : 298;
    state_.ball_y = ball_radius + static_cast<int>(next_random() % 207u);
    state_.ball_vx = (missed_side_ == 0 ? 4 : -4) * (state_.high_speed ? 2 : 1);
    const bool downward = (next_random() % 2u) != 0;
    const int max_slope = downward && missed_side_ == 1 ? 6 : 4;
    // A horizontal integer trajectory outside the opening would never score.
    const int magnitude = 1 + static_cast<int>(next_random() % max_slope);
    state_.ball_vy = (downward ? magnitude : -magnitude) * (state_.high_speed ? 2 : 1);
}

int TennisB::paddle_rebound(int owner, int offset) {
    const int speed = state_.high_speed ? 2 : 1;
    int rebound;
    if (movement_rebound_ && paddle_displacement_[owner] != 0) {
        const int distance = std::abs(offset);
        const int magnitude = distance <= 3 ? 0 : distance <= 7 ? 1 : distance <= 14 ? 2 : 3;
        rebound = (paddle_displacement_[owner] < 0 ? -magnitude : magnitude) * speed;
    } else {
        rebound = (offset < -7 ? -3 : offset > 7 ? 3 : (state_.ball_vy < 0 ? -2 : 2)) * speed;
    }
    if (!anti_stalemate_ || rebound != 0) {
        horizontal_rebounds_ = 0;
    } else if (++horizontal_rebounds_ == 3) {
        horizontal_rebounds_ = 0;
        rebound = speed;
    }
    return rebound;
}

void TennisB::reset() {
    const bool doubles = state_.doubles, hockey = state_.hockey, volleyball = state_.volleyball, tennis_a = state_.tennis_a, hockey_a = state_.hockey_a, volleyball_a = state_.volleyball_a, ping_pong = state_.ping_pong;
    const bool high_speed = state_.high_speed;
    const auto small_paddle = state_.small_paddle;
    state_ = {};
    paddle_displacement_ = {};
    horizontal_rebounds_ = 0;
    state_.high_speed = high_speed;
    state_.small_paddle = small_paddle;
    state_.doubles = doubles;
    state_.hockey = hockey;
    state_.volleyball = volleyball;
    state_.volleyball_a = volleyball_a;
    state_.tennis_a = tennis_a;
    state_.hockey_a = hockey_a;
    state_.ping_pong = ping_pong;
    last_block_ = -1;
    block_collision_speed_ = 0;
    volleyball_block_bounces_ = 0;
    state_.winner = -1;
    state_.paddle_y = {height / 2, height / 2};
    random_state_ = 1;
    missed_side_ = 0;
    hockey_scoring_side_ = false;
    tennis_spawning_ = false;
    state_.serve_wait = hockey_serve_delay;
}

unsigned TennisB::step(TennisInput input) {
    if (state_.winner != -1) return sound_none;
    const int controls[] = {input.left, input.right};
    for (int i = 0; i < 2; ++i) {
        const int direction = (controls[i] > 0) - (controls[i] < 0);
        const int previous_y = state_.paddle_y[i];
        state_.paddle_y[i] = std::clamp(state_.paddle_y[i] + direction * paddle_speed,
                                        state_.hockey ? hockey_paddle_min : paddle_half(i),
                                        state_.hockey ? hockey_paddle_max : height - paddle_half(i));
        const int displacement = state_.paddle_y[i] - previous_y;
        if (displacement != 0) paddle_displacement_[i] = displacement;
    }

    if (state_.serve_wait) {
        horizontal_rebounds_ = 0;
        if (--state_.serve_wait == 0) {
            hockey_serve();
            tennis_spawning_ = !state_.hockey && !state_.volleyball;
        }
        return sound_none;
    }

    unsigned sounds = sound_none;
    const auto obstacle_vy = [this](int vy) {
        if (vy != state_.ball_vy) horizontal_rebounds_ = 0;
        state_.ball_vy = vy;
    };
    const auto score = [this, &sounds](int scorer) {
        horizontal_rebounds_ = 0;
        ++state_.score[scorer];
        volleyball_block_bounces_ = 0;
        hockey_scoring_side_ = false;
        sounds |= sound_point;
        if (state_.score[scorer] >= winning_score_) {
            state_.winner = scorer;
            state_.ball_vx = state_.ball_vy = 0;
            sounds |= sound_win;
        } else {
            missed_side_ = 1 - scorer;
            state_.ball_vx = state_.ball_vy = 0;
            state_.serve_wait = hockey_serve_delay;
            tennis_spawning_ = false;
        }
    };
    // The reference checks the ball's rectangle against the full display before movement.
    if (state_.hockey && (hockey_y(state_.ball_y) - 2 <= 5 || hockey_y(state_.ball_y) + 3 >= 240)) {
        score(0);
        return sounds;
    }
    if (tennis_spawning_ && hockey_x(state_.ball_x) >= 75 && hockey_x(state_.ball_x) <= 200)
        tennis_spawning_ = false;
    const int old_x = state_.ball_x;
    if (state_.hockey && (hockey_x(old_x) - 3 <= 35 || hockey_x(old_x) + 3 >= 285))
        hockey_scoring_side_ = true;
    // The reference tests the pre-movement center in its 1280-pixel display.
    // Compare against the same quantized center used by our 320-pixel renderer.
    if (state_.volleyball && !state_.volleyball_a && 635 <= 4 * hockey_x(old_x) && 4 * hockey_x(old_x) <= 645) {
        random_state_ = random_state_ * 1664525u + 1013904223u;
        if ((random_state_ >> 8) % 10u != 0) obstacle_vy(-state_.ball_vy);
        sounds |= sound_wall;
    }
    if (state_.tennis_a) {
        for (int color = 0; color < 2; ++color) {
            for (int row = 0; row < 6; ++row) {
                for (int col = 0; col < 2; ++col) {
                    const auto block = tennis_a_block(color, row, col);
                    const int x = hockey_x(state_.ball_x), y = hockey_y(state_.ball_y);
                    if (x + 4 <= block.x || x - 3 >= block.x + block.width ||
                        y + 3 <= block.y || y - 2 >= block.y + block.height) continue;
                    const int id = color * 12 + row * 2 + col;
                    if (last_block_ != id || block_collision_speed_ != state_.ball_vy) {
                        sounds |= sound_wall;
                        last_block_ = id;
                        block_collision_speed_ = state_.ball_vy;
                    }
                    if ((block.red && state_.ball_vy > 0) || (!block.red && state_.ball_vy < 0)) {
                        obstacle_vy(-state_.ball_vy);
                        // Place the rendered ball immediately outside the struck face.
                        if (block.red) {
                            while (hockey_y(state_.ball_y) + 3 > block.y) --state_.ball_y;
                        } else {
                            while (hockey_y(state_.ball_y) - 2 < block.y + block.height) ++state_.ball_y;
                        }
                    }
                }
            }
        }
    }
    if (state_.volleyball_a && state_.ball_vx != 0) {
        // Resolve the first crossed block face in travel order; the integer court
        // step can otherwise skip a thin block or reflect repeatedly inside it.
        int hit_face = state_.ball_vx > 0 ? width + 1 : -1;
        bool hit = false;
        TennisABlock hit_block{};
        for (int color = 0; color < 2; ++color)
            for (int row = 0; row < 6; ++row)
                for (int col = 0; col < 2; ++col) {
                    const auto block = tennis_a_block(color, row, col);
                    const int y = hockey_y(state_.ball_y);
                    if (y + 3 <= block.y || y - 2 >= block.y + block.height) continue;
                    int face = 0;
                    if (state_.ball_vx > 0) {
                        while (hockey_x(face) + 4 <= block.x) ++face;
                        if (state_.ball_x <= face && state_.ball_x + state_.ball_vx >= face && face < hit_face) {
                            hit_face = face; hit = true; hit_block = block;
                        }
                    } else {
                        face = width;
                        while (hockey_x(face) - 3 >= block.x + block.width) --face;
                        if (state_.ball_x >= face && state_.ball_x + state_.ball_vx <= face && face > hit_face) {
                            hit_face = face; hit = true; hit_block = block;
                        }
                    }
                }
        if (hit) {
            state_.ball_x = 2 * hit_face - state_.ball_x;
            const bool directed = (hit_block.red ? state_.ball_vx < 0 : state_.ball_vx > 0) && volleyball_block_bounces_ < 10;
            if (directed) {
                // Normalize contact by the reference's nominal 130-pixel paddle height.
                const double offset = std::clamp((4.0 * hockey_y(state_.ball_y) - (4.0 * hit_block.y + 12.5)) / 65.0, -1.0, 1.0);
                obstacle_vy(static_cast<int>(std::lround(std::abs(state_.ball_vx) * std::tan(offset * 50.0 * 3.14159265358979323846 / 180.0))));
            } else {
                random_state_ = random_state_ * 1664525u + 1013904223u;
                const unsigned choice = random_state_ >> 8;
                if (state_.ball_vy == 0) obstacle_vy((static_cast<int>(choice % 3u) - 1) * std::abs(state_.ball_vx));
                else if (choice % 2u == 0) obstacle_vy(-state_.ball_vy);
                else obstacle_vy(0);
            }
            ++volleyball_block_bounces_;
            state_.ball_vx = -state_.ball_vx;
            sounds |= sound_paddle;
        }
    }
    if (state_.ping_pong && state_.ball_vx != 0) {
        // The visible net is dashed, but its lower collision region is solid.
        int left_face = 0, right_face = width;
        while (hockey_x(left_face) + 3 < 156) ++left_face;
        while (hockey_x(right_face) - 3 >= 164) --right_face;
        const bool crosses = state_.ball_vx > 0 ? old_x < left_face && old_x + state_.ball_vx >= left_face
                                                : old_x > right_face && old_x + state_.ball_vx <= right_face;
        if (crosses && hockey_y(state_.ball_y) + 3 >= 155) {
            const int face = state_.ball_vx > 0 ? left_face : right_face;
            state_.ball_x = 2 * face - state_.ball_x;
            state_.ball_vx = -state_.ball_vx;
            sounds |= sound_wall;
        } else if (hockey_y(state_.ball_y) + 3 < 155 &&
                   ((hockey_x(old_x) <= 160 && hockey_x(old_x + state_.ball_vx) >= 160) ||
                    (hockey_x(old_x) >= 160 && hockey_x(old_x + state_.ball_vx) <= 160))) {
            if (state_.ball_vy < 0) obstacle_vy(-state_.ball_vy);
            sounds |= sound_wall;
        }
    }
    state_.ball_x += state_.ball_vx;
    state_.ball_y += state_.ball_vy;
    if (!hockey_scoring_side_ && (state_.ball_y < ball_radius || state_.ball_y > height - ball_radius)) {
        if (state_.ball_y < ball_radius) state_.ball_y = 2 * ball_radius - state_.ball_y;
        else state_.ball_y = 2 * (height - ball_radius) - state_.ball_y;
        obstacle_vy(-state_.ball_vy);
        sounds |= sound_wall;
    }


    if (state_.hockey_a) {
        // Collision region is continuous despite the dashed visual treatment.
        int left_face = 0, right_face = width;
        while (hockey_x(left_face) + 3 < 156) ++left_face;
        while (hockey_x(right_face) - 3 >= 164) --right_face;
        const bool crosses = state_.ball_vx > 0 ? old_x < left_face && state_.ball_x >= left_face
                                                : state_.ball_vx < 0 && old_x > right_face && state_.ball_x <= right_face;
        const int y = hockey_y(state_.ball_y);
        if (crosses && (state_.hockey_barrier_closed || y - 2 <= 68 || y + 3 >= 155)) {
            const int face = state_.ball_vx > 0 ? left_face : right_face;
            state_.ball_x = 2 * face - state_.ball_x;
            state_.ball_vx = -state_.ball_vx;
            state_.hockey_barrier_closed = false;
            sounds |= sound_wall;
        }
    }

    if (state_.doubles && state_.hockey) {
        for (int i = 0; i < 2; ++i) {
            const int paddle_x = i == 0 ? hockey_inner_left : hockey_inner_right;
            const int owner = 1 - i;
            // Find the first ball-center position touching each drawn face.
            int left_face = 0, right_face = width;
            while (hockey_x(left_face) + 3 < paddle_x) ++left_face;
            while (hockey_x(right_face) - 3 >= paddle_x + 8) --right_face;
            const int face = state_.ball_vx > 0 ? left_face : right_face;
            const bool crosses = state_.ball_vx > 0 ? old_x < face && state_.ball_x >= face
                                                    : state_.ball_vx < 0 && old_x > face && state_.ball_x <= face;
            const int top = std::max(hockey_court_top, hockey_paddle_top(state_.paddle_y[owner], paddle_pixels(owner)));
            const int bottom = std::min(hockey_court_bottom, hockey_paddle_top(state_.paddle_y[owner], paddle_pixels(owner)) + paddle_pixels(owner));
            const int ball_y = hockey_y(state_.ball_y);
            if (crosses && ball_y + 3 > top && ball_y - 2 < bottom) {
                const int offset = state_.ball_y - state_.paddle_y[owner];
                state_.ball_x = 2 * face - state_.ball_x;
                state_.ball_vx = -state_.ball_vx;
                state_.ball_vy = paddle_rebound(owner, offset);
                sounds |= sound_paddle;
                break;
            }
        }
    } else if (state_.doubles && !tennis_spawning_) {
        // Tennis inner paddles allow behind-side crossings without deflection.
        for (int i = 0; i < 2; ++i) {
            const int x = i == 0 ? inner_left : inner_right;
            const int face = x + (state_.ball_vx < 0 ? ball_radius : -ball_radius);
            const bool crosses = i == 0 ? state_.ball_vx < 0 && old_x >= face && state_.ball_x <= face
                                        : state_.ball_vx > 0 && old_x <= face && state_.ball_x >= face;
            const int offset = state_.ball_y - state_.paddle_y[i];
            const bool overlaps = state_.small_paddle[i] ? hockey_y(state_.ball_y) + 3 > paddle_top(i) && hockey_y(state_.ball_y) - 2 < paddle_top(i) + paddle_pixels(i)
                                                          : std::abs(offset) <= paddle_half_height + ball_radius;
            if (crosses && overlaps) {
                state_.ball_x = 2 * face - state_.ball_x;
                state_.ball_vx = -state_.ball_vx;
                state_.ball_vy = paddle_rebound(i, offset);
                if (state_.volleyball_a) volleyball_block_bounces_ = 0;
                sounds |= sound_paddle;
                break;
            }
        }
    }

    for (int i = 0; i < 2; ++i) {
        // Hockey faces correspond to the drawn paddle and seven-pixel ball edges.
        const int face = state_.hockey ? (i == 0 ? 27 : 293)
                                       : (i == 0 ? paddle_left + ball_radius : paddle_right - ball_radius);
        const bool crosses = i == 0 ? state_.ball_vx < 0 && old_x >= face && state_.ball_x <= face
                                    : state_.ball_vx > 0 && old_x <= face && state_.ball_x >= face;
        const int offset = state_.ball_y - state_.paddle_y[i];
        const int top = std::max(hockey_court_top, hockey_paddle_top(state_.paddle_y[i], paddle_pixels(i)));
        const int bottom = std::min(hockey_court_bottom, hockey_paddle_top(state_.paddle_y[i], paddle_pixels(i)) + paddle_pixels(i));
        const int ball_y = hockey_y(state_.ball_y);
        const bool overlaps = state_.hockey ? ball_y + 3 > top && ball_y - 2 < bottom
                            : state_.small_paddle[i] ? ball_y + 3 > paddle_top(i) && ball_y - 2 < paddle_top(i) + paddle_pixels(i)
                                                     : std::abs(offset) <= paddle_half_height + ball_radius;
        if (!tennis_spawning_ && crosses && overlaps) {
            state_.ball_x = 2 * face - state_.ball_x;
            state_.ball_vx = -state_.ball_vx;
            state_.ball_vy = paddle_rebound(i, offset);
            if (state_.volleyball_a) volleyball_block_bounces_ = 0;
            sounds |= sound_paddle;
        }
    }

    if (state_.hockey && !hockey_scoring_side_ && (state_.ball_y - ball_radius < hockey_goal_top ||
                          state_.ball_y + ball_radius > hockey_goal_bottom)) {
        const int left = hockey_wall_left + ball_radius, right = hockey_wall_right - ball_radius;
        if (state_.ball_vx < 0 && old_x >= left && state_.ball_x <= left) {
            state_.ball_x = 2 * left - state_.ball_x;
            state_.ball_vx = -state_.ball_vx;
            if (state_.hockey_a) state_.hockey_barrier_closed = true;
            sounds |= sound_wall;
        } else if (state_.ball_vx > 0 && old_x <= right && state_.ball_x >= right) {
            state_.ball_x = 2 * right - state_.ball_x;
            state_.ball_vx = -state_.ball_vx;
            if (state_.hockey_a) state_.hockey_barrier_closed = true;
            sounds |= sound_wall;
        }
    }

    if (state_.ball_x < 0 || state_.ball_x > width) {
        score(state_.ball_x < 0 ? 1 : 0);
    }
    return sounds;
}
}
