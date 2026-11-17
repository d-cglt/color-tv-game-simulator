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
    const int left_limit = state.road.x + ctg::racing_road_border_width;
    const int right_limit = state.road.x + state.road.width - ctg::racing_road_border_width - player.width;
    const int left_target = car.x - player.width;
    const int right_target = car.x + car.width;
    const bool go_left = left_target >= left_limit &&
        (right_target > right_limit || player.x <= car.x);
    input.left = go_left && player.x > left_target;
    input.right = !go_left && player.x < right_target;
    game.step(input);
}

int main() {
    {
        ctg::Racing game;
        game.set_endless(true);
        game.reset();
        for (unsigned i = 0; i < ctg::Racing::duration_frames + 100; ++i)
            CHECK(!(game.step() & ctg::racing_finished));
        CHECK(!game.state().finished && game.state().frames_remaining == 0);
        auto& s = const_cast<ctg::RacingState&>(game.state());
        s.frame = s.score = s.collisions = std::numeric_limits<unsigned>::max();
        s.road_scroll = 239; s.speed = ctg::Racing::low_speed;
        s.movement_remainder = 0; s.section_collided = false;
        s.traffic[0].rect = s.player; s.traffic[0].collided = false;
        CHECK(game.step() & ctg::racing_collision);
        CHECK(s.collisions == std::numeric_limits<unsigned>::max());
        s.road_scroll = 239; s.speed = ctg::Racing::low_speed;
        s.movement_remainder = 0; s.section_collided = false;
        s.traffic[0].rect.y = 0;
        CHECK(game.step() & ctg::racing_pass);
        CHECK(s.score == std::numeric_limits<unsigned>::max());
        CHECK(s.frame == std::numeric_limits<unsigned>::max());
        game.set_endless(false);
        CHECK(game.step() & ctg::racing_finished);
        CHECK(s.finished && s.frames_remaining == 0);
    }
    {
        ctg::Racing game;
        auto& s = const_cast<ctg::RacingState&>(game.state());
        game.set_car_collision(false);
        game.reset(); // Configuration survives a match reset.
        s.traffic[0].rect = s.player;
        s.road_scroll = 239; s.speed = ctg::Racing::low_speed;
        unsigned events = game.step();
        CHECK(!(events & ctg::racing_collision) && (events & ctg::racing_pass));
        CHECK(s.collisions == 0 && s.score == 1 && !s.section_collided && !s.traffic[0].collided);
        for (int i = 0; i < 100; ++i) game.step({true, false});
        CHECK(s.player.x == s.road.x + ctg::racing_road_border_width);
        const unsigned score_before_collision = s.score;
        s.traffic[0].rect = s.player;
        game.set_car_collision(true);
        CHECK(game.step() & ctg::racing_collision);
        CHECK(s.section_collided && s.collisions == 1);
        game.set_car_collision(false);
        s.road_scroll = 239; s.speed = ctg::Racing::low_speed; s.movement_remainder = 0;
        CHECK(!(game.step() & ctg::racing_pass));
        CHECK(s.score == score_before_collision && !s.section_collided);
        game.set_car_collision(true);
        CHECK(!(game.step() & ctg::racing_collision)); // Same car cannot collide twice.
    }
    // Half-open full-sprite hitboxes collide only when both axes overlap.
    for (int dx : {ctg::racing_car_width - 1, ctg::racing_car_width}) {
        ctg::Racing game;
        auto& s = const_cast<ctg::RacingState&>(game.state());
        s.traffic[0].rect = {s.player.x + dx, s.player.y, ctg::racing_car_width, ctg::racing_car_height};
        CHECK(!!(game.step() & ctg::racing_collision) == (dx < ctg::racing_car_width));
    }
    for (int dy : {ctg::racing_car_height - 1, ctg::racing_car_height}) {
        ctg::Racing game;
        auto& s = const_cast<ctg::RacingState&>(game.state());
        s.traffic[0].rect = {s.player.x, s.player.y + dy, ctg::racing_car_width, ctg::racing_car_height};
        CHECK(!!(game.step() & ctg::racing_collision) == (dy < ctg::racing_car_height));
    }
    const ctg::RacingRect road{54, 0, 212, 240};
    CHECK(ctg::racing_x(26) == 158 && ctg::racing_x(53) == 266);
    CHECK(ctg::racing_car_width == 24 && ctg::racing_car_height == 30);
    bool seen[181]{};
    for (unsigned i = 0; i < 181; ++i) {
        const int x = ctg::racing_opponent_x(road, i);
        CHECK(x >= 58 && x + ctg::racing_car_width <= 262);
        CHECK(x <= 238);
        CHECK(x == ctg::racing_opponent_x(road, i + 181));
        seen[x - 58] = true;
    }
    for (bool position_seen : seen) CHECK(position_seen);
    ctg::Racing cycle;
    cycle.set_endless(true);
    auto& cycle_state = const_cast<ctg::RacingState&>(cycle.state());
    CHECK(cycle_state.road.x == 54 && cycle_state.road.width == 212 && cycle_state.road.height == 240);
    CHECK(ctg::racing_road_rows == 48 && ctg::racing_cell_height == 5);
    CHECK(cycle_state.player.y == 194 && cycle_state.player.y + cycle_state.player.height == 224);
    CHECK(cycle_state.traffic[0].rect.height == 30);
    CHECK(cycle_state.traffic[0].rect.x == ctg::racing_opponent_x(road, 0));
    for (unsigned i = 1; i <= 182; ++i) {
        cycle_state.traffic[0].rect.y = cycle_state.road.height;
        cycle.step();
        CHECK(cycle_state.traffic[0].rect.x == ctg::racing_opponent_x(road, i));
    }
    ctg::Racing preview;
    preview.reset_preview();
    const auto preview_initial = preview.state();
    preview.step_preview();
    CHECK(preview.state().road_scroll == ctg::racing_road_dash_length + ctg::racing_road_gap_length - 2);
    CHECK(preview.state().traffic[0].rect.y == preview_initial.traffic[0].rect.y - 2);
    CHECK(preview.state().player.x == preview_initial.player.x);
    CHECK(preview.state().frames_remaining == preview_initial.frames_remaining);
    for (int i = 0; i < 34; ++i) preview.step_preview();
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
    CHECK(game.state().player.x == 148);
    game.step({true, false}); CHECK(game.state().player.x == 142);
    game.step({true, true}); CHECK(game.state().player.x == 142);
    game.step({false, true}); CHECK(game.state().player.x == 148);
    game.step({false, true, true, std::numeric_limits<int>::min()});
    CHECK(game.state().player.x == 142);
    for (int i = 0; i < 100; ++i) game.step({false, false, true, std::numeric_limits<int>::min()});
    CHECK(game.state().player.x == 58);
    for (int i = 0; i < 30; ++i) game.step({false, false, true, 9000});
    CHECK(game.state().player.x > 58);
    for (int i = 0; i < 120; ++i) game.step({true, false, true, std::numeric_limits<int>::max()});
    CHECK(game.state().player.x == 238);
    for (int i = 0; i < 100; ++i) game.step({false, true});
    CHECK(game.state().player.x == 238);
    for (int i = 0; i < 100; ++i) game.step({true, false});
    CHECK(game.state().player.x == 58);

    game.reset();
    for (int i = 0; i < 30; ++i) game.step({false, false, true, 8192});
    CHECK(game.state().player.x == 148);
    for (int i = 0; i < 30; ++i) game.step({false, false, true, 20480});
    CHECK(game.state().player.x == 238);
    const int position = game.state().player.x;
    for (int i = 0; i < 30; ++i) game.step({false, false, true, 0});
    CHECK(game.state().player.x == position);

    game.reset();
    ctg::RacingInput speed_input{};
    for (int i = 0; i < 120; ++i) avoid_traffic(game, speed_input);
    CHECK(game.state().speed == ctg::Racing::low_speed);
    speed_input.select_high_speed = true;
    avoid_traffic(game, speed_input);
    CHECK(game.state().selected_speed == ctg::Racing::medium_speed);
    speed_input.select_high_speed = false;
    avoid_traffic(game, speed_input);
    speed_input.select_high_speed = true;
    avoid_traffic(game, speed_input);
    speed_input.select_high_speed = false;
    for (unsigned i = 0; i < ctg::Racing::max_speed - ctg::Racing::low_speed; ++i) avoid_traffic(game, speed_input);
    CHECK(game.state().speed == ctg::Racing::max_speed);
    CHECK(game.state().selected_speed == ctg::Racing::max_speed);
    speed_input.select_high_speed = true;
    avoid_traffic(game, speed_input); // Upper gear clamps.
    CHECK(game.state().selected_speed == ctg::Racing::max_speed);
    speed_input.select_high_speed = speed_input.select_low_speed = true;
    avoid_traffic(game, speed_input);
    CHECK(game.state().selected_speed == ctg::Racing::max_speed);
    speed_input.select_high_speed = false;
    avoid_traffic(game, speed_input);
    CHECK(game.state().selected_speed == ctg::Racing::medium_speed);
    CHECK(game.state().speed == ctg::Racing::max_speed - 1);
    speed_input.select_low_speed = false;
    avoid_traffic(game, speed_input);
    speed_input.select_low_speed = true;
    avoid_traffic(game, speed_input);
    CHECK(game.state().selected_speed == ctg::Racing::low_speed);
    speed_input.select_low_speed = false;
    for (unsigned i = 0; i < ctg::Racing::max_speed - ctg::Racing::low_speed; ++i) avoid_traffic(game, speed_input);
    CHECK(game.state().speed == ctg::Racing::low_speed);
    speed_input.select_low_speed = true;
    avoid_traffic(game, speed_input); // Lower gear clamps.
    CHECK(game.state().speed == ctg::Racing::low_speed && game.state().selected_speed == ctg::Racing::low_speed);
    speed_input.select_low_speed = false;
    speed_input.select_high_speed = true;
    avoid_traffic(game, speed_input);
    avoid_traffic(game, speed_input);
    CHECK(game.state().selected_speed == ctg::Racing::max_speed);
    game.reset();
    CHECK(game.state().selected_speed == ctg::Racing::low_speed && game.state().speed == ctg::Racing::start_speed);

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
    game.step();
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
    CHECK(low_distance == 236 || low_distance == 237);
    away.select_high_speed = true;
    avoid_traffic(speed_check, away);
    away.select_high_speed = false;
    avoid_traffic(speed_check, away);
    away.select_high_speed = true;
    avoid_traffic(speed_check, away);
    away.select_high_speed = false;
    for (int i = 0; i < 124; ++i) avoid_traffic(speed_check, away);
    const unsigned high_distance = measure_distance();
    CHECK(high_distance == 403 || high_distance == 404);

    // Include acceleration and the entire active timer from different preview phases.
    // Phase 239 is immediately before the scoring boundary; 240 is just after it.
    struct PhaseCase { unsigned phase, score, final_phase; };
    for (const PhaseCase test : {PhaseCase{0, 56, 192}, {47, 56, 239},
                                 {48, 57, 240}, {100, 57, 28},
                                 {239, 57, 167}, {240, 56, 168}, {263, 56, 191}}) {
        ctg::Racing low_perfect_run;
        auto layout = low_perfect_run.state();
        layout.road_scroll = test.phase;
        low_perfect_run.reset_from_layout(layout);
        for (unsigned i = 0; i < ctg::Racing::duration_frames; ++i)
            avoid_traffic(low_perfect_run);
        CHECK(low_perfect_run.state().finished && low_perfect_run.state().frames_remaining == 0);
        CHECK(low_perfect_run.state().collisions == 0 && low_perfect_run.state().score == test.score);
        CHECK(low_perfect_run.state().road_scroll == test.final_phase);
    }

    ctg::Racing perfect_run;
    ctg::RacingInput full_speed{};
    for (unsigned i = 0; i < 60 * ctg::Racing::frames_per_second; ++i) {
        full_speed.select_high_speed = i == 0 || i == 2;
        avoid_traffic(perfect_run, full_speed);
    }
    CHECK(!perfect_run.state().finished && perfect_run.state().frames_remaining == 4 * 60);
    CHECK(perfect_run.state().collisions == 0 && perfect_run.state().score == 89);
    while (!perfect_run.state().finished) avoid_traffic(perfect_run, full_speed);
    CHECK(perfect_run.state().collisions == 0);
    CHECK(perfect_run.state().score == 95);
    // The scoring boundary's starting phase can add at most one point.
    for (unsigned phase = 0; phase < 264; ++phase) {
        ctg::Racing run;
        auto layout = run.state();
        layout.road_scroll = phase;
        run.reset_from_layout(layout);
        for (unsigned i = 0; i < ctg::Racing::duration_frames; ++i) {
            ctg::RacingInput shift{};
            shift.select_high_speed = i == 0 || i == 2;
            avoid_traffic(run, shift);
        }
        CHECK(run.state().finished && run.state().collisions == 0);
        CHECK(run.state().score == (phase >= 65 && phase < 240 ? 96u : 95u));
    }

    CHECK(ctg::Racing::duration_frames == 64 * 60);
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
