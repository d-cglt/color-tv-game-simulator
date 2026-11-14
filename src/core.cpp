#include "content.h"
#include "generator.h"
#include "hockey_ball.h"
#include "libretro.h"
#include "tennis_b.h"
#include "opponent.h"
#include "shooting.h"
#include "racing.h"
#include "score_digits.h"
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
    2, 3, 6, 7, 10, 11, 0, 1, 4, 5, 8, 9, 12, 13, 14, 15
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
    {"SHOOTING GAME", ""}, {"RACING 112 (PROTO)", ""}
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
bool loaded = false, detail = false;
unsigned shooting_target = 0, shooting_direction = 0, shooting_blinking = 1;
std::mt19937 shooting_random{std::random_device{}()};
bool selector = false, paused = false;
bool test_grid = false, test_fixed_score = false, test_menu = false;
unsigned test_left_score = 0, test_right_score = 0, test_choice = 0;
std::array<std::uint32_t, width * height> test_background{}, test_clean_frame{};
unsigned selector_choice = 0, menu_choice = 1, pause_choice = 0, result_choice = 0;
unsigned rotation_frames = 0;
int repeat_menu = -1, repeat_direction = 0;
unsigned repeat_frames = 0;
constexpr unsigned repeat_delay = 24, repeat_interval = 6;
bool playable() { return content_id < ctg::slot_count; }
std::array<bool, 2> joypad{{true, true}};
retro_rumble_interface rumble{};
std::array<unsigned, 2> rumble_frames{};
bool rumble_enabled = false;
constexpr unsigned sound_mask = ctg::sound_wall | ctg::sound_paddle | ctg::sound_point | ctg::sound_win;

void stop_rumble(unsigned port) {
    if (rumble_frames[port] && rumble.set_rumble_state)
        rumble.set_rumble_state(port, RETRO_RUMBLE_STRONG, 0);
    rumble_frames[port] = 0;
}
void stop_rumble() {
    for (unsigned port = 0; port < rumble_frames.size(); ++port) stop_rumble(port);
}
void pulse_rumble(unsigned port) {
    if (!rumble_enabled || !joypad[port] || !rumble.set_rumble_state) return;
    rumble.set_rumble_state(port, RETRO_RUMBLE_STRONG, 0x8000);
    rumble_frames[port] = 6;
}
void advance_rumble() {
    for (unsigned port = 0; port < rumble_frames.size(); ++port)
        if (rumble_frames[port] && --rumble_frames[port] == 0 && rumble.set_rumble_state)
            rumble.set_rumble_state(port, RETRO_RUMBLE_STRONG, 0);
}
ctg::TennisB tennis;
ctg::Opponent opponent;
ctg::RallyDifficulty rally_difficulty;
std::array<unsigned, 14> opponent_modes{};
ctg::Shooting shooting;
ctg::Racing racing, demo_racing;
bool racing_analog_active = false;
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
    racing_analog_active = false;
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
        demo_racing.reset_preview();
        demo_shooting = ctg::Shooting{};
        demo_shooting.set_target_behavior(11, 1, true);
        for (auto& ai : demo_opponents) ai.reset();
    }
    unsigned events = 0;
    if (id == 15) {
        demo_racing.step_preview();
    } else if (id == 14) {
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
    events &= sound_mask;
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
bool preferences_dirty = false;
void apply_options() { tennis.set_anti_stalemate(anti_stalemate); tennis.set_rebound_mode(paddle_movement_rebound); tennis.set_options(high_speed, small_paddle); tennis.set_winning_score(winning_score); shooting.set_options(high_speed, small_paddle); shooting.set_shot_limit(shot_limit); }
enum ShootingRow : unsigned { shooting_return, shooting_start, shooting_target_row, shooting_direction_row,
                              shooting_blinking_row, shooting_speed_row, shooting_p1_size_row,
                              shooting_p2_size_row, shooting_shots_row, shooting_row_count };
unsigned option_count() { return content_id == 15 ? 2 : content_id == 14 ? shooting_row_count : 7; }
void resolve_shooting_settings() {
    const unsigned target = shooting_target == 4 ? std::uniform_int_distribution<unsigned>(1, 3)(shooting_random) : shooting_target;
    const unsigned direction = shooting_target != 0 && shooting_direction == 2 ? std::uniform_int_distribution<unsigned>(0, 1)(shooting_random) : shooting_direction;
    const unsigned blinking = shooting_blinking == 2 ? std::uniform_int_distribution<unsigned>(0, 1)(shooting_random) : shooting_blinking;
    const unsigned speeds[] = {11, 6, 11, 16};
    shooting.set_target_behavior(speeds[target], direction == 0 ? 1 : -1, blinking != 0);
}
void start_shooting_match() {
    resolve_shooting_settings();
    shooting.reset();
}
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
bool result() { return detail && (content_id == 15 ? racing.state().finished : content_id == 14 ? shooting.state().finished : content_id < opponent_modes.size() && tennis.state().winner != -1); }
unsigned tone_remaining = 0, tone_phase = 0, tone_period = 0;
unsigned racing_crash_sample = 0, racing_crash_phase = 0;
constexpr unsigned racing_crash_segment_samples = 12960;
constexpr unsigned racing_crash_total_samples = 6 * racing_crash_segment_samples;
constexpr unsigned racing_bonus_tone_samples = 6336;
constexpr unsigned racing_bonus_total_samples = 8 * racing_bonus_tone_samples + 12288;
unsigned racing_bonus_sample = racing_bonus_total_samples, racing_bonus_phase = 0;
constexpr unsigned racing_start_interval = 51200;
constexpr unsigned racing_start_pulse = 25440;
constexpr unsigned racing_start_total = 3 * racing_start_interval + 38640;
unsigned racing_start_sample = racing_start_total, racing_start_phase = 0;
unsigned racing_engine_phase = 0;

void start_racing_match(bool from_preview = true) {
    racing.reset_from_layout(from_preview ? demo_racing.state() : racing.state());
    racing_start_sample = racing_start_phase = 0;
    racing_engine_phase = 0;
    racing_crash_sample = racing_crash_total_samples;
    racing_bonus_sample = racing_bonus_total_samples;
    racing_bonus_phase = 0;
}

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
    case '(': return "00010001000100001000010000010000010";
    case ')': return "01000001000001000010000100010001000";
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
void score_digit(int x, int y, unsigned digit, std::uint32_t color, int cell_width) {
    for (int row = 0; row < 8; ++row) {
        const int top = row * 33 / 8;
        const int bottom = (row + 1) * 33 / 8;
        for (int col = 0; col < 4; ++col)
            if (ctg::score_digits[digit][row] & (8u >> col))
                rect(x + col * cell_width, y + top, cell_width, bottom - top, color);
    }
}

constexpr int rally_x(int col) { return 37 + col * 6; }
constexpr int rally_y(int row) { return 20 + row * 4; }

void test_grid_overlay() {
    // White at 25% opacity over the upper-left 5 x 3 pixels of each cell.
    for (int row = 0; row < 50; ++row)
        for (int col = 0; col < 41; ++col)
            for (int dy = 0; dy < 3; ++dy)
                for (int dx = 0; dx < 5; ++dx) {
                    auto& p = pixels[(rally_y(row) + dy) * width + rally_x(col) + dx];
                    const auto blend = [](unsigned c) { return (c * 3 + 255) / 4; };
                    p = (blend((p >> 16) & 255) << 16) | (blend((p >> 8) & 255) << 8) | blend(p & 255);
                }
    constexpr unsigned char digits[10][5] = {
        {7, 5, 5, 5, 7}, {1, 1, 1, 1, 1}, {7, 1, 7, 4, 7},
        {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1}, {7, 4, 7, 1, 7},
        {7, 4, 7, 5, 7}, {7, 1, 1, 1, 1}, {7, 5, 7, 5, 7},
        {7, 5, 7, 1, 7}
    };
    const auto digit = [&](int x, int y, unsigned value) {
        for (int r = 0; r < 5; ++r)
            for (int c = 0; c < 3; ++c)
                if (digits[value][r] & (4u >> c))
                    rect(x + c, y + r, 1, 1, 0xffffff);
    };
    const auto backdrop = [](int x, int y, int w, int h) {
        for (int row = y; row < y + h; ++row)
            for (int col = x; col < x + w; ++col) {
                auto& p = pixels[row * width + col];
                p = (p & 0xfefefeu) >> 1;
            }
    };
    for (int col = 0; col < 41; ++col) {
        backdrop(rally_x(col), 8, 5, 11);
        digit(rally_x(col) + 1, 8, col / 10);
        digit(rally_x(col) + 1, 14, col % 10);
    }
    for (int row = 0; row < 50; row += 2) {
        backdrop(29, rally_y(row), 7, 5);
        digit(29, rally_y(row), row / 10);
        digit(33, rally_y(row), row % 10);
    }
}

void rally_score_value(bool right, unsigned value, std::uint32_t color) {
    const auto digit = [&](int col, unsigned number) {
        for (int row = 0; row < 8; ++row)
            for (int cell = 0; cell < 4; ++cell)
                if (ctg::score_digits[number][row] & (8u >> cell))
                    rect(rally_x(col + cell), rally_y(6 + row), 6, 4, color);
    };
    const int ones_col = right ? 34 : (value >= 20 ? 8 : 5);
    if (value >= 10) digit(ones_col - 5, value / 10 % 10);
    digit(ones_col, value % 10);
}

constexpr std::uint32_t tennis_boundary = 0xdad2b0;
constexpr std::uint32_t tennis_ball = 0xfae7b4;
constexpr std::uint32_t volleyball_boundary = 0xcddcfb;
constexpr std::uint32_t volleyball_ball = 0xc7dbfd;
constexpr std::uint32_t volleyball_score = 0xedfd84;
constexpr std::uint32_t hockey_boundary = 0x98ff8d;

void render_tennis(const ctg::TennisB& tennis, bool show_score = true) {
    const auto& state = tennis.state();
    pixels.fill(state.ping_pong ? 0x000000 : state.hockey ? 0x505068 : state.volleyball ? 0x19b537 : 0x0e7aab);
    rect(rally_x(0), rally_y(0), 246, 200, state.ping_pong ? 0x000000 : state.hockey ? 0x2cc94c : state.volleyball ? 0x6d70f9 : 0xc07645);
    if (state.ping_pong) {
        rect(rally_x(0), rally_y(49), 246, 4, 0xffffff);
        rect(rally_x(20), rally_y(34), 6, 16, 0xffffff);
        rect(rally_x(20), rally_y(42), 6, 16, 0xffffff);
    } else if (state.hockey) {
        const int top = 21 + ctg::TennisB::hockey_goal_top * 188 / ctg::TennisB::height;
        const int bottom = 21 + ctg::TennisB::hockey_goal_bottom * 188 / ctg::TennisB::height;
        for (int x = rally_x(0); x < rally_x(41); ++x) {
            const int progress = std::min(x - rally_x(0), 25);
            const std::uint32_t wall = ((153 + 24 * progress / 25) << 16) | 0xff00 | (142 + 49 * progress / 25);
            rect(x, rally_y(0), 1, rally_y(1) - rally_y(0), wall);
            rect(x, rally_y(49), 1, rally_y(50) - rally_y(49), wall);
            if (x < rally_x(1) || x >= rally_x(40)) {
                rect(x, rally_y(1), 1, top - rally_y(1), wall);
                rect(x, bottom, 1, rally_y(49) - bottom, wall);
            }
        }

        if (state.hockey_a) {
            for (int i = 0; i < 6; ++i) {
                if (state.hockey_barrier_closed || (i != 2 && i != 3))
                    rect(rally_x(20), rally_y(2 + i * 8), 6, 16, hockey_boundary);
            }
        }
    } else if (state.volleyball) {
        rect(rally_x(0), rally_y(0), 246, 4, volleyball_boundary);
        rect(rally_x(0), rally_y(49), 246, 4, volleyball_boundary);
        if (state.volleyball_a) {
            for (int color = 0; color < 2; ++color)
                for (int row = 0; row < 6; ++row)
                    for (int col = 0; col < ctg::tennis_a_block_columns; ++col) {
                        const auto block = ctg::tennis_a_block(color, row, col);
                        rect(block.x, block.y, block.width, block.height, block.red ? 0xc84458 : 0x53c178);
                    }
        } else {
            for (int row = 2; row <= 42; row += 8) rect(rally_x(20), rally_y(row), 6, 16, volleyball_ball);
        }
    } else {
        rect(rally_x(0), rally_y(0), 246, 4, tennis_boundary);
        rect(rally_x(0), rally_y(49), 246, 4, tennis_boundary);
        if (state.tennis_a) {
            for (int color = 0; color < 2; ++color)
                for (int row = 0; row < 6; ++row)
                    for (int col = 0; col < ctg::tennis_a_block_columns; ++col) {
                        const auto block = ctg::tennis_a_block(color, row, col);
                        rect(block.x, block.y, block.width, block.height, block.red ? 0xc84458 : 0x53c178);
                    }
        } else {
            for (int row = 2; row <= 42; row += 8) rect(rally_x(20), rally_y(row), 6, 16, tennis_ball);
        }
    }

    if ((show_score && (state.serve_wait || state.winner != -1)) || test_fixed_score) {
        const std::uint32_t score_color = state.ping_pong ? 0x8affa1 : state.hockey ? 0xaefbff : state.volleyball ? volleyball_score : 0xcdf0ff;
        rally_score_value(false, test_fixed_score ? test_left_score : state.score[0], score_color);
        rally_score_value(true, test_fixed_score ? test_right_score : state.score[1], score_color);
    }
    const auto draw_paddle = [&](int x, int owner, std::uint32_t color) {
        const int top = tennis.paddle_visible_top(owner);
        rect(x, top, 6, tennis.paddle_visible_bottom(owner) - top, color);
    };
    for (int i = 0; i < 2; ++i) {
        draw_paddle(rally_x(i == 0 ? 2 : 38), i,
             state.ping_pong ? 0x9b8cff : state.hockey ? (i == 0 ? 0xff9fff : 0xd5e87c) : state.volleyball ? 0xa6e6ff : 0xd0ecb2);
    }
    if (state.doubles) {
        for (int i = 0; i < 2; ++i) {
            if (state.hockey) {
                const int owner = 1 - i;
                draw_paddle(rally_x(i == 0 ? 14 : 26), owner,
                     owner == 0 ? 0xff9fff : 0xd5e87c);
            } else {
                draw_paddle(rally_x(i == 0 ? 12 : 28), i,
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
            rect(ball_x - 3, ball_y - 2, 7, 5, state.ping_pong ? 0xffffff : state.volleyball ? volleyball_ball : tennis_ball);
        }
    }

}
void render_shooting(const ctg::Shooting& shooting, bool show_score = true) {
    const auto& s = shooting.state();
    const std::uint32_t background = s.hit ? 0xcb4042 : 0x46a79a;
    const std::uint32_t objects = s.hit ? 0xffd5ff : 0xf3d66b;
    pixels.fill(background);
    rect(rally_x(0), rally_y(0), 246, 4, 0x86ffff);
    rect(rally_x(0), rally_y(49), 246, 4, 0x86ffff);
    if (show_score || test_fixed_score) {
        rally_score_value(false, test_fixed_score ? test_left_score : s.shots, 0xffc9fa);
        rally_score_value(true, test_fixed_score ? test_right_score : s.hits, 0xffc9fa);
    }
    const int shooter_height = shooting.paddle_height(0) <= 16 ? 16 : 32;
    const int target_height = shooting.paddle_height(1) <= 16 ? 16 : 32;
    const auto paddle = [&](int x, int top, int height) {
        const int clipped_top = std::max(top, 20);
        const int clipped_bottom = std::min(top + height, 220);
        if (clipped_bottom > clipped_top) rect(x, clipped_top, 6, clipped_bottom - clipped_top, objects);
    };
    paddle(rally_x(2), shooting.shooter_top() + shooting.paddle_height(0) / 2 - shooter_height / 2, shooter_height);
    if (shooting.target_visible())
        paddle(rally_x(38), shooting.target_top() + shooting.paddle_height(1) / 2 - target_height / 2, target_height);
    if (s.flying) rect(s.shot_x, s.shot_y - 2, 7, 5, s.hit ? 0xffd5ff : 0x8ddade);

}
void render_racing_score(const ctg::RacingState& s) {
    const unsigned score = test_fixed_score ? test_left_score : s.score;
    score_digit(s.road.x + 12, 6, (score / 10) % 10, 0xffffff, 4);
    score_digit(s.road.x + 34, 6, score % 10, 0xffffff, 4);
}
void render_racing(const ctg::Racing& game, bool show_score = true) {
    const auto& s = game.state();
    pixels.fill(0x66594a);
    rect(s.road.x, s.road.y, s.road.width, s.road.height, 0x39b800);
    constexpr int road_period = ctg::racing_road_dash_length + ctg::racing_road_gap_length;
    for (int y = -road_period + static_cast<int>(s.road_scroll); y < static_cast<int>(height); y += road_period) {
        rect(s.road.x, y, 4, ctg::racing_road_dash_length, 0xc3f5df);
        rect(s.road.x + s.road.width - 4, y, 4, ctg::racing_road_dash_length, 0xc3f5df);
    }
    const auto car_shape = [](const ctg::RacingRect& car, std::uint32_t color, bool opponent) {
        const auto piece = [color, opponent](int x, int y, int w, int h) {
            const int top = opponent ? std::max(y, 9) : y;
            const int bottom = opponent ? std::min(y + h, 224) : y + h;
            if (bottom > top) rect(x, top, w, bottom - top, color);
        };
        piece(car.x + 4, car.y - 2, 8, 28);
        piece(car.x - ctg::racing_car_overhang, car.y + 2, 4, 10);
        piece(car.x + car.width + ctg::racing_car_overhang - 4, car.y + 2, 4, 10);
        piece(car.x - ctg::racing_car_overhang, car.y + 16, 4, 10);
        piece(car.x + car.width + ctg::racing_car_overhang - 4, car.y + 16, 4, 10);
        piece(car.x + 1, car.y + 6, 3, 2);
        piece(car.x + 12, car.y + 6, 3, 2);
        piece(car.x + 1, car.y + 20, 3, 2);
        piece(car.x + 12, car.y + 20, 3, 2);
    };
    for (const auto& car : s.traffic)
        car_shape(car.rect, car.collided ? 0xffc782 : 0xf5ed48, true);
    if (!show_score || racing_crash_sample >= racing_crash_total_samples ||
        (racing_crash_sample / racing_crash_segment_samples) % 2 == 0)
        car_shape(s.player, 0xeb65e7, false);
    if (show_score || test_fixed_score) render_racing_score(s);
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
    if (content_id == 15) {
        const bool background = preview || result();
        render_racing(background ? demo_racing : racing, !background);
        if (result()) render_racing_score(racing.state());
    }
    else if (content_id == 14) render_shooting(preview ? demo_shooting : shooting, !preview);
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
        centered_text(70, finished ? (content_id >= 14 ? "GAME OVER" : tennis.state().winner == 0 ? "PLAYER 1 WINS" : "PLAYER 2 WINS") : "PAUSED", 0xffffff);
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
        if (content_id == 15) {
            centered_text(145, "PLAYABLE PROTOTYPE", 0xffffff);
            centered_text(157, "IN DEVELOPMENT", 0xffffff);
        }
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
                if (content_id == 14) {
                    const char* targets[] = {"MANUAL", "SLOW", "NORMAL", "FAST", "RANDOM"};
                    const char* directions[] = {"DOWN", "UP", "RANDOM"};
                    const char* blinkings[] = {"OFF", "ON", "RANDOM"};
                    const char* labels[] = {"TARGET", "DIRECTION", "BLINKING", "SPEED", "P1 SIZE", "P2 SIZE", "SHOTS"};
                    char shots[4];
                    std::snprintf(shots, sizeof shots, "%u", shot_limit);
                    const char* values[] = {targets[shooting_target], directions[shooting_direction], blinkings[shooting_blinking],
                                            high_speed ? "HIGH" : "LOW", small_paddle[0] ? "SMALL" : "LARGE",
                                            small_paddle[1] ? "SMALL" : "LARGE", shots};
                    const std::uint32_t row_color = i == shooting_direction_row && shooting_target == 0 ? 0x888888 : color;
                    text(87, y + 3, labels[i - shooting_target_row], row_color);
                    horizontal_arrow(151, y + 6, true, row_color);
                    text(167, y + 3, values[i - shooting_target_row], row_color);
                    horizontal_arrow(218, y + 6, false, row_color);
                    continue;
                }
                if (content_id >= opponent_modes.size()) continue;
                const unsigned setting = i - 2;
                const bool mode = setting == 0;
                const unsigned option = setting - 1;
                const bool score = i == 6;
                const char* label = mode ? "MODE" : score ? "SCORE" : option == 0 ? "SPEED" : option == 1 ? "P1 SIZE" : "P2 SIZE";
                const char* modes[] = {"2 PLAYERS", "1P EASY", "1P NORMAL", "1P HARD"};
                char score_text[4];
                std::snprintf(score_text, sizeof score_text, "%u", winning_score);
                const char* value = mode ? modes[opponent_modes[content_id]] : score ? score_text : option == 0 ? (high_speed ? "HIGH" : "LOW") : (small_paddle[option - 1] ? "SMALL" : "LARGE");
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
    racing.reset();
    racing_analog_active = false;
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
    static const retro_input_descriptor racing_descriptors[] = {
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Confirm"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Menu back"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Pause / resume"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Steer left"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Steer right"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "Select speed 1"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "Select speed 2"},
        {0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Steering"},
        {0, 0, 0, 0, nullptr}
    };
    if (environment) environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, const_cast<retro_input_descriptor*>(content_id == 15 ? racing_descriptors : content_id == 14 ? shooting_descriptors : descriptors));
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

std::filesystem::path save_path(const char* name) {
    const char* directory = nullptr;
    if (!environment || !environment(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &directory) ||
        !directory || !*directory) return {};
    return std::filesystem::u8path(directory) / name;
}

unsigned read_last_game() noexcept {
    try {
        const auto path = save_path("colortvgame-last-game");
        if (path.empty()) return selector_order[0];
        std::ifstream file(path, std::ios::binary);
        char id;
        if (!file.get(id) || file.peek() != std::char_traits<char>::eof() || file.bad()) return selector_order[0];
        const auto value = static_cast<unsigned char>(id);
        return value < ctg::slot_count ? value : selector_order[0];
    } catch (...) { return selector_order[0]; }
}

bool atomic_write(const char* name, const unsigned char* data, std::size_t size) noexcept {
    try {
        const auto path = save_path(name);
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
            std::size_t offset = 0;
            while (offset < size) {
#ifdef _WIN32
                const int written = _write(fd, data + offset, static_cast<unsigned>(size - offset));
#else
                const ssize_t written = write(fd, data + offset, size - offset);
#endif
                if (written < 0 && errno == EINTR) continue;
                if (written <= 0) break;
                offset += static_cast<std::size_t>(written);
            }
#ifdef _WIN32
            const int closed = _close(fd);
            const bool replaced = offset == size && closed == 0 &&
                MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
#else
            const int closed = close(fd);
            const bool replaced = offset == size && closed == 0 &&
                std::rename(temporary.c_str(), path.c_str()) == 0;
#endif
            if (!replaced) { std::error_code ec; std::filesystem::remove(temporary, ec); }
            return replaced;
        }
    } catch (...) {}
    return false;
}

bool write_last_game(unsigned id) noexcept {
    const unsigned char value = static_cast<unsigned char>(id);
    return atomic_write("colortvgame-last-game", &value, 1);
}

// CTGP, version 1, then eight single-byte settings and one mode per rally game ID.
using PreferenceData = std::array<unsigned char, 27>;
PreferenceData preference_data() {
    PreferenceData data{{'C', 'T', 'G', 'P', 1,
        static_cast<unsigned char>(high_speed), static_cast<unsigned char>(small_paddle[0]),
        static_cast<unsigned char>(small_paddle[1]), static_cast<unsigned char>(winning_score),
        static_cast<unsigned char>(shot_limit), static_cast<unsigned char>(shooting_target),
        static_cast<unsigned char>(shooting_direction), static_cast<unsigned char>(shooting_blinking)}};
    for (std::size_t i = 0; i < opponent_modes.size(); ++i)
        data[13 + i] = static_cast<unsigned char>(opponent_modes[i]);
    return data;
}
void read_preferences() noexcept {
    if (preferences_dirty) return;
    // Invalid or unavailable files leave the current session's preferences intact.
    try {
        const auto path = save_path("colortvgame-preferences");
        if (path.empty()) return;
        std::ifstream file(path, std::ios::binary);
        PreferenceData data{};
        if (!file.read(reinterpret_cast<char*>(data.data()), data.size()) ||
            file.peek() != std::char_traits<char>::eof() || file.bad()) return;
        if (data[0] != 'C' || data[1] != 'T' || data[2] != 'G' || data[3] != 'P' || data[4] != 1 ||
            data[5] > 1 || data[6] > 1 || data[7] > 1 ||
            data[8] < 5 || data[8] > 30 || data[8] % 5 ||
            data[9] < 5 || data[9] > 30 || data[9] % 5 ||
            data[10] > 4 || data[11] > 2 || data[12] > 2) return;
        for (std::size_t i = 13; i < data.size(); ++i) if (data[i] > 3) return;
        high_speed = data[5]; small_paddle = {{bool(data[6]), bool(data[7])}};
        winning_score = data[8]; shot_limit = data[9]; shooting_target = data[10];
        shooting_direction = data[11]; shooting_blinking = data[12];
        for (std::size_t i = 0; i < opponent_modes.size(); ++i) opponent_modes[i] = data[13 + i];
        apply_options();
    } catch (...) {}
}
void save_preferences(bool notify = true) {
    const auto data = preference_data();
    preferences_dirty = !atomic_write("colortvgame-preferences", data.data(), data.size());
    if (preferences_dirty && notify) message("Could not save menu preferences");
}
void retry_preferences() {
    if (preferences_dirty) save_preferences(false);
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
                {"colortvgame_vibration", "Vibration", nullptr, "Vibrate controllers during gameplay.", nullptr, nullptr,
                 {{"Off", nullptr}, {"On", nullptr}, {nullptr, nullptr}}, "Off"},
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
                 {{"0%", nullptr}, {"10%", nullptr}, {"20%", nullptr}, {"30%", nullptr}, {"40%", nullptr}, {"50%", nullptr}, {"60%", nullptr}, {"70%", nullptr}, {"80%", nullptr}, {"90%", nullptr}, {"100%", nullptr}, {nullptr, nullptr}}, "100%"}, {}
            };
            static retro_core_options_v2 options{nullptr, definitions};
            environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &options);
        } else if (version == 1) {
            static retro_core_option_definition definitions[] = {
                {"colortvgame_generate_files", "Generate game files", description,
                 {{"Ready", nullptr}, {"Generate", nullptr}, {nullptr, nullptr}}, "Ready"},
                {"colortvgame_vibration", "Vibration", "Vibrate controllers during gameplay.",
                 {{"Off", nullptr}, {"On", nullptr}, {nullptr, nullptr}}, "Off"},
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
                 {{"0%", nullptr}, {"10%", nullptr}, {"20%", nullptr}, {"30%", nullptr}, {"40%", nullptr}, {"50%", nullptr}, {"60%", nullptr}, {"70%", nullptr}, {"80%", nullptr}, {"90%", nullptr}, {"100%", nullptr}, {nullptr, nullptr}}, "100%"}, {}
            };
            environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, definitions);
        } else {
            static retro_variable variables[] = {
                {"colortvgame_generate_files", "Generate game files; Ready|Generate"},
                {"colortvgame_vibration", "Vibration; Off|On"},
                {"colortvgame_demo_sounds", "Demo sounds; On|Off"},
                {"colortvgame_start_game", "Selected game at start; Last used|First|Random"},
                {"colortvgame_demo_rotation", "Demo game rotation; Random every 15 seconds|Off|Every 15 seconds"},
                {"colortvgame_rebound", "Rebound behavior; Hit position|Paddle movement"},
                {"colortvgame_anti_stalemate", "Anti-stalemate; On|Off"},
                {"colortvgame_color", "Color; On|Off"},
                {"colortvgame_rf_filter", "RF filter; Off|On"},
                {"colortvgame_rf_quality", "RF signal quality (no effect when RF filter is off); 100%|0%|10%|20%|30%|40%|50%|60%|70%|80%|90%"}, {nullptr, nullptr}
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
void retro_init() {
    test_grid = test_fixed_score = test_menu = false;
    test_left_score = test_right_score = test_choice = 0;
    preferences_dirty = false; rumble = {}; rumble_frames = {}; rumble_enabled = false; if (environment) environment(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &rumble); reset_demo(); rf_noise_frame = 0; option_known = false; selector_choice = 0; opponent_modes = {}; opponent.reset(); rally_difficulty.reset(); high_speed = false; small_paddle = {}; winning_score = ctg::TennisB::winning_score; shot_limit = ctg::Shooting::default_shot_limit; apply_options(); repeat_menu = -1; repeat_direction = 0; repeat_frames = 0; content_id = previous = 0; loaded = detail = selector = paused = false; shooting_target = 0; shooting_direction = 0; shooting_blinking = 1; menu_choice = 1; pause_choice = result_choice = 0; joypad = {{true, true}}; tennis.reset(); shooting.reset(); tone_remaining = 0; }
void retro_deinit() { retry_preferences(); stop_rumble(); rumble = {}; rumble_enabled = false; reset_demo(); demo_sounds = false; loaded = detail = false; previous = content_id = 0; }
unsigned retro_api_version() { return RETRO_API_VERSION; }
void retro_get_system_info(retro_system_info* info) { *info = {"Color TV-Game Simulator", CORE_VERSION, "ctg", false, false}; }
void retro_get_system_av_info(retro_system_av_info* info) { *info = {{width, height, width, height, 4.0f / 3.0f}, {60.0, 48000.0}}; }
void retro_set_controller_port_device(unsigned port, unsigned device) { if (port < joypad.size()) { stop_rumble(port); joypad[port] = (device == RETRO_DEVICE_JOYPAD || device == RETRO_DEVICE_ANALOG); } }
void retro_reset() {
    test_menu = false;
    stop_rumble(); reset_demo(); opponent.reset(); rally_difficulty.reset(); repeat_menu = -1; repeat_direction = 0; repeat_frames = 0; detail = false; selector = paused = false; menu_choice = 1; tennis.reset(); shooting.reset(); racing.reset(); tone_remaining = 0; }
void retro_run() {
    retro_variable vibration_option{"colortvgame_vibration", nullptr};
    rumble_enabled = environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE, &vibration_option) &&
                     vibration_option.value && std::strcmp(vibration_option.value, "On") == 0;
    if (!rumble_enabled) stop_rumble();
    else advance_rumble();
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
    tennis.set_endless(test_fixed_score);
    shooting.set_endless(test_fixed_score);
    racing.set_endless(test_fixed_score);
    if (poll) poll();
    unsigned buttons = 0;
    if (joypad[0] && input) {
        for (unsigned id : {RETRO_DEVICE_ID_JOYPAD_UP, RETRO_DEVICE_ID_JOYPAD_DOWN, RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_RIGHT, RETRO_DEVICE_ID_JOYPAD_A, RETRO_DEVICE_ID_JOYPAD_B, RETRO_DEVICE_ID_JOYPAD_START, RETRO_DEVICE_ID_JOYPAD_Y, RETRO_DEVICE_ID_JOYPAD_L2, RETRO_DEVICE_ID_JOYPAD_R2})
            if (input(0, RETRO_DEVICE_JOYPAD, 0, id)) buttons |= 1u << id;
    }
    if (input) {

        if ((!detail || test_menu) && input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LEFT)) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_LEFT;
        if ((!detail || test_menu) && input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RIGHT)) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_RIGHT;
        if ((!detail || paused || result() || test_menu) && input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_UP)) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_UP;
        if ((!detail || paused || result() || test_menu) && input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_DOWN)) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
        if ((!detail || paused || result() || test_menu) && joypad[0]) {
            int axis = input(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
            if (axis < -8192) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_UP;
            if (axis > 8192) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
        }
        if (input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_p)) buttons |= 1u << 31;
        if (input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RETURN)) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_A;
        if (input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_ESCAPE)) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_B;
        if (content_id == 14 && (input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_x) || input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_SPACE))) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_Y;
    }
    const unsigned chord = (1u << RETRO_DEVICE_ID_JOYPAD_START) |
                           (1u << RETRO_DEVICE_ID_JOYPAD_L2) | (1u << RETRO_DEVICE_ID_JOYPAD_R2);
    const bool open_test = loaded && !test_menu && (buttons & chord) == chord && (previous & chord) != chord;
    if (open_test) {
        test_menu = true;
        test_choice = 0;
        test_background = test_clean_frame;
        stop_rumble();
    }
    if (test_menu) {
        const unsigned pressed_test = open_test ? 0 : buttons & ~previous;
        previous = buttons;
        const unsigned up = 1u << RETRO_DEVICE_ID_JOYPAD_UP, down = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
        const unsigned left = 1u << RETRO_DEVICE_ID_JOYPAD_LEFT, right = 1u << RETRO_DEVICE_ID_JOYPAD_RIGHT;
        const unsigned confirm = 1u << RETRO_DEVICE_ID_JOYPAD_A, back = 1u << RETRO_DEVICE_ID_JOYPAD_B;
        if (pressed_test & back) test_menu = false;
        else if (pressed_test & up) test_choice = (test_choice + 4) % 5;
        else if (pressed_test & down) test_choice = (test_choice + 1) % 5;
        else if (pressed_test & (left | right | confirm)) {
            const int delta = (pressed_test & right) ? 1 : (pressed_test & left) ? -1 : 0;
            if (test_choice == 0) test_grid = !test_grid;
            else if (test_choice == 1) test_fixed_score = !test_fixed_score;
            else if (test_choice == 2 && delta) test_left_score = static_cast<unsigned>(std::clamp(static_cast<int>(test_left_score) + delta, 0, 30));
            else if (test_choice == 3 && delta) test_right_score = static_cast<unsigned>(std::clamp(static_cast<int>(test_right_score) + delta, 0, 30));
            else if (test_choice == 4 && (pressed_test & confirm)) test_menu = false;
        }
        pixels = test_background;
        if (test_menu) {
            rect(50, 54, 220, 132, 0x101820);
            centered_text(64, "TEST MENU", 0xffffff);
            char labels[5][40];
            std::snprintf(labels[0], sizeof labels[0], "GRID: %s", test_grid ? "ON" : "OFF");
            std::snprintf(labels[1], sizeof labels[1], "FIXED SCORE: %s", test_fixed_score ? "ON" : "OFF");
            std::snprintf(labels[2], sizeof labels[2], "LEFT SCORE: %u", test_left_score);
            std::snprintf(labels[3], sizeof labels[3], "RIGHT SCORE: %u", test_right_score);
            std::snprintf(labels[4], sizeof labels[4], "EXIT");
            for (unsigned i = 0; i < 5; ++i) {
                if (i == test_choice) rect(59, 82 + i * 18, 202, 14, 0x295166);
                centered_text(85 + i * 18, labels[i], i == test_choice ? 0x70e0ff : 0xffffff);
            }
        }
        if (test_grid) test_grid_overlay();
        if (video) video(pixels.data(), width, height, width * sizeof(pixels[0]));
        std::array<std::int16_t, 1600> silence{};
        if (audio_batch) audio_batch(silence.data(), 800);
        else if (audio) for (unsigned i = 0; i < 800; ++i) audio(0, 0);
        return;
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
        if (content_id != 15 || !result()) demo_id = ctg::slot_count;
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
                rally_difficulty.reset(); tennis.reset(); opponent.reset();
                if (content_id == 14 && result_choice == 0) start_shooting_match(); else shooting.reset();
                if (content_id == 15 && result_choice == 0) start_racing_match(); else racing.reset();
                tone_remaining = 0; result_choice = 0;
            }
        } else if (paused) {
            if (pressed & ((1u << 31) | (1u << RETRO_DEVICE_ID_JOYPAD_START))) paused = false;
            else if (navigation & up) pause_choice = (pause_choice + 2) % 3;
            else if (navigation & down) pause_choice = (pause_choice + 1) % 3;
            else if (pressed & (1u << RETRO_DEVICE_ID_JOYPAD_A)) {
                paused = false;
                if (pause_choice != 0) {
                    if (pause_choice == 2) { detail = false; menu_choice = 1; }
                    rally_difficulty.reset(); tennis.reset(); opponent.reset();
                    if (content_id == 14 && pause_choice == 1) start_shooting_match(); else shooting.reset();
                    if (content_id == 15 && pause_choice == 1) start_racing_match(false); else racing.reset();
                    tone_remaining = 0;
                }
            }
        } else if (detail) {
            if (pressed & ((1u << RETRO_DEVICE_ID_JOYPAD_START) | (1u << 31))) { paused = true; pause_choice = 0; }
            else if (content_id == 15) {
                ctg::RacingInput steering{};
                if (input && joypad[0]) {
                    const int axis = input(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
                    if (axis < -8192 || axis > 8192) racing_analog_active = true;
                    steering.analog = racing_analog_active;
                    steering.steering = axis;
                    steering.left = input(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) != 0;
                    steering.right = input(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) != 0;
                    steering.select_high_speed = input(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2) != 0;
                    steering.select_low_speed = input(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2) != 0;
                }
                if (input) {
                    steering.left |= input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LEFT) || input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_a);
                    steering.right |= input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RIGHT) || input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_d);
                }
                if ((steering.left || steering.right) && steering.steering >= -8192 && steering.steering <= 8192)
                    racing_analog_active = steering.analog = false;
                steering.recovering = racing_crash_sample < racing_crash_total_samples;
                const unsigned previous_score = racing.state().score;
                unsigned events = ctg::racing_none;
                if (racing_start_sample < racing_start_total) racing.step_preview();
                else events = racing.step(steering);
                if (previous_score < 50 && racing.state().score >= 50) {
                    racing_bonus_sample = 0;
                    racing_bonus_phase = 0;
                }
                if (events & ctg::racing_collision) {
                    pulse_rumble(0);
                    racing_crash_sample = 0;
                    racing_crash_phase = 0;
                }
                if (events & ctg::racing_finished) result_choice = 0;
            } else if (content_id == 14) {
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
                    unsigned events = shooting.step(direction, pressed & ((1u << RETRO_DEVICE_ID_JOYPAD_A) | (1u << RETRO_DEVICE_ID_JOYPAD_Y)), target_direction, shooting_target != 0);
                    if (events & ctg::event_player1_shot) pulse_rumble(0);
                    if (events & ctg::sound_point) resolve_shooting_settings();
                    events &= sound_mask;
                    if (events) {
                        if (events & ctg::sound_win) result_choice = 0;
                        tone_period = (events & ctg::sound_win) ? 80 : (events & ctg::sound_point) ? 120 : 60;
                        tone_remaining = (events & ctg::sound_win) ? 7200 : 2400;
                        tone_phase = 0;
                    }
                }
            } else if (content_id < opponent_modes.size()) {
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
                    rally_difficulty.advance(before, events & sound_mask);
                    if (events & ctg::event_player1_hit) pulse_rumble(0);
                    if (!opponent_modes[content_id] && (events & ctg::event_player2_hit)) pulse_rumble(1);
                    events &= sound_mask;
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
            else if (content_id < 15 && menu_choice >= 2 && (pressed & ((1u << RETRO_DEVICE_ID_JOYPAD_LEFT) | (1u << RETRO_DEVICE_ID_JOYPAD_RIGHT)))) {
                const auto before = preference_data();
                if (content_id == 14) {
                    const int delta = (pressed & (1u << RETRO_DEVICE_ID_JOYPAD_RIGHT)) ? 1 : -1;
                    switch (menu_choice) {
                    case shooting_target_row: shooting_target = (shooting_target + 5 + delta) % 5; break;
                    case shooting_direction_row: if (shooting_target != 0) shooting_direction = (shooting_direction + 3 + delta) % 3; break;
                    case shooting_blinking_row: shooting_blinking = (shooting_blinking + 3 + delta) % 3; break;
                    case shooting_speed_row: high_speed = !high_speed; apply_options(); break;
                    case shooting_p1_size_row: case shooting_p2_size_row:
                        small_paddle[menu_choice - shooting_p1_size_row] = !small_paddle[menu_choice - shooting_p1_size_row]; apply_options(); break;
                    case shooting_shots_row: shot_limit = ((shot_limit / 5 - 1 + 6 + delta) % 6 + 1) * 5; apply_options(); break;
                    }
                } else if (menu_choice == 2) {
                    const int delta = (pressed & (1u << RETRO_DEVICE_ID_JOYPAD_RIGHT)) ? 1 : -1;
                    opponent_modes[content_id] = (opponent_modes[content_id] + 4 + delta) % 4;
                } else if (menu_choice == 6) {
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
                if (preference_data() != before) save_preferences();
            }
            else if (pressed & confirm) {
                if (menu_choice == 0) { selector = true; selector_choice = selector_position(content_id); select_game(content_id); }
                else if (menu_choice == 1) { detail = true; opponent.reset(); rally_difficulty.reset(); if (content_id == 14) start_shooting_match(); else if (content_id == 15) start_racing_match(); else tennis.reset(); }
            }
        }
        const bool preview = selector || !detail;
        if (preview || paused || result()) stop_rumble();
        if (preview || (content_id == 15 && result())) step_demo(content_id);
        else demo_tone = {};
        if (detail && !paused && !result() && content_id == 15) render_racing(racing); else if (detail && !paused && !result() && content_id == 14) render_shooting(shooting); else if (detail && !paused && !result() && playable()) render_tennis(tennis); else render();
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
        test_clean_frame = pixels;
        if (test_grid) test_grid_overlay();
        if (video) video(pixels.data(), width, height, width * sizeof(pixels[0]));
        std::array<std::int16_t, 1600> samples{};
        if (preview || result()) {
            racing_crash_sample = racing_crash_total_samples;
            racing_bonus_sample = racing_bonus_total_samples;
            racing_start_sample = racing_start_total;
        }
        unsigned& remaining = preview ? demo_tone.remaining : tone_remaining;
        unsigned& phase = preview ? demo_tone.phase : tone_phase;
        const unsigned period = preview ? demo_tone.period : tone_period;
        for (unsigned i = 0; i < 800 && remaining && !paused; ++i, --remaining) {
            samples[2 * i] = samples[2 * i + 1] = phase < period / 2 ? 6000 : -6000;
            phase = (phase + 1) % period;
        }
        if (content_id == 15 && !preview && !result() && !paused) {
            for (unsigned i = 0; i < 800; ++i) {
                if (racing_start_sample < racing_start_total) {
                    const bool final_tone = racing_start_sample >= 3 * racing_start_interval;
                    if (final_tone || racing_start_sample % racing_start_interval < racing_start_pulse) {
                        samples[2 * i] = samples[2 * i + 1] = racing_start_phase < 4800 ? 6000 : -667;
                        racing_start_phase = (racing_start_phase + (final_tone ? 484 : 164)) % 48000;
                    }
                    ++racing_start_sample;
                } else {
                    const unsigned speed = racing.state().speed;
                    const unsigned frequency = speed <= ctg::Racing::low_speed
                        ? 128 + 65 * (speed - ctg::Racing::start_speed) / (ctg::Racing::low_speed - ctg::Racing::start_speed)
                        : 193 + 200 * (speed - ctg::Racing::low_speed) / (ctg::Racing::max_speed - ctg::Racing::low_speed);
                    samples[2 * i] = samples[2 * i + 1] = racing_engine_phase < 4800 ? 6000 : -667;
                    racing_engine_phase = (racing_engine_phase + frequency) % 48000;

                }
                if (racing_crash_sample >= racing_crash_total_samples && racing_bonus_sample < racing_bonus_total_samples) {
                    const unsigned tone = racing_bonus_sample / racing_bonus_tone_samples;
                    const unsigned frequency = tone >= 8 ? 874 : tone % 3 == 0 ? 475 : tone % 3 == 1 ? 627 : 874;
                    samples[2 * i] = samples[2 * i + 1] = racing_bonus_phase < 4800 ? 6000 : -667;
                    racing_bonus_phase = (racing_bonus_phase + frequency) % 48000;
                    ++racing_bonus_sample;
                }
                if (racing_crash_sample < racing_crash_total_samples) {
                    const unsigned frequency = (racing_crash_sample / racing_crash_segment_samples) % 2 == 0 ? 320 : 627;
                    samples[2 * i] = samples[2 * i + 1] = racing_crash_phase < 4800 ? 6000 : -667;
                    racing_crash_phase = (racing_crash_phase + frequency) % 48000;
                    ++racing_crash_sample;
                }
            }
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
    stop_rumble();
    reset_demo();
    opponent.reset(); rally_difficulty.reset();
    repeat_menu = -1; repeat_direction = 0; repeat_frames = 0;
    test_menu = false;
    loaded = detail = selector = paused = false; menu_choice = 1; pause_choice = 0; content_id = previous = 0; tone_remaining = 0; tennis.reset(); shooting.reset(); racing.reset();
    auto id = game ? ctg::parse_content(game->data, game->size) : std::optional<std::uint8_t>{0};
    if (!id) { message("Invalid CTG1 content: expected five bytes and ID 0-15"); return false; }
    retro_pixel_format format = RETRO_PIXEL_FORMAT_XRGB8888;
    if (!environment || !environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &format)) { message("XRGB8888 video is required"); return false; }
    if (preferences_dirty) retry_preferences();
    else read_preferences();
    selector = !game;
    selector_choice = selector ? initial_selector_choice() : selector_position(*id);
    select_game(selector ? selector_order[selector_choice] : *id);
    loaded = true; return true;
}
bool retro_load_game_special(unsigned, const retro_game_info*, size_t) { return false; }
void retro_unload_game() {
    test_menu = false;
    retry_preferences(); stop_rumble(); reset_demo(); loaded = detail = false; content_id = previous = 0; tone_remaining = 0; tennis.reset(); racing.reset(); }
unsigned retro_get_region() { return RETRO_REGION_NTSC; }
void* retro_get_memory_data(unsigned) { return nullptr; }
size_t retro_get_memory_size(unsigned) { return 0; }
}
