#include "racing.h"
#include <algorithm>
#include <limits>

namespace ctg {
namespace {
constexpr int traffic_width = racing_car_width;
constexpr int traffic_height = racing_car_height;

int player_center(const RacingState& state) {
    const int right_edge = state.narrow ? racing_x(racing_center_column) + racing_cell_width :
                           state.road.x + state.road.width;
    return state.road.x + (right_edge - state.road.x - state.player.width) / 2;
}

int player_right(const RacingState& state) {
    return (state.narrow ? racing_x(racing_center_column) + racing_cell_width :
            state.road.x + state.road.width) - state.player.width;
}


void clamp_narrow_layout(RacingState& state) {
    state.player.x = std::clamp(state.player.x, state.road.x, player_right(state));
    for (auto& car : state.traffic)
        car.rect.x = std::clamp(car.rect.x, state.road.x,
                                racing_x(racing_center_column) + racing_cell_width - car.rect.width);
}

bool overlaps(const RacingRect& a, const RacingRect& b) {
    return a.x < b.x + b.width && b.x < a.x + a.width &&
           a.y < b.y + b.height && b.y < a.y + a.height;
}
}

int Racing::opponent_x(bool second) {
    if (two_players_ && second)
        return state_.traffic[0].rect.x + racing_center_column * racing_cell_width;
    const int divider = racing_x(racing_center_column) + racing_cell_width;
    const int left = two_players_ ? (second ? racing_x(racing_center_column) : state_.road.x) :
                     narrow_ ? state_.road.x : state_.road.x + racing_road_border_width;
    const int right = two_players_ ? (second ? state_.road.x + state_.road.width : divider) - traffic_width :
                      narrow_ ? divider - traffic_width :
                      state_.road.x + state_.road.width - racing_road_border_width - traffic_width;
    return std::uniform_int_distribution<int>(left, right)(random_);
}

void Racing::reset() {
    state_ = {};
    state_.two_players = two_players_;
    state_.narrow = narrow_ && !two_players_;
    if (state_.narrow) clamp_narrow_layout(state_);
    state_.player.x = two_players_ ? state_.road.x + (racing_x(racing_center_column) + racing_cell_width - state_.road.x - state_.player.width) / 2 : player_center(state_);
    if (two_players_) {
        const int left = racing_x(racing_center_column);
        state_.player2.x = left + (state_.road.x + state_.road.width - left - state_.player2.width) / 2;
        state_.selected_speed = high_gear_speed_;
    }
    steering_remainder_ = steering_remainder2_ = 0;
    for (auto& car : state_.traffic)
        car = {{opponent_x(), racing_opponent_gap_offset, traffic_width, traffic_height}, false};
    if (two_players_)
        for (auto& car : state_.traffic2)
            car = {{opponent_x(true), racing_opponent_gap_offset, traffic_width, traffic_height}, false};
}

void Racing::set_speed_switch(unsigned setting) {
    high_gear_speed_ = setting == 0 ? low_speed : setting == 1 ? medium_speed : max_speed;
    if (two_players_) state_.selected_speed = high_gear_speed_;
}

void Racing::set_narrow(bool enabled) {
    const bool changed = narrow_ != enabled;
    narrow_ = enabled;
    if (two_players_) return;
    state_.narrow = enabled;
    if (enabled) clamp_narrow_layout(state_);
    if (changed) state_.player.x = player_center(state_);
    steering_remainder_ = 0;
}

void Racing::set_two_players(bool enabled) {
    if (two_players_ == enabled) return;
    two_players_ = enabled;
    reset();
}

void Racing::reset_preview() {
    reset();

}

void Racing::reset_from_layout(const RacingState& layout) {
    const auto road_scroll = layout.road_scroll;
    const auto traffic = layout.traffic;
    const auto player = layout.player;
    const auto player2 = layout.player2;
    const auto traffic2 = layout.traffic2;
    reset();
    state_.road_scroll = road_scroll;
    state_.player = player;
    for (unsigned i = 0; i < state_.traffic.size(); ++i)
        state_.traffic[i].rect = traffic[i].rect;
    if (two_players_) {
        const int divider = racing_x(racing_center_column) + racing_cell_width;
        state_.player.x = std::clamp(state_.player.x, state_.road.x, divider - state_.player.width);
        state_.player2 = player2;
        state_.player2.x = std::clamp(state_.player2.x, racing_x(racing_center_column), state_.road.x + state_.road.width - state_.player2.width);
        for (unsigned i = 0; i < state_.traffic.size(); ++i)
            state_.traffic[i].rect.x = std::clamp(state_.traffic[i].rect.x, state_.road.x, divider - state_.traffic[i].rect.width);
        for (unsigned i = 0; i < state_.traffic2.size(); ++i) {
            state_.traffic2[i].rect = traffic2[i].rect;
            state_.traffic2[i].rect.x = state_.traffic[i].rect.x + racing_center_column * racing_cell_width;
        }
    } else if (narrow_) clamp_narrow_layout(state_);
}

void Racing::step_preview(RacingInput input, RacingInput player2) {
    steer(input);
    if (two_players_) steer(player2, true);
    constexpr unsigned period = racing_road_dash_length + racing_road_gap_length;
    constexpr int distance = low_speed / frames_per_second;
    state_.road_scroll = (state_.road_scroll + period - distance) % period;
    for (auto& car : state_.traffic) {
        car.rect.y -= distance;
        if (car.rect.y + car.rect.height <= 0) {
            car.rect.y += static_cast<int>(period);
            car.rect.x = opponent_x();
        }
    }
    if (two_players_) {
        for (auto& car : state_.traffic2) {
            car.rect.y -= distance;
            if (car.rect.y + car.rect.height <= 0) {
                car.rect.y += static_cast<int>(period);
                car.rect.x = opponent_x(true);
            }
        }
    }
    if (state_.frame < std::numeric_limits<unsigned>::max()) ++state_.frame;
}

void Racing::steer(RacingInput input, bool second) {
    auto& player = second ? state_.player2 : state_.player;
    auto& steering_remainder = second ? steering_remainder2_ : steering_remainder_;
    const int divider = racing_x(racing_center_column) + racing_cell_width;
    const int left = second ? racing_x(racing_center_column) : state_.road.x;
    const int right = two_players_ ? (second ? state_.road.x + state_.road.width : divider) - player.width : player_right(state_);
    constexpr int steering_speed = 6;
    if (input.analog) {
        constexpr int deadzone = 8192, range = 32767 - deadzone;
        const int steering = std::clamp(input.steering, -32767, 32767);
        if (steering >= -deadzone && steering <= deadzone) steering_remainder = 0;
        else {
            const int deflection = steering > 0 ? steering - deadzone : steering + deadzone;
            steering_remainder += deflection * steering_speed;
            const int movement = steering_remainder / range;
            steering_remainder %= range;
            player.x = std::clamp(player.x + movement, left, right);
            if ((player.x == left && deflection < 0) || (player.x == right && deflection > 0))
                steering_remainder = 0;
        }
    } else {
        steering_remainder = 0;
        player.x = std::clamp(player.x + (static_cast<int>(input.right) -
                                          static_cast<int>(input.left)) * steering_speed, left, right);
    }

}

unsigned Racing::step(RacingInput input, RacingInput player2) {
    if (state_.finished) return racing_none;
    steer(input);
    if (two_players_) steer(player2, true);

    if (!two_players_ && input.select_high_speed != input.select_low_speed) {
        state_.selected_speed = input.select_high_speed ? high_gear_speed_ : low_speed;
    }
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

    const auto advance_blink = [](unsigned& remaining) {
        remaining = remaining > racing_blink_frame_samples ? remaining - racing_blink_frame_samples : 0;
    };
    if (two_players_) {
        advance_blink(state_.blink_samples);
        advance_blink(state_.blink_samples2);
    }
    unsigned events = racing_none;
    const auto advance_traffic = [&](auto& traffic, const RacingRect& player, bool second, unsigned& collisions, bool& section_collided) {
    for (auto& car : traffic) {
        car.rect.y += static_cast<int>(distance);
        if (car_collision_ && !car.collided && overlaps(player, car.rect)) {
            car.collided = true;
            section_collided = true;
            if (collisions < std::numeric_limits<unsigned>::max()) ++collisions;
            if (two_players_)
                (second ? state_.blink_samples2 : state_.blink_samples) = racing_blink_total_samples;
            state_.speed = start_speed;
            state_.movement_remainder = 0;
            events |= racing_collision;
        }
        if (car.rect.y >= state_.road.height) {
            car.rect = {opponent_x(second), car.rect.y - static_cast<int>(racing_road_dash_length + racing_road_gap_length),
                        traffic_width, traffic_height};
            car.collided = false;
        }
    }
    };
    advance_traffic(state_.traffic, state_.player, false, state_.collisions, state_.section_collided);
    if (two_players_)
        advance_traffic(state_.traffic2, state_.player2, true, state_.collisions2, state_.section_collided2);
    if (section_complete) {
        const auto score_section = [&](unsigned& score, bool& collided) {
            if (!collided) {
                if (score < std::numeric_limits<unsigned>::max()) ++score;
                events |= racing_pass;
            }
            collided = false;
        };
        score_section(state_.score, state_.section_collided);
        if (two_players_) score_section(state_.score2, state_.section_collided2);
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
