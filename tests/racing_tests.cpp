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
    // The first seven entries follow the reported chart; the last two are
    // additional low/low one-player settings exposed by the menu.
    struct Configuration { bool narrow, two_players; unsigned speed; };
    const Configuration configurations[] = {
        {false, false, 1}, {false, false, 2}, {true, false, 1}, {true, false, 2},
        {false, true, 0}, {false, true, 1}, {false, true, 2},
        {false, false, 0}, {true, false, 0}
    };
    for (unsigned group = 0; group < 9; ++group) for (unsigned switches = 0; switches < 16; ++switches) {
        const auto config = configurations[group];
        const bool bad_road = switches & 1, wall = switches & 2;
        const bool more = switches & 4, zigzag = switches & 8;
        ctg::Racing game;
        game.set_random_seed(1234);
        game.set_two_players(config.two_players);
        game.set_narrow(config.narrow);
        game.set_speed_switch(config.speed);
        game.set_more_cars(more);
        game.set_wall_crash(wall);
        game.set_zigzag(zigzag);
        game.set_bad_road(bad_road);
        game.reset();
        const auto& state = game.state();
        CHECK(state.narrow == config.narrow && state.two_players == config.two_players);
        CHECK(state.bad_road == bad_road && state.wall_crash == wall);
        CHECK(state.more_cars == more && state.zigzag == zigzag);
        CHECK(state.frame == 0 && state.frames_remaining == ctg::Racing::duration_frames && !state.finished);
        CHECK(state.speed == ctg::Racing::start_speed && state.score == 0 && state.score2 == 0);
        const unsigned target = config.speed == 0 ? ctg::Racing::low_speed :
            config.speed == 1 ? ctg::Racing::medium_speed : ctg::Racing::max_speed;
        CHECK(state.selected_speed == (config.two_players ? target : ctg::Racing::low_speed));
        CHECK(state.traffic[0].rect.width == ctg::racing_car_width);
        if (more) CHECK(state.traffic[1].rect.width == ctg::racing_car_width);
        if (config.two_players) {
            CHECK(state.traffic2[0].rect.width == ctg::racing_car_width);
            if (more) CHECK(state.traffic2[1].rect.width == ctg::racing_car_width);
        }
        ctg::RacingInput high{}; high.select_high_speed = true;
        game.step(high);
        CHECK(state.frame == 1 && state.frames_remaining == ctg::Racing::duration_frames - 1);
        CHECK(state.selected_speed == target);
        CHECK(state.bad_road == bad_road && state.wall_crash == wall && state.more_cars == more && state.zigzag == zigzag);
    }
    {
        ctg::Racing game;
        CHECK(!game.state().bad_road);
        for (int i = 0; i < 17; ++i) game.step_preview();
        game.set_bad_road(true);
        CHECK(game.state().road_position == game.state().road_scroll);
        game.set_bad_road(false);
        game.set_random_seed(42);
        game.set_bad_road(true);
        game.reset();
        auto& s = const_cast<ctg::RacingState&>(game.state());
        // Across negative and positive world coordinates, all transitions line up
        // with the point three cells before the road-edge gap.
        int transitions = 0, dirt_length = 0, normal_length = 0;
        unsigned gaps = 0;
        bool previous = ctg::racing_dirt_at(s, -100 * 270 + 225);
        for (int section = -99; section < 100; ++section) {
            const int y = section * 270 + 225;
            const bool dirt = ctg::racing_dirt_at(s, y);
            CHECK(ctg::racing_dirt_at(s, y + 269) == dirt);
            if (dirt != previous) {
                CHECK(ctg::racing_dirt_at(s, y - 1) == previous);
                if (!dirt && transitions > 0) CHECK(dirt_length == 5 || dirt_length == 6);
                else if (transitions > 0) { CHECK(normal_length >= 5 && normal_length <= 12); gaps |= 1u << normal_length; }
                ++transitions; dirt_length = normal_length = 0;
            }
            if (dirt) ++dirt_length; else ++normal_length;
            previous = dirt;
        }
        CHECK(transitions >= 18);
        CHECK((gaps & (gaps - 1)) != 0);
        bool multiple_starts = false;
        for (int block = -5; block < 5; ++block) {
            int starts = 0;
            for (int section = block * 20; section < (block + 1) * 20; ++section)
                if (ctg::racing_dirt_at(s, section * 270 + 225) &&
                    !ctg::racing_dirt_at(s, (section - 1) * 270 + 225)) ++starts;
            multiple_starts |= starts > 1;
        }
        CHECK(multiple_starts);
        game.set_two_players(true);
        auto& t = const_cast<ctg::RacingState&>(game.state());
        while (!ctg::racing_dirt_at(t, t.player.y + t.player.height / 2)) t.road_position += 270;
        t.speed = t.selected_speed = 0;
        game.set_car_collision(false);
        const int x1 = t.player.x, x2 = t.player2.x;
        for (int frame = 0; frame < 32; ++frame) {
            const unsigned events = game.step();
            const unsigned wiggles = ctg::racing_wiggle_player1 | ctg::racing_wiggle_player2;
            CHECK((events & wiggles) == (frame > 0 && frame % 8 == 0 ? wiggles : 0));
            const int offset = (frame / 8) % 2 ? -8 : 0;
            CHECK(t.player.x == x1 + offset && t.player2.x == x2 + offset);
        }
        ctg::RacingInput right{}; right.right = true;
        game.step(right, right);
        CHECK(t.player.x == x1 + 6 && t.player2.x == x2 + 6);
        auto layout = t;
        ctg::Racing resumed;
        resumed.set_two_players(true); resumed.set_bad_road(true);
        resumed.reset_from_layout(layout);
        CHECK(resumed.state().road_position == layout.road_position && resumed.state().road_seed == layout.road_seed);
        game.set_bad_road(false);
        CHECK(!t.bad_road && t.player.x == x1 + 6);
        game.reset(); CHECK(!game.state().bad_road);
    }
    {
        ctg::Racing game;
        game.set_two_players(true); game.set_bad_road(true); game.set_car_collision(false);
        auto& s = const_cast<ctg::RacingState&>(game.state());
        while (!ctg::racing_dirt_at(s, s.player.y + s.player.height / 2)) s.road_position += 270;
        s.speed = s.selected_speed = 0;
        s.player.x = s.road.x;
        for (int frame = 0; frame < 32; ++frame) {
            const auto events = game.step();
            CHECK(!(events & ctg::racing_wiggle_player1));
            CHECK(bool(events & ctg::racing_wiggle_player2) == (frame > 0 && frame % 8 == 0));
        }
    }
    {
        for (bool shifted : {false, true}) {
            ctg::Racing game;
            game.set_two_players(true); game.set_bad_road(true); game.set_car_collision(false);
            auto& s = const_cast<ctg::RacingState&>(game.state());
            while (!ctg::racing_dirt_at(s, s.player.y + s.player.height / 2)) s.road_position += 270;
            s.speed = s.selected_speed = 0;
            s.player.x = s.road.x; // Clipped at the edge; only player 2 can move.
            const int x2 = s.player2.x;
            for (int i = 0; i < (shifted ? 9 : 4); ++i) game.step();
            CHECK(s.player2.x == x2 - (shifted ? 8 : 0));
            while (ctg::racing_dirt_at(s, s.player.y + s.player.height / 2)) s.road_position += 270;
            const auto events = game.step();
            CHECK(!(events & ctg::racing_wiggle_player1));
            CHECK(bool(events & ctg::racing_wiggle_player2) == shifted);
            CHECK(s.player2.x == x2);
            CHECK(!(game.step() & (ctg::racing_wiggle_player1 | ctg::racing_wiggle_player2)));
        }
    }
    {
        ctg::Racing game;
        game.set_bad_road(true);
        auto& s = const_cast<ctg::RacingState&>(game.state());
        while (!ctg::racing_dirt_at(s, s.player.y + s.player.height / 2)) s.road_position += 270;
        s.speed = s.selected_speed = 0;
        s.frames_remaining = ctg::Racing::duration_frames - 100;
        s.traffic[0].rect = {s.player.x - 20, s.player.y, ctg::racing_car_width, ctg::racing_car_height};
        const int traffic_x = s.traffic[0].rect.x;
        for (int i = 0; i < 8; ++i) CHECK(!(game.step() & ctg::racing_collision));
        CHECK(game.step() & ctg::racing_collision);
        CHECK(s.traffic[0].rect.x == traffic_x && s.blink_samples != 0);
        CHECK(!(game.step() & ctg::racing_collision));
    }
    {
        ctg::Racing game;
        game.set_bad_road(true); game.set_wall_crash(true); game.set_car_collision(false);
        auto& s = const_cast<ctg::RacingState&>(game.state());
        while (!ctg::racing_dirt_at(s, s.player.y + s.player.height / 2)) s.road_position += 270;
        s.speed = s.selected_speed = 0;
        s.frames_remaining = ctg::Racing::duration_frames - 100;
        s.player.x = s.road.x + ctg::racing_cell_width;
        for (int i = 0; i < 8; ++i) CHECK(!(game.step() & ctg::racing_collision));
        CHECK(game.step() & ctg::racing_collision);
        CHECK(s.blink_samples != 0);
        CHECK(!(game.step() & ctg::racing_collision));
    }
    {
        ctg::Racing a, b, preview;
        a.set_random_seed(77); b.set_random_seed(77); preview.set_random_seed(77);
        a.set_bad_road(true); preview.set_bad_road(true);
        a.set_car_collision(false); b.set_car_collision(false);
        for (int i = 0; i < 500; ++i) preview.step_preview();
        a.reset(); b.reset();
        CHECK(a.state().traffic[0].rect.x == b.state().traffic[0].rect.x);
        for (int i = 0; i < 2000; ++i) {
            a.step(); b.step(); preview.step_preview();
            CHECK(a.state().traffic[0].rect.x == b.state().traffic[0].rect.x);
            CHECK(a.state().traffic[0].rect.y == b.state().traffic[0].rect.y);
        }
    }
    // Consecutive sections have independent phases and corresponding 2P lanes match.
    for (bool two : {false, true}) for (bool preview_steps : {false, true}) {
        ctg::Racing game, twin;
        for (auto* g : {&game, &twin}) {
            g->set_narrow(!two);
            g->set_two_players(two);
            g->set_more_cars(true);
            g->set_zigzag(true);
            g->set_car_collision(false);
            g->set_random_seed(772);
            g->reset();
        }
        unsigned distinct = 0, wraps[2] = {}, varied[2] = {};
        int last_spawn[2] = {-1, -1};
        for (int frame = 0; frame < 2500; ++frame) {
            const auto before = game.state();
            if (preview_steps) { game.step_preview(); twin.step_preview(); }
            else { game.step(); twin.step(); }
            const auto& s = game.state();
            CHECK(s.path_position == twin.state().path_position);
            CHECK(s.extra_path_position == twin.state().extra_path_position);
            for (unsigned i = 0; i < 2; ++i) {
                const int position = i ? s.extra_path_position : s.path_position;
                CHECK(position >= 0 && position <= 20 * ctg::racing_cell_width * 60);
                CHECK(s.traffic[i].rect.x == ctg::racing_x(0) + position / 60);
                if (two) {
                    CHECK(s.traffic2[i].rect.x == s.traffic[i].rect.x + 26 * ctg::racing_cell_width);
                    CHECK(s.traffic2[i].rect.y == s.traffic[i].rect.y);
                }
                const int dy = s.traffic[i].rect.y - before.traffic[i].rect.y;
                if ((preview_steps && dy > 0) || (!preview_steps && dy < 0)) {
                    ++wraps[i];
                    if (last_spawn[i] >= 0 && last_spawn[i] != position) ++varied[i];
                    last_spawn[i] = position;
                }
            }
            if (s.traffic[0].rect.y >= 0 && s.traffic[0].rect.y < 240 &&
                s.traffic[1].rect.y >= 0 && s.traffic[1].rect.y < 240 &&
                s.traffic[0].rect.x != s.traffic[1].rect.x) ++distinct;
            if (frame == 300) {
                ctg::Racing resumed;
                resumed.set_narrow(!two); resumed.set_two_players(two);
                resumed.set_more_cars(true); resumed.set_zigzag(true);
                resumed.reset_from_layout(s);
                CHECK(resumed.state().path_position == s.path_position);
                CHECK(resumed.state().path_direction == s.path_direction);
                CHECK(resumed.state().extra_path_position == s.extra_path_position);
                CHECK(resumed.state().extra_path_direction == s.extra_path_direction);
                for (unsigned i = 0; i < 2; ++i) {
                    CHECK(resumed.state().traffic[i].rect.x == s.traffic[i].rect.x);
                    CHECK(resumed.state().traffic[i].rect.y == s.traffic[i].rect.y);
                    if (two) CHECK(resumed.state().traffic2[i].rect.x == s.traffic2[i].rect.x);
                }
            }
        }
        CHECK(distinct > 10);
        CHECK(wraps[0] > 2 && wraps[1] > 2);
        CHECK(varied[0] > 0 && varied[1] > 0);
    }
    // A seeded zigzag reset selects bounded starting paths in every layout.
    for (unsigned mode : {0u, 1u, 2u}) for (bool more : {false, true}) {
        ctg::Racing a, b, preview;
        for (auto* game : {&a, &b, &preview}) {
            game->set_two_players(mode == 2);
            game->set_narrow(mode == 1);
            game->set_more_cars(more);
            game->set_zigzag(true);
            game->set_car_collision(false);
        }
        a.set_random_seed(123); b.set_random_seed(123);
        const int limit = (mode != 0 || more ? 20 : 47) * ctg::racing_cell_width;
        bool varied = false;
        int first = -1;
        for (int n = 0; n < 64; ++n) {
            a.reset(); b.reset();
            const auto& s = a.state();
            CHECK(s.path_position == b.state().path_position);
            CHECK(s.path_position >= 0 && s.path_position <= limit * 60);
            CHECK(s.traffic[0].rect.x == ctg::racing_x(0) + s.path_position / 60);
            CHECK(s.path_direction == (s.path_position == limit * 60 ? -1 : 1));
            if (n == 0) first = s.path_position;
            else varied |= s.path_position != first;
            if (more) CHECK(s.traffic[1].rect.x == (mode == 0 ? s.traffic[0].rect.x + 26 * ctg::racing_cell_width : ctg::racing_x(0) + s.extra_path_position / 60));
            if (mode == 2) for (unsigned i = 0; i < (more ? 2u : 1u); ++i)
                CHECK(s.traffic2[i].rect.x == s.traffic[i].rect.x + 26 * ctg::racing_cell_width);
        }
        CHECK(varied);
        a.set_random_seed(123); a.reset();
        CHECK(a.state().path_position == first);

        // Preview transfers its phase, but its RNG activity cannot perturb gameplay's stream.
        preview.set_random_seed(987);
        preview.reset_preview();
        for (int i = 0; i < 173; ++i) preview.step_preview();
        const auto layout = preview.state();
        a.set_random_seed(456); b.set_random_seed(456);
        a.reset_from_layout(layout); b.reset_from_layout(layout);
        CHECK(a.state().path_position == layout.path_position);
        CHECK(a.state().path_direction == layout.path_direction);
        for (int i = 0; i < 300; ++i) preview.step_preview();
        a.reset(); b.reset();
        CHECK(a.state().path_position == b.state().path_position);

        // A newly spawned section remains bounded and paired lanes remain synchronized.
        auto& s = const_cast<ctg::RacingState&>(a.state());
        s.traffic[0].rect.y = s.road.height;
        if (mode == 2) s.traffic2[0].rect.y = s.road.height;
        a.step();
        CHECK(s.path_position >= 0 && s.path_position <= limit * 60);
        if (more) CHECK(s.traffic[1].rect.x == (mode == 0 ? s.traffic[0].rect.x + 26 * ctg::racing_cell_width : ctg::racing_x(0) + s.extra_path_position / 60));
        if (mode == 2) CHECK(s.traffic2[0].rect.x == s.traffic[0].rect.x + 26 * ctg::racing_cell_width);
    }
    {
        ctg::Racing game;
        game.set_car_collision(false);
        game.set_zigzag(true);
        auto& state = const_cast<ctg::RacingState&>(game.state());
        {
            for (int column : {6, 34}) for (int direction : {-1, 1}) {
                bool reversed = false, continued = false;
                for (unsigned seed = 0; seed < 64; ++seed) {
                    game.set_random_seed(seed);
                    state.path_position = column * ctg::racing_cell_width * 60 - direction;
                    state.path_direction = direction;
                    game.step_preview();
                    reversed |= state.path_direction != direction;
                    continued |= state.path_direction == direction;
                }
                CHECK(continued);
                CHECK(reversed == (column != 6 || direction < 0));
            }
        }
    }
    {
        for (unsigned mode : {0u, 1u, 2u}) for (bool more_cars : {false, true})
        for (bool preview : {false, true}) for (unsigned seed : {0u, 1u, 42u}) {
            if (mode == 0 && !more_cars) continue;
            const bool two_players = mode == 2;
            ctg::Racing game;
            game.set_narrow(mode == 1);
            game.set_two_players(two_players);
            game.set_more_cars(more_cars);
            game.set_car_collision(false);
            game.set_zigzag(true);
            game.set_random_seed(seed);
            auto& state = const_cast<ctg::RacingState&>(game.state());
            constexpr int increment = 20 * ctg::racing_cell_width;
            constexpr int limit = increment * 60;
            // Force an unobstructed path each frame to verify its speed and endpoint bounce.
            for (int frame = 1; frame <= 120; ++frame) {
                state.path_position = (frame <= 60 ? frame - 1 : 121 - frame) * increment;
                state.path_direction = frame <= 60 ? 1 : -1;
                // Keep the tested section on screen to isolate its traversal from spawning.
                state.traffic[0].rect.y = 50;
                if (mode == 0 && more_cars) state.traffic[1].rect.y = 50;
                if (two_players) state.traffic2[0].rect.y = 50;
                if (preview) game.step_preview(); else game.step();
                CHECK(state.path_position == (frame <= 60 ? frame : 120 - frame) * increment);
                if (frame == 60 || frame == 120)
                    CHECK(state.path_direction == (frame == 60 ? -1 : 1));
                CHECK(state.traffic[0].rect.x == ctg::racing_x(0) + state.path_position / 60);
                if (more_cars) {
                    CHECK(state.traffic[1].rect.x == (mode == 0 ? state.traffic[0].rect.x + 26 * ctg::racing_cell_width : ctg::racing_x(0) + state.extra_path_position / 60));
                    if (mode == 0) CHECK(state.traffic[1].rect.y == state.traffic[0].rect.y);
                }
                if (two_players) {
                    CHECK(state.traffic2[0].rect.x == state.traffic[0].rect.x + 26 * ctg::racing_cell_width);
                    if (more_cars) CHECK(state.traffic2[1].rect.x == state.traffic[1].rect.x + 26 * ctg::racing_cell_width);
                }
                CHECK(state.path_position >= 0 && state.path_position <= limit);
            }
        }
    }
    // Both independent paths use the paired intermediate rules in gameplay and preview.
    for (unsigned mode : {0u, 1u, 2u}) for (bool more : {false, true}) {
        if (mode == 0 && !more) continue;
        for (bool preview : {false, true}) for (unsigned path = 0; path < (more && mode != 0 ? 2u : 1u); ++path)
        for (int column : {2, 6}) for (int direction : {-1, 1}) {
            ctg::Racing game;
            game.set_narrow(mode == 1); game.set_two_players(mode == 2);
            game.set_more_cars(more); game.set_car_collision(false); game.set_zigzag(true);
            auto& state = const_cast<ctg::RacingState&>(game.state());
            bool reversed = false, continued = false;
            for (unsigned seed = 0; seed < 64; ++seed) {
                game.set_random_seed(seed);
                state.traffic[0].rect.y = 50;
                state.traffic[1].rect.y = 50;
                state.traffic2[0].rect.y = state.traffic2[1].rect.y = 50;
                int& position = path ? state.extra_path_position : state.path_position;
                int& heading = path ? state.extra_path_direction : state.path_direction;
                position = column * ctg::racing_cell_width * 60 - direction;
                heading = direction;
                // Keep the other path away from its own decision points.
                (path ? state.path_position : state.extra_path_position) = 10 * ctg::racing_cell_width * 60;
                (path ? state.path_direction : state.extra_path_direction) = 1;
                if (preview) game.step_preview(); else game.step();
                reversed |= heading != direction;
                continued |= heading == direction;
                CHECK(position == column * ctg::racing_cell_width * 60 + heading * (20 * ctg::racing_cell_width - 1));
            }
            CHECK(continued);
            CHECK(reversed == (column != 6 || direction < 0));
        }
    }
    {
        ctg::Racing game;
        game.set_car_collision(false);
        game.set_zigzag(true);
        auto& state = const_cast<ctg::RacingState&>(game.state());
        for (int i = 0; i < 60; ++i) {
            state.path_position = i * 47 * ctg::racing_cell_width;
            state.path_direction = 1;
            game.step();
        }
        CHECK(state.path_position == 47 * ctg::racing_cell_width * 60);
        CHECK(state.path_direction == -1);
        CHECK(state.traffic[0].rect.x + state.traffic[0].rect.width == ctg::racing_x(53));
        game.set_more_cars(true);
        for (int i = 0; i < 300; ++i) {
            game.step();
            CHECK(state.traffic[1].rect.x - state.traffic[0].rect.x == 26 * ctg::racing_cell_width);
            CHECK(state.traffic[0].rect.x >= ctg::racing_x(0));
            CHECK(state.traffic[0].rect.x + state.traffic[0].rect.width <= ctg::racing_x(26));
        }
        game.set_two_players(true);
        for (int i = 0; i < 100; ++i) {
            game.step_preview();
            CHECK(state.traffic2[0].rect.x - state.traffic[0].rect.x == 26 * ctg::racing_cell_width);
            CHECK(state.traffic2[1].rect.x - state.traffic[1].rect.x == 26 * ctg::racing_cell_width);
        }
        game.set_zigzag(false);
        CHECK(!state.zigzag);
    }
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
    // Countdown preview does not consume the active-play grace period. Suppressed
    // contacts remain eligible for a real crash after the exact 60-frame window.
    for (bool two : {false, true}) for (bool walls : {false, true}) {
        ctg::Racing game;
        game.set_two_players(two);
        game.set_wall_crash(walls);
        auto& s = const_cast<ctg::RacingState&>(game.state());
        for (unsigned i = 0; i < 180; ++i) game.step_preview();
        CHECK(s.frames_remaining == ctg::Racing::duration_frames);
        for (unsigned i = 0; i < ctg::Racing::frames_per_second; ++i) {
            s.traffic[0].rect = s.player;
            if (two) s.traffic2[0].rect = s.player2;
            if (i == 0) { s.road_scroll = s.road.height - 1; s.movement_remainder = 539; }
            const unsigned speed = s.speed;
            const unsigned events = game.step();
            CHECK(!(events & ctg::racing_collision));
            CHECK(s.speed >= speed && !s.blink_samples && !s.blink_samples2);
            CHECK(!s.collisions && !s.collisions2 && !s.section_collided && !s.section_collided2);
            CHECK(!s.traffic[0].collided && (!two || !s.traffic2[0].collided));
            if (i == 0) CHECK((events & ctg::racing_pass) && s.score == 1 && (!two || s.score2 == 1));
        }
        s.traffic[0].rect = s.player;
        if (two) s.traffic2[0].rect = s.player2;
        CHECK(game.step() & ctg::racing_collision);
        CHECK(s.collisions == 1 && (!two || s.collisions2 == 1));
        const auto layout = s;
        game.reset_from_layout(layout);
        s.traffic[0].rect = s.player;
        CHECK(!(game.step() & ctg::racing_collision));
        game.reset();
        s.traffic[0].rect = s.player;
        CHECK(!(game.step() & ctg::racing_collision));
    }
    for (bool two : {false, true}) {
        ctg::Racing game;
        game.set_two_players(two);
        game.set_wall_crash(true);
        game.set_car_collision(false);
        auto& s = const_cast<ctg::RacingState&>(game.state());
        s.player.x = s.road.x;
        if (two) s.player2.x = s.road.x + s.road.width - s.player2.width;
        for (unsigned i = 0; i < ctg::Racing::frames_per_second; ++i) {
            CHECK(!(game.step() & ctg::racing_collision));
            CHECK(s.player.x == s.road.x && (!two || s.player2.x == s.road.x + s.road.width - s.player2.width));
            CHECK(!s.blink_samples && !s.blink_samples2 && !s.section_collided && !s.section_collided2);
        }
        CHECK(game.step() & ctg::racing_collision);
        CHECK(s.collisions == 1 && (!two || s.collisions2 == 1));
    }
    // Recovery protects each player from additional contacts regardless of wall setting.
    for (int mode = 0; mode < 3; ++mode) for (bool walls : {false, true}) {
        ctg::Racing game;
        game.set_narrow(mode == 1);
        game.set_two_players(mode == 2);
        game.set_more_cars(true);
        game.set_wall_crash(walls);
        auto& s = const_cast<ctg::RacingState&>(game.state());
        s.frames_remaining -= ctg::Racing::frames_per_second;
        s.traffic[0].rect = s.traffic[1].rect = s.player;
        s.traffic2[0].rect.y = s.traffic2[1].rect.y = -100;
        CHECK(game.step() & ctg::racing_collision);
        CHECK(s.collisions == 1 && s.traffic[0].collided && !s.traffic[1].collided);
        CHECK(s.blink_samples == ctg::racing_blink_total_samples);
        s.traffic[1].rect = s.player;
        ctg::RacingInput recovering{}; recovering.recovering = true;
        CHECK(!(game.step(recovering) & ctg::racing_collision));
        CHECK(s.collisions == 1 && !s.traffic[1].collided && s.speed == ctg::Racing::start_speed);
        if (mode == 2) {
            s.traffic2[0].rect = s.player2;
            CHECK(game.step(recovering) & ctg::racing_collision);
            CHECK(s.collisions == 1 && s.collisions2 == 1 && s.speed == ctg::Racing::start_speed);
            CHECK(s.blink_samples2 > s.blink_samples);
        }
        if (walls) {
            s.player.x = s.road.x;
            CHECK(!(game.step(recovering) & ctg::racing_collision));
            CHECK(s.collisions == 1 && s.player.x > s.road.x);
            s.player.x = s.road.x + ctg::racing_cell_width;
        }
        while (s.blink_samples > ctg::racing_blink_frame_samples) game.step(recovering);
        s.traffic[1].rect = s.player;
        if (mode != 2) {
            CHECK(!(game.step(recovering) & ctg::racing_collision));
            CHECK(s.blink_samples == 0 && s.collisions == 1 && !s.traffic[1].collided);
            s.traffic[1].rect = s.player;
        }
        CHECK(game.step() & ctg::racing_collision);
        CHECK(s.collisions == 2 && s.traffic[1].collided);
    }
    // Each car loses one cell on both sides; half-open edges do not collide.
    const int hitbox_width = ctg::racing_car_width - 2 * ctg::racing_cell_width;
    for (int mode = 0; mode < 3; ++mode) for (int lane = 0; lane < (mode == 2 ? 2 : 1); ++lane)
        for (int opponent = 0; opponent < 2; ++opponent) for (int sign : {-1, 1})
            for (int dx : {hitbox_width - 1, hitbox_width, hitbox_width + ctg::racing_cell_width,
                           ctg::racing_car_width - 1, ctg::racing_car_width}) {
                ctg::Racing game;
                game.set_narrow(mode == 1);
                game.set_two_players(mode == 2);
                game.set_more_cars(true);
                auto& s = const_cast<ctg::RacingState&>(game.state());
                s.frames_remaining -= ctg::Racing::frames_per_second;
                for (auto& car : s.traffic) car.rect.y = -100;
                for (auto& car : s.traffic2) car.rect.y = -100;
                const auto& player = lane ? s.player2 : s.player;
                auto& car = lane ? s.traffic2[opponent] : s.traffic[opponent];
                car.rect = {player.x + sign * dx, player.y, ctg::racing_car_width, ctg::racing_car_height};
                const bool hit = dx < hitbox_width;
                CHECK(!!(game.step() & ctg::racing_collision) == hit);
                CHECK(car.collided == hit);
                CHECK((lane ? s.collisions2 : s.collisions) == unsigned(hit));
            }
    for (int dy : {ctg::racing_car_height - 1, ctg::racing_car_height}) {
        ctg::Racing game;
        auto& s = const_cast<ctg::RacingState&>(game.state());
        s.frames_remaining -= ctg::Racing::frames_per_second;
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
    CHECK(game.state().traffic.size() == 2 && !game.state().more_cars);
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
        s.frames_remaining -= ctg::Racing::frames_per_second;
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
    {
        constexpr int offset = ctg::racing_center_column * ctg::racing_cell_width;
        constexpr int half = (ctg::racing_road_dash_length + ctg::racing_road_gap_length) / 2;
        ctg::Racing game;
        game.set_random_seed(42);
        game.set_more_cars(true);
        CHECK(game.state().more_cars && game.state().frame == 0);
        CHECK(game.state().traffic[1].rect.x == game.state().traffic[0].rect.x + offset);
        CHECK(game.state().traffic[1].rect.y == game.state().traffic[0].rect.y);
        game.set_car_collision(false);
        for (int i = 0; i < 1000; ++i) {
            game.step_preview();
            CHECK(game.state().traffic[1].rect.x == game.state().traffic[0].rect.x + offset);
            CHECK(game.state().traffic[1].rect.y == game.state().traffic[0].rect.y);
        }
        auto layout = game.state();
        layout.traffic[1].rect.x = 0;
        layout.traffic[1].rect.y = 0;
        game.reset_from_layout(layout);
        CHECK(game.state().traffic[1].rect.x == game.state().traffic[0].rect.x + offset);
        CHECK(game.state().traffic[1].rect.y == game.state().traffic[0].rect.y);
        for (int i = 0; i < 1000; ++i) {
            game.step();
            CHECK(game.state().traffic[1].rect.x == game.state().traffic[0].rect.x + offset);
            CHECK(game.state().traffic[1].rect.y == game.state().traffic[0].rect.y);
        }
        game.set_narrow(true);
        CHECK(game.state().traffic[1].rect.y == game.state().traffic[0].rect.y - half);
        game.reset_preview();
        CHECK(game.state().more_cars && game.state().narrow);
        CHECK(game.state().traffic[1].rect.y == game.state().traffic[0].rect.y - half);
        for (int i = 0; i < 500; ++i) {
            game.step_preview();
            CHECK((game.state().traffic[0].rect.y - game.state().traffic[1].rect.y + 2 * half) % (2 * half) == half);
        }
        game.set_two_players(true);
        CHECK(game.state().more_cars && game.state().two_players);
        CHECK(game.state().traffic[1].rect.y == game.state().traffic[0].rect.y - half);
        for (int i = 0; i < 1000; ++i) {
            if (i % 2) game.step(); else game.step_preview();
            for (unsigned j = 0; j < 2; ++j) {
                CHECK(game.state().traffic2[j].rect.x == game.state().traffic[j].rect.x + offset);
                CHECK(game.state().traffic2[j].rect.y == game.state().traffic[j].rect.y);
            }
        }
        layout = game.state();
        layout.traffic2[1].rect.y = 123;
        game.reset_from_layout(layout);
        CHECK(game.state().traffic2[1].rect.y == game.state().traffic[1].rect.y);
        game.set_more_cars(false);
        CHECK(!game.state().more_cars && game.state().frame == 0);
        game.reset();
        CHECK(!game.state().more_cars);
    }
    {
        ctg::Racing game;
        game.set_two_players(true);
        game.set_more_cars(true);
        auto& s = const_cast<ctg::RacingState&>(game.state());
        s.frames_remaining -= ctg::Racing::frames_per_second;
        s.traffic[0].rect.y = s.traffic2[0].rect.y = -100;
        s.traffic[1].rect = s.player;
        s.traffic2[1].rect.y = -100;
        CHECK(game.step() & ctg::racing_collision);
        CHECK(s.collisions == 1 && s.collisions2 == 0 && s.section_collided);
        s.road_scroll = s.road.height - 1;
        s.movement_remainder = 539;
        CHECK(game.step() & ctg::racing_pass);
        CHECK(s.score == 0 && s.score2 == 1 && !s.section_collided);
        s.road_scroll = s.road.height - 1;
        s.movement_remainder = 539;
        game.step();
        CHECK(s.score == 1 && s.score2 == 2);
        s.traffic2[1].rect = s.player2;
        CHECK(game.step() & ctg::racing_collision);
        CHECK(s.collisions2 == 1 && s.section_collided2);
        s.road_scroll = s.road.height - 1;
        s.movement_remainder = 539;
        game.step();
        CHECK(s.score == 2 && s.score2 == 2);
    }
    {
        // Boundary cells are solid for collision purposes, even in visual dash gaps.
        ctg::Racing game;
        game.set_car_collision(false);
        auto& s = const_cast<ctg::RacingState&>(game.state());
        s.player.x = s.road.x;
        CHECK(!(game.step() & ctg::racing_collision));
        CHECK(s.player.x == s.road.x && s.collisions == 0);
        game.set_wall_crash(true);
        game.reset();
        CHECK(s.wall_crash);
        s.player.x = s.road.x;
        s.road_scroll = 245;
        game.step_preview();
        CHECK(s.player.x == s.road.x && s.collisions == 0);
        s.frames_remaining -= ctg::Racing::frames_per_second;
        CHECK(game.step() & ctg::racing_collision);
        CHECK(s.player.x == s.road.x + 1);
        CHECK(s.collisions == 1 && s.section_collided && s.speed == ctg::Racing::start_speed);
        ctg::RacingInput left{}; left.left = true; left.recovering = true;
        for (int i = 0; i < 10; ++i) CHECK(!(game.step(left) & ctg::racing_collision));
        CHECK(s.player.x >= s.road.x && s.collisions == 1);
        s.road_scroll = s.road.height - 1; s.movement_remainder = 539;
        CHECK(!(game.step(left) & ctg::racing_pass));
        CHECK(s.score == 0 && !s.section_collided);
        game.reset();
        s.frames_remaining -= ctg::Racing::frames_per_second;
        s.player.x = s.road.x + s.road.width - s.player.width;
        CHECK(game.step() & ctg::racing_collision);
        CHECK(s.player.x + s.player.width == s.road.x + s.road.width - 1);
        game.reset();
        s.player.x = ctg::racing_x(ctg::racing_center_column);
        CHECK(!(game.step() & ctg::racing_collision));
        game.set_narrow(true);
        s.frames_remaining -= ctg::Racing::frames_per_second;
        s.player.x = ctg::racing_x(ctg::racing_center_column) + ctg::racing_cell_width - s.player.width;
        CHECK(game.step() & ctg::racing_collision);
        CHECK(s.player.x + s.player.width == ctg::racing_x(ctg::racing_center_column) + ctg::racing_cell_width - 1);
        game.set_wall_crash(false);
        game.reset();
        CHECK(!s.wall_crash);
    }
    {
        ctg::Racing game;
        game.set_wall_crash(true);
        game.set_two_players(true);
        auto& s = const_cast<ctg::RacingState&>(game.state());
        s.frames_remaining -= ctg::Racing::frames_per_second;
        s.traffic[0].rect.y = s.traffic2[0].rect.y = -100;
        s.player.x = ctg::racing_x(ctg::racing_center_column) + ctg::racing_cell_width - s.player.width;
        s.player2.x = s.road.x + s.road.width - s.player2.width;
        CHECK(game.step() & ctg::racing_collision);
        CHECK(s.collisions == 1 && s.collisions2 == 1);
        CHECK(s.player.x + s.player.width == ctg::racing_x(ctg::racing_center_column) + ctg::racing_cell_width - 1);
        CHECK(s.player2.x + s.player2.width == s.road.x + s.road.width - 1);
        CHECK(s.blink_samples == ctg::racing_blink_total_samples && s.blink_samples2 == s.blink_samples);
        game.reset();
        s.frames_remaining -= ctg::Racing::frames_per_second;
        s.traffic[0].rect = s.player;
        s.traffic2[0].rect.y = -100;
        CHECK(game.step() & ctg::racing_collision);
        s.player.x = s.road.x;
        s.player2.x = ctg::racing_x(ctg::racing_center_column);
        ctg::RacingInput recovering{}; recovering.recovering = true;
        CHECK(game.step(recovering) & ctg::racing_collision);
        CHECK(s.collisions == 1 && s.collisions2 == 1);
        CHECK(s.player.x == s.road.x + 1);
        CHECK(s.player2.x == ctg::racing_x(ctg::racing_center_column) + 1);
        CHECK(s.speed == ctg::Racing::start_speed);
        s.traffic2[0].rect = s.player2;
        CHECK(!(game.step(recovering) & ctg::racing_collision));
        CHECK(s.collisions2 == 1 && !s.traffic2[0].collided);
        game.set_car_collision(false);
        for (unsigned i = 0; i < 98; ++i) game.step(recovering);
        game.set_car_collision(true);
        s.traffic2[0].rect = s.player2;
        CHECK(game.step() & ctg::racing_collision);
        CHECK(s.collisions2 == 2);
    }
    {
        ctg::Racing game;
        game.set_wall_crash(true);
        auto& s = const_cast<ctg::RacingState&>(game.state());
        s.frames_remaining -= ctg::Racing::frames_per_second;
        s.player.x = s.road.x;
        s.traffic[0].rect.y = -100;
        CHECK(game.step() & ctg::racing_collision);
        s.traffic[0].rect = s.player;
        CHECK(!(game.step() & ctg::racing_collision));
        CHECK(s.collisions == 1 && !s.traffic[0].collided);
        for (unsigned i = 0; i < 98; ++i) game.step();
        s.traffic[0].rect = s.player;
        ctg::RacingInput recovery{}; recovery.recovering = true;
        CHECK(!(game.step(recovery) & ctg::racing_collision));
        CHECK(game.step() & ctg::racing_collision);
        CHECK(s.collisions == 2);
    }
    {
        // Both boundaries in each lane: held steering cannot prevent recovery,
        // and steering away works normally once the car is clear.
        for (int mode = 0; mode < 3; ++mode) {
            for (int side = 0; side < 2; ++side) {
                ctg::Racing game;
                game.set_wall_crash(true);
                game.set_car_collision(false);
                if (mode == 1) game.set_narrow(true);
                if (mode == 2) game.set_two_players(true);
                auto& s = const_cast<ctg::RacingState&>(game.state());
                const int count = mode == 2 ? 2 : 1;
                for (int player = 0; player < count; ++player) {
                    game.reset();
                    s.frames_remaining -= ctg::Racing::frames_per_second;
                    auto& car = player ? s.player2 : s.player;
                    const int left = player ? ctg::racing_x(ctg::racing_center_column) : s.road.x;
                    const int right = player || mode == 0 ? s.road.x + s.road.width :
                                      ctg::racing_x(ctg::racing_center_column) + ctg::racing_cell_width;
                    car.x = side == 0 ? left : right - car.width;
                    const int initial = car.x;
                    const int direction = side == 0 ? 1 : -1;
                    ctg::RacingInput held{};
                    held.left = side == 0; held.right = side == 1;
                    for (int frame = 0; frame < ctg::racing_cell_width; ++frame) {
                        const unsigned events = player ? game.step({}, held) : game.step(held);
                        CHECK(!!(events & ctg::racing_collision) == (frame == 0));
                        CHECK(car.x == initial + direction * (frame + 1));
                    }
                    CHECK((player ? s.collisions2 : s.collisions) == 1);
                    ctg::RacingInput away{};
                    away.right = side == 0; away.left = side == 1;
                    const int cleared = car.x;
                    CHECK(!((player ? game.step({}, away) : game.step(away)) & ctg::racing_collision));
                    CHECK(car.x == cleared + direction * 6);
                    for (int frame = 0; frame < 80; ++frame)
                        CHECK(!((player ? game.step({}, held) : game.step(held)) & ctg::racing_collision));
                    CHECK((player ? s.collisions2 : s.collisions) == 1);
                }
            }
        }
    }
    return 0;
}
