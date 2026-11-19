#pragma once
#include <array>
#include <random>

namespace ctg {
constexpr int racing_cell_width = 4;
constexpr int racing_cell_height = 5;
constexpr int racing_road_rows = 48;
constexpr int racing_car_width = 6 * racing_cell_width;
constexpr int racing_car_height = 6 * racing_cell_height;
constexpr int racing_road_x = 54;
constexpr int racing_road_columns = 53;
constexpr int racing_center_column = 26;
constexpr int racing_road_border_width = racing_cell_width;
constexpr int racing_x(int col) { return racing_road_x + col * racing_cell_width; }
constexpr unsigned racing_frames_per_second = 60;
constexpr unsigned racing_blink_frame_samples = 800;
constexpr unsigned racing_blink_segment_samples = 12960;
constexpr unsigned racing_blink_total_samples = 6 * racing_blink_segment_samples;
constexpr unsigned racing_duration_frames = 64 * racing_frames_per_second;
constexpr unsigned racing_start_speed = 30;
constexpr unsigned racing_low_speed = 136;
constexpr unsigned racing_road_dash_length = 240;
constexpr unsigned racing_road_gap_length = 6 * racing_cell_height;
constexpr int racing_opponent_gap_offset = 10 * racing_cell_height;

// All rectangles use top-left coordinates and half-open pixel bounds in a
// 320x240 playfield. Traffic moves toward the bottom of the screen.
struct RacingRect {
    int x, y, width, height;
};


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
    bool narrow = false; // Left-hand road, with a boundary at racing_x(26).
    bool two_players = false;
    RacingRect player2{(320 - racing_car_width) / 2, 224 - racing_car_height, racing_car_width, racing_car_height};
    std::array<RacingTraffic, 1> traffic2{};
    unsigned score2 = 0, collisions2 = 0;
    bool section_collided2 = false;
    unsigned blink_samples = 0, blink_samples2 = 0;
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
    static constexpr unsigned medium_speed = 166;
    static constexpr unsigned max_speed = 233;

    Racing() { reset(); }
    void reset();
    // Configuration persists across reset; enabling does not reopen a finished game.
    void set_endless(bool enabled) { endless_ = enabled; }
    void set_car_collision(bool enabled) { car_collision_ = enabled; }
    // HIGH lever speed: 0 low, 1 medium, 2 high; LOW is always low.
    // In two-player mode both lanes accelerate to the switch-selected speed;
    // either player's collision slows the shared speed during recovery.
    void set_speed_switch(unsigned setting);
    void set_random_seed(unsigned seed) { random_.seed(seed); }
    // Switch geometry immediately; configuration persists across reset and preview.
    void set_narrow(bool enabled);
    // Changing mode resets the game. Two players use separate halves of the road;
    // narrow geometry is ignored until returning to single-player mode.
    void set_two_players(bool enabled);
    void reset_preview();
    // Restores both players' positions in two-player mode, clamped to their lanes.
    void reset_from_layout(const RacingState& layout);
    void step_preview(RacingInput input = {}, RacingInput player2 = {});
    // Advances exactly one frame; returns a bitmask of RacingEvent values.
    // After the timer expires, steps do nothing until reset(), unless endless is enabled.
    // Collision and pass event bits indicate an event for either player.
    unsigned step(RacingInput input = {}, RacingInput player2 = {});
    const RacingState& state() const { return state_; }

private:
    void steer(RacingInput input, bool second = false);
    int opponent_x(bool second = false);
    RacingState state_{};
    bool endless_ = false;
    bool car_collision_ = true;
    bool narrow_ = false;
    bool two_players_ = false;
    unsigned high_gear_speed_ = max_speed;
    std::mt19937 random_{1};
    int steering_remainder_ = 0, steering_remainder2_ = 0;
};
}
