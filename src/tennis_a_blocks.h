#pragma once

namespace ctg {
struct TennisABlock { int x, y, width, height; bool red; };
constexpr int tennis_a_block_columns = 2;

constexpr TennisABlock tennis_a_block(int color, int row, int column) {
    return {37 + (17 + 2 * color + 4 * column) * 6,
            20 + ((color == 0 ? 6 : 2) + 8 * row) * 4,
            6, 4, color == 0};
}
}
