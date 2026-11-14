#include "racing.h"
#include <algorithm>
#include <limits>

namespace ctg {
namespace {
constexpr int traffic_width = 16;
constexpr int traffic_height = 24;
}

int racing_opponent_x(const RacingRect& road, unsigned index) {
    constexpr int border = 4;
    const int left = road.x + border + racing_car_overhang;
    const int right = road.x + road.width - border - racing_car_overhang - traffic_width;
    const unsigned positions = static_cast<unsigned>(right - left + 1);
    return left + static_cast<int>((25 + (index % positions) * 80) % positions);
}

namespace {
bool overlaps(const RacingRect& a, const RacingRect& b) {
    return a.x < b.x + b.width && b.x < a.x + a.width &&
           a.y < b.y + b.height && b.y < a.y + a.height;
}
}

void Racing::reset() {
    state_ = {};
    next_lane_ = 0;
    steering_remainder_ = 0;
    for (unsigned i = 0; i < state_.traffic.size(); ++i) {
        state_.traffic[i] = {{racing_opponent_x(state_.road, next_lane_++), 40, traffic_width, traffic_height}, false};
    }
}

void Racing::reset_preview() {
    reset();

}

void Racing::reset_from_layout(const RacingState& layout) {
    const auto road_scroll = layout.road_scroll;
    const auto traffic = layout.traffic;
    const auto player = layout.player;
    reset();
    state_.road_scroll = road_scroll;
    state_.player = player;
    for (unsigned i = 0; i < state_.traffic.size(); ++i)
        state_.traffic[i].rect = traffic[i].rect;
}

void Racing::step_preview() {
    constexpr unsigned period = racing_road_dash_length + racing_road_gap_length;
    constexpr int distance = low_speed / frames_per_second;
    state_.road_scroll = (state_.road_scroll + period - distance) % period;
    for (auto& car : state_.traffic) {
        car.rect.y -= distance;
        if (car.rect.y + car.rect.height <= 0) car.rect.y = state_.road.height;
    }
    if (state_.frame < std::numeric_limits<unsigned>::max()) ++state_.frame;
}

unsigned Racing::step(RacingInput input) {
    if (state_.finished) return racing_none;

    auto& player = state_.player;
    const int left = state_.road.x + racing_car_overhang;
    const int right = state_.road.x + state_.road.width - player.width - racing_car_overhang;
    constexpr int steering_speed = 6;
    if (input.analog) {
        constexpr int deadzone = 8192, range = 32767 - deadzone;
        const int steering = std::clamp(input.steering, -32767, 32767);
        if (steering >= -deadzone && steering <= deadzone) steering_remainder_ = 0;
        else {
            const int deflection = steering > 0 ? steering - deadzone : steering + deadzone;
            steering_remainder_ += deflection * steering_speed;
            const int movement = steering_remainder_ / range;
            steering_remainder_ %= range;
            player.x = std::clamp(player.x + movement, left, right);
            if ((player.x == left && deflection < 0) || (player.x == right && deflection > 0))
                steering_remainder_ = 0;
        }
    } else {
        steering_remainder_ = 0;
        player.x = std::clamp(player.x + (static_cast<int>(input.right) -
                                          static_cast<int>(input.left)) * steering_speed, left, right);
    }

    if (input.select_high_speed != input.select_low_speed)
        state_.selected_speed = input.select_high_speed ? max_speed : low_speed;
    if (!input.recovering) {
        if (state_.speed < state_.selected_speed) ++state_.speed;
        else if (state_.speed > state_.selected_speed) --state_.speed;
    }
    state_.movement_remainder += state_.speed * 16;
    const unsigned distance = state_.movement_remainder / (frames_per_second * 9);
    state_.movement_remainder %= frames_per_second * 9;
    const bool section_complete = state_.road_scroll < static_cast<unsigned>(state_.road.height) &&
                                  state_.road_scroll + distance >= static_cast<unsigned>(state_.road.height);
    state_.road_scroll = (state_.road_scroll + distance) % (racing_road_dash_length + racing_road_gap_length);

    unsigned events = racing_none;
    for (auto& car : state_.traffic) {
        car.rect.y += static_cast<int>(distance);
        if (!car.collided && overlaps(player, car.rect)) {
            car.collided = true;
            state_.section_collided = true;
            if (state_.collisions < std::numeric_limits<unsigned>::max()) ++state_.collisions;
            state_.speed = start_speed;
            state_.movement_remainder = 0;
            events |= racing_collision;
        }
        if (car.rect.y >= state_.road.height) {
            car.rect = {racing_opponent_x(state_.road, next_lane_),
                        car.rect.y - static_cast<int>(racing_road_dash_length + racing_road_gap_length),
                        traffic_width, traffic_height};
            // Keep the placement sequence periodic without overflowing the lane counter.
            const unsigned positions = static_cast<unsigned>(state_.road.width - 8 - 2 * racing_car_overhang - traffic_width + 1);
            next_lane_ = (next_lane_ + 1) % positions;
            car.collided = false;
        }
    }
    if (section_complete) {
        if (!state_.section_collided) {
            if (state_.score < std::numeric_limits<unsigned>::max()) ++state_.score;
            events |= racing_pass;
        }
        state_.section_collided = false;
    }
    if (state_.frame < std::numeric_limits<unsigned>::max()) ++state_.frame;
    if (state_.frames_remaining > 0) --state_.frames_remaining;
    if (!endless_ && state_.frames_remaining == 0) {
        state_.finished = true;
        events |= racing_finished;
    }
    return events;
}
}
