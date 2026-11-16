#include "core_test_harness.h"

#include <algorithm>

#include <cstring>
#include <utility>

namespace core_test {
int video_rumble_scenarios(ContentContext& context) {
    auto& bytes = context.bytes;
    auto& game = context.game;
    // Compare deterministic gameplay frame hashes after selecting a game. Changing
    // the option before launch also exercises construction of the selected engine.
    start_game_value = "First"; demo_rotation_value = "Off"; demo_sounds_value = "Off";
    save_available = false; set_color("On");
    auto rebound_replay = [&](unsigned id, const char* initial, bool selector, bool switch_live) {
        set_rebound(initial); retro_init(); buttons = buttons2 = 0; axes = {{0, 0}}; keys.fill(false);
        bytes[4] = static_cast<unsigned char>(id);
        if (!retro_load_game(selector ? nullptr : &game)) return std::vector<std::uint64_t>{};
        retro_run();
        if (selector) {
            for (unsigned i = 0; display_ids[i] != id; ++i) {
                buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run(); buttons = 0; retro_run();
            }
            buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run(); buttons = 0; retro_run();
        }
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
        buttons = 0;
        std::vector<std::uint64_t> result;
        for (int i = 0; i < 700; ++i) {
            if (switch_live && i == 10) set_rebound("Paddle movement");
            buttons = 1u << ((i % 2) ? RETRO_DEVICE_ID_JOYPAD_UP : RETRO_DEVICE_ID_JOYPAD_DOWN);
            // The first serve reaches the right paddle near y=226 after a wall
            // bounce. Position P2 there during the serve delay, then keep it
            // moving so the two rebound modes produce different trajectories.
            buttons2 = 1u << (i < 27 || i % 2 == 0 ? RETRO_DEVICE_ID_JOYPAD_DOWN : RETRO_DEVICE_ID_JOYPAD_UP);
            retro_run();
            std::uint64_t hash = 14695981039346656037ull;
            for (auto byte : frame) { hash ^= byte; hash *= 1099511628211ull; }
            result.push_back(hash);
        }
        retro_unload_game(); retro_deinit(); return result;
    };
    const auto hit = rebound_replay(2, "Hit position", false, false);
    CHECK(hit.size() == 700);
    CHECK(rebound_replay(2, "", false, false) == hit);
    const auto movement = rebound_replay(2, "Paddle movement", false, false);
    CHECK(movement.size() == hit.size() && movement != hit);
    CHECK(rebound_replay(2, "Hit position", false, true) == movement);
    CHECK(rebound_replay(2, "Paddle movement", true, false) == movement);
    const auto shooting_hit = rebound_replay(14, "Hit position", false, false);
    CHECK(shooting_hit.size() == 700);
    CHECK(rebound_replay(14, "Paddle movement", false, false) == shooting_hit);
    CHECK(rebound_replay(14, "Hit position", false, true) == shooting_hit);
    set_rebound("Hit position"); buttons = buttons2 = 0;

    // Replay identical inputs with live color changes: output conversion must not
    // change the selected game or the progression of the underlying match.
    CHECK(color_definition.find("; On|Off") != std::string::npos);
    start_game_value = "First"; demo_rotation_value = "Off"; demo_sounds_value = "Off";
    auto grayscale = [](const std::vector<unsigned char>& image) {
        if (image.size() != 320u * 240u * 4u) return false;
        for (size_t i = 0; i < image.size(); i += 4) {
            std::uint32_t pixel; std::memcpy(&pixel, image.data() + i, sizeof pixel);
            if (((pixel >> 16) & 255) != ((pixel >> 8) & 255) ||
                ((pixel >> 8) & 255) != (pixel & 255)) return false;
        }
        return true;
    };
    const std::array<unsigned, 9> sequence = {{
        0, 0, 0, 1u << RETRO_DEVICE_ID_JOYPAD_START, 0,
        1u << RETRO_DEVICE_ID_JOYPAD_START, 0, 0, 0
    }};
    std::vector<std::vector<unsigned char>> color_frames;
    set_color("On"); retro_init(); buttons = buttons2 = 0; axes = {{0, 0}}; keys.fill(false);
    CHECK(retro_load_game(nullptr));
    for (auto input_buttons : sequence) { buttons = input_buttons; retro_run(); color_frames.push_back(frame); }
    retro_unload_game(); retro_deinit();
    CHECK(!grayscale(color_frames[0]) && !grayscale(color_frames[3]) && !grayscale(color_frames[6]));

    retro_init(); buttons = 0; CHECK(retro_load_game(nullptr));
    for (size_t i = 0; i < sequence.size(); ++i) {
        set_color((i == 1 || i == 3 || i == 4 || i == 6 || i == 7) ? "Off" : "On");
        buttons = sequence[i]; retro_run();
        if (color_value == "Off") CHECK(grayscale(frame));
        else CHECK(frame == color_frames[i]);
    }
    retro_unload_game(); retro_deinit();

    // A frontend that cannot supply the option must behave like the On default.
    color_value.clear(); retro_init(); buttons = 0; CHECK(retro_load_game(nullptr));
    for (size_t i = 0; i < sequence.size(); ++i) {
        buttons = sequence[i]; retro_run(); CHECK(frame == color_frames[i]);
    }
    retro_unload_game(); retro_deinit();

    // Starting with Off must also convert a direct-content launch and gameplay.
    bytes[4] = 14; set_color("On"); retro_init(); buttons = 0;
    CHECK(retro_load_game(&game)); color_frames.clear();
    for (auto input_buttons : sequence) { buttons = input_buttons; retro_run(); color_frames.push_back(frame); }
    retro_unload_game(); retro_deinit();
    CHECK(!grayscale(color_frames[0]) && !grayscale(color_frames[4]));
    set_color("Off"); retro_init(); buttons = 0; CHECK(retro_load_game(&game));
    for (size_t i = 0; i < sequence.size(); ++i) {
        if (i == sequence.size() - 1) set_color("On");
        buttons = sequence[i]; retro_run();
        if (color_value == "Off") CHECK(grayscale(frame));
        else CHECK(frame == color_frames[i]);
    }
    retro_unload_game(); retro_deinit();

    // Monochrome menus outline the selected row without changing color-mode rendering.
    set_color("Off"); buttons = 0; retro_init(); CHECK(retro_load_game(nullptr)); retro_run();
    CHECK(demo_pixel(frame, 55, 88) == 0xffffff && demo_pixel(frame, 264, 100) == 0xffffff);
    for (unsigned id = 1; id < display_ids.size(); ++id) {
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
        buttons = 0; retro_run();
        const unsigned first = id > 2 ? std::min(id - 2, static_cast<unsigned>(display_ids.size() - 6)) : 0;
        const unsigned y = 88 + (id - first) * 16;
        CHECK(demo_pixel(frame, 55, y) == 0xffffff && demo_pixel(frame, 264, y + 12) == 0xffffff);
    }
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    buttons = 0; retro_run();
    CHECK(demo_pixel(frame, 55, 88) == 0xffffff && demo_pixel(frame, 264, 100) == 0xffffff);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
    CHECK(demo_pixel(frame, 53, 103) == 0xffffff && demo_pixel(frame, 265, 116) == 0xffffff);
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(demo_pixel(frame, 75, 99) == 0xffffff && demo_pixel(frame, 244, 112) == 0xffffff);
    retro_unload_game(); retro_deinit();

    // Replay the same selector and direct-content inputs with live RF changes.
    // Comparing the unfiltered frames after switching back also checks that the
    // filter never changes game selection or simulation state.
    set_color("On"); start_game_value = "First"; demo_rotation_value = "Off";
    demo_sounds_value = "Off"; save_available = false;
    bytes[4] = 14;
    for (bool direct : {false, true}) {
        std::vector<std::vector<unsigned char>> baseline;
        set_rf_filter("Off"); retro_init(); buttons = buttons2 = 0;
        axes = {{0, 0}}; keys.fill(false);
        CHECK(retro_load_game(direct ? &game : nullptr));
        for (auto input_buttons : sequence) {
            buttons = input_buttons; const unsigned before = frames; retro_run();
            CHECK(frames == before + 1 && frame.size() == 320u * 240u * 4u);
            baseline.push_back(frame);
        }
        retro_unload_game(); retro_deinit();

        // An unavailable value must be identical to explicit Off, frame for frame.
        rf_filter_value.clear(); retro_init(); buttons = 0;
        CHECK(retro_load_game(direct ? &game : nullptr));
        for (size_t i = 0; i < sequence.size(); ++i) {
            buttons = sequence[i]; const unsigned before = frames; retro_run();
            CHECK(frames == before + 1 && frame == baseline[i]);
        }
        retro_unload_game(); retro_deinit();

        retro_init(); buttons = 0; CHECK(retro_load_game(direct ? &game : nullptr));
        for (size_t i = 0; i < sequence.size(); ++i) {
            const bool filtered = i == 0 || i == 1 || i == 3 || i == 4 || i == 6 || i == 7;
            set_rf_filter(filtered ? "On" : "Off");
            buttons = sequence[i]; const unsigned before = frames; retro_run();
            CHECK(frames == before + 1 && frame.size() == baseline[i].size());
            if (filtered) CHECK(frame != baseline[i]);
            else CHECK(frame == baseline[i]);
        }
        retro_unload_game(); retro_deinit();

        // RF must not reintroduce color when monochrome output is requested.
        set_color("Off"); set_rf_filter("On"); retro_init(); buttons = 0;
        CHECK(retro_load_game(direct ? &game : nullptr));
        for (auto input_buttons : sequence) {
            buttons = input_buttons; const unsigned before = frames; retro_run();
            CHECK(frames == before + 1 && grayscale(frame));
        }
        retro_unload_game(); retro_deinit(); set_color("On");
    }
    CHECK(rf_filter_definition.find("; Off|On") != std::string::npos);

    // Capture the existing clean RF output with the quality option unavailable.
    const std::array<const char*, 11> qualities = {{
        "100%", "90%", "80%", "70%", "60%", "50%",
        "40%", "30%", "20%", "10%", "0%"
    }};
    set_rf_filter("On"); rf_quality_value.clear(); retro_init(); buttons = 0;
    CHECK(retro_load_game(&game));
    std::vector<std::vector<unsigned char>> clean_rf;
    for (auto input_buttons : sequence) {
        buttons = input_buttons; retro_run(); clean_rf.push_back(frame);
    }
    retro_unload_game(); retro_deinit();

    // Identical inputs isolate the video effect; aggregate RGB error over several
    // frames rather than depending on a particular random pixel or noise pattern.
    std::array<unsigned long long, 11> errors{};
    for (size_t q = 0; q < qualities.size(); ++q) {
        set_rf_quality(qualities[q]); retro_init(); buttons = 0;
        CHECK(retro_load_game(&game));
        bool noise_changes = false;
        std::vector<unsigned char> previous_noise;
        for (size_t i = 0; i < sequence.size(); ++i) {
            buttons = sequence[i]; retro_run();
            CHECK(frame.size() == clean_rf[i].size());
            std::vector<unsigned char> noise;
            for (size_t p = 0; p < frame.size(); p += 4) {
                std::uint32_t actual, clean;
                std::memcpy(&actual, frame.data() + p, 4);
                std::memcpy(&clean, clean_rf[i].data() + p, 4);
                for (unsigned shift : {0u, 8u, 16u}) {
                    const int delta = int((actual >> shift) & 255) - int((clean >> shift) & 255);
                    errors[q] += delta < 0 ? -delta : delta;
                    noise.push_back(static_cast<unsigned char>(delta));
                }
            }
            if (!previous_noise.empty() && noise != previous_noise) noise_changes = true;
            previous_noise = std::move(noise);
        }
        retro_unload_game(); retro_deinit();
        if (q == 0) CHECK(errors[q] == 0);
        else CHECK(errors[q] > 0 && noise_changes);
    }
    CHECK(errors.back() > errors[1]);

    // Quality changes cannot affect unfiltered output or gameplay progression.
    set_rf_filter("Off"); set_rf_quality("100%"); retro_init(); buttons = 0;
    CHECK(retro_load_game(&game));
    std::vector<std::vector<unsigned char>> unfiltered;
    for (auto input_buttons : sequence) { buttons = input_buttons; retro_run(); unfiltered.push_back(frame); }
    retro_unload_game(); retro_deinit();
    set_rf_quality("0%"); retro_init(); buttons = 0; CHECK(retro_load_game(&game));
    for (size_t i = 0; i < sequence.size(); ++i) {
        buttons = sequence[i]; retro_run(); CHECK(frame == unfiltered[i]);
    }
    retro_unload_game(); retro_deinit();

    set_rf_filter("On"); set_rf_quality("0%"); retro_init(); buttons = 0;
    CHECK(retro_load_game(&game));
    for (size_t i = 0; i < sequence.size(); ++i) {
        if (i == sequence.size() - 1) set_rf_filter("Off");
        buttons = sequence[i]; retro_run();
        if (i == sequence.size() - 1) CHECK(frame == unfiltered[i]);
    }
    retro_unload_game(); retro_deinit();
    set_color("Off"); set_rf_filter("On"); retro_init(); buttons = 0;
    CHECK(retro_load_game(&game));
    for (auto input_buttons : sequence) { buttons = input_buttons; retro_run(); CHECK(grayscale(frame)); }
    retro_unload_game(); retro_deinit();

    CHECK(rf_quality_definition.find("; 100%|0%|10%|20%|30%|40%|50%|60%|70%|80%|90%") != std::string::npos);

    // Exercise rumble through the frontend interface rather than engine event bits.
    set_color("On"); set_rf_filter("Off");
    bytes[4] = 14; buttons = buttons2 = 0; keys.fill(false); axes = {{0, 0}};
    rumble_available = true; rumble_calls.clear(); vibration_value.clear();
    retro_init(); CHECK(retro_load_game(&game)); retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_Y; retro_run();
    for (const auto& call : rumble_calls) CHECK(call.strength == 0);
    retro_unload_game(); retro_deinit();

    set_vibration("On"); rumble_queries = 0; rumble_calls.clear(); buttons = 0;
    retro_init(); CHECK(retro_load_game(&game));
    for (int i = 0; i < 1200; ++i) retro_run();
    CHECK(rumble_queries > 0);
    for (const auto& call : rumble_calls) CHECK(call.strength == 0);
    retro_unload_game(); retro_deinit();

    // Starting each ball game and completing its initial automatic serve is silent
    // on both controllers, before the first ball movement or paddle collision.
    for (unsigned id = 0; id < 14; ++id) {
        bytes[4] = static_cast<unsigned char>(id);
        set_vibration("On"); buttons = buttons2 = 0; rumble_calls.clear();
        retro_init(); CHECK(retro_load_game(&game)); retro_run();
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
        buttons = 0;
        for (int i = 0; i < 130; ++i) retro_run();
        CHECK(rumble_calls.empty());
        retro_unload_game(); retro_deinit();
    }
    bytes[4] = 14;

    // Start a fresh shot for each interruption so a previous stop cannot mask it.
    for (int interruption = 0; interruption < 4; ++interruption) {
        set_vibration("On"); shooting_fire_sound_value = "Off"; buttons = 0; rumble_calls.clear();
        retro_init(); CHECK(retro_load_game(&game)); retro_run();
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
        buttons = 0; retro_run(); rumble_calls.clear();
        const unsigned before_shot = nonzero_audio;
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_Y; retro_run();
        CHECK(nonzero_audio == before_shot);
        bool active[2] = {};
        for (const auto& call : rumble_calls) {
            CHECK(call.port == 0);
            CHECK(call.effect == RETRO_RUMBLE_STRONG || call.effect == RETRO_RUMBLE_WEAK);
            active[static_cast<unsigned>(call.effect)] = call.strength != 0;
        }
        CHECK(active[0] || active[1]);
        rumble_calls.clear(); buttons = 0;
        if (interruption == 0) { set_vibration("Off"); retro_run(); }
        if (interruption == 1) retro_unload_game();
        if (interruption == 2) retro_reset();
        if (interruption == 3) { buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run(); }
        for (const auto& call : rumble_calls) {
            CHECK(call.port == 0);
            CHECK(call.effect == RETRO_RUMBLE_STRONG || call.effect == RETRO_RUMBLE_WEAK);
            active[static_cast<unsigned>(call.effect)] = call.strength != 0;
        }
        CHECK(!active[0] && !active[1]);
        if (interruption != 1) retro_unload_game();
        retro_deinit();
    }

    shooting_fire_sound_value.clear();
    // A missing interface or a controller that rejects rumble must not prevent play.
    for (int unavailable = 0; unavailable < 2; ++unavailable) {
        rumble_available = unavailable != 0; rumble_accept = false;
        set_vibration("On"); buttons = 0; rumble_calls.clear(); rumble_queries = 0;
        retro_init(); CHECK(retro_load_game(&game)); retro_run();
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
        buttons = 0; retro_run();
        const unsigned before = nonzero_audio;
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_Y; retro_run();
        CHECK(rumble_queries > 0 && nonzero_audio > before);
        if (!rumble_available) CHECK(rumble_calls.empty());
        else CHECK(!rumble_calls.empty());
        retro_unload_game(); retro_deinit();
    }
    rumble_available = false; rumble_accept = true; vibration_value.clear(); buttons = 0;
    return 0;
}
} // namespace core_test
