#include "demo.h"
#include "game_catalog.h"
#include <utility>

namespace ctg {
void Demo::resolve_shooting_settings(const DemoOptions& options) {
    const unsigned target = options.shooting_target == 4
        ? std::uniform_int_distribution<unsigned>(1, 3)(shooting_random_)
        : options.shooting_target < 4 ? options.shooting_target : 2;
    const unsigned direction = options.shooting_target != 0 && options.shooting_direction == 2
        ? std::uniform_int_distribution<unsigned>(0, 1)(shooting_random_) : options.shooting_direction;
    const bool small = options.shooting_size == 2
        ? std::uniform_int_distribution<unsigned>(0, 1)(shooting_random_) != 0 : options.shooting_size == 1;
    const bool high = options.shooting_speed == 2
        ? std::uniform_int_distribution<unsigned>(0, 1)(shooting_random_) != 0 : options.shooting_speed == 1;
    const unsigned speeds[] = {11, 6, 11, 16};
    shooting.set_options(high, {!high, small});
    shooting.set_target_behavior(speeds[target], direction == 1 ? -1 : 1, true, direction == 3);
}

unsigned Demo::step(unsigned id, bool rebound, bool anti, const DemoOptions& options) {
    const bool changed_game = id_ != id;
    if (changed_game) {
        id_ = id;
        frame_ = 0;
        tennis = make_rally_game(id);
        tennis.set_rebound_mode(rebound);
        tennis.set_anti_stalemate(anti);
        racing.set_two_players(options.racing_mode == 2);
        racing.set_narrow(options.racing_mode == 1);
        racing.reset_preview();
        shooting = ctg::Shooting{};
        shooting_random_.seed(1);
        for (auto& ai : opponents_) ai.reset();
    }
    if (id == 15) {
        if (!changed_game && options.racing_mode != options_.racing_mode) {
            racing.set_two_players(options.racing_mode == 2);
            racing.set_narrow(options.racing_mode == 1);
        }
    } else if (id == 14) {
        if (changed_game || options.shooting_target != options_.shooting_target ||
            options.shooting_direction != options_.shooting_direction ||
            options.shooting_size != options_.shooting_size || options.shooting_speed != options_.shooting_speed) {
            resolve_shooting_settings(options);
        }
    } else {
        tennis.set_options(options.high_speed, options.small_paddle);
    }
    options_ = options;
    unsigned events = 0;
    if (id == 15) {
        racing.step_preview();
    } else if (id == 14) {
        const auto& state = shooting.state();
        if (state.finished) {
            resolve_shooting_settings(options);
            shooting.reset();
        }
        const int delta = shooting.target_top() + shooting.paddle_height(1) / 2 - shooting.state().shooter_y;
        const int target_direction = (frame_ / 60) % 2 == 0 ? 1 : -1;
        events = shooting.step((delta > 3) - (delta < -3), frame_ % 45 == 0,
                               target_direction, options.shooting_target != 0, false);
        if (events & sound_paddle) resolve_shooting_settings(options);
    } else {
        auto mirrored = tennis.state();
        mirrored.ball_x = ctg::TennisB::width - mirrored.ball_x;
        mirrored.ball_vx = -mirrored.ball_vx;
        std::swap(mirrored.paddle_y[0], mirrored.paddle_y[1]);
        std::swap(mirrored.small_paddle[0], mirrored.small_paddle[1]);
        const unsigned difficulty = (frame_ / 180) % 2 == 0 ? 3 : 1;
        const int left = opponents_[0].step(mirrored, difficulty);
        const int right = opponents_[1].step(tennis.state(), difficulty);
        events = tennis.step({left, right, {false, false}});
        if (events & ctg::sound_point) {
            tennis.reset();
            for (auto& ai : opponents_) ai.reset();
        }
    }
    events &= sound_mask;
    ++frame_;
    return events;
}
}
