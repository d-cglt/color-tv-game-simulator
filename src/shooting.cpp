#include "shooting.h"
#include "tennis_b.h"
#include <algorithm>

namespace ctg {
unsigned Shooting::step(int direction, bool fire, int target_direction, bool automatic_target) {
    if (state_.finished) return sound_none;
    state_.shooter_y = std::clamp(state_.shooter_y + std::clamp(direction, -1, 1) * 3, 24, 211);
    if (automatic_target) {
        target_position_ += 11;
        if (target_position_ >= 875) target_position_ = -65;
    } else {
        target_position_ += std::clamp(target_direction, -1, 1) * 12;
        if (target_position_ >= 875) target_position_ -= 940;
        if (target_position_ < -65) target_position_ += 940;
    }
    state_.target_y = target_position_ / 4;
    ++state_.frame;
    unsigned events = sound_none;
    if (fire && !state_.flying) {
        state_.flying = true;
        state_.hit = false;
        state_.shot_y = state_.shooter_y;
        shot_position_ = 190;
        state_.shot_x = shot_position_ / 4;
        events |= sound_paddle;
    }
    if (state_.flying) {
        shot_position_ += high_speed_ ? 24 : 12;
        state_.shot_x = shot_position_ / 4;
        if (!state_.hit && target_visible() && state_.shot_x + 7 > 265 && state_.shot_x < 273 &&
            state_.shot_y + 2 > target_top() && state_.shot_y - 2 < target_top() + paddle_height(1)) {
            state_.hit = true;
            ++state_.hits;
            events |= sound_point;
        }
        if (state_.shot_x >= 320) {
            state_.flying = false;
            state_.hit = false;
            ++state_.shots;
            if (state_.shots >= shot_limit_) { state_.finished = true; events |= sound_win; }
        }
    }
    return events;
}
}
