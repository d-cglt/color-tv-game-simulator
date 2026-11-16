#include "tennis_test_harness.h"
#include "tennis_b.h"

#include <initializer_list>

namespace tennis_tests {

int volleyball_singles() {
    // Volleyball B checks the rendered center before movement, on every update in the band.
    ctg::TennisB volleyball(false, false, true), volleyball_twin(false, false, true);
    auto& v = const_cast<ctg::TennisState&>(volleyball.state());
    auto& vt = const_cast<ctg::TennisState&>(volleyball_twin.state());
    CHECK(v.volleyball && !v.hockey && !v.doubles && v.serve_wait == 130);
    for (int i = 0; i < 130; ++i) CHECK(volleyball.step() == volleyball_twin.step());
    CHECK(v.serve_wait == 0 && v.ball_x == 35 && same(v, vt));
    int reversed = 0, unchanged = 0;
    for (int n = 0; n < 1000; ++n) {
        const int direction = n % 2 ? -1 : 1;
        for (auto* state : {&v, &vt}) {
            state->ball_x = 160;
            state->ball_y = 120; state->ball_vx = direction * 4; state->ball_vy = 2;
        }
        CHECK(volleyball.step() == ctg::sound_wall && volleyball_twin.step() == ctg::sound_wall);
        CHECK(same(v, vt) && v.ball_vx == direction * 4 && v.ball_x == 160 + direction * 4);
        CHECK(v.ball_y == 120 + v.ball_vy);
        reversed += v.ball_vy == -2; unchanged += v.ball_vy == 2;
        CHECK(!(volleyball.step() & ctg::sound_wall));
        volleyball_twin.step();
        CHECK(same(v, vt));
    }
    CHECK(reversed > 800 && reversed < 950 && unchanged > 50);
    for (bool doubles_mode : {false, true}) {
        ctg::TennisB band(doubles_mode, false, true), fresh(doubles_mode, false, true);
        auto& s = const_cast<ctg::TennisState&>(band.state());
        auto& f = const_cast<ctg::TennisState&>(fresh.state());
        s.serve_wait = f.serve_wait = 0;
        for (int direction : {-1, 1}) {
            // Outside -> inside: the first sound is delayed until the next update.
            s.ball_x = direction > 0 ? 158 : 163;
            s.ball_y = 120; s.ball_vx = direction; s.ball_vy = 2;
            CHECK(band.step() == ctg::sound_none);
            for (int i = 0; i < 4; ++i) {
                const int old_y = s.ball_y;
                CHECK(band.step() == ctg::sound_wall);
                CHECK(s.ball_vx == direction && s.ball_y == old_y + s.ball_vy);
            }
            CHECK(band.step() == ctg::sound_none);
        }
        // Reset restores the random stream, including after repeated checks.
        band.reset(); s.serve_wait = 0;
        for (int i = 0; i < 30; ++i) {
            s.ball_x = f.ball_x = 160; s.ball_y = f.ball_y = 120;
            s.ball_vx = f.ball_vx = 0; s.ball_vy = f.ball_vy = 2;
            CHECK(band.step() == ctg::sound_wall && fresh.step() == ctg::sound_wall);
            CHECK(same(s, f));
        }
    }
    v.ball_x = 375; v.ball_y = 120; v.ball_vx = 4; v.ball_vy = 0;
    CHECK(volleyball.step() & ctg::sound_point);
    CHECK(v.score[0] == 1 && v.serve_wait == 130);
    for (int i = 0; i < 130; ++i) volleyball.step();
    CHECK(v.ball_x == 35 && v.ball_vx == 4 && v.ball_y == v.paddle_y[0] && v.ball_vy == 0);
    v.score[0] = 14; v.ball_x = 375; v.ball_y = 120; v.ball_vx = 4; v.ball_vy = 0;
    CHECK((volleyball.step() & (ctg::sound_point | ctg::sound_win)) == (ctg::sound_point | ctg::sound_win));
    CHECK(v.winner == 0);
    volleyball.reset();
    CHECK(v.volleyball && v.winner == -1 && v.score[0] == 0 && v.serve_wait == 130);
    return 0;
}

int volleyball_doubles() {
    // Volleyball doubles uses same-side pairs and the singles net and serve rules.
    ctg::TennisB vd(true, false, true), vs(false, false, true);
    auto& d = const_cast<ctg::TennisState&>(vd.state());
    auto& single = const_cast<ctg::TennisState&>(vs.state());
    CHECK(d.doubles && d.volleyball && !d.hockey && d.serve_wait == 130);
    vd.step({-1, 1});
    CHECK(d.paddle_y[0] == 116 && d.paddle_y[1] == 124);
    for (int player = 0; player < 2; ++player) for (int direction : {-1, 1}) {
        vd.reset(); d.serve_wait = 0;
        d.paddle_y[player] = 80; d.paddle_y[1 - player] = 160;
        const int x = player == 0 ? ctg::TennisB::inner_left : ctg::TennisB::inner_right;
        d.ball_x = x - direction * 4; d.ball_y = 80;
        d.ball_vx = direction * 4; d.ball_vy = 0;
        const bool front = player == 0 ? direction < 0 : direction > 0;
        CHECK(bool(vd.step() & ctg::sound_paddle) == front);
        CHECK(d.ball_vx == (front ? -direction : direction) * 4);
        if (!front) CHECK(d.ball_vy == 0);
        vd.reset(); d.serve_wait = 0;
        d.paddle_y[player] = 80; d.paddle_y[1 - player] = 160;
        d.ball_x = x - direction * 4; d.ball_y = 160;
        d.ball_vx = direction * 4; d.ball_vy = 0;
        CHECK(!(vd.step() & ctg::sound_paddle));
    }
    vd.reset(); vs.reset();
    for (int i = 0; i < 130; ++i) CHECK(vd.step() == vs.step());
    for (int n = 0; n < 100; ++n) {
        const int direction = n % 2 ? -1 : 1;
        for (auto* state : {&d, &single}) {
            state->ball_x = 160; state->ball_y = 120;
            state->ball_vx = direction * 4; state->ball_vy = 2;
        }
        CHECK(vd.step() == vs.step());
        CHECK(d.ball_x == single.ball_x && d.ball_vx == single.ball_vx && d.ball_vy == single.ball_vy);
        CHECK(vd.step() == vs.step());
    }
    d.ball_x = 375; d.ball_y = 120; d.ball_vx = 4; d.ball_vy = 0;
    CHECK(vd.step() & ctg::sound_point);
    CHECK(d.score[0] == 1 && d.serve_wait == 130);
    for (int i = 0; i < 130; ++i) vd.step();
    CHECK(d.ball_x == 35 && d.ball_vx == 4 && d.ball_y == d.paddle_y[0] && d.ball_vy == 0);
    d.score[0] = 14; d.ball_x = 375; d.ball_y = 120; d.ball_vx = 4; d.ball_vy = 0;
    CHECK((vd.step() & (ctg::sound_point | ctg::sound_win)) == (ctg::sound_point | ctg::sound_win));
    const auto vd_final = d;
    CHECK(vd.step({1, 1}) == ctg::sound_none && same(d, vd_final));
    vd.reset();
    CHECK(d.doubles && d.volleyball && d.score[0] == 0 && d.winner == -1 && d.serve_wait == 130);
    // Volleyball A doubles combines the same-side paddles with A's blocks.
    ctg::TennisB va(true, false, true, false, false, true);
    auto& a = const_cast<ctg::TennisState&>(va.state());
    va.step({-1, 1});
    CHECK(a.paddle_y[0] == 116 && a.paddle_y[1] == 124);
    for (int player = 0; player < 2; ++player) for (int direction : {-1, 1}) {
        va.reset(); a.serve_wait = 0;
        a.paddle_y[player] = 80; a.paddle_y[1 - player] = 160;
        const int x = player == 0 ? ctg::TennisB::inner_left : ctg::TennisB::inner_right;
        a.ball_x = x - direction * 4; a.ball_y = 80;
        a.ball_vx = direction * 4; a.ball_vy = 0;
        const bool front = player == 0 ? direction < 0 : direction > 0;
        CHECK(bool(va.step() & ctg::sound_paddle) == front);
        CHECK(a.ball_vx == (front ? -direction : direction) * 4);
        va.reset(); a.serve_wait = 0;
        a.paddle_y[player] = 80; a.paddle_y[1 - player] = 160;
        a.ball_x = x - direction * 4; a.ball_y = 160;
        a.ball_vx = direction * 4; a.ball_vy = 0;
        CHECK(!(va.step() & ctg::sound_paddle));
    }
    for (int missed = 0; missed < 2; ++missed) {
        va.reset(); a.serve_wait = 0;
        a.ball_x = missed == 0 ? -60 : 380;
        a.ball_y = 120; a.ball_vx = missed == 0 ? -4 : 4; a.ball_vy = 0;
        CHECK(va.step() == ctg::sound_point);
        CHECK(a.score[1 - missed] == 1 && a.serve_wait == 130);
        for (int i = 0; i < 130; ++i) va.step();
        CHECK(a.ball_x == (missed == 0 ? 286 : 35));
        a.score[1 - missed] = 14;
        a.ball_x = missed == 0 ? -60 : 380;
        a.ball_y = 120; a.ball_vx = missed == 0 ? -4 : 4; a.ball_vy = 0;
        CHECK((va.step() & (ctg::sound_point | ctg::sound_win)) == (ctg::sound_point | ctg::sound_win));
        CHECK(a.winner == 1 - missed);
        const auto final = a;
        CHECK(va.step({1, -1}) == ctg::sound_none && same(a, final));
        va.reset();
        ctg::TennisB fresh(true, false, true, false, false, true);
        CHECK(same(a, fresh.state()));
    }
    return 0;
}

} // namespace tennis_tests
