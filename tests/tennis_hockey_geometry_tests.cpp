#include "tennis_test_harness.h"
#include "hockey_ball.h"
#include "tennis_b.h"
#include <algorithm>

#include <initializer_list>

namespace tennis_tests {

int hockey_paddle_geometry() {
    // Center-based faces: crossing from either side, including full-speed
    // crossings, reflects overshoot and attributes the hit to the paddle owner.
    CHECK(ctg::hockey_paddle_face(49, false) == 20);
    CHECK(ctg::hockey_paddle_face(265, true) == 300);
    CHECK(ctg::hockey_paddle_face(121, true) == 111);
    CHECK(ctg::hockey_paddle_face(121, false) == 115);
    CHECK(ctg::hockey_paddle_face(193, true) == 205);
    CHECK(ctg::hockey_paddle_face(193, false) == 209);
    for (bool variant : {false, true}) for (int paddle = 0; paddle < 4; ++paddle)
        for (int direction : {-1, 1}) for (int speed : {4, 8}) {
            const bool inner = paddle >= 2;
            const int owner = inner ? 3 - paddle : paddle;
            const int x = paddle == 0 ? 49 : paddle == 1 ? 265 : paddle == 2 ? 121 : 193;
            const int face = ctg::hockey_paddle_face(x, direction > 0);
            ctg::TennisB game(inner, true, false, false, variant);
            auto& state = const_cast<ctg::TennisState&>(game.state());
            state.serve_wait = 0; state.paddle_y = {120, 120};
            state.ball_x = face - direction * (speed - 1);
            state.ball_y = 120; state.ball_vx = direction * speed; state.ball_vy = 0;
            const bool front = inner || (paddle == 0 ? direction < 0 : direction > 0);
            const bool back = inner && (paddle == 2 ? direction < 0 : direction > 0);
            const unsigned events = game.step();
            CHECK(bool(events & ctg::sound_paddle) == front);
            if (front) {
                CHECK(state.ball_x == face + (back ? direction : -direction));
                CHECK(state.ball_vx == (back ? direction : -direction) * speed);
                CHECK(events & (owner == 0 ? ctg::event_player1_hit : ctg::event_player2_hit));
                CHECK(!(game.step() & ctg::sound_paddle));
            }
        }
    // Hockey doubles' inner paddles belong to the opposite-side controller,
    // and both faces use the same clipped geometry used to draw them.
    for (bool hockey_a : {false, true}) {
    ctg::TennisB hockey_doubles(true, true, false, false, hockey_a);
    auto& hd = const_cast<ctg::TennisState&>(hockey_doubles.state());
    for (int inner = 0; inner < 2; ++inner) {
        const int x = inner == 0 ? ctg::hockey_inner_left : ctg::hockey_inner_right;
        for (int direction : {-1, 1}) for (int paddle_y : {0, 20, 120, 220, 240}) {
            for (int y = 2; y <= 238; ++y) {
                hockey_doubles.reset(); hd.serve_wait = 0;
                hd.paddle_y[1 - inner] = paddle_y;
                hd.paddle_y[inner] = paddle_y < 120 ? 240 : 0;
                const int face = ctg::hockey_paddle_face(x, direction > 0);
                hd.ball_x = face - direction * 2;
                hd.ball_y = y; hd.ball_vx = direction * 4; hd.ball_vy = 0;
                const int top = std::max(20, ctg::hockey_paddle_top(paddle_y, 32));
                const int bottom = std::min(220, ctg::hockey_paddle_top(paddle_y, 32) + 32);
                const bool hit = ctg::hockey_y(y) + 3 > top && ctg::hockey_y(y) - 2 < bottom;
                const unsigned events = hockey_doubles.step();
                CHECK(bool(events & ctg::sound_paddle) == hit);
                CHECK((events & (ctg::event_player1_hit | ctg::event_player2_hit)) ==
                      (hit ? (inner == 0 ? ctg::event_player2_hit : ctg::event_player1_hit) : 0u));
                const bool back = inner == 0 ? direction < 0 : direction > 0;
                CHECK(hd.ball_vx == (hit && !back ? -direction : direction) * 4);
                if (hit) CHECK(!(hockey_doubles.step() & ctg::sound_paddle));
            }
        }
    }
    hockey_doubles.reset();
    hockey_doubles.step({-1, 1});
    CHECK(hd.paddle_y[0] == 116 && hd.paddle_y[1] == 124);
    hockey_doubles.reset();
    CHECK(hd.hockey && hd.doubles && hd.hockey_a == hockey_a && hd.paddle_y[0] == 120 && hd.paddle_y[1] == 120);
    }
    // Both hockey variants preserve horizontal movement and reverse vertical velocity on back contacts.
    for (bool variant : {false, true}) for (int inner : {0, 1}) for (int direction : {-1, 1})
        for (int speed : {4, 8}) for (int vy : {-3, 0, 3}) {
            ctg::TennisB game(true, true, false, false, variant);
            game.set_options(speed == 8, {false, false});
            auto& state = const_cast<ctg::TennisState&>(game.state());
            state.serve_wait = 0;
            state.paddle_y = {120, 120};
            const int face = ctg::hockey_paddle_face(inner == 0 ? ctg::hockey_inner_left : ctg::hockey_inner_right, direction > 0);
            state.ball_x = face - direction * (speed - 1);
            state.ball_y = 120 - vy; state.ball_vx = direction * speed; state.ball_vy = vy;
            const bool back = inner == 0 ? direction < 0 : direction > 0;
            const unsigned events = game.step();
            CHECK((events & (ctg::sound_paddle | ctg::event_player1_hit | ctg::event_player2_hit)) ==
                  (ctg::sound_paddle | (inner == 0 ? ctg::event_player2_hit : ctg::event_player1_hit)));
            CHECK(state.ball_x == face + (back ? direction : -direction));
            CHECK(state.ball_vx == (back ? direction : -direction) * speed);
            if (back) CHECK(state.ball_vy == -vy);
            CHECK(!(game.step() & ctg::sound_paddle));
        }
    return 0;
}

} // namespace tennis_tests
