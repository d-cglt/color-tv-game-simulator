#include "racing.h"
#include <cstdio>
#include <limits>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

static bool same(const ctg::RacingState& a, const ctg::RacingState& b) {
    if (a.frame != b.frame || a.frames_remaining != b.frames_remaining ||
        a.speed != b.speed || a.selected_speed != b.selected_speed || a.movement_remainder != b.movement_remainder || a.road_scroll != b.road_scroll ||
        a.score != b.score || a.collisions != b.collisions || a.section_collided != b.section_collided || a.finished != b.finished || a.narrow != b.narrow ||
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
        game.set_narrow(true);
        CHECK(game.state().narrow && game.state().road.width == 53 * ctg::racing_cell_width);
        CHECK(game.state().player.x == 96);
        game.reset_preview();
        CHECK(game.state().player.x == 96);
        CHECK(game.state().narrow);
        const auto preview = game.state();
        game.step_preview();
        CHECK(game.state().narrow && game.state().player.x == preview.player.x);
        game.reset_from_layout(preview);
        CHECK(game.state().narrow && game.state().traffic[0].rect.x == preview.traffic[0].rect.x);
        for (int i = 0; i < 100; ++i) game.step({true, false});
        CHECK(game.state().player.x == ctg::racing_x(0));
        for (int i = 0; i < 100; ++i) game.step({false, true});
        CHECK(game.state().player.x + game.state().player.width == ctg::racing_x(27));

        auto& state = const_cast<ctg::RacingState&>(game.state());
        game.set_car_collision(false);
        game.reset();

        auto layout = state;
        layout.player.x = ctg::racing_x(50);
        layout.traffic[0].rect.x = ctg::racing_x(50);
        layout.traffic[0].rect.width = 3 * ctg::racing_cell_width;
        game.reset_from_layout(layout);
        CHECK(state.player.x + state.player.width == ctg::racing_x(27));
        CHECK(state.traffic[0].rect.x + state.traffic[0].rect.width == ctg::racing_x(27));
        game.set_narrow(false);
        CHECK(!state.narrow);
        game.reset();
        CHECK(!state.narrow && state.player.x == (320 - ctg::racing_car_width) / 2);
        CHECK(state.traffic[0].rect.x >= 58 && state.traffic[0].rect.x <= 238);
    }
    for (bool narrow : {false, true}) for (bool analog : {false, true}) {
        ctg::Racing countdown;
        countdown.set_narrow(narrow);
        countdown.reset();
        const auto before = countdown.state();
        ctg::RacingInput steering{};
        steering.left = true;
        steering.analog = analog;
        steering.steering = -32767;
        steering.select_high_speed = true;
        countdown.step_preview(steering);
        CHECK(countdown.state().player.x == before.player.x - 6);
        CHECK(countdown.state().frames_remaining == before.frames_remaining);
        CHECK(countdown.state().speed == before.speed && countdown.state().selected_speed == before.selected_speed);
        CHECK(countdown.state().score == before.score && countdown.state().collisions == before.collisions);
        countdown.step();
        CHECK(countdown.state().player.x == before.player.x - 6);
    }
    for (bool narrow : {false, true}) for (bool analog : {false, true}) {
        ctg::Racing game;
        game.set_narrow(narrow);
        game.set_car_collision(false);
        ctg::RacingInput input{};
        input.analog = analog;
        input.left = true;
        input.steering = -32767;
        for (int i = 0; i < 100; ++i) game.step(input);
        CHECK(game.state().player.x == game.state().road.x);
        input.left = false;
        input.right = true;
        input.steering = 32767;
        for (int i = 0; i < 100; ++i) game.step(input);
        const int boundary = narrow ? ctg::racing_x(27) : game.state().road.x + game.state().road.width;
        CHECK(game.state().player.x + game.state().player.width == boundary);
    }
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
        CHECK(s.player.x == s.road.x);
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

    CHECK(ctg::racing_x(26) == 158 && ctg::racing_x(53) == 266);
    CHECK(ctg::racing_car_width == 24 && ctg::racing_car_height == 30);
    // Compare streams, not particular values or a permutation of legal positions.
    for (bool narrow : {false, true}) {
        ctg::Racing a, b, noise;
        a.set_narrow(narrow); b.set_narrow(narrow); noise.set_narrow(narrow);
        a.set_random_seed(12345); b.set_random_seed(12345);
        noise.set_random_seed(98765);
        a.set_car_collision(false); b.set_car_collision(false); noise.set_car_collision(false);
        a.set_endless(true); b.set_endless(true); noise.set_endless(true);
        a.reset(); b.reset();
        auto& as = const_cast<ctg::RacingState&>(a.state());
        auto& bs = const_cast<ctg::RacingState&>(b.state());
        auto& ns = const_cast<ctg::RacingState&>(noise.state());
        const int left = narrow ? 54 : 58, right = narrow ? 138 : 238;
        bool seen[181]{};
        bool repeated = false, varied = false, reset_changed = false;
        const int initial = as.traffic[0].rect.x;
        CHECK(initial == bs.traffic[0].rect.x);
        CHECK(initial >= left && initial <= right);
        seen[initial - left] = true;
        for (unsigned i = 0; i < 512; ++i) {
            // Interleave an unrelated instance, including reseeding and resetting it.
            if (i % 17 == 0) { noise.set_random_seed(i); noise.reset(); }
            ns.traffic[0].rect.y = ns.road.height; noise.step();
            as.traffic[0].rect.y = as.road.height; a.step();
            bs.traffic[0].rect.y = bs.road.height; b.step();
            const int x = as.traffic[0].rect.x;
            CHECK(x == bs.traffic[0].rect.x);
            CHECK(x >= left && x <= right);
            repeated |= seen[x - left];
            varied |= x != initial;
            seen[x - left] = true;
        }
        CHECK(repeated && varied);
        // A reset consumes the continuing stream rather than replaying its start.
        for (unsigned i = 0; i < 32; ++i) {
            a.reset(); b.reset();
            CHECK(as.traffic[0].rect.x == bs.traffic[0].rect.x);
            CHECK(as.traffic[0].rect.x >= left && as.traffic[0].rect.x <= right);
            reset_changed |= as.traffic[0].rect.x != initial;
        }
        CHECK(reset_changed);
        a.set_random_seed(12345); a.reset();
        CHECK(as.traffic[0].rect.x == initial);
    }
    {
        ctg::Racing a, b;
        CHECK(same(a.state(), b.state()));
        for (unsigned i = 0; i < 64; ++i) {
            auto& as = const_cast<ctg::RacingState&>(a.state());
            auto& bs = const_cast<ctg::RacingState&>(b.state());
            as.traffic[0].rect.y = as.road.height;
            bs.traffic[0].rect.y = bs.road.height;
            CHECK(a.step() == b.step());
            CHECK(same(a.state(), b.state()));
        }
    }
    ctg::Racing cycle;
    cycle.set_endless(true);
    auto& cycle_state = const_cast<ctg::RacingState&>(cycle.state());
    CHECK(cycle_state.road.x == 54 && cycle_state.road.width == 212 && cycle_state.road.height == 240);
    CHECK(ctg::racing_road_rows == 48 && ctg::racing_cell_height == 5);
    CHECK(cycle_state.player.y == 194 && cycle_state.player.y + cycle_state.player.height == 224);
    CHECK(ctg::racing_road_gap_length == 30 && ctg::racing_opponent_gap_offset == 50);
    CHECK(cycle_state.traffic[0].rect.height == 30 && cycle_state.traffic[0].rect.y == 50);

    ctg::Racing preview;
    preview.reset_preview();
    const auto preview_initial = preview.state();
    preview.step_preview();
    CHECK(preview.state().road_scroll == ctg::racing_road_dash_length + ctg::racing_road_gap_length - 2);
    CHECK(preview.state().traffic[0].rect.y == preview_initial.traffic[0].rect.y - 2);
    CHECK(preview.state().player.x == preview_initial.player.x);
    CHECK(preview.state().frames_remaining == preview_initial.frames_remaining);
    for (int i = 0; i < 39; ++i) preview.step_preview();
    CHECK(preview.state().traffic[0].rect.y == 240);
    // Preserve the relative phase even when preview wrapping has pixel overshoot.
    auto odd_layout = preview_initial;
    odd_layout.road_scroll = 1;
    odd_layout.traffic[0].rect.y = 51;
    ctg::Racing odd_preview;
    odd_preview.reset_from_layout(odd_layout);
    for (int i = 0; i < 300; ++i) {
        odd_preview.step_preview();
        const auto& state = odd_preview.state();
        CHECK((state.traffic[0].rect.y - static_cast<int>(state.road_scroll) + 540) % 270 == 50);
    }
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
        CHECK((state.traffic[0].rect.y - static_cast<int>(state.road_scroll) + period) % period == ctg::racing_opponent_gap_offset);
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
    CHECK(game.state().player.x == 54);
    for (int i = 0; i < 30; ++i) game.step({false, false, true, 9000});
    CHECK(game.state().player.x > 54);
    for (int i = 0; i < 120; ++i) game.step({true, false, true, std::numeric_limits<int>::max()});
    CHECK(game.state().player.x == 242);
    for (int i = 0; i < 100; ++i) game.step({false, true});
    CHECK(game.state().player.x == 242);
    for (int i = 0; i < 100; ++i) game.step({true, false});
    CHECK(game.state().player.x == 54);

    game.reset();
    for (int i = 0; i < 30; ++i) game.step({false, false, true, 8192});
    CHECK(game.state().player.x == 148);
    for (int i = 0; i < 30; ++i) game.step({false, false, true, 20480});
    CHECK(game.state().player.x == 238);
    const int position = game.state().player.x;
    for (int i = 0; i < 30; ++i) game.step({false, false, true, 0});
    CHECK(game.state().player.x == position);

    // Default switch is HIGH; a single HIGH input selects the maximum directly.
    ctg::Racing default_switch;
    ctg::RacingInput high{}; high.select_high_speed = true;
    ctg::RacingInput low{}; low.select_low_speed = true;
    ctg::RacingInput both{}; both.select_high_speed = both.select_low_speed = true;
    default_switch.step(high);
    CHECK(default_switch.state().selected_speed == ctg::Racing::max_speed);

    const unsigned caps[] = {ctg::Racing::low_speed, ctg::Racing::medium_speed, ctg::Racing::max_speed};
    for (unsigned setting = 0; setting < 3; ++setting) {
        ctg::Racing gears;
        gears.set_car_collision(false);
        gears.set_speed_switch(setting);
        CHECK(gears.state().selected_speed == ctg::Racing::low_speed);
        gears.step(both); // Simultaneous inputs leave LOW selected.
        CHECK(gears.state().selected_speed == ctg::Racing::low_speed);
        gears.step(low); gears.step(low);
        CHECK(gears.state().selected_speed == ctg::Racing::low_speed);
        gears.step(high);
        CHECK(gears.state().selected_speed == caps[setting]);
        gears.step(high); gears.step(high); // Repeated selection is idempotent.
        CHECK(gears.state().selected_speed == caps[setting]);
        gears.step(both); // Simultaneous inputs leave HIGH selected.
        CHECK(gears.state().selected_speed == caps[setting]);
        for (unsigned i = 0; i < ctg::Racing::max_speed; ++i) gears.step();
        CHECK(gears.state().speed == caps[setting]);
        gears.step(low);
        CHECK(gears.state().selected_speed == ctg::Racing::low_speed);
        gears.step(low);
        CHECK(gears.state().selected_speed == ctg::Racing::low_speed);
        for (unsigned i = 0; i < ctg::Racing::max_speed; ++i) gears.step();
        CHECK(gears.state().speed == ctg::Racing::low_speed);
        gears.step(high);
        gears.reset();
        CHECK(gears.state().selected_speed == ctg::Racing::low_speed && gears.state().speed == ctg::Racing::start_speed);
        gears.step(high); // Reset returns to LOW but preserves the switch setting.
        CHECK(gears.state().selected_speed == caps[setting]);
    }

    // Score only when the gap's bottom reaches the bottom of the road.
    game.set_car_collision(false);
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
    game.set_car_collision(true);
    game.reset();
    auto& collision_state = const_cast<ctg::RacingState&>(game.state());
    collision_state.traffic[0].rect.x = collision_state.road.x;
    unsigned collision_events = 0, pass_events = 0;
    for (int i = 0; i < 400 && collision_events == 0; ++i) {
        const int previous_y = collision_state.traffic[0].rect.y;
        unsigned events = game.step();
        if (collision_state.traffic[0].rect.y < previous_y)
            collision_state.traffic[0].rect.x = collision_state.player.x;
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
    const_cast<ctg::RacingState&>(game.state()).traffic[0].rect.x = game.state().player.x;
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
    CHECK(low_distance == 241 || low_distance == 242);
    away.select_high_speed = true;
    avoid_traffic(speed_check, away);
    away.select_high_speed = false;
    for (int i = 0; i < 124; ++i) avoid_traffic(speed_check, away);
    const unsigned high_distance = measure_distance();
    CHECK(high_distance == 414 || high_distance == 415);

    // Every possible starting road phase, with the configured HIGH gear selected immediately.
    constexpr unsigned period = ctg::racing_road_dash_length + ctg::racing_road_gap_length;
    struct GearCase { unsigned switch_setting, cap, distance, minimum, maximum; };
    for (const GearCase test : {GearCase{0, 136, 15308, 56, 57},
                                {1, 166, 18615, 68, 69},
                                {2, 233, 25902, 95, 96}}) {
        unsigned min_score = test.maximum, max_score = 0;
        for (unsigned phase = 0; phase < period; ++phase) {
            ctg::Racing run;
            run.set_car_collision(false);
            run.set_speed_switch(test.switch_setting);
            auto layout = run.state();
            layout.road_scroll = phase;
            run.reset_from_layout(layout);
            for (unsigned i = 0; i < ctg::Racing::duration_frames; ++i) {
                ctg::RacingInput shift{};
                shift.select_high_speed = i == 0;
                run.step(shift);
            }
            const auto& state = run.state();
            CHECK(state.finished && state.frames_remaining == 0 && state.collisions == 0);
            CHECK(state.speed == test.cap && state.selected_speed == test.cap);
            CHECK(state.road_scroll == (phase + test.distance) % period);
            const unsigned expected = (phase + test.distance + period - 240) / period -
                                      (phase + period - 240) / period;
            CHECK(state.score == expected);
            if (state.score < min_score) min_score = state.score;
            if (state.score > max_score) max_score = state.score;
        }
        CHECK(min_score == test.minimum && max_score == test.maximum);
    }

    CHECK(ctg::Racing::duration_frames == 64 * 60);
    ctg::Racing other;
    game.set_random_seed(2468); other.set_random_seed(2468);
    game.reset(); other.reset();
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
    ctg::Racing fresh;
    game.set_random_seed(1357); fresh.set_random_seed(1357);
    game.reset(); fresh.reset();
    CHECK(same(game.state(), fresh.state()));
    for (int i = 0; i < 500; ++i) {
        CHECK(game.step() == fresh.step());
        CHECK(same(game.state(), fresh.state()));
    }
    for (unsigned setting = 0; setting < 3; ++setting) {
        ctg::Racing two;
        two.set_two_players(true);
        two.set_speed_switch(setting);
        two.set_car_collision(false);
        two.reset();
        const unsigned target = setting == 0 ? ctg::Racing::low_speed : setting == 1 ? ctg::Racing::medium_speed : ctg::Racing::max_speed;
        CHECK(two.state().speed == ctg::Racing::start_speed && two.state().selected_speed == target);
        for (int i = 0; i < 100; ++i) two.step_preview();
        CHECK(two.state().speed == ctg::Racing::start_speed && two.state().frames_remaining == ctg::Racing::duration_frames);
        for (unsigned speed = ctg::Racing::start_speed + 1; speed <= target; ++speed) {
            ctg::RacingInput input{}; input.select_low_speed = true;
            two.step(input);
            CHECK(two.state().speed == speed && two.state().selected_speed == target);
        }
        two.step();
        CHECK(two.state().speed == target);
    }
    {
        ctg::Racing two;
        two.set_speed_switch(1);
        two.set_two_players(true);
        const int divider = ctg::racing_x(ctg::racing_center_column) + ctg::racing_cell_width;
        CHECK(two.state().two_players);
        CHECK(two.state().speed == ctg::Racing::start_speed && two.state().selected_speed == ctg::Racing::medium_speed);
        CHECK(two.state().player.x == 96);
        CHECK(two.state().player2.x == 200);
        two.set_car_collision(false);
        ctg::RacingInput left{}, right{};
        left.left = true; right.right = true;
        left.select_low_speed = true; right.select_high_speed = true;
        for (int i = 0; i < 140; ++i) two.step(left, right);
        CHECK(two.state().player.x == ctg::racing_road_x);
        CHECK(two.state().player2.x == ctg::racing_road_x + ctg::racing_road_columns * ctg::racing_cell_width - ctg::racing_car_width);
        CHECK(two.state().speed == ctg::Racing::medium_speed);
        for (int i = 0; i < 100; ++i) two.step(right, left);
        CHECK(two.state().player.x == divider - ctg::racing_car_width);
        CHECK(two.state().player2.x == ctg::racing_x(ctg::racing_center_column));
        two.set_speed_switch(0);
        CHECK(two.state().selected_speed == ctg::Racing::low_speed);
        two.step();
        CHECK(two.state().speed == ctg::Racing::medium_speed - 1);
        two.reset();
        CHECK(two.state().speed == ctg::Racing::start_speed && two.state().selected_speed == ctg::Racing::low_speed);
        two.set_two_players(false);
        CHECK(!two.state().two_players);
        CHECK(two.state().speed == ctg::Racing::start_speed);
    }
    {
        ctg::Racing two;
        two.set_two_players(true);
        auto& s = const_cast<ctg::RacingState&>(two.state());
        s.traffic[0].rect = s.player;
        s.traffic2[0].rect.y = 0;
        s.speed = ctg::Racing::max_speed;
        s.road_scroll = ctg::racing_road_rows * ctg::racing_cell_height - 1;
        const unsigned events = two.step();
        CHECK(events & ctg::racing_collision);
        CHECK(events & ctg::racing_pass);
        CHECK(s.collisions == 1 && s.collisions2 == 0);
        CHECK(s.blink_samples == ctg::racing_blink_total_samples && s.blink_samples2 == 0);
        CHECK(s.score == 0 && s.score2 == 1);
        CHECK(s.speed == ctg::Racing::start_speed && s.selected_speed == ctg::Racing::max_speed);
        CHECK(!s.section_collided && !s.section_collided2);
        s.traffic2[0].rect = s.player2;
        two.step();
        CHECK(s.collisions2 == 1 && s.collisions == 1);
        CHECK(s.speed == ctg::Racing::start_speed);
        ctg::RacingInput recovering{}; recovering.recovering = true;
        for (int i = 0; i < 10; ++i) two.step(recovering);
        CHECK(s.speed == ctg::Racing::start_speed && s.selected_speed == ctg::Racing::max_speed);
        two.step();
        CHECK(s.speed == ctg::Racing::start_speed + 1);
        CHECK(s.section_collided2 && !s.section_collided);
        CHECK(s.blink_samples == ctg::racing_blink_total_samples - 12 * ctg::racing_blink_frame_samples);
        CHECK(s.blink_samples2 == ctg::racing_blink_total_samples - 11 * ctg::racing_blink_frame_samples);
        two.step_preview();
        CHECK(s.blink_samples2 == ctg::racing_blink_total_samples - 11 * ctg::racing_blink_frame_samples);
        two.set_car_collision(false);
        for (unsigned i = 0; i < 98; ++i) two.step();
        CHECK(s.blink_samples == 0 && s.blink_samples2 == 0);
        s.traffic[0].rect = s.player;
        s.traffic2[0].rect = s.player2;
        s.traffic[0].collided = s.traffic2[0].collided = false;
        two.set_car_collision(true);
        CHECK(two.step() & ctg::racing_collision);
        CHECK(s.blink_samples == ctg::racing_blink_total_samples && s.blink_samples2 == s.blink_samples);
        two.reset();
        CHECK(s.blink_samples == 0 && s.blink_samples2 == 0);
    }
    {
        ctg::Racing two;
        two.set_random_seed(456);
        two.set_two_players(true);
        two.set_car_collision(false);
        const int offset = ctg::racing_center_column * ctg::racing_cell_width;
        auto& s = const_cast<ctg::RacingState&>(two.state());
        const int initial = s.traffic[0].rect.x;
        bool varied = false;
        CHECK(s.traffic2[0].rect.x == initial + offset);
        for (int i = 0; i < 100; ++i) {
            s.traffic[0].rect.y = s.traffic2[0].rect.y = s.road.height;
            two.step();
            CHECK(s.traffic2[0].rect.x == s.traffic[0].rect.x + offset);
            varied |= s.traffic[0].rect.x != initial;
        }
        CHECK(varied);
        for (int i = 0; i < 100; ++i) {
            s.traffic[0].rect.y = s.traffic2[0].rect.y = -ctg::racing_car_height + 1;
            two.step_preview();
            CHECK(s.traffic2[0].rect.x == s.traffic[0].rect.x + offset);
        }
        auto layout = s;
        layout.traffic2[0].rect.x = 200;
        two.reset_from_layout(layout);
        CHECK(s.traffic2[0].rect.x == s.traffic[0].rect.x + offset);
    }
    {
        ctg::Racing a, b, noise;
        a.set_random_seed(123); b.set_random_seed(123);
        a.set_two_players(true); b.set_two_players(true); noise.set_two_players(true);
        ctg::RacingInput right{}; right.right = true;
        a.step_preview({}, right); b.step_preview({}, right);
        CHECK(a.state().player2.x == b.state().player2.x);
        auto layout = a.state();
        a.reset_from_layout(layout);
        CHECK(a.state().player2.x == layout.player2.x);
        CHECK(a.state().traffic2[0].rect.x == layout.traffic2[0].rect.x);
        b.reset_from_layout(layout);
        for (int i = 0; i < 500; ++i) {
            noise.step();
            CHECK(a.step({}, right) == b.step({}, right));
            CHECK(a.state().traffic[0].rect.x == b.state().traffic[0].rect.x);
            CHECK(a.state().traffic2[0].rect.x == b.state().traffic2[0].rect.x);
            CHECK(a.state().score2 == b.state().score2);
        }
        for (unsigned i = a.state().frame; i < ctg::Racing::duration_frames; ++i) a.step();
        CHECK(a.state().finished && a.state().frames_remaining == 0);
        CHECK(a.step({}, right) == ctg::racing_none);
    }
    return 0;
}
