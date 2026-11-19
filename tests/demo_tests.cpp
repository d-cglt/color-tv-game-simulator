#include "demo.h"
#include "game_catalog.h"
#include <cstdio>
#include <tuple>
#include <utility>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

namespace {
// Literal original simulator, independent of Demo and the shared constructor.
struct Original {
    ctg::TennisB demo_tennis;
    ctg::Shooting demo_shooting;
    ctg::Racing demo_racing;
    std::array<ctg::Opponent, 2> demo_opponents;
    unsigned demo_id = ctg::slot_count, demo_frame = 0;
    bool paddle_movement_rebound = false, anti_stalemate = true;
    static constexpr unsigned sound_mask = ctg::sound_wall | ctg::sound_paddle | ctg::sound_point | ctg::sound_win | ctg::sound_shot;
    void reset() { demo_id = ctg::slot_count; demo_frame = 0; }
    unsigned step(unsigned id) {
    if (demo_id != id) {
        demo_id = id;
        demo_frame = 0;
        demo_tennis = ctg::TennisB(id % 2 == 1 && id < 14,
                                   id >= 4 && id <= 7, id >= 8 && id <= 11,
                                   id <= 1, id == 4 || id == 5,
                                   id == 8 || id == 9, id == 12 || id == 13);
        demo_tennis.set_rebound_mode(paddle_movement_rebound);
        demo_tennis.set_anti_stalemate(anti_stalemate);
        demo_racing.reset_preview();
        demo_shooting = ctg::Shooting{};
        demo_shooting.set_options(false, {true, false});
        demo_shooting.set_target_behavior(11, 1, true);
        for (auto& ai : demo_opponents) ai.reset();
    }
    unsigned events = 0;
    if (id == 15) {
        demo_racing.step_preview();
    } else if (id == 14) {
        const auto& state = demo_shooting.state();
        if (state.finished) demo_shooting.reset();
        const int delta = demo_shooting.state().target_y + 16 - demo_shooting.state().shooter_y;
        events = demo_shooting.step((delta > 3) - (delta < -3), demo_frame % 45 == 0, 0, true, false);
    } else {
        auto mirrored = demo_tennis.state();
        mirrored.ball_x = ctg::TennisB::width - mirrored.ball_x;
        mirrored.ball_vx = -mirrored.ball_vx;
        std::swap(mirrored.paddle_y[0], mirrored.paddle_y[1]);
        std::swap(mirrored.small_paddle[0], mirrored.small_paddle[1]);
        const unsigned difficulty = (demo_frame / 180) % 2 == 0 ? 3 : 1;
        const int left = demo_opponents[0].step(mirrored, difficulty);
        const int right = demo_opponents[1].step(demo_tennis.state(), difficulty);
        events = demo_tennis.step({left, right, {false, false}});
        if (events & ctg::sound_point) {
            demo_tennis.reset();
            for (auto& ai : demo_opponents) ai.reset();
        }
    }
    events &= sound_mask;
    ++demo_frame;
    return events;
}
 };
bool same(const ctg::TennisState& a, const ctg::TennisState& b) {
    return std::tie(a.ball_x, a.ball_y, a.ball_vx, a.ball_vy, a.paddle_y, a.high_speed, a.small_paddle, a.doubles, a.hockey, a.hockey_a, a.hockey_barrier_closed, a.volleyball, a.volleyball_a, a.tennis_a, a.ping_pong, a.score, a.winner, a.serve_wait) ==
           std::tie(b.ball_x, b.ball_y, b.ball_vx, b.ball_vy, b.paddle_y, b.high_speed, b.small_paddle, b.doubles, b.hockey, b.hockey_a, b.hockey_barrier_closed, b.volleyball, b.volleyball_a, b.tennis_a, b.ping_pong, b.score, b.winner, b.serve_wait);
}
bool same(const ctg::ShootingState& a, const ctg::ShootingState& b) {
    return std::tie(a.shooter_y, a.target_y, a.shot_x, a.shot_y, a.shots, a.hits, a.frame, a.flying, a.hit, a.finished) ==
           std::tie(b.shooter_y, b.target_y, b.shot_x, b.shot_y, b.shots, b.hits, b.frame, b.flying, b.hit, b.finished);
}
bool same(const ctg::RacingRect& a, const ctg::RacingRect& b) {
    return std::tie(a.x, a.y, a.width, a.height) ==
           std::tie(b.x, b.y, b.width, b.height);
}
bool same(const ctg::RacingState& a, const ctg::RacingState& b) {
    if (!same(a.road, b.road) || !same(a.player, b.player)) return false;
    for (unsigned i = 0; i < a.traffic.size(); ++i)
        if (!same(a.traffic[i].rect, b.traffic[i].rect) || a.traffic[i].collided != b.traffic[i].collided) return false;
    return std::tie(a.frame, a.speed, a.selected_speed, a.movement_remainder, a.road_scroll, a.frames_remaining, a.score, a.collisions, a.section_collided, a.finished, a.narrow) ==
           std::tie(b.frame, b.speed, b.selected_speed, b.movement_remainder, b.road_scroll, b.frames_remaining, b.score, b.collisions, b.section_collided, b.finished, b.narrow);
}
bool same(const ctg::Demo& demo, const Original& original) {
    return demo.current_id() == original.demo_id &&
        same(demo.tennis.state(), original.demo_tennis.state()) &&
        same(demo.shooting.state(), original.demo_shooting.state()) &&
        demo.shooting.target_visible() == original.demo_shooting.target_visible() &&
        same(demo.racing.state(), original.demo_racing.state());
}
}

int main() {
    {
        ctg::Demo shooting_demo;
        bool saw_shot = false, saw_hit = false, saw_miss = false, saw_final = false;
        for (unsigned frame = 0; frame < 4000; ++frame) {
            const auto before = shooting_demo.shooting.state();
            const unsigned events = shooting_demo.step(14, false, true);
            const auto& after = shooting_demo.shooting.state();
            if (events & ctg::sound_shot) {
                saw_shot = true;
                CHECK(!(events & ctg::event_player1_shot));
            }
            if (!before.hit && after.hit) {
                saw_hit = true;
                CHECK(events & ctg::sound_paddle);
                CHECK(!(events & ctg::sound_point));
            }
            if (before.flying && !after.flying) {
                saw_miss |= !before.hit;
                saw_final |= after.finished;
                CHECK(events & ctg::sound_point);
                CHECK(bool(events & ctg::sound_win) == after.finished);
            }
        }
        CHECK(saw_shot && saw_hit && saw_miss && saw_final);
    }
    ctg::Demo demo;
    Original original;
    CHECK(demo.current_id() == ctg::slot_count);
    // Reuse instances across slots to exercise switching as well as initialization.
    for (unsigned id = 0; id < ctg::slot_count; ++id) {
        for (unsigned frame = 0; frame < 4000; ++frame) {
            if (frame == 600 || frame == 1200) {
                original.paddle_movement_rebound = frame == 600;
                original.anti_stalemate = frame != 600;
                demo.tennis.set_rebound_mode(original.paddle_movement_rebound);
                demo.tennis.set_anti_stalemate(original.anti_stalemate);
                original.demo_tennis.set_rebound_mode(original.paddle_movement_rebound);
                original.demo_tennis.set_anti_stalemate(original.anti_stalemate);
            }
            if (frame == 2000) {
                const auto tennis = demo.tennis.state();
                const auto shooting = demo.shooting.state();
                const auto racing = demo.racing.state();
                demo.reset();
                original.reset();
                CHECK(demo.current_id() == ctg::slot_count);
                CHECK(same(tennis, demo.tennis.state()));
                CHECK(same(shooting, demo.shooting.state()));
                CHECK(same(racing, demo.racing.state()));
                demo.reset(); // Reset is idempotent and does not reset games.
            }
            const unsigned events = demo.step(id, original.paddle_movement_rebound, original.anti_stalemate);
            CHECK(events == original.step(id));
            CHECK((events & ~(ctg::sound_wall | ctg::sound_paddle | ctg::sound_point | ctg::sound_win | ctg::sound_shot)) == 0);
            CHECK(same(demo, original));
        }
    }
    // Rally options apply immediately, survive scoring resets, and follow slot changes.
    for (unsigned id = 0; id < 14; ++id) {
        ctg::Demo d;
        ctg::DemoOptions options;
        for (unsigned bits = 0; bits < 8; ++bits) {
            options.high_speed = bits & 1;
            options.small_paddle = {{bool(bits & 2), bool(bits & 4)}};
            for (unsigned frame = 0; frame < 300; ++frame) {
                d.step(id, false, true, options);
                CHECK(d.tennis.state().high_speed == options.high_speed);
                CHECK(d.tennis.state().small_paddle == options.small_paddle);
            }
        }
        d.reset();
        d.step(id, false, true, options);
        CHECK(d.tennis.state().high_speed && d.tennis.state().small_paddle[0] && d.tennis.state().small_paddle[1]);
    }
    {
        ctg::Demo d;
        ctg::DemoOptions options;
        for (unsigned mode : {0u, 1u, 0u, 1u}) {
            options.racing_mode = mode;
            const unsigned frame = d.current_id() == 15 ? d.racing.state().frame : 0;
            d.step(15, false, true, options);
            const auto& state = d.racing.state();
            CHECK(state.narrow == (mode == 1));
            CHECK(state.frame == frame + 1);
            CHECK(state.player.x == (mode == 1 ? 96 : 148));
            if (mode == 1) {
                CHECK(state.player.x + state.player.width <= ctg::racing_x(ctg::racing_center_column) + ctg::racing_cell_width);
                CHECK(state.traffic[0].rect.x + state.traffic[0].rect.width <= ctg::racing_x(ctg::racing_center_column) + ctg::racing_cell_width);
            }
        }
        ctg::Racing match;
        match.set_narrow(true);
        match.reset_from_layout(d.racing.state());
        for (unsigned i = 0; i < ctg::Racing::duration_frames; ++i) match.step();
        CHECK(match.state().finished && match.state().narrow);
        CHECK(match.state().player.x + match.state().player.width <= ctg::racing_x(ctg::racing_center_column) + ctg::racing_cell_width);
        d.reset(); d.step(15, false, true, options);
        CHECK(d.racing.state().narrow && d.racing.state().frame == 1);
        options.racing_mode = 0;
        d.step(0, false, true, options); d.step(15, false, true, options);
        CHECK(!d.racing.state().narrow);
    }
    // Racing preview chooses a new legal position on each wrap in either mode.
    for (unsigned mode : {0u, 1u}) {
        ctg::Demo d;
        ctg::DemoOptions options;
        options.racing_mode = mode;
        d.racing.set_random_seed(12345);
        d.step(15, false, true, options);
        const int initial = d.racing.state().traffic[0].rect.x;
        bool varied = false;
        for (unsigned i = 0; i < 20; ++i) {
            auto& state = const_cast<ctg::RacingState&>(d.racing.state());
            state.traffic[0].rect.y = -state.traffic[0].rect.height + 1;
            d.step(15, false, true, options);
            const int x = state.traffic[0].rect.x;
            CHECK(x >= (mode ? 54 : 58) && x <= (mode ? 138 : 238));
            varied |= x != initial;
        }
        CHECK(varied);
    }
    // Automatic target speeds and directions are measured before any shot can hit.
    for (unsigned target = 1; target <= 3; ++target) for (unsigned direction = 0; direction <= 1; ++direction) {
        ctg::Demo d;
        ctg::DemoOptions options;
        options.shooting_target = target; options.shooting_direction = direction;
        d.step(14, false, true, options);
        const int start = d.shooting.state().target_y;
        for (unsigned i = 0; i < 19; ++i) d.step(14, false, true, options);
        const int displacement = d.shooting.state().target_y - start;
        const unsigned speeds[] = {0, 6, 11, 16};
        const int expected = static_cast<int>(19 * speeds[target] / 4);
        CHECK(direction == 0 ? displacement >= expected && displacement <= expected + 1 :
                               displacement <= -expected && displacement >= -expected - 1);
    }
    {
        ctg::Demo d;
        ctg::DemoOptions o;
        o.shooting_target = 0;
        d.step(14, false, true, o);
        const int start = d.shooting.state().target_y;
        for (int i = 0; i < 10; ++i) d.step(14, false, true, o);
        CHECK(d.shooting.state().target_y == start + 60);
        bool manual_reversed = false;
        for (int i = 11; i < 80; ++i) {
            const int y = d.shooting.state().target_y;
            d.step(14, false, true, o);
            if (i >= 60 && d.shooting.state().target_y == y - 6) manual_reversed = true;
        }
        CHECK(manual_reversed);
        d.reset(); d.step(14, false, true, o);
        o.shooting_target = 1; o.shooting_direction = 1; o.shooting_size = 1; o.shooting_speed = 1;
        const auto frame = d.shooting.state().frame;
        const int before = d.shooting.state().target_y;
        d.step(14, false, true, o);
        CHECK(d.shooting.state().frame == frame + 1);
        CHECK(d.shooting.state().target_y < before);
        CHECK(d.shooting.paddle_height(0) == 32 && d.shooting.paddle_height(1) == 16);
        const int shot = d.shooting.state().shot_x;
        d.step(14, false, true, o);
        CHECK(d.shooting.state().shot_x - shot == 6);
        o.shooting_size = 0; o.shooting_speed = 0;
        d.step(14, false, true, o);
        CHECK(d.shooting.paddle_height(0) == 16 && d.shooting.paddle_height(1) == 33);
        const int slow_shot = d.shooting.state().shot_x;
        d.step(14, false, true, o);
        CHECK(d.shooting.state().shot_x - slow_shot == 3);
    }
    {
        ctg::Demo d;
        ctg::DemoOptions o;
        o.shooting_target = 3; o.shooting_direction = 3;
        int previous = 0; bool rising = false, falling = false;
        for (unsigned i = 0; i < 250; ++i) {
            d.step(14, false, true, o);
            const int y = d.shooting.state().target_y;
            if (i) { CHECK(y - previous <= 4 && previous - y <= 4); rising |= y > previous; falling |= y < previous; }
            previous = y;
        }
        CHECK(rising && falling);
    }
    {
        ctg::Demo a, b, noise;
        ctg::DemoOptions o;
        o.shooting_target = 4; o.shooting_direction = 2; o.shooting_size = 2; o.shooting_speed = 2;
        bool small = false, large = false, fast = false, slow = false, reset_seen = false;
        for (unsigned i = 0; i < 12000; ++i) {
            const auto previous = a.shooting.state();
            const unsigned ea = a.step(14, false, true, o);
            for (unsigned n = 0; n < 3; ++n) noise.step(14, false, true, o);
            const unsigned eb = b.step(14, false, true, o);
            CHECK(ea == eb && same(a.shooting.state(), b.shooting.state()));
            CHECK(a.shooting.target_visible() == b.shooting.target_visible());
            CHECK(a.shooting.paddle_height(0) == b.shooting.paddle_height(0));
            CHECK(a.shooting.paddle_height(1) == b.shooting.paddle_height(1));
            small |= a.shooting.paddle_height(1) == 16; large |= a.shooting.paddle_height(1) == 33;
            fast |= a.shooting.paddle_height(0) == 32; slow |= a.shooting.paddle_height(0) == 16;
            reset_seen |= previous.finished && !a.shooting.state().finished;
        }
        CHECK(small && large && fast && slow && reset_seen);
        a.reset(); b.reset();
        for (unsigned i = 0; i < 500; ++i) {
            CHECK(a.step(14, false, true, o) == b.step(14, false, true, o));
            CHECK(same(a.shooting.state(), b.shooting.state()));
        }
        ctg::Demo fresh;
        a.reset();
        for (unsigned i = 0; i < 500; ++i) {
            CHECK(a.step(14, false, true, o) == fresh.step(14, false, true, o));
            CHECK(same(a.shooting.state(), fresh.shooting.state()));
            CHECK(a.shooting.paddle_height(1) == fresh.shooting.paddle_height(1));
        }
    }
    // The compact odd-id mapping also matches the explicit original for non-rally IDs.
    for (unsigned id : {0u, 1u, 13u, 14u, 15u, 16u, 17u, ~0u}) {
        const auto game = ctg::make_rally_game(id);
        const ctg::TennisB expected(id == 1 || id == 3 || id == 5 || id == 7 || id == 9 || id == 11 || id == 13,
            id == 4 || id == 5 || id == 6 || id == 7, id == 8 || id == 9 || id == 10 || id == 11,
            id == 0 || id == 1, id == 4 || id == 5, id == 8 || id == 9, id == 12 || id == 13);
        CHECK(same(game.state(), expected.state()));
    }
    return 0;
}
