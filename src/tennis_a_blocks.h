#pragma once

namespace ctg {
struct TennisABlock { int x, y, width, height; bool red; };

// The reference lays out its blocks in 1280x960 display coordinates.
constexpr TennisABlock tennis_a_block(int color, int row, int column) {
    return {(color == 0 ? 555 + 96 * column : 597 + 96 * column) / 4,
            (color == 0 ? 160 + 129 * row : 95 + 129 * row) / 4,
            8, 6, color == 0};
}
}
