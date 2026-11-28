#pragma once

#include "graphics.h"
#include "tennis_b.h"
#include "shooting.h"
#include "racing.h"

namespace ctg {
constexpr int rally_x(int col) { return 37 + col * 6; }
constexpr int rally_y(int row) { return 20 + row * 4; }
inline constexpr std::uint32_t tennis_boundary = 0xdad2b0;
inline constexpr std::uint32_t tennis_ball = 0xfae7b4;
inline constexpr std::uint32_t volleyball_boundary = 0xcddcfb;
inline constexpr std::uint32_t volleyball_ball = 0xc7dbfd;
inline constexpr std::uint32_t volleyball_score = 0xedfd84;
inline constexpr std::uint32_t hockey_boundary = 0x98ff8d;

namespace menu {
enum RacingRow : unsigned { racing_return, racing_start, racing_mode_row, racing_speed_row,
                            racing_bad_road_row, racing_wall_crash_row, racing_opponents_row, racing_car_path_row,
                            racing_row_count };
enum ShootingRow : unsigned { shooting_return, shooting_start, shooting_target_row, shooting_direction_row,
                              shooting_target_size_row, shooting_speed_row, shooting_shots_row, shooting_row_count };
constexpr unsigned option_count(unsigned id) { return id == 15 ? static_cast<unsigned>(racing_row_count) : id == 14 ? static_cast<unsigned>(shooting_row_count) : (id >= 4 && id <= 7) ? 8u : 7u; }
constexpr bool row_visible(unsigned id, unsigned row, bool original, bool locked) {
    if (locked && row == 0) return false;
    if (!original || id == 15) return true;
    return id == 14 ? row != shooting_direction_row && row != shooting_shots_row
                    : row != 2 && row != 6 && row != 7;
}
constexpr unsigned visible_count(unsigned id, bool original, bool locked) {
    unsigned count = 0;
    for (unsigned row = 0; row < option_count(id); ++row) count += row_visible(id, row, original, locked);
    return count;
}
constexpr unsigned visible_row(unsigned id, unsigned index, bool original, bool locked) {
    for (unsigned row = 0; row < option_count(id); ++row)
        if (row_visible(id, row, original, locked) && index-- == 0) return row;
    return 1;
}
constexpr unsigned visible_index(unsigned id, unsigned choice, bool original, bool locked) {
    unsigned index = 0;
    for (unsigned row = 0; row < choice && row < option_count(id); ++row)
        index += row_visible(id, row, original, locked);
    return index;
}

struct LaunchLayout {
    unsigned first, rows, y, spacing;
    bool scroll;
};
constexpr LaunchLayout launch_layout(unsigned count, unsigned choice) {
    return count <= 7 ? LaunchLayout{0, count, count == 7 ? 88u : 98u, count == 7 ? 15u : 16u, false}
                      : LaunchLayout{choice >= 5 ? choice - 4 : 0, 5, 98, 16, true};
}
static_assert(launch_layout(7, 6).first == 0 && launch_layout(7, 6).rows == 7 &&
              !launch_layout(7, 6).scroll && launch_layout(7, 6).y > 76 &&
              launch_layout(7, 6).y + 6 * launch_layout(7, 6).spacing + 14 <= 204);
static_assert(launch_layout(8, 7).scroll && launch_layout(8, 7).first == 3 &&
              launch_layout(8, 7).first + launch_layout(8, 7).rows == 8);
}

struct ScoreOverride {
    bool enabled = false;
    unsigned left = 0, right = 0;
};

// A read-only snapshot: rendering never changes the session or game simulation.
struct RenderState {
    unsigned content_id;
    bool selector, detail, paused, finished, menu_hidden;
    unsigned selector_choice, menu_choice, pause_choice, result_choice;
    bool high_speed;
    std::array<bool, 2> small_paddle;
    unsigned winning_score, shot_limit, shooting_target, shooting_direction, shooting_size, shooting_speed, racing_mode;
    const std::array<unsigned, 14>& opponent_modes;
    const TennisB& tennis;
    const TennisB& demo_tennis;
    const Shooting& shooting;
    const Shooting& demo_shooting;
    const Racing& racing;
    const Racing& demo_racing;
    bool racing_player_visible;
    ScoreOverride scores;
    bool rally_score_visible = true;
    unsigned racing_speed = 2; // HIGH lever selection: 0 low, 1 medium, 2 high.
    bool racing_more_cars = false;
    bool racing_wall_crash = false;
    bool racing_zigzag = false;
    bool racing_bad_road = false;
    bool direct_launch_locked = false;
    bool ice_hockey = false;
    bool original_features = false;
};

class Renderer : public Graphics {
public:
    void test_grid_overlay(bool racing = false);
    void rally_score_value(bool right, unsigned value, std::uint32_t color);
    void render_tennis(const TennisB& game, bool show_score = true, ScoreOverride scores = {});
    void render_shooting(const Shooting& game, bool show_score = true, ScoreOverride scores = {});
    void render_racing_score(const RacingState& state, ScoreOverride scores = {});
    void render_racing(const Racing& game, bool show_score = true, bool player_visible = true, ScoreOverride scores = {});
    void render(const RenderState& state);
    void monochrome_selection_outline(const RenderState& state);
    void diagnostic_menu(bool grid, ScoreOverride scores, bool car_collision, unsigned choice);
};
}
