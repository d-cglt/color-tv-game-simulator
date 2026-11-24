#include "racing.h"
#include <algorithm>
#include <limits>
#include <vector>

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
    return a.x + racing_cell_width < b.x + b.width - racing_cell_width &&
           b.x + racing_cell_width < a.x + a.width - racing_cell_width &&
           a.y < b.y + b.height && b.y < a.y + a.height;
}

int wall_overlap(const RacingState& state, const RacingRect& player, bool second) {
    if (player.y >= state.road.y + state.road.height || player.y + player.height <= state.road.y)
        return 0;
    const int left = second ? racing_x(racing_center_column) + racing_cell_width :
                              state.road.x + racing_road_border_width;
    const int right = (second || (!state.two_players && !state.narrow)) ?
                      state.road.x + state.road.width - racing_road_border_width :
                      racing_x(racing_center_column);
    if (player.x < left) return 1;
    if (player.x + player.width > right) return -1;
    return 0;
}
}

int Racing::opponent_x() {
    const int divider = racing_x(racing_center_column) + racing_cell_width;
    const bool paired = two_players_ || (more_cars_ && !narrow_);
    const int left = paired || narrow_ ? state_.road.x : state_.road.x + racing_road_border_width;
    const int right = paired || narrow_ ? divider - traffic_width :
                      state_.road.x + state_.road.width - racing_road_border_width - traffic_width;
    return std::uniform_int_distribution<int>(left, right)(random_);
}

void Racing::arrange_extra_traffic() {
    if (!more_cars_) return;
    constexpr int offset = racing_center_column * racing_cell_width;
    constexpr int half_period = (racing_road_dash_length + racing_road_gap_length) / 2;
    auto& first = state_.traffic[0];
    auto& extra = state_.traffic[1];
    if (!two_players_ && !narrow_) {
        first.rect.x = std::clamp(first.rect.x, state_.road.x,
                                  racing_x(racing_center_column) + racing_cell_width - first.rect.width);
        extra = first;
        extra.rect.x += offset;
    } else {
        extra = {{opponent_x(), first.rect.y - half_period, traffic_width, traffic_height}, false};
    }
}

bool racing_dirt_at(const RacingState& state, int y) {
    if (!state.bad_road) return false;
    constexpr std::int64_t period = racing_road_dash_length + racing_road_gap_length;
    const auto floor_div = [](std::int64_t a, std::int64_t b) {
        return a / b - (a % b < 0 ? 1 : 0);
    };
    const auto section = floor_div(static_cast<std::int64_t>(y) - state.road_position - (racing_road_dash_length - 3 * racing_cell_height), period);
    // Cache a deterministic renewal schedule in both directions from an origin
    // stretch. Indexing by stretch keeps queries independent of rendering order.
    struct Stretch { std::int64_t start, end; };
    struct Schedule { unsigned seed = 0; bool initialized = false; std::vector<Stretch> forward, backward; };
    static thread_local Schedule schedule;
    const auto hash = [&](std::int64_t index) {
        std::uint32_t h = static_cast<std::uint32_t>(index) ^ state.road_seed;
        h ^= h >> 16; h *= 0x7feb352dU; h ^= h >> 15; h *= 0x846ca68bU; h ^= h >> 16;
        return h;
    };
    if (!schedule.initialized || schedule.seed != state.road_seed) {
        schedule = {}; schedule.initialized = true; schedule.seed = state.road_seed;
        schedule.forward.push_back({0, 5 + (hash(0) & 1)});
    }
    if (section >= 0) {
        auto& runs = schedule.forward;
        while (section >= runs.back().end) {
            const auto h = hash(static_cast<std::int64_t>(runs.size()));
            const auto start = runs.back().end + 5 + h % 8;
            runs.push_back({start, start + 5 + ((h >> 8) & 1)});
        }
        const auto it = std::upper_bound(runs.begin(), runs.end(), section,
            [](std::int64_t value, const Stretch& run) { return value < run.start; });
        return it != runs.begin() && section < (it - 1)->end;
    }
    auto& runs = schedule.backward;
    while (runs.empty() || section < runs.back().start) {
        const auto h = hash(-1 - static_cast<std::int64_t>(runs.size()));
        const auto end = (runs.empty() ? 0 : runs.back().start) - 5 - h % 8;
        runs.push_back({end - 5 - ((h >> 8) & 1), end});
    }
    const auto it = std::lower_bound(runs.begin(), runs.end(), section,
        [](const Stretch& run, std::int64_t value) { return run.start > value; });
    return it != runs.end() && section < it->end;
}

void Racing::reset() {
    const bool wall_crash = state_.wall_crash;
    state_ = {};
    state_.wall_crash = wall_crash;
    state_.two_players = two_players_;
    state_.more_cars = more_cars_;
    state_.zigzag = zigzag_;
    state_.bad_road = bad_road_;
    if (bad_road_) state_.road_seed = road_random_();
    wiggle_offset_ = wiggle_offset2_ = 0;
    wiggle_frame_ = wiggle_frame2_ = 0;
    state_.narrow = narrow_ && !two_players_;
    if (state_.narrow) clamp_narrow_layout(state_);
    state_.player.x = two_players_ ? state_.road.x + (racing_x(racing_center_column) + racing_cell_width - state_.road.x - state_.player.width) / 2 : player_center(state_);
    if (two_players_) {
        const int left = racing_x(racing_center_column);
        state_.player2.x = left + (state_.road.x + state_.road.width - left - state_.player2.width) / 2;
        state_.selected_speed = high_gear_speed_;
    }
    steering_remainder_ = steering_remainder2_ = 0;
    state_.traffic[0] = {{opponent_x(), racing_opponent_gap_offset, traffic_width, traffic_height}, false};
    arrange_extra_traffic();
    if (two_players_) {
        const unsigned count = more_cars_ ? 2 : 1;
        for (unsigned i = 0; i < count; ++i) {
            state_.traffic2[i] = state_.traffic[i];
            state_.traffic2[i].rect.x += racing_center_column * racing_cell_width;
        }
    }
    if (zigzag_) randomize_paths();
}

void Racing::set_speed_switch(unsigned setting) {
    high_gear_speed_ = setting == 0 ? low_speed : setting == 1 ? medium_speed : max_speed;
    if (two_players_) state_.selected_speed = high_gear_speed_;
}

void Racing::set_narrow(bool enabled) {
    const bool changed = narrow_ != enabled;
    narrow_ = enabled;
    if (two_players_) return;
    if (changed) remove_wiggle();
    state_.narrow = enabled;
    if (enabled) clamp_narrow_layout(state_);
    if (changed) {
        state_.player.x = player_center(state_);
        arrange_extra_traffic();
        if (zigzag_) {
            randomize_paths();
        }
    }
    steering_remainder_ = 0;
}

void Racing::set_two_players(bool enabled) {
    if (two_players_ == enabled) return;
    two_players_ = enabled;
    reset();
}

void Racing::set_more_cars(bool enabled) {
    if (more_cars_ == enabled) return;
    more_cars_ = enabled;
    reset();
}

void Racing::set_zigzag(bool enabled) {
    if (zigzag_ == enabled) return;
    zigzag_ = enabled;
    reset();
}

void Racing::set_bad_road(bool enabled) {
    if (bad_road_ == enabled) return;
    remove_wiggle();
    bad_road_ = state_.bad_road = enabled;
    wiggle_frame_ = wiggle_frame2_ = 0;
    if (enabled) {
        state_.road_position = state_.road_scroll;
        state_.road_seed = road_random_();
    }
}

void Racing::randomize_path(unsigned index) {
    const int limit = (narrow_ || two_players_ || more_cars_ ? 20 : 47) * racing_cell_width;
    const int position = std::uniform_int_distribution<int>(0, limit)(random_);
    (index ? state_.extra_path_position : state_.path_position) = position * static_cast<int>(frames_per_second);
    (index ? state_.extra_path_direction : state_.path_direction) = position == limit ? -1 : 1;
}

void Racing::randomize_paths() {
    randomize_path(0);
    if (more_cars_ && (narrow_ || two_players_)) randomize_path(1);
    apply_path();
}

void Racing::apply_path() {
    const unsigned count = more_cars_ ? 2 : 1;
    for (unsigned i = 0; i < count; ++i) {
        const int position = i == 1 && (narrow_ || two_players_) ? state_.extra_path_position : state_.path_position;
        const int x = racing_x(0) + position / static_cast<int>(frames_per_second);
        state_.traffic[i].rect.x = x + ((!two_players_ && !narrow_ && more_cars_ && i == 1)
            ? racing_center_column * racing_cell_width : 0);
        if (two_players_) state_.traffic2[i].rect.x = x + racing_center_column * racing_cell_width;
    }
}

void Racing::advance_path() {
    if (!zigzag_) return;
    const bool paired = narrow_ || two_players_ || more_cars_;
    constexpr int scale = static_cast<int>(frames_per_second);
    const int limit = (paired ? 20 : 47) * racing_cell_width * scale;
    const unsigned paths = more_cars_ && (narrow_ || two_players_) ? 2 : 1;
    for (unsigned index = 0; index < paths; ++index) {
        int& position = index ? state_.extra_path_position : state_.path_position;
        int& path_direction = index ? state_.extra_path_direction : state_.path_direction;
        int remaining = (paired ? 20 : 47) * racing_cell_width;
        while (remaining > 0) {
            const int direction = path_direction;
            int target = direction > 0 ? limit : 0;
            const int points[] = {6 * racing_cell_width * scale, (paired ? 2 : 34) * racing_cell_width * scale};
            for (int i = 0; i < 2; ++i) {
                if (i == 0 && direction > 0) continue;
                const int point = points[i];
                if ((direction > 0 && point > position && point < target) ||
                    (direction < 0 && point < position && point > target)) target = point;
            }
            const int distance = direction > 0 ? target - position : position - target;
            const int travel = std::min(distance, remaining);
            position += direction * travel;
            remaining -= travel;
            if (travel == distance) {
                if (target == 0 || target == limit || std::uniform_int_distribution<int>(0, 1)(random_))
                    path_direction = -direction;
            }
        }
    }
    apply_path();
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
    if (bad_road_) state_.road_position = road_scroll;
    if (bad_road_ && layout.bad_road) {
        state_.road_position = layout.road_position;
        state_.road_seed = layout.road_seed;
    }
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
            state_.traffic2[i].rect.y = state_.traffic[i].rect.y;
        }
    } else if (narrow_) clamp_narrow_layout(state_);
    else if (more_cars_) {
        state_.traffic[0].rect.x = std::clamp(state_.traffic[0].rect.x, state_.road.x,
                                               racing_x(racing_center_column) + racing_cell_width - state_.traffic[0].rect.width);
        state_.traffic[1].rect.x = state_.traffic[0].rect.x + racing_center_column * racing_cell_width;
        state_.traffic[1].rect.y = state_.traffic[0].rect.y;
    }
    if (zigzag_) {
        const int limit = (narrow_ || two_players_ || more_cars_ ? 20 : 47) * racing_cell_width * static_cast<int>(frames_per_second);
        if (layout.zigzag) {
            state_.path_position = std::clamp(layout.path_position, 0, limit);
            state_.path_direction = layout.path_direction < 0 ? -1 : 1;
            state_.extra_path_position = std::clamp(layout.extra_path_position, 0, limit);
            state_.extra_path_direction = layout.extra_path_direction < 0 ? -1 : 1;
        }
        if (state_.path_position == limit) state_.path_direction = -1;
        if (state_.extra_path_position == limit) state_.extra_path_direction = -1;
        apply_path();
    }
}

void Racing::step_preview(RacingInput input, RacingInput player2) {
    remove_wiggle();
    steer(input);
    if (two_players_) steer(player2, true);
    constexpr unsigned period = racing_road_dash_length + racing_road_gap_length;
    constexpr int distance = low_speed / frames_per_second;
    state_.road_scroll = (state_.road_scroll + period - distance) % period;
    if (bad_road_) state_.road_position -= distance;
    apply_wiggle();
    const unsigned count = more_cars_ ? 2 : 1;
    for (unsigned i = 0; i < count; ++i) {
        auto& car = state_.traffic[i];
        car.rect.y -= distance;
        if (car.rect.y + car.rect.height <= 0) {
            car.rect.y += static_cast<int>(period);
            if (zigzag_ && (i == 0 || narrow_ || two_players_)) randomize_path(i);
            if (!zigzag_) car.rect.x = more_cars_ && !two_players_ && !narrow_ && i == 1
                ? state_.traffic[0].rect.x + racing_center_column * racing_cell_width : opponent_x();
        }
        if (two_players_) {
            state_.traffic2[i].rect.y = car.rect.y;
            state_.traffic2[i].rect.x = car.rect.x + racing_center_column * racing_cell_width;
        }
    }
    advance_path();
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

RacingState Racing::unwiggled_state() const {
    auto layout = state_;
    layout.player.x -= wiggle_offset_;
    layout.player2.x -= wiggle_offset2_;
    return layout;
}

void Racing::remove_wiggle() {
    state_.player.x -= wiggle_offset_;
    state_.player2.x -= wiggle_offset2_;
    wiggle_offset_ = wiggle_offset2_ = 0;
}

unsigned Racing::apply_wiggle() {
    unsigned events = racing_none;
    const auto apply = [&](RacingRect& player, bool second, unsigned& frame, int& offset) {
        if (!racing_dirt_at(state_, player.y + player.height / 2)) { frame = 0; return; }
        const int left = second ? racing_x(racing_center_column) : state_.road.x;
        if (frame != 0 && frame % 8 == 0 && player.x > left)
            events |= second ? racing_wiggle_player2 : racing_wiggle_player1;
        if ((frame / 8) % 2 != 0) {
            offset = std::max(left - player.x, -2 * racing_cell_width);
            player.x += offset;
        }
        frame = frame == 16 ? 1 : frame + 1;
    };
    apply(state_.player, false, wiggle_frame_, wiggle_offset_);
    if (two_players_) apply(state_.player2, true, wiggle_frame2_, wiggle_offset2_);
    return events;
}

unsigned Racing::step(RacingInput input, RacingInput player2) {
    if (state_.finished) return racing_none;
    const bool starting_grace = state_.frames_remaining > duration_frames - frames_per_second;
    const bool wall1 = state_.wall_crash && wall_overlap(state_, state_.player, false);
    const bool wall2 = two_players_ && state_.wall_crash && wall_overlap(state_, state_.player2, true);
    const int previous_offset = wiggle_offset_, previous_offset2 = wiggle_offset2_;
    remove_wiggle();
    if (starting_grace || !wall1) steer(input);
    if (two_players_ && (starting_grace || !wall2)) steer(player2, true);

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
    if (bad_road_) state_.road_position += distance;
    unsigned events = apply_wiggle();
    if (previous_offset && !racing_dirt_at(state_, state_.player.y + state_.player.height / 2))
        events |= racing_wiggle_player1;
    if (two_players_ && previous_offset2 && !racing_dirt_at(state_, state_.player2.y + state_.player2.height / 2))
        events |= racing_wiggle_player2;

    const auto advance_blink = [](unsigned& remaining) {
        remaining = remaining > racing_blink_frame_samples ? remaining - racing_blink_frame_samples : 0;
    };
    advance_blink(state_.blink_samples);
    if (two_players_) advance_blink(state_.blink_samples2);
    const auto crash = [&](bool second, unsigned& collisions, bool& section_collided) {
        section_collided = true;
        if (collisions < std::numeric_limits<unsigned>::max()) ++collisions;
        (second ? state_.blink_samples2 : state_.blink_samples) = racing_blink_total_samples;
        state_.speed = start_speed;
        state_.movement_remainder = 0;
        events |= racing_collision;
    };
    const auto invulnerable = [&](bool second) {
        return (second ? state_.blink_samples2 : state_.blink_samples) != 0 ||
               (!two_players_ && input.recovering);
    };
    const auto check_wall = [&](RacingRect& player, bool second, unsigned& collisions, bool& section_collided) {
        // Boundary cells remain solid through visual dash gaps.
        const int direction = wall_overlap(state_, player, second);
        if (direction) {
            if (!invulnerable(second)) crash(second, collisions, section_collided);
            // Recovery overrides steering until the car clears the wall.
            player.x += direction;
            (second ? steering_remainder2_ : steering_remainder_) = 0;
        }
    };
    if (state_.wall_crash && !starting_grace) {
        check_wall(state_.player, false, state_.collisions, state_.section_collided);
        if (two_players_) check_wall(state_.player2, true, state_.collisions2, state_.section_collided2);
    }
    advance_path();
    const auto advance_traffic = [&](auto& traffic, const RacingRect& player, bool second, unsigned& collisions, bool& section_collided) {
    const unsigned count = more_cars_ ? 2 : 1;
    for (unsigned i = 0; i < count; ++i) {
        auto& car = traffic[i];
        car.rect.y += static_cast<int>(distance);
        if (!starting_grace && car_collision_ && !car.collided && !invulnerable(second) && overlaps(player, car.rect)) {
            car.collided = true;
            crash(second, collisions, section_collided);
        }
        if (car.rect.y >= state_.road.height) {
            if (zigzag_ && !second && (i == 0 || narrow_ || two_players_)) randomize_path(i);
            const int x = zigzag_ ? car.rect.x : second ? state_.traffic[i].rect.x + racing_center_column * racing_cell_width :
                more_cars_ && !two_players_ && !narrow_ && i == 1 ?
                state_.traffic[0].rect.x + racing_center_column * racing_cell_width : opponent_x();
            car.rect = {zigzag_ ? car.rect.x : x, car.rect.y - static_cast<int>(racing_road_dash_length + racing_road_gap_length),
                        traffic_width, traffic_height};
            car.collided = false;
        }
    }
    };
    advance_traffic(state_.traffic, state_.player, false, state_.collisions, state_.section_collided);
    if (two_players_)
        advance_traffic(state_.traffic2, state_.player2, true, state_.collisions2, state_.section_collided2);
    if (zigzag_) apply_path();
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
