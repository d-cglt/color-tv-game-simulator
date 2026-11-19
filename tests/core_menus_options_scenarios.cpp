#include "core_test_harness.h"

#include <algorithm>
#include <cstdio>

namespace core_test {
int menus_options_scenarios(ContentContext& context, const MenuSnapshot& snapshot) {
    const auto& option = snapshot.option;
    auto& bytes = context.bytes;
    auto& game = context.game;
    // Display positions map to stable content IDs, independently of their numeric order.
    const std::array<unsigned, 16> display_ids{{2, 3, 6, 7, 10, 11, 0, 1, 4, 5, 8, 9, 12, 13, 14, 15}};
    buttons = buttons2 = 0; axes = {{0, 0}}; keys.fill(false);
    CHECK(retro_load_game(nullptr)); retro_run();
    auto first_selector = frame;
    CHECK(at(first_selector, 160, 185) == 0xffffff && at(first_selector, 156, 187) == 0xffffff);
    CHECK(at(first_selector, 160, 191) == 0xffffff && at(first_selector, 156, 191) != 0xffffff);
    CHECK(at(first_selector, 55, 88) == 0x295166 && at(first_selector, 160, 77) != 0xffffff);
    keys[RETROK_p] = true; retro_run(); CHECK(same_menu(frame, first_selector));
    keys[RETROK_p] = false; retro_run();
    // Escape at the selector is a no-op; no frontend shutdown is requested.
    keys[RETROK_ESCAPE] = true; retro_run(); keys[RETROK_ESCAPE] = false; retro_run();
    CHECK(same_menu(frame, first_selector) && shutdowns == 0);
    for (unsigned id = 0; id < display_ids.size(); ++id) {
        auto selected = frame;
        const unsigned first = id > 2 ? std::min(id - 2, static_cast<unsigned>(display_ids.size() - 6)) : 0;
        CHECK(at(selected, 55, 88 + (id - first) * 16) == 0x295166);
        CHECK((at(selected, 160, 77) == 0xffffff) == (first != 0));
        CHECK((at(selected, 160, 191) == 0xffffff) == (first + 6 < display_ids.size()));
        for (unsigned row = 0; row < 6; ++row) {
            bool ink = false;
            for (unsigned y = 91 + row * 16; y < 98 + row * 16; ++y)
                for (unsigned x = 56; x < 264; ++x)
                    ink |= at(selected, x, y) == (first + row == id ? 0x70e0ff : 0xffffff);
            CHECK(ink);
        }
        retro_run(); CHECK(same_menu(frame, selected)); // The selector UI stays unchanged as the preview animates.
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
        auto launch = frame;
        buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
        auto started = frame;
        retro_reset(); retro_run(); CHECK(same_menu(frame, launch));
        buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_B; retro_run(); CHECK(same_menu(frame, selected));
        buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
        auto next = frame;
        retro_run(); CHECK(same_menu(frame, next)); // Held direction only moves once.
        buttons = 0; retro_run();
        retro_unload_game(); bytes[4] = static_cast<unsigned char>(display_ids[id]);
        CHECK(retro_load_game(&game)); retro_run();
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
        if (display_ids[id] == 15) {
            CHECK(at(frame, 160, 210) == 0xeb65e7 && at(started, 160, 210) == 0xeb65e7);
        } else CHECK(frame == started);
        retro_unload_game(); buttons = 0;
        CHECK(retro_load_game(nullptr)); retro_run();
        for (unsigned n = 0; n <= id; ++n) {
            buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
            buttons = 0; retro_run();
        }
        CHECK(same_menu(frame, next));
    }
    CHECK(same_menu(frame, first_selector));
    keys[RETROK_UP] = true; retro_run(); keys[RETROK_UP] = false; retro_run();
    auto last_selector = frame; CHECK(!same_menu(last_selector, first_selector));
    keys[RETROK_DOWN] = true; retro_run(); keys[RETROK_DOWN] = false; retro_run();
    CHECK(same_menu(frame, first_selector));
    keys[RETROK_UP] = true; retro_run(); keys[RETROK_UP] = false; retro_run();
    CHECK(same_menu(frame, last_selector));
    CHECK(at(last_selector, 160, 77) == 0xffffff && at(last_selector, 156, 77) != 0xffffff);
    CHECK(at(last_selector, 156, 81) == 0xffffff && at(last_selector, 160, 83) == 0xffffff);
    CHECK(at(last_selector, 55, 168) == 0x295166 && at(last_selector, 160, 191) != 0xffffff);

    axes[0] = 20000; retro_run(); CHECK(same_menu(frame, first_selector));
    retro_run(); CHECK(same_menu(frame, first_selector));
    axes[0] = 0; retro_run();
    keys[RETROK_DOWN] = true; retro_run(); keys[RETROK_DOWN] = false; retro_run();
    keys[RETROK_RETURN] = true; retro_run(); keys[RETROK_RETURN] = false; retro_run();
    auto selected_menu = frame;
    keys[RETROK_UP] = true; retro_run(); keys[RETROK_UP] = false; retro_run();
    auto return_selected = frame; CHECK(!same_menu(return_selected, selected_menu));
    keys[RETROK_RIGHT] = true; retro_run(); keys[RETROK_RIGHT] = false; retro_run(); CHECK(same_menu(frame, return_selected));
    keys[RETROK_DOWN] = true; retro_run(); keys[RETROK_DOWN] = false; retro_run(); CHECK(same_menu(frame, selected_menu));
    keys[RETROK_ESCAPE] = true; retro_run(); keys[RETROK_ESCAPE] = false; retro_run(); CHECK(!same_menu(frame, selected_menu));
    retro_reset(); retro_run(); CHECK(same_menu(frame, selected_menu));
    retro_unload_game();

    // Direction repeat: immediate, 24-frame delay, then every 6 frames.
    buttons = 0; keys.fill(false); axes = {{0, 0}};
    CHECK(retro_load_game(nullptr)); retro_run(); CHECK(same_menu(frame, first_selector));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; retro_run(); CHECK(same_menu(frame, last_selector));
    for (int i = 0; i < 23; ++i) retro_run();
    CHECK(same_menu(frame, last_selector));
    retro_run(); auto before_last = frame; CHECK(!same_menu(before_last, last_selector));
    for (int i = 0; i < 5; ++i) retro_run();
    CHECK(same_menu(frame, before_last));
    retro_run(); CHECK(!same_menu(frame, before_last));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run(); CHECK(same_menu(frame, before_last));
    buttons = 0; retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run(); CHECK(same_menu(frame, last_selector));
    retro_run(); CHECK(same_menu(frame, last_selector));
    buttons = 0; retro_run();
    axes[0] = 20000; retro_run(); CHECK(same_menu(frame, first_selector));
    for (int i = 0; i < 24; ++i) retro_run();
    CHECK(!same_menu(frame, first_selector));
    axes[0] = 0; retro_run();
    retro_unload_game();

    bytes[4] = 14; CHECK(retro_load_game(&game)); retro_run();
    keys[RETROK_DOWN] = true; retro_run(); CHECK(same_menu(frame, option));
    for (int i = 0; i < 24; ++i) retro_run();
    CHECK(!same_menu(frame, option));
    keys[RETROK_DOWN] = false; retro_run();
    retro_reset(); retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run(); auto moving = frame;
    retro_run(); CHECK(frame != moving); moving = frame;
    retro_run(); CHECK(frame != moving); // Gameplay continues moving every frame, without menu repeat timing.
    buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    auto repeat_pause = frame;
    for (int i = 0; i < 30; ++i) retro_run();
    CHECK(frame == repeat_pause); // A direction held when entering pause does not navigate it.
    buttons = 0; retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run(); CHECK(at(frame, 75, 115) == 0x295166);
    for (int i = 0; i < 24; ++i) retro_run();
    CHECK(at(frame, 75, 131) == 0x295166);
    for (int i = 0; i < 6; ++i) retro_run();
    CHECK(frame == repeat_pause);
    buttons = 0; retro_run();
    retro_unload_game();

    // Shooting settings survive reset, game switching, and reload even when the
    // save directory is unavailable; a fresh core starts with defaults.
    buttons = buttons2 = 0; keys.fill(false); axes = {{0, 0}};
    bytes[4] = 14; CHECK(retro_load_game(&game)); retro_run();
    const auto shooting_defaults = frame;
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); // TARGET MOVE defaults to Normal.
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); press(RETRO_DEVICE_ID_JOYPAD_LEFT); // Manual.
    std::vector<std::vector<unsigned char>> targets{frame};
    for (int i = 0; i < 4; ++i) {
        press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        for (const auto& value : targets) CHECK(!same_menu(frame, value));
        targets.push_back(frame);
    }
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, targets[0]));
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); // DIRECTION is disabled in Manual.
    const auto disabled_direction = frame;
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, disabled_direction));
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, disabled_direction));
    press(RETRO_DEVICE_ID_JOYPAD_UP); press(RETRO_DEVICE_ID_JOYPAD_RIGHT); // Slow.
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    std::vector<std::vector<unsigned char>> directions{frame}; // Down, Up, Random, Bounce.
    for (int i = 0; i < 3; ++i) {
        press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        for (const auto& value : directions) CHECK(!same_menu(frame, value));
        directions.push_back(frame);
    }
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, directions[0]));
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, directions[3]));
    retro_reset(); buttons = 0; retro_run();
    for (int i = 0; i < 2; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, directions[3]));
    press(RETRO_DEVICE_ID_JOYPAD_B); press(RETRO_DEVICE_ID_JOYPAD_UP); press(RETRO_DEVICE_ID_JOYPAD_A);
    press(RETRO_DEVICE_ID_JOYPAD_B); press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    for (int i = 0; i < 2; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, directions[3]));
    retro_unload_game(); buttons = 0; CHECK(retro_load_game(&game)); retro_run();
    for (int i = 0; i < 2; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, directions[3]));
    retro_unload_game(); retro_deinit(); retro_init(); buttons = 0;
    CHECK(retro_load_game(&game)); retro_run(); CHECK(same_menu(frame, shooting_defaults));
    retro_unload_game();

    // Match the rendered target against every valid once-per-match resolution.
    // Sampling both before and after a shot rules out re-rolling during play, without
    // requiring any particular random draw or assuming consecutive draws differ.
    struct TargetCandidate { int speed, direction, position; bool valid; };
    auto check_random_match = [&](bool manual) {
        std::vector<TargetCandidate> candidates;
        for (int speed : {6, 11, 16}) for (int direction : {-1, 1})
            if (!manual || (speed == 6 && direction == -1))
                candidates.push_back({speed, direction, 405, true});
        for (int tick = 1; tick <= 200; ++tick) {
            buttons = tick == 26 ? 1u << RETRO_DEVICE_ID_JOYPAD_Y : 0;
            buttons2 = manual && tick >= 5 && tick < 10 ? 1u << RETRO_DEVICE_ID_JOYPAD_DOWN : 0;
            retro_run();
            for (auto& c : candidates) {
                c.position += manual ? (buttons2 ? 24 : 0) : c.speed * c.direction;
                // The 32-pixel target wraps after its top travels from -8 to 216.
                if (c.position < -8 * 4) c.position += 225 * 4;
                if (c.position >= 217 * 4) c.position -= 225 * 4;
                // The shot may cross the target column or flash the screen here.
                if (tick > 25 && tick < 130) continue;
                const bool visible = (tick % 30) * 2 < 45;
                const int top = c.position / 4;
                for (int y = 0; y < 240; ++y) {
                    const bool target_ink = pixel(265, y) == 0xf3d66b;
                    if (target_ink != (visible && y >= 20 && y < 220 && y >= top && y < top + 32)) { c.valid = false; break; }
                }
            }
        }
        buttons = buttons2 = 0;
        for (const auto& c : candidates) if (c.valid) return true;
        return false;
    };
    for (bool manual : {false, true}) {
        retro_deinit(); retro_init();
        buttons = buttons2 = 0; keys.fill(false); axes = {{0, 0}};
        bytes[4] = 14; CHECK(retro_load_game(&game)); retro_run();
        press(RETRO_DEVICE_ID_JOYPAD_DOWN); // TARGET MOVE
        if (manual) { press(RETRO_DEVICE_ID_JOYPAD_LEFT); press(RETRO_DEVICE_ID_JOYPAD_LEFT); }
        else { press(RETRO_DEVICE_ID_JOYPAD_RIGHT); press(RETRO_DEVICE_ID_JOYPAD_RIGHT); } // Random
        press(RETRO_DEVICE_ID_JOYPAD_DOWN); // DIRECTION
        if (!manual) { press(RETRO_DEVICE_ID_JOYPAD_RIGHT); press(RETRO_DEVICE_ID_JOYPAD_RIGHT); } // Random
        for (int i = 0; i < 2; ++i) press(RETRO_DEVICE_ID_JOYPAD_UP);
        press(RETRO_DEVICE_ID_JOYPAD_START);
        CHECK(check_random_match(manual));
        // One shot was already fired by the trajectory check; fourteen more
        // must finish the default 15-shot match, including after result restart.
        for (int round = 0; round < 2; ++round) {
            for (int shot = 0; shot < 14; ++shot) {
                press(RETRO_DEVICE_ID_JOYPAD_Y);
                buttons = 0; for (int i = 0; i < 100; ++i) retro_run();
                CHECK((pixel(75, 99) == 0x295166) == (shot == 13));
            }
            press(RETRO_DEVICE_ID_JOYPAD_A); // Result restart resolves a new match.
            CHECK(check_random_match(manual));
        }
        press(RETRO_DEVICE_ID_JOYPAD_START); // Pause restart resolves another match.
        press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
        CHECK(check_random_match(manual));
        retro_unload_game();
    }

    // Rally titles expose two sizes; shooting has an independent target size.
    for (unsigned id = 0; id < 15; ++id) {
        retro_deinit(); retro_init();
        buttons = buttons2 = 0; keys.fill(false); axes = {{0, 0}};
        bytes[4] = static_cast<unsigned char>(id);
        CHECK(retro_load_game(&game)); retro_run();
        auto defaults = frame;
        const unsigned count = 7;
        for (unsigned row = 0; row < count; ++row) {
            const unsigned choice = (row + 1) % count;
            CHECK(at(frame, 75, 88 + choice * 15) == 0x295166);
            CHECK(at(frame, 160, 87) != 0xffffff && at(frame, 156, 181) != 0xffffff);
            press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        }
        CHECK(same_menu(frame, defaults));
        press(RETRO_DEVICE_ID_JOYPAD_DOWN); // Mode or target.
        if (id == 14) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        press(RETRO_DEVICE_ID_JOYPAD_DOWN); // Rally speed or Shooting target size.
        auto low = frame; press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(!same_menu(frame, low));
        press(RETRO_DEVICE_ID_JOYPAD_DOWN); // P1 size or Shooting shots speed.
        if (id != 14) CHECK(at(frame, 117, 151) == 0x70e0ff); // Z in P1 SIZE.
        auto large_left = frame; press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(!same_menu(frame, large_left));
        if (id != 14) {
            press(RETRO_DEVICE_ID_JOYPAD_DOWN); // P2 size.
            auto large_right = frame; press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(!same_menu(frame, large_right));
        } else {
            auto small = frame; press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(!same_menu(frame, small));
            press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, large_left));
            press(RETRO_DEVICE_ID_JOYPAD_RIGHT); // High shots speed.
        }

        retro_reset(); retro_run();
        CHECK(!same_menu(frame, defaults)); // Switches survive reset.
        press(RETRO_DEVICE_ID_JOYPAD_START);
        const std::uint32_t paddle_color = id == 14 ? 0xf3d66b : id >= 12 ? 0x9b8cff : id >= 8 ? 0xa6e6ff : id >= 4 ? 0xff9fff : 0xd0ecb2;
        CHECK(at(frame, 49, 117) == paddle_color);
        CHECK(at(frame, 49, 109) == paddle_color);
        CHECK((at(frame, 49, 103) == paddle_color) == (id == 14));
        press(RETRO_DEVICE_ID_JOYPAD_START); // Pause, then restart.
        press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
        CHECK(at(frame, 49, 109) == paddle_color);
        CHECK((at(frame, 49, 103) == paddle_color) == (id == 14));
        retro_unload_game(); buttons = 0;
        CHECK(retro_load_game(&game)); retro_run();
        for (int i = 0; i < (id == 14 ? 3 : 2); ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        CHECK(!same_menu(frame, low));
        retro_unload_game();
    }
    // Rally paddle preferences do not affect the independent Shooting sizes.
    for (unsigned settings = 0; settings < 4; ++settings) {
        const bool target_small = (settings & 1) != 0;
        const bool shots_high = (settings & 2) != 0;
        retro_deinit(); retro_init();
        buttons = buttons2 = 0; keys.fill(false); axes = {{0, 0}};
        bytes[4] = 0; CHECK(retro_load_game(&game)); retro_run();
        for (int i = 0; i < 3; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        retro_unload_game();
        bytes[4] = 14; CHECK(retro_load_game(&game)); retro_run();
        for (int i = 0; i < 3; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN); // TARGET SIZE
        if (target_small) press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        press(RETRO_DEVICE_ID_JOYPAD_DOWN); // SHOTS SPEED
        if (shots_high) press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        for (int i = 0; i < 4; ++i) press(RETRO_DEVICE_ID_JOYPAD_UP);
        press(RETRO_DEVICE_ID_JOYPAD_START);
        for (int side = 0; side < 2; ++side) {
            const int x = side ? 265 : 49;
            const int height = (side ? target_small : !shots_high) ? 16 : 32;
            int top = 0;
            while (top < 240 && at(frame, x, top) != 0xf3d66b) ++top;
            CHECK(top > 0 && top + height < 240);
            for (int y = top - 1; y <= top + height; ++y)
                CHECK((at(frame, x, y) == 0xf3d66b) == (y >= top && y < top + height));
        }
        retro_unload_game();
    }
    // Both rally size switches independently control the complete paddle rectangles.
    for (unsigned id = 0; id < 14; ++id) {
        for (unsigned sizes = 0; sizes < 4; ++sizes) {
            retro_deinit(); retro_init();
            buttons = buttons2 = 0; keys.fill(false); axes = {{0, 0}};
            bytes[4] = static_cast<unsigned char>(id);
            CHECK(retro_load_game(&game)); retro_run();
            for (int i = 0; i < (id == 14 ? 4 : 3); ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
            if (sizes & 1) press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
            press(RETRO_DEVICE_ID_JOYPAD_DOWN);
            if (sizes & 2) press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
            for (int i = 0; i < (id == 14 ? 5 : 4); ++i) press(RETRO_DEVICE_ID_JOYPAD_UP);
            press(RETRO_DEVICE_ID_JOYPAD_START);
            for (int side = 0; side < 2; ++side) {
                const int x = side ? 265 : 49;
                const std::uint32_t color = id == 14 ? 0xf3d66b : id >= 12 ? 0x9b8cff :
                    id >= 8 ? 0xa6e6ff : id >= 4 ? (side ? 0xd5e87c : 0xff9fff) : 0xd0ecb2;
                const int height = sizes & (1u << side) ? 16 : 32;
                int top = 0;
                while (top < 240 && at(frame, x, top) != color) ++top;
                if (!(top > 0 && top + height < 240))
                    std::fprintf(stderr, "paddle missing: id=%u sizes=%u side=%d top=%d\n", id, sizes, side, top);
                CHECK(top > 0 && top + height < 240);
                for (int y = top - 1; y <= top + height; ++y)
                    for (int dx = -1; dx <= 6; ++dx) {
                        const bool expected = y >= top && y < top + height && dx >= 0 && dx < 6;
                        if ((at(frame, x + dx, y) == color) != expected)
                            std::fprintf(stderr, "paddle mismatch: id=%u sizes=%u side=%d top=%d y=%d dx=%d\n", id, sizes, side, top, y, dx);
                        CHECK((at(frame, x + dx, y) == color) == expected);
                    }
            }
            if (id == 1 || id == 3 || id == 9 || id == 11 || id == 13) {
                const std::uint32_t color = id == 13 ? 0x9b8cff : id >= 8 ? 0xa6e6ff : 0xd0ecb2;
                const std::uint32_t background = id == 13 ? 0x706a6a : id >= 8 ? 0x6d70f9 : 0xc07645;
                for (int side = 0; side < 2; ++side) {
                    const int x = side ? 205 : 109;
                    const int height = sizes & (1u << side) ? 16 : 32;
                    int top = 0;
                    while (top < 240 && at(frame, x, top) != color) ++top;
                    CHECK(top > 0 && top + height < 240);
                    for (int y = top - 1; y <= top + height; ++y)
                        for (int dx = -1; dx <= 6; ++dx) {
                            const bool inside = y >= top && y < top + height && dx >= 0 && dx < 6;
                            CHECK(at(frame, x + dx, y) == (inside ? color : background));
                        }
                }
            }
            retro_unload_game();
        }
    }
    // Competitive score cycles through all targets and survives reset, restart and game switching.
    retro_deinit(); retro_init();
    buttons = buttons2 = 0; bytes[4] = 2; CHECK(retro_load_game(&game)); retro_run();
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    std::vector<std::vector<unsigned char>> scores{frame};
    for (int i = 0; i < 5; ++i) {
        press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        for (const auto& score : scores) CHECK(!same_menu(frame, score));
        scores.push_back(frame);
    }
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, scores[0]));
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, scores[5])); // 10
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, scores[4])); // 5
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, scores[3])); // 30
    retro_reset(); retro_run();
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, scores[3]));
    press(RETRO_DEVICE_ID_JOYPAD_B); // selector
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_UP);
    press(RETRO_DEVICE_ID_JOYPAD_A);
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, scores[3]));
    press(RETRO_DEVICE_ID_JOYPAD_UP); // Select AI without disturbing target.
    for (int i = 0; i < 3; ++i) press(RETRO_DEVICE_ID_JOYPAD_UP);
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    press(RETRO_DEVICE_ID_JOYPAD_UP); press(RETRO_DEVICE_ID_JOYPAD_START);
    press(RETRO_DEVICE_ID_JOYPAD_START); press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    retro_reset(); retro_run();
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(!same_menu(frame, scores[0])); // AI mode remains configured too.
    for (int i = 0; i < 4; ++i) press(RETRO_DEVICE_ID_JOYPAD_UP);
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); // Restore two-player mode.
    for (int i = 0; i < 4; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, scores[3]));
    auto retained_score = frame;
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_UP);
    press(RETRO_DEVICE_ID_JOYPAD_START);
    for (int round = 0; round < 2; ++round) {
        buttons = buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
        int remaining = 30000;
        do { retro_run(); } while (pixel(75, 99) != 0x295166 && --remaining);
        CHECK(remaining > 0);
        // The winner's two digits are 30, not the former fixed 15.
        CHECK((pixel(55, 44) == 0xcdf0ff && pixel(85, 44) == 0xcdf0ff) ||
              (pixel(217, 44) == 0xcdf0ff && pixel(247, 44) == 0xcdf0ff));
        buttons = buttons2 = 0; retro_run();
        press(RETRO_DEVICE_ID_JOYPAD_A); // Result restart retains 30.
    }
    retro_unload_game(); buttons = 0; CHECK(retro_load_game(&game)); retro_run();
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, retained_score) && !same_menu(frame, scores[0]));
    retro_unload_game();

    // SHOTS cycles independently of SCORE and survives switches, reset, and both restarts.
    retro_deinit(); retro_init();
    buttons = buttons2 = 0; bytes[4] = 14; CHECK(retro_load_game(&game)); retro_run();
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    std::vector<std::vector<unsigned char>> shots{frame};
    for (int i = 0; i < 5; ++i) {
        press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        for (const auto& value : shots) CHECK(!same_menu(frame, value));
        shots.push_back(frame);
    }
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, shots[0]));
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, shots[5]));
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, shots[4])); // 5
    retro_reset(); buttons = 0; retro_run();
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, shots[4]));
    press(RETRO_DEVICE_ID_JOYPAD_B); press(RETRO_DEVICE_ID_JOYPAD_UP); press(RETRO_DEVICE_ID_JOYPAD_A);
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    auto competitive_default = frame;
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); auto competitive_changed = frame;
    press(RETRO_DEVICE_ID_JOYPAD_B); press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, shots[4]));
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_UP);
    press(RETRO_DEVICE_ID_JOYPAD_START);
    press(RETRO_DEVICE_ID_JOYPAD_START); press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    for (int round = 0; round < 2; ++round) {
        for (int shot = 0; shot < 5; ++shot) {
            press(RETRO_DEVICE_ID_JOYPAD_Y);
            buttons = 0; for (int i = 0; i < 100; ++i) retro_run();
            CHECK((pixel(75, 99) == 0x295166) == (shot == 4));
        }
        if (round == 0) press(RETRO_DEVICE_ID_JOYPAD_A);
    }
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, shots[4]));
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, shots[3])); // 30
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_UP);
    press(RETRO_DEVICE_ID_JOYPAD_START);
    for (int shot = 0; shot < 30; ++shot) {
        press(RETRO_DEVICE_ID_JOYPAD_Y);
        buttons = 0; for (int i = 0; i < 100; ++i) retro_run();
        CHECK((pixel(75, 99) == 0x295166) == (shot == 29));
    }
    CHECK(pixel(55, 44) == 0xffc9fa && pixel(85, 44) == 0xffc9fa);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    press(RETRO_DEVICE_ID_JOYPAD_B); press(RETRO_DEVICE_ID_JOYPAD_UP); press(RETRO_DEVICE_ID_JOYPAD_A);
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, competitive_changed) && !same_menu(frame, competitive_default));
    retro_unload_game(); buttons = 0; CHECK(retro_load_game(&game)); retro_run();
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, shots[3]));
    retro_unload_game();

    // P2 modes are available for every tennis-family title and cycle in both directions.
    for (unsigned id = 0; id < 14; ++id) {
        retro_deinit(); retro_init();
        buttons = buttons2 = 0; axes = {{0, 0}};
        bytes[4] = static_cast<unsigned char>(id);
        CHECK(retro_load_game(&game)); retro_run();
        press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        CHECK(at(frame, 75, 118) == 0x295166 && at(frame, 91, 124) == 0x70e0ff && at(frame, 220, 124) == 0x70e0ff);
        std::vector<std::vector<unsigned char>> modes{frame};
        for (int i = 0; i < 3; ++i) {
            press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
            for (const auto& mode : modes) CHECK(!same_menu(frame, mode));
            modes.push_back(frame);
        }
        press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, modes[0]));
        press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, modes[3]));
        press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, modes[0]));
        retro_unload_game();
    }

    // AI ignores both digital and analog P2 input, while classic responds to it.
    // Replaying after reset must reproduce the same frame sequence for every mode.
    for (unsigned mode = 0; mode < 4; ++mode) {
        retro_deinit(); retro_init();
        buttons = buttons2 = 0; axes = {{0, 0}}; bytes[4] = 2;
        CHECK(retro_load_game(&game)); retro_run();
        press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        for (unsigned i = 0; i < mode; ++i) press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        retro_reset(); buttons = 0; retro_run();
        press(RETRO_DEVICE_ID_JOYPAD_START);
        std::vector<std::vector<unsigned char>> baseline;
        buttons = 0;
        for (int i = 0; i < 180; ++i) { retro_run(); baseline.push_back(frame); }
        retro_reset(); buttons = 0; retro_run();
        press(RETRO_DEVICE_ID_JOYPAD_START);
        buttons = 0; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; axes[1] = -20000;
        bool differs = false;
        for (int i = 0; i < 180; ++i) {
            retro_run();
            if (frame != baseline[i]) differs = true;
        }
        CHECK(differs == (mode == 0));
        if (mode != 0) {
            buttons = buttons2 = 0; axes[1] = 0;
            retro_reset(); retro_run(); press(RETRO_DEVICE_ID_JOYPAD_START);
            buttons = 0;
            for (int i = 0; i < 60; ++i) { retro_run(); CHECK(frame == baseline[i]); }
            buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
            auto paused_frame = frame;
            buttons = 0;
            // Longer than the initial Hard phase and a full Normal/Easy loop.
            // Paused frames must not advance the clock.
            for (int i = 0; i < 3660; ++i) { retro_run(); CHECK(frame == paused_frame); }
            buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
            buttons = 0;
            for (int i = 60; i < 180; ++i) { retro_run(); CHECK(frame == baseline[i]); }
            // Pause-menu restart resets the AI sequence as well as the match.
            buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
            press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
            buttons = 0;
            for (int i = 0; i < 180; ++i) { retro_run(); CHECK(frame == baseline[i]); }
        }
        retro_unload_game();
    }

    // Without a save directory, selections remain associated with their content
    // IDs across game changes, reset and reload for the lifetime of the core.
    retro_deinit(); retro_init();
    buttons = buttons2 = 0; axes = {{0, 0}};
    CHECK(retro_load_game(nullptr)); retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_A);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    auto classic_p2 = frame;
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); auto easy_p2 = frame;
    press(RETRO_DEVICE_ID_JOYPAD_B);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    auto other_classic = frame;
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(!same_menu(frame, other_classic));
    press(RETRO_DEVICE_ID_JOYPAD_B);
    press(RETRO_DEVICE_ID_JOYPAD_UP); press(RETRO_DEVICE_ID_JOYPAD_A);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, easy_p2));
    retro_reset(); buttons = 0; retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, easy_p2));
    retro_unload_game(); buttons = 0;
    CHECK(retro_load_game(nullptr)); retro_run(); press(RETRO_DEVICE_ID_JOYPAD_A);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, easy_p2) && !same_menu(frame, classic_p2));
    retro_unload_game();
    retro_deinit();

    return 0;
}
} // namespace core_test
