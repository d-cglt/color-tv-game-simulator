#include "racing.h"
#include <cstdio>
#include <limits>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

static bool same(const ctg::RacingState& a, const ctg::RacingState& b) {
    if (a.frame != b.frame || a.frames_remaining != b.frames_remaining ||
        a.speed != b.speed || a.selected_speed != b.selected_speed || a.movement_remainder != b.movement_remainder || a.road_scroll != b.road_scroll ||
        a.score != b.score || a.collisions != b.collisions || a.section_collided != b.section_collided || a.finished != b.finished ||
        a.player.x != b.player.x) return false;
    for (unsigned i = 0; i < a.traffic.size(); ++i)
        if (a.traffic[i].rect.x != b.traffic[i].rect.x ||
            a.traffic[i].rect.y != b.traffic[i].rect.y ||
            a.traffic[i].collided != b.traffic[i].collided) return false;
    return true;
}

static void avoid_traffic(ctg::Racing& game, ctg::RacingInput input = {}) {
    const auto& state = game.state();
    const auto& player = state.player;
    const auto& car = state.traffic[0].rect;
    const int left_limit = state.road.x + ctg::racing_car_overhang;
    const int right_limit = state.road.x + state.road.width - player.width - ctg::racing_car_overhang;
    const int left_target = car.x - player.width;
    const int right_target = car.x + car.width;
    const bool go_left = left_target >= left_limit &&
        (right_target > right_limit || player.x <= car.x);
    input.left = go_left && player.x > left_target;
    input.right = !go_left && player.x < right_target;
    game.step(input);
}

int main() {
    const ctg::RacingRect road{40, 0, 240, 240};
    bool seen[211]{};
    for (unsigned i = 0; i < 211; ++i) {
        const int x = ctg::racing_opponent_x(road, i);
        CHECK(x - ctg::racing_car_overhang >= road.x + 4);
        CHECK(x + 16 + ctg::racing_car_overhang <= road.x + road.width - 4);
        CHECK(x >= 47 && x <= 257);
        seen[x - 47] = true;
    }
    for (bool position_seen : seen) CHECK(position_seen);
    ctg::Racing preview;
    preview.reset_preview();
    const auto preview_initial = preview.state();
    preview.step_preview();
    CHECK(preview.state().road_scroll == ctg::racing_road_dash_length + ctg::racing_road_gap_length - 2);
    CHECK(preview.state().traffic[0].rect.y == preview_initial.traffic[0].rect.y - 2);
    CHECK(preview.state().player.x == preview_initial.player.x);
    CHECK(preview.state().frames_remaining == preview_initial.frames_remaining);
    for (int i = 0; i < 31; ++i) preview.step_preview();
    CHECK(preview.state().traffic[0].rect.y == 240);
    ctg::Racing continued;
    continued.reset_from_layout(preview.state());
    CHECK(continued.state().road_scroll == preview.state().road_scroll);
    for (unsigned i = 0; i < preview.state().traffic.size(); ++i) {
        CHECK(continued.state().traffic[i].rect.x == preview.state().traffic[i].rect.x);
        CHECK(continued.state().traffic[i].rect.y == preview.state().traffic[i].rect.y);
    }
    CHECK(continued.state().speed == ctg::Racing::start_speed);
    CHECK(continued.state().frames_remaining == ctg::Racing::duration_frames);
    continued.step_preview();
    CHECK(continued.state().traffic[0].rect.y == preview.state().traffic[0].rect.y - 2);
    CHECK(continued.state().frames_remaining == ctg::Racing::duration_frames);

    ctg::Racing game;
    CHECK(game.state().traffic.size() == 1);
    for (int i = 0; i < 1000; ++i) {
        const auto& state = game.state();
        const int period = ctg::racing_road_dash_length + ctg::racing_road_gap_length;
        CHECK((state.traffic[0].rect.y - static_cast<int>(state.road_scroll) + period) % period == 40);
        game.step({true, false});
    }
    game.reset();
    CHECK(game.state().frames_remaining == ctg::Racing::duration_frames);
    CHECK(game.state().player.x == 152);
    game.step({true, false}); CHECK(game.state().player.x == 146);
    game.step({true, true}); CHECK(game.state().player.x == 146);
    game.step({false, true}); CHECK(game.state().player.x == 152);
    game.step({false, true, true, std::numeric_limits<int>::min()});
    CHECK(game.state().player.x == 146);
    for (int i = 0; i < 100; ++i) game.step({false, false, true, std::numeric_limits<int>::min()});
    CHECK(game.state().player.x == game.state().road.x + ctg::racing_car_overhang);
    for (int i = 0; i < 30; ++i) game.step({false, false, true, 9000});
    CHECK(game.state().player.x > game.state().road.x + ctg::racing_car_overhang);
    for (int i = 0; i < 120; ++i) game.step({true, false, true, std::numeric_limits<int>::max()});
    CHECK(game.state().player.x == game.state().road.x + game.state().road.width - game.state().player.width - ctg::racing_car_overhang);
    for (int i = 0; i < 100; ++i) game.step({false, true});
    CHECK(game.state().player.x == game.state().road.x + game.state().road.width - game.state().player.width - ctg::racing_car_overhang);

    game.reset();
    for (int i = 0; i < 30; ++i) game.step({false, false, true, 8192});
    CHECK(game.state().player.x == 152);
    for (int i = 0; i < 30; ++i) game.step({false, false, true, 20480});
    CHECK(game.state().player.x == 242);
    const int position = game.state().player.x;
    for (int i = 0; i < 30; ++i) game.step({false, false, true, 0});
    CHECK(game.state().player.x == position);

    game.reset();
    ctg::RacingInput speed_input{};
    for (int i = 0; i < 120; ++i) avoid_traffic(game, speed_input);
    CHECK(game.state().speed == ctg::Racing::low_speed);
    speed_input.select_high_speed = true;
    avoid_traffic(game, speed_input);
    speed_input.select_high_speed = false;
    for (unsigned i = 1; i < ctg::Racing::max_speed - ctg::Racing::low_speed; ++i) avoid_traffic(game, speed_input);
    CHECK(game.state().speed == ctg::Racing::max_speed);
    CHECK(game.state().selected_speed == ctg::Racing::max_speed);
    speed_input.select_high_speed = speed_input.select_low_speed = true;
    avoid_traffic(game, speed_input);
    CHECK(game.state().selected_speed == ctg::Racing::max_speed);
    speed_input.select_high_speed = false;
    avoid_traffic(game, speed_input);
    CHECK(game.state().selected_speed == ctg::Racing::low_speed);
    CHECK(game.state().speed == ctg::Racing::max_speed - 1);
    speed_input.select_low_speed = false;
    for (unsigned i = 1; i < ctg::Racing::max_speed - ctg::Racing::low_speed; ++i) avoid_traffic(game, speed_input);
    CHECK(game.state().speed == ctg::Racing::low_speed);
    avoid_traffic(game, speed_input);
    CHECK(game.state().speed == ctg::Racing::low_speed);

    // Score only when the gap's bottom reaches the bottom of the road.
    game.reset();
    bool scored = false;
    for (int i = 0; i < 400 && !scored; ++i) {
        const auto before = game.state();
        const unsigned events = game.step({true, false});
        const auto& after = game.state();
        if (events & ctg::racing_pass) {
            CHECK(before.road_scroll < 240 && after.road_scroll >= 240);
            CHECK(after.score == before.score + 1);
            scored = true;
        } else CHECK(after.score == before.score);
    }
    CHECK(scored);

    // A collision invalidates the current road section.
    game.reset();
    unsigned collision_events = 0, pass_events = 0;
    for (int i = 0; i < 400 && collision_events == 0; ++i) {
        unsigned events = game.step();
        collision_events += !!(events & ctg::racing_collision);
        pass_events += !!(events & ctg::racing_pass);
    }
    CHECK(collision_events == 1);
    CHECK(pass_events == 1);
    CHECK(game.state().collisions == 1);
    CHECK(game.state().section_collided);
    CHECK(game.state().speed == ctg::Racing::start_speed);
    CHECK(game.state().score == 1);
    // Recovery holds the starting speed; acceleration resumes afterward.
    ctg::RacingInput recovery{};
    recovery.recovering = true;
    for (int i = 0; i < 30; ++i) game.step(recovery);
    CHECK(game.state().speed == ctg::Racing::start_speed);
    for (int i = 0; i < 30; ++i) game.step();
    CHECK(game.state().speed == ctg::Racing::start_speed + 30);
    CHECK(game.state().selected_speed == ctg::Racing::low_speed);
    CHECK(game.state().score == 1);
    for (int i = 0; i < 400 && game.state().section_collided; ++i) game.step({true, false});
    CHECK(!game.state().section_collided && game.state().score == 1);
    unsigned next_section = 0;
    for (int i = 0; i < 400 && !(next_section & ctg::racing_pass); ++i) next_section = game.step({true, false});
    CHECK((next_section & ctg::racing_pass) && game.state().score == 2);

    game.reset();
    ctg::RacingInput high_gear{};
    high_gear.select_high_speed = true;
    game.step(high_gear);
    unsigned event = 0;
    for (int i = 0; i < 400 && !(event & ctg::racing_collision); ++i) event = game.step();
    CHECK(event & ctg::racing_collision);
    CHECK(game.state().speed == ctg::Racing::start_speed && game.state().selected_speed == ctg::Racing::max_speed);
    game.step();
    CHECK(game.state().speed == ctg::Racing::start_speed + 1 && game.state().selected_speed == ctg::Racing::max_speed);

    ctg::Racing speed_check;
    ctg::RacingInput away{};
    for (int i = 0; i < 120; ++i) avoid_traffic(speed_check, away);
    const auto measure_distance = [&]() {
        unsigned distance = 0;
        constexpr unsigned period = ctg::racing_road_dash_length + ctg::racing_road_gap_length;
        for (int i = 0; i < 60; ++i) {
            const unsigned before = speed_check.state().road_scroll;
            avoid_traffic(speed_check, away);
            distance += (speed_check.state().road_scroll + period - before) % period;
        }
        return distance;
    };
    const unsigned low_distance = measure_distance();
    CHECK(low_distance == 213 || low_distance == 214);
    away.select_high_speed = true;
    for (int i = 0; i < 124; ++i) avoid_traffic(speed_check, away);
    const unsigned high_distance = measure_distance();
    CHECK(high_distance == 433 || high_distance == 434);

    ctg::Racing perfect_run;
    ctg::RacingInput full_speed{};
    full_speed.select_high_speed = true;
    while (!perfect_run.state().finished) avoid_traffic(perfect_run, full_speed);
    CHECK(perfect_run.state().collisions == 0);
    CHECK(perfect_run.state().score == 96);

    ctg::Racing other;
    game.reset();
    CHECK(same(game.state(), other.state()));
    for (unsigned i = 0; i < ctg::Racing::duration_frames; ++i) {
        ctg::RacingInput input{ i % 100 < 20, i % 100 >= 80, i % 3 == 0, static_cast<int>(i % 65536) - 32768 };
        unsigned a = game.step(input), b = other.step(input);
        CHECK(a == b); CHECK(same(game.state(), other.state()));
        CHECK(!!(a & ctg::racing_finished) == (i + 1 == ctg::Racing::duration_frames));
    }
    CHECK(game.state().finished);
    CHECK(game.state().frame == ctg::Racing::duration_frames);
    CHECK(game.state().frames_remaining == 0);
    CHECK(game.step({true, false}) == ctg::racing_none);
    CHECK(same(game.state(), other.state()));
    game.reset();
    ctg::Racing fresh;
    CHECK(same(game.state(), fresh.state()));
    for (int i = 0; i < 500; ++i) {
        CHECK(game.step() == fresh.step());
        CHECK(same(game.state(), fresh.state()));
    }
    return 0;
}
