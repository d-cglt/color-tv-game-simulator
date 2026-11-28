#include "../src/core.cpp"
#include <cstdio>

#include <string>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

namespace {
bool mode = false;
unsigned buttons = 0, buttons2 = 0;
unsigned options_version = 0, registered_command = 0, display_requests = 0;
bool vibration = true, stereo = true, fire_sound = true;
bool unequal_samples = false;
std::string directory;
bool callback(unsigned command, void* data) {
    if (command == RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION) { *static_cast<unsigned*>(data) = options_version; return true; }
    if (command == RETRO_ENVIRONMENT_SET_VARIABLES || command == RETRO_ENVIRONMENT_SET_CORE_OPTIONS || command == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2) { registered_command = command; return true; }
    if (command == RETRO_ENVIRONMENT_GET_VARIABLE) {
        auto* variable = static_cast<retro_variable*>(data);
        if (std::strcmp(variable->key, "colortvgame_original_features") == 0) variable->value = mode ? "On" : "Off";
        else if (std::strcmp(variable->key, "colortvgame_vibration") == 0) variable->value = vibration ? "On" : "Off";
        else if (std::strcmp(variable->key, "colortvgame_stereo") == 0) variable->value = stereo ? "Yes" : "No";
        else if (std::strcmp(variable->key, "colortvgame_shooting_fire_sound") == 0) variable->value = fire_sound ? "On" : "Off";
        else return false;
        return true;
    }
    if (command == RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY) { *static_cast<const char**>(data) = directory.c_str(); return true; }
    if (command == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY) {
        ++display_requests;
        return true;
    }
    return command == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT;
}
int16_t input(unsigned port, unsigned device, unsigned, unsigned id) {
    return port < 2 && device == RETRO_DEVICE_JOYPAD && id < 16 && ((port ? buttons2 : buttons) & (1u << id)) ? 1 : 0;
}
void press(unsigned id) { buttons = 1u << id; retro_run(); buttons = 0; retro_run(); }
size_t capture_audio(const int16_t* samples, size_t frames) {
    for (size_t i = 0; i < frames; ++i) if (samples[2 * i] != samples[2 * i + 1]) unequal_samples = true;
    return frames;
}
bool frame_is_stereo() {
    unequal_samples = false;
    audio_engine.trigger_game(ctg::sound_paddle, false, 0);
    retro_run();
    return unequal_samples;
}
}

int main() {
    directory = (std::filesystem::temp_directory_path() / ("ctg-original-test-" + std::to_string(std::random_device{}()))).string();
    CHECK(std::filesystem::create_directories(directory));
    retro_set_environment(callback);
    retro_set_input_state(input);
    retro_set_audio_sample_batch(capture_audio);
    retro_init();
    CHECK(retro_load_game(nullptr));
    selector = false; select_game(4);
    detail = true;
    for (unsigned version : {0u, 1u, 2u}) {
        options_version = version; retro_set_environment(callback);
        CHECK(registered_command == (version == 2 ? RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2 : version == 1 ? RETRO_ENVIRONMENT_SET_CORE_OPTIONS : RETRO_ENVIRONMENT_SET_VARIABLES));
        mode = false; vibration = stereo = fire_sound = true; retro_run();
        CHECK(rumble_enabled && shooting_fire_sound && frame_is_stereo());
        mode = true; retro_run();
        CHECK(!rumble_enabled && !shooting_fire_sound && !frame_is_stereo());
        vibration = stereo = fire_sound = false; retro_run();
        CHECK(!rumble_enabled && !shooting_fire_sound && !frame_is_stereo());
        mode = false; retro_run();
        CHECK(!rumble_enabled && !shooting_fire_sound && !frame_is_stereo());
        mode = true; retro_run();
        vibration = stereo = fire_sound = true; retro_run();
        CHECK(!rumble_enabled && !shooting_fire_sound && !frame_is_stereo());
        mode = false; retro_run();
        CHECK(rumble_enabled && shooting_fire_sound && frame_is_stereo());
        CHECK(display_requests == 0);
    }
    detail = false; options_version = 0; retro_set_environment(callback);
    winning_score = 30; shot_limit = 25; opponent_modes[4] = 3; ice_hockey = true;
    shooting_target = 4; shooting_direction = 2; shooting_size = shooting_speed = 2;
    small_paddle = {true, false};
    save_preferences(false);
    const auto saved = preference_data();
    mode = true; retro_run();
    CHECK(original_features && !rumble_enabled && !shooting_fire_sound);
    CHECK(registered_command == RETRO_ENVIRONMENT_SET_VARIABLES);
    for (unsigned version : {1u, 2u, 0u}) {
        options_version = version; retro_set_environment(callback); retro_run();
        CHECK(registered_command == (version == 2 ? RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2 : version == 1 ? RETRO_ENVIRONMENT_SET_CORE_OPTIONS : RETRO_ENVIRONMENT_SET_VARIABLES));
        CHECK(original_features && !rumble_enabled && !shooting_fire_sound);
    }
    CHECK(display_requests == 0);
    CHECK(preference_data() == saved && !tennis.state().ice_hockey && tennis.target_score() == 15);
    CHECK(tennis.state().small_paddle == small_paddle);
    detail = true; tennis.reset();
    const int before_p2 = tennis.state().paddle_y[1];
    buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run(); buttons2 = 0;
    CHECK(tennis.state().paddle_y[1] > before_p2);
    auto& active_rally = const_cast<ctg::TennisState&>(tennis.state());
    active_rally.serve_wait = 0; active_rally.ball_x = 200; active_rally.ball_y = 30;
    active_rally.ball_vx = 4; active_rally.ball_vy = 0;
    const int stationary_p2 = tennis.state().paddle_y[1];
    for (int i = 0; i < 20; ++i) retro_run();
    CHECK(tennis.state().paddle_y[1] == stationary_p2);
    mode = false; retro_run(); CHECK(tennis.target_score() == 30 && tennis.state().ice_hockey);
    mode = true; retro_run(); CHECK(tennis.target_score() == 15 && tennis.state().score[0] == 0 && !tennis.state().ice_hockey);
    detail = false;
    CHECK(effective_target() == 2 && effective_direction() == 0 && effective_size() == 0 && effective_speed() == 0);
    for (unsigned target : {1u, 3u, 4u}) { shooting_target = target; CHECK(effective_target() == 2); }
    shooting_target = 4;
    CHECK(ctg::menu::visible_count(4, true, false) == 5);
    menu_choice = 1; press(RETRO_DEVICE_ID_JOYPAD_DOWN); CHECK(menu_choice == 3);
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    CHECK(high_speed && shooting_target == 4 && shooting_size == 2 && shooting_speed == 2);
    press(RETRO_DEVICE_ID_JOYPAD_UP); CHECK(menu_choice == 1);
    direct_launch_locked = true;
    CHECK(ctg::menu::visible_count(4, true, true) == 4);
    CHECK(ctg::menu::visible_row(4, 0, true, true) == 1);
    direct_launch_locked = false;
    select_game(4);
    auto& rally = const_cast<ctg::TennisState&>(tennis.state());
    rally.serve_wait = 0; rally.score[0] = 14; rally.ball_x = 375; rally.ball_y = 120; rally.ball_vx = 4; rally.ball_vy = 0;
    CHECK(tennis.step() & ctg::sound_win); CHECK(rally.score[0] == 15 && rally.winner == 0);
    for (unsigned id = 0; id < 14; ++id) {
        select_game(id);
        CHECK(tennis.target_score() == 15 && !tennis.state().ice_hockey && tennis.state().small_paddle == small_paddle);
    }
    select_game(14);
    CHECK(ctg::menu::visible_count(14, true, false) == 5);
    menu_choice = 2; press(RETRO_DEVICE_ID_JOYPAD_DOWN); CHECK(menu_choice == 4);
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    CHECK(shooting_size == 1 && shooting_target == 4 && shooting_speed == 2);
    CHECK(shooting.paddle_height(0) == 16 && shooting.paddle_height(1) == 16);
    CHECK(demo.shooting.paddle_height(0) == 16 && demo.shooting.paddle_height(1) == 16);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); CHECK(menu_choice == 5);
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    CHECK(shooting_speed == 1 && shooting_target == 4 && shooting_size == 1);
    CHECK(shooting.paddle_height(0) == 16 && shooting.paddle_height(1) == 16);
    CHECK(demo.shooting.paddle_height(0) == 16 && demo.shooting.paddle_height(1) == 16);
    reset_demo();
    for (unsigned speed = 0; speed < 2; ++speed) for (unsigned size = 0; size < 2; ++size) {
        shooting_speed = speed; shooting_size = size; resolve_shooting_settings(); apply_options();
        auto check_sizes = [size](const ctg::Shooting& game) {
            return game.paddle_height(0) == (size ? 16 : 32) && game.paddle_height(1) == (size ? 16 : 33);
        };
        CHECK(check_sizes(shooting));
        shooting.reset(); shooting.step(0, true); const int x = shooting.state().shot_x;
        shooting.step(0, false); CHECK(shooting.state().shot_x - x == (speed ? 6 : 3));
        while (shooting.state().shot_x < 256) shooting.step(0, false);
        auto& game_shot = const_cast<ctg::ShootingState&>(shooting.state());
        game_shot.shot_y = shooting.target_top() + shooting.paddle_height(1) / 2;
        detail = true;
        for (int i = 0; i < 4 && game_shot.hits == 0; ++i) retro_run();
        CHECK(game_shot.hits == 1 && check_sizes(shooting));
        CHECK(shooting_resolved_high_speed == bool(speed) && shooting_resolved_small == bool(size));
        detail = false;
        step_demo(14);
        CHECK(check_sizes(demo.shooting));
        demo.shooting.reset(); demo.shooting.step(0, true); const int demo_x = demo.shooting.state().shot_x;
        demo.shooting.step(0, false); CHECK(demo.shooting.state().shot_x - demo_x == (speed ? 6 : 3));
        while (demo.shooting.state().shot_x < 256) demo.shooting.step(0, false);
        auto& preview_shot = const_cast<ctg::ShootingState&>(demo.shooting.state());
        preview_shot.shot_y = demo.shooting.target_top() + demo.shooting.paddle_height(1) / 2;
        for (int i = 0; i < 4 && preview_shot.hits == 0; ++i) step_demo(14);
        CHECK(preview_shot.hits == 1 && check_sizes(demo.shooting));
        const int hit_x = preview_shot.shot_x;
        step_demo(14); CHECK(preview_shot.shot_x - hit_x == (speed ? 6 : 3));
    }
    shooting_size = shooting_speed = 0; apply_options(); start_shooting_match();
    shooting.set_target_behavior(11, 1, false);
    shooting.step(0, true);
    while (shooting.state().shot_x < 256) shooting.step(0, false);
    auto& shot = const_cast<ctg::ShootingState&>(shooting.state());
    shot.shot_y = shooting.target_top() + shooting.paddle_height(1) / 2;
    while (shot.flying) shooting.step(0, false);
    CHECK(shot.hits == 1);
    start_shooting_match();
    for (int shot = 0; shot < 15; ++shot) {
        shooting.step(0, true);
        while (shooting.state().flying) shooting.step(0, false);
        CHECK(shooting.state().finished == (shot == 14));
    }
    // The Racing mode and switches remain available and unaffected.
    select_game(15); racing_mode = 2; racing_bad_road = true; start_racing_match(false);
    CHECK(racing.state().two_players && racing_bad_road);
    detail = true; const auto race_frame = racing.state().frame;
    mode = false; retro_run(); mode = true; retro_run();
    CHECK(original_features && racing.state().two_players && racing.state().frame >= race_frame);
    detail = false;
    // An unrelated save while overridden must persist the underlying excluded values.
    shooting_size = shooting_speed = 2;
    high_speed = true; save_preferences(false);
    retro_unload_game(); retro_deinit(); retro_init(); CHECK(retro_load_game(nullptr));
    retro_run();
    CHECK(original_features && winning_score == 30 && shot_limit == 25 && opponent_modes[4] == 3 && ice_hockey);
    CHECK(shooting_target == 4 && shooting_direction == 2 && shooting_size == 2 && shooting_speed == 2 && high_speed);
    mode = false; retro_run();
    CHECK(!original_features && display_requests == 0);
    CHECK(effective_target() == 4 && effective_direction() == 2 && effective_size() == 2 && effective_speed() == 2);
    select_game(4); CHECK(tennis.state().ice_hockey);
    retro_unload_game(); retro_deinit();
    std::filesystem::remove_all(directory);
    return 0;
}
