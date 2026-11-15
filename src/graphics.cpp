#include "graphics.h"
#include "score_digits.h"
#include <algorithm>
#include <cstring>

namespace ctg {
const char* Graphics::glyph(char c) {
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

void Graphics::text(unsigned x, unsigned y, const char* s, std::uint32_t color) {
    for (; *s; ++s, x += 6) {
        const char* g = glyph(*s);
        for (unsigned row = 0; row < 7; ++row)
            for (unsigned col = 0; col < 5; ++col)
                if (g[row * 5 + col] == '1' && x + col < width && y + row < height)
                    pixels[(y + row) * width + x + col] = color;
    }
}

void Graphics::rect(int x, int y, int w, int h, std::uint32_t color) {
    for (int row = std::max(0, y); row < std::min<int>(height, y + h); ++row)
        for (int col = std::max(0, x); col < std::min<int>(width, x + w); ++col)
            pixels[row * width + col] = color;
}

void Graphics::darken_panel() {
    for (unsigned y = 48; y < 204; ++y)
        for (unsigned x = 48; x < 272; ++x) {
            auto& pixel = pixels[y * width + x];
            pixel = (pixel & 0xfefefe) >> 1;
        }
}

void Graphics::arrow(int y, bool up) {
    for (int row = 0; row < 5; ++row) {
        int half = up ? row : 4 - row;
        rect(160 - half, y + row, half * 2 + 1, 1, 0xffffff);
    }
    rect(159, up ? y + 5 : y - 2, 3, 2, 0xffffff);
}

void Graphics::horizontal_arrow(int x, int y, bool left, std::uint32_t color) {
    for (int col = 0; col < 5; ++col) {
        int half = left ? col : 4 - col;
        rect(x + col, y - half, 1, half * 2 + 1, color);
    }
    rect(left ? x + 5 : x - 2, y - 1, 2, 3, color);
}

void Graphics::score_digit(int x, int y, unsigned digit, std::uint32_t color, int cell_width) {
    for (int row = 0; row < 8; ++row) {
        const int top = row * 33 / 8;
        const int bottom = (row + 1) * 33 / 8;
        for (int col = 0; col < 4; ++col)
            if (ctg::score_digits[digit][row] & (8u >> col))
                rect(x + col * cell_width, y + top, cell_width, bottom - top, color);
    }
}

void Graphics::centered_text(unsigned y, const char* value, std::uint32_t color) {
    text((width - static_cast<unsigned>(std::strlen(value)) * 6) / 2, y, value, color);
}

void Graphics::apply_rf_filter(unsigned quality) {
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

void Graphics::grayscale() {
    for (auto& pixel : pixels) {
        const unsigned luma = (299 * ((pixel >> 16) & 0xff) + 587 * ((pixel >> 8) & 0xff) +
                               114 * (pixel & 0xff) + 500) / 1000;
        pixel = (pixel & 0xff000000u) | (luma << 16) | (luma << 8) | luma;
    }
}
}
