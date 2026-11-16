#include "audio.h"
#include "content.h"
#include "demo.h"
#include "controls.h"
#include "core_options.h"
#include "generator.h"
#include "preferences.h"
#include "renderer.h"
#include "game_catalog.h"
#include "libretro.h"
#include "tennis_b.h"
#include "opponent.h"
#include "shooting.h"
#include "racing.h"
#include <algorithm>
#include <cstdio>
#include <array>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <filesystem>
#include <random>


namespace {
constexpr unsigned width = ctg::Graphics::width, height = ctg::Graphics::height;
using ctg::selector_order;
using namespace ctg::menu;
retro_environment_t environment = nullptr;
bool option_known = false, generate_selected = false;

retro_video_refresh_t video = nullptr;
retro_audio_sample_t audio = nullptr;
retro_audio_sample_batch_t audio_batch = nullptr;
ctg::Controls controls;
ctg::Renderer renderer;
auto& pixels = renderer.pixels;
unsigned selector_position(unsigned id) {
    return static_cast<unsigned>(std::find(selector_order.begin(), selector_order.end(), id) - selector_order.begin());
}
unsigned content_id = 0, previous = 0;
bool loaded = false, detail = false;
unsigned shooting_target = 0, shooting_direction = 0, shooting_blinking = 1;
std::mt19937 shooting_random{std::random_device{}()};
bool selector = false, paused = false;
bool test_grid = false, test_fixed_score = false, test_menu = false;
unsigned test_left_score = 0, test_right_score = 0, test_choice = 0;
std::array<std::uint32_t, width * height> test_background{}, test_clean_frame{};
unsigned selector_choice = 0, menu_choice = 1, pause_choice = 0, result_choice = 0;
unsigned rotation_frames = 0;
unsigned rally_result_frames = 0;
bool playable() { return content_id < ctg::slot_count; }
retro_rumble_interface rumble{};
std::array<unsigned, 2> rumble_frames{};
bool rumble_enabled = false;
constexpr unsigned sound_mask = ctg::sound_wall | ctg::sound_paddle | ctg::sound_point | ctg::sound_win | ctg::sound_shot;

void stop_rumble(unsigned port) {
    if (rumble_frames[port] && rumble.set_rumble_state)
        rumble.set_rumble_state(port, RETRO_RUMBLE_STRONG, 0);
    rumble_frames[port] = 0;
}
void stop_rumble() {
    for (unsigned port = 0; port < rumble_frames.size(); ++port) stop_rumble(port);
}
void pulse_rumble(unsigned port) {
    if (!rumble_enabled || !controls.enabled(port) || !rumble.set_rumble_state) return;
    rumble.set_rumble_state(port, RETRO_RUMBLE_STRONG, 0x8000);
    rumble_frames[port] = 6;
}
void advance_rumble() {
    for (unsigned port = 0; port < rumble_frames.size(); ++port)
        if (rumble_frames[port] && --rumble_frames[port] == 0 && rumble.set_rumble_state)
            rumble.set_rumble_state(port, RETRO_RUMBLE_STRONG, 0);
}
ctg::TennisB tennis;
ctg::Opponent opponent;
ctg::RallyDifficulty rally_difficulty;
std::array<unsigned, 14> opponent_modes{};
ctg::Shooting shooting;
ctg::Racing racing;
ctg::Demo demo;
bool paddle_movement_rebound = false;
bool anti_stalemate = true;
unsigned inactivity_frames = 0;
bool menu_hidden = false, suppress_wake_input = false;
bool demo_sounds = true;
bool shooting_fire_sound = true;
ctg::Audio audio_engine;
void reset_demo() {
    controls.reset_racing();
    audio_engine.clear_demo();
    demo.reset();
    inactivity_frames = rotation_frames = 0;
    menu_hidden = suppress_wake_input = false;
}
void step_demo(unsigned id) {
    if (demo.current_id() != id) audio_engine.clear_demo();
    unsigned events = demo.step(id, paddle_movement_rebound, anti_stalemate);
    if (id == 14 && !shooting_fire_sound) events &= ~ctg::sound_shot;
    if (demo_sounds && events) audio_engine.trigger_demo(events, id);
}
bool high_speed = false;
std::array<bool, 2> small_paddle{};
unsigned winning_score = ctg::TennisB::winning_score;
unsigned shot_limit = ctg::Shooting::default_shot_limit;
bool preferences_dirty = false;
void apply_options() { tennis.set_anti_stalemate(anti_stalemate); tennis.set_rebound_mode(paddle_movement_rebound); tennis.set_options(high_speed, small_paddle); tennis.set_winning_score(winning_score); shooting.set_options(high_speed, small_paddle); shooting.set_shot_limit(shot_limit); }
unsigned option_count() { return ctg::menu::option_count(content_id); }
void resolve_shooting_settings() {
    const unsigned target = shooting_target == 4 ? std::uniform_int_distribution<unsigned>(1, 3)(shooting_random) : shooting_target;
    const unsigned direction = shooting_target != 0 && shooting_direction == 2 ? std::uniform_int_distribution<unsigned>(0, 1)(shooting_random) : shooting_direction;
    const unsigned blinking = shooting_blinking == 2 ? std::uniform_int_distribution<unsigned>(0, 1)(shooting_random) : shooting_blinking;
    const unsigned speeds[] = {11, 6, 11, 16};
    shooting.set_target_behavior(speeds[target], direction == 0 ? 1 : -1, blinking != 0);
}
void start_shooting_match() {
    resolve_shooting_settings();
    shooting.reset();
}
bool result() { return detail && (content_id == 15 ? racing.state().finished : content_id == 14 ? shooting.state().finished : content_id < opponent_modes.size() && tennis.state().winner != -1); }
void start_racing_match(bool from_preview = true) {
    racing.reset_from_layout(from_preview ? demo.racing.state() : racing.state());
    audio_engine.start_racing();
}

ctg::RenderState render_state() {
    return {content_id, selector, detail, paused, result(), menu_hidden,
            selector_choice, menu_choice, pause_choice, result_choice, high_speed, small_paddle,
            winning_score, shot_limit, shooting_target, shooting_direction, shooting_blinking,
            opponent_modes, tennis, demo.tennis, shooting, demo.shooting, racing, demo.racing,
            audio_engine.racing_player_visible(), {test_fixed_score, test_left_score, test_right_score},
            rally_result_frames < 24};
}
void select_game(unsigned id) {
    content_id = id;
    opponent.reset(); rally_difficulty.reset();
    tennis = ctg::make_rally_game(id);
    apply_options();
    shooting.reset();
    racing.reset();
    controls.reset_racing();
    audio_engine.clear_game_tone();
    if (environment) environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, const_cast<retro_input_descriptor*>(controls.descriptors(content_id)));
}
void message(const char* s) {
    if (environment) { retro_message msg{s, 180}; environment(RETRO_ENVIRONMENT_SET_MESSAGE, &msg); }
}
std::mt19937& random_engine() {
    static std::mt19937 engine([] {
        auto seed = static_cast<std::mt19937::result_type>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count());
        try { seed ^= std::random_device{}(); } catch (...) {}
        return seed;
    }());
    return engine;
}

std::filesystem::path save_path(const char* name) {
    const char* directory = nullptr;
    if (!environment || !environment(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &directory) ||
        !directory || !*directory) return {};
    return std::filesystem::u8path(directory) / name;
}

unsigned read_last_game() noexcept {
    try {
        return ctg::preferences::read_last_game(save_path("colortvgame-last-game"), selector_order[0]);
    } catch (...) { return selector_order[0]; }
}

bool write_last_game(unsigned id) noexcept {
    try {
        return ctg::preferences::write_last_game(save_path("colortvgame-last-game"), id, random_engine());
    } catch (...) { return false; }
}

using PreferenceData = ctg::preferences::Data;
PreferenceData preference_data() {
    PreferenceData data{{'C', 'T', 'G', 'P', 1,
        static_cast<unsigned char>(high_speed), static_cast<unsigned char>(small_paddle[0]),
        static_cast<unsigned char>(small_paddle[1]), static_cast<unsigned char>(winning_score),
        static_cast<unsigned char>(shot_limit), static_cast<unsigned char>(shooting_target),
        static_cast<unsigned char>(shooting_direction), static_cast<unsigned char>(shooting_blinking)}};
    for (std::size_t i = 0; i < opponent_modes.size(); ++i)
        data[13 + i] = static_cast<unsigned char>(opponent_modes[i]);
    return data;
}
void read_preferences() noexcept {
    if (preferences_dirty) return;
    // Invalid or unavailable files leave the current session's preferences intact.
    try {
        PreferenceData data{};
        if (!ctg::preferences::read(save_path("colortvgame-preferences"), data)) return;
        high_speed = data[5]; small_paddle = {{bool(data[6]), bool(data[7])}};
        winning_score = data[8]; shot_limit = data[9]; shooting_target = data[10];
        shooting_direction = data[11]; shooting_blinking = data[12];
        for (std::size_t i = 0; i < opponent_modes.size(); ++i) opponent_modes[i] = data[13 + i];
        apply_options();
    } catch (...) {}
}
void save_preferences(bool notify = true) {
    const auto data = preference_data();
    try {
        preferences_dirty = !ctg::preferences::write(save_path("colortvgame-preferences"), data, random_engine());
    } catch (...) { preferences_dirty = true; }
    if (preferences_dirty && notify) message("Could not save menu preferences");
}
void retry_preferences() {
    if (preferences_dirty) save_preferences(false);
}

unsigned initial_selector_choice() {
    retro_variable variable{"colortvgame_start_game", nullptr};
    const char* value = environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE, &variable) ? variable.value : nullptr;
    if (value && std::strcmp(value, "First") == 0) return 0;
    if (value && std::strcmp(value, "Random") == 0) {
        static unsigned last_random = ctg::slot_count;
        unsigned choice;
        if (last_random >= ctg::slot_count)
            choice = std::uniform_int_distribution<unsigned>(0, ctg::slot_count - 1)(random_engine());
        else {
            choice = std::uniform_int_distribution<unsigned>(0, ctg::slot_count - 2)(random_engine());
            if (choice >= last_random) ++choice;
        }
        return last_random = choice;
    }
    return selector_position(read_last_game());
}

void generate_game_files() {
    const char* directory = nullptr;
    if (environment && !environment(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &directory)) directory = nullptr;
    const auto result = ctg::generate_game_files(directory);
    char text[160];
    std::snprintf(text, sizeof text, "Game files: %u created, %u existing, %u failed%s",
                  result.created, result.existing, result.failed,
                  !directory || !*directory ? " (system directory unavailable)" : "");
    message(text);
}
void update_generate_option() {

    if (!environment) { option_known = false; return; }
    retro_variable variable{"colortvgame_generate_files", nullptr};
    if (!environment(RETRO_ENVIRONMENT_GET_VARIABLE, &variable) || !variable.value ||
        (std::strcmp(variable.value, "Ready") != 0 && std::strcmp(variable.value, "Generate") != 0)) {
        option_known = false;
        return;
    }
    const bool selected = std::strcmp(variable.value, "Generate") == 0;
    if (option_known && !generate_selected && selected) {
        generate_game_files();
        retro_variable reset{"colortvgame_generate_files", "Ready"};
        generate_selected = !environment(RETRO_ENVIRONMENT_SET_VARIABLE, &reset);
    } else generate_selected = selected;
    option_known = true;
}
}

extern "C" {
void retro_set_environment(retro_environment_t cb) {
    environment = cb;
    option_known = false;
    demo_sounds = true;
    audio_engine.clear_demo();
    renderer.rf_noise_frame = 0;
    if (environment) {
        bool supported = true;
        environment(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &supported);
        ctg::core_options::register_options(environment);
    }
}
void retro_set_video_refresh(retro_video_refresh_t cb) { video = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { audio = cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { controls.set_input_poll(cb); }
void retro_set_input_state(retro_input_state_t cb) { controls.set_input_state(cb); }
void retro_init() {
    test_grid = test_fixed_score = test_menu = false;
    test_left_score = test_right_score = test_choice = 0;
    preferences_dirty = false;
    rumble = {};
    rumble_frames = {};
    rumble_enabled = false;
    if (environment) environment(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &rumble);
    reset_demo();
    renderer.rf_noise_frame = 0;
    option_known = false;
    selector_choice = 0;
    opponent_modes = {};
    opponent.reset();
    rally_difficulty.reset();
    high_speed = false;
    small_paddle = {};
    winning_score = ctg::TennisB::winning_score;
    shot_limit = ctg::Shooting::default_shot_limit;
    apply_options();
    controls.reset_repeat();
    content_id = previous = 0;
    loaded = detail = selector = paused = false;
    shooting_target = 0;
    shooting_direction = 0;
    shooting_blinking = 1;
    menu_choice = 1;
    pause_choice = result_choice = 0;
    controls.set_enabled(0, true);
    controls.set_enabled(1, true);
    tennis.reset();
    shooting.reset();
    audio_engine.clear_game_tone();
}
void retro_deinit() {
    retry_preferences();
    stop_rumble();
    rumble = {};
    rumble_enabled = false;
    reset_demo();
    demo_sounds = false;
    loaded = detail = false;
    previous = content_id = 0;
}
unsigned retro_api_version() { return RETRO_API_VERSION; }
void retro_get_system_info(retro_system_info* info) { *info = {"Color TV-Game Simulator", CORE_VERSION, "ctg", false, false}; }
void retro_get_system_av_info(retro_system_av_info* info) { *info = {{width, height, width, height, 4.0f / 3.0f}, {60.0, 48000.0}}; }
void retro_set_controller_port_device(unsigned port, unsigned device) { if (port < rumble_frames.size()) { stop_rumble(port); controls.set_device(port, device); } }
void retro_reset() {
    test_menu = false;
    stop_rumble();
    reset_demo();
    opponent.reset();
    rally_difficulty.reset();
    controls.reset_repeat();
    detail = false;
    selector = paused = false;
    menu_choice = 1;
    tennis.reset();
    shooting.reset();
    racing.reset();
    audio_engine.clear_game_tone();
}
void retro_run() {
    retro_variable vibration_option{"colortvgame_vibration", nullptr};
    rumble_enabled = environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE, &vibration_option) &&
                     vibration_option.value && std::strcmp(vibration_option.value, "On") == 0;
    if (!rumble_enabled) stop_rumble();
    else advance_rumble();
    update_generate_option();
    retro_variable rebound_option{"colortvgame_rebound", nullptr};
    const bool rebound = environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE, &rebound_option) &&
                         rebound_option.value && std::strcmp(rebound_option.value, "Paddle movement") == 0;
    if (rebound != paddle_movement_rebound) {
        paddle_movement_rebound = rebound;
        tennis.set_rebound_mode(rebound);
        demo.tennis.set_rebound_mode(rebound);
    }
    retro_variable anti_stalemate_option{"colortvgame_anti_stalemate", nullptr};
    const bool enabled = !(environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE, &anti_stalemate_option) &&
                           anti_stalemate_option.value && std::strcmp(anti_stalemate_option.value, "Off") == 0);
    if (enabled != anti_stalemate) {
        anti_stalemate = enabled;
        tennis.set_anti_stalemate(enabled);
        demo.tennis.set_anti_stalemate(enabled);
    }
    retro_variable fire_option{"colortvgame_shooting_fire_sound", nullptr};
    shooting_fire_sound = !(environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE, &fire_option) &&
                            fire_option.value && std::strcmp(fire_option.value, "Off") == 0);
    retro_variable demo_option{"colortvgame_demo_sounds", nullptr};
    demo_sounds = !(environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE, &demo_option) &&
                    demo_option.value && std::strcmp(demo_option.value, "Off") == 0);
    if (!demo_sounds) audio_engine.clear_demo();
    retro_variable rotation_option{"colortvgame_demo_rotation", nullptr};
    const char* rotation_value = environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE, &rotation_option)
                                     ? rotation_option.value : nullptr;
    const bool rotation_enabled = !rotation_value || std::strcmp(rotation_value, "Off") != 0;
    const bool rotation_sequential = rotation_value && std::strcmp(rotation_value, "Every 15 seconds") == 0;
    tennis.set_endless(test_fixed_score);
    shooting.set_endless(test_fixed_score);
    racing.set_endless(test_fixed_score);
    controls.poll();
    const unsigned buttons = controls.read_buttons({detail, paused, result(), test_menu, content_id});
    const unsigned chord = (1u << RETRO_DEVICE_ID_JOYPAD_START) |
                           (1u << RETRO_DEVICE_ID_JOYPAD_L2) | (1u << RETRO_DEVICE_ID_JOYPAD_R2);
    const bool open_test = loaded && !test_menu && (buttons & chord) == chord && (previous & chord) != chord;
    if (open_test) {
        test_menu = true;
        test_choice = 0;
        test_background = test_clean_frame;
        stop_rumble();
    }
    if (test_menu) {
        const unsigned pressed_test = open_test ? 0 : buttons & ~previous;
        previous = buttons;
        const unsigned up = 1u << RETRO_DEVICE_ID_JOYPAD_UP, down = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
        const unsigned left = 1u << RETRO_DEVICE_ID_JOYPAD_LEFT, right = 1u << RETRO_DEVICE_ID_JOYPAD_RIGHT;
        const unsigned confirm = 1u << RETRO_DEVICE_ID_JOYPAD_A, back = 1u << RETRO_DEVICE_ID_JOYPAD_B;
        if (pressed_test & back) test_menu = false;
        else if (pressed_test & up) test_choice = (test_choice + 4) % 5;
        else if (pressed_test & down) test_choice = (test_choice + 1) % 5;
        else if (pressed_test & (left | right | confirm)) {
            const int delta = (pressed_test & right) ? 1 : (pressed_test & left) ? -1 : 0;
            if (test_choice == 0) test_grid = !test_grid;
            else if (test_choice == 1) test_fixed_score = !test_fixed_score;
            else if (test_choice == 2 && delta) test_left_score = static_cast<unsigned>(std::clamp(static_cast<int>(test_left_score) + delta, 0, 30));
            else if (test_choice == 3 && delta) test_right_score = static_cast<unsigned>(std::clamp(static_cast<int>(test_right_score) + delta, 0, 30));
            else if (test_choice == 4 && (pressed_test & confirm)) test_menu = false;
        }
        pixels = test_background;
        if (test_menu) renderer.diagnostic_menu(test_grid, {test_fixed_score, test_left_score, test_right_score}, test_choice);
        if (test_grid) renderer.test_grid_overlay();
        if (video) video(pixels.data(), width, height, width * sizeof(pixels[0]));
        std::array<std::int16_t, 1600> silence{};
        if (audio_batch) audio_batch(silence.data(), 800);
        else if (audio) for (unsigned i = 0; i < 800; ++i) audio(0, 0);
        return;
    }
    if (result() && content_id <= 14 && !paused)
        rally_result_frames = (rally_result_frames + 1) % 32;
    else if (!result())
        rally_result_frames = 0;
    const bool preview_menu = loaded && (selector || !detail);
    const bool any_input = controls.any_input(preview_menu, buttons);
    const bool was_hidden = menu_hidden;
    if (!preview_menu) {
        inactivity_frames = 0;
        if (content_id != 15 || !result()) demo.reset();
        menu_hidden = suppress_wake_input = false;
    } else if (any_input) {
        inactivity_frames = 0;
        if (menu_hidden) { menu_hidden = false; suppress_wake_input = true; }
    } else {
        suppress_wake_input = false;
        if (inactivity_frames < 600 && ++inactivity_frames == 600) menu_hidden = true;
    }
    if (!loaded || !selector || !rotation_enabled || !menu_hidden) rotation_frames = 0;
    else if (was_hidden && ++rotation_frames == 900) {
        rotation_frames = 0;
        if (rotation_sequential) selector_choice = (selector_choice + 1) % ctg::slot_count;
        else {
            unsigned choice = std::uniform_int_distribution<unsigned>(0, ctg::slot_count - 2)(random_engine());
            if (choice >= selector_choice) ++choice;
            selector_choice = choice;
        }
    }
    const unsigned pressed = suppress_wake_input ? 0 : buttons & ~previous;
    previous = buttons;
    const int menu = !loaded ? -1 : selector ? 0 : result() ? 1 : paused ? 2 : detail ? -1 : 3;
    const unsigned navigation = controls.menu_navigation(menu, buttons, pressed, suppress_wake_input);
    if (loaded) {
        const unsigned up = 1u << RETRO_DEVICE_ID_JOYPAD_UP, down = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
        const unsigned confirm = (1u << RETRO_DEVICE_ID_JOYPAD_A) | (1u << RETRO_DEVICE_ID_JOYPAD_START);
        const unsigned back = 1u << RETRO_DEVICE_ID_JOYPAD_B;
        if (selector) {
            if (navigation & (up | down)) {
                selector_choice = (selector_choice + ((navigation & up) ? ctg::slot_count - 1 : 1)) % ctg::slot_count;
                if (!write_last_game(selector_order[selector_choice])) message("Could not save last selected game");
            } else if (pressed & confirm) { selector = false; menu_choice = 1; }
            if (selector_order[selector_choice] != content_id) select_game(selector_order[selector_choice]);
        } else if (result()) {
            if (navigation & (up | down)) result_choice = 1 - result_choice;
            else if (pressed & (1u << RETRO_DEVICE_ID_JOYPAD_A)) {
                if (result_choice == 1) { detail = false; menu_choice = 1; }
                rally_difficulty.reset(); tennis.reset(); opponent.reset();
                if (content_id == 14 && result_choice == 0) start_shooting_match(); else shooting.reset();
                if (content_id == 15 && result_choice == 0) start_racing_match(); else racing.reset();
                audio_engine.clear_game_tone(); result_choice = 0;
            }
        } else if (paused) {
            if (pressed & ((1u << 31) | (1u << RETRO_DEVICE_ID_JOYPAD_START))) paused = false;
            else if (navigation & up) pause_choice = (pause_choice + 2) % 3;
            else if (navigation & down) pause_choice = (pause_choice + 1) % 3;
            else if (pressed & (1u << RETRO_DEVICE_ID_JOYPAD_A)) {
                paused = false;
                if (pause_choice != 0) {
                    if (pause_choice == 2) { detail = false; menu_choice = 1; }
                    rally_difficulty.reset(); tennis.reset(); opponent.reset();
                    if (content_id == 14 && pause_choice == 1) start_shooting_match(); else shooting.reset();
                    if (content_id == 15 && pause_choice == 1) start_racing_match(false); else racing.reset();
                    audio_engine.clear_game_tone();
                }
            }
        } else if (detail) {
            if (pressed & ((1u << RETRO_DEVICE_ID_JOYPAD_START) | (1u << 31))) { paused = true; pause_choice = 0; }
            else if (content_id == 15) {
                const ctg::RacingInput steering = controls.racing_input(audio_engine.racing_recovering());
                const unsigned previous_score = racing.state().score;
                unsigned events = ctg::racing_none;
                if (audio_engine.racing_starting()) racing.step_preview();
                else events = racing.step(steering);
                if (previous_score < 50 && racing.state().score >= 50) {
                    audio_engine.bonus();
                }
                if (events & ctg::racing_collision) {
                    pulse_rumble(0);
                    audio_engine.collision();
                }
                if (events & ctg::racing_finished) result_choice = 0;
            } else if (content_id == 14) {
                {
                    const int direction = controls.shooting_direction(0);
                    const int target_direction = controls.shooting_direction(1);
                    unsigned events = shooting.step(direction, pressed & ((1u << RETRO_DEVICE_ID_JOYPAD_A) | (1u << RETRO_DEVICE_ID_JOYPAD_Y)), target_direction, shooting_target != 0);
                    if (events & ctg::event_player1_shot) pulse_rumble(0);
                    if (events & ctg::sound_paddle) resolve_shooting_settings();
                    events &= sound_mask;
                    if (!shooting_fire_sound) events &= ~ctg::sound_shot;
                    if (events) {
                        if (events & ctg::sound_win) { result_choice = 0; rally_result_frames = 0; }
                        audio_engine.trigger_game(events, true);
                    }
                }
            } else if (content_id < opponent_modes.size()) {
                {
                    int directions[2]{controls.paddle_direction(0), controls.paddle_direction(1)};
                    if (opponent_modes[content_id]) {
                        const unsigned difficulty = rally_difficulty.effective(opponent_modes[content_id]);
                        directions[1] = opponent.step(tennis.state(), difficulty, difficulty == 1);
                    }
                    const ctg::TennisState before = tennis.state();
                    unsigned events = tennis.step({directions[0], directions[1]});
                    rally_difficulty.advance(before, events & sound_mask);
                    if (events & ctg::event_player1_hit) pulse_rumble(0);
                    if (!opponent_modes[content_id] && (events & ctg::event_player2_hit)) pulse_rumble(1);
                    events &= sound_mask;
                    if (events) {
                        if (events & ctg::sound_win) { result_choice = 0; rally_result_frames = 0; }
                        audio_engine.trigger_game(events, false);
                    }
                }
            }
        } else {
            unsigned count = option_count();
            if (pressed & back) { selector = true; selector_choice = selector_position(content_id); select_game(content_id); }
            else if (navigation & up) menu_choice = (menu_choice + count - 1) % count;
            else if (navigation & down) menu_choice = (menu_choice + 1) % count;
            else if (content_id < 15 && menu_choice >= 2 && (pressed & ((1u << RETRO_DEVICE_ID_JOYPAD_LEFT) | (1u << RETRO_DEVICE_ID_JOYPAD_RIGHT)))) {
                const auto before = preference_data();
                if (content_id == 14) {
                    const int delta = (pressed & (1u << RETRO_DEVICE_ID_JOYPAD_RIGHT)) ? 1 : -1;
                    switch (menu_choice) {
                    case shooting_target_row: shooting_target = (shooting_target + 5 + delta) % 5; break;
                    case shooting_direction_row: if (shooting_target != 0) shooting_direction = (shooting_direction + 3 + delta) % 3; break;
                    case shooting_blinking_row: shooting_blinking = (shooting_blinking + 3 + delta) % 3; break;
                    case shooting_speed_row: high_speed = !high_speed; apply_options(); break;
                    case shooting_p1_size_row: case shooting_p2_size_row:
                        small_paddle[menu_choice - shooting_p1_size_row] = !small_paddle[menu_choice - shooting_p1_size_row]; apply_options(); break;
                    case shooting_shots_row: shot_limit = ((shot_limit / 5 - 1 + 6 + delta) % 6 + 1) * 5; apply_options(); break;
                    }
                } else if (menu_choice == 2) {
                    const int delta = (pressed & (1u << RETRO_DEVICE_ID_JOYPAD_RIGHT)) ? 1 : -1;
                    opponent_modes[content_id] = (opponent_modes[content_id] + 4 + delta) % 4;
                } else if (menu_choice == 6) {
                    const int delta = (pressed & (1u << RETRO_DEVICE_ID_JOYPAD_RIGHT)) ? 1 : -1;
                    unsigned& value = content_id == 14 ? shot_limit : winning_score;
                    value = ((value / 5 - 1 + 6 + delta) % 6 + 1) * 5;
                    apply_options();
                } else {
                    const unsigned option = menu_choice - 3;
                    if (option == 0) high_speed = !high_speed;
                    else small_paddle[option - 1] = !small_paddle[option - 1];
                    apply_options();
                }
                if (preference_data() != before) save_preferences();
            }
            else if (pressed & confirm) {
                if (menu_choice == 0) { selector = true; selector_choice = selector_position(content_id); select_game(content_id); }
                else if (menu_choice == 1) { detail = true; opponent.reset(); rally_difficulty.reset(); if (content_id == 14) start_shooting_match(); else if (content_id == 15) start_racing_match(); else tennis.reset(); }
            }
        }
        const bool preview = selector || !detail;
        if (preview || paused || result()) stop_rumble();
        if (preview || (content_id == 15 && result())) step_demo(content_id);
        else audio_engine.clear_demo();
        const auto view = render_state();
        if (detail && !paused && !result()) {
            if (content_id == 15) renderer.render_racing(racing, true, view.racing_player_visible, view.scores);
            else if (content_id == 14) renderer.render_shooting(shooting, true, view.scores);
            else if (playable()) renderer.render_tennis(tennis, true, view.scores);
            else renderer.render(view);
        } else renderer.render(view);
        retro_variable color_option{"colortvgame_color", nullptr};
        const bool monochrome = environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE, &color_option) &&
                                color_option.value && std::strcmp(color_option.value, "Off") == 0;
        retro_variable rf_option{"colortvgame_rf_filter", nullptr};
        const bool rf_filter = environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE, &rf_option) &&
                               rf_option.value && std::strcmp(rf_option.value, "On") == 0;
        const bool show_outline = (selector || !detail || paused || result()) &&
                                  !((selector || !detail) && menu_hidden);
        if (rf_filter) {
            unsigned quality = 100;
            retro_variable quality_option{"colortvgame_rf_quality", nullptr};
            if (environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE, &quality_option) && quality_option.value) {
                static constexpr const char* values[] = {"100%", "90%", "80%", "70%", "60%", "50%", "40%", "30%", "20%", "10%", "0%"};
                for (unsigned index = 0; index < 11; ++index)
                    if (std::strcmp(quality_option.value, values[index]) == 0) { quality = 100 - 10 * index; break; }
            }
            renderer.apply_rf_filter(quality);
        }
        if (monochrome) {
            renderer.grayscale();
            if (show_outline) renderer.monochrome_selection_outline(view);
        }
        test_clean_frame = pixels;
        if (test_grid) renderer.test_grid_overlay();
        if (video) video(pixels.data(), width, height, width * sizeof(pixels[0]));
        const auto samples = audio_engine.render(preview, result(), paused, content_id, racing.state().speed);
        if (audio_batch) audio_batch(samples.data(), 800);
        else if (audio) for (unsigned i = 0; i < 800; ++i) audio(samples[2 * i], samples[2 * i + 1]);
    }
}
size_t retro_serialize_size() { return 0; }
bool retro_serialize(void*, size_t) { return false; }
bool retro_unserialize(const void*, size_t) { return false; }
void retro_cheat_reset() {}
void retro_cheat_set(unsigned, bool, const char*) {}
bool retro_load_game(const retro_game_info* game) {
    stop_rumble();
    reset_demo();
    opponent.reset(); rally_difficulty.reset();
    controls.reset_repeat();
    test_menu = false;
    loaded = detail = selector = paused = false; menu_choice = 1; pause_choice = 0; content_id = previous = 0; audio_engine.clear_game_tone(); tennis.reset(); shooting.reset(); racing.reset();
    auto id = game ? ctg::parse_content(game->data, game->size) : std::optional<std::uint8_t>{0};
    if (!id) { message("Invalid CTG1 content: expected five bytes and ID 0-15"); return false; }
    retro_pixel_format format = RETRO_PIXEL_FORMAT_XRGB8888;
    if (!environment || !environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &format)) { message("XRGB8888 video is required"); return false; }
    if (preferences_dirty) retry_preferences();
    else read_preferences();
    selector = !game;
    selector_choice = selector ? initial_selector_choice() : selector_position(*id);
    select_game(selector ? selector_order[selector_choice] : *id);
    loaded = true; return true;
}
bool retro_load_game_special(unsigned, const retro_game_info*, size_t) { return false; }
void retro_unload_game() {
    test_menu = false;
    retry_preferences(); stop_rumble(); reset_demo(); loaded = detail = false; content_id = previous = 0; audio_engine.clear_game_tone(); tennis.reset(); racing.reset(); }
unsigned retro_get_region() { return RETRO_REGION_NTSC; }
void* retro_get_memory_data(unsigned) { return nullptr; }
size_t retro_get_memory_size(unsigned) { return 0; }
}
