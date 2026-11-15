#include "tennis_test_harness.h"
#include "tennis_b.h"

#include <initializer_list>

namespace tennis_tests {

int ping_pong_singles() {
    {
        ctg::TennisB ping(false, false, false, false, false, false, true);
        auto& s = const_cast<ctg::TennisState&>(ping.state());
        for (int direction : {-1, 1}) {
            ping.reset();
            s.serve_wait = 0; s.ball_x = direction > 0 ? 145 : 175;
            s.ball_y = 190; s.ball_vx = direction * 4; s.ball_vy = 0;
            unsigned events = 0;
            for (int i = 0; i < 8 && !(events & ctg::sound_wall); ++i) events |= ping.step();
            CHECK(events & ctg::sound_wall);
            CHECK(s.ball_vx == -direction * 4);
        }
        for (int vy : {-2, 2}) {
            ping.reset();
            s.serve_wait = 0; s.ball_x = 156; s.ball_y = 80; s.ball_vx = 4; s.ball_vy = vy;
            unsigned events = 0;
            for (int i = 0; i < 4 && !(events & ctg::sound_wall); ++i) events |= ping.step();
            CHECK(events & ctg::sound_wall);
            CHECK(s.ball_vx == 4 && s.ball_vy == 2);
        }
        ping.reset();
        CHECK(s.ping_pong && s.score[0] == 0 && s.winner == -1);
        s.serve_wait = 0; s.ball_x = 375; s.ball_y = 100; s.ball_vx = 4; s.ball_vy = 0;
        s.score[0] = 14;
        CHECK(ping.step() & ctg::sound_win);
        CHECK(s.winner == 0 && s.score[0] == 15);
        ping.reset(); CHECK(s.ping_pong && s.score[0] == 0 && s.winner == -1);
    }
    return 0;
}

int ping_pong_doubles() {
    {
        ctg::TennisB ping(true, false, false, false, false, false, true);
        auto& s = const_cast<ctg::TennisState&>(ping.state());
        CHECK(s.doubles && s.ping_pong && !s.hockey && !s.volleyball);
        ping.step({-1, 1});
        CHECK(s.paddle_y[0] == 116 && s.paddle_y[1] == 124);
        for (int player = 0; player < 2; ++player) for (int direction : {-1, 1}) {
            for (int overlap : {0, 1}) {
                ping.reset(); s.serve_wait = 0;
                s.paddle_y[player] = 80; s.paddle_y[1 - player] = 160;
                const int x = player == 0 ? ctg::TennisB::inner_left : ctg::TennisB::inner_right;
                s.ball_x = x - direction * 4; s.ball_y = overlap ? 80 : 160;
                s.ball_vx = direction * 4; s.ball_vy = 0;
                const bool hit = overlap && (player == 0 ? direction < 0 : direction > 0);
                CHECK(bool(ping.step() & ctg::sound_paddle) == hit);
                CHECK(s.ball_vx == (hit ? -direction : direction) * 4);
            }
        }
        for (int direction : {-1, 1}) {
            ping.reset(); s.serve_wait = 0;
            s.ball_x = direction > 0 ? 145 : 175; s.ball_y = 190;
            s.ball_vx = direction * 4; s.ball_vy = 0;
            unsigned events = 0;
            for (int i = 0; i < 8 && !(events & ctg::sound_wall); ++i) events |= ping.step();
            CHECK((events & ctg::sound_wall) && s.ball_vx == -direction * 4);
        }
        for (int vy : {-2, 2}) {
            ping.reset(); s.serve_wait = 0;
            s.ball_x = 156; s.ball_y = 80; s.ball_vx = 4; s.ball_vy = vy;
            unsigned events = 0;
            for (int i = 0; i < 4 && !(events & ctg::sound_wall); ++i) events |= ping.step();
            CHECK((events & ctg::sound_wall) && s.ball_vx == 4 && s.ball_vy == 2);
        }
        for (int missed = 0; missed < 2; ++missed) {
            ping.reset(); s.serve_wait = 0;
            s.ball_x = missed == 0 ? -60 : 380;
            s.ball_y = 120; s.ball_vx = missed == 0 ? -4 : 4; s.ball_vy = 0;
            CHECK(ping.step() == ctg::sound_point);
            CHECK(s.score[1 - missed] == 1 && s.serve_wait == 130);
            for (int i = 0; i < 130; ++i) ping.step();
            CHECK(s.ball_x == (missed == 0 ? 13 : 298));
            s.score[1 - missed] = 14;
            s.ball_x = missed == 0 ? -60 : 380;
            s.ball_y = 120; s.ball_vx = missed == 0 ? -4 : 4; s.ball_vy = 0;
            CHECK((ping.step() & (ctg::sound_point | ctg::sound_win)) == (ctg::sound_point | ctg::sound_win));
            CHECK(s.winner == 1 - missed);
            const auto final = s;
            CHECK(ping.step({1, -1}) == ctg::sound_none && same(s, final));
            ping.reset();
            ctg::TennisB fresh(true, false, false, false, false, false, true);
            CHECK(same(s, fresh.state()));
        }
    }
    return 0;
}

} // namespace tennis_tests
