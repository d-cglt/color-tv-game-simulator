#include "demo.h"
#include "game_catalog.h"
#include <utility>

namespace ctg {
unsigned Demo::step(unsigned id, bool rebound, bool anti) {
    if (id_ != id) {
        id_ = id;
        frame_ = 0;
        tennis = make_rally_game(id);
        tennis.set_rebound_mode(rebound);
        tennis.set_anti_stalemate(anti);
        racing.reset_preview();
        shooting = ctg::Shooting{};
        shooting.set_target_behavior(11, 1, true);
        for (auto& ai : opponents_) ai.reset();
    }
    unsigned events = 0;
    if (id == 15) {
        racing.step_preview();
    } else if (id == 14) {
        const auto& state = shooting.state();
        if (state.finished) shooting.reset();
        const int delta = shooting.state().target_y + 16 - shooting.state().shooter_y;
        events = shooting.step((delta > 3) - (delta < -3), frame_ % 45 == 0, 0, true);
    } else {
        auto mirrored = tennis.state();
        mirrored.ball_x = ctg::TennisB::width - mirrored.ball_x;
        mirrored.ball_vx = -mirrored.ball_vx;
        std::swap(mirrored.paddle_y[0], mirrored.paddle_y[1]);
        std::swap(mirrored.small_paddle[0], mirrored.small_paddle[1]);
        const unsigned difficulty = (frame_ / 180) % 2 == 0 ? 3 : 1;
        const int left = opponents_[0].step(mirrored, difficulty);
        const int right = opponents_[1].step(tennis.state(), difficulty);
        events = tennis.step({left, right});
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
