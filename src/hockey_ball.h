#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

namespace ctg {
// Hockey geometry is expressed in the same display pixels used for drawing.
constexpr std::uint32_t hockey_puck_color = 0x98ff8d;
constexpr std::uint32_t hockey_exterior_puck_color = 0xa098ac;
constexpr int hockey_court_top = 21, hockey_court_bottom = 214;
constexpr int hockey_paddle_height = 33;
constexpr int hockey_outer_left = 49, hockey_outer_right = 265;
constexpr int hockey_inner_left = 121, hockey_inner_right = 193;
constexpr int hockey_paddle_width = 6;
inline int hockey_y(int y) { return 21 + y * 188 / 240; }
inline int hockey_x(int x) { return 38 + x * 244 / 320; }
// Match the other rally games: the physics face is one ball radius from
// the paddle center, rather than the first overlap of rendered pixels.
constexpr int hockey_paddle_face(int display_left, bool moving_right) {
    const int center = display_left + hockey_paddle_width / 2;
    const int physics_center = ((center - 38) * 320 + 122) / 244;
    return physics_center + (moving_right ? -2 : 2);
}
inline int hockey_paddle_top(int y, int height = hockey_paddle_height) { return hockey_y(y) - height / 2; }

// Draw at display coordinates, clipping to the framebuffer and changing color
// at the horizontal court boundaries.
inline void draw_hockey_ball(std::array<std::uint32_t, 320 * 240>& pixels, int x, int y) {
    for (int row = std::max(0, y - 2); row < std::min(240, y + 3); ++row) {
        for (int col = std::max(0, x - 3); col < std::min(320, x + 4); ++col) {
            pixels[row * 320 + col] = col >= 35 && col < 285 ? hockey_puck_color : hockey_exterior_puck_color;
        }
    }
}
}
