#include "shooting.h"
#include "tennis_b.h"
#include <cstdio>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

int main() {
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
    CHECK(game.step(0, true) & ctg::sound_paddle);
    CHECK(game.state().flying);
    game.step(0, true);
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
