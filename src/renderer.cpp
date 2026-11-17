#include "renderer.h"
#include "game_catalog.h"
#include "hockey_ball.h"
#include "score_digits.h"
#include "tennis_a_blocks.h"
#include <algorithm>
#include <cstdio>

namespace ctg {
using namespace menu;
void Renderer::test_grid_overlay(bool racing) {
    const int columns = racing ? racing_road_columns : 41;
    const auto grid_x = [racing](int col) { return racing ? racing_x(col) : rally_x(col); };
    // Leave the last pixel of each cell untinted in either grid.
    const int tint_width = racing ? racing_cell_width - 1 : 5;
    const int rows = racing ? racing_road_rows : 50;
    const int tint_height = racing ? racing_cell_height - 1 : 3;
    const auto grid_y = [racing](int row) { return racing ? row * racing_cell_height : rally_y(row); };
    for (int row = 0; row < rows; ++row)
        for (int col = 0; col < columns; ++col)
            for (int dy = 0; dy < tint_height; ++dy)
                for (int dx = 0; dx < tint_width; ++dx) {
                    auto& p = pixels[(grid_y(row) + dy) * width + grid_x(col) + dx];
                    const auto blend = [](unsigned c) { return (c * 3 + 255) / 4; };
                    p = (blend((p >> 16) & 255) << 16) | (blend((p >> 8) & 255) << 8) | blend(p & 255);
                }
    constexpr unsigned char digits[10][5] = {
        {7, 5, 5, 5, 7}, {1, 1, 1, 1, 1}, {7, 1, 7, 4, 7},
        {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1}, {7, 4, 7, 1, 7},
        {7, 4, 7, 5, 7}, {7, 1, 1, 1, 1}, {7, 5, 7, 5, 7},
        {7, 5, 7, 1, 7}
    };
    const auto digit = [&](int x, int y, unsigned value) {
        for (int r = 0; r < 5; ++r)
            for (int c = 0; c < 3; ++c)
                if (digits[value][r] & (4u >> c))
                    rect(x + c, y + r, 1, 1, 0xffffff);
    };
    const auto backdrop = [this](int x, int y, int w, int h) {
        for (int row = y; row < y + h; ++row)
            for (int col = x; col < x + w; ++col) {
                auto& p = pixels[row * width + col];
                p = (p & 0xfefefeu) >> 1;
            }
    };
    const int column_label_y = racing ? 0 : 8;
    for (int col = 0; col < columns; ++col) {
        backdrop(grid_x(col), column_label_y, racing ? 4 : 5, 11);
        const int digit_x = grid_x(col) + (racing ? 0 : 1);
        digit(digit_x, column_label_y, col / 10);
        digit(digit_x, column_label_y + 6, col % 10);
    }
    const int row_label_x = racing ? racing_road_x - 8 : 29;
    for (int row = 0; row < rows; row += 2) {
        backdrop(row_label_x, grid_y(row), 7, 5);
        digit(row_label_x, grid_y(row), row / 10);
        digit(row_label_x + 4, grid_y(row), row % 10);
    }
}

void Renderer::rally_score_value(bool right, unsigned value, std::uint32_t color) {
    const auto digit = [&](int col, unsigned number) {
        for (int row = 0; row < 8; ++row)
            for (int cell = 0; cell < 4; ++cell)
                if (ctg::score_digits[number][row] & (8u >> cell))
                    rect(rally_x(col + cell), rally_y(6 + row), 6, 4, color);
    };
    const int ones_col = right ? 34 : (value >= 20 ? 8 : 5);
    if (value >= 10) digit(ones_col - 5, value / 10 % 10);
    digit(ones_col, value % 10);
}


void Renderer::render_tennis(const ctg::TennisB& tennis, bool show_score, ScoreOverride scores) {
    const auto& state = tennis.state();
    pixels.fill(state.ping_pong ? 0x706a6a : state.hockey ? 0x505068 : state.volleyball ? 0x19b537 : 0x0e7aab);
    rect(rally_x(0), rally_y(0), 246, 200, state.ping_pong ? 0x706a6a : state.hockey ? 0x2cc94c : state.volleyball ? 0x6d70f9 : 0xc07645);
    if (state.ping_pong) {
        rect(rally_x(0), rally_y(49), 246, 4, 0xffffff);
        rect(rally_x(20), rally_y(34), 6, 16, 0xffffff);
        rect(rally_x(20), rally_y(42), 6, 16, 0xffffff);
    } else if (state.hockey) {
        const int top = 21 + ctg::TennisB::hockey_goal_top * 188 / ctg::TennisB::height;
        const int bottom = 21 + ctg::TennisB::hockey_goal_bottom * 188 / ctg::TennisB::height;
        for (int x = rally_x(0); x < rally_x(41); ++x) {
            const int progress = std::min(x - rally_x(0), 25);
            const std::uint32_t wall = ((153 + 24 * progress / 25) << 16) | 0xff00 | (142 + 49 * progress / 25);
            rect(x, rally_y(0), 1, rally_y(1) - rally_y(0), wall);
            rect(x, rally_y(49), 1, rally_y(50) - rally_y(49), wall);
            if (x < rally_x(1) || x >= rally_x(40)) {
                rect(x, rally_y(1), 1, top - rally_y(1), wall);
                rect(x, bottom, 1, rally_y(49) - bottom, wall);
            }
        }

        if (state.hockey_a) {
            for (int i = 0; i < 6; ++i) {
                if (state.hockey_barrier_closed || (i != 2 && i != 3))
                    rect(rally_x(20), rally_y(2 + i * 8), 6, 16, hockey_boundary);
            }
        }
    } else if (state.volleyball) {
        rect(rally_x(0), rally_y(0), 246, 4, volleyball_boundary);
        rect(rally_x(0), rally_y(49), 246, 4, volleyball_boundary);
        if (state.volleyball_a) {
            for (int color = 0; color < 2; ++color)
                for (int row = 0; row < 6; ++row)
                    for (int col = 0; col < ctg::tennis_a_block_columns; ++col) {
                        const auto block = ctg::tennis_a_block(color, row, col);
                        rect(block.x, block.y, block.width, block.height, block.red ? 0xc84458 : 0x53c178);
                    }
        } else {
            for (int row = 2; row <= 42; row += 8) rect(rally_x(20), rally_y(row), 6, 16, volleyball_ball);
        }
    } else {
        rect(rally_x(0), rally_y(0), 246, 4, tennis_boundary);
        rect(rally_x(0), rally_y(49), 246, 4, tennis_boundary);
        if (state.tennis_a) {
            for (int color = 0; color < 2; ++color)
                for (int row = 0; row < 6; ++row)
                    for (int col = 0; col < ctg::tennis_a_block_columns; ++col) {
                        const auto block = ctg::tennis_a_block(color, row, col);
                        rect(block.x, block.y, block.width, block.height, block.red ? 0xc84458 : 0x53c178);
                    }
        } else {
            for (int row = 2; row <= 42; row += 8) rect(rally_x(20), rally_y(row), 6, 16, tennis_ball);
        }
    }

    if ((show_score && (state.serve_wait || state.winner != -1)) || scores.enabled) {
        const std::uint32_t score_color = state.ping_pong ? 0x8affa1 : state.hockey ? 0xaefbff : state.volleyball ? volleyball_score : 0xcdf0ff;
        rally_score_value(false, scores.enabled ? scores.left : state.score[0], score_color);
        rally_score_value(true, scores.enabled ? scores.right : state.score[1], score_color);
    }
    const auto draw_paddle = [&](int x, int owner, std::uint32_t color) {
        const int top = tennis.paddle_visible_top(owner);
        rect(x, top, 6, tennis.paddle_visible_bottom(owner) - top, color);
    };
    for (int i = 0; i < 2; ++i) {
        draw_paddle(rally_x(i == 0 ? 2 : 38), i,
             state.ping_pong ? 0x9b8cff : state.hockey ? (i == 0 ? 0xff9fff : 0xd5e87c) : state.volleyball ? 0xa6e6ff : 0xd0ecb2);
    }
    if (state.doubles) {
        for (int i = 0; i < 2; ++i) {
            if (state.hockey) {
                const int owner = 1 - i;
                draw_paddle(rally_x(i == 0 ? 14 : 26), owner,
                     owner == 0 ? 0xff9fff : 0xd5e87c);
            } else {
                draw_paddle(rally_x(i == 0 ? 12 : 28), i,
                                     state.ping_pong ? 0x9b8cff : state.volleyball ? 0xa6e6ff : 0xd0ecb2);
            }
        }
    }
    const int ball_x = ctg::hockey_x(state.ball_x);
    const int ball_y = ctg::hockey_y(state.ball_y);
    if (!state.serve_wait) {
        if (state.hockey) {
            ctg::draw_hockey_ball(pixels, ball_x, ball_y);
        } else {
            rect(ball_x - 3, ball_y - 2, 7, 5, state.ping_pong ? 0xffffff : state.volleyball ? volleyball_ball : tennis_ball);
        }
    }

}
void Renderer::render_shooting(const ctg::Shooting& shooting, bool show_score, ScoreOverride scores) {
    const auto& s = shooting.state();
    const std::uint32_t background = s.hit ? 0xcb4042 : 0x46a79a;
    const std::uint32_t objects = s.hit ? 0xffd5ff : 0xf3d66b;
    pixels.fill(background);
    rect(rally_x(0), rally_y(0), 246, 4, 0x86ffff);
    rect(rally_x(0), rally_y(49), 246, 4, 0x86ffff);
    if (show_score || scores.enabled) {
        rally_score_value(false, scores.enabled ? scores.left : s.shots, 0xffc9fa);
        rally_score_value(true, scores.enabled ? scores.right : s.hits, 0xffc9fa);
    }
    const int shooter_height = shooting.paddle_height(0) <= 16 ? 16 : 32;
    const int target_height = shooting.paddle_height(1) <= 16 ? 16 : 32;
    const auto paddle = [&](int x, int top, int height) {
        const int clipped_top = std::max(top, 20);
        const int clipped_bottom = std::min(top + height, 220);
        if (clipped_bottom > clipped_top) rect(x, clipped_top, 6, clipped_bottom - clipped_top, objects);
    };
    paddle(rally_x(2), shooting.shooter_top() + shooting.paddle_height(0) / 2 - shooter_height / 2, shooter_height);
    if (shooting.target_visible())
        paddle(rally_x(38), shooting.target_top() + shooting.paddle_height(1) / 2 - target_height / 2, target_height);
    if (s.flying) rect(s.shot_x, s.shot_y - 2, 7, 5, s.hit ? 0xffd5ff : 0x8ddade);

}
void Renderer::render_racing_score(const ctg::RacingState& s, ScoreOverride scores) {
    const unsigned score = scores.enabled ? scores.left : s.score;
    score_digit(racing_x(4), 4 * racing_cell_height, (score / 10) % 10, 0xffffff, racing_cell_width);
    score_digit(racing_x(12), 4 * racing_cell_height, score % 10, 0xffffff, racing_cell_width);
}
void Renderer::render_racing(const ctg::Racing& game, bool show_score, bool player_visible, ScoreOverride scores) {
    const auto& s = game.state();
    pixels.fill(0x66594a);
    rect(s.road.x, s.road.y, s.road.width, s.road.height, 0x39b800);
    constexpr int road_period = ctg::racing_road_dash_length + ctg::racing_road_gap_length;
    for (int y = -road_period + static_cast<int>(s.road_scroll); y < static_cast<int>(height); y += road_period) {
        rect(s.road.x, y, ctg::racing_road_border_width, ctg::racing_road_dash_length, 0xc3f5df);
        rect(s.road.x + s.road.width - ctg::racing_road_border_width, y, ctg::racing_road_border_width, ctg::racing_road_dash_length, 0xc3f5df);
    }
    const auto car_shape = [this](const ctg::RacingRect& car, std::uint32_t color, bool opponent) {
        const auto piece = [this, color, opponent](int x, int y, int w, int h) {
            const int top = opponent ? std::max(y, 9) : y;
            const int bottom = opponent ? std::min(y + h, 224) : y + h;
            if (bottom > top) rect(x, top, w, bottom - top, color);
        };
        constexpr int cw = ctg::racing_cell_width, ch = ctg::racing_cell_height;
        piece(car.x + 2 * cw, car.y, 2 * cw, 6 * ch);
        for (int row : {1, 4}) {
            piece(car.x, car.y + row * ch, cw, 2 * ch);
            piece(car.x + 5 * cw, car.y + row * ch, cw, 2 * ch);
            piece(car.x + cw, car.y + (row + 1) * ch - 1, cw, 2);
            piece(car.x + 4 * cw, car.y + (row + 1) * ch - 1, cw, 2);
        }
    };
    for (const auto& car : s.traffic)
        car_shape(car.rect, car.collided ? 0xffc782 : 0xf5ed48, true);
    if (!show_score || player_visible)
        car_shape(s.player, 0xeb65e7, false);
    if (show_score || scores.enabled) render_racing_score(s, scores);
}
constexpr unsigned selector_rows = 6;
unsigned selector_first(unsigned selector_choice) {
    return std::min(selector_choice > 2 ? selector_choice - 2 : 0,
                    static_cast<unsigned>(ctg::slot_count - selector_rows));
}
void Renderer::monochrome_selection_outline(const RenderState& state) {
    int x, y, w, h;
    if (state.selector) {
        const unsigned first = selector_first(state.selector_choice);
        x = 55; y = 88 + (state.selector_choice - first) * 16; w = 210; h = 13;
    } else if (state.paused || state.finished) {
        x = 75; y = 99 + (state.finished ? state.result_choice : state.pause_choice) * 16; w = 170; h = 14;
    } else {
        const auto layout = launch_layout(option_count(state.content_id), state.menu_choice);
        x = 53; w = 214;
        y = layout.y + (state.menu_choice - layout.first) * layout.spacing; h = 14;
    }
    rect(x, y, w, 1, 0xffffff);
    rect(x, y + h - 1, w, 1, 0xffffff);
    rect(x, y, 1, h, 0xffffff);
    rect(x + w - 1, y, 1, h, 0xffffff);
}
// MSVC optimized builds have dropped menu labels drawn before the selected row.
// Keep these menu paths unoptimized while retaining the pixel regression tests.
#if defined(_MSC_VER) && !defined(__clang__)
#pragma optimize("", off)
#endif
void Renderer::render(const RenderState& state) {
    const bool preview = state.selector || !state.detail;
    if (state.content_id == 15) {
        const bool background = preview || state.finished;
        render_racing(background ? state.demo_racing : state.racing, !background, state.racing_player_visible, state.scores);
        if (state.finished) render_racing_score(state.racing.state(), state.scores);
    }
    else if (state.content_id == 14) render_shooting(preview ? state.demo_shooting : state.shooting, !preview && (!state.finished || state.rally_score_visible), state.scores);
    else render_tennis(preview ? state.demo_tennis : state.tennis, !preview && state.rally_score_visible, state.scores);
    if (preview && state.menu_hidden) {
        centered_text(4, "DEMO MODE - PRESS ANY BUTTON TO SHOW MENU", 0xffffff);
        char label[64];
        std::snprintf(label, sizeof label, "%s%s%s", titles[state.content_id].name,
                      titles[state.content_id].variant[0] ? " " : "", titles[state.content_id].variant);
        centered_text(226, label, 0xffffff);
        return;
    }
    darken_panel();
    if (state.selector) {
        centered_text(58, "SELECT GAME", 0xffffff);
        const unsigned first = selector_first(state.selector_choice);
        if (first) arrow(77, true);
        for (unsigned i = 0; i < selector_rows; ++i) {
            unsigned id = selector_order[first + i];
            char label[64];
            std::snprintf(label, sizeof label, "%s%s%s", titles[id].name,
                          titles[id].variant[0] ? " " : "", titles[id].variant);
            if (first + i == state.selector_choice) rect(55, 88 + i * 16, 210, 13, 0x295166);
            centered_text(91 + i * 16, label, first + i == state.selector_choice ? 0x70e0ff : 0xffffff);
        }
        if (first + selector_rows < ctg::slot_count) arrow(187, false);
    } else if (state.paused || state.finished) {
        const bool finished = state.finished;
        centered_text(70, finished ? (state.content_id >= 14 ? "GAME OVER" : state.tennis.state().winner == 0 ? "PLAYER 1 WINS" : "PLAYER 2 WINS") : "PAUSED", 0xffffff);
        if (finished && state.content_id == 14) {
            centered_text(83, "SHOTS                HITS", 0xffffff);
        }
        const char* pause_choices[] = {"RESUME", "RESTART", "EXIT"};
        const char* result_choices[] = {"RESTART", "EXIT"};
        const unsigned choice = finished ? state.result_choice : state.pause_choice;
        const unsigned count = finished ? 2 : 3;
        for (unsigned i = 0; i < count; ++i) {
            if (i == choice) rect(75, 99 + i * 16, 170, 14, 0x295166);
            centered_text(102 + i * 16, finished ? result_choices[i] : pause_choices[i], i == choice ? 0x70e0ff : 0xffffff);
        }
    } else {
        centered_text(58, titles[state.content_id].name, 0xffffff);
        centered_text(70, titles[state.content_id].variant, 0xffffff);
        if (state.content_id == 15) {
            centered_text(145, "PLAYABLE PROTOTYPE", 0xffffff);
            centered_text(157, "IN DEVELOPMENT", 0xffffff);
        }
        const unsigned count = option_count(state.content_id);
        const auto layout = launch_layout(count, state.menu_choice);
        const unsigned first = layout.first;
        if (layout.scroll && first) arrow(87, true);
        for (unsigned row = 0; row < layout.rows && first + row < count; ++row) {
            const unsigned i = first + row, y = layout.y + row * layout.spacing;
            if (i == state.menu_choice) rect(53, y, 214, 14, 0x295166);
            const std::uint32_t color = i == state.menu_choice ? 0x70e0ff : 0xffffff;
            if (i == 0) {
                centered_text(y + 3, "RETURN", color);
                horizontal_arrow(130, y + 6, true, color);
            } else if (i == 1) {
                centered_text(y + 3, "START", color);
            } else {
                if (state.content_id == 14) {
                    const char* targets[] = {"MANUAL", "SLOW", "NORMAL", "FAST", "RANDOM"};
                    const char* directions[] = {"DOWN", "UP", "RANDOM"};
                    const char* blinkings[] = {"OFF", "ON", "RANDOM"};
                    const char* labels[] = {"TARGET", "DIRECTION", "BLINKING", "SPEED", "P1 SIZE", "P2 SIZE", "SHOTS"};
                    char shots[4];
                    std::snprintf(shots, sizeof shots, "%u", state.shot_limit);
                    const char* values[] = {targets[state.shooting_target], directions[state.shooting_direction], blinkings[state.shooting_blinking],
                                            state.high_speed ? "HIGH" : "LOW", state.small_paddle[0] ? "SMALL" : "LARGE",
                                            state.small_paddle[1] ? "SMALL" : "LARGE", shots};
                    const std::uint32_t row_color = i == shooting_direction_row && state.shooting_target == 0 ? 0x888888 : color;
                    text(87, y + 3, labels[i - shooting_target_row], row_color);
                    horizontal_arrow(151, y + 6, true, row_color);
                    text(167, y + 3, values[i - shooting_target_row], row_color);
                    horizontal_arrow(218, y + 6, false, row_color);
                    continue;
                }
                if (state.content_id >= state.opponent_modes.size()) continue;
                const unsigned setting = i - 2;
                const bool mode = setting == 0;
                const unsigned option = setting - 1;
                const bool score = i == 6;
                const char* label = mode ? "MODE" : score ? "SCORE" : option == 0 ? "SPEED" : option == 1 ? "P1 SIZE" : "P2 SIZE";
                const char* modes[] = {"2 PLAYERS", "1P EASY", "1P NORMAL", "1P HARD"};
                char score_text[4];
                std::snprintf(score_text, sizeof score_text, "%u", state.winning_score);
                const char* value = mode ? modes[state.opponent_modes[state.content_id]] : score ? score_text : option == 0 ? (state.high_speed ? "HIGH" : "LOW") : (state.small_paddle[option - 1] ? "SMALL" : "LARGE");
                if (mode) {
                    text(87, y + 3, label, color);
                    horizontal_arrow(151, y + 6, true, color);
                    text(161, y + 3, value, color);
                    horizontal_arrow(218, y + 6, false, color);
                } else {
                    text(87, y + 3, label, color);
                    horizontal_arrow(151, y + 6, true, color);
                    text(167, y + 3, value, color);
                    horizontal_arrow(218, y + 6, false, color);
                }
            }
        }
        if (layout.scroll && first + layout.rows < count) arrow(181, false);
    }
}
void Renderer::diagnostic_menu(bool grid, ScoreOverride scores, bool car_collision, unsigned choice) {
    rect(50, 54, 220, 150, 0x101820);
    centered_text(64, "TEST MENU", 0xffffff);
    char labels[6][40];
    std::snprintf(labels[0], sizeof labels[0], "GRID: %s", grid ? "ON" : "OFF");
    std::snprintf(labels[1], sizeof labels[1], "FIXED SCORE: %s", scores.enabled ? "ON" : "OFF");
    std::snprintf(labels[2], sizeof labels[2], "LEFT SCORE: %u", scores.left);
    std::snprintf(labels[3], sizeof labels[3], "RIGHT SCORE: %u", scores.right);
    std::snprintf(labels[4], sizeof labels[4], "CAR COLLISION: %s", car_collision ? "ON" : "OFF");
    std::snprintf(labels[5], sizeof labels[5], "EXIT");
    for (unsigned i = 0; i < 6; ++i) {
        if (i == choice) rect(59, 82 + i * 18, 202, 14, 0x295166);
        centered_text(85 + i * 18, labels[i], i == choice ? 0x70e0ff : 0xffffff);
    }
}
#if defined(_MSC_VER) && !defined(__clang__)
#pragma optimize("", on)
#endif
}
