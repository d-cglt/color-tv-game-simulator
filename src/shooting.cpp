#include "shooting.h"
#include "tennis_b.h"
#include <algorithm>
#include <limits>

namespace ctg {
void Shooting::clamp_shooter() {
    const int half_height = paddle_height(0) / 2;
    state_.shooter_y = std::clamp(state_.shooter_y, 24 - half_height, 216 + half_height);
}

void Shooting::set_options(bool high_speed, std::array<bool, 2> small_paddle) {
    high_speed_ = high_speed;
    small_paddle_ = small_paddle;
    clamp_shooter();
    target_position_ = std::clamp(target_position_, target_min_y() * 4, target_max_y() * 4 + 3);
    state_.target_y = target_position_ / 4;
}

unsigned Shooting::step(int direction, bool fire, int target_direction, bool automatic_target) {
    if (state_.finished) return sound_none;
    state_.shooter_y += std::clamp(direction, -1, 1) * 3;
    clamp_shooter();
    const int min_position = target_min_y() * 4;
    const int range = (target_max_y() - target_min_y() + 1) * 4;
    const int movement = automatic_target
        ? static_cast<int>(target_speed_ % static_cast<unsigned>(range)) * target_direction_
        : std::clamp(target_direction, -1, 1) * 12;
    target_position_ = (target_position_ - min_position + movement + range) % range + min_position;
    state_.target_y = target_position_ / 4;
    // Wrap on a blink-period boundary so visibility continues uninterrupted.
    state_.frame = state_.frame == std::numeric_limits<unsigned>::max() ? 0 : state_.frame + 1;
    unsigned events = sound_none;
    if (fire && !state_.flying) {
        state_.flying = true;
        state_.hit = false;
        state_.shot_y = state_.shooter_y;
        shot_position_ = 190;
        state_.shot_x = shot_position_ / 4;
        events |= sound_paddle | event_player1_shot;
    }
    if (state_.flying) {
        shot_position_ += high_speed_ ? 24 : 12;
        state_.shot_x = shot_position_ / 4;
        if (!state_.hit && target_visible() && state_.shot_x + 7 > 265 && state_.shot_x < 273 &&
            state_.shot_y + 2 > target_top() && state_.shot_y - 2 < target_top() + paddle_height(1)) {
            state_.hit = true;
            if (state_.hits < std::numeric_limits<unsigned>::max()) ++state_.hits;
            events |= sound_point;
        }
        if (state_.shot_x >= 320) {
            state_.flying = false;
            state_.hit = false;
            if (state_.shots < std::numeric_limits<unsigned>::max()) ++state_.shots;
            if (!endless_ && state_.shots >= shot_limit_) { state_.finished = true; events |= sound_win; }
        }
    }
    return events;
}
}
