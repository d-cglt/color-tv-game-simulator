#include "tennis_test_harness.h"
#include "tennis_b.h"

#include <initializer_list>

namespace tennis_tests {

int tennis_lifecycle_and_doubles() {
    for (bool doubles_mode : {false, true}) {
        for (int missed = 0; missed < 2; ++missed) {
            ctg::TennisB a(doubles_mode), b(doubles_mode);
            auto& as = const_cast<ctg::TennisState&>(a.state());
            auto& bs = const_cast<ctg::TennisState&>(b.state());
            CHECK(as.serve_wait == ctg::TennisB::hockey_serve_delay);
            as.serve_wait = bs.serve_wait = 0;
            as.ball_x = bs.ball_x = missed == 0 ? -60 : 380;
            as.ball_y = bs.ball_y = 120;
            as.ball_vx = bs.ball_vx = missed == 0 ? -4 : 4;
            CHECK(a.step() == ctg::sound_point && b.step() == ctg::sound_point);
            CHECK(as.score[1 - missed] == 1 && as.serve_wait == ctg::TennisB::hockey_serve_delay);
            for (unsigned i = 0; i < ctg::TennisB::hockey_serve_delay; ++i) {
                CHECK(a.step() == ctg::sound_none && b.step() == ctg::sound_none);
                CHECK(same(as, bs));
            }
            CHECK(as.serve_wait == 0 && as.ball_x == (missed == 0 ? 13 : 298));
            CHECK((missed == 0 && as.ball_vx > 0) || (missed == 1 && as.ball_vx < 0));
            if (doubles_mode) {
                as.paddle_y[missed] = as.ball_y;
                for (int i = 0; i < 20 && (missed == 0 ? as.ball_x < 96 : as.ball_x > 224); ++i) {
                    as.paddle_y[missed] = as.ball_y;
                    CHECK(!(a.step() & ctg::sound_paddle));
                }
            }
            a.reset();
            ctg::TennisB fresh(doubles_mode);
            CHECK(same(a.state(), fresh.state()));
        }
    }
    ctg::TennisB game, twin;
    CHECK(same(game.state(), twin.state()));
    CHECK(game.state().winner == -1 && game.state().score[0] == 0);
    for (int i = 0; i < 100; ++i) game.step({-100, 100});
    CHECK(game.state().paddle_y[0] == game.paddle_min(0));
    CHECK(game.state().paddle_y[1] == game.paddle_max(1));
    game.reset();
    CHECK(same(game.state(), twin.state()));

    unsigned walls = 0, paddles = 0;
    for (int i = 0; i < 5000; ++i) {
        const auto& s = game.state();
        ctg::TennisInput input{(s.ball_y > s.paddle_y[0]) - (s.ball_y < s.paddle_y[0]),
                               (s.ball_y > s.paddle_y[1]) - (s.ball_y < s.paddle_y[1])};
        unsigned events = game.step(input);
        CHECK(events == twin.step(input));
        CHECK(same(game.state(), twin.state()));
        walls += !!(events & ctg::sound_wall);
        paddles += !!(events & ctg::sound_paddle);
        CHECK(!(events & ctg::sound_point));
        CHECK(game.state().serve_wait || (game.state().ball_y >= ctg::TennisB::ball_radius &&
              game.state().ball_y <= ctg::TennisB::height - ctg::TennisB::ball_radius));
    }
    CHECK(walls > 0 && paddles > 2);

    // Keep both paddles away from the serve trajectory until a player wins.
    game.reset();
    unsigned points = 0, wins = 0;
    for (int i = 0; i < 10000 && game.state().winner == -1; ++i) {
        unsigned events = game.step({-1, -1});
        points += !!(events & ctg::sound_point);
        wins += !!(events & ctg::sound_win);
        if ((events & ctg::sound_point) && !(events & ctg::sound_win)) {
            CHECK(game.state().serve_wait == ctg::TennisB::hockey_serve_delay);
            CHECK(game.state().ball_vx == 0 && game.state().ball_vy == 0);
        }
    }
    CHECK(game.state().winner != -1);
    CHECK(game.state().score[game.state().winner] == 15);
    CHECK(points == game.state().score[0] + game.state().score[1] && wins == 1);
    const auto final = game.state();
    CHECK(game.step({1, 1}) == ctg::sound_none && same(game.state(), final));
    game.reset();
    // The right player returns the first serve while the left player misses.
    for (int i = 0; i < 1000 && game.state().score[1] == 0; ++i) {
        const auto& s = game.state();
        game.step({-1, (s.ball_y > s.paddle_y[1]) - (s.ball_y < s.paddle_y[1])});
    }
    CHECK(game.state().score[1] == 1 && game.state().score[0] == 0);
    CHECK(game.state().serve_wait == ctg::TennisB::hockey_serve_delay);
    game.reset();
    ctg::TennisB fresh;
    CHECK(same(game.state(), fresh.state()));
    ctg::TennisB doubles(true);
    CHECK(doubles.state().doubles && !fresh.state().doubles);
    doubles.step({-1, 1});
    CHECK(doubles.state().paddle_y[0] == 116 && doubles.state().paddle_y[1] == 124);
    doubles.reset();
    CHECK(doubles.state().doubles && doubles.state().paddle_y[0] == 120);
    // Front faces rebound; behind-side crossings pass through without deflection.
    for (bool tennis_a : {false, true}) for (int player = 0; player < 2; ++player) {
        for (int direction : {-1, 1}) {
            doubles = ctg::TennisB(true, false, false, tennis_a);
            doubles.reset();
            auto& s = const_cast<ctg::TennisState&>(doubles.state());
            const int x = player == 0 ? ctg::TennisB::inner_left : ctg::TennisB::inner_right;
            s.serve_wait = 0;
            s.ball_x = x - direction * (ctg::TennisB::ball_radius + 2);
            s.ball_y = s.paddle_y[player];
            s.ball_vx = direction * 4; s.ball_vy = 0;
            const unsigned events = doubles.step();
            const bool front = player == 0 ? direction < 0 : direction > 0;
            CHECK(bool(events & ctg::sound_paddle) == front);
            CHECK((events & (ctg::event_player1_hit | ctg::event_player2_hit)) ==
                  (front ? (player == 0 ? ctg::event_player1_hit : ctg::event_player2_hit) : 0u));
            CHECK(s.ball_vx == (front ? -direction : direction) * 4);
            CHECK(s.ball_x == (front ? x - direction * (ctg::TennisB::ball_radius + 2)
                                     : x - direction * (ctg::TennisB::ball_radius - 2)));
            if (!front) CHECK(s.ball_vy == 0 && s.ball_y == s.paddle_y[player]);
        }
    }
    // The same inner crossing is non-colliding in singles.
    auto& s = const_cast<ctg::TennisState&>(fresh.state());
    s.serve_wait = 0;
    s.ball_x = ctg::TennisB::inner_right - 4; s.ball_y = 120;
    s.ball_vx = 4; s.ball_vy = 0;
    CHECK(!(fresh.step() & ctg::sound_paddle) && s.ball_vx == 4);
    return 0;
}

} // namespace tennis_tests
