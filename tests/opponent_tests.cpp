#include "opponent.h"
#include <cstdio>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

struct Result { unsigned left, right, paddle_hits; };

static Result hockey_match(bool selected_easy, bool doubles, bool hockey_a, int strategy) {
    ctg::TennisB game(doubles, true, false, false, hockey_a);
    game.set_winning_score(10000);
    ctg::Opponent opponent;
    unsigned hits = 0;
    for (int frame = 0; frame < 120000; ++frame) {
        const auto& state = game.state();
        int left = 0;
        if (strategy == 1) left = (state.ball_y > state.paddle_y[0] + 2) - (state.ball_y < state.paddle_y[0] - 2);
        if (strategy == 2) left = ((frame / 75) % 2) ? 1 : -1;
        const int right = opponent.step(state, 1, selected_easy);
        if (game.step({left, right}) & ctg::sound_paddle) ++hits;
    }
    return {game.state().score[0], game.state().score[1], hits};
}

int main() {
    ctg::TennisState s;
    // Check every frame, including both sides of each boundary and several cycles.
    ctg::RallyDifficulty rally;
    for (unsigned frame = 0; frame <= 9000; ++frame) {
        const unsigned hard = frame < 1800 ? 3 : (frame - 1800) % 1800 < 900 ? 2 : 1;
        CHECK(rally.effective(0) == 0);
        CHECK(rally.effective(1) == 1);
        CHECK(rally.effective(2) == (frame < 900 ? 2 : 1));
        CHECK(rally.effective(3) == hard);
        rally.advance(s, ctg::sound_wall);
    }
    // Point, serve, winner and explicit reset each restart the shared clock.
    for (unsigned reset_kind = 0; reset_kind < 4; ++reset_kind) {
        rally.reset();
        for (unsigned i = 0; i < 2700; ++i) rally.advance(s, 0);
        CHECK(rally.effective(2) == 1 && rally.effective(3) == 1);
        if (reset_kind == 0) rally.advance(s, ctg::sound_point);
        if (reset_kind == 1) { s.serve_wait = 1; rally.advance(s, 0); s.serve_wait = 0; }
        if (reset_kind == 2) { s.winner = 0; rally.advance(s, ctg::sound_win); s.winner = -1; }
        if (reset_kind == 3) rally.reset();
        CHECK(rally.effective(2) == 2 && rally.effective(3) == 3);
        for (unsigned i = 0; i < 899; ++i) rally.advance(s, 0);
        CHECK(rally.effective(2) == 2 && rally.effective(3) == 3);
        rally.advance(s, 0);
        CHECK(rally.effective(2) == 1 && rally.effective(3) == 3);
    }
    // Fallback Easy in hockey uses the same tweak as explicitly selected Easy.
    for (unsigned level : {2u, 3u}) {
        ctg::RallyDifficulty clock;
        ctg::Opponent controlled, selected_easy, baseline;
        bool easy_tweak_differs = false;
        s.hockey = true; s.paddle_y = {120, 120}; s.ball_vx = 4; s.ball_vy = 1;
        for (unsigned i = 0; i < 6300; ++i) {
            s.ball_x = 100 + i % 180; s.ball_y = 20 + i % 200;
            const unsigned expected = level == 2 ? (i < 900 ? 2 : 1) :
                (i < 1800 ? 3 : (i - 1800) % 1800 < 900 ? 2 : 1);
            const auto before = s;
            const unsigned effective = clock.effective(level);
            const int actual = controlled.step(s, effective, effective == 1);
            CHECK(actual == selected_easy.step(s, expected, true));
            const int without_tweak = baseline.step(s, expected, false);
            if (expected == 1) easy_tweak_differs |= actual != without_tweak;
            CHECK(s.ball_x == before.ball_x && s.ball_y == before.ball_y &&
                  s.ball_vx == before.ball_vx && s.ball_vy == before.ball_vy);
            clock.advance(s, 0);
        }
        CHECK(easy_tweak_differs);
    }
    s.hockey = false;
    s.paddle_y = {120, 120};
    s.ball_x = 202; s.ball_y = 200; s.ball_vx = 4; s.ball_vy = 8;
    ctg::Opponent ai;
    // The direct extrapolation goes down, but a wall bounce puts the intercept above us.
    CHECK(ai.step(s, 3) == -1);
    s.ball_y = 50; s.ball_vy = 0; ai.reset();
    CHECK(ai.step(s, 3) == -1);
    s.ball_y = 190; ai.reset();
    CHECK(ai.step(s, 3) == 1);
    s.ball_vx = -4; s.paddle_y[1] = 80; ai.reset();
    CHECK(ai.step(s, 3) == 1);
    s.winner = 0;
    CHECK(ai.step(s, 3) == 0);
    s.winner = -1;
    s.serve_wait = 10; s.paddle_y[1] = 80; ai.reset();
    CHECK(ai.step(s, 3) == 1);

    // Doubles tennis intercepts the right inner paddle before the outer paddle.
    s.serve_wait = 0; s.paddle_y[1] = 120; s.doubles = true;
    s.ball_x = 200; s.ball_y = 80; s.ball_vx = 4; s.ball_vy = 4; ai.reset();
    CHECK(ai.step(s, 3) == -1);
    // Hockey player 1 also owns the left inner paddle, in both directions.
    s.hockey = true; s.ball_x = 100; s.ball_y = 180; s.ball_vx = 4; s.ball_vy = 0; ai.reset();
    CHECK(ai.step(s, 3) == 1);
    s.ball_x = 130; s.ball_vx = -4; ai.reset();
    CHECK(ai.step(s, 3) == 1);

    // Replay identical sequences, including resets, difficulty changes and clamping.
    for (unsigned level = 0; level <= 5; ++level) {
        ctg::Opponent a, b;
        for (int frame = 0; frame < 300; ++frame) {
            s.ball_x = 100 + frame % 180; s.ball_y = 20 + frame % 200;
            s.ball_vx = frame % 2 ? 4 : -4; s.ball_vy = frame % 7 - 3;
            const auto before = s;
            const int x = a.step(s, level), y = b.step(s, level == 0 ? 1 : level > 3 ? 3 : level);
            CHECK(x == y && x >= -1 && x <= 1);
            CHECK(s.ball_x == before.ball_x && s.ball_y == before.ball_y &&
                  s.ball_vx == before.ball_vx && s.ball_vy == before.ball_vy && s.paddle_y == before.paddle_y);
            if (frame == 150) { a.reset(); b.reset(); }
        }
    }
    // The hockey Easy tweak does not affect higher difficulties or non-hockey games.
    for (bool hockey : {false, true}) for (unsigned level : {1u, 2u, 3u}) {
        ctg::Opponent baseline, selected;
        bool differs = false;
        ctg::TennisState state;
        state.hockey = hockey; state.paddle_y = {120, 120};
        for (int frame = 0; frame < 600; ++frame) {
            state.ball_x = 100 + frame % 180; state.ball_y = 20 + frame % 200;
            state.ball_vx = frame % 2 ? 4 : -4; state.ball_vy = frame % 7 - 3;
            differs |= baseline.step(state, level) != selected.step(state, level, true);
        }
        CHECK(differs == (hockey && level == 1));
    }
    unsigned baseline_goals = 0, selected_goals = 0;
    for (bool doubles : {false, true}) for (bool hockey_a : {false, true})
        for (int strategy = 0; strategy < 3; ++strategy) {
            const auto baseline = hockey_match(false, doubles, hockey_a, strategy);
            const auto selected = hockey_match(true, doubles, hockey_a, strategy);
            std::printf("hockey doubles=%d A=%d strategy=%d baseline=%u:%u hits=%u selected=%u:%u hits=%u\n",
                        doubles, hockey_a, strategy, baseline.left, baseline.right, baseline.paddle_hits,
                        selected.left, selected.right, selected.paddle_hits);
            baseline_goals += baseline.left;
            selected_goals += selected.left;
        }
    CHECK(selected_goals > baseline_goals);

    // Exercise every game family, pairing, speed and difficulty through real frames.
    for (int variant = 0; variant < 7; ++variant) for (bool doubles : {false, true})
        for (unsigned level = 1; level <= 3; ++level) {
            ctg::TennisB game(doubles, variant == 2 || variant == 3,
                              variant == 4 || variant == 5, variant == 0,
                              variant == 2, variant == 4, variant == 6);
            game.set_options(level % 2 == 0, {false, level % 2 == 0});
            ctg::Opponent first, second;
            for (int frame = 0; frame < 1500; ++frame) {
                const int direction = first.step(game.state(), level);
                CHECK(direction == second.step(game.state(), level));
                CHECK(direction >= -1 && direction <= 1);
                game.step({0, direction});
            }
        }
    return 0;
}
