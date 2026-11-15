#include "tennis_test_harness.h"
#include "hockey_ball.h"
#include "tennis_a_blocks.h"
#include "tennis_b.h"

#include <cstdlib>
#include <initializer_list>

namespace tennis_tests {

int rebound_history_and_stalemate() {
    {
        // Each collision path and owner contributes to the same rebound streak.
        for (int path : {0, 1, 2}) for (int owner : {0, 1}) for (bool fast : {false, true}) {
            ctg::TennisB game(path != 0, path == 2);
            game.set_options(fast, {false, false});
            game.set_rebound_mode(true);
            auto& s = const_cast<ctg::TennisState&>(game.state());
            game.step({owner == 0 ? 1 : 0, owner == 1 ? 1 : 0});
            const auto hit = [&](int offset = 0) {
                s.serve_wait = 0;
                if (path == 0) {
                    s.ball_x = owner == 0 ? 20 : 300;
                    s.ball_vx = owner == 0 ? -4 : 4;
                } else if (path == 1) {
                    s.ball_x = owner == 0 ? ctg::TennisB::inner_left + 4 : ctg::TennisB::inner_right - 4;
                    s.ball_vx = owner == 0 ? -4 : 4;
                } else {
                    const int inner = owner == 0 ? ctg::hockey_inner_right : ctg::hockey_inner_left;
                    const int direction = owner == 0 ? -1 : 1;
                    const int face = ctg::hockey_paddle_face(inner, direction > 0);
                    s.ball_x = face - direction * 2; s.ball_vx = direction * 4;
                }
                s.ball_y = s.paddle_y[owner] + offset; s.ball_vy = 0;
                return game.step();
            };
            CHECK(hit() & ctg::sound_paddle); CHECK(s.ball_vy == 0);
            // Arbitrarily many horizontal frames cannot trigger or erase the streak.
            for (int i = 0; i < 700; ++i) {
                s.ball_x = 50; s.ball_y = 120; s.ball_vx = 0; s.ball_vy = 0;
                CHECK(game.step() == ctg::sound_none); CHECK(s.ball_vy == 0);
            }
            CHECK(hit() & ctg::sound_paddle); CHECK(s.ball_vy == 0);
            CHECK(hit() & ctg::sound_paddle); CHECK(s.ball_vy == (fast ? 2 : 1));
            CHECK(hit() & ctg::sound_paddle); CHECK(s.ball_vy == 0);
            CHECK(hit(8) & ctg::sound_paddle); CHECK(s.ball_vy != 0);
            CHECK(hit() & ctg::sound_paddle); CHECK(s.ball_vy == 0);
            CHECK(hit() & ctg::sound_paddle); CHECK(s.ball_vy == 0);
            game.set_anti_stalemate(false);
            for (int i = 0; i < 5; ++i) { CHECK(hit() & ctg::sound_paddle); CHECK(s.ball_vy == 0); }
            game.set_anti_stalemate(true);
            CHECK(hit() & ctg::sound_paddle); CHECK(s.ball_vy == 0);
            CHECK(hit() & ctg::sound_paddle); CHECK(s.ball_vy == 0);
            CHECK(hit() & ctg::sound_paddle); CHECK(s.ball_vy == (fast ? 2 : 1));
        }
        // Reset, point and serve each discard an unfinished streak.
        for (int reset_kind : {0, 1, 2}) {
            ctg::TennisB game;
            game.set_rebound_mode(true);
            auto& s = const_cast<ctg::TennisState&>(game.state());
            game.step({1, 0});
            const auto hit = [&]() {
                s.serve_wait = 0; s.ball_x = 20; s.ball_y = s.paddle_y[0]; s.ball_vx = -4; s.ball_vy = 0;
                return game.step();
            };
            CHECK(hit() & ctg::sound_paddle); CHECK(hit() & ctg::sound_paddle);
            if (reset_kind == 0) { game.reset(); game.step({1, 0}); }
            if (reset_kind == 1) { s.ball_x = -60; s.ball_vx = -4; CHECK(game.step() & ctg::sound_point); }
            if (reset_kind == 2) {
                s.serve_wait = 1; game.step();
                s.ball_x = 160; s.ball_y = 120; s.ball_vx = 0; s.ball_vy = 0; game.step();
            }
            CHECK(hit() & ctg::sound_paddle); CHECK(s.ball_vy == 0);
            CHECK(hit() & ctg::sound_paddle); CHECK(s.ball_vy == 0);
            CHECK(hit() & ctg::sound_paddle); CHECK(s.ball_vy == 1);
        }
        // A volleyball obstacle changes vertical velocity and clears the streak;
        // its paddle sound does not itself count as a paddle rebound.
        ctg::TennisB game(false, false, true, false, false, true);
        game.set_rebound_mode(true);
        auto& s = const_cast<ctg::TennisState&>(game.state());
        game.step({1, 0});
        const auto hit = [&]() {
            s.serve_wait = 0; s.ball_x = 20; s.ball_y = s.paddle_y[0]; s.ball_vx = -4; s.ball_vy = 0;
            return game.step();
        };
        CHECK(hit() & ctg::sound_paddle); CHECK(hit() & ctg::sound_paddle);
        const auto block = ctg::tennis_a_block(0, 2, 0);
        s.ball_y = 0; while (ctg::hockey_y(s.ball_y) < block.y + 5) ++s.ball_y;
        s.ball_x = 0; while (ctg::hockey_x(s.ball_x) - 3 < block.x + block.width) ++s.ball_x;
        s.ball_x += 2; s.ball_vx = -8; s.ball_vy = 0;
        CHECK(game.step() & ctg::sound_paddle); CHECK(s.ball_vy != 0);
        CHECK(hit() & ctg::sound_paddle); CHECK(s.ball_vy == 0);
        CHECK(hit() & ctg::sound_paddle); CHECK(s.ball_vy == 0);
        CHECK(hit() & ctg::sound_paddle); CHECK(s.ball_vy == 1);
    }
    {
        // Exercise outer, tennis inner, and hockey inner paddle ownership.
        for (int path : {0, 1, 2}) for (int owner : {0, 1}) for (bool fast : {false, true}) {
            ctg::TennisB game(path != 0, path == 2);
            game.set_options(fast, {false, false});
            game.set_rebound_mode(true);
            game.set_anti_stalemate(false);
            auto& s = const_cast<ctg::TennisState&>(game.state());
            game.step({owner == 0 ? -1 : 1, owner == 1 ? -1 : 1}); // During serve wait.
            const auto hit = [&](int offset) {
                s.serve_wait = 0;
                if (path == 0) {
                    s.ball_x = owner == 0 ? 20 : 300;
                    s.ball_vx = owner == 0 ? -4 : 4;
                } else if (path == 1) {
                    s.ball_x = owner == 0 ? ctg::TennisB::inner_left + 4 : ctg::TennisB::inner_right - 4;
                    s.ball_vx = owner == 0 ? -4 : 4;
                } else {
                    const int inner = owner == 0 ? ctg::hockey_inner_right : ctg::hockey_inner_left;
                    const int direction = owner == 0 ? -1 : 1;
                    const int face = ctg::hockey_paddle_face(inner, direction > 0);
                    s.ball_x = face - direction * 2;
                    s.ball_vx = direction * 4;
                }
                s.ball_y = s.paddle_y[owner] + offset;
                s.ball_vy = 0;
                return game.step();
            };
            for (int offset : {-18, -14, -8, -7, -4, -3, 0, 3, 4, 7, 8, 14, 18}) {
                CHECK(hit(offset) & ctg::sound_paddle);
                const int distance = std::abs(offset);
                const int magnitude = distance <= 3 ? 0 : distance <= 7 ? 1 : distance <= 14 ? 2 : 3;
                CHECK(s.ball_vy == -magnitude * (fast ? 2 : 1));
            }
            // Reverse, then stop; the last nonzero displacement remains authoritative.
            s.serve_wait = 10;
            game.step({owner == 0 ? 1 : 0, owner == 1 ? 1 : 0});
            game.step();
            CHECK(hit(-8) & ctg::sound_paddle);
            CHECK(s.ball_vy == 2 * (fast ? 2 : 1));
            // A point and its serve wait do not erase movement history.
            s.ball_x = -60; s.ball_y = 120; s.ball_vx = -4; s.ball_vy = 0;
            CHECK(game.step() & ctg::sound_point);
            CHECK(hit(-8) & ctg::sound_paddle);
            CHECK(s.ball_vy == 2 * (fast ? 2 : 1));
            game.set_rebound_mode(false);
            CHECK(hit(-8) & ctg::sound_paddle);
            CHECK(s.ball_vy == -3 * (fast ? 2 : 1));
            game.set_rebound_mode(true);
            CHECK(hit(-8) & ctg::sound_paddle);
            CHECK(s.ball_vy == 2 * (fast ? 2 : 1));
            game.reset(); // Mode persists, but history does not.
            CHECK(hit(-8) & ctg::sound_paddle);
            CHECK(s.ball_vy == -3 * (fast ? 2 : 1));
            s.serve_wait = 10;
            game.step({owner == 0 ? 1 : 0, owner == 1 ? 1 : 0});
            CHECK(hit(-8) & ctg::sound_paddle);
            CHECK(s.ball_vy == 2 * (fast ? 2 : 1));
        }
    }
    {
        ctg::TennisB game;
        game.set_rebound_mode(true);
        auto& s = const_cast<ctg::TennisState&>(game.state());
        s.paddle_y[0] = game.paddle_min(0) + 2;
        game.step({-1, 0}); // Partial displacement into the clamp.
        game.step({-1, 0}); // No displacement must not replace history.
        s.serve_wait = 0; s.ball_x = 20; s.ball_y = 0; s.ball_vx = -4; s.ball_vy = 0;
        CHECK(game.step({-1, 0}) & ctg::sound_paddle);
        CHECK(s.ball_vy == -3);
    }
    {
        ctg::TennisB game; // Legacy mode is the default even after movement.
        auto& s = const_cast<ctg::TennisState&>(game.state());
        game.step({1, 0});
        s.serve_wait = 0; s.ball_x = 20; s.ball_y = s.paddle_y[0] - 8; s.ball_vx = -4; s.ball_vy = 0;
        CHECK(game.step() & ctg::sound_paddle);
        CHECK(s.ball_vy == -3);
    }
    return 0;
}

} // namespace tennis_tests
