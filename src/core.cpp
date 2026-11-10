#include "content.h"
#include "generator.h"
#include "hockey_ball.h"
#include "libretro.h"
#include "tennis_b.h"
#include "opponent.h"
#include "shooting.h"
#include "tennis_a_blocks.h"
#include <algorithm>
#include <cstdio>
#include <array>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <cerrno>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif


namespace {
constexpr unsigned width = 320, height = 240;
retro_environment_t environment = nullptr;
bool option_known = false, generate_selected = false;

retro_video_refresh_t video = nullptr;
retro_audio_sample_t audio = nullptr;
retro_audio_sample_batch_t audio_batch = nullptr;
retro_input_poll_t poll = nullptr;
retro_input_state_t input = nullptr;
std::array<std::uint32_t, width * height> pixels{};
std::uint32_t rf_noise_frame = 0;
// A scanline-local YIQ approximation: luma retains more bandwidth than chroma,
// and a small carrier-phase-dependent luma detail leaks into chroma.
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
constexpr std::array<unsigned, ctg::slot_count> selector_order{{
    2, 3, 6, 7, 10, 11, 0, 1, 4, 5, 8, 9, 12, 13, 14
}};
unsigned selector_position(unsigned id) {
    return static_cast<unsigned>(std::find(selector_order.begin(), selector_order.end(), id) - selector_order.begin());
}
struct Title { const char* name; const char* variant; };
constexpr std::array<Title, ctg::slot_count> titles{{
    {"TENNIS VARIANT", "SINGLES"}, {"TENNIS VARIANT", "DOUBLES"},
    {"TENNIS ORIGINAL", "SINGLES"}, {"TENNIS ORIGINAL", "DOUBLES"},
    {"HOCKEY VARIANT", "SINGLES"}, {"HOCKEY VARIANT", "DOUBLES"},
    {"HOCKEY ORIGINAL", "SINGLES"}, {"HOCKEY ORIGINAL", "DOUBLES"},
    {"VOLLEYBALL VARIANT", "SINGLES"}, {"VOLLEYBALL VARIANT", "DOUBLES"},
    {"VOLLEYBALL ORIGINAL", "SINGLES"}, {"VOLLEYBALL ORIGINAL", "DOUBLES"},
    {"PING PONG", "SINGLES"}, {"PING PONG", "DOUBLES"},
    {"SHOOTING GAME", ""}
}};
constexpr unsigned title_length(const char* s) {
    unsigned length = 0;
    while (*s++) ++length;
    return length;
}
constexpr bool titles_fit_selector() {
    for (const auto& title : titles) {
        const unsigned length = title_length(title.name) + title_length(title.variant) + (title.variant[0] ? 1 : 0);
        if (length * 6 > 210) return false;
    }
    return true;
}
static_assert(titles_fit_selector(), "Game titles must fit the selector highlight");
unsigned content_id = 0, previous = 0;
bool loaded = false, detail = false, automatic_target = false;
bool selector = false, paused = false;
unsigned selector_choice = 0, menu_choice = 1, pause_choice = 0, result_choice = 0;
unsigned rotation_frames = 0;
int repeat_menu = -1, repeat_direction = 0;
unsigned repeat_frames = 0;
constexpr unsigned repeat_delay = 24, repeat_interval = 6;
bool playable() { return content_id < ctg::slot_count; }
std::array<bool, 2> joypad{{true, true}};
ctg::TennisB tennis;
ctg::Opponent opponent;
ctg::RallyDifficulty rally_difficulty;
std::array<unsigned, 14> opponent_modes{};
ctg::Shooting shooting;
ctg::TennisB demo_tennis;
bool paddle_movement_rebound = false;
bool anti_stalemate = true;
ctg::Shooting demo_shooting;
std::array<ctg::Opponent, 2> demo_opponents;
unsigned demo_id = ctg::slot_count, inactivity_frames = 0, demo_frame = 0;
bool menu_hidden = false, suppress_wake_input = false;
bool demo_sounds = true;
struct DemoTone { unsigned remaining = 0, phase = 0, period = 0; } demo_tone;
void reset_demo() {
    demo_tone = {};
    demo_id = ctg::slot_count;
    inactivity_frames = demo_frame = rotation_frames = 0;
    menu_hidden = suppress_wake_input = false;
}
void step_demo(unsigned id) {
    if (demo_id != id) {
        demo_id = id;
        demo_frame = 0;
        demo_tone = {};
        demo_tennis = ctg::TennisB(id % 2 == 1 && id < 14,
                                   id >= 4 && id <= 7, id >= 8 && id <= 11,
                                   id <= 1, id == 4 || id == 5,
                                   id == 8 || id == 9, id == 12 || id == 13);
        demo_tennis.set_rebound_mode(paddle_movement_rebound);
        demo_tennis.set_anti_stalemate(anti_stalemate);
        demo_shooting = ctg::Shooting{};
        for (auto& ai : demo_opponents) ai.reset();
    }
    unsigned events = 0;
    if (id == 14) {
        const auto& state = demo_shooting.state();
        if (state.finished) demo_shooting.reset();
        const int delta = demo_shooting.state().target_y + 16 - demo_shooting.state().shooter_y;
        events = demo_shooting.step((delta > 3) - (delta < -3), demo_frame % 45 == 0, 0, true);
    } else {
        auto mirrored = demo_tennis.state();
        mirrored.ball_x = ctg::TennisB::width - mirrored.ball_x;
        mirrored.ball_vx = -mirrored.ball_vx;
        std::swap(mirrored.paddle_y[0], mirrored.paddle_y[1]);
        std::swap(mirrored.small_paddle[0], mirrored.small_paddle[1]);
        const unsigned difficulty = (demo_frame / 180) % 2 == 0 ? 3 : 1;
        const int left = demo_opponents[0].step(mirrored, difficulty);
        const int right = demo_opponents[1].step(demo_tennis.state(), difficulty);
        events = demo_tennis.step({left, right});
        if (events & ctg::sound_point) {
            demo_tennis.reset();
            for (auto& ai : demo_opponents) ai.reset();
        }
    }
    if (demo_sounds && events) {
        demo_tone.period = (events & ctg::sound_win) ? 80 : (events & ctg::sound_point) ? 120 :
                           (id == 14 || (events & ctg::sound_paddle)) ? 60 : 100;
        demo_tone.remaining = (events & ctg::sound_win) ? 7200 : 2400;
        demo_tone.phase = 0;
    }
    ++demo_frame;
}
bool high_speed = false;
std::array<bool, 2> small_paddle{};
unsigned winning_score = ctg::TennisB::winning_score;
unsigned shot_limit = ctg::Shooting::default_shot_limit;
void apply_options() { tennis.set_anti_stalemate(anti_stalemate); tennis.set_rebound_mode(paddle_movement_rebound); tennis.set_options(high_speed, small_paddle); tennis.set_winning_score(winning_score); shooting.set_options(high_speed, small_paddle); shooting.set_shot_limit(shot_limit); }
unsigned option_count() { return 7; }
struct LaunchLayout {
    unsigned first, rows, y, spacing;
    bool scroll;
};
constexpr LaunchLayout launch_layout(unsigned count, unsigned choice) {
    return count <= 7 ? LaunchLayout{0, count, count == 7 ? 88u : 98u, count == 7 ? 15u : 16u, false}
                      : LaunchLayout{choice >= 5 ? choice - 4 : 0, 5, 98, 16, true};
}
static_assert(launch_layout(7, 6).first == 0 && launch_layout(7, 6).rows == 7 &&
              !launch_layout(7, 6).scroll && launch_layout(7, 6).y > 76 &&
              launch_layout(7, 6).y + 6 * launch_layout(7, 6).spacing + 14 <= 204);
static_assert(launch_layout(8, 7).scroll && launch_layout(8, 7).first == 3 &&
              launch_layout(8, 7).first + launch_layout(8, 7).rows == 8);
bool result() { return detail && (content_id == 14 ? shooting.state().finished : playable() && tennis.state().winner != -1); }
unsigned tone_remaining = 0, tone_phase = 0, tone_period = 0;

// Five-bit rows for the small uppercase status font.
const char* glyph(char c) {
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
    default: return "00000000000000000000000000000000000";
    }
}
void text(unsigned x, unsigned y, const char* s, std::uint32_t color) {
    for (; *s; ++s, x += 6) {
        const char* g = glyph(*s);
        for (unsigned row = 0; row < 7; ++row)
            for (unsigned col = 0; col < 5; ++col)
                if (g[row * 5 + col] == '1' && x + col < width && y + row < height)
                    pixels[(y + row) * width + x + col] = color;
    }
}
void rect(int x, int y, int w, int h, std::uint32_t color) {
    for (int row = std::max(0, y); row < std::min<int>(height, y + h); ++row)
        for (int col = std::max(0, x); col < std::min<int>(width, x + w); ++col)
            pixels[row * width + col] = color;
}
void darken_panel() {
    for (unsigned y = 48; y < 204; ++y)
        for (unsigned x = 48; x < 272; ++x) {
            auto& pixel = pixels[y * width + x];
            pixel = (pixel & 0xfefefe) >> 1;
        }
}
void arrow(int y, bool up) {
    for (int row = 0; row < 5; ++row) {
        int half = up ? row : 4 - row;
        rect(160 - half, y + row, half * 2 + 1, 1, 0xffffff);
    }
    rect(159, up ? y + 5 : y - 2, 3, 2, 0xffffff);
}
void horizontal_arrow(int x, int y, bool left, std::uint32_t color) {
    for (int col = 0; col < 5; ++col) {
        int half = left ? col : 4 - col;
        rect(x + col, y - half, 1, half * 2 + 1, color);
    }
    rect(left ? x + 5 : x - 2, y - 1, 2, 3, color);
}
void score_digit(int x, int y, unsigned digit, std::uint32_t color) {
    static constexpr unsigned segments[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};
    const unsigned s = segments[digit];
    if (s & 0x01) rect(x + 3, y, 12, 3, color);
    if (s & 0x02) rect(x + 15, y + 3, 3, 12, color);
    if (s & 0x04) rect(x + 15, y + 18, 3, 12, color);
    if (s & 0x08) rect(x + 3, y + 30, 12, 3, color);
    if (s & 0x10) rect(x, y + 18, 3, 12, color);
    if (s & 0x20) rect(x, y + 3, 3, 12, color);
    if (s & 0x40) rect(x + 3, y + 15, 12, 3, color);
}
void score_value(int center, unsigned value, std::uint32_t color) {
    if (value >= 10) score_digit(center - 20, 42, value / 10, color);
    score_digit(center + (value >= 10 ? 2 : -9), 42, value % 10, color);
}
void render_tennis(const ctg::TennisB& tennis, bool show_score = true) {
    const auto& state = tennis.state();
    pixels.fill(state.ping_pong ? 0x000000 : state.hockey ? 0x505068 : state.volleyball ? 0x19b537 : 0x0e7aab);
    rect(35, 16, 250, 203, state.ping_pong ? 0x000000 : state.hockey ? 0x2cc94c : state.volleyball ? 0x6d70f9 : 0xc07645);
    if (state.ping_pong) {
        rect(35, 214, 250, 5, 0xffffff);
        rect(156, 155, 8, 16, 0xffffff);
        rect(156, 187, 8, 16, 0xffffff);
    } else if (state.hockey) {
        const int top = 21 + ctg::TennisB::hockey_goal_top * 188 / ctg::TennisB::height;
        const int bottom = 21 + ctg::TennisB::hockey_goal_bottom * 188 / ctg::TennisB::height;
        for (int x = 35; x < 285; ++x) {
            const int progress = std::min(x - 35, 25);
            const std::uint32_t wall = ((153 + 24 * progress / 25) << 16) | (255 << 8) | (142 + 49 * progress / 25);
            rect(x, 16, 1, 5, wall);
            rect(x, 214, 1, 5, wall);
            if (x < 43 || x >= 277) {
                rect(x, 21, 1, top - 21, wall);
                rect(x, bottom, 1, 214 - bottom, wall);
            }
        }
        rect(35, top, 2, bottom - top, 0x3a687f);
        rect(283, top, 2, bottom - top, 0x51bc29);
        if (state.hockey_a) {
            for (int i = 0; i < 6; ++i) {
                if (state.hockey_barrier_closed || (i != 2 && i != 3))
                    rect(156, 25 + i * 32, 8, 16, 0x98ff8d);
            }
        }
    } else if (state.volleyball) {
        rect(35, 16, 250, 5, 0xcddcfb);
        rect(35, 214, 250, 5, 0xcddcfb);
        rect(35, 21, 2, 193, 0x84686a);
        rect(283, 21, 2, 193, 0x408caf);
        if (state.volleyball_a) {
            for (int color = 0; color < 2; ++color)
                for (int row = 0; row < 6; ++row)
                    for (int col = 0; col < 2; ++col) {
                        const auto block = ctg::tennis_a_block(color, row, col);
                        rect(block.x, block.y, block.width, block.height, block.red ? 0xc84458 : 0x53c178);
                    }
        } else {
            for (int y = 25; y <= 187; y += 32) rect(156, y, 8, 16, 0xc7dbfd);
        }
    } else {
        rect(35, 16, 250, 5, 0xdad2b0);
        rect(35, 214, 250, 5, 0xdad2b0);
        rect(35, 21, 2, 193, 0x4cbd7c);
        rect(283, 21, 2, 193, 0xa36878);
        if (state.tennis_a) {
            for (int color = 0; color < 2; ++color)
                for (int row = 0; row < 6; ++row)
                    for (int col = 0; col < 2; ++col) {
                        const auto block = ctg::tennis_a_block(color, row, col);
                        rect(block.x, block.y, block.width, block.height, block.red ? 0xc84458 : 0x53c178);
                    }
        } else {
            for (int y = 25; y <= 187; y += 32) rect(156, y, 8, 16, 0xfae7b4);
        }
    }
    rect(35, 16, 250, 1, 0x000000);
    rect(35, 218, 250, 1, 0x000000);
    if (show_score && (state.serve_wait || state.winner != -1)) {
        const std::uint32_t score_color = state.ping_pong ? 0x8affa1 : state.hockey ? 0xaefbff : state.volleyball ? 0xedfd84 : 0xcdf0ff;
        score_value(80, state.score[0], score_color);
        score_value(240, state.score[1], score_color);
    }
    const auto paddle_top = [&](int owner) { return tennis.paddle_top(owner); };
    for (int i = 0; i < 2; ++i) {
        int top = state.hockey ? ctg::hockey_paddle_top(state.paddle_y[i], tennis.paddle_pixels(i)) : paddle_top(i);
        int bottom = top + tennis.paddle_pixels(i);
        if (state.hockey) {
            top = std::max(top, ctg::hockey_court_top);
            bottom = std::min(bottom, ctg::hockey_court_bottom);
        }
        rect(i == 0 ? 47 : 265, top, 8, bottom - top,
             state.ping_pong ? 0x9b8cff : state.hockey ? (i == 0 ? 0xff9fff : 0xd5e87c) : state.volleyball ? 0xa6e6ff : 0xd0ecb2);
    }
    if (state.doubles) {
        for (int i = 0; i < 2; ++i) {
            if (state.hockey) {
                const int owner = 1 - i;
                const int top = std::max(ctg::hockey_court_top, ctg::hockey_paddle_top(state.paddle_y[owner], tennis.paddle_pixels(owner)));
                const int bottom = std::min(ctg::hockey_court_bottom, ctg::hockey_paddle_top(state.paddle_y[owner], tennis.paddle_pixels(owner)) + tennis.paddle_pixels(owner));
                rect(i == 0 ? ctg::hockey_inner_left : ctg::hockey_inner_right, top, 8, bottom - top,
                     owner == 0 ? 0xff9fff : 0xd5e87c);
            } else {
                const int x = i == 0 ? ctg::TennisB::inner_left : ctg::TennisB::inner_right;
                rect(38 + x * 244 / ctg::TennisB::width - 4, paddle_top(i), 8, tennis.paddle_pixels(i),
                                     state.ping_pong ? 0x9b8cff : state.volleyball ? 0xa6e6ff : 0xd0ecb2);
            }
        }
    }
    const int ball_x = ctg::hockey_x(state.ball_x);
    const int ball_y = ctg::hockey_y(state.ball_y);
    if (!state.serve_wait) {
        if (state.hockey) {
            ctg::draw_hockey_ball(pixels, ball_x, ball_y);
        } else {
            rect(ball_x - 3, ball_y - 2, 7, 5, state.ping_pong ? 0xffffff : state.volleyball ? 0xc7dbfd : 0xfae7b4);
        }
    }

}
void render_shooting(const ctg::Shooting& shooting, bool show_score = true) {
    const auto& s = shooting.state();
    const std::uint32_t background = s.hit ? 0xcb4042 : 0x46a79a;
    const std::uint32_t objects = s.hit ? 0xffd5ff : 0xf3d66b;
    pixels.fill(background);
    rect(35, 16, 250, 5, 0x86ffff);
    rect(35, 214, 250, 5, 0x86ffff);
    if (show_score) {
        score_value(80, s.shots, 0xffc9fa);
        score_value(240, s.hits, 0xffc9fa);
    }
    rect(47, shooting.shooter_top(), 8, shooting.paddle_height(0), objects);
    if (shooting.target_visible()) rect(265, shooting.target_top(), 8, shooting.paddle_height(1), objects);
    if (s.flying) rect(s.shot_x, s.shot_y - 2, 7, 5, s.hit ? 0xffd5ff : 0x8ddade);

}
void centered_text(unsigned y, const char* value, std::uint32_t color) {
    text((width - static_cast<unsigned>(std::strlen(value)) * 6) / 2, y, value, color);
}
constexpr unsigned selector_rows = 6;
unsigned selector_first() {
    return std::min(selector_choice > 2 ? selector_choice - 2 : 0,
                    static_cast<unsigned>(ctg::slot_count - selector_rows));
}
void monochrome_selection_outline() {
    int x, y, w, h;
    if (selector) {
        const unsigned first = selector_first();
        x = 55; y = 88 + (selector_choice - first) * 16; w = 210; h = 13;
    } else if (paused || result()) {
        x = 75; y = 99 + (result() ? result_choice : pause_choice) * 16; w = 170; h = 14;
    } else {
        const auto layout = launch_layout(option_count(), menu_choice);
        x = 53; w = 214;
        y = layout.y + (menu_choice - layout.first) * layout.spacing; h = 14;
    }
    rect(x, y, w, 1, 0xffffff);
    rect(x, y + h - 1, w, 1, 0xffffff);
    rect(x, y, 1, h, 0xffffff);
    rect(x + w - 1, y, 1, h, 0xffffff);
}
void render() {
    const bool preview = selector || !detail;
    if (content_id == 14) render_shooting(preview ? demo_shooting : shooting, !preview);
    else render_tennis(preview ? demo_tennis : tennis, !preview);
    if (preview && menu_hidden) {
        centered_text(4, "DEMO MODE - PRESS ANY BUTTON TO SHOW MENU", 0xffffff);
        char label[64];
        std::snprintf(label, sizeof label, "%s%s%s", titles[content_id].name,
                      titles[content_id].variant[0] ? " " : "", titles[content_id].variant);
        centered_text(226, label, 0xffffff);
        return;
    }
    darken_panel();
    if (selector) {
        centered_text(58, "SELECT GAME", 0xffffff);
        const unsigned first = selector_first();
        if (first) arrow(77, true);
        for (unsigned i = 0; i < selector_rows; ++i) {
            unsigned id = selector_order[first + i];
            char label[64];
            std::snprintf(label, sizeof label, "%s%s%s", titles[id].name,
                          titles[id].variant[0] ? " " : "", titles[id].variant);
            if (first + i == selector_choice) rect(55, 88 + i * 16, 210, 13, 0x295166);
            centered_text(91 + i * 16, label, first + i == selector_choice ? 0x70e0ff : 0xffffff);
        }
        if (first + selector_rows < ctg::slot_count) arrow(187, false);
    } else if (paused || result()) {
        const bool finished = result();
        centered_text(70, finished ? (content_id == 14 ? "GAME OVER" : tennis.state().winner == 0 ? "PLAYER 1 WINS" : "PLAYER 2 WINS") : "PAUSED", 0xffffff);
        if (finished && content_id == 14) {
            centered_text(83, "SHOTS                HITS", 0xffffff);
        }
        const char* pause_choices[] = {"RESUME", "RESTART", "EXIT"};
        const char* result_choices[] = {"RESTART", "EXIT"};
        const unsigned choice = finished ? result_choice : pause_choice;
        const unsigned count = finished ? 2 : 3;
        for (unsigned i = 0; i < count; ++i) {
            if (i == choice) rect(75, 99 + i * 16, 170, 14, 0x295166);
            centered_text(102 + i * 16, finished ? result_choices[i] : pause_choices[i], i == choice ? 0x70e0ff : 0xffffff);
        }
    } else {
        centered_text(58, titles[content_id].name, 0xffffff);
        centered_text(70, titles[content_id].variant, 0xffffff);
        const unsigned count = option_count();
        const auto layout = launch_layout(count, menu_choice);
        const unsigned first = layout.first;
        if (layout.scroll && first) arrow(87, true);
        for (unsigned row = 0; row < layout.rows && first + row < count; ++row) {
            const unsigned i = first + row, y = layout.y + row * layout.spacing;
            if (i == menu_choice) rect(53, y, 214, 14, 0x295166);
            const std::uint32_t color = i == menu_choice ? 0x70e0ff : 0xffffff;
            if (i == 0) {
                centered_text(y + 3, "RETURN", color);
                horizontal_arrow(130, y + 6, true, color);
            } else if (i == 1) {
                centered_text(y + 3, "START", color);
            } else {
                const unsigned setting = i - 2;
                const bool target = content_id == 14 && setting == 0;
                const bool mode = content_id != 14 && setting == 0;
                const unsigned option = setting - 1;
                const bool score = i == 6;
                const char* label = mode ? "MODE" : target ? "TARGET" : score ? (content_id == 14 ? "SHOTS" : "SCORE") : option == 0 ? "SPEED" : option == 1 ? "P1 SIZE" : "P2 SIZE";
                const char* modes[] = {"2 PLAYERS", "1P EASY", "1P NORMAL", "1P HARD"};
                char score_text[4];
                std::snprintf(score_text, sizeof score_text, "%u", content_id == 14 ? shot_limit : winning_score);
                const char* value = mode ? modes[opponent_modes[content_id]] : target ? (automatic_target ? "AUTO" : "MANUAL") : score ? score_text : option == 0 ? (high_speed ? "HIGH" : "LOW") : (small_paddle[option - 1] ? "SMALL" : "LARGE");
                if (mode) {
                    text(87, y + 3, label, color);
                    horizontal_arrow(151, y + 6, true, color);
                    text(161, y + 3, value, color);
                    horizontal_arrow(218, y + 6, false, color);
                } else {
                    text(87, y + 3, label, color);
                    horizontal_arrow(151, y + 6, true, color);
                    text(167, y + 3, value, color);
                    horizontal_arrow(218, y + 6, false, color);
                }
            }
        }
        if (layout.scroll && first + layout.rows < count) arrow(181, false);
    }
}
void select_game(unsigned id) {
    content_id = id;
    opponent.reset(); rally_difficulty.reset();
    tennis = ctg::TennisB(id == 1 || id == 3 || id == 5 || id == 7 || id == 9 || id == 11 || id == 13,
                          id == 4 || id == 5 || id == 6 || id == 7, id == 8 || id == 9 || id == 10 || id == 11, id == 0 || id == 1, id == 4 || id == 5, id == 8 || id == 9, id == 12 || id == 13);
    apply_options();
    shooting.reset();
    tone_remaining = 0;
    static const retro_input_descriptor descriptors[] = {
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Confirm"},
                {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Pause / resume"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Menu back"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "P1 up"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "P1 down"},
        {1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "P2 up"},
        {1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "P2 down"},
        {0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "P1 paddle"},
        {1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "P2 paddle"},
        {0, 0, 0, 0, nullptr}
    };
    static const retro_input_descriptor shooting_descriptors[] = {
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Confirm / fire"},
                {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Pause / resume"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Fire"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Menu back"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Aim up"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Aim down"},
        {0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Aim"},
        {1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Manual target up"},
        {1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Manual target down"},
        {1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Manual target"},
        {0, 0, 0, 0, nullptr}
    };
    if (environment) environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, const_cast<retro_input_descriptor*>(content_id == 14 ? shooting_descriptors : descriptors));
}
void message(const char* s) {
    if (environment) { retro_message msg{s, 180}; environment(RETRO_ENVIRONMENT_SET_MESSAGE, &msg); }
}
std::mt19937& random_engine() {
    static std::mt19937 engine([] {
        auto seed = static_cast<std::mt19937::result_type>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count());
        try { seed ^= std::random_device{}(); } catch (...) {}
        return seed;
    }());
    return engine;
}

std::filesystem::path last_game_path() {
    const char* directory = nullptr;
    if (!environment || !environment(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &directory) ||
        !directory || !*directory) return {};
    return std::filesystem::u8path(directory) / "colortvgame-last-game";
}

unsigned read_last_game() noexcept {
    try {
        const auto path = last_game_path();
        if (path.empty()) return selector_order[0];
        std::ifstream file(path, std::ios::binary);
        char id;
        if (!file.get(id) || file.peek() != std::char_traits<char>::eof() || file.bad()) return selector_order[0];
        const auto value = static_cast<unsigned char>(id);
        return value < ctg::slot_count ? value : selector_order[0];
    } catch (...) { return selector_order[0]; }
}

bool write_last_game(unsigned id) noexcept {
    try {
        const auto path = last_game_path();
        if (path.empty()) return false;
        for (unsigned attempt = 0; attempt < 16; ++attempt) {
            auto temporary = path;
            temporary += ".tmp-" + std::to_string(random_engine()());
#ifdef _WIN32
            const int fd = _wopen(temporary.c_str(), _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY,
                                  _S_IREAD | _S_IWRITE);
#else
            const int fd = open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
#endif
            if (fd < 0) { if (errno == EEXIST) continue; return false; }
            const unsigned char value = static_cast<unsigned char>(id);
#ifdef _WIN32
            int written;
            do { written = _write(fd, &value, 1); } while (written < 0 && errno == EINTR);
            const int closed = _close(fd);
            const bool replaced = written == 1 && closed == 0 &&
                MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
#else
            ssize_t written;
            do { written = write(fd, &value, 1); } while (written < 0 && errno == EINTR);
            const int closed = close(fd);
            const bool replaced = written == 1 && closed == 0 &&
                std::rename(temporary.c_str(), path.c_str()) == 0;
#endif
            if (!replaced) { std::error_code ec; std::filesystem::remove(temporary, ec); }
            return replaced;
        }
    } catch (...) {}
    return false;
}

unsigned initial_selector_choice() {
    retro_variable variable{"colortvgame_start_game", nullptr};
    const char* value = environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE, &variable) ? variable.value : nullptr;
    if (value && std::strcmp(value, "First") == 0) return 0;
    if (value && std::strcmp(value, "Random") == 0) {
        static unsigned last_random = ctg::slot_count;
        unsigned choice;
        if (last_random >= ctg::slot_count)
            choice = std::uniform_int_distribution<unsigned>(0, ctg::slot_count - 1)(random_engine());
        else {
            choice = std::uniform_int_distribution<unsigned>(0, ctg::slot_count - 2)(random_engine());
            if (choice >= last_random) ++choice;
        }
        return last_random = choice;
    }
    return selector_position(read_last_game());
}

void generate_game_files() {
    const char* directory = nullptr;
    if (environment && !environment(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &directory)) directory = nullptr;
    const auto result = ctg::generate_game_files(directory);
    char text[160];
    std::snprintf(text, sizeof text, "Game files: %u created, %u existing, %u failed%s",
                  result.created, result.existing, result.failed,
                  !directory || !*directory ? " (system directory unavailable)" : "");
    message(text);
}
void update_generate_option() {
    if (!environment) { option_known = false; return; }
    if (option_known) {
        bool updated = false;
        if (!environment(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated)) { option_known = false; return; }
        if (!updated) return;
    }
    retro_variable variable{"colortvgame_generate_files", nullptr};
    if (!environment(RETRO_ENVIRONMENT_GET_VARIABLE, &variable) || !variable.value ||
        (std::strcmp(variable.value, "Ready") != 0 && std::strcmp(variable.value, "Generate") != 0)) {
        option_known = false;
        return;
    }
    const bool selected = std::strcmp(variable.value, "Generate") == 0;
    if (option_known && !generate_selected && selected) {
        generate_game_files();
        retro_variable reset{"colortvgame_generate_files", "Ready"};
        generate_selected = !environment(RETRO_ENVIRONMENT_SET_VARIABLE, &reset);
    } else generate_selected = selected;
    option_known = true;
}
}

extern "C" {
void retro_set_environment(retro_environment_t cb) {
    environment = cb;
    option_known = false;
    demo_sounds = true;
    demo_tone = {};
    rf_noise_frame = 0;
    if (environment) {
        bool supported = true;
        environment(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &supported);
        static constexpr const char* description = "Generate CTG game files in system/Nintendo Color TV-Game. Files can be moved and used like ROMs. Select Generate, then resume the core.";
        unsigned version = 0;
        if (!environment(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version)) version = 0;
        if (version >= 2) {
            static retro_core_option_v2_definition definitions[] = {
                {"colortvgame_generate_files", "Generate game files", nullptr, description, nullptr, nullptr,
                 {{"Ready", nullptr}, {"Generate", nullptr}, {nullptr, nullptr}}, "Ready"},
                {"colortvgame_demo_sounds", "Demo sounds", nullptr, "Play sounds during menu previews.", nullptr, nullptr,
                 {{"On", nullptr}, {"Off", nullptr}, {nullptr, nullptr}}, "On"},
                {"colortvgame_start_game", "Selected game at start", nullptr, "Choose the initial game in the no-content selector.", nullptr, nullptr,
                 {{"Last used", nullptr}, {"First", nullptr}, {"Random", nullptr}, {nullptr, nullptr}}, "Last used"},
                {"colortvgame_demo_rotation", "Demo game rotation", nullptr, "Rotate games while the selector is idle.", nullptr, nullptr,
                 {{"Random every 15 seconds", nullptr}, {"Off", nullptr}, {"Every 15 seconds", nullptr}, {nullptr, nullptr}}, "Random every 15 seconds"},
                {"colortvgame_rebound", "Rebound behavior", nullptr, "Choose the approximate paddle rebound model.", nullptr, nullptr,
                 {{"Hit position", nullptr}, {"Paddle movement", nullptr}, {nullptr, nullptr}}, "Hit position"},
                {"colortvgame_anti_stalemate", "Anti-stalemate", nullptr, "On the third consecutive horizontal paddle rebound, add a small vertical angle at the hit. Modern approximation, not proven original hardware behavior. Applies to competitive games and demos; Off disables it.", nullptr, nullptr,
                 {{"On", nullptr}, {"Off", nullptr}, {nullptr, nullptr}}, "On"},
                {"colortvgame_color", "Color", nullptr, "Display the rendered image in color or grayscale.", nullptr, nullptr,
                 {{"On", nullptr}, {"Off", nullptr}, {nullptr, nullptr}}, "On"},
                {"colortvgame_rf_filter", "RF filter", nullptr, "Approximate composite signal color and bandwidth artifacts.", nullptr, nullptr,
                 {{"Off", nullptr}, {"On", nullptr}, {nullptr, nullptr}}, "Off"},
                {"colortvgame_rf_quality", "RF signal quality", nullptr, "Lower quality adds color bleed and noise. No effect when the RF filter is off.", nullptr, nullptr,
                 {{"100%", nullptr}, {"90%", nullptr}, {"80%", nullptr}, {"70%", nullptr}, {"60%", nullptr}, {"50%", nullptr}, {"40%", nullptr}, {"30%", nullptr}, {"20%", nullptr}, {"10%", nullptr}, {"0%", nullptr}, {nullptr, nullptr}}, "100%"}, {}
            };
            static retro_core_options_v2 options{nullptr, definitions};
            environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &options);
        } else if (version == 1) {
            static retro_core_option_definition definitions[] = {
                {"colortvgame_generate_files", "Generate game files", description,
                 {{"Ready", nullptr}, {"Generate", nullptr}, {nullptr, nullptr}}, "Ready"},
                {"colortvgame_demo_sounds", "Demo sounds", "Play sounds during menu previews.",
                 {{"On", nullptr}, {"Off", nullptr}, {nullptr, nullptr}}, "On"},
                {"colortvgame_start_game", "Selected game at start", "Choose the initial game in the no-content selector.",
                 {{"Last used", nullptr}, {"First", nullptr}, {"Random", nullptr}, {nullptr, nullptr}}, "Last used"},
                {"colortvgame_demo_rotation", "Demo game rotation", "Rotate games while the selector is idle.",
                 {{"Random every 15 seconds", nullptr}, {"Off", nullptr}, {"Every 15 seconds", nullptr}, {nullptr, nullptr}}, "Random every 15 seconds"},
                {"colortvgame_rebound", "Rebound behavior", "Choose the approximate paddle rebound model.",
                 {{"Hit position", nullptr}, {"Paddle movement", nullptr}, {nullptr, nullptr}}, "Hit position"},
                {"colortvgame_anti_stalemate", "Anti-stalemate", "On the third consecutive horizontal paddle rebound, add a small vertical angle at the hit. Modern approximation, not proven original hardware behavior. Applies to competitive games and demos; Off disables it.",
                 {{"On", nullptr}, {"Off", nullptr}, {nullptr, nullptr}}, "On"},
                {"colortvgame_color", "Color", "Display the rendered image in color or grayscale.",
                 {{"On", nullptr}, {"Off", nullptr}, {nullptr, nullptr}}, "On"},
                {"colortvgame_rf_filter", "RF filter", "Approximate composite signal color and bandwidth artifacts.",
                 {{"Off", nullptr}, {"On", nullptr}, {nullptr, nullptr}}, "Off"},
                {"colortvgame_rf_quality", "RF signal quality", "Lower quality adds color bleed and noise. No effect when the RF filter is off.",
                 {{"100%", nullptr}, {"90%", nullptr}, {"80%", nullptr}, {"70%", nullptr}, {"60%", nullptr}, {"50%", nullptr}, {"40%", nullptr}, {"30%", nullptr}, {"20%", nullptr}, {"10%", nullptr}, {"0%", nullptr}, {nullptr, nullptr}}, "100%"}, {}
            };
            environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, definitions);
        } else {
            static retro_variable variables[] = {
                {"colortvgame_generate_files", "Generate game files; Ready|Generate"},
                {"colortvgame_demo_sounds", "Demo sounds; On|Off"},
                {"colortvgame_start_game", "Selected game at start; Last used|First|Random"},
                {"colortvgame_demo_rotation", "Demo game rotation; Random every 15 seconds|Off|Every 15 seconds"},
                {"colortvgame_rebound", "Rebound behavior; Hit position|Paddle movement"},
                {"colortvgame_anti_stalemate", "Anti-stalemate; On|Off"},
                {"colortvgame_color", "Color; On|Off"},
                {"colortvgame_rf_filter", "RF filter; Off|On"},
                {"colortvgame_rf_quality", "RF signal quality (no effect when RF filter is off); 100%|90%|80%|70%|60%|50%|40%|30%|20%|10%|0%"}, {nullptr, nullptr}
            };
            environment(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
        }
    }
}
void retro_set_video_refresh(retro_video_refresh_t cb) { video = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { audio = cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { poll = cb; }
void retro_set_input_state(retro_input_state_t cb) { input = cb; }
void retro_init() { reset_demo(); rf_noise_frame = 0; option_known = false; selector_choice = 0; opponent_modes = {}; opponent.reset(); rally_difficulty.reset(); high_speed = false; small_paddle = {}; winning_score = ctg::TennisB::winning_score; shot_limit = ctg::Shooting::default_shot_limit; apply_options(); repeat_menu = -1; repeat_direction = 0; repeat_frames = 0; content_id = previous = 0; loaded = detail = automatic_target = selector = paused = false; menu_choice = 1; pause_choice = result_choice = 0; joypad = {{true, true}}; tennis.reset(); shooting.reset(); tone_remaining = 0; }
void retro_deinit() { reset_demo(); demo_sounds = false; loaded = detail = false; previous = content_id = 0; }
unsigned retro_api_version() { return RETRO_API_VERSION; }
void retro_get_system_info(retro_system_info* info) { *info = {"Color TV-Game Simulator", CORE_VERSION, "ctg", false, false}; }
void retro_get_system_av_info(retro_system_av_info* info) { *info = {{width, height, width, height, 4.0f / 3.0f}, {60.0, 48000.0}}; }
void retro_set_controller_port_device(unsigned port, unsigned device) { if (port < joypad.size()) joypad[port] = (device == RETRO_DEVICE_JOYPAD || device == RETRO_DEVICE_ANALOG); }
void retro_reset() { reset_demo(); opponent.reset(); rally_difficulty.reset(); repeat_menu = -1; repeat_direction = 0; repeat_frames = 0; detail = false; selector = paused = false; menu_choice = 1; tennis.reset(); shooting.reset(); tone_remaining = 0; }
void retro_run() {
    update_generate_option();
    retro_variable rebound_option{"colortvgame_rebound", nullptr};
    const bool rebound = environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE, &rebound_option) &&
                         rebound_option.value && std::strcmp(rebound_option.value, "Paddle movement") == 0;
    if (rebound != paddle_movement_rebound) {
        paddle_movement_rebound = rebound;
        tennis.set_rebound_mode(rebound);
        demo_tennis.set_rebound_mode(rebound);
    }
    retro_variable anti_stalemate_option{"colortvgame_anti_stalemate", nullptr};
    const bool enabled = !(environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE, &anti_stalemate_option) &&
                           anti_stalemate_option.value && std::strcmp(anti_stalemate_option.value, "Off") == 0);
    if (enabled != anti_stalemate) {
        anti_stalemate = enabled;
        tennis.set_anti_stalemate(enabled);
        demo_tennis.set_anti_stalemate(enabled);
    }
    retro_variable demo_option{"colortvgame_demo_sounds", nullptr};
    demo_sounds = !(environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE, &demo_option) &&
                    demo_option.value && std::strcmp(demo_option.value, "Off") == 0);
    if (!demo_sounds) demo_tone = {};
    retro_variable rotation_option{"colortvgame_demo_rotation", nullptr};
    const char* rotation_value = environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE, &rotation_option)
                                     ? rotation_option.value : nullptr;
    const bool rotation_enabled = !rotation_value || std::strcmp(rotation_value, "Off") != 0;
    const bool rotation_sequential = rotation_value && std::strcmp(rotation_value, "Every 15 seconds") == 0;
    if (poll) poll();
    unsigned buttons = 0;
    if (joypad[0] && input) {
        for (unsigned id : {RETRO_DEVICE_ID_JOYPAD_UP, RETRO_DEVICE_ID_JOYPAD_DOWN, RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_RIGHT, RETRO_DEVICE_ID_JOYPAD_A, RETRO_DEVICE_ID_JOYPAD_B, RETRO_DEVICE_ID_JOYPAD_START, RETRO_DEVICE_ID_JOYPAD_Y})
            if (input(0, RETRO_DEVICE_JOYPAD, 0, id)) buttons |= 1u << id;
    }
    if (input) {

        if (!detail && input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LEFT)) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_LEFT;
        if (!detail && input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RIGHT)) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_RIGHT;
        if ((!detail || paused || result()) && input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_UP)) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_UP;
        if ((!detail || paused || result()) && input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_DOWN)) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
        if ((!detail || paused || result()) && joypad[0]) {
            int axis = input(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
            if (axis < -8192) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_UP;
            if (axis > 8192) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
        }
        if (input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_p)) buttons |= 1u << 31;
        if (input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RETURN)) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_A;
        if (input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_ESCAPE)) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_B;
        if (content_id == 14 && (input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_x) || input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_SPACE))) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_Y;
    }
    const bool preview_menu = loaded && (selector || !detail);
    bool any_input = buttons != 0;
    if (preview_menu && input) {
        for (unsigned port = 0; port < 2; ++port) if (joypad[port]) {
            for (unsigned id = 0; id <= RETRO_DEVICE_ID_JOYPAD_R3; ++id)
                any_input |= input(port, RETRO_DEVICE_JOYPAD, 0, id) != 0;
            for (unsigned stick = 0; stick < 2; ++stick)
                for (unsigned axis = 0; axis < 2; ++axis) {
                    const int value = input(port, RETRO_DEVICE_ANALOG, stick, axis);
                    any_input |= value < -8192 || value > 8192;
                }
        }
        for (unsigned key = 0; key < RETROK_LAST; ++key)
            any_input |= input(0, RETRO_DEVICE_KEYBOARD, 0, key) != 0;
    }
    const bool was_hidden = menu_hidden;
    if (!preview_menu) {
        inactivity_frames = 0;
        demo_id = ctg::slot_count;
        menu_hidden = suppress_wake_input = false;
    } else if (any_input) {
        inactivity_frames = 0;
        if (menu_hidden) { menu_hidden = false; suppress_wake_input = true; }
    } else {
        suppress_wake_input = false;
        if (inactivity_frames < 600 && ++inactivity_frames == 600) menu_hidden = true;
    }
    if (!loaded || !selector || !rotation_enabled || !menu_hidden) rotation_frames = 0;
    else if (was_hidden && ++rotation_frames == 900) {
        rotation_frames = 0;
        if (rotation_sequential) selector_choice = (selector_choice + 1) % ctg::slot_count;
        else {
            unsigned choice = std::uniform_int_distribution<unsigned>(0, ctg::slot_count - 2)(random_engine());
            if (choice >= selector_choice) ++choice;
            selector_choice = choice;
        }
    }
    const unsigned pressed = suppress_wake_input ? 0 : buttons & ~previous;
    previous = buttons;
    const unsigned up_bit = 1u << RETRO_DEVICE_ID_JOYPAD_UP, down_bit = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
    const int menu = !loaded ? -1 : selector ? 0 : result() ? 1 : paused ? 2 : detail ? -1 : 3;
    if (menu != repeat_menu) { repeat_menu = menu; repeat_direction = 0; repeat_frames = 0; }
    unsigned navigation = 0;
    const int direction = suppress_wake_input ? 0 : int(bool(buttons & down_bit)) - int(bool(buttons & up_bit));
    if (menu < 0 || direction == 0) { repeat_direction = 0; repeat_frames = 0; }
    else if (direction != repeat_direction) {
        repeat_direction = direction;
        repeat_frames = 0;
        if (pressed & (direction < 0 ? up_bit : down_bit)) navigation = direction < 0 ? up_bit : down_bit;
        else repeat_direction = 0;
    } else if (++repeat_frames >= repeat_delay && (repeat_frames - repeat_delay) % repeat_interval == 0) {
        navigation = direction < 0 ? up_bit : down_bit;
        repeat_frames = repeat_delay;
    }
    if (loaded) {
        const unsigned up = 1u << RETRO_DEVICE_ID_JOYPAD_UP, down = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
        const unsigned confirm = (1u << RETRO_DEVICE_ID_JOYPAD_A) | (1u << RETRO_DEVICE_ID_JOYPAD_START);
        const unsigned back = 1u << RETRO_DEVICE_ID_JOYPAD_B;
        if (selector) {
            if (navigation & (up | down)) {
                selector_choice = (selector_choice + ((navigation & up) ? ctg::slot_count - 1 : 1)) % ctg::slot_count;
                if (!write_last_game(selector_order[selector_choice])) message("Could not save last selected game");
            } else if (pressed & confirm) { selector = false; menu_choice = 1; }
            if (selector_order[selector_choice] != content_id) select_game(selector_order[selector_choice]);
        } else if (result()) {
            if (navigation & (up | down)) result_choice = 1 - result_choice;
            else if (pressed & (1u << RETRO_DEVICE_ID_JOYPAD_A)) {
                if (result_choice == 1) { detail = false; menu_choice = 1; }
                rally_difficulty.reset(); tennis.reset(); opponent.reset(); shooting.reset(); tone_remaining = 0; result_choice = 0;
            }
        } else if (paused) {
            if (pressed & ((1u << 31) | (1u << RETRO_DEVICE_ID_JOYPAD_START))) paused = false;
            else if (navigation & up) pause_choice = (pause_choice + 2) % 3;
            else if (navigation & down) pause_choice = (pause_choice + 1) % 3;
            else if (pressed & (1u << RETRO_DEVICE_ID_JOYPAD_A)) {
                paused = false;
                if (pause_choice != 0) {
                    if (pause_choice == 2) { detail = false; menu_choice = 1; }
                    rally_difficulty.reset(); tennis.reset(); opponent.reset(); shooting.reset(); tone_remaining = 0;
                }
            }
        } else if (detail) {
            if (pressed & ((1u << RETRO_DEVICE_ID_JOYPAD_START) | (1u << 31))) { paused = true; pause_choice = 0; }
            else if (content_id == 14) {
                {
                    int direction = 0;
                    if (input && joypad[0]) {
                        int axis = input(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
                        direction = (axis > 8192) - (axis < -8192);
                        int digital = (input(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) != 0) - (input(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) != 0);
                        if (digital) direction = digital;
                    }
                    if (input) {
                        int keyboard = (input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_s) != 0) - (input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_w) != 0);
                        if (keyboard) direction = keyboard;
                    }
                    int target_direction = 0;
                    if (input && joypad[1]) {
                        int axis = input(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
                        target_direction = (axis > 8192) - (axis < -8192);
                        int digital = (input(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) != 0) - (input(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) != 0);
                        if (digital) target_direction = digital;
                    }
                    unsigned events = shooting.step(direction, pressed & ((1u << RETRO_DEVICE_ID_JOYPAD_A) | (1u << RETRO_DEVICE_ID_JOYPAD_Y)), target_direction, automatic_target);
                    if (events) {
                        if (events & ctg::sound_win) result_choice = 0;
                        tone_period = (events & ctg::sound_win) ? 80 : (events & ctg::sound_point) ? 120 : 60;
                        tone_remaining = (events & ctg::sound_win) ? 7200 : 2400;
                        tone_phase = 0;
                    }
                }
            } else if (playable()) {
                {
                    int directions[2]{};
                    for (unsigned port = 0; port < 2; ++port) {
                        if (input && joypad[port]) {
                            int axis = input(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
                            directions[port] = (axis > 8192) - (axis < -8192);
                            int digital = (input(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) != 0) - (input(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) != 0);
                            if (digital) directions[port] = digital;
                        }

                    }
                    if (opponent_modes[content_id]) {
                        const unsigned difficulty = rally_difficulty.effective(opponent_modes[content_id]);
                        directions[1] = opponent.step(tennis.state(), difficulty, difficulty == 1);
                    }
                    const ctg::TennisState before = tennis.state();
                    unsigned events = tennis.step({directions[0], directions[1]});
                    rally_difficulty.advance(before, events);
                    if (events) {
                        if (events & ctg::sound_win) result_choice = 0;
                        tone_period = (events & ctg::sound_win) ? 80 : (events & ctg::sound_point) ? 120 : (events & ctg::sound_paddle) ? 60 : 100;
                        tone_remaining = (events & ctg::sound_win) ? 7200 : 2400;
                        tone_phase = 0;
                    }
                }
            }
        } else {
            unsigned count = option_count();
            if (pressed & back) { selector = true; selector_choice = selector_position(content_id); select_game(content_id); }
            else if (navigation & up) menu_choice = (menu_choice + count - 1) % count;
            else if (navigation & down) menu_choice = (menu_choice + 1) % count;
            else if (menu_choice >= 2 && (pressed & ((1u << RETRO_DEVICE_ID_JOYPAD_LEFT) | (1u << RETRO_DEVICE_ID_JOYPAD_RIGHT)))) {
                if (content_id != 14 && menu_choice == 2) {
                    const int delta = (pressed & (1u << RETRO_DEVICE_ID_JOYPAD_RIGHT)) ? 1 : -1;
                    opponent_modes[content_id] = (opponent_modes[content_id] + 4 + delta) % 4;
                } else if (content_id == 14 && menu_choice == 2) automatic_target = !automatic_target;
                else if (menu_choice == 6) {
                    const int delta = (pressed & (1u << RETRO_DEVICE_ID_JOYPAD_RIGHT)) ? 1 : -1;
                    unsigned& value = content_id == 14 ? shot_limit : winning_score;
                    value = ((value / 5 - 1 + 6 + delta) % 6 + 1) * 5;
                    apply_options();
                } else {
                    const unsigned option = menu_choice - 3;
                    if (option == 0) high_speed = !high_speed;
                    else small_paddle[option - 1] = !small_paddle[option - 1];
                    apply_options();
                }
            }
            else if (pressed & confirm) {
                if (menu_choice == 0) { selector = true; selector_choice = selector_position(content_id); select_game(content_id); }
                else if (menu_choice == 1) { detail = true; opponent.reset(); rally_difficulty.reset(); if (content_id == 14) shooting.reset(); else tennis.reset(); }
            }
        }
        const bool preview = selector || !detail;
        if (preview) step_demo(content_id);
        else demo_tone = {};
        if (detail && !paused && !result() && content_id == 14) render_shooting(shooting); else if (detail && !paused && !result() && playable()) render_tennis(tennis); else render();
        retro_variable color_option{"colortvgame_color", nullptr};
        const bool monochrome = environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE, &color_option) &&
                                color_option.value && std::strcmp(color_option.value, "Off") == 0;
        retro_variable rf_option{"colortvgame_rf_filter", nullptr};
        const bool rf_filter = environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE, &rf_option) &&
                               rf_option.value && std::strcmp(rf_option.value, "On") == 0;
        const bool show_outline = (selector || !detail || paused || result()) &&
                                  !((selector || !detail) && menu_hidden);
        if (rf_filter) {
            unsigned quality = 100;
            retro_variable quality_option{"colortvgame_rf_quality", nullptr};
            if (environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE, &quality_option) && quality_option.value) {
                static constexpr const char* values[] = {"100%", "90%", "80%", "70%", "60%", "50%", "40%", "30%", "20%", "10%", "0%"};
                for (unsigned index = 0; index < 11; ++index)
                    if (std::strcmp(quality_option.value, values[index]) == 0) { quality = 100 - 10 * index; break; }
            }
            apply_rf_filter(quality);
        }
        if (monochrome) {
            for (auto& pixel : pixels) {
                const unsigned luma = (299 * ((pixel >> 16) & 0xff) + 587 * ((pixel >> 8) & 0xff) +
                                       114 * (pixel & 0xff) + 500) / 1000;
                pixel = (pixel & 0xff000000u) | (luma << 16) | (luma << 8) | luma;
            }
            if (show_outline) monochrome_selection_outline();
        }
        if (video) video(pixels.data(), width, height, width * sizeof(pixels[0]));
        std::array<std::int16_t, 1600> samples{};
        unsigned& remaining = preview ? demo_tone.remaining : tone_remaining;
        unsigned& phase = preview ? demo_tone.phase : tone_phase;
        const unsigned period = preview ? demo_tone.period : tone_period;
        for (unsigned i = 0; i < 800 && remaining && !paused; ++i, --remaining) {
            samples[2 * i] = samples[2 * i + 1] = phase < period / 2 ? 6000 : -6000;
            phase = (phase + 1) % period;
        }
        if (audio_batch) audio_batch(samples.data(), 800);
        else if (audio) for (unsigned i = 0; i < 800; ++i) audio(samples[2 * i], samples[2 * i + 1]);
    }
}
size_t retro_serialize_size() { return 0; }
bool retro_serialize(void*, size_t) { return false; }
bool retro_unserialize(const void*, size_t) { return false; }
void retro_cheat_reset() {}
void retro_cheat_set(unsigned, bool, const char*) {}
bool retro_load_game(const retro_game_info* game) {
    reset_demo();
    opponent_modes = {}; opponent.reset(); rally_difficulty.reset();
    repeat_menu = -1; repeat_direction = 0; repeat_frames = 0;
    loaded = detail = automatic_target = selector = paused = false; high_speed = false; small_paddle = {}; winning_score = ctg::TennisB::winning_score; shot_limit = ctg::Shooting::default_shot_limit; apply_options(); menu_choice = 1; pause_choice = 0; content_id = previous = 0; tone_remaining = 0; tennis.reset(); shooting.reset();
    auto id = game ? ctg::parse_content(game->data, game->size) : std::optional<std::uint8_t>{0};
    if (!id) { message("Invalid CTG1 content: expected five bytes and ID 0-14"); return false; }
    retro_pixel_format format = RETRO_PIXEL_FORMAT_XRGB8888;
    if (!environment || !environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &format)) { message("XRGB8888 video is required"); return false; }
    selector = !game;
    selector_choice = selector ? initial_selector_choice() : selector_position(*id);
    select_game(selector ? selector_order[selector_choice] : *id);
    loaded = true; return true;
}
bool retro_load_game_special(unsigned, const retro_game_info*, size_t) { return false; }
void retro_unload_game() { reset_demo(); loaded = detail = false; content_id = previous = 0; tone_remaining = 0; tennis.reset(); }
unsigned retro_get_region() { return RETRO_REGION_NTSC; }
void* retro_get_memory_data(unsigned) { return nullptr; }
size_t retro_get_memory_size(unsigned) { return 0; }
}
