#include "tennis_test_harness.h"
#include "hockey_ball.h"
#include "tennis_b.h"
#include <algorithm>

#include <initializer_list>

namespace tennis_tests {

int hockey_a_barrier() {
    // Hockey A: either end wall closes the opening; any center hit opens it.
    for (bool doubles : {false, true}) for (int direction : {-1, 1}) {
        ctg::TennisB a(doubles, true, false, false, true);
        auto& s = const_cast<ctg::TennisState&>(a.state());
        s.serve_wait = 0;
        s.ball_x = direction < 0 ? 14 : 306; s.ball_y = 30;
        s.ball_vx = direction * 4; s.ball_vy = 0;
        CHECK(a.step() & ctg::sound_wall);
        CHECK(s.hockey_barrier_closed && s.ball_vx == -direction * 4);
        s.ball_x = direction < 0 ? 150 : 170; s.ball_y = 120;
        s.ball_vx = -direction * 4;
        CHECK(a.step() & ctg::sound_wall);
        CHECK(!s.hockey_barrier_closed && s.ball_vx == direction * 4);
        s.ball_x = direction < 0 ? 150 : 170; s.ball_vx = -direction * 4;
        CHECK(!(a.step() & ctg::sound_wall));
        CHECK(s.ball_vx == -direction * 4);
        for (int y : {30, 210}) {
            s.ball_x = direction < 0 ? 150 : 170; s.ball_y = y; s.ball_vx = -direction * 4;
            CHECK(a.step() & ctg::sound_wall);
            CHECK(s.ball_vx == direction * 4 && !s.hockey_barrier_closed);
        }
        s.hockey_barrier_closed = true;
        s.ball_x = direction < 0 ? -60 : 380; s.ball_y = 120; s.ball_vx = direction * 4;
        CHECK(a.step() & ctg::sound_point);
        CHECK(s.score[direction < 0 ? 1 : 0] == 1 && s.hockey_barrier_closed);
        CHECK(s.serve_wait == ctg::TennisB::hockey_serve_delay);
        for (unsigned i = 0; i < ctg::TennisB::hockey_serve_delay; ++i) a.step();
        CHECK(s.hockey_barrier_closed && s.serve_wait == 0);
        s.score[direction < 0 ? 1 : 0] = 14;
        s.ball_x = direction < 0 ? -60 : 380; s.ball_y = 120; s.ball_vx = direction * 4; s.ball_vy = 0;
        const unsigned win = a.step();
        CHECK((win & ctg::sound_point) && (win & ctg::sound_win));
        CHECK(s.winner == (direction < 0 ? 1 : 0) && s.hockey_barrier_closed);
        a.reset();
        CHECK(s.hockey && s.hockey_a && s.doubles == doubles && !s.hockey_barrier_closed && s.score[0] == 0 && s.score[1] == 0);
    }
    return 0;
}

int hockey_lifecycle_and_scoring() {
    ctg::TennisB hockey(false, true);
    auto& h = const_cast<ctg::TennisState&>(hockey.state());
    ctg::TennisB hockey_twin(false, true);
    CHECK(h.serve_wait == ctg::TennisB::hockey_serve_delay);
    for (unsigned i = 0; i < ctg::TennisB::hockey_serve_delay; ++i) {
        const unsigned events = hockey.step({-1, 1});
        CHECK(events == ctg::sound_none);
        CHECK(events == hockey_twin.step({-1, 1}));
        CHECK(same(h, hockey_twin.state()));
        CHECK(h.serve_wait == ctg::TennisB::hockey_serve_delay - i - 1);
    }
    CHECK(h.ball_x == 13 && h.ball_vx == 4);
    CHECK(h.paddle_y[0] == hockey.paddle_min(0));
    CHECK(h.paddle_y[1] == hockey.paddle_max(1));
    // A hockey ball on the scoring side can leave the field vertically. Only
    // its overlap with the visible paddle, not the hidden extension, can hit.
    for (bool small : {false, true}) for (int player = 0; player < 2; ++player)
        for (int direction : {-1, 1}) for (bool visible : {false, true}) {
            ctg::TennisB game(false, true);
            game.set_options(false, {small, small});
            auto& s = const_cast<ctg::TennisState&>(game.state());
            s.serve_wait = 0; s.ball_x = 0; s.ball_y = 120; s.ball_vx = s.ball_vy = 0;
            game.step(); // Enter the scoring side, where vertical wall bounces are disabled.
            s.paddle_y[player] = direction < 0 ? game.paddle_min(player) : game.paddle_max(player);
            const int face = ctg::hockey_paddle_face(player == 0 ? ctg::hockey_outer_left : ctg::hockey_outer_right, player == 1);
            s.ball_x = face + (player == 0 ? 2 : -2);
            s.ball_y = direction < 0 ? (visible ? 0 : -10) : (visible ? 252 : 260);
            s.ball_vx = player == 0 ? -4 : 4; s.ball_vy = 0;
            const unsigned events = game.step();
            CHECK(bool(events & ctg::sound_paddle) == visible);
            CHECK(bool(events & (player == 0 ? ctg::event_player1_hit : ctg::event_player2_hit)) == visible);
        }
    // Test every reachable ball row against the clipped, drawn paddle on both sides.
    for (int player = 0; player < 2; ++player) {
        for (int paddle_y : {hockey.paddle_min(player), 0, 1, 20, 120, 220, 239, 240, hockey.paddle_max(player)}) {
            for (int y = 2; y <= 238; ++y) {
                hockey.reset(); h.serve_wait = 0;
                h.paddle_y[player] = paddle_y;
                h.ball_x = ctg::hockey_paddle_face(player == 0 ? ctg::hockey_outer_left : ctg::hockey_outer_right, player == 1) + (player == 0 ? 2 : -2);
                h.ball_y = y; h.ball_vx = player == 0 ? -4 : 4; h.ball_vy = 0;
                const int top = std::max(20, ctg::hockey_paddle_top(paddle_y, 32));
                const int bottom = std::min(220, ctg::hockey_paddle_top(paddle_y, 32) + 32);
                const int center = ctg::hockey_y(y);
                const bool contact = center + 3 > top && center - 2 < bottom;
                const unsigned events = hockey.step();
                CHECK(bool(events & ctg::sound_paddle) == contact);
                CHECK((events & (ctg::event_player1_hit | ctg::event_player2_hit)) ==
                      (contact ? (player == 0 ? ctg::event_player1_hit : ctg::event_player2_hit) : 0u));
                CHECK(h.ball_vx == (contact ? (player == 0 ? 4 : -4) : (player == 0 ? -4 : 4)));
            }
        }
    }
    hockey.reset();
    for (int i = 0; i < 100; ++i) hockey.step({-1, 1});
    CHECK(h.paddle_y[0] == hockey.paddle_min(0) && h.paddle_y[1] == hockey.paddle_max(1));
    hockey.step({1, -1});
    CHECK(h.paddle_y[0] == hockey.paddle_min(0) + 4 && h.paddle_y[1] == hockey.paddle_max(1) - 4);
    hockey.step({0, 0});
    CHECK(h.paddle_y[0] == hockey.paddle_min(0) + 4 && h.paddle_y[1] == hockey.paddle_max(1) - 4);
    for (int direction : {-1, 1}) {
        for (int y : {30, 74, 166, 210}) {
            hockey.reset(); h.serve_wait = 0;
            h.ball_x = direction < 0 ? 14 : 306;
            h.ball_y = y; h.ball_vx = direction * 4; h.ball_vy = 0;
            const unsigned events = hockey.step();
            CHECK((events & ctg::sound_wall) && !(events & ctg::sound_point));
            CHECK(h.ball_vx == -direction * 4);
        }
        for (int y : {77, 120, 163}) {
            hockey.reset(); h.serve_wait = 0;
            h.ball_x = direction < 0 ? 14 : 306;
            h.ball_y = y; h.ball_vx = direction * 4; h.ball_vy = 0;
            CHECK(!(hockey.step() & ctg::sound_wall));
            unsigned events = 0;
            for (int i = 0; i < 30 && !(events & ctg::sound_point); ++i) events = hockey.step();
            CHECK(events & ctg::sound_point);
            CHECK(h.score[direction < 0 ? 1 : 0] == 1);
            CHECK(h.serve_wait == ctg::TennisB::hockey_serve_delay);
            for (unsigned i = 1; i < ctg::TennisB::hockey_serve_delay; ++i) {
                CHECK(hockey.step() == ctg::sound_none);
                CHECK(h.serve_wait == ctg::TennisB::hockey_serve_delay - i);
            }
            CHECK(hockey.step() == ctg::sound_none && h.serve_wait == 0);
            CHECK(h.ball_x == (direction < 0 ? 13 : 298));
            CHECK(h.ball_vx == -direction * 4);
            CHECK(h.ball_y >= ctg::TennisB::ball_radius && h.ball_y <= ctg::TennisB::height - ctg::TennisB::ball_radius);
            h.paddle_y[direction < 0 ? 0 : 1] = h.ball_y;
            CHECK(!(hockey.step() & ctg::sound_paddle));
            CHECK(h.ball_vx == -direction * 4);
            CHECK(h.ball_x == (direction < 0 ? 17 : 294));
        }
    }

    // Repeated side serves remain deterministic and cover multiple positions and slopes.
    for (int side = 0; side < 2; ++side) {
        ctg::TennisB a(false, true), b(false, true);
        auto& as = const_cast<ctg::TennisState&>(a.state());
        auto& bs = const_cast<ctg::TennisState&>(b.state());
        bool upward = false, downward = false, steep = false;
        int min_y = ctg::TennisB::height, max_y = 0;
        for (int n = 0; n < 100; ++n) {
            for (auto* state : {&as, &bs}) {
                state->serve_wait = 0;
                state->score = {};
                state->ball_x = side == 0 ? -55 : 375;
                state->ball_y = 120;
                state->ball_vx = side == 0 ? -4 : 4;
                state->ball_vy = 0;
            }
            CHECK(a.step() == b.step());
            CHECK(as.serve_wait == ctg::TennisB::hockey_serve_delay);
            for (unsigned i = 0; i < ctg::TennisB::hockey_serve_delay; ++i)
                CHECK(a.step() == b.step());
            CHECK(same(as, bs));
            CHECK(as.ball_x == (side == 0 ? 13 : 298));
            CHECK(as.ball_vx == (side == 0 ? 4 : -4));
            CHECK(as.ball_y >= 2 && as.ball_y <= 208);
            CHECK(as.ball_vy >= -4 && as.ball_vy <= (side == 0 ? 4 : 6));
            if (as.ball_y < min_y) min_y = as.ball_y;
            if (as.ball_y > max_y) max_y = as.ball_y;
            upward |= as.ball_vy < 0;
            downward |= as.ball_vy > 0;
            CHECK(as.ball_vy != 0);
            steep |= as.ball_vy > 4;
        }
        CHECK(upward && downward && max_y - min_y > 150);
        if (side == 1) CHECK(steep);
    }
    // The opening includes exactly the rows where the entire ball fits.
    for (bool doubles : {false, true}) for (int direction : {-1, 1}) {
        for (int y : {76, 77, 163, 164}) {
            ctg::TennisB edge(doubles, true);
            auto& e = const_cast<ctg::TennisState&>(edge.state());
            e.serve_wait = 0;
            e.ball_x = direction < 0 ? 14 : 306;
            e.ball_y = y; e.ball_vx = direction * 4; e.ball_vy = 0;
            const bool opening = y >= 77 && y <= 163;
            const unsigned first = edge.step();
            CHECK(bool(first & ctg::sound_wall) == !opening);
            if (opening) {
                unsigned events = 0;
                for (int i = 0; i < 30 && !(events & ctg::sound_point); ++i) events = edge.step();
                CHECK(events & ctg::sound_point);
                CHECK(e.score[direction < 0 ? 1 : 0] == 1);
            } else CHECK(e.ball_vx == -direction * 4);
        }
        // Reaching the side latches suppression even if the ball returns inward.
        ctg::TennisB edge(doubles, true);
        auto& e = const_cast<ctg::TennisState&>(edge.state());
        e.serve_wait = 0;
        e.ball_x = direction < 0 ? 0 : 320;
        e.ball_y = 120; e.ball_vx = -direction * 4; e.ball_vy = 0;
        CHECK(!(edge.step() & (ctg::sound_wall | ctg::sound_point)));
        e.ball_y = 2; e.ball_vy = -3;
        CHECK(!(edge.step() & ctg::sound_wall));
        CHECK(e.ball_y == -1 && e.ball_vy == -3);
        e.ball_x = direction < 0 ? 14 : 306;
        e.ball_y = 238; e.ball_vx = direction * 4; e.ball_vy = 3;
        CHECK(!(edge.step() & (ctg::sound_wall | ctg::sound_point)));
        CHECK(e.ball_y == 241 && e.ball_vy == 3 && e.ball_vx == direction * 4);
        e.ball_x = direction < 0 ? -55 : 375;
        e.ball_y = 120; e.ball_vx = direction * 4; e.ball_vy = 0;
        CHECK(edge.step() & ctg::sound_point);
        e.serve_wait = 0; e.ball_x = 160; e.ball_y = 2; e.ball_vx = 0; e.ball_vy = -3;
        CHECK(edge.step() & ctg::sound_wall);
        CHECK(e.ball_vy == 3);
        e.ball_x = direction < 0 ? 0 : 320;
        e.ball_y = 120; e.ball_vx = -direction * 4; e.ball_vy = 0;
        edge.step();
        edge.reset(); e.serve_wait = 0;
        e.ball_x = 160; e.ball_y = 238; e.ball_vx = 0; e.ball_vy = 3;
        CHECK(edge.step() & ctg::sound_wall);
    }
    // Full-display vertical escapes score for the left player when no pixels remain.
    for (bool doubles : {false, true}) for (int direction : {-1, 1}) {
        for (int end : {-1, 1}) {
            ctg::TennisB escape(doubles, true);
            auto& s = const_cast<ctg::TennisState&>(escape.state());
            s.serve_wait = 0;
            s.ball_x = direction < 0 ? 0 : 320;
            s.ball_y = 120; s.ball_vx = -direction * 4; s.ball_vy = 0;
            CHECK(!(escape.step() & ctg::sound_point));
            s.ball_x = 160;
            s.ball_y = end < 0 ? -29 : 281;
            s.ball_vx = 0; s.ball_vy = end;
            CHECK(!(escape.step() & (ctg::sound_point | ctg::sound_wall)));
            CHECK(s.ball_y == (end < 0 ? -30 : 282));
            CHECK(escape.step() == ctg::sound_point);
            CHECK(s.score[0] == 1 && s.score[1] == 0);
            CHECK(s.serve_wait == ctg::TennisB::hockey_serve_delay);
            s.serve_wait = 0; s.ball_x = 160; s.ball_y = 2; s.ball_vy = -3;
            CHECK(escape.step() & ctg::sound_wall);
        }
        ctg::TennisB winning(doubles, true);
        auto& s = const_cast<ctg::TennisState&>(winning.state());
        s.serve_wait = 0; s.score[0] = 14; s.ball_x = -1; s.ball_y = -31;
        CHECK(winning.step() == (ctg::sound_point | ctg::sound_win));
        CHECK(s.winner == 0 && s.score[0] == 15 && s.score[1] == 0);
    }
    for (bool doubles : {false, true}) for (bool volleyball : {false, true}) {
        ctg::TennisB other(doubles, false, volleyball);
        auto& s = const_cast<ctg::TennisState&>(other.state());
        s.serve_wait = 0; s.ball_x = 160; s.ball_y = -18; s.ball_vx = 0; s.ball_vy = -1;
        CHECK((other.step() & (ctg::sound_wall | ctg::sound_point)) == ctg::sound_wall);
        CHECK(s.score[0] == 0 && s.score[1] == 0);
    }
    // Hockey singles has no inner paddles, even on a front-face crossing.
    for (int direction : {-1, 1}) {
        hockey.reset(); h.serve_wait = 0;
        h.ball_x = direction < 0 ? ctg::TennisB::inner_left + 4 : ctg::TennisB::inner_right - 4;
        h.ball_y = 120; h.ball_vx = direction * 4; h.ball_vy = 0;
        CHECK(!(hockey.step() & ctg::sound_paddle));
        CHECK(h.ball_vx == direction * 4);
    }
    hockey.reset();
    CHECK(h.hockey && !h.doubles && h.score[0] == 0 && h.score[1] == 0);
    h.serve_wait = 0; h.score[0] = 14; h.ball_x = 375; h.ball_y = 120; h.ball_vx = 4; h.ball_vy = 0;
    CHECK((hockey.step() & (ctg::sound_point | ctg::sound_win)) == (ctg::sound_point | ctg::sound_win));
    CHECK(h.winner == 0 && h.score[0] == 15 && h.serve_wait == 0);
    const auto hockey_final = h;
    CHECK(hockey.step() == ctg::sound_none && same(h, hockey_final));
    return 0;
}

} // namespace tennis_tests
