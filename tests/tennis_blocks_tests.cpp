#include "tennis_test_harness.h"
#include "hockey_ball.h"
#include "tennis_a_blocks.h"
#include "tennis_b.h"

#include <initializer_list>

namespace tennis_tests {

int variant_block_collisions() {
    for (bool doubles_mode : {false, true}) for (int color = 0; color < 2; ++color)
        for (int row = 0; row < 6; ++row) for (int col = 0; col < 2; ++col)
        for (int direction : {-1, 1}) {
            ctg::TennisB game(doubles_mode, false, true, false, false, true);
            auto& s = const_cast<ctg::TennisState&>(game.state());
            const ctg::TennisABlock block{139 + 12 * color + 24 * col, (color == 0 ? 44 : 28) + 32 * row, 6, 4, color == 0};
            s.serve_wait = 0;
            s.ball_y = 0; while (ctg::hockey_y(s.ball_y) < block.y + 3) ++s.ball_y;
            s.ball_x = 0;
            if (direction > 0) {
                while (ctg::hockey_x(s.ball_x) + 4 <= block.x) ++s.ball_x;
                s.ball_x -= 3;
            } else {
                while (ctg::hockey_x(s.ball_x) - 3 < block.x + block.width) ++s.ball_x;
                s.ball_x += 2;
            }
            s.ball_vx = direction * 4; s.ball_vy = 0;
            const int start_x = s.ball_x;
            const bool solid = (color == 0 && direction < 0) || (color == 1 && direction > 0);
            CHECK(bool(game.step() & ctg::sound_paddle) == solid);
            CHECK(s.ball_vx == (solid ? -direction : direction) * 4);
            CHECK(s.ball_x == start_x + direction * (solid ? 2 : 4));
            CHECK(s.ball_vy == 0);
            if (!solid) {
                // Traverse the whole transparent block without a delayed collision.
                for (int i = 0; i < 4; ++i) {
                    CHECK(!(game.step() & ctg::sound_paddle));
                    CHECK(s.ball_vx == direction * 4 && s.ball_vy == 0);
                }
            } else if ((direction > 0 && col == 0) || (direction < 0 && col == 1))
                CHECK(!(game.step() & ctg::sound_paddle));
            game.reset(); CHECK(game.state().volleyball_a && game.state().volleyball && game.state().doubles == doubles_mode);
        }
    for (bool doubles : {false, true}) for (bool high : {false, true}) for (int color : {0, 1}) {
        ctg::TennisB game(doubles, false, true, false, false, true);
        ctg::TennisB twin(doubles, false, true, false, false, true);
        game.set_options(high, {true, false}); twin.set_options(high, {true, false});
        const auto contact = [color, high](ctg::TennisB& g, int direction, int offset, int vy) {
            auto& s = const_cast<ctg::TennisState&>(g.state());
            const auto b = ctg::tennis_a_block(color, 2, 0);
            s.serve_wait = 0;
            s.ball_y = 0; while (ctg::hockey_y(s.ball_y) < b.y + offset) ++s.ball_y;
            s.ball_x = 0;
            if (direction > 0) {
                while (ctg::hockey_x(s.ball_x) + 4 <= b.x) ++s.ball_x;
                s.ball_x -= 3;
            } else {
                while (ctg::hockey_x(s.ball_x) - 3 < b.x + b.width) ++s.ball_x;
                s.ball_x += 2;
            }
            s.ball_vx = direction * (high ? 8 : 4); s.ball_vy = vy;
            return g.step();
        };
        const int directed = color == 0 ? -1 : 1;
        for (int offset : {-1, 5}) {
            CHECK(contact(game, directed, offset, 3) & ctg::sound_paddle);
            CHECK(offset < 0 ? game.state().ball_vy < 0 : (high ? game.state().ball_vy > 0 : game.state().ball_vy == 0));
        }
        game.reset(); twin.reset();
        for (int n = 0; n < 25; ++n) {
            CHECK(!(contact(game, -directed, 3, n % 2 ? 2 : 0) & ctg::sound_paddle));
            CHECK(game.state().ball_vy == (n % 2 ? 2 : 0));
        }
        // Passing through blocks does not consume rebounds or advance randomness.
        CHECK(contact(game, directed, 5, 2) & ctg::sound_paddle);
        CHECK(contact(twin, directed, 5, 2) & ctg::sound_paddle);
        CHECK(same(game.state(), twin.state()));
        // Ten consecutive block hits switch even a normally directed hit to the random rule.
        game.reset();
        for (int n = 0; n < 10; ++n) CHECK(contact(game, directed, 3, 2) & ctg::sound_paddle);
        CHECK(contact(game, directed, 3, 2) & ctg::sound_paddle);
        CHECK(game.state().ball_vy == -2 || game.state().ball_vy == 0);
        // A point resets the count, as does a real paddle contact and a new game.
        auto& s = const_cast<ctg::TennisState&>(game.state());
        s.ball_x = -60; s.ball_y = 20; s.ball_vx = -4; s.ball_vy = 0;
        CHECK(game.step() & ctg::sound_point);
        CHECK(contact(game, directed, 5, 2) & ctg::sound_paddle); CHECK(high ? s.ball_vy > 0 : s.ball_vy == 0);
        for (int n = 0; n < 10; ++n) contact(game, directed, 3, 2);
        s.ball_x = 20; s.ball_y = s.paddle_y[0]; s.ball_vx = -4; s.ball_vy = 0;
        CHECK(game.step() & ctg::sound_paddle);
        CHECK(contact(game, directed, 5, 2) & ctg::sound_paddle); CHECK(high ? s.ball_vy > 0 : s.ball_vy == 0);
        if (doubles) {
            for (int n = 0; n < 10; ++n) contact(game, directed, 3, 2);
            s.ball_x = ctg::TennisB::inner_left + 4; s.ball_y = s.paddle_y[0]; s.ball_vx = -4; s.ball_vy = 0;
            CHECK(game.step() & ctg::sound_paddle);
            CHECK(contact(game, directed, 5, 2) & ctg::sound_paddle); CHECK(high ? s.ball_vy > 0 : s.ball_vy == 0);
        }
        game.reset(); CHECK(contact(game, directed, 5, 2) & ctg::sound_paddle); CHECK(high ? s.ball_vy > 0 : s.ball_vy == 0);
        CHECK(s.high_speed == high && s.small_paddle[0]);
    }
    {
        ctg::TennisB game(false, false, true, false, false, true);
        auto& s = const_cast<ctg::TennisState&>(game.state());
        s.serve_wait = 0; s.ball_x = 156; s.ball_y = 120; s.ball_vx = 4; s.ball_vy = 2;
        CHECK(game.step() == ctg::sound_none && s.ball_vx == 4 && s.ball_vy == 2);
    }
    for (bool doubles_mode : {false, true}) for (int color = 0; color < 2; ++color) {
        for (int row = 0; row < 6; ++row) for (int col = 0; col < 2; ++col) {
            const ctg::TennisABlock block{139 + 12 * color + 24 * col, (color == 0 ? 44 : 28) + 32 * row, 6, 4, color == 0};
            for (int direction : {-1, 0, 1}) {
                ctg::TennisB a(doubles_mode, false, false, true);
                auto& s = const_cast<ctg::TennisState&>(a.state());
                s.serve_wait = 0;
                s.ball_x = 0; while (ctg::hockey_x(s.ball_x) < block.x + 4) ++s.ball_x;
                s.ball_y = 0; while (ctg::hockey_y(s.ball_y) < block.y + 3) ++s.ball_y;
                s.ball_vx = 4; s.ball_vy = direction * 2;
                CHECK(a.step() & ctg::sound_wall);
                CHECK(s.ball_vx == 4);
                const bool bounce = color == 0 ? direction > 0 : direction < 0;
                CHECK(s.ball_vy == (bounce ? -direction * 2 : direction * 2));
                a.reset(); CHECK(a.state().tennis_a && a.state().doubles == doubles_mode && a.state().serve_wait == ctg::TennisB::hockey_serve_delay);
            }
        }
    }
    {
        ctg::TennisB a(false, false, false, true);
        auto& s = const_cast<ctg::TennisState&>(a.state());
        s.serve_wait = 0;
        s.ball_x = 160; s.ball_y = 120; s.ball_vx = 4; s.ball_vy = 2;
        CHECK(a.step() == ctg::sound_none && s.ball_vy == 2);
        const auto b = ctg::tennis_a_block(0, 2, 0);
        s.ball_x = 0; while (ctg::hockey_x(s.ball_x) < b.x + 4) ++s.ball_x;
        s.ball_y = 0; while (ctg::hockey_y(s.ball_y) < b.y + 3) ++s.ball_y;
        s.ball_vx = 0; s.ball_vy = -2;
        CHECK(a.step() & ctg::sound_wall);
        s.ball_y += 2;
        CHECK(!(a.step() & ctg::sound_wall));
        s.ball_y += 2; s.ball_vy = -1;
        CHECK(a.step() & ctg::sound_wall);
    }
    // The other color's columns must not collide on any of these rows.
    for (bool volleyball : {false, true}) for (bool doubles : {false, true})
        for (int color : {0, 1}) for (int row = 0; row < 6; ++row)
            for (int col = 0; col < 2; ++col) for (int direction : {-1, 1}) {
                ctg::TennisB game(doubles, false, volleyball, !volleyball, false, volleyball);
                auto& s = const_cast<ctg::TennisState&>(game.state());
                const int x = 139 + 12 * (1 - color) + 24 * col;
                const int y = (color == 0 ? 44 : 28) + 32 * row;
                s.serve_wait = 0;
                s.ball_x = 0; while (ctg::hockey_x(s.ball_x) < x + 3) ++s.ball_x;
                s.ball_y = 0; while (ctg::hockey_y(s.ball_y) < y + 2) ++s.ball_y;
                s.ball_vx = direction; s.ball_vy = 0;
                CHECK(game.step() == ctg::sound_none);
                CHECK(s.ball_vx == direction && s.ball_vy == 0);
            }
    return 0;
}

} // namespace tennis_tests
