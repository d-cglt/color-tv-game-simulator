#include "tennis_b.h"
#include "hockey_ball.h"
#include "tennis_a_blocks.h"
#include <algorithm>
#include <cstdio>
#include <limits>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

static bool same(const ctg::TennisState& a, const ctg::TennisState& b) {
    return a.ball_x == b.ball_x && a.ball_y == b.ball_y &&
           a.ball_vx == b.ball_vx && a.ball_vy == b.ball_vy &&
           a.paddle_y == b.paddle_y && a.doubles == b.doubles && a.hockey == b.hockey && a.volleyball == b.volleyball && a.volleyball_a == b.volleyball_a && a.tennis_a == b.tennis_a && a.ping_pong == b.ping_pong &&
           a.score == b.score && a.winner == b.winner && a.serve_wait == b.serve_wait;
}

int main() {
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
            CHECK(game.step() & ctg::sound_paddle);
            CHECK(s.ball_vx == -direction * 4);
            CHECK(s.ball_x == start_x + direction * 2);
            if ((color == 0 && direction < 0) || (color == 1 && direction > 0)) CHECK(s.ball_vy == 0);
            else CHECK(s.ball_vy == -4 || s.ball_vy == 0 || s.ball_vy == 4);
            if ((direction > 0 && col == 0) || (direction < 0 && col == 1))
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
            CHECK(contact(game, -directed, 3, n % 2 ? 2 : 0) & ctg::sound_paddle);
            CHECK(contact(twin, -directed, 3, n % 2 ? 2 : 0) & ctg::sound_paddle);
            CHECK(same(game.state(), twin.state()));
            const int vy = game.state().ball_vy;
            CHECK(n % 2 ? (vy == -2 || vy == 0) : (vy == -(high ? 8 : 4) || vy == 0 || vy == (high ? 8 : 4)));
        }
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
    // Volleyball B checks the rendered center before movement, on every update in the band.
    ctg::TennisB volleyball(false, false, true), volleyball_twin(false, false, true);
    auto& v = const_cast<ctg::TennisState&>(volleyball.state());
    auto& vt = const_cast<ctg::TennisState&>(volleyball_twin.state());
    CHECK(v.volleyball && !v.hockey && !v.doubles && v.serve_wait == 130);
    for (int i = 0; i < 130; ++i) CHECK(volleyball.step() == volleyball_twin.step());
    CHECK(v.serve_wait == 0 && v.ball_x == 13 && same(v, vt));
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
    CHECK(v.ball_x == 298 && v.ball_vx == -4);
    v.score[0] = 14; v.ball_x = 375; v.ball_y = 120; v.ball_vx = 4; v.ball_vy = 0;
    CHECK((volleyball.step() & (ctg::sound_point | ctg::sound_win)) == (ctg::sound_point | ctg::sound_win));
    CHECK(v.winner == 0);
    volleyball.reset();
    CHECK(v.volleyball && v.winner == -1 && v.score[0] == 0 && v.serve_wait == 130);
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
    CHECK(d.ball_x == 298 && d.ball_vx == -4);
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
        CHECK(a.ball_x == (missed == 0 ? 13 : 298));
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
