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
enum ShootingRow : unsigned { shooting_return, shooting_start, shooting_target_row, shooting_direction_row,
                              shooting_blinking_row, shooting_speed_row, shooting_p1_size_row,
                              shooting_p2_size_row, shooting_shots_row, shooting_row_count };
constexpr unsigned option_count(unsigned id) { return id == 15 ? 2u : id == 14 ? static_cast<unsigned>(shooting_row_count) : 7u; }
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
    unsigned winning_score, shot_limit, shooting_target, shooting_direction, shooting_blinking;
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
