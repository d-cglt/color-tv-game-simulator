#include "core_test_harness.h"


#include <cstring>
#include <set>

namespace core_test {
int demo_preview_scenarios(ContentContext& context) {
    auto& bytes = context.bytes;
    auto& game = context.game;
    // The first Shooting preview frame fires; disabling only that event leaves later sounds intact.
    bytes[4] = 14; buttons = buttons2 = 0; keys.fill(false);
    shooting_fire_sound_value = "Off";
    CHECK(retro_load_game(&game));
    unsigned fire_audio = nonzero_audio;
    retro_run(); CHECK(nonzero_audio == fire_audio);
    for (int i = 0; i < 600; ++i) retro_run();
    CHECK(nonzero_audio > fire_audio);
    retro_unload_game();
    shooting_fire_sound_value = "On";
    CHECK(retro_load_game(&game));
    fire_audio = nonzero_audio;
    retro_run(); CHECK(nonzero_audio > fire_audio);
    retro_unload_game(); shooting_fire_sound_value.clear();
    // The declared default and an unavailable frontend value both enable demo sound.
    CHECK(demo_sounds_definition.find("; On|Off") != std::string::npos);
    for (unsigned id : {2u, 14u}) {
        bytes[4] = static_cast<unsigned char>(id);
        buttons = buttons2 = 0; axes = {{0, 0}}; keys.fill(false);
        CHECK(retro_load_game(&game));
        unsigned before = nonzero_audio;
        for (int i = 0; i < 600; ++i) retro_run();
        CHECK(nonzero_audio > before);
        set_demo_sounds("Off"); retro_run();
        before = nonzero_audio;
        for (int i = 0; i < 600; ++i) retro_run();
        CHECK(nonzero_audio == before);
        set_demo_sounds("On"); retro_run();
        before = nonzero_audio;
        for (int i = 0; i < 600; ++i) retro_run();
        CHECK(nonzero_audio > before);
        // Starting from a sounding preview must not carry its tone into gameplay.
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run(); // wake
        buttons = 0; retro_run();
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run(); // start
        before = nonzero_audio;
        for (int i = 0; i < 5; ++i) retro_run();
        CHECK(nonzero_audio == before);
        if (id == 14) {
            // Disabling demo sounds must not mute real shots, including live changes.
            set_demo_sounds("Off"); buttons = 0; retro_run();
            before = nonzero_audio;
            buttons = 1u << RETRO_DEVICE_ID_JOYPAD_Y; retro_run();
            CHECK(nonzero_audio > before);
            buttons = 0; for (int i = 0; i < 110; ++i) retro_run();
            set_demo_sounds("On"); retro_run();
            before = nonzero_audio;
            buttons = 1u << RETRO_DEVICE_ID_JOYPAD_Y; retro_run();
            CHECK(nonzero_audio > before);
        }
        retro_unload_game();
        demo_sounds_value.clear();
    }
    buttons = 0; CHECK(retro_load_game(nullptr));
    unsigned selector_sound = nonzero_audio;
    for (int i = 0; i < 600; ++i) retro_run();
    CHECK(nonzero_audio > selector_sound);
    set_demo_sounds("Off"); retro_run();
    selector_sound = nonzero_audio;
    for (int i = 0; i < 600; ++i) retro_run();
    CHECK(nonzero_audio == selector_sound);
    retro_unload_game();
    // Attract previews run independently of the real match and are silent when disabled.
    // Menu markers use the existing layout; motion is checked outside its panel.
    auto outside_changed = [](const std::vector<unsigned char>& a, const std::vector<unsigned char>& b) {
        for (int y = 21; y < 214; ++y)
            for (int x = 35; x < 285; ++x)
                if ((x < 48 || x >= 272 || y < 48 || y >= 204) &&
                    std::memcmp(a.data() + (y * 320 + x) * 4, b.data() + (y * 320 + x) * 4, 4) != 0) return true;
        return false;
    };
    for (unsigned id : {2u, 14u}) {
        bytes[4] = static_cast<unsigned char>(id);
        buttons = buttons2 = 0; axes = {{0, 0}}; keys.fill(false);
        CHECK(retro_load_game(&game)); retro_run();
        const unsigned preview_audio = nonzero_audio;
        auto preview_first = frame;
        bool animated = false;
        for (int i = 0; i < 590; ++i) {
            retro_run();
            animated |= outside_changed(preview_first, frame);
        }
        CHECK(animated && nonzero_audio == preview_audio);
        CHECK(demo_pixel(frame, 75, id == 14 ? 114 : 103) == 0x295166); // START is still selected before 10 seconds.
        for (int i = 0; i < 20; ++i) retro_run();
        CHECK(demo_pixel(frame, 75, id == 14 ? 114 : 103) != 0x295166);
        // The centered demo prompt remains visible while the menu is hidden.
        const char prompt[] = "DEMO MODE - PRESS ANY BUTTON TO SHOW MENU";
        const int prompt_x = (320 - (sizeof(prompt) - 1) * 6) / 2;
        const char* demo_top[] = {"11110", "11111", "10001", "01110"};
        for (int c = 0; c < 4; ++c) for (int x = 0; x < 5; ++x)
            CHECK((demo_pixel(frame, prompt_x + c * 6 + x, 4) == 0xffffff) == (demo_top[c][x] == '1'));
        // Scores are absent even while the menu is hidden.
        for (int y = 42; y < 75; ++y) for (int x : {74, 80, 86, 234, 240, 246})
            CHECK(demo_pixel(frame, x, y) != (id == 14 ? 0xffc9fa : 0xcdf0ff));
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
        CHECK(demo_pixel(frame, 75, id == 14 ? 114 : 103) == 0x295166); // Wake does not start.
        for (int i = 0; i < 30; ++i) retro_run();
        CHECK(demo_pixel(frame, 75, id == 14 ? 114 : 103) == 0x295166); // Nor does holding the wake button.
        buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
        auto after_preview_start = frame;
        retro_reset(); buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
        CHECK(frame == after_preview_start); // A real game starts from the same state.
        retro_unload_game();
    }
    buttons = 0; CHECK(retro_load_game(nullptr)); retro_run();
    const unsigned selector_audio = nonzero_audio;
    auto selector_preview = frame;
    bool selector_animated = false;
    for (int i = 0; i < 590; ++i) { retro_run(); selector_animated |= outside_changed(selector_preview, frame); }
    CHECK(selector_animated && nonzero_audio == selector_audio);
    CHECK(demo_pixel(frame, 55, 88) == 0x295166);
    for (int i = 0; i < 20; ++i) retro_run();
    CHECK(demo_pixel(frame, 55, 88) != 0x295166);
    CHECK(demo_pixel(frame, 37, 4) == 0xffffff); // DEMO prompt also appears in selector.
    keys[RETROK_RETURN] = true; retro_run();
    CHECK(demo_pixel(frame, 55, 88) == 0x295166); // Wake does not select a game.
    retro_run(); CHECK(demo_pixel(frame, 55, 88) == 0x295166);
    keys[RETROK_RETURN] = false; retro_run();
    keys[RETROK_RETURN] = true; retro_run(); CHECK(demo_pixel(frame, 75, 103) == 0x295166);
    keys.fill(false); retro_unload_game();

    // Exercise the actual frontend path: select zigzag, launch, then restart
    // from the pause menu. Inspect the rendered opponent, not engine state.
    retro_deinit(); retro_init();
    bytes[4] = 15; buttons = buttons2 = 0; keys.fill(false);
    CHECK(retro_load_game(&game)); retro_run();
    for (int i = 0; i < 6; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    for (int i = 0; i < 6; ++i) press(RETRO_DEVICE_ID_JOYPAD_UP);
    auto opponent_x = [] {
        for (int x = 54; x < 266; ++x)
            for (int y = 9; y < 180; ++y)
                if (pixel(x, y) == 0xf5ed48) return x;
        return -1;
    };
    press(RETRO_DEVICE_ID_JOYPAD_START);
    CHECK(opponent_x() >= 0);
    bool rerolled = false;
    for (int i = 0; i < 10; ++i) {
        const int before = opponent_x();
        press(RETRO_DEVICE_ID_JOYPAD_START); // pause
        press(RETRO_DEVICE_ID_JOYPAD_DOWN);  // restart
        press(RETRO_DEVICE_ID_JOYPAD_A);
        const int after = opponent_x();

        CHECK(before >= 0 && after >= 0);
        rerolled |= after < before - 10 || after > before + 10;
    }
    CHECK(rerolled);
    std::set<int> launches;
    for (int i = 0; i < 16; ++i) {
        retro_reset(); buttons = 0; retro_run();
        press(RETRO_DEVICE_ID_JOYPAD_START);
        const int x = opponent_x(); CHECK(x >= 0); launches.insert(x);
    }
    CHECK(launches.size() > 1);
    buttons = 0; retro_unload_game();
    launches.clear();
    for (int i = 0; i < 12; ++i) {
        retro_deinit(); retro_init(); buttons = 0;
        CHECK(retro_load_game(&game)); retro_run();
        for (int row = 0; row < 6; ++row) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        for (int row = 0; row < 6; ++row) press(RETRO_DEVICE_ID_JOYPAD_UP);
        press(RETRO_DEVICE_ID_JOYPAD_START);
        const int x = opponent_x(); CHECK(x >= 0); launches.insert(x);
        buttons = 0; retro_unload_game();
    }
    CHECK(launches.size() > 1);
    return 0;
}
} // namespace core_test
