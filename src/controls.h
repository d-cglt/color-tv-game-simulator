#pragma once
#include "libretro.h"
#include "racing.h"
#include <array>

namespace ctg {
struct ControlsContext {
    bool detail = false;
    bool paused = false;
    bool result = false;
    bool test_menu = false;
    unsigned content_id = 0;
};

class Controls {
public:
    void set_input_poll(retro_input_poll_t callback) { poll_ = callback; }
    void set_input_state(retro_input_state_t callback) { input_ = callback; }
    void poll() const;
    // Sentinel-terminated descriptors with static lifetime, independent of Controls instances.
    static const retro_input_descriptor* descriptors(unsigned content_id);
    unsigned read_buttons(const ControlsContext& context) const;
    bool any_input(bool preview_menu, unsigned buttons) const;
    int paddle_direction(unsigned port) const;
    int shooting_direction(unsigned port) const;
    // Raw active stick deflection; zero inside the deadzone or when disconnected.
    int paddle_axis(unsigned port, bool shooting = false) const;
    // Port 0 also supports keyboard steering and gear selection; port 1 is steering only.
    RacingInput racing_input(bool recovering, unsigned port = 0);
    unsigned menu_navigation(int menu, unsigned buttons, unsigned pressed, bool suppress);
    void reset_repeat();
    void reset_racing() { racing_analog_active_.fill(false); }

    // Rumble remains the caller's responsibility, including stopping it before a device change.
    void set_device(unsigned port, unsigned device);
    void set_enabled(unsigned port, bool enabled);
    bool enabled(unsigned port) const;

private:
    retro_input_poll_t poll_ = nullptr;
    retro_input_state_t input_ = nullptr;
    std::array<bool, 2> joypad_{{true, true}};
    int repeat_menu_ = -1, repeat_direction_ = 0;
    unsigned repeat_frames_ = 0;
    std::array<bool, 2> racing_analog_active_{{false, false}};
};
}
