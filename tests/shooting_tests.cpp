#include "shooting.h"
#include "tennis_b.h"
#include <cstdio>
#include <limits>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

int main() {
    {
        ctg::Shooting game;
        game.set_shot_limit(1);
        game.set_target_behavior(11, 1, false);
        game.set_endless(true);
        game.reset();
        for (int shot = 0; shot < 2; ++shot) {
            unsigned events = game.step(0, true);
            while (game.state().flying) events |= game.step(0, false);
            CHECK(!(events & ctg::sound_win));
            CHECK(!game.state().finished && game.state().shots == static_cast<unsigned>(shot + 1));
        }
        auto& s = const_cast<ctg::ShootingState&>(game.state());
        s.shots = s.hits = s.frame = std::numeric_limits<unsigned>::max();
        unsigned events = game.step(0, true);
        CHECK(s.frame == 0);
        while (s.flying) events |= game.step(0, false);
        CHECK((events & ctg::sound_point) && !(events & ctg::sound_win));
        CHECK(s.shots == std::numeric_limits<unsigned>::max());
        CHECK(s.hits == std::numeric_limits<unsigned>::max());
        game.set_endless(false);
        events = game.step(0, true);
        while (s.flying) events |= game.step(0, false);
        CHECK((events & ctg::sound_win) && s.finished);
    }
    {
        for (unsigned speed : {6u, 11u, 16u}) {
            for (int direction : {-1, 1}) {
                ctg::Shooting target;
                target.set_target_behavior(speed, direction, true);
                target.step(0, false, 0, true);
                CHECK(target.state().target_y == (405 + static_cast<int>(speed) * direction) / 4);
                target.reset();
                target.step(0, false, 0, true);
                CHECK(target.state().target_y == (405 + static_cast<int>(speed) * direction) / 4);
            }
        }
        ctg::Shooting manual;
        manual.set_target_behavior(16, -1, false);
        manual.step(0, false, 1);
        CHECK(manual.state().target_y == (405 + 12) / 4);
        manual.step(0, false, -1);
        CHECK(manual.state().target_y == 405 / 4);
    }
    {
        for (int direction : {-1, 1}) {
            ctg::Shooting target;
            const unsigned speed = std::numeric_limits<unsigned>::max();
            target.set_target_behavior(speed, direction, false);
            int position = 405;
            for (int i = 0; i < 100; ++i) {
                constexpr int min_position = -8 * 4, range = (216 + 8 + 1) * 4;
                position = (position - min_position + static_cast<int>(speed % range) * direction + range) % range + min_position;
                target.step(0, false, 0, true);
                CHECK(target.state().target_y == position / 4);
                CHECK(target.target_visible());
            }
        }
        for (bool small : {false, true}) for (bool automatic : {false, true}) for (int direction : {-1, 1}) {
            ctg::Shooting target;
            target.set_options(false, {false, small});
            target.set_target_behavior(11, direction, false);
            bool wrapped = false;
            for (int i = 0; i < 300; ++i) {
                const int before = target.target_top();
                target.step(0, false, direction, automatic);
                const int top = target.target_top();
                CHECK(top <= 216 && top + (small ? 16 : 32) >= 24);
                wrapped |= direction > 0 ? top < before : top > before;
            }
            CHECK(wrapped);
        }
    }
    {
        for (bool blinking : {false, true}) {
            ctg::Shooting target;
            target.set_target_behavior(6, -1, blinking);
            for (int i = 0; i < 19; ++i) target.step(0, false);
            target.step(0, true);
            while (target.state().flying) target.step(0, false);
            CHECK(target.state().hits == (blinking ? 0u : 1u));
        }
    }
    {
        ctg::Shooting slow, fast;
        fast.set_options(true, {true, false});
        slow.step(0, true); fast.step(0, true);
        CHECK(fast.state().shot_x > slow.state().shot_x);
        CHECK(fast.paddle_height(0) == 16 && fast.paddle_height(1) == 33);
        fast.reset(); CHECK(fast.paddle_height(0) == 16);
        fast.step(0, true); CHECK(fast.state().shot_x > slow.state().shot_x);
    }
    for (bool small : {false, true}) {
        ctg::Shooting shooter;
        shooter.set_options(false, {small, false});
        const int height = small ? 16 : 32;
        CHECK(shooter.paddle_height(0) == height);
        for (int i = 0; i < 100; ++i) shooter.step(-1, false);
        CHECK(shooter.shooter_top() == 24 - height);
        CHECK(shooter.state().shooter_y == 24 - height / 2);
        shooter.step(-1, true);
        CHECK(shooter.state().shot_y == shooter.state().shooter_y);
        while (shooter.state().flying) shooter.step(0, false);
        for (int i = 0; i < 100; ++i) shooter.step(1, false);
        CHECK(shooter.shooter_top() == 216);
        CHECK(shooter.state().shooter_y == 216 + height / 2);
        shooter.step(1, true);
        CHECK(shooter.state().shot_y == shooter.state().shooter_y);
        shooter.set_options(false, {false, false});
        CHECK(shooter.shooter_top() <= 216);
    }
    {
        ctg::Shooting shooter;
        shooter.set_options(false, {true, false});
        for (int i = 0; i < 100; ++i) shooter.step(-1, false);
        shooter.set_options(false, {false, false});
        CHECK(shooter.state().shooter_y == 16);
        CHECK(shooter.shooter_top() + shooter.paddle_height(0) >= 24);
    }
    for (bool small : {false, true}) {
        ctg::Shooting target;
        target.set_options(false, {false, small});
        for (int i = 0; i < 5; ++i) target.step(-1, false);
        target.step(0, true);
        while (target.state().flying) target.step(0, false);
        CHECK(target.state().hits == (small ? 0u : 1u));
    }
    for (unsigned limit : {5u, 10u, 15u, 20u, 25u, 30u}) {
        ctg::Shooting configured;
        configured.set_shot_limit(limit);
        for (int round = 0; round < 2; ++round) {
            configured.reset();
            for (unsigned shot = 0; shot < limit; ++shot) {
                unsigned events = configured.step(0, true);
                CHECK(!configured.state().finished && configured.state().shots == shot);
                while (configured.state().flying) events |= configured.step(0, false);
                CHECK(configured.state().shots == shot + 1);
                CHECK(configured.state().finished == (shot + 1 == limit));
                CHECK(bool(events & ctg::sound_win) == (shot + 1 == limit));
            }
            CHECK(configured.step(0, true) == ctg::sound_none);
        }
    }
    ctg::Shooting game;
    CHECK(game.state().shots == 0 && !game.state().flying);
    int initial = game.state().target_y;
    game.step(-1, false);
    CHECK(game.state().target_y == initial && game.state().shooter_y == 114);
    game.step(0, false, 1);
    CHECK(game.state().target_y == initial + 3 && game.state().shooter_y == 114);
    game.step(0, false, -1);
    CHECK(game.state().target_y == initial);
    game.step(0, false, -1, true);
    CHECK(game.state().target_y > initial);
    game.reset();
    bool wrapped_down = false, wrapped_up = false;
    for (int i = 0; i < 100; ++i) {
        int before = game.state().target_y; game.step(0, false, 1);
        wrapped_down |= game.state().target_y < before;
    }
    for (int i = 0; i < 100; ++i) {
        int before = game.state().target_y; game.step(0, false, -1);
        wrapped_up |= game.state().target_y > before;
    }
    CHECK(wrapped_down && wrapped_up);
    game.reset();
    bool hidden = false, visible = false;
    for (int i = 0; i < 40; ++i) { game.step(0, false); hidden |= !game.target_visible(); visible |= game.target_visible(); }
    CHECK(hidden && visible);
    game.reset();
    CHECK(game.step(0, false) == ctg::sound_none);
    CHECK(game.step(0, true) == (ctg::sound_paddle | ctg::event_player1_shot));
    CHECK(game.state().flying);
    CHECK(!(game.step(0, true) & ctg::event_player1_shot));
    CHECK(game.state().shots == 0);
    for (int i = 0; i < 100 && game.state().flying; ++i) game.step(0, false);
    CHECK(game.state().shots == 1 && !game.state().flying);
    for (int shot = 1; shot < 15; ++shot) {
        game.step(0, true);
        for (int i = 0; i < 100 && game.state().flying; ++i) game.step(0, false);
        CHECK(game.state().shots == static_cast<unsigned>(shot + 1));
        CHECK(game.state().hits <= game.state().shots);
    }
    CHECK(game.state().finished);
    game.step(0, true); CHECK(!game.state().flying && game.state().shots == 15);
    game.reset(); CHECK(!game.state().finished && game.state().shots == 0 && game.state().hits == 0);
    bool hit = false;
    for (int frame = 0; frame < 10000 && !hit; ++frame) {
        if (!game.state().flying && game.state().shots < 15) game.step(0, true);
        else {
            int direction = (game.state().target_y + 16 > game.state().shooter_y) - (game.state().target_y + 16 < game.state().shooter_y);
            game.step(direction, false);
        }
        hit = game.state().hits > 0;
        if (game.state().finished && !hit) game.reset();
    }
    CHECK(hit && game.state().hit);
    unsigned hits = game.state().hits;
    while (game.state().flying) game.step(0, false);
    CHECK(game.state().hits == hits);
    return 0;
}
