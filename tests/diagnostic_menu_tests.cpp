#include "libretro.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return false; } } while (0)
constexpr unsigned bit(unsigned id) { return 1u << id; }
constexpr unsigned chord = bit(RETRO_DEVICE_ID_JOYPAD_START) | bit(RETRO_DEVICE_ID_JOYPAD_L2) | bit(RETRO_DEVICE_ID_JOYPAD_R2);
unsigned buttons = 0, key = 0;
int16_t axis = 0;
std::vector<std::uint32_t> frame;
bool silent = false;
std::string save_dir;
unsigned save_errors = 0;

bool environment(unsigned command, void* data) {
    if (command == RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY) {
        if (save_dir.empty()) return false;
        *static_cast<const char**>(data) = save_dir.c_str(); return true;
    }
    if (command == RETRO_ENVIRONMENT_SET_MESSAGE) {
        const auto* msg = static_cast<retro_message*>(data);
        if (std::string(msg->msg) == "Could not save test menu preferences") ++save_errors;
        return true;
    }
    if (command == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT)
        return *static_cast<retro_pixel_format*>(data) == RETRO_PIXEL_FORMAT_XRGB8888;
    return command == RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME || command == RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS;
}
void video(const void* data, unsigned w, unsigned h, size_t pitch) {
    if (!data || w != 320 || h != 240 || pitch != 320 * 4) { frame.clear(); return; }
    const auto* p = static_cast<const std::uint32_t*>(data);
    frame.assign(p, p + w * h);
}
void poll() {}
int16_t input(unsigned port, unsigned device, unsigned index, unsigned id) {
    if (port != 0) return 0;
    if (device == RETRO_DEVICE_JOYPAD && index == 0 && id < 32) return (buttons & bit(id)) != 0;
    if (device == RETRO_DEVICE_KEYBOARD) return key && id == key;
    if (device == RETRO_DEVICE_ANALOG && index == RETRO_DEVICE_INDEX_ANALOG_LEFT && id == RETRO_DEVICE_ID_ANALOG_Y) return axis;
    return 0;
}
size_t audio(const int16_t* data, size_t count) {
    silent = data && count == 800 && std::all_of(data, data + count * 2, [](int16_t s) { return s == 0; });
    return count;
}
void run(unsigned b = 0) { buttons = b; retro_run(); }
void press(unsigned id) { run(); run(bit(id)); run(); }
void keyboard(unsigned k) { key = k; run(); key = 0; run(); }
void open() { run(); run(chord); }
bool menu() { return frame.size() == 320 * 240 && frame[55 * 320 + 50] == 0x101820; }
unsigned selection() {
    for (unsigned i = 0; i < 6; ++i) if (frame[(82 + i * 18) * 320 + 60] == 0x295166) return i;
    return 99;
}
std::vector<std::uint32_t> row(unsigned i) {
    std::vector<std::uint32_t> result;
    for (unsigned y = 85 + i * 18; y < 98 + i * 18; ++y)
        for (unsigned x = 60; x < 260; ++x) {
            auto pixel = frame[y * 320 + x];
            // Grid blending is applied after the menu; normalize both backgrounds and ink.
            if (pixel == 0x5e7c8c || pixel == 0x295166) pixel = 0x101820;
            if (pixel == 0x4b5157) pixel = 0x101820;
            if (pixel == 0x93e7ff || pixel == 0x70e0ff) pixel = 0xffffff;
            result.push_back(pixel);
        }
    return result;
}
bool reopened_snapshot(const std::array<std::vector<std::uint32_t>, 4>& expected) {
    const bool trace = std::getenv("CTG_RENDER_DIAGNOSTICS") != nullptr;
    const bool valid_frame = frame.size() == 320 * 240;
    const bool actual_menu = menu();
    const unsigned actual_selection = valid_frame ? selection() : 99;
    bool ok = actual_menu && actual_selection == 0;
    std::array<std::vector<std::uint32_t>, 4> actual;
    if (valid_frame) {
        for (unsigned i = 0; i < actual.size(); ++i) {
            actual[i] = row(i);
            if (actual[i] != expected[i]) ok = false;
        }
    } else {
        ok = false;
    }
    if (trace || !ok) {
        // One increment followed by 40 more reaches the upper bound (30), then one decrement.
        const unsigned saved_score = std::min(1u + 40u, 30u) - 1u;
        std::fprintf(stderr, "reopened menu: frame pixels=%zu (expected 76800), menu=%d (expected 1), selection=%u (expected 0)\n",
                     frame.size(), actual_menu, actual_selection);
        std::fprintf(stderr, "expected controls: grid=on fixed=on left=%u right=%u (bounds 0..30)\n", saved_score, saved_score);
        const char* names[] = {"grid", "fixed", "left", "right"};
        for (unsigned i = 0; i < actual.size(); ++i) {
            const auto mismatch = std::mismatch(expected[i].begin(), expected[i].end(), actual[i].begin(), actual[i].end());
            const size_t pos = static_cast<size_t>(mismatch.first - expected[i].begin());
            std::fprintf(stderr, "row %u (%s): frame x=60..259 y=%u..%u expected=%zu actual=%zu %s", i, names[i],
                         85 + i * 18, 97 + i * 18, expected[i].size(), actual[i].size(),
                         expected[i] == actual[i] ? "match" : "DIFFER");
            if (mismatch.first != expected[i].end() && mismatch.second != actual[i].end()) {
                const unsigned x = 60 + static_cast<unsigned>(pos % 200);
                const unsigned y = 85 + i * 18 + static_cast<unsigned>(pos / 200);
                std::fprintf(stderr, " first offset=%zu row=(%zu,%zu) frame=(%u,%u) expected=0x%08x actual=0x%08x raw=0x%08x",
                             pos, pos % 200, pos / 200, x, y,
                             static_cast<unsigned>(*mismatch.first), static_cast<unsigned>(*mismatch.second),
                             static_cast<unsigned>(frame[y * 320 + x]));
            }
            std::fprintf(stderr, "\n");
        }
    }
    return ok;
}
bool load(unsigned id) {
    std::array<unsigned char, 5> bytes{{'C', 'T', 'G', '1', static_cast<unsigned char>(id)}};
    retro_game_info game{}; game.data = bytes.data(); game.size = bytes.size();
    buttons = key = 0; axis = 0;
    return retro_load_game(&game);
}
bool contexts() {
    // Selector, each direct game menu, gameplay and pause, including shooting and racing.
    CHECK(retro_load_game(nullptr)); run();
    const auto background = frame;
    run(chord); CHECK(menu() && selection() == 0 && silent);
    run(); run(bit(RETRO_DEVICE_ID_JOYPAD_B)); CHECK(frame == background);
    run(); CHECK(!menu()); retro_unload_game();
    for (unsigned id = 0; id < 16; ++id) {
        CHECK(load(id)); run(); open(); CHECK(menu() && selection() == 0 && silent);
        press(RETRO_DEVICE_ID_JOYPAD_B); CHECK(!menu());
        press(RETRO_DEVICE_ID_JOYPAD_A); // start game
        open(); CHECK(menu() && silent);
        press(RETRO_DEVICE_ID_JOYPAD_B); CHECK(!menu());
        press(RETRO_DEVICE_ID_JOYPAD_START); // pause
        open(); CHECK(menu() && silent);
        press(RETRO_DEVICE_ID_JOYPAD_B); CHECK(!menu());
        retro_unload_game();
    }
    CHECK(load(14)); run(); press(RETRO_DEVICE_ID_JOYPAD_START);
    for (int shot = 0; shot < 15; ++shot) {
        run(bit(RETRO_DEVICE_ID_JOYPAD_Y));
        for (int i = 0; i < 100; ++i) run();
    }
    CHECK(frame[99 * 320 + 75] == 0x295166); // Result menu selection.
    const auto result = frame;
    open(); CHECK(menu() && silent);
    run(); run(bit(RETRO_DEVICE_ID_JOYPAD_B)); CHECK(frame == result);
    run(); CHECK(frame == result);
    retro_unload_game();
    return true;
}
bool controls() {
    CHECK(load(0)); run(); open(); CHECK(menu() && selection() == 0);
    const auto initial = row(0);
    // A held chord or button must not repeatedly trigger a command.
    run(chord); CHECK(row(0) == initial && selection() == 0);
    run(bit(RETRO_DEVICE_ID_JOYPAD_A)); const auto grid_on = row(0); CHECK(grid_on != initial);
    for (int i = 0; i < 30; ++i) run(bit(RETRO_DEVICE_ID_JOYPAD_A));
    CHECK(row(0) == grid_on);
    keyboard(RETROK_DOWN); CHECK(selection() == 1);
    const auto fixed_off = row(1); keyboard(RETROK_RETURN); const auto fixed_on = row(1); CHECK(fixed_on != fixed_off);
    axis = 20000; run(); axis = 0; run(); CHECK(selection() == 2);
    const auto left_zero = row(2); press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(row(2) == left_zero);
    press(RETRO_DEVICE_ID_JOYPAD_A); CHECK(row(2) == left_zero);
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); const auto left_one = row(2); CHECK(left_one != left_zero);
    for (int i = 0; i < 40; ++i) press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    const auto left_max = row(2); press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(row(2) == left_max);
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(row(2) != left_max);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); CHECK(selection() == 3);
    const auto right_zero = row(3); press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(row(3) == right_zero);
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(row(3) != right_zero);
    for (int i = 0; i < 40; ++i) press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    const auto right_max = row(3); press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(row(3) == right_max);
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(row(3) != right_max);
    const auto left_saved = row(2), right_saved = row(3);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); CHECK(selection() == 4);
    const auto collision_on = row(4);
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); const auto collision_off = row(4); CHECK(collision_off != collision_on);
    keyboard(RETROK_ESCAPE); CHECK(!menu());
    open(); CHECK(reopened_snapshot({{grid_on, fixed_on, left_saved, right_saved}}));
    CHECK(row(4) == collision_off);
    press(RETRO_DEVICE_ID_JOYPAD_UP); CHECK(selection() == 5);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); CHECK(selection() == 0);
    press(RETRO_DEVICE_ID_JOYPAD_UP); press(RETRO_DEVICE_ID_JOYPAD_A); CHECK(!menu());
    open(); CHECK(menu()); retro_reset(); run(); CHECK(!menu());
    open(); CHECK(row(0) == grid_on && row(1) == fixed_on && row(2) == left_saved && row(3) == right_saved && row(4) == collision_off);
    retro_unload_game(); CHECK(load(14)); run(); open();
    CHECK(row(0) == grid_on && row(1) == fixed_on && row(2) == left_saved && row(3) == right_saved && row(4) == collision_off);
    retro_unload_game();
    retro_deinit(); retro_init(); CHECK(load(0)); run(); open();
    CHECK(row(0) == initial && row(1) == fixed_off && row(2) == left_zero && row(3) == right_zero && row(4) == collision_on);
    retro_unload_game();
    return true;
}

bool persistence() {
    namespace fs = std::filesystem;
    const auto dir = fs::temp_directory_path() / ("ctg-diagnostic-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    CHECK(fs::create_directory(dir));
    struct Cleanup { fs::path path; ~Cleanup() { std::error_code ec; fs::remove_all(path, ec); } } cleanup{dir};
    const auto file = dir / "colortvgame-diagnostics";
    auto bytes = [&]() { std::ifstream in(file, std::ios::binary); return std::vector<unsigned char>(std::istreambuf_iterator<char>(in), {}); };
    auto put = [&](const std::vector<unsigned char>& data) { std::ofstream out(file, std::ios::binary | std::ios::trunc); out.write(reinterpret_cast<const char*>(data.data()), data.size()); return bool(out); };
    retro_deinit(); retro_init();
    save_dir = dir.string();
    CHECK(retro_load_game(nullptr)); run(); open();
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_RIGHT); press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    const std::vector<unsigned char> expected{'C','T','G','D',1,1,1,1,2,0};
    CHECK(bytes() == expected);
    const auto timestamp = fs::last_write_time(file);
    for (int i = 0; i < 10; ++i) run();
    CHECK(fs::last_write_time(file) == timestamp);
    retro_unload_game(); retro_deinit(); retro_init();
    CHECK(load(15)); run(); open();
    // Editing one field verifies all other fields were restored, not merely left on disk.
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    auto changed = expected; changed[5] = 0; CHECK(bytes() == changed);
    retro_unload_game(); retro_deinit(); retro_init();
    CHECK(put({'C','T','G','D',1,1}));
    CHECK(retro_load_game(nullptr)); run(); open(); press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    CHECK(bytes() == std::vector<unsigned char>({'C','T','G','D',1,1,0,0,0,1}));
    retro_unload_game(); retro_deinit(); retro_init();
    // Failed writes must not let a stale disk record overwrite changed session values.
    save_dir.clear(); CHECK(load(0)); run(); open();
    const auto errors = save_errors;
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(save_errors == errors + 1);
    CHECK(put({'C','T','G','D',1,0,1,30,30,0}));
    save_dir = dir.string(); retro_unload_game();
    CHECK(bytes() == std::vector<unsigned char>({'C','T','G','D',1,1,0,0,0,1}));
    CHECK(load(14)); run(); open(); press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    CHECK(bytes() == std::vector<unsigned char>({'C','T','G','D',1,0,0,0,0,1}));
    retro_unload_game(); save_dir.clear(); retro_deinit(); retro_init();
    return true;
}

std::uint32_t blend(std::uint32_t p) {
    auto channel = [](unsigned c) { return (3 * c + 255) / 4; };
    return (channel((p >> 16) & 255) << 16) | (channel((p >> 8) & 255) << 8) | channel(p & 255);
}
bool grid_overlay()  {
    retro_deinit(); retro_init(); CHECK(load(0)); run(); open(); CHECK(menu());
    const auto clean = frame;
    press(RETRO_DEVICE_ID_JOYPAD_A); CHECK(menu());
    // The toggle changes row zero itself. Every other cell must blend exactly once,
    // including cells over both the underlying picture and the menu.
    for (int r = 0; r < 50; ++r) for (int c = 0; c < 41; ++c)
        for (int dy = 0; dy < 4; ++dy) for (int dx = 0; dx < 6; ++dx) {
            const int x = 37 + c * 6 + dx, y = 20 + r * 4 + dy;
            if (y >= 82 && y < 96) continue;
            const auto expected = dx < 5 && dy < 3 ? blend(clean[y * 320 + x]) : clean[y * 320 + x];
            CHECK(frame[y * 320 + x] == expected);
        }
    // Check all 3x5 coordinate digits independently, including leading zeroes.
    const char* digits[10] = {"111101101101111", "001001001001001", "111001111100111",
                              "111001111001111", "101101111001001", "111100111001111",
                              "111100111101111", "111001001001001", "111101111101111",
                              "111101111001111"};
    auto digit = [&](int x, int y, unsigned value) {
        for (int r = 0; r < 5; ++r) for (int c = 0; c < 3; ++c) {
            const int px = x + c, py = y + r;
            const auto expected = digits[value][r * 3 + c] == '1' ? 0xffffffu : (clean[py * 320 + px] & 0xfefefeu) >> 1;
            if (frame[py * 320 + px] != expected) return false;
        }
        return true;
    };
    for (unsigned c = 0; c < 41; ++c) {
        CHECK(digit(38 + c * 6, 8, c / 10));
        CHECK(digit(38 + c * 6, 14, c % 10));
        for (unsigned y = 8; y < 19; ++y)
            CHECK(frame[y * 320 + 37 + c * 6] == ((clean[y * 320 + 37 + c * 6] & 0xfefefeu) >> 1));
        CHECK(frame[13 * 320 + 38 + c * 6] == ((clean[13 * 320 + 38 + c * 6] & 0xfefefeu) >> 1));
    }
    for (unsigned r = 0; r < 50; r += 2) {
        CHECK(digit(29, 20 + r * 4, r / 10));
        CHECK(digit(33, 20 + r * 4, r % 10));
        for (unsigned y = 20 + r * 4; y < 25 + r * 4; ++y)
            CHECK(frame[y * 320 + 32] == ((clean[y * 320 + 32] & 0xfefefeu) >> 1));
        for (unsigned y = 25 + r * 4; y < 28 + r * 4; ++y)
            for (unsigned x = 29; x < 36; ++x)
                CHECK(frame[y * 320 + x] == clean[y * 320 + x]);
    }
    // Repeated frames must not compound the blend; disabling restores the clean menu.
    const auto overlaid = frame; for (int i = 0; i < 5; ++i) run(); CHECK(frame == overlaid);
    press(RETRO_DEVICE_ID_JOYPAD_A); CHECK(frame == clean);
    retro_unload_game(); return true;
}

bool racing_grid_contexts() {
    retro_deinit(); retro_init(); CHECK(load(15)); run();
    // Toggle over the frozen launch-menu frame and verify both road edges.
    open(); const auto clean = frame;
    press(RETRO_DEVICE_ID_JOYPAD_A);
    for (int row = 0; row < 48; ++row) for (int col = 0; col < 53; ++col)
        for (int dy = 0; dy < 5; ++dy) for (int dx = 0; dx < 4; ++dx) {
            const int x = 54 + col * 4 + dx, y = row * 5 + dy;
            if (y < 11 || (y >= 82 && y < 96)) continue; // Labels and changed menu row.
            const auto original = clean[y * 320 + x];
            CHECK(frame[y * 320 + x] == (dx < 3 && dy < 4 ? blend(original) : original));
        }
    CHECK(frame[40 * 320 + 53] == clean[40 * 320 + 53]);
    CHECK(frame[40 * 320 + 266] == clean[40 * 320 + 266]);
    // Check the last coordinate label, X52, including its leading digit.
    CHECK(frame[0 * 320 + 262] == 0xffffff);
    CHECK(frame[6 * 320 + 262] == 0xffffff);
    press(RETRO_DEVICE_ID_JOYPAD_B);
    // Return to the selector with Racing selected, then to the launch menu.
    press(RETRO_DEVICE_ID_JOYPAD_B);
    open(); const auto selector_grid = frame;
    press(RETRO_DEVICE_ID_JOYPAD_A); const auto selector_clean = frame;
    CHECK(selector_grid[40 * 320 + 264] == blend(selector_clean[40 * 320 + 264]));
    CHECK(selector_grid[40 * 320 + 265] == selector_clean[40 * 320 + 265]);
    press(RETRO_DEVICE_ID_JOYPAD_A); press(RETRO_DEVICE_ID_JOYPAD_B);
    press(RETRO_DEVICE_ID_JOYPAD_A); press(RETRO_DEVICE_ID_JOYPAD_A);
    open(); const auto gameplay_grid = frame;
    press(RETRO_DEVICE_ID_JOYPAD_A); const auto gameplay_clean = frame;
    CHECK(gameplay_grid[40 * 320 + 264] == blend(gameplay_clean[40 * 320 + 264]));
    CHECK(gameplay_grid[40 * 320 + 265] == gameplay_clean[40 * 320 + 265]);
    retro_unload_game(); return true;
}

bool racing_collision_setting() {
    auto play = [](bool enabled) {
        retro_deinit(); retro_init();
        if (!load(15)) return std::vector<std::vector<std::uint32_t>>{};
        run(); open();
        for (int i = 0; i < 4; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        press(RETRO_DEVICE_ID_JOYPAD_A);
        if (enabled) press(RETRO_DEVICE_ID_JOYPAD_A);
        press(RETRO_DEVICE_ID_JOYPAD_B);
        press(RETRO_DEVICE_ID_JOYPAD_A);
        for (int i = 0; i < 300; ++i) run();
        for (int i = 0; i < 10; ++i) run(bit(RETRO_DEVICE_ID_JOYPAD_LEFT));
        std::vector<std::vector<std::uint32_t>> frames;
        for (int i = 0; i < 600; ++i) {
            run();
            if (i % 10 == 0) frames.push_back(frame);
        }
        retro_unload_game();
        return frames;
    };
    const auto on = play(true), off = play(false);
    CHECK(!on.empty() && on.size() == off.size());
    CHECK(on != off);
    return true;
}

bool fixed_scores() {
    retro_deinit(); retro_init(); CHECK(load(0)); run();
    press(RETRO_DEVICE_ID_JOYPAD_A); // Start gameplay.
    open(); press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); CHECK(selection() == 2);
    const char* digits[10] = {"11111001100110011001100110011111", "00010001000100010001000100010001",
        "11110001000111111000100010001111", "11110001000111110001000100011111",
        "10011001100111110001000100010001", "11111000100011110001000100011111",
        "11111000100011111001100110011111", "11111001100110010001000100010001",
        "11111001100111111001100110011111", "11111001100111110001000100011111"};
    auto score = [&](bool right, unsigned value, std::uint32_t color) {
        const int first = right ? 29 : 0, last = right ? 40 : 14;
        const int ones = right ? 34 : (value >= 20 ? 8 : 5);
        for (int y = 0; y < 32; ++y) for (int x = first * 6; x < last * 6; ++x) {
            const int col = x / 6;
            bool ink = false;
            if (col >= ones && col < ones + 4)
                ink = digits[value % 10][(y / 4) * 4 + col - ones] == '1';
            if (value >= 10 && col >= ones - 5 && col < ones - 1)
                ink = digits[value / 10][(y / 4) * 4 + col - (ones - 5)] == '1';
            if ((frame[(44 + y) * 320 + 37 + x] == color) != ink) return false;
        }
        return true;
    };
    for (unsigned value = 0; value <= 30; ++value) {
        press(RETRO_DEVICE_ID_JOYPAD_B); run();
        CHECK(score(false, value, 0xcdf0ff) && score(true, value, 0xcdf0ff));
        if (value == 30) break;
        open(); press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        press(RETRO_DEVICE_ID_JOYPAD_RIGHT); press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    }
    open(); press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    press(RETRO_DEVICE_ID_JOYPAD_B); run(); CHECK(!score(false, 30, 0xcdf0ff) && !score(true, 30, 0xcdf0ff));
    // Enable again and exercise the same fixed values in shooting and racing.
    open(); press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    retro_unload_game(); CHECK(load(14)); run(); press(RETRO_DEVICE_ID_JOYPAD_A);
    auto large_digit = [&](int x, int y, int cell_width, unsigned digit) {
        for (int r = 0; r < 8; ++r) for (int c = 0; c < 4; ++c) {
            const int px = x + c * cell_width + cell_width / 2, py = y + r * 33 / 8 + 1;
            const auto color = cell_width == 8 ? 0xffc9fau : 0xffffffu;
            if ((frame[py * 320 + px] == color) != (digits[digit][r * 4 + c] == '1')) return false;
        }
        return true;
    };
    CHECK(score(false, 30, 0xffc9fa) && score(true, 30, 0xffc9fa));
    for (unsigned value = 30; value > 0; --value) {
        open(); press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        press(RETRO_DEVICE_ID_JOYPAD_LEFT); press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        press(RETRO_DEVICE_ID_JOYPAD_LEFT); press(RETRO_DEVICE_ID_JOYPAD_B); run();
        CHECK(score(false, value - 1, 0xffc9fa) && score(true, value - 1, 0xffc9fa));
    }
    open(); press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    for (int i = 0; i < 30; ++i) press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    for (int i = 0; i < 30; ++i) press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    press(RETRO_DEVICE_ID_JOYPAD_B); run();
    for (int shot = 0; shot < 17; ++shot) {
        run(bit(RETRO_DEVICE_ID_JOYPAD_Y));
        for (int i = 0; i < 100; ++i) run();
    }
    CHECK(frame[99 * 320 + 75] != 0x295166); // Endless mode passed the normal shot limit.
    CHECK(score(false, 30, 0xffc9fa) && score(true, 30, 0xffc9fa));
    open(); press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    press(RETRO_DEVICE_ID_JOYPAD_B); run();
    run(bit(RETRO_DEVICE_ID_JOYPAD_Y)); for (int i = 0; i < 100; ++i) run();
    CHECK(frame[99 * 320 + 75] == 0x295166);
    open(); press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    retro_unload_game(); CHECK(load(15)); run(); press(RETRO_DEVICE_ID_JOYPAD_A);
    CHECK(large_digit(70, 20, 4, 3) && large_digit(102, 20, 4, 0));
    retro_unload_game();
    return true;
}
}
int main() {
    retro_set_environment(environment); retro_set_video_refresh(video); retro_set_audio_sample_batch(audio);
    retro_set_input_poll(poll); retro_set_input_state(input); retro_init();
    const bool ok = contexts() && controls() && persistence() && grid_overlay() && racing_grid_contexts() && racing_collision_setting() && fixed_scores();
    retro_deinit(); return ok ? 0 : 1;
}
