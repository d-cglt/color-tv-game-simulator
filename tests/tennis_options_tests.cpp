#include "tennis_test_harness.h"
#include "hockey_ball.h"
#include "tennis_b.h"
#include <algorithm>
#include <array>

#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <limits>

namespace tennis_tests {

int paddle_limits_and_visible_misses() {
    for (int mode = 0; mode < 7; ++mode) for (bool doubles : {false, true})
        for (bool small : {false, true}) {
            ctg::TennisB game(doubles, mode == 2 || mode == 3, mode == 4 || mode == 5,
                              mode == 1, mode == 3, mode == 5, mode == 6);
            game.set_options(false, {small, small});
            for (int direction : {-1, 1}) {
                for (int n = 0; n < 100; ++n) game.step({direction, direction});
                for (int owner = 0; owner < 2; ++owner) {
                    CHECK(game.state().paddle_y[owner] == (direction < 0 ? game.paddle_min(owner) : game.paddle_max(owner)));
                    CHECK(game.paddle_pixels(owner) == (small ? 16 : 32));
                    CHECK(game.paddle_top(owner) == (direction < 0 ? 24 - game.paddle_pixels(owner) : 216));
                    CHECK(game.paddle_visible_bottom(owner) - game.paddle_visible_top(owner) == 4);
                }
            }
        }
    // A missed ball remains in play until its entire drawn rectangle leaves the display.
    for (int mode = 0; mode < 7; ++mode) for (bool doubles : {false, true}) for (int direction : {-1, 1}) {
        const bool hockey = mode == 2 || mode == 3;
        ctg::TennisB game(doubles, hockey, mode == 4 || mode == 5,
                          mode == 1, mode == 3, mode == 5, mode == 6);
        auto& s = const_cast<ctg::TennisState&>(game.state());
        s.serve_wait = 0;
        s.paddle_y = {20, 20};
        s.ball_x = direction < 0 ? (hockey ? 21 : 19) : (hockey ? 299 : 301);
        s.ball_y = 120; s.ball_vx = direction; s.ball_vy = 0;
        CHECK(!(game.step() & (ctg::sound_paddle | ctg::sound_point)));
        // Moving into the ball's path after it has passed cannot recover the miss.
        s.paddle_y = {120, 120};
        bool saw_surround = false, saw_clipped = false, scored = false;
        for (int frame = 0; frame < 100; ++frame) {
            const unsigned events = game.step();
            const int x = ctg::hockey_x(s.ball_x);
            const bool visible = x + 4 > 0 && x - 3 < ctg::TennisB::width;
            CHECK(!(events & ctg::sound_paddle));
            CHECK(bool(events & ctg::sound_point) == !visible);
            if (!visible) {
                CHECK(s.score[direction < 0 ? 1 : 0] == 1);
                CHECK(s.serve_wait == ctg::TennisB::hockey_serve_delay);
                scored = true;
                break;
            }
            CHECK(s.score[0] == 0 && s.score[1] == 0 && s.serve_wait == 0);
            saw_surround |= direction < 0 ? s.ball_x < 0 : s.ball_x > 320;
            if (x - 3 < 0 || x + 4 > 320) {
                saw_clipped = true;
                if (hockey) {
                    std::array<std::uint32_t, 320 * 240> pixels{};
                    ctg::draw_hockey_ball(pixels, x, ctg::hockey_y(s.ball_y));
                    const int count = static_cast<int>(std::count(pixels.begin(), pixels.end(), ctg::hockey_exterior_puck_color));
                    CHECK(count > 0 && count < 35);
                }
            }
        }
        CHECK(scored && saw_surround && saw_clipped);
    }
    {
        ctg::TennisB game;
        game.set_winning_score(1);
        game.set_endless(true);
        game.reset();
        auto& s = const_cast<ctg::TennisState&>(game.state());
        const auto point = [&]() {
            s.serve_wait = 0; s.ball_x = -60; s.ball_y = 120; s.ball_vx = -4; s.ball_vy = 0;
            return game.step();
        };
        unsigned events = point();
        CHECK((events & ctg::sound_point) && !(events & ctg::sound_win));
        CHECK(s.score[1] == 1 && s.winner == -1 && s.serve_wait > 0);
        s.score[1] = std::numeric_limits<unsigned>::max();
        events = point();
        CHECK((events & ctg::sound_point) && !(events & ctg::sound_win));
        CHECK(s.score[1] == std::numeric_limits<unsigned>::max() && s.winner == -1);
        game.set_endless(false);
        CHECK(point() & ctg::sound_win);
        CHECK(s.winner == 1 && s.score[1] == std::numeric_limits<unsigned>::max());
    }
    return 0;
}

int winning_scores_and_paddle_options() {
    for (unsigned target : {5u, 10u, 15u, 20u, 25u, 30u}) for (int scorer : {0, 1}) {
        ctg::TennisB game;
        CHECK(game.target_score() == 15);
        game.set_options(true, {true, false});
        game.set_winning_score(target);
        game.reset();
        CHECK(game.target_score() == target && game.state().high_speed && game.state().small_paddle[0]);
        auto& s = const_cast<ctg::TennisState&>(game.state());
        for (unsigned previous : {target - 2, target - 1, target + 1}) {
            game.reset();
            s.score[scorer] = previous;
            s.serve_wait = 0; s.ball_x = scorer == 0 ? 375 : -55; s.ball_y = 100;
            s.ball_vx = scorer == 0 ? 4 : -4; s.ball_vy = 0;
            const unsigned events = game.step();
            CHECK(events & ctg::sound_point);
            CHECK(bool(events & ctg::sound_win) == (previous + 1 >= target));
            CHECK(s.winner == (previous + 1 >= target ? scorer : -1));
            CHECK(s.score[scorer] == previous + 1);
        }
    }
    for (bool hockey : {false, true}) for (bool doubles : {false, true}) for (int owner : {0, 1}) {
        ctg::TennisB game(doubles, hockey);
        game.set_options(true, {owner == 0, owner == 1});
        game.reset();
        CHECK(game.state().high_speed && game.paddle_half(owner) == 10 && game.paddle_pixels(1 - owner) == 32);
        for (unsigned i = 0; i < ctg::TennisB::hockey_serve_delay; ++i) game.step();
        CHECK(std::abs(game.state().ball_vx) == 8);
        game.reset();
        auto& s = const_cast<ctg::TennisState&>(game.state());
        s.serve_wait = 0; s.paddle_y = {120, 120};
        s.ball_x = hockey ? (owner == 0 ? 22 : 298) : (owner == 0 ? 20 : 300);
        s.ball_y = 136; s.ball_vx = owner == 0 ? -4 : 4; s.ball_vy = 0;
        CHECK(!(game.step() & ctg::sound_paddle));
        game.set_options(false, {false, false});
        s.ball_x = hockey ? (owner == 0 ? 22 : 298) : (owner == 0 ? 20 : 300); s.ball_y = 136; s.ball_vx = owner == 0 ? -4 : 4; s.ball_vy = 0;
        CHECK(game.step() & ctg::sound_paddle);
    }
    for (bool hockey : {false, true}) for (int owner : {0, 1}) {
        ctg::TennisB game(true, hockey);
        game.set_options(false, {owner == 0, owner == 1});
        auto& s = const_cast<ctg::TennisState&>(game.state());
        s.serve_wait = 0; s.paddle_y = {120, 120};
        const int inner = hockey ? (owner == 0 ? ctg::hockey_inner_right : ctg::hockey_inner_left)
                                 : (owner == 0 ? ctg::TennisB::inner_left : ctg::TennisB::inner_right);
        int x = hockey ? ctg::hockey_paddle_face(inner, true) : inner;
        if (!hockey) x -= ctg::TennisB::ball_radius;
        s.ball_x = x - 2; s.ball_y = 136; s.ball_vx = 4; s.ball_vy = 0;
        if (!hockey && owner == 0) { s.ball_x = inner + 4; s.ball_vx = -4; }
        CHECK(!(game.step() & ctg::sound_paddle));
        game.set_options(false, {false, false});
        s.ball_x = x - 2; s.ball_y = 136; s.ball_vx = 4; s.ball_vy = 0;
        if (!hockey && owner == 0) { s.ball_x = inner + 4; s.ball_vx = -4; }
        CHECK(game.step() & ctg::sound_paddle);
    }
    return 0;
}

} // namespace tennis_tests
