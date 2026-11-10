#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

namespace ctg {
// Hockey geometry is expressed in the same display pixels used for drawing.
constexpr int hockey_court_top = 21, hockey_court_bottom = 214;
constexpr int hockey_paddle_height = 33;
constexpr int hockey_inner_left = 119, hockey_inner_right = 194;
inline int hockey_y(int y) { return 21 + y * 188 / 240; }
inline int hockey_x(int x) { return 38 + x * 244 / 320; }
inline int hockey_paddle_top(int y, int height = hockey_paddle_height) { return hockey_y(y) - height / 2; }

// Draw at display coordinates, clipping to the framebuffer and changing color
// at the horizontal court boundaries.
inline void draw_hockey_ball(std::array<std::uint32_t, 320 * 240>& pixels, int x, int y) {
    for (int row = std::max(0, y - 2); row < std::min(240, y + 3); ++row) {
        for (int col = std::max(0, x - 3); col < std::min(320, x + 4); ++col) {
            pixels[row * 320 + col] = col >= 35 && col < 285 ? 0x98ff8d : 0xa098ac;
        }
    }
}
}
