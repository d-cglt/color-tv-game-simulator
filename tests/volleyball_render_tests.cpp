#include "../src/core.cpp"
#include <cstdio>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

int main() {
    // Exercise each score glyph on a contrasting surface.
    CHECK(volleyball_score == 0xedfd84);
    for (bool right : {false, true}) for (unsigned value = 0; value < 10; ++value) {
        pixels.fill(0);
        rally_score_value(right, value, volleyball_score);
        const int col = right ? 34 : 5;
        for (int row = 0; row < 8; ++row) for (int cell = 0; cell < 4; ++cell)
            for (int dy = 0; dy < 4; ++dy) for (int dx = 0; dx < 6; ++dx) {
                const bool stroke = ctg::score_digits[value][row] & (8u >> cell);
                CHECK(pixels[(rally_y(6 + row) + dy) * width + rally_x(col + cell) + dx] ==
                      (stroke ? volleyball_score : 0u));
            }
    }
    for (bool doubles : {false, true}) for (bool variant : {false, true}) {
        ctg::TennisB game(doubles, false, true, false, false, variant);
        CHECK(game.state().serve_wait == 130);
        render_tennis(game);
        const auto serve_frame = pixels;
        for (int i = 0; i < 130; ++i) game.step();
        CHECK(game.state().serve_wait == 0);
        render_tennis(game);
        const int x = ctg::hockey_x(game.state().ball_x);
        const int y = ctg::hockey_y(game.state().ball_y);
        CHECK(pixels[y * width + x] == volleyball_ball);
        CHECK(serve_frame[y * width + x] != volleyball_ball);
    }
    return 0;
}
