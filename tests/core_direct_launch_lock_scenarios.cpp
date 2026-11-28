#include "core_test_harness.h"
#include <cstring>

namespace core_test {
namespace {
bool has_descriptor(unsigned port, unsigned device, unsigned index, unsigned id, const char* label) {
    for (const auto& d : descriptors)
        if (d.port == port && d.device == device && d.index == index && d.id == id &&
            std::strcmp(d.description, label) == 0) return true;
    return false;
}
}
int direct_launch_lock_scenarios(ContentContext& context) {
    auto& bytes = context.bytes;
    auto& game = context.game;
    save_available = false; start_game_value = "First"; demo_rotation_value = "Off";
    buttons = buttons2 = 0; keys.fill(false); axes = {{0, 0}};
    direct_launch_lock_value = "On";
    retro_init();
    const unsigned initial_shutdowns = shutdowns;
    std::vector<retro_input_descriptor> shared_descriptors;
    for (unsigned id = 0; id < 16; ++id) {
        bytes[4] = static_cast<unsigned char>(id);
        buttons = 0; CHECK(retro_load_game(&game)); retro_run();
        CHECK(has_descriptor(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Confirm/Fire"));
        CHECK(has_descriptor(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "P1 steering"));
        CHECK(has_descriptor(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "P2 paddle/manual target"));
        CHECK(has_descriptor(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "Low gear (Racing 1P)"));
        CHECK(has_descriptor(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "High gear (Racing 1P)"));
        for (const auto& d : descriptors) if (d.port == 1) CHECK(std::strstr(d.description, "gear") == nullptr);
        if (id == 0) shared_descriptors = descriptors;
        CHECK(descriptors.size() == shared_descriptors.size());
        for (size_t i = 0; i < descriptors.size(); ++i) {
            const auto& actual = descriptors[i];
            const auto& expected = shared_descriptors[i];
            CHECK(actual.port == expected.port && actual.device == expected.device &&
                  actual.index == expected.index && actual.id == expected.id &&
                  std::strcmp(actual.description, expected.description) == 0);
        }
        const auto launch = frame;
        last_message.clear();
        press(RETRO_DEVICE_ID_JOYPAD_B);
        CHECK(same_menu(frame, launch));
        CHECK(last_message.empty());
        // START is the first visible row, including for the formerly scrolling Racing menu.
        const unsigned first_y = id == 15 ? 88 : 98;
        CHECK(pixel(53, first_y) == 0x295166);
        // Return is skipped in both navigation directions, not a dead action.
        press(RETRO_DEVICE_ID_JOYPAD_UP);
        CHECK(pixel(53, 178) == 0x295166);
        press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        CHECK(same_menu(frame, launch));
        press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        CHECK(pixel(53, first_y + (id == 15 ? 15 : 16)) == 0x295166);

        press(RETRO_DEVICE_ID_JOYPAD_UP);
        CHECK(same_menu(frame, launch));
        press(RETRO_DEVICE_ID_JOYPAD_START);
        exit_pause(); CHECK(same_menu(frame, launch));
        retro_reset(); buttons = 0; retro_run(); CHECK(same_menu(frame, launch));
        retro_unload_game();
    }
    // No-content sessions remain unrestricted even with the option enabled.
    buttons = 0; CHECK(retro_load_game(nullptr)); retro_run();
    const auto selector = frame;
    press(RETRO_DEVICE_ID_JOYPAD_A);
    press(RETRO_DEVICE_ID_JOYPAD_B); CHECK(same_menu(frame, selector));
    retro_unload_game();

    // Enabling in the selector and after switching games restores the loaded ID.
    bytes[4] = 14; direct_launch_lock_value = "Off"; buttons = 0;
    CHECK(retro_load_game(&game)); retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_B);
    direct_launch_lock_value = "On"; buttons = 0; retro_run();
    const auto locked_shooting = frame;
    press(RETRO_DEVICE_ID_JOYPAD_B); CHECK(same_menu(frame, locked_shooting));
    direct_launch_lock_value = "Off"; press(RETRO_DEVICE_ID_JOYPAD_B);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    press(RETRO_DEVICE_ID_JOYPAD_START);
    direct_launch_lock_value = "On"; buttons = 0; retro_run();
    CHECK(same_menu(frame, locked_shooting));
    // Direct reload replaces the target, even without an intervening unload.
    bytes[4] = 15; CHECK(retro_load_game(&game)); buttons = 0; retro_run();
    const auto locked_racing = frame;
    CHECK(!same_menu(locked_racing, locked_shooting));
    press(RETRO_DEVICE_ID_JOYPAD_B); CHECK(same_menu(frame, locked_racing));
    retro_unload_game();
    buttons = 0; CHECK(retro_load_game(nullptr)); retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_A); press(RETRO_DEVICE_ID_JOYPAD_B);
    CHECK(same_menu(frame, selector));
    retro_unload_game();

    // Shooting reaches a real result screen; Exit returns to its launch menu.
    bytes[4] = 14; buttons = 0; CHECK(retro_load_game(&game)); retro_run();
    const auto shooting_menu = frame;
    press(RETRO_DEVICE_ID_JOYPAD_START);
    for (int shot = 0; shot < 15; ++shot) {
        press(RETRO_DEVICE_ID_JOYPAD_Y);
        buttons = 0; for (int i = 0; i < 100; ++i) retro_run();
    }
    CHECK(pixel(75, 99) == 0x295166);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    CHECK(same_menu(frame, shooting_menu));
    press(RETRO_DEVICE_ID_JOYPAD_B); CHECK(same_menu(frame, shooting_menu));
    retro_unload_game();

    bytes[4] = 2; buttons = buttons2 = 0; CHECK(retro_load_game(&game)); retro_run();
    const auto rally_menu = frame;
    press(RETRO_DEVICE_ID_JOYPAD_START);
    buttons = buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    int remaining = 30000;
    do { retro_run(); } while (pixel(75, 99) != 0x295166 && --remaining);
    CHECK(remaining > 0);
    buttons = buttons2 = 0; retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    CHECK(same_menu(frame, rally_menu));
    retro_unload_game();

    bytes[4] = 15; buttons = 0; CHECK(retro_load_game(&game)); retro_run();
    const auto racing_menu = frame;
    press(RETRO_DEVICE_ID_JOYPAD_START); buttons = 0;
    remaining = 6000;
    do { retro_run(); } while (pixel(75, 99) != 0x295166 && --remaining);
    CHECK(remaining > 0);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    CHECK(same_menu(frame, racing_menu));
    CHECK(shutdowns == initial_shutdowns);
    retro_unload_game(); retro_deinit(); direct_launch_lock_value.clear();
    return 0;
}
}
