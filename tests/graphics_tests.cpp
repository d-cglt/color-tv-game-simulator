#include "graphics.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace {
void check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(1);
    }
}
using Graphics = ctg::Graphics;
void primitives() {
    Graphics g;
    check(g.rf_noise_frame == 0, "initial RF state");
    check(std::all_of(g.pixels.begin(), g.pixels.end(), [](auto p) { return p == 0; }), "initial pixels");
    g.rect(-2, -3, 5, 5, 0x123456);
    for (unsigned y = 0; y < g.height; ++y)
        for (unsigned x = 0; x < g.width; ++x)
            check(g.pixels[y * g.width + x] == (x < 3 && y < 2 ? 0x123456u : 0u), "negative rect clipping");
    g.pixels.fill(0);
    g.rect(318, 239, 10, 10, 7);
    g.rect(12, 12, -1, 5, 9);
    g.rect(12, 12, 5, 0, 9);
    for (unsigned i = 0; i < g.pixels.size(); ++i)
        check(g.pixels[i] == (i >= 239 * g.width + 318 ? 7u : 0u), "rect edge/empty clipping");
    g.pixels.fill(0xffabcdef);
    g.darken_panel();
    for (unsigned y = 0; y < g.height; ++y)
        for (unsigned x = 0; x < g.width; ++x)
            check(g.pixels[y * g.width + x] == (x >= 48 && x < 272 && y >= 48 && y < 204 ? 0x556677u : 0xffabcdefu), "panel bounds and channels");
}
void arrows() {
    Graphics g;
    for (bool up : {false, true}) {
        g.pixels.fill(0);
        g.arrow(20, up);
        for (int y = 0; y < 240; ++y)
            for (int x = 0; x < 320; ++x) {
                const int r = y - 20;
                const int half = up ? r : 4 - r;
                const bool triangle = r >= 0 && r < 5 && x >= 160 - half && x <= 160 + half;
                const bool stem = x >= 159 && x < 162 && y >= (up ? 25 : 18) && y < (up ? 27 : 20);
                check(g.pixels[y * 320 + x] == (triangle || stem ? 0xffffffu : 0u), "vertical arrow");
            }
    }
    for (bool left : {false, true}) {
        for (int origin : {20, -2, 318}) {
            g.pixels.fill(0);
            g.horizontal_arrow(origin, 2, left, 0xabcdef);
            for (int y = 0; y < 240; ++y)
                for (int x = 0; x < 320; ++x) {
                    const int c = x - origin;
                    const int half = left ? c : 4 - c;
                    const bool triangle = c >= 0 && c < 5 && y >= 2 - half && y <= 2 + half;
                    const int stem_x = left ? origin + 5 : origin - 2;
                    const bool stem = x >= stem_x && x < stem_x + 2 && y >= 1 && y < 4;
                    check(g.pixels[y * 320 + x] == (triangle || stem ? 0xabcdefu : 0u), "horizontal arrow clipping");
                }
        }
    }
}
void scores() {
    const char* digits[] = {
        "f999999f", "11111111", "f11f888f", "f11f111f", "999f1111",
        "f88f111f", "f88f999f", "f9991111", "f99f999f", "f99f111f"
    };
    Graphics g;
    for (unsigned digit = 0; digit < 10; ++digit)
        for (int cell_width : {1, 4, 6}) {
            g.pixels.fill(0);
            g.score_digit(-1, 220, digit, 0x123456, cell_width);
            for (int y = 0; y < 240; ++y)
                for (int x = 0; x < 320; ++x) {
                    bool on = false;
                    if (y >= 220 && x + 1 < 4 * cell_width) {
                        int row = 0;
                        while ((row + 1) * 33 / 8 <= y - 220) ++row;
                        const char value = digits[digit][row];
                        const unsigned bits = value == 'f' ? 15 : value - '0';
                        on = bits & (8u >> ((x + 1) / cell_width));
                    }
                    check(g.pixels[y * 320 + x] == (on ? 0x123456u : 0u), "score pixels and clipping");
                }
        }
}
void grayscale() {
    Graphics g;
    const std::uint32_t input[] = {0x12345678, 0xffff0000, 0x8000ff00, 0x420000ff, 0xffffffff, 0};
    const std::uint32_t expected[] = {0x12505050, 0xff4c4c4c, 0x80969696, 0x421d1d1d, 0xffffffff, 0};
    for (unsigned i = 0; i < g.pixels.size(); ++i) g.pixels[i] = input[i % 6];
    g.rf_noise_frame = 17;
    g.grayscale();
    for (unsigned i = 0; i < g.pixels.size(); ++i) check(g.pixels[i] == expected[i % 6], "grayscale rounding and alpha");
    check(g.rf_noise_frame == 17, "grayscale leaves RF state alone");
    const auto frame = g.pixels;
    g.grayscale();
    check(frame == g.pixels, "grayscale idempotence");
}
}

namespace {
const char* original_glyph(char c) {
    switch(c) {
    case 'A': return "01110100011000111111100011000110001";
    case 'B': return "11110100011000111110100011000111110";
    case 'C': return "01111100001000010000100001000001111";
    case 'D': return "11110100011000110001100011000111110";
    case 'E': return "11111100001000011110100001000011111";
    case 'F': return "11111100001000011110100001000010000";
    case 'G': return "01111100001000010111100011000101111";
    case 'H': return "10001100011000111111100011000110001";
    case 'K': return "10001100101010011000101001001010001";
    case 'I': return "11111001000010000100001000010011111";
    case 'L': return "10000100001000010000100001000011111";
    case 'M': return "10001110111010110101100011000110001";
    case 'N': return "10001110011010110011100011000110001";
    case 'O': return "01110100011000110001100011000101110";
    case 'P': return "11110100011000111110100001000010000";
    case 'R': return "11110100011000111110101001001010001";
    case 'S': return "01111100001000001110000010000111110";
    case 'T': return "11111001000010000100001000010000100";
    case 'U': return "10001100011000110001100011000101110";
    case 'W': return "10001100011000110101101011101110001";
    case 'V': return "10001100011000110001100010101000100";
    case 'X': return "10001100010101000100010101000110001";
    case 'Y': return "10001100010101000100001000010000100";
    case 'Z': return "11111000010001000100010001000011111";
    case '0': return "01110100011001110101110011000101110";
    case '1': return "00100011000010000100001000010001110";
    case '2': return "01110100010000100010001000100011111";
    case '3': return "11110000010000101110000010000111110";
    case '4': return "00010001100101010010111110001000010";
    case '5': return "11111100001000011110000010000111110";
    case '6': return "01110100001000011110100011000101110";
    case '7': return "11111000010001000100010000100001000";
    case '8': return "01110100011000101110100011000101110";
    case '9': return "01110100011000101111000010000101110";

    case '<': return "00001000100010001000001000001000001";
    case '>': return "10000010000001000001000100100010000";
    case '-': return "00000000000000011111000000000000000";
    case '(': return "00010001000100001000010000010000010";
    case ')': return "01000001000001000010000100010001000";
    default: return "00000000000000000000000000000000000";
    }
}
struct OriginalRF {
    static constexpr unsigned width = 320, height = 240;
    Graphics::Frame pixels{};
    std::uint32_t rf_noise_frame = 0;
void apply_rf_filter(unsigned quality) {
    const float degradation = (100 - quality) * 0.01f;
    std::uint32_t noise = 0x9e3779b9u ^ (++rf_noise_frame * 0x85ebca6bu);
    struct Signal { float y, i, q; };
    std::array<Signal, width> line{};
    for (unsigned row = 0; row < height; ++row) {
        const unsigned base = row * width;
        for (unsigned x = 0; x < width; ++x) {
            const auto pixel = pixels[base + x];
            const float r = static_cast<float>((pixel >> 16) & 255);
            const float g = static_cast<float>((pixel >> 8) & 255);
            const float b = static_cast<float>(pixel & 255);
            line[x] = {0.299f*r + 0.587f*g + 0.114f*b,
                       0.596f*r - 0.274f*g - 0.322f*b,
                       0.211f*r - 0.523f*g + 0.312f*b};
        }
        for (unsigned x = 0; x < width; ++x) {
            const auto& a = line[x > 0 ? x - 1 : 0];
            const auto& b = line[x];
            const auto& c = line[std::min(x + 1, width - 1)];
            const auto& d = line[x > 1 ? x - 2 : 0];
            const auto& e = line[std::min(x + 2, width - 1)];
            float y = (a.y + 2.0f*b.y + c.y) * 0.25f;
            const float detail = (b.y - (a.y + c.y) * 0.5f) * 0.12f;
            const unsigned phase = (x + 2 * (row & 1)) & 3;
            float i = (d.i + 2.0f*a.i + 2.0f*b.i + 2.0f*c.i + e.i) * 0.125f +
                            (phase == 0 ? detail : phase == 2 ? -detail : 0.0f);
            float q = (d.q + 2.0f*a.q + 2.0f*b.q + 2.0f*c.q + e.q) * 0.125f +
                            (phase == 1 ? detail : phase == 3 ? -detail : 0.0f);
            if (quality < 100) {
                const auto& left = line[x > 3 ? x - 4 : 0];
                const auto& right = line[std::min(x + 4, width - 1)];
                const float bleed = degradation * 0.45f;
                i += ((left.i + right.i) * 0.5f - i) * bleed;
                q += ((left.q + right.q) * 0.5f - q) * bleed;
                // Private per-frame visual noise, never shared with gameplay RNG.
                noise ^= noise << 13; noise ^= noise >> 17; noise ^= noise << 5;
                y += (static_cast<int>(noise & 255u) - 127.5f) * (degradation * 0.10f);
            }
            const auto channel = [](float value) {
                return static_cast<std::uint32_t>(std::clamp(value, 0.0f, 255.0f) + 0.5f);
            };
            pixels[base + x] = (pixels[base + x] & 0xff000000u) |
                               (channel(y + 0.956f*i + 0.621f*q) << 16) |
                               (channel(y - 0.272f*i - 0.647f*q) << 8) |
                               channel(y - 1.106f*i + 1.703f*q);
        }
    }
}
};
void glyphs() {
    Graphics g;
    for (unsigned c = 0; c < 256; ++c) {
        const char character = static_cast<char>(c);
        check(std::strcmp(Graphics::glyph(character), original_glyph(character)) == 0, "literal glyph table");
        if (!c) continue;
        const char value[] = {character, 0};
        for (unsigned origin : {0u, 318u}) {
            g.pixels.fill(0);
            g.text(origin, 237, value, 0xabcdef);
            for (unsigned y = 0; y < 240; ++y)
                for (unsigned x = 0; x < 320; ++x) {
                    const bool on = y >= 237 && x >= origin && x - origin < 5 &&
                                    original_glyph(character)[(y - 237) * 5 + x - origin] == '1';
                    check(g.pixels[y * 320 + x] == (on ? 0xabcdefu : 0u), "glyph pixels and clipping");
                }
        }
    }
    g.pixels.fill(0);
    g.text(10, 10, "A A", 7);
    for (unsigned y = 0; y < 240; ++y)
        for (unsigned x = 0; x < 320; ++x) {
            bool on = false;
            for (unsigned origin : {10u, 22u})
                if (y >= 10 && y < 17 && x >= origin && x < origin + 5)
                    on = original_glyph('A')[(y - 10) * 5 + x - origin] == '1';
            check(g.pixels[y * 320 + x] == (on ? 7u : 0u), "text advance and spaces");
        }
    g.pixels.fill(0);
    Graphics expected;
    expected.text(151, 20, "ABC", 9);
    g.centered_text(20, "ABC", 9);
    check(g.pixels == expected.pixels, "centered text positioning");
    g.text(400, 300, "A", 1);
    g.centered_text(240, "", 1);
    check(g.pixels == expected.pixels, "offscreen and empty text");
}
void rf() {
    auto graphics = std::make_unique<Graphics>();
    auto baseline = std::make_unique<OriginalRF>();
    auto input_buffer = std::make_unique<Graphics::Frame>();
    auto first_buffer = std::make_unique<Graphics::Frame>();
    auto& g = *graphics;
    auto& original = *baseline;
    auto& input = *input_buffer;
    auto& first = *first_buffer;
    for (unsigned i = 0; i < input.size(); ++i)
        input[i] = (i * 2654435761u) ^ (i << 17);
    for (unsigned quality : {100u, 90u, 50u, 0u}) {
        g.rf_noise_frame = original.rf_noise_frame = 0;
        first.fill(0);
        for (unsigned frame = 1; frame <= 3; ++frame) {
            g.pixels = original.pixels = input;
            g.apply_rf_filter(quality);
            original.apply_rf_filter(quality);
            check(g.pixels == original.pixels, "RF exact original pixels");
            check(g.rf_noise_frame == frame, "RF frame advancement");
            for (unsigned i = 0; i < input.size(); ++i)
                check((g.pixels[i] & 0xff000000u) == (input[i] & 0xff000000u), "RF preserves alpha");
            if (frame == 1) first = g.pixels;
            else check((g.pixels == first) == (quality == 100), "RF noise evolves only at degraded quality");
        }
        g.rf_noise_frame = 0;
        g.pixels = input;
        g.apply_rf_filter(quality);
        check(g.pixels == first, "RF reset reproduces frame");
    }
    g.rf_noise_frame = original.rf_noise_frame = 0xffffffffu;
    g.pixels = original.pixels = input;
    g.apply_rf_filter(0);
    original.apply_rf_filter(0);
    check(g.rf_noise_frame == 0 && g.pixels == original.pixels, "RF state wraps deterministically");
}
}
int main() {
    primitives();
    glyphs();
    arrows();
    scores();
    grayscale();
    rf();
    std::puts("graphics tests passed");
}
