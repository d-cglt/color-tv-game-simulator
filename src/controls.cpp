#include "controls.h"

namespace ctg {
namespace {
constexpr int threshold = 8192;
constexpr unsigned repeat_delay = 24, repeat_interval = 6;
constexpr unsigned up = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
constexpr unsigned down = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
}
const retro_input_descriptor* Controls::descriptors(unsigned content_id) {
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
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "Low lever"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "High lever"},
        {0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Steering"},
        {1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "P2 steer left"},
        {1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "P2 steer right"},
        {1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "P2 steering"},
        {0, 0, 0, 0, nullptr}
    };
    return content_id == 15 ? racing_descriptors : content_id == 14 ? shooting_descriptors : descriptors;
}
void Controls::poll() const { if (poll_) poll_(); }
bool Controls::enabled(unsigned port) const { return port < joypad_.size() && joypad_[port]; }
void Controls::set_enabled(unsigned port, bool value) {
    if (port < joypad_.size()) joypad_[port] = value;
}
void Controls::set_device(unsigned port, unsigned device) {
    set_enabled(port, device == RETRO_DEVICE_JOYPAD || device == RETRO_DEVICE_ANALOG);
}
unsigned Controls::read_buttons(const ControlsContext& c) const {
    unsigned buttons = 0;
    if (!input_) return buttons;
    if (enabled(0)) {
        for (unsigned id : {RETRO_DEVICE_ID_JOYPAD_UP, RETRO_DEVICE_ID_JOYPAD_DOWN,
             RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_RIGHT,
             RETRO_DEVICE_ID_JOYPAD_A, RETRO_DEVICE_ID_JOYPAD_B,
             RETRO_DEVICE_ID_JOYPAD_START, RETRO_DEVICE_ID_JOYPAD_Y,
             RETRO_DEVICE_ID_JOYPAD_L2, RETRO_DEVICE_ID_JOYPAD_R2})
            if (input_(0, RETRO_DEVICE_JOYPAD, 0, id)) buttons |= 1u << id;
    }
    if ((!c.detail || c.test_menu) && input_(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LEFT)) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_LEFT;
    if ((!c.detail || c.test_menu) && input_(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RIGHT)) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_RIGHT;
    const bool vertical = !c.detail || c.paused || c.result || c.test_menu;
    if (vertical && input_(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_UP)) buttons |= up;
    if (vertical && input_(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_DOWN)) buttons |= down;
    if (vertical && enabled(0)) {
        const int axis = input_(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
        if (axis < -threshold) buttons |= up;
        if (axis > threshold) buttons |= down;
    }
    if (input_(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_p)) buttons |= 1u << 31;
    if (input_(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RETURN)) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_A;
    if (input_(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_ESCAPE)) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_B;
    if (c.content_id == 14 && (input_(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_x) || input_(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_SPACE))) buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_Y;
    return buttons;
}
bool Controls::any_input(bool preview_menu, unsigned buttons) const {
    bool any = buttons != 0;
    if (preview_menu && input_) {
        for (unsigned port = 0; port < 2; ++port) if (enabled(port)) {
            for (unsigned id = 0; id <= RETRO_DEVICE_ID_JOYPAD_R3; ++id)
                any |= input_(port, RETRO_DEVICE_JOYPAD, 0, id) != 0;
            for (unsigned stick = 0; stick < 2; ++stick)
                for (unsigned axis = 0; axis < 2; ++axis) {
                    const int value = input_(port, RETRO_DEVICE_ANALOG, stick, axis);
                    any |= value < -threshold || value > threshold;
                }
        }
        for (unsigned key = 0; key < RETROK_LAST; ++key)
            any |= input_(0, RETRO_DEVICE_KEYBOARD, 0, key) != 0;
    }
    return any;
}
int Controls::paddle_direction(unsigned port) const {
    if (!input_ || !enabled(port)) return 0;
    const int axis = input_(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
    const int analog = (axis > threshold) - (axis < -threshold);
    const int digital = (input_(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) != 0) -
                        (input_(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) != 0);
    return digital ? digital : analog;
}
int Controls::paddle_axis(unsigned port, bool shooting) const {
    if (!input_ || !enabled(port)) return 0;
    if (shooting && port == 0 && (input_(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_w) || input_(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_s))) return 0;
    const int axis = input_(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
    return axis < -threshold || axis > threshold ? axis : 0;
}
int Controls::shooting_direction(unsigned port) const {
    int direction = paddle_direction(port);
    if (port == 0 && input_) {
        const int keyboard = (input_(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_s) != 0) -
                             (input_(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_w) != 0);
        if (keyboard) direction = keyboard;
    }
    return direction;
}
RacingInput Controls::racing_input(bool recovering, unsigned port) {
    RacingInput steering{};
    steering.recovering = recovering;
    if (port >= racing_analog_active_.size()) return steering;
    if (input_ && enabled(port)) {
        const int axis = input_(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
        if (axis < -threshold || axis > threshold) racing_analog_active_[port] = true;
        steering.analog = racing_analog_active_[port];
        steering.steering = axis;
        steering.left = input_(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) != 0;
        steering.right = input_(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) != 0;
        if (port == 0) {
            steering.select_high_speed = input_(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2) != 0;
            steering.select_low_speed = input_(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2) != 0;
        }
    }

    if ((steering.left || steering.right) && steering.steering >= -threshold && steering.steering <= threshold)
        racing_analog_active_[port] = steering.analog = false;
    return steering;
}
void Controls::reset_repeat() {
    repeat_menu_ = -1;
    repeat_direction_ = 0;
    repeat_frames_ = 0;
}
unsigned Controls::menu_navigation(int menu, unsigned buttons, unsigned pressed, bool suppress) {
    if (menu != repeat_menu_) { repeat_menu_ = menu; repeat_direction_ = 0; repeat_frames_ = 0; }
    unsigned navigation = 0;
    const int direction = suppress ? 0 : int(bool(buttons & down)) - int(bool(buttons & up));
    if (menu < 0 || direction == 0) { repeat_direction_ = 0; repeat_frames_ = 0; }
    else if (direction != repeat_direction_) {
        repeat_direction_ = direction;
        repeat_frames_ = 0;
        if (pressed & (direction < 0 ? up : down)) navigation = direction < 0 ? up : down;
        else repeat_direction_ = 0;
    } else if (++repeat_frames_ >= repeat_delay && (repeat_frames_ - repeat_delay) % repeat_interval == 0) {
        navigation = direction < 0 ? up : down;
        // Preserve the original counter reset, including subsequent six-frame repeats.
        repeat_frames_ = repeat_delay;
    }
    return navigation;
}
}
