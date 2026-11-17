#pragma once
#include <array>

namespace ctg {
constexpr int racing_cell_width = 4;
constexpr int racing_cell_height = 5;
constexpr int racing_road_rows = 48;
constexpr int racing_car_width = 6 * racing_cell_width;
constexpr int racing_car_height = 6 * racing_cell_height;
constexpr int racing_road_x = 54;
constexpr int racing_road_columns = 53; // X26 is the conceptual center; no divider is drawn.
constexpr int racing_road_border_width = racing_cell_width;
constexpr int racing_x(int col) { return racing_road_x + col * racing_cell_width; }
constexpr unsigned racing_frames_per_second = 60;
constexpr unsigned racing_duration_frames = 64 * racing_frames_per_second;
constexpr unsigned racing_start_speed = 30;
constexpr unsigned racing_low_speed = 133;
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
    RacingRect road{racing_road_x, 0, racing_road_columns * racing_cell_width, racing_road_rows * racing_cell_height};
    RacingRect player{(320 - racing_car_width) / 2, 224 - racing_car_height, racing_car_width, racing_car_height};
    std::array<RacingTraffic, 1> traffic{};
    unsigned frame = 0;
    // Prototype speed units; road movement uses an additional 16/9 scale.
    unsigned speed = racing_start_speed, selected_speed = racing_low_speed, movement_remainder = 0, road_scroll = 0;
    unsigned frames_remaining = racing_duration_frames;
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
    // One-frame shift presses, supplied by the caller.
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
    static constexpr unsigned frames_per_second = racing_frames_per_second;
    static constexpr unsigned duration_frames = racing_duration_frames;
    static constexpr unsigned start_speed = racing_start_speed;
    static constexpr unsigned low_speed = racing_low_speed;
    static constexpr unsigned medium_speed = 161;
    static constexpr unsigned max_speed = 227;

    Racing() { reset(); }
    void reset();
    // Configuration persists across reset; enabling does not reopen a finished game.
    void set_endless(bool enabled) { endless_ = enabled; }
    void set_car_collision(bool enabled) { car_collision_ = enabled; }
    void reset_preview();
    void reset_from_layout(const RacingState& layout);
    void step_preview();
    // Advances exactly one frame; returns a bitmask of RacingEvent values.
    // After the timer expires, steps do nothing until reset(), unless endless is enabled.
    unsigned step(RacingInput input = {});
    const RacingState& state() const { return state_; }

private:
    RacingState state_{};
    bool endless_ = false;
    bool car_collision_ = true;
    unsigned next_lane_ = 0;
    int steering_remainder_ = 0;
};
}
