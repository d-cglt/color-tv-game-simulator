#include "controls.h"
#include "tennis_b.h"
#include "shooting.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <tuple>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)
namespace {
using Key = std::tuple<unsigned, unsigned, unsigned, unsigned>;
std::map<Key, int16_t> values;
unsigned polls = 0, reads = 0;
void poll() { ++polls; }
int16_t input(unsigned port, unsigned device, unsigned index, unsigned id) {
    ++reads;
    const auto it = values.find({port, device, index, id});
    return it == values.end() ? 0 : it->second;
}
void pad(unsigned port, unsigned id, int16_t value = 1) { values[{port, RETRO_DEVICE_JOYPAD, 0, id}] = value; }
void key(unsigned id, int16_t value = 1) { values[{0, RETRO_DEVICE_KEYBOARD, 0, id}] = value; }
void axis(unsigned port, unsigned id, int16_t value, unsigned stick = 0) { values[{port, RETRO_DEVICE_ANALOG, stick, id}] = value; }
constexpr unsigned up = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
constexpr unsigned down = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
constexpr unsigned left = 1u << RETRO_DEVICE_ID_JOYPAD_LEFT;
constexpr unsigned right = 1u << RETRO_DEVICE_ID_JOYPAD_RIGHT;
}
int descriptor_tests() {
    static const retro_input_descriptor expected[] = {
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Confirm/Fire"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Menu back"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Pause / resume"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Fire (Shooting)"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "P1 paddle/aim up"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "P1 paddle/aim down"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "P1 steer left"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "P1 steer right"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "Low gear (Racing 1P)"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "High gear (Racing 1P)"},
        {0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "P1 paddle/aim"},
        {0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "P1 steering"},
        {1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "P2 paddle/manual target up"},
        {1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "P2 paddle/manual target down"},
        {1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "P2 steer left"},
        {1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "P2 steer right"},
        {1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "P2 paddle/manual target"},
        {1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "P2 steering"},
        {0, 0, 0, 0, nullptr}
    };
    const auto* shared = ctg::Controls::descriptors(0);
    for (unsigned id = 0; id <= 17; ++id) {
        const auto* actual = ctg::Controls::descriptors(id);
        CHECK(actual == shared);
        for (unsigned row = 0; row < sizeof(expected) / sizeof(expected[0]); ++row) {
            CHECK(actual[row].port == expected[row].port);
            CHECK(actual[row].device == expected[row].device);
            CHECK(actual[row].index == expected[row].index);
            CHECK(actual[row].id == expected[row].id);
            if (!expected[row].description) CHECK(actual[row].description == nullptr);
            else {
                CHECK(actual[row].description != nullptr);
                CHECK(std::strcmp(actual[row].description, expected[row].description) == 0);
            }
        }
    }
    CHECK(ctg::Controls::descriptors(~0u) == shared);
    ctg::Controls temporary;
    temporary.reset_repeat(); temporary.reset_racing(); temporary.set_enabled(0, false);
    CHECK(temporary.descriptors(14) == shared && temporary.descriptors(15) == shared);
    return 0;
}
int analog_movement_tests() {
    ctg::Controls controls;
    controls.set_input_state(input);
    for (unsigned port = 0; port < 2; ++port) for (int sign : {-1, 1}) {
        for (int deflection : {8193, 16384, 32767}) {
            values.clear();
            axis(port, RETRO_DEVICE_ID_ANALOG_Y, sign * deflection);
            // Frontends may synthesize D-pad input from the same stick.
            pad(port, sign < 0 ? RETRO_DEVICE_ID_JOYPAD_UP : RETRO_DEVICE_ID_JOYPAD_DOWN);
            CHECK(controls.paddle_axis(port) == sign * deflection);
            ctg::TennisB rally;
            ctg::Shooting shooting;
            const int start = rally.state().paddle_y[port];
            for (int frame = 0; frame < 8; ++frame) {
                ctg::TennisInput input{};
                input.analog[port] = controls.paddle_axis(port);
                rally.step(input);
                std::array<int, 2> axes{}; axes[port] = controls.paddle_axis(port, true);
                shooting.step(0, false, 0, false, true, axes);
            }
            const int expected = (deflection - 8192) * 8 * 8 / 24575;
            CHECK(rally.state().paddle_y[port] == start + sign * expected);
            if (port == 0) CHECK(shooting.state().shooter_y == 117 + sign * ((deflection - 8192) * 6 * 8 / 24575));
            else CHECK(shooting.state().target_y == (405 + sign * ((deflection - 8192) * 24 * 8 / 24575)) / 4);
        }
    }
    values.clear(); axis(0, RETRO_DEVICE_ID_ANALOG_Y, 8192);
    CHECK(controls.paddle_axis(0) == 0);
    axis(0, RETRO_DEVICE_ID_ANALOG_Y, -32768);
    CHECK(controls.paddle_axis(0) == -32768);
    key(RETROK_w); CHECK(controls.paddle_axis(0, true) == 0);
    controls.set_enabled(0, false); CHECK(controls.paddle_axis(0) == 0);
    ctg::TennisB fractional;
    ctg::TennisInput slow{}; slow.analog[0] = 10000;
    const int center = fractional.state().paddle_y[0];
    fractional.step(slow);
    slow.analog[0] = -10000; fractional.step(slow);
    CHECK(fractional.state().paddle_y[0] == center);
    fractional.step(slow);
    CHECK(fractional.state().paddle_y[0] == center - 1);
    fractional.reset(); slow.analog[0] = 10000; fractional.step(slow);
    fractional.reset_input(0); fractional.step(slow);
    CHECK(fractional.state().paddle_y[0] == center);
    slow.analog[0] = 0; fractional.step(slow);
    slow.analog[0] = 10000; fractional.step(slow);
    CHECK(fractional.state().paddle_y[0] == center);
    ctg::TennisB rally;
    ctg::TennisInput in{}; in.analog[0] = 16384;
    rally.step(in); rally.reset();
    const int initial = rally.state().paddle_y[0];
    in.analog[0] = -8193; rally.step(in);
    CHECK(rally.state().paddle_y[0] == initial);
    in.analog[0] = 32767;
    for (int i = 0; i < 100; ++i) rally.step(in);
    CHECK(rally.state().paddle_y[0] == rally.paddle_max(0));
    in.analog[0] = -32767; rally.step(in);
    CHECK(rally.state().paddle_y[0] == rally.paddle_max(0) - 8);
    ctg::Shooting shooting;
    for (int i = 0; i < 100; ++i) shooting.step(0, false, 0, false, true, {32767, 0});
    const int edge = shooting.state().shooter_y;
    shooting.step(0, false, 0, false, true, {-32767, 0});
    CHECK(shooting.state().shooter_y == edge - 6);
    shooting.reset();
    shooting.step(0, false, 0, true, true, {0, 32767});
    ctg::Shooting automatic;
    automatic.step(0, false, 0, true);
    CHECK(shooting.state().target_y == automatic.state().target_y);
    return 0;
}
int main() {
    CHECK(descriptor_tests() == 0);
    CHECK(analog_movement_tests() == 0);
    values.clear();
    ctg::Controls controls;
    ctg::ControlsContext context;
    controls.poll();
    CHECK(controls.read_buttons(context) == 0);
    CHECK(!controls.any_input(true, 0));
    CHECK(controls.any_input(false, up));
    CHECK(controls.paddle_direction(0) == 0);
    CHECK(!controls.racing_input(true).analog);
    CHECK(controls.racing_input(true).recovering);
    controls.set_input_poll(poll);
    controls.set_input_state(input);
    controls.poll();
    CHECK(polls == 1);
    CHECK(controls.enabled(0) && controls.enabled(1) && !controls.enabled(2));

    key(RETROK_UP); key(RETROK_DOWN); key(RETROK_LEFT); key(RETROK_RIGHT);
    CHECK(controls.read_buttons(context) == (up | down | left | right));
    context.detail = true;
    CHECK(controls.read_buttons(context) == 0);
    for (unsigned gate = 0; gate < 3; ++gate) {
        context.paused = gate == 0; context.result = gate == 1; context.test_menu = gate == 2;
        CHECK(controls.read_buttons(context) == (up | down | (gate == 2 ? left | right : 0)));
    }
    context = {}; context.detail = true;
    values.clear();
    key(RETROK_p); key(RETROK_RETURN); key(RETROK_ESCAPE); key(RETROK_x);
    const unsigned common = (1u << 31) | (1u << RETRO_DEVICE_ID_JOYPAD_A) | (1u << RETRO_DEVICE_ID_JOYPAD_B);
    CHECK(controls.read_buttons(context) == common);
    context.content_id = 14;
    CHECK(controls.read_buttons(context) == (common | (1u << RETRO_DEVICE_ID_JOYPAD_Y)));
    key(RETROK_x, 0); key(RETROK_SPACE);
    CHECK(controls.read_buttons(context) == (common | (1u << RETRO_DEVICE_ID_JOYPAD_Y)));

    values.clear(); context = {};
    for (unsigned port = 0; port < 2; ++port) for (int direction : {-1, 1}) {
        values.clear();
        pad(port, direction < 0 ? RETRO_DEVICE_ID_JOYPAD_UP : RETRO_DEVICE_ID_JOYPAD_DOWN);
        ctg::TennisB rally;
        ctg::Shooting shooting;
        const int start = rally.state().paddle_y[port];
        rally.step(port == 0 ? ctg::TennisInput{controls.paddle_direction(port), 0} : ctg::TennisInput{0, controls.paddle_direction(port)});
        shooting.step(controls.shooting_direction(0), false, controls.shooting_direction(1));
        CHECK(rally.state().paddle_y[port] == start + direction * 8);
        CHECK(port == 0 ? shooting.state().shooter_y == 117 + direction * 6 : shooting.state().target_y == (405 + direction * 24) / 4);
        values.clear();
        axis(port, RETRO_DEVICE_ID_ANALOG_Y, direction * 9000);
        CHECK(controls.paddle_direction(port) == direction && controls.shooting_direction(port) == direction);
    }
    values.clear();
    for (int value : {-8193, -8192, 8192, 8193}) {
        axis(0, RETRO_DEVICE_ID_ANALOG_Y, value);
        CHECK(controls.read_buttons(context) == (value < -8192 ? up : value > 8192 ? down : 0));
        CHECK(controls.paddle_direction(0) == (value > 8192) - (value < -8192));
        CHECK(controls.any_input(true, 0) == (value < -8192 || value > 8192));
    }
    context.detail = true;
    CHECK(controls.read_buttons(context) == 0);
    pad(0, RETRO_DEVICE_ID_JOYPAD_UP);
    CHECK(controls.paddle_direction(0) == -1);
    CHECK(controls.read_buttons(context) == up); // Joypad buttons are not context-gated.
    pad(0, RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(controls.paddle_direction(0) == 1); // Opposite digital inputs cancel, leaving analog.
    key(RETROK_w);
    CHECK(controls.shooting_direction(0) == -1);
    axis(1, RETRO_DEVICE_ID_ANALOG_Y, 8193);
    CHECK(controls.shooting_direction(1) == 1);
    CHECK(controls.paddle_direction(0) == 1); // W/S apply only to shooting.
    key(RETROK_s);
    CHECK(controls.shooting_direction(0) == 1);
    controls.set_device(0, RETRO_DEVICE_NONE);
    controls.set_enabled(1, false);
    CHECK(!controls.enabled(0) && !controls.enabled(1));
    CHECK(controls.paddle_direction(0) == 0 && controls.shooting_direction(1) == 0);
    key(RETROK_s, 0);
    CHECK(controls.shooting_direction(0) == -1); // Keyboard survives disconnection.
    values.clear(); pad(0, RETRO_DEVICE_ID_JOYPAD_A); pad(1, RETRO_DEVICE_ID_JOYPAD_R3);
    CHECK(controls.read_buttons({}) == 0 && !controls.any_input(true, 0));
    controls.set_device(1, RETRO_DEVICE_ANALOG);
    CHECK(controls.any_input(true, 0));
    CHECK(!controls.any_input(false, 0));
    values.clear(); axis(1, RETRO_DEVICE_ID_ANALOG_X, -8193, 1);
    CHECK(controls.any_input(true, 0));
    values.clear(); key(RETROK_F1);
    CHECK(controls.any_input(true, 0));
    CHECK(controls.read_buttons({}) == 0);
    controls.set_device(2, RETRO_DEVICE_JOYPAD);
    CHECK(!controls.enabled(2) && controls.paddle_direction(2) == 0);
    controls.set_device(0, RETRO_DEVICE_JOYPAD);

    for (unsigned port = 0; port < 2; ++port) {
        for (unsigned id : {RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_RIGHT}) {
            controls.reset_racing(); values.clear(); pad(port, id);
            const auto direction = controls.racing_input(false, port);
            CHECK(direction.left == (id == RETRO_DEVICE_ID_JOYPAD_LEFT));
            CHECK(direction.right == (id == RETRO_DEVICE_ID_JOYPAD_RIGHT));
        }
        for (int sign : {-1, 1}) {
            controls.reset_racing(); values.clear(); axis(port, RETRO_DEVICE_ID_ANALOG_X, sign * 9000);
            const auto direction = controls.racing_input(false, port);
            CHECK(direction.analog && direction.steering == sign * 9000);
        }
    }
    for (unsigned id : {RETRO_DEVICE_ID_JOYPAD_L2, RETRO_DEVICE_ID_JOYPAD_R2}) {
        values.clear(); pad(0, id);
        const auto gear = controls.racing_input(false, 0);
        CHECK(gear.select_low_speed == (id == RETRO_DEVICE_ID_JOYPAD_L2));
        CHECK(gear.select_high_speed == (id == RETRO_DEVICE_ID_JOYPAD_R2));
    }
    controls.reset_racing(); values.clear();
    for (int value : {-8192, 8192}) {
        axis(0, RETRO_DEVICE_ID_ANALOG_X, value);
        CHECK(!controls.racing_input(false).analog);
    }
    axis(0, RETRO_DEVICE_ID_ANALOG_X, -8193);
    auto steering = controls.racing_input(true);
    CHECK(steering.analog && steering.steering == -8193 && steering.recovering);
    axis(0, RETRO_DEVICE_ID_ANALOG_X, 0);
    CHECK(controls.racing_input(false).analog);
    pad(0, RETRO_DEVICE_ID_JOYPAD_LEFT);
    pad(0, RETRO_DEVICE_ID_JOYPAD_R2); pad(0, RETRO_DEVICE_ID_JOYPAD_L2);
    steering = controls.racing_input(false);
    CHECK(!steering.analog && steering.left && steering.select_high_speed && steering.select_low_speed);
    axis(0, RETRO_DEVICE_ID_ANALOG_X, 8193);
    CHECK(controls.racing_input(false).analog); // Deflection wins over digital.
    controls.reset_racing();
    values.clear();
    CHECK(!controls.racing_input(false).analog);
    axis(0, RETRO_DEVICE_ID_ANALOG_X, 8193);
    CHECK(controls.racing_input(false).analog);
    controls.set_enabled(0, false);
    CHECK(!controls.racing_input(false).analog);
    controls.set_enabled(0, true); values.clear();
    CHECK(controls.racing_input(false).analog); // Disconnect does not reset the latch.
    key(RETROK_a); key(RETROK_d);
    steering = controls.racing_input(false);
    CHECK(!steering.left && !steering.right);
    values.clear(); key(RETROK_LEFT); key(RETROK_RIGHT);
    controls.set_enabled(0, false);
    steering = controls.racing_input(true);
    CHECK(!steering.left && !steering.right && steering.recovering);

    controls.set_enabled(0, true);
    controls.reset_racing(); values.clear();
    axis(1, RETRO_DEVICE_ID_ANALOG_X, 8192);
    CHECK(!controls.racing_input(false, 1).analog);
    axis(1, RETRO_DEVICE_ID_ANALOG_X, -8193);
    pad(1, RETRO_DEVICE_ID_JOYPAD_RIGHT);
    pad(1, RETRO_DEVICE_ID_JOYPAD_L2); pad(1, RETRO_DEVICE_ID_JOYPAD_R2);
    steering = controls.racing_input(true, 1);
    CHECK(steering.analog && steering.steering == -8193 && steering.right && !steering.left && steering.recovering);
    CHECK(!steering.select_high_speed && !steering.select_low_speed);
    CHECK(!controls.racing_input(false).analog);
    axis(1, RETRO_DEVICE_ID_ANALOG_X, 0);
    pad(1, RETRO_DEVICE_ID_JOYPAD_RIGHT, 0);
    CHECK(controls.racing_input(false, 1).analog);
    axis(0, RETRO_DEVICE_ID_ANALOG_X, 9000);
    CHECK(controls.racing_input(false).analog);
    pad(1, RETRO_DEVICE_ID_JOYPAD_LEFT);
    CHECK(controls.racing_input(false, 1).left && !controls.racing_input(false, 1).analog);
    CHECK(controls.racing_input(false).analog);
    pad(1, RETRO_DEVICE_ID_JOYPAD_LEFT, 0);
    key(RETROK_LEFT); key(RETROK_d);
    steering = controls.racing_input(false, 1);
    CHECK(!steering.left && !steering.right && !steering.analog);
    axis(1, RETRO_DEVICE_ID_ANALOG_X, 9000);
    CHECK(controls.racing_input(false, 1).analog);
    controls.set_enabled(1, false);
    steering = controls.racing_input(true, 1);
    CHECK(!steering.analog && !steering.left && !steering.right && steering.steering == 0 && steering.recovering);
    controls.set_enabled(1, true); values.clear();
    CHECK(controls.racing_input(false, 1).analog);
    CHECK(controls.racing_input(false).analog);
    controls.reset_racing();
    CHECK(!controls.racing_input(false).analog && !controls.racing_input(false, 1).analog);
    const unsigned before_invalid = reads;
    steering = controls.racing_input(true, 2);
    CHECK(steering.recovering && !steering.analog && !steering.left && !steering.right && steering.steering == 0);
    CHECK(reads == before_invalid);

    CHECK(controls.menu_navigation(0, down, down, false) == down);
    for (unsigned frame = 1; frame <= 60; ++frame)
        CHECK(controls.menu_navigation(0, down, 0, false) == (frame >= 24 && (frame - 24) % 6 == 0 ? down : 0));
    CHECK(controls.menu_navigation(1, down, 0, false) == 0);
    // A held direction without a fresh edge must never rearm after a menu change.
    for (unsigned frame = 0; frame < 80; ++frame) CHECK(controls.menu_navigation(1, down, 0, false) == 0);
    CHECK(controls.menu_navigation(1, up, up, false) == up);
    CHECK(controls.menu_navigation(1, up | down, down, false) == 0);
    CHECK(controls.menu_navigation(1, up, 0, false) == 0);
    CHECK(controls.menu_navigation(1, down, down, true) == 0);
    for (unsigned frame = 0; frame < 30; ++frame) CHECK(controls.menu_navigation(1, down, 0, false) == 0);
    CHECK(controls.menu_navigation(-1, down, down, false) == 0);
    controls.reset_repeat();
    CHECK(controls.menu_navigation(0, up, 0, false) == 0);
    CHECK(controls.menu_navigation(0, 0, 0, false) == 0);
    CHECK(controls.menu_navigation(0, up, up, false) == up);
    controls.reset_repeat();
    for (unsigned frame = 0; frame < 30; ++frame) CHECK(controls.menu_navigation(0, up, 0, false) == 0);
    controls.set_input_state(nullptr);
    const unsigned before = reads;
    CHECK(controls.read_buttons({}) == 0 && !controls.any_input(true, 0));
    CHECK(controls.shooting_direction(0) == 0 && !controls.racing_input(false).left);
    CHECK(reads == before);
    return 0;
}
