#include "core_test_harness.h"
#include "hockey_ball.h"


#include <cstring>

namespace core_test {
int gameplay_rendering_scenarios(ContentContext& context, MenuSnapshot& snapshot) {
    auto& bytes = context.bytes;
    auto& game = context.game;
    frames = polls = audio_frames = 0;
    std::vector<std::vector<unsigned char>> title_frames;
    for (unsigned id = 0; id < 15; ++id) {
        bytes[4] = static_cast<unsigned char>(id);
        CHECK(retro_load_game(&game)); retro_run();
        for (const auto& existing : title_frames) CHECK(!same_menu(existing, frame));
        title_frames.push_back(frame);
        retro_unload_game();
    }
    bytes[4] = 14; CHECK(retro_load_game(&game)); retro_run();
    CHECK(frames == 16 && polls == 16 && audio_frames == 16 * 800);
    auto initial = frame;
    // The first glyph of Variant or Original appears at the expected centered position.
    for (unsigned id = 0; id < 12; ++id) {
        const int sport_length = id < 8 ? 6 : 10;
        const bool variant = id % 4 < 2;
        const int length = sport_length + 1 + (variant ? 7 : 8);
        const int x = (320 - length * 6) / 2 + (sport_length + 1) * 6;
        const char* top = variant ? "10001" : "01110";
        for (int col = 0; col < 5; ++col)
            CHECK((at(title_frames[id], x + col, 58) == 0xffffff) == (top[col] == '1'));
    }
    // Verify the centered title's first glyph row spells SHOOTING GAME.
        const char* shooting_top[] = {"01111", "10001", "01110", "01110", "11111", "11111", "10001", "01111", "00000", "01111", "01110", "10001", "11111"};
        for (int x = 0; x < 320; ++x) {
            const int offset = x - 121;
            const bool ink = offset >= 0 && offset < 78 && offset % 6 < 5 && shooting_top[offset / 6][offset % 6] == '1';
            CHECK((at(initial, x, 58) == 0xffffff) == ink);
            for (int y = 70; y < 77; ++y) CHECK(at(initial, x, y) != 0xffffff);
        }
        CHECK(at(initial, 60, 150) == ((0x46a79a & 0xfefefe) >> 1));
    CHECK(at(initial, 60, 200) == ((0x46a79a & 0xfefefe) >> 1));
    CHECK(at(initial, 60, 204) == 0x46a79a);
    CHECK(at(initial, 75, 114) == 0x295166);
    CHECK(at(initial, 151, 136) == 0xffffff && at(initial, 222, 136) == 0xffffff);
    keys[RETROK_p] = true; retro_run(); CHECK(same_menu(frame, initial));
    keys[RETROK_p] = false; retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, initial));
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); auto& option = snapshot.option; option = frame; CHECK(!same_menu(option, initial));
    CHECK(at(option, 75, 130) == 0x295166 && at(option, 151, 136) == 0x70e0ff && at(option, 222, 136) == 0x70e0ff);
    press(RETRO_DEVICE_ID_JOYPAD_A); CHECK(same_menu(frame, option));
    keys[RETROK_p] = true; retro_run(); CHECK(same_menu(frame, option));
    keys[RETROK_p] = false; retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(!same_menu(frame, option));
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, option));
    press(RETRO_DEVICE_ID_JOYPAD_UP); CHECK(same_menu(frame, initial));
    buttons = 0; retro_run(); keys[RETROK_RETURN] = true; retro_run();
    auto detail = frame; CHECK(detail != initial);
    retro_run(); CHECK(frame == detail); // Held confirmation does not fire a shot.
    keys[RETROK_RETURN] = false; retro_run();
    keys[RETROK_p] = true; retro_run(); auto keyboard_pause = frame; CHECK(keyboard_pause != detail);
    CHECK(at(keyboard_pause, 60, 150) == ((at(detail, 60, 150) & 0xfefefe) >> 1));
    CHECK(at(keyboard_pause, 75, 99) == 0x295166);
    retro_run(); CHECK(frame == keyboard_pause);
    keys[RETROK_p] = false; retro_run();
    keys[RETROK_DOWN] = true; retro_run(); CHECK(frame != keyboard_pause);
    keys[RETROK_DOWN] = false; keys[RETROK_p] = true; retro_run(); CHECK(frame == detail);
    keys[RETROK_p] = false; retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_B); CHECK(!same_menu(frame, initial));
    press(RETRO_DEVICE_ID_JOYPAD_START); auto pause = frame;
    unsigned silent = nonzero_audio;
    for (int i = 0; i < 10; ++i) retro_run();
    CHECK(frame == pause && nonzero_audio == silent);
    keys[RETROK_DOWN] = true; retro_run(); CHECK(frame != pause);
    keys[RETROK_DOWN] = false; retro_run(); keys[RETROK_UP] = true; retro_run(); CHECK(frame == pause);
    keys[RETROK_UP] = false; retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_UP); CHECK(at(frame, 75, 131) == 0x295166);
    press(RETRO_DEVICE_ID_JOYPAD_START); CHECK(frame != pause && !same_menu(frame, initial));
    press(RETRO_DEVICE_ID_JOYPAD_START); CHECK(frame == pause);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); CHECK(at(frame, 75, 115) == 0x295166);
    press(RETRO_DEVICE_ID_JOYPAD_START); CHECK(frame != pause && !same_menu(frame, initial));
    press(RETRO_DEVICE_ID_JOYPAD_START); CHECK(frame == pause);
    press(RETRO_DEVICE_ID_JOYPAD_A); CHECK(frame == detail);
    press(RETRO_DEVICE_ID_JOYPAD_START); CHECK(frame == pause);
    press(RETRO_DEVICE_ID_JOYPAD_B); CHECK(frame == pause);
    press(RETRO_DEVICE_ID_JOYPAD_UP); press(RETRO_DEVICE_ID_JOYPAD_A); CHECK(same_menu(frame, initial));
    press(RETRO_DEVICE_ID_JOYPAD_B); CHECK(!same_menu(frame, initial) && shutdowns == 0);
    retro_reset(); buttons = 0; retro_run(); CHECK(same_menu(frame, initial));
    CHECK(retro_serialize_size() == 0 && !retro_serialize(nullptr, 0) && !retro_unserialize(nullptr, 0));
    CHECK(!retro_get_memory_data(0) && retro_get_memory_size(0) == 0);
    retro_unload_game();
    keys.fill(false); buttons = buttons2 = 0;
    bytes[4] = 2; CHECK(retro_load_game(&game)); retro_run();
    auto menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, menu));
    auto tennis_pixel = [](int x, int y) { std::uint32_t p; std::memcpy(&p, frame.data() + (y * 320 + x) * 4, sizeof p); return p; };
    CHECK(tennis_pixel(0, 0) == 0x0e7aab && tennis_pixel(100, 100) == 0xc07645);
    CHECK(tennis_pixel(100, 20) == 0xdad2b0 && tennis_pixel(100, 216) == 0xdad2b0);
    CHECK(tennis_pixel(67, 44) == 0xcdf0ff);
    for (int row = 1; row < 49; ++row)
        for (int dy = 0; dy < 4; ++dy) {
            const int y = 20 + row * 4 + dy;
            const bool dash = row >= 2 && row <= 45 && (row - 2) % 8 < 4;
            for (int x = 157; x < 163; ++x)
                CHECK(tennis_pixel(x, y) == (dash ? 0xfae7b4u : 0xc07645u));
            CHECK(tennis_pixel(156, y) == 0xc07645 && tennis_pixel(163, y) == 0xc07645);
        }
    auto tennis_start = frame;
    press(RETRO_DEVICE_ID_JOYPAD_UP); CHECK(frame != tennis_start);
    press(RETRO_DEVICE_ID_JOYPAD_START);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); CHECK(at(frame, 75, 115) == 0x295166);
    press(RETRO_DEVICE_ID_JOYPAD_A); CHECK(frame == tennis_start);
    buttons = 0;
    for (int i = 0; i < 130; ++i) retro_run();
    CHECK(tennis_pixel(67, 44) == 0xc07645);
    retro_reset(); retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    auto paddle = [](int x, int y) { std::uint32_t pixel; std::memcpy(&pixel, frame.data() + (y * 320 + x) * 4, sizeof pixel); return pixel == 0xd0ecb2; };
    CHECK(paddle(49, 101) && paddle(265, 101));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    CHECK(paddle(49, 97) && !paddle(49, 133));
    CHECK(!paddle(265, 101) && paddle(265, 133));
    buttons = buttons2 = 0; axes = {{-20000, 20000}}; retro_run();
    CHECK(paddle(49, 94) && paddle(265, 136));
    axes = {{0, 0}}; keys[RETROK_s] = true; keys[RETROK_UP] = true; retro_run();
    CHECK(paddle(49, 94) && paddle(265, 136));
    // Simulate the frontend mapping W/S to P1 and arrows to P2 RetroPad input.
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
    buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_UP; retro_run();
    CHECK(!paddle(49, 94) && !paddle(265, 136));
    buttons = buttons2 = 0;
    keys.fill(false); exit_pause(); CHECK(same_menu(frame, menu));
    keys[RETROK_ESCAPE] = false; buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
    CHECK(!same_menu(frame, menu)); retro_reset(); retro_run(); CHECK(same_menu(frame, menu));
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run(); buttons = 0;
    buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    for (int i = 0; i < 30000; ++i) retro_run();
    buttons = 0;
    CHECK(nonzero_audio > 0);
    auto won = frame;
    for (int i = 0; i < 10; ++i) retro_run();
    CHECK(frame == won);
    buttons2 = 0; buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
    CHECK(frame != won);
    retro_unload_game();
    buttons = buttons2 = 0; keys.fill(false);
    bytes[4] = 3; CHECK(retro_load_game(&game)); retro_run();
    auto doubles_menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, doubles_menu));
    CHECK(tennis_pixel(67, 44) == 0xcdf0ff);
    buttons = 0;
    for (int i = 0; i < 130; ++i) retro_run();
    CHECK(tennis_pixel(67, 44) == 0xc07645);
    retro_reset(); retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(paddle(49, 101) && paddle(109, 101) && paddle(205, 101) && paddle(265, 101));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    CHECK(paddle(49, 97) && paddle(109, 97));
    CHECK(paddle(205, 133) && paddle(265, 133));
    buttons = 0; buttons2 = 0; retro_reset(); retro_run(); CHECK(same_menu(frame, doubles_menu));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
    CHECK(paddle(109, 101) && paddle(205, 101));
    retro_unload_game();
    buttons = 0; bytes[4] = 2; CHECK(retro_load_game(&game)); retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!paddle(109, 101) && !paddle(205, 101));
    retro_unload_game();
    buttons = 0; bytes[4] = 4; CHECK(retro_load_game(&game)); retro_run();
    auto hockey_a_menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, hockey_a_menu));
    auto a_pixel = [](int x, int y) { std::uint32_t p; std::memcpy(&p, frame.data() + (y * 320 + x) * 4, sizeof p); return p; };
    auto check_grid_separator = [&a_pixel](std::uint32_t color, std::uint32_t background, bool middle_open) {
        for (int y = 24; y < 208; ++y) for (int x = 156; x < 164; ++x) {
            const int dy = y - 28;
            const int segment = dy >= 0 ? dy / 32 : -1;
            const bool stroke = x >= 157 && x < 163 && dy >= 0 && segment < 6 &&
                                dy % 32 < 16 && !(middle_open && (segment == 2 || segment == 3));
            if (a_pixel(x, y) != (stroke ? color : background)) return false;
        }
        return true;
    };
    CHECK(check_grid_separator(0x98ff8d, 0x2cc94c, true));
    CHECK(a_pixel(160, 30) == 0x98ff8d && a_pixel(160, 62) == 0x98ff8d);
    CHECK(a_pixel(160, 94) == 0x2cc94c && a_pixel(160, 126) == 0x2cc94c);
    CHECK(a_pixel(160, 158) == 0x98ff8d && a_pixel(160, 190) == 0x98ff8d);
    CHECK(a_pixel(49, 105) == 0xff9fff && a_pixel(265, 105) == 0xd5e87c);
    buttons = 0;
    bool closed_seen = false, reopened_seen = false;
    for (int i = 0; i < 10000 && !reopened_seen; ++i) {
        retro_run();
        if (check_grid_separator(0x98ff8d, 0x2cc94c, false)) closed_seen = true;
        else if (closed_seen && check_grid_separator(0x98ff8d, 0x2cc94c, true)) reopened_seen = true;
    }
    CHECK(closed_seen && reopened_seen);
    retro_reset(); buttons = 0; retro_run(); CHECK(same_menu(frame, hockey_a_menu));
    retro_unload_game();
    buttons = 0; bytes[4] = 5; CHECK(retro_load_game(&game)); retro_run();
    auto hockey_a_doubles_menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, hockey_a_doubles_menu));
    CHECK(check_grid_separator(0x98ff8d, 0x2cc94c, true));
    CHECK(a_pixel(160, 30) == 0x98ff8d && a_pixel(160, 94) == 0x2cc94c);
    auto check_hockey_inner = [&a_pixel](int yellow_y, int pink_y) {
        for (int x = 120; x <= 127; ++x)
            if (a_pixel(x, yellow_y) != (x >= 121 && x <= 126 ? 0xd5e87c : 0x2cc94c)) return false;
        for (int x = 192; x <= 199; ++x)
            if (a_pixel(x, pink_y) != (x >= 193 && x <= 198 ? 0xff9fff : 0x2cc94c)) return false;
        return true;
    };
    CHECK(a_pixel(49, 105) == 0xff9fff && a_pixel(195, 105) == 0xff9fff);
    CHECK(a_pixel(123, 105) == 0xd5e87c && a_pixel(265, 105) == 0xd5e87c);
    CHECK(check_hockey_inner(105, 105));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
    for (int i = 0; i < 40; ++i) retro_run();
    CHECK(a_pixel(49, 20) == 0xff9fff && a_pixel(195, 20) == 0xff9fff);
    CHECK(a_pixel(123, 219) == 0xd5e87c && a_pixel(265, 219) == 0xd5e87c);
    buttons = buttons2 = 0; axes = {{20000, -20000}};
    for (int i = 0; i < 10; ++i) retro_run();
    CHECK(a_pixel(49, 30) == 0xff9fff && a_pixel(195, 30) == 0xff9fff);
    CHECK(a_pixel(123, 200) == 0xd5e87c && a_pixel(265, 200) == 0xd5e87c);
    axes = {{0, 0}}; retro_reset(); retro_run(); CHECK(same_menu(frame, hockey_a_doubles_menu));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(a_pixel(160, 94) == 0x2cc94c && a_pixel(123, 105) == 0xd5e87c);
    buttons = 0; closed_seen = reopened_seen = false;
    for (int i = 0; i < 20000 && !reopened_seen; ++i) {
        retro_run();
        if (check_grid_separator(0x98ff8d, 0x2cc94c, false)) closed_seen = true;
        else if (closed_seen && check_grid_separator(0x98ff8d, 0x2cc94c, true)) reopened_seen = true;
    }
    CHECK(closed_seen && reopened_seen);
    retro_unload_game();
    buttons = 0; bytes[4] = 4; CHECK(retro_load_game(&game)); retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(a_pixel(160, 30) == 0x98ff8d && a_pixel(160, 94) == 0x2cc94c);
    CHECK(a_pixel(123, 105) == 0x2cc94c && a_pixel(195, 105) == 0x2cc94c);
    retro_unload_game();
    buttons = 0; bytes[4] = 6; CHECK(retro_load_game(&game)); retro_run();
    auto hockey_menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, hockey_menu));
    CHECK(pixel(0, 0) == 0x505068 && pixel(100, 100) == 0x2cc94c);
    CHECK(pixel(37, 40) == 0x99ff8e && pixel(280, 40) == 0xb1ffbf);
    CHECK(pixel(37, 120) == 0x2cc94c && pixel(282, 120) == 0x2cc94c);
    CHECK(pixel(44, 40) == 0x2cc94c && pixel(275, 40) == 0x2cc94c);
    CHECK(pixel(160, 30) == 0x2cc94c);
    CHECK(pixel(49, 105) == 0xff9fff && pixel(265, 105) == 0xd5e87c);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
    for (int i = 0; i < 40; ++i) retro_run();
    CHECK(pixel(49, 19) != 0xff9fff && pixel(49, 20) == 0xff9fff);
    CHECK(pixel(49, 23) == 0xff9fff && pixel(49, 24) != 0xff9fff);
    CHECK(pixel(265, 215) != 0xd5e87c && pixel(265, 216) == 0xd5e87c);
    CHECK(pixel(265, 219) == 0xd5e87c && pixel(265, 220) != 0xd5e87c);
    buttons = buttons2 = 0; axes = {{20000, -20000}};
    for (int i = 0; i < 10; ++i) retro_run();
    CHECK(pixel(49, 30) == 0xff9fff && pixel(49, 55) != 0xff9fff);
    CHECK(pixel(265, 200) == 0xd5e87c && pixel(265, 180) != 0xd5e87c);
    axes = {{0, 0}}; retro_reset(); retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    auto count_color = [](std::uint32_t color) {
        unsigned count = 0;
        for (size_t i = 0; i < frame.size(); i += 4) {
            std::uint32_t p; std::memcpy(&p, frame.data() + i, 4);
            count += p == color;
        }
        return count;
    };
    CHECK(count_color(0xaefbff) > 0 && count_color(0x98ff8d) == 0);
    // Rally zero is a connected rectangular outline, including its corners.
    for (int left : {67, 241}) {
        for (int y = 0; y < 32; ++y) for (int x = 0; x < 24; ++x) {
            const bool stroke = x < 6 || x >= 18 || y < 4 || y >= 28;
            CHECK((pixel(left + x, 44 + y) == 0xaefbff) == stroke);
        }
    }
    buttons = 0;
    for (int i = 0; i < 129; ++i) {
        retro_run();
        CHECK(count_color(0xaefbff) > 0 && count_color(0x98ff8d) == 0);
    }
    retro_run();
    CHECK(count_color(0xaefbff) == 0 && count_color(0x98ff8d) == 35);
    int serve_min_x = 320, serve_max_x = -1;
    for (int y = 0; y < 240; ++y) for (int x = 0; x < 320; ++x) {
        if (pixel(x, y) == 0x98ff8d) {
            if (x < serve_min_x) serve_min_x = x;
            if (x > serve_max_x) serve_max_x = x;
        }
    }
    CHECK(serve_min_x == 44 && serve_max_x == 50);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    bool saw_point = false;
    for (int i = 0; i < 10000 && !saw_point; ++i) {
        retro_run();
        saw_point = count_color(0xaefbff) > 0;
    }
    CHECK(saw_point && count_color(0x98ff8d) == 0);
    // The scorer's single-digit one is a narrow, uninterrupted vertical stroke.
    const auto is_one = [&](int left) {
        for (int y = 0; y < 32; ++y) for (int x = 0; x < 24; ++x) {
            const bool stroke = x >= 18;
            if ((pixel(left + x, 44 + y) == 0xaefbff) != stroke) return false;
        }
        return true;
    };
    CHECK(is_one(67) != is_one(247));
    for (int i = 0; i < 129; ++i) {
        retro_run();
        CHECK(count_color(0xaefbff) > 0 && count_color(0x98ff8d) == 0);
    }
    retro_run();
    CHECK(count_color(0xaefbff) == 0 && count_color(0x98ff8d) > 0);
    // Exercise both court boundaries and framebuffer edges independently of rally timing.
    for (int ball_x : {-2, 0, 31, 34, 35, 37, 38, 160, 281, 282, 284, 285, 288, 319, 321}) {
        for (int ball_y : {-1, 0, 120, 239, 240}) {
            std::array<std::uint32_t, 320 * 240> image;
            image.fill(0x123456);
            ctg::draw_hockey_ball(image, ball_x, ball_y);
            for (int y = 0; y < 240; ++y) for (int x = 0; x < 320; ++x) {
                const bool in_ball = x >= ball_x - 3 && x < ball_x + 4 &&
                                     y >= ball_y - 2 && y < ball_y + 3;
                const std::uint32_t expected = !in_ball ? 0x123456 :
                    (x >= 35 && x < 285 ? 0x98ff8d : 0xa098ac);
                CHECK(image[y * 320 + x] == expected);
            }
        }
    }
    // Move away from the visible ball, allowing either side to score.
    for (int i = 0; i < 30000 && pixel(75, 99) != 0x295166; ++i) {
        int ball_y = -1;
        for (int y = 0; y < 240 && ball_y < 0; ++y)
            for (int x = 35; x < 285; ++x)
                if (pixel(x, y) == 0x98ff8d) { ball_y = y + 2; break; }
        if (ball_y >= 0) {
            buttons = buttons2 = 1u << (ball_y < 115 ? RETRO_DEVICE_ID_JOYPAD_DOWN : RETRO_DEVICE_ID_JOYPAD_UP);
        }
        if ((i / 180) % 2) buttons = buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
        retro_run();
    }

    CHECK(pixel(75, 99) == 0x295166 && count_color(0xaefbff) > 0);
    auto hockey_won = frame;
    for (int i = 0; i < 10; ++i) retro_run();
    CHECK(frame == hockey_won);
    buttons2 = 0; buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
    CHECK(frame != hockey_won);
    retro_unload_game();
    buttons = buttons2 = 0; axes = {{0, 0}};
    bytes[4] = 7; CHECK(retro_load_game(&game)); retro_run();
    auto hockey_doubles_menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(pixel(49, 105) == 0xff9fff && pixel(123, 105) == 0xd5e87c);
    CHECK(pixel(195, 105) == 0xff9fff && pixel(265, 105) == 0xd5e87c);
    CHECK(check_hockey_inner(105, 105));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
    for (int i = 0; i < 40; ++i) retro_run();
    for (int x : {49, 195}) {
        CHECK(pixel(x, 20) == 0xff9fff && pixel(x, 23) == 0xff9fff && pixel(x, 24) != 0xff9fff);
        CHECK(pixel(x, 19) != 0xff9fff);
    }
    for (int x : {123, 265}) {
        CHECK(pixel(x, 216) == 0xd5e87c && pixel(x, 219) == 0xd5e87c);
        CHECK(pixel(x, 220) != 0xd5e87c);
    }
    buttons = buttons2 = 0; axes = {{20000, -20000}};
    for (int i = 0; i < 10; ++i) retro_run();
    CHECK(pixel(49, 30) == 0xff9fff && pixel(195, 30) == 0xff9fff);
    CHECK(pixel(123, 200) == 0xd5e87c && pixel(265, 200) == 0xd5e87c);
    axes = {{0, 0}}; retro_reset(); retro_run(); CHECK(same_menu(frame, hockey_doubles_menu));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(pixel(123, 105) == 0xd5e87c && pixel(195, 105) == 0xff9fff);
    retro_unload_game();
    buttons = 0; bytes[4] = 6; CHECK(retro_load_game(&game)); retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(pixel(123, 105) == 0x2cc94c && pixel(195, 105) == 0x2cc94c);
    retro_unload_game();
    buttons = 0; bytes[4] = 3; CHECK(retro_load_game(&game)); retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(paddle(109, 101) && paddle(205, 101));
    retro_unload_game();
    buttons = buttons2 = 0; bytes[4] = 8; CHECK(retro_load_game(&game)); retro_run();
    auto check_separator = []() {
        for (int y = 28; y < 208; ++y) for (int x = 138; x < 181; ++x) {
            const int dx = x - 139;
            const bool red_column = dx >= 0 && dx < 30 && dx % 24 < 6;
            const bool green_column = dx >= 12 && dx < 42 && (dx - 12) % 24 < 6;
            const int dy = y - 28;
            const bool green = green_column && dy < 164 && dy % 32 < 4;
            const bool red = red_column && dy >= 16 && dy < 180 && (dy - 16) % 32 < 4;
            if ((pixel(x, y) == 0x53c178) != green || (pixel(x, y) == 0xc84458) != red) return false;
        }
        return true;
    };
    retro_unload_game();
    for (int id : {0, 1}) {
        buttons = 0; bytes[4] = id; CHECK(retro_load_game(&game)); retro_run();
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
        CHECK(check_separator());
        retro_unload_game();
    }
    auto volleyball_score_area = [](bool visible = true) {
        for (int y = 44; y < 76; ++y) for (int x : {68, 74, 80, 86, 242, 248, 254, 260}) {
            const bool stroke = y < 48 || y >= 72 || x == 68 || x == 86 || x == 242 || x == 260;
            if (pixel(x, y) != (visible && stroke ? 0xedfd84u : 0x6d70f9u)) return false;
        }
        return true;
    };
    buttons = 0; bytes[4] = 8; CHECK(retro_load_game(&game)); retro_run();
    auto volleyball_a_menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, volleyball_a_menu));
    CHECK(pixel(100, 20) == 0xcddcfb && pixel(100, 216) == 0xcddcfb);
    CHECK(pixel(0, 0) == 0x19b537 && pixel(100, 100) == 0x6d70f9);
    CHECK(check_separator());
    CHECK(pixel(160, 36) == 0x6d70f9 && pixel(49, 105) == 0xa6e6ff && pixel(265, 105) == 0xa6e6ff);
    CHECK(pixel(108, 105) == 0x6d70f9 && volleyball_score_area());
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    CHECK(pixel(49, 97) == 0xa6e6ff && pixel(265, 133) == 0xa6e6ff);
    buttons = buttons2 = 0; for (int i = 0; i < 129; ++i) retro_run();
    CHECK(volleyball_score_area(false));
    exit_pause(); CHECK(same_menu(frame, volleyball_a_menu));
    retro_reset(); retro_run(); CHECK(same_menu(frame, volleyball_a_menu));
    retro_unload_game();
    buttons = buttons2 = 0; bytes[4] = 9; CHECK(retro_load_game(&game)); retro_run();
    auto volleyball_a_doubles_menu = frame;
    CHECK(!same_menu(volleyball_a_doubles_menu, volleyball_a_menu));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, volleyball_a_doubles_menu));
    CHECK(pixel(100, 20) == 0xcddcfb && pixel(100, 216) == 0xcddcfb);
    CHECK(pixel(0, 0) == 0x19b537 && pixel(100, 100) == 0x6d70f9);
    CHECK(check_separator());
    CHECK(pixel(160, 36) == 0x6d70f9 && volleyball_score_area());
    for (int x : {49, 109, 205, 265}) CHECK(pixel(x, 105) == 0xa6e6ff);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    for (int x : {49, 109}) CHECK(pixel(x, 97) == 0xa6e6ff && pixel(x, 133) != 0xa6e6ff);
    for (int x : {205, 265}) CHECK(pixel(x, 133) == 0xa6e6ff && pixel(x, 101) != 0xa6e6ff);
    buttons = buttons2 = 0; axes = {{-20000, 20000}}; retro_run();
    for (int x : {49, 109}) CHECK(pixel(x, 94) == 0xa6e6ff);
    for (int x : {205, 265}) CHECK(pixel(x, 136) == 0xa6e6ff);
    axes = {{0, 0}}; for (int i = 0; i < 128; ++i) retro_run();
    CHECK(volleyball_score_area(false));
    exit_pause(); CHECK(same_menu(frame, volleyball_a_doubles_menu));
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(volleyball_score_area());
    for (int x : {49, 109, 205, 265}) CHECK(pixel(x, 105) == 0xa6e6ff);
    retro_reset(); retro_run(); CHECK(same_menu(frame, volleyball_a_doubles_menu));
    retro_unload_game();
    buttons = buttons2 = 0; bytes[4] = 10; CHECK(retro_load_game(&game)); retro_run();
    auto volleyball_menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, volleyball_menu));
    CHECK(pixel(100, 20) == 0xcddcfb && pixel(100, 216) == 0xcddcfb);
    CHECK(check_grid_separator(0xc7dbfd, 0x6d70f9, false));
    CHECK(pixel(0, 0) == 0x19b537 && pixel(100, 100) == 0x6d70f9);
    CHECK(pixel(160, 30) == 0xc7dbfd && pixel(160, 45) == 0x6d70f9);
    CHECK(pixel(49, 105) == 0xa6e6ff && pixel(265, 105) == 0xa6e6ff);
    CHECK(volleyball_score_area());
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
    retro_run();
    CHECK(pixel(49, 97) == 0xa6e6ff && pixel(265, 133) == 0xa6e6ff);
    buttons = buttons2 = 0;
    const auto volleyball_lines = count_color(0xc7dbfd);
    for (int i = 0; i < 129; ++i) retro_run();
    CHECK(volleyball_score_area(false) && count_color(0xc7dbfd) == volleyball_lines + 35);
    exit_pause(); CHECK(same_menu(frame, volleyball_menu));
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(volleyball_score_area());
    retro_reset(); retro_run(); CHECK(same_menu(frame, volleyball_menu));
    retro_unload_game();
    buttons = buttons2 = 0; bytes[4] = 11; CHECK(retro_load_game(&game)); retro_run();
    auto volleyball_doubles_menu = frame;
    CHECK(!same_menu(volleyball_doubles_menu, volleyball_menu));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(pixel(100, 20) == 0xcddcfb && pixel(100, 216) == 0xcddcfb);
    CHECK(check_grid_separator(0xc7dbfd, 0x6d70f9, false));
    CHECK(pixel(0, 0) == 0x19b537 && pixel(100, 100) == 0x6d70f9);
    CHECK(pixel(160, 30) == 0xc7dbfd && volleyball_score_area());
    for (int x : {49, 109, 205, 265}) CHECK(pixel(x, 105) == 0xa6e6ff);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
    retro_run();
    for (int x : {49, 109}) CHECK(pixel(x, 97) == 0xa6e6ff && pixel(x, 133) != 0xa6e6ff);
    for (int x : {205, 265}) CHECK(pixel(x, 133) == 0xa6e6ff && pixel(x, 101) != 0xa6e6ff);
    buttons = buttons2 = 0; axes = {{-20000, 20000}}; retro_run();
    for (int x : {49, 109}) CHECK(pixel(x, 94) == 0xa6e6ff);
    for (int x : {205, 265}) CHECK(pixel(x, 136) == 0xa6e6ff);
    axes = {{0, 0}};
    for (int i = 0; i < 128; ++i) retro_run();
    CHECK(volleyball_score_area(false));
    exit_pause(); CHECK(same_menu(frame, volleyball_doubles_menu));
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(volleyball_score_area() && pixel(109, 105) == 0xa6e6ff);
    retro_reset(); retro_run(); CHECK(same_menu(frame, volleyball_doubles_menu));
    retro_unload_game();
    buttons = buttons2 = 0; bytes[4] = 0; CHECK(retro_load_game(&game)); retro_run();
    auto tennis_a_menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, tennis_a_menu));
    CHECK(check_separator());
    CHECK(pixel(160, 36) == 0xc07645 && pixel(100, 100) == 0xc07645);
    CHECK(pixel(49, 105) == 0xd0ecb2 && pixel(265, 105) == 0xd0ecb2);
    exit_pause(); CHECK(same_menu(frame, tennis_a_menu));
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    retro_reset(); retro_run(); CHECK(same_menu(frame, tennis_a_menu));
    retro_unload_game();
    buttons = buttons2 = 0; bytes[4] = 1; CHECK(retro_load_game(&game)); retro_run();
    auto tennis_a_doubles_menu = frame;
    CHECK(!same_menu(tennis_a_doubles_menu, tennis_a_menu));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, tennis_a_doubles_menu));
    CHECK(check_separator());
    CHECK(pixel(160, 36) == 0xc07645);
    for (int x : {49, 109, 205, 265}) CHECK(paddle(x, 101));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    for (int x : {49, 109}) CHECK(paddle(x, 97) && !paddle(x, 133));
    for (int x : {205, 265}) CHECK(paddle(x, 133) && !paddle(x, 101));
    buttons = buttons2 = 0; axes = {{-20000, 20000}}; retro_run();
    for (int x : {49, 109}) CHECK(paddle(x, 94));
    for (int x : {205, 265}) CHECK(paddle(x, 136));
    axes = {{0, 0}}; exit_pause(); CHECK(same_menu(frame, tennis_a_doubles_menu));
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
    for (int x : {49, 109, 205, 265}) CHECK(paddle(x, 101));
    retro_reset(); retro_run(); CHECK(same_menu(frame, tennis_a_doubles_menu));
    retro_unload_game();
    buttons = buttons2 = 0; bytes[4] = 12; CHECK(retro_load_game(&game)); retro_run();
    auto ping_menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, ping_menu));
    CHECK(pixel(100, 60) == 0 && pixel(160, 156) == 0xffffff);
    CHECK(pixel(160, 177) == 0 && pixel(160, 188) == 0xffffff);
    CHECK(pixel(100, 216) == 0xffffff && pixel(49, 105) == 0x9b8cff);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    CHECK(pixel(49, 98) == 0x9b8cff && pixel(265, 133) == 0x9b8cff);
    exit_pause(); CHECK(same_menu(frame, ping_menu));
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    retro_reset(); retro_run(); CHECK(same_menu(frame, ping_menu));
    retro_unload_game();
    buttons = buttons2 = 0; bytes[4] = 13; CHECK(retro_load_game(&game)); retro_run();
    auto ping_doubles_menu = frame;
    CHECK(!same_menu(ping_doubles_menu, ping_menu));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, ping_doubles_menu));
    CHECK(pixel(100, 60) == 0 && pixel(160, 156) == 0xffffff);
    CHECK(pixel(160, 177) == 0 && pixel(160, 188) == 0xffffff && pixel(100, 216) == 0xffffff);
    CHECK(count_color(0x8affa1) > 0);
    for (int x : {49, 109, 205, 265}) CHECK(pixel(x, 105) == 0x9b8cff);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    for (int x : {49, 109}) CHECK(pixel(x, 97) == 0x9b8cff && pixel(x, 133) != 0x9b8cff);
    for (int x : {205, 265}) CHECK(pixel(x, 133) == 0x9b8cff && pixel(x, 101) != 0x9b8cff);
    buttons = buttons2 = 0; axes = {{-20000, 20000}}; retro_run();
    for (int x : {49, 109}) CHECK(pixel(x, 94) == 0x9b8cff);
    for (int x : {205, 265}) CHECK(pixel(x, 136) == 0x9b8cff);
    axes = {{0, 0}};
    for (int i = 0; i < 128; ++i) retro_run();
    CHECK(count_color(0x8affa1) == 0);
    exit_pause(); CHECK(same_menu(frame, ping_doubles_menu));
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(count_color(0x8affa1) > 0);
    for (int x : {49, 109, 205, 265}) CHECK(pixel(x, 105) == 0x9b8cff);
    retro_reset(); retro_run(); CHECK(same_menu(frame, ping_doubles_menu));
    retro_unload_game();
    buttons = buttons2 = 0; keys.fill(false); bytes[4] = 14; CHECK(retro_load_game(&game)); retro_run();
    auto shooting_menu = frame;
    bool p2_up = false, p2_down = false, p2_axis = false;
    for (const auto& d : descriptors) {
        if (d.port != 1) continue;
        p2_up |= d.device == RETRO_DEVICE_JOYPAD && d.id == RETRO_DEVICE_ID_JOYPAD_UP && std::strstr(d.description, "target");
        p2_down |= d.device == RETRO_DEVICE_JOYPAD && d.id == RETRO_DEVICE_ID_JOYPAD_DOWN && std::strstr(d.description, "target");
        p2_axis |= d.device == RETRO_DEVICE_ANALOG && d.id == RETRO_DEVICE_ID_ANALOG_Y && std::strstr(d.description, "target");
    }
    CHECK(p2_up && p2_down && p2_axis);
    buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_Y; retro_run(); CHECK(same_menu(frame, shooting_menu));
    buttons2 = 0;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    for (int left : {67, 241}) {
        for (int y = 0; y < 32; ++y) for (int x = 0; x < 24; ++x) {
            const bool stroke = x < 6 || x >= 18 || y < 4 || y >= 28;
            CHECK((pixel(left + x, 44 + y) == 0xffc9fa) == stroke);
        }
    }
    CHECK(pixel(265, 101) == 0xf3d66b);
    buttons = 0; retro_run(); CHECK(pixel(265, 101) == 0xf3d66b && pixel(265, 132) == 0xf3d66b);
    keys[RETROK_DOWN] = true; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    CHECK(pixel(265, 101) == 0x46a79a && pixel(265, 135) == 0xf3d66b && pixel(49, 101) == 0xf3d66b && pixel(49, 132) == 0xf3d66b);
    keys[RETROK_DOWN] = false; keys[RETROK_UP] = true; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_UP; retro_run();
    CHECK(pixel(265, 101) == 0xf3d66b && pixel(265, 135) == 0x46a79a && pixel(49, 101) == 0xf3d66b && pixel(49, 132) == 0xf3d66b);
    keys[RETROK_UP] = false; buttons2 = 0; axes[1] = -20000; retro_run(); axes[1] = 0;
    CHECK(pixel(265, 98) == 0xf3d66b && pixel(265, 129) == 0xf3d66b);
    retro_set_controller_port_device(1, RETRO_DEVICE_NONE);
    buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    CHECK(pixel(265, 98) == 0xf3d66b && pixel(265, 129) == 0xf3d66b);
    retro_set_controller_port_device(1, RETRO_DEVICE_JOYPAD); buttons2 = 0;
    CHECK(pixel(100, 100) == 0x46a79a && pixel(49, 110) == 0xf3d66b);
    buttons = 0; keys[RETROK_w] = true; retro_run();
    CHECK(pixel(49, 99) == 0xf3d66b && pixel(49, 132) == 0x46a79a);
    keys[RETROK_w] = false; keys[RETROK_s] = true; retro_run();
    CHECK(pixel(49, 101) == 0xf3d66b && pixel(49, 132) == 0xf3d66b);
    keys[RETROK_s] = false;
    unsigned before_sound = nonzero_audio;
    keys[RETROK_x] = true; retro_run();
    CHECK(nonzero_audio > before_sound && count_color(0x8ddade) > 0);
    keys[RETROK_x] = false;
    for (int i = 0; i < 100; ++i) retro_run();
    before_sound = nonzero_audio;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_Y; retro_run();
    CHECK(nonzero_audio > before_sound && count_color(0x8ddade) > 0);
    exit_pause(); CHECK(same_menu(frame, shooting_menu));
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    retro_reset(); retro_run(); CHECK(same_menu(frame, shooting_menu));
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); auto auto_option = frame;
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(!same_menu(frame, auto_option));
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, auto_option));
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); // Normal matches the original automatic speed.
    press(RETRO_DEVICE_ID_JOYPAD_UP);
    auto auto_menu = frame; CHECK(!same_menu(auto_menu, shooting_menu));
    press(RETRO_DEVICE_ID_JOYPAD_START);
    auto auto_start = frame;
    buttons = 0; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_UP; retro_run();
    CHECK(pixel(265, 101) == 0x46a79a && pixel(265, 135) == 0xf3d66b);
    buttons2 = 0;
    keys[RETROK_p] = true; retro_run(); keys[RETROK_p] = false; retro_run();
    keys[RETROK_DOWN] = true; retro_run(); CHECK(pixel(75, 115) == 0x295166);
    keys[RETROK_DOWN] = false; retro_run(); keys[RETROK_RETURN] = true; retro_run();
    CHECK(frame == auto_start);
    keys[RETROK_RETURN] = false; retro_run();
    CHECK(pixel(265, 101) == 0x46a79a);
    for (int shot = 0; shot < 15; ++shot) {
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_Y; retro_run();
        buttons = 0; for (int i = 0; i < 100; ++i) retro_run();
    }
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
    CHECK(pixel(265, 101) == 0xf3d66b);
    buttons = 0; retro_run(); CHECK(pixel(265, 101) == 0x46a79a);
    exit_pause(); CHECK(same_menu(frame, auto_menu));
    retro_reset(); buttons = 0; retro_run(); CHECK(same_menu(frame, auto_menu));
    retro_unload_game(); CHECK(retro_load_game(&game)); retro_run(); CHECK(same_menu(frame, auto_menu));
    // Start the following gameplay checks with defaults and no save directory.
    retro_unload_game(); retro_deinit(); retro_init(); buttons = 0;
    CHECK(retro_load_game(&game)); retro_run(); CHECK(same_menu(frame, shooting_menu));
    // Finish with A held: the scoring input must not confirm the result.
    press(RETRO_DEVICE_ID_JOYPAD_START);
    for (int shot = 0; shot < 15; ++shot) {
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
        if (shot != 14) { buttons = 0; for (int i = 0; i < 100; ++i) retro_run(); }
    }
    for (int i = 0; i < 100; ++i) retro_run();
    CHECK(pixel(75, 99) == 0x295166);
    auto shooting_result = frame;
    retro_run(); CHECK(frame == shooting_result);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run(); CHECK(pixel(75, 115) == 0x295166);
    for (int i = 0; i < 23; ++i) retro_run();
    CHECK(pixel(75, 115) == 0x295166);
    retro_run(); CHECK(frame == shooting_result);
    buttons = 0; retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_START); CHECK(frame == shooting_result);
    keys[RETROK_p] = true; buttons = 0; retro_run(); keys[RETROK_p] = false; retro_run(); CHECK(frame == shooting_result);
    keys[RETROK_DOWN] = true; retro_run(); CHECK(pixel(75, 115) == 0x295166);
    keys[RETROK_DOWN] = false; retro_run(); keys[RETROK_UP] = true; retro_run(); CHECK(frame == shooting_result);
    keys[RETROK_UP] = false; retro_run(); keys[RETROK_RETURN] = true; retro_run();
    CHECK(pixel(75, 99) != 0x295166);
    keys[RETROK_RETURN] = false; retro_run();
    for (int shot = 0; shot < 15; ++shot) {
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_Y; retro_run();
        buttons = 0; for (int i = 0; i < 100; ++i) retro_run();
    }
    CHECK(pixel(75, 99) == 0x295166);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A); CHECK(same_menu(frame, shooting_menu));
    retro_unload_game();

    // Tennis B doubles reaches a winner with both paddles held away from play.
    buttons = buttons2 = 0; bytes[4] = 3; CHECK(retro_load_game(&game)); retro_run();
    auto result_tennis_menu = frame;
    press(RETRO_DEVICE_ID_JOYPAD_START);
    for (int round = 0; round < 2; ++round) {
        buttons = buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
        int remaining = 20000;
        do { retro_run(); } while (pixel(75, 99) != 0x295166 && --remaining);
        CHECK(remaining > 0);
        auto tennis_result = frame;
        retro_run(); CHECK(frame == tennis_result);
        buttons = buttons2 = 0; retro_run();
        press(RETRO_DEVICE_ID_JOYPAD_START); CHECK(frame == tennis_result);
        press(RETRO_DEVICE_ID_JOYPAD_DOWN); CHECK(pixel(75, 115) == 0x295166);
        if (round == 0) {
            press(RETRO_DEVICE_ID_JOYPAD_UP); CHECK(frame == tennis_result);
            press(RETRO_DEVICE_ID_JOYPAD_A); CHECK(pixel(75, 99) != 0x295166);
            CHECK(paddle(109, 101) && paddle(205, 101));
        } else {
            keys[RETROK_RETURN] = true; buttons = 0; retro_run(); keys[RETROK_RETURN] = false;
            CHECK(same_menu(frame, result_tennis_menu));
        }
    }
    retro_unload_game();

    return 0;
}
} // namespace core_test
