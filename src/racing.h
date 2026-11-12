#pragma once
#include <array>

namespace ctg {
// The rendered car extends beyond its collision rectangle on each side.
constexpr int racing_car_overhang = 3;
constexpr unsigned racing_start_speed = 30;
constexpr unsigned racing_road_dash_length = 240;
constexpr unsigned racing_road_gap_length = 24;

// All rectangles use top-left coordinates and half-open pixel bounds in a
// 320x240 playfield. Traffic moves toward the bottom of the screen.
struct RacingRect {
    int x, y, width, height;
};

// Provisional deterministic placement of an opponent within the road.
int racing_opponent_x(const RacingRect& road, unsigned index);

struct RacingTraffic {
    RacingRect rect{};
    bool collided = false; // This car cannot score after a collision.
};

struct RacingState {
    RacingRect road{40, 0, 240, 240};
    RacingRect player{152, 200, 16, 24};
    std::array<RacingTraffic, 1> traffic{};
    unsigned frame = 0;
    // Prototype speed units; road movement uses an additional 16/9 scale.
    unsigned speed = racing_start_speed, selected_speed = 120, movement_remainder = 0, road_scroll = 0;
    unsigned frames_remaining = 60 * 60;
    unsigned score = 0;      // One point per collision-free road section.
    unsigned collisions = 0;
    bool section_collided = false;
    bool finished = false;
};

struct RacingInput {
    // Digital steering moves 6 pixels per frame; opposite directions cancel.
    bool left = false;
    bool right = false;
    // Analog deflection controls steering speed and overrides digital input.
    // The center dead zone is 8192; full deflection moves 6 pixels per frame.
    bool analog = false;
    int steering = 0;
    bool select_high_speed = false;
    bool select_low_speed = false;
    bool recovering = false;
};

enum RacingEvent : unsigned {
    racing_none = 0,
    racing_collision = 1 << 0,
    racing_pass = 1 << 1,
    racing_finished = 1 << 2
};

// Original, simplified arcade approximation, not a hardware simulation.
class Racing {
public:
    static constexpr unsigned frames_per_second = 60;
    static constexpr unsigned duration_frames = 60 * frames_per_second;
    static constexpr unsigned start_speed = racing_start_speed;
    static constexpr unsigned low_speed = 120;
    // 120 * 393 / 193, rounded to the nearest speed unit.
    static constexpr unsigned max_speed = 244;

    Racing() { reset(); }
    void reset();
    void reset_preview();
    void reset_from_layout(const RacingState& layout);
    void step_preview();
    // Advances exactly one frame; returns a bitmask of RacingEvent values.
    // After the timer expires, steps do nothing until reset().
    unsigned step(RacingInput input = {});
    const RacingState& state() const { return state_; }

private:
    RacingState state_{};
    unsigned next_lane_ = 0;
    int steering_remainder_ = 0;
};
}
