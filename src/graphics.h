#pragma once
#include <array>
#include <cstdint>

namespace ctg {
class Graphics {
public:
    static constexpr unsigned width = 320, height = 240;
    using Frame = std::array<std::uint32_t, width * height>;
    Frame pixels{};
    std::uint32_t rf_noise_frame = 0;

    static const char* glyph(char c);
    void text(unsigned x, unsigned y, const char* s, std::uint32_t color);
    void rect(int x, int y, int w, int h, std::uint32_t color);
    void darken_panel();
    void arrow(int y, bool up);
    void horizontal_arrow(int x, int y, bool left, std::uint32_t color);
    void score_digit(int x, int y, unsigned digit, std::uint32_t color, int cell_width);
    void centered_text(unsigned y, const char* value, std::uint32_t color);
    void apply_rf_filter(unsigned quality);
    void grayscale();
};
}
