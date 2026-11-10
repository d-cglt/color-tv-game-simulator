#include "content.h"
#include "hockey_ball.h"
#include "libretro.h"
#include <array>
#include <chrono>
#include <filesystem>
#include <string>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <utility>
#include <vector>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)
namespace {
bool support = false, accept_format = true;
bool system_available = true;
std::string system_directory, last_message;
unsigned system_queries = 0, set_variable_calls = 0;
bool set_variable_supported = true, variable_updated = false;
std::string generate_value = "Ready", last_set_key, last_set_value, generate_definition;
std::string demo_sounds_value, demo_sounds_definition;
std::string rebound_value, rebound_definition;
std::string anti_stalemate_value, anti_stalemate_definition;
unsigned core_options_version = 0;
bool anti_stalemate_modern_definition = false;
void set_rebound(const char* value) { rebound_value = value; variable_updated = true; }
std::string color_value = "On", color_definition;
std::string rf_filter_value, rf_filter_definition;
std::string rf_quality_value = "100%", rf_quality_definition;
void set_rf_quality(const char* value) { rf_quality_value = value; variable_updated = true; }
void set_rf_filter(const char* value) { rf_filter_value = value; variable_updated = true; }
void set_color(const char* value) { color_value = value; variable_updated = true; }
std::string demo_rotation_value = "Off", demo_rotation_definition;
std::string start_game_value = "First", start_game_definition, save_directory;
bool save_available = false;
void set_demo_sounds(const char* value) { demo_sounds_value = value; variable_updated = true; }
void set_generate_value(const char* value) { generate_value = value; variable_updated = true; }
unsigned buttons = 0, buttons2 = 0, frames = 0, polls = 0, audio_frames = 0, messages = 0, shutdowns = 0, nonzero_audio = 0;
std::array<int16_t, 2> axes{};
std::array<bool, RETROK_LAST> keys{};
std::vector<unsigned char> frame;
std::vector<retro_input_descriptor> descriptors;
bool env(unsigned cmd, void* data) {
    if (cmd == RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION) {
        *static_cast<unsigned*>(data) = core_options_version; return true;
    }
    if (cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS || cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2) {
        auto check = [](const auto* definitions) {
            for (auto* d = definitions; d->key; ++d) {
                if (std::strcmp(d->key, "colortvgame_anti_stalemate") == 0)
                    return std::strcmp(d->default_value, "On") == 0 &&
                           std::strcmp(d->values[0].value, "On") == 0 &&
                           std::strcmp(d->values[1].value, "Off") == 0 && !d->values[2].value;
            }
            return false;
        };
        anti_stalemate_modern_definition = cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS
            ? check(static_cast<retro_core_option_definition*>(data))
            : check(static_cast<retro_core_options_v2*>(data)->definitions);
        return true;
    }

    if (cmd == RETRO_ENVIRONMENT_SET_VARIABLES) {
        for (auto* variable = static_cast<retro_variable*>(data); variable->key; ++variable) {
            if (std::strcmp(variable->key, "colortvgame_generate_files") == 0) generate_definition = variable->value;
            if (std::strcmp(variable->key, "colortvgame_demo_sounds") == 0) demo_sounds_definition = variable->value;
            if (std::strcmp(variable->key, "colortvgame_rebound") == 0) rebound_definition = variable->value;
            if (std::strcmp(variable->key, "colortvgame_anti_stalemate") == 0) anti_stalemate_definition = variable->value;
            if (std::strcmp(variable->key, "colortvgame_color") == 0) color_definition = variable->value;
            if (std::strcmp(variable->key, "colortvgame_rf_filter") == 0) rf_filter_definition = variable->value;
            if (std::strcmp(variable->key, "colortvgame_rf_quality") == 0) rf_quality_definition = variable->value;
            if (std::strcmp(variable->key, "colortvgame_demo_rotation") == 0) demo_rotation_definition = variable->value;
            if (std::strcmp(variable->key, "colortvgame_start_game") == 0) start_game_definition = variable->value;
        }
        return true;
    }
    if (cmd == RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE) {
        *static_cast<bool*>(data) = variable_updated; variable_updated = false; return true;
    }
    if (cmd == RETRO_ENVIRONMENT_GET_VARIABLE) {
        auto* variable = static_cast<retro_variable*>(data);
        if (std::strcmp(variable->key, "colortvgame_generate_files") == 0) variable->value = generate_value.c_str();
        else if (std::strcmp(variable->key, "colortvgame_demo_sounds") == 0 && !demo_sounds_value.empty()) variable->value = demo_sounds_value.c_str();
        else if (std::strcmp(variable->key, "colortvgame_rebound") == 0 && !rebound_value.empty()) variable->value = rebound_value.c_str();
        else if (std::strcmp(variable->key, "colortvgame_anti_stalemate") == 0 && !anti_stalemate_value.empty()) variable->value = anti_stalemate_value.c_str();
        else if (std::strcmp(variable->key, "colortvgame_color") == 0 && !color_value.empty()) variable->value = color_value.c_str();
        else if (std::strcmp(variable->key, "colortvgame_rf_filter") == 0 && !rf_filter_value.empty()) variable->value = rf_filter_value.c_str();
        else if (std::strcmp(variable->key, "colortvgame_rf_quality") == 0 && !rf_quality_value.empty()) variable->value = rf_quality_value.c_str();
        else if (std::strcmp(variable->key, "colortvgame_demo_rotation") == 0 && !demo_rotation_value.empty()) variable->value = demo_rotation_value.c_str();
        else if (std::strcmp(variable->key, "colortvgame_start_game") == 0 && !start_game_value.empty()) variable->value = start_game_value.c_str();
        else return false;
        return true;
    }
    if (cmd == RETRO_ENVIRONMENT_SET_VARIABLE) {
        auto* variable = static_cast<retro_variable*>(data);
        ++set_variable_calls; last_set_key = variable->key; last_set_value = variable->value;
        if (!set_variable_supported) return false;
        if (last_set_key != "colortvgame_generate_files") return false;
        generate_value = last_set_value; variable_updated = true; return true;
    }
    if (cmd == RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY) {
        ++system_queries;
        if (!system_available) return false;
        *static_cast<const char**>(data) = system_directory.c_str(); return true;
    }
    if (cmd == RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY) {
        if (!save_available) return false;
        *static_cast<const char**>(data) = save_directory.c_str(); return true;
    }
    if (cmd == RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME) { support = *static_cast<bool*>(data); return true; }
    if (cmd == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT) return accept_format && *static_cast<retro_pixel_format*>(data) == RETRO_PIXEL_FORMAT_XRGB8888;
    if (cmd == RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS) {
        descriptors.clear();
        for (auto* d = static_cast<retro_input_descriptor*>(data); d->description; ++d) descriptors.push_back(*d);
        return true;
    }
    if (cmd == RETRO_ENVIRONMENT_SET_MESSAGE) { ++messages; last_message = static_cast<retro_message*>(data)->msg; return true; }
    if (cmd == RETRO_ENVIRONMENT_SHUTDOWN) { ++shutdowns; return true; }
    return false;
}
void video(const void* data, unsigned w, unsigned h, size_t pitch) {
    if (data && w == 320 && h == 240 && pitch == 1280) {
        const auto* p = static_cast<const unsigned char*>(data);
        frame.assign(p, p + pitch * h); ++frames;
    }
}
void poll() { ++polls; }
int16_t input(unsigned port, unsigned device, unsigned index, unsigned id) {
    if (port >= 2) return 0;
    if (device == RETRO_DEVICE_KEYBOARD) return port == 0 && id < keys.size() && keys[id];
    if (device == RETRO_DEVICE_ANALOG) return index == RETRO_DEVICE_INDEX_ANALOG_LEFT && id == RETRO_DEVICE_ID_ANALOG_Y ? axes[port] : 0;
    return index == 0 && device == RETRO_DEVICE_JOYPAD && id < 32 && ((port == 0 ? buttons : buttons2) & (1u << id)) ? 1 : 0;
}
size_t audio(const int16_t* data, size_t n) {
    for (size_t i = 0; i < n * 2; ++i) if (data[i] != 0) ++nonzero_audio;
    audio_frames += static_cast<unsigned>(n); return n;
}
}
int main() {
    namespace fs = std::filesystem;
    const auto temp = fs::temp_directory_path() / ("ctg-core-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    CHECK(fs::create_directory(temp));
    system_directory = temp.u8string();
    // An initial Generate value is a baseline, not an action.
    set_generate_value("Generate");
    retro_set_environment(env); retro_set_video_refresh(video); retro_set_audio_sample_batch(audio);
    retro_set_input_poll(poll); retro_set_input_state(input); retro_init();
    CHECK(generate_definition.find("; Ready|Generate") != std::string::npos);
    CHECK(rebound_definition == "Rebound behavior; Hit position|Paddle movement");
    CHECK(anti_stalemate_definition == "Anti-stalemate; On|Off");
    for (unsigned version : {1u, 2u}) {
        core_options_version = version; anti_stalemate_modern_definition = false;
        retro_set_environment(env);
        CHECK(anti_stalemate_modern_definition);
    }
    core_options_version = 0; retro_set_environment(env);
    CHECK(retro_load_game(nullptr)); retro_run();
    CHECK(system_queries == 0 && set_variable_calls == 0 && !fs::exists(temp / "Nintendo Color TV-Game"));
    for (int i = 0; i < 3; ++i) retro_run();
    CHECK(system_queries == 0);
    set_generate_value("Ready"); retro_run();
    set_generate_value("Generate"); retro_run();
    CHECK(system_queries == 1 && last_message.find("15 created, 0 existing, 0 failed") != std::string::npos);
    CHECK(fs::exists(temp / "Nintendo Color TV-Game" / "Tennis Variant - Singles (Color TV Game 15).ctg"));
    CHECK(set_variable_calls == 1 && last_set_key == "colortvgame_generate_files" && last_set_value == "Ready" && generate_value == "Ready");
    for (int i = 0; i < 30; ++i) retro_run();
    CHECK(system_queries == 1 && set_variable_calls == 1);
    set_generate_value("Generate"); retro_run();
    CHECK(system_queries == 2 && last_message.find("0 created, 15 existing, 0 failed") != std::string::npos);
    CHECK(set_variable_calls == 2 && generate_value == "Ready");
    retro_run();
    system_available = false;
    set_generate_value("Generate"); retro_run();
    CHECK(system_queries == 3 && last_message.find("15 failed") != std::string::npos && last_message.find("system directory unavailable") != std::string::npos);
    CHECK(set_variable_calls == 3 && last_set_value == "Ready" && generate_value == "Ready");
    system_available = true;

    // Older frontends cannot reset the option: a held Generate must not repeat,
    // but an explicit Ready -> Generate transition must trigger again.
    set_variable_supported = false; retro_run();
    set_generate_value("Generate"); retro_run();
    CHECK(system_queries == 4 && set_variable_calls == 4 && generate_value == "Generate");
    for (int i = 0; i < 30; ++i) retro_run();
    CHECK(system_queries == 4 && set_variable_calls == 4);
    set_generate_value("Ready"); retro_run();
    set_generate_value("Generate"); retro_run();
    CHECK(system_queries == 5 && set_variable_calls == 5 && last_set_key == "colortvgame_generate_files" && last_set_value == "Ready");
    CHECK(last_message.find("0 created, 15 existing, 0 failed") != std::string::npos);
    set_variable_supported = true; set_generate_value("Ready");
    retro_unload_game(); retro_deinit(); fs::remove_all(temp);
    frames = polls = audio_frames = messages = 0;
    std::array<unsigned char, 6> bytes{{'C','T','G','1',0,0}};
    CHECK(!ctg::parse_content(nullptr, 5));
    for (size_t n = 0; n <= bytes.size(); ++n) CHECK(bool(ctg::parse_content(bytes.data(), n)) == (n == 5));
    for (unsigned id = 0; id < 256; ++id) {
        bytes[4] = static_cast<unsigned char>(id);
        auto parsed = ctg::parse_content(bytes.data(), 5);
        CHECK(bool(parsed) == (id < 15));
        if (parsed) CHECK(*parsed == id);
    }
    bytes[4] = 0;
    for (unsigned i = 0; i < 4; ++i) { bytes[i] ^= 1; CHECK(!ctg::parse_content(bytes.data(), 5)); bytes[i] ^= 1; }
    for (unsigned id = 0; id < 15; ++id) {
        char path[512]; std::snprintf(path, sizeof path, "%s/slot-%02u.ctg", CONTENT_DIR, id);
        std::ifstream file(path, std::ios::binary);
        CHECK(file.good());
        std::vector<unsigned char> content((std::istreambuf_iterator<char>(file)), {});
        auto parsed = ctg::parse_content(content.data(), content.size());
        CHECK(parsed && *parsed == id);
    }
    retro_set_environment(env); retro_set_video_refresh(video); retro_set_audio_sample_batch(audio);
    retro_set_input_poll(poll); retro_set_input_state(input); retro_init();
    CHECK(support && retro_api_version() == RETRO_API_VERSION);
    retro_system_info info{}; retro_get_system_info(&info);
    CHECK(info.library_name && std::strcmp(info.library_name, "Color TV-Game Simulator") == 0);
    CHECK(info.library_version && std::strcmp(info.library_version, "0.1.0") == 0);
    CHECK(std::strlen(info.library_name) == 23);
    CHECK(!info.need_fullpath && std::strcmp(info.valid_extensions, "ctg") == 0);
    retro_system_av_info av{}; retro_get_system_av_info(&av);
    CHECK(av.geometry.base_width == 320 && av.timing.fps == 60.0);
    retro_game_info game{}; game.data = bytes.data(); game.size = 6;
    CHECK(!retro_load_game(&game));
    game.size = 5; accept_format = false; CHECK(!retro_load_game(&game)); accept_format = true;
    CHECK(messages >= 2);

    // Compare menu ink and selection highlights, not the moving scenery behind them.
    auto same_menu = [](const std::vector<unsigned char>& a, const std::vector<unsigned char>& b) {
        for (int y = 48; y < 204; ++y) for (int x = 48; x < 272; ++x) {
            std::uint32_t p, q;
            std::memcpy(&p, a.data() + (y * 320 + x) * 4, 4);
            std::memcpy(&q, b.data() + (y * 320 + x) * 4, 4);
            auto ink = [](std::uint32_t c) { return c == 0xffffff || c == 0x70e0ff || c == 0x295166 ? c : 0u; };
            if (ink(p) != ink(q)) return false;
        }
        return true;
    };

    // The declared default and an unavailable frontend value both enable demo sound.
    CHECK(demo_sounds_definition.find("; On|Off") != std::string::npos);
    for (unsigned id : {2u, 14u}) {
        bytes[4] = static_cast<unsigned char>(id);
        buttons = buttons2 = 0; axes = {{0, 0}}; keys.fill(false);
        CHECK(retro_load_game(&game));
        unsigned before = nonzero_audio;
        for (int i = 0; i < 600; ++i) retro_run();
        CHECK(nonzero_audio > before);
        set_demo_sounds("Off"); retro_run();
        before = nonzero_audio;
        for (int i = 0; i < 600; ++i) retro_run();
        CHECK(nonzero_audio == before);
        set_demo_sounds("On"); retro_run();
        before = nonzero_audio;
        for (int i = 0; i < 600; ++i) retro_run();
        CHECK(nonzero_audio > before);
        // Starting from a sounding preview must not carry its tone into gameplay.
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run(); // wake
        buttons = 0; retro_run();
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run(); // start
        before = nonzero_audio;
        for (int i = 0; i < 5; ++i) retro_run();
        CHECK(nonzero_audio == before);
        if (id == 14) {
            // Disabling demo sounds must not mute real shots, including live changes.
            set_demo_sounds("Off"); buttons = 0; retro_run();
            before = nonzero_audio;
            buttons = 1u << RETRO_DEVICE_ID_JOYPAD_Y; retro_run();
            CHECK(nonzero_audio > before);
            buttons = 0; for (int i = 0; i < 110; ++i) retro_run();
            set_demo_sounds("On"); retro_run();
            before = nonzero_audio;
            buttons = 1u << RETRO_DEVICE_ID_JOYPAD_Y; retro_run();
            CHECK(nonzero_audio > before);
        }
        retro_unload_game();
        demo_sounds_value.clear();
    }
    buttons = 0; CHECK(retro_load_game(nullptr));
    unsigned selector_sound = nonzero_audio;
    for (int i = 0; i < 600; ++i) retro_run();
    CHECK(nonzero_audio > selector_sound);
    set_demo_sounds("Off"); retro_run();
    selector_sound = nonzero_audio;
    for (int i = 0; i < 600; ++i) retro_run();
    CHECK(nonzero_audio == selector_sound);
    retro_unload_game();
    // Attract previews run independently of the real match and are silent when disabled.
    // Menu markers use the existing layout; motion is checked outside its panel.
    auto demo_pixel = [](const std::vector<unsigned char>& image, int x, int y) {
        std::uint32_t value; std::memcpy(&value, image.data() + (y * 320 + x) * 4, sizeof value); return value;
    };
    auto outside_changed = [](const std::vector<unsigned char>& a, const std::vector<unsigned char>& b) {
        for (int y = 21; y < 214; ++y)
            for (int x = 35; x < 285; ++x)
                if ((x < 48 || x >= 272 || y < 48 || y >= 204) &&
                    std::memcmp(a.data() + (y * 320 + x) * 4, b.data() + (y * 320 + x) * 4, 4) != 0) return true;
        return false;
    };
    for (unsigned id : {2u, 14u}) {
        bytes[4] = static_cast<unsigned char>(id);
        buttons = buttons2 = 0; axes = {{0, 0}}; keys.fill(false);
        CHECK(retro_load_game(&game)); retro_run();
        const unsigned preview_audio = nonzero_audio;
        auto preview_first = frame;
        bool animated = false;
        for (int i = 0; i < 590; ++i) {
            retro_run();
            animated |= outside_changed(preview_first, frame);
        }
        CHECK(animated && nonzero_audio == preview_audio);
        CHECK(demo_pixel(frame, 75, 103) == 0x295166); // START is still selected before 10 seconds.
        for (int i = 0; i < 20; ++i) retro_run();
        CHECK(demo_pixel(frame, 75, 103) != 0x295166);
        // The centered demo prompt remains visible while the menu is hidden.
        const char prompt[] = "DEMO MODE - PRESS ANY BUTTON TO SHOW MENU";
        const int prompt_x = (320 - (sizeof(prompt) - 1) * 6) / 2;
        const char* demo_top[] = {"11110", "11111", "10001", "01110"};
        for (int c = 0; c < 4; ++c) for (int x = 0; x < 5; ++x)
            CHECK((demo_pixel(frame, prompt_x + c * 6 + x, 4) == 0xffffff) == (demo_top[c][x] == '1'));
        // Scores are absent even while the menu is hidden.
        for (int y = 42; y < 75; ++y) for (int x : {74, 80, 86, 234, 240, 246})
            CHECK(demo_pixel(frame, x, y) != (id == 14 ? 0xffc9fa : 0xcdf0ff));
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
        CHECK(demo_pixel(frame, 75, 103) == 0x295166); // Wake does not start.
        for (int i = 0; i < 30; ++i) retro_run();
        CHECK(demo_pixel(frame, 75, 103) == 0x295166); // Nor does holding the wake button.
        buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
        auto after_preview_start = frame;
        retro_reset(); buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
        CHECK(frame == after_preview_start); // A real game starts from the same state.
        retro_unload_game();
    }
    buttons = 0; CHECK(retro_load_game(nullptr)); retro_run();
    const unsigned selector_audio = nonzero_audio;
    auto selector_preview = frame;
    bool selector_animated = false;
    for (int i = 0; i < 590; ++i) { retro_run(); selector_animated |= outside_changed(selector_preview, frame); }
    CHECK(selector_animated && nonzero_audio == selector_audio);
    CHECK(demo_pixel(frame, 55, 88) == 0x295166);
    for (int i = 0; i < 20; ++i) retro_run();
    CHECK(demo_pixel(frame, 55, 88) != 0x295166);
    CHECK(demo_pixel(frame, 37, 4) == 0xffffff); // DEMO prompt also appears in selector.
    keys[RETROK_RETURN] = true; retro_run();
    CHECK(demo_pixel(frame, 55, 88) == 0x295166); // Wake does not select a game.
    retro_run(); CHECK(demo_pixel(frame, 55, 88) == 0x295166);
    keys[RETROK_RETURN] = false; retro_run();
    keys[RETROK_RETURN] = true; retro_run(); CHECK(demo_pixel(frame, 75, 103) == 0x295166);
    keys.fill(false); retro_unload_game();
    frames = polls = audio_frames = 0;
    std::vector<std::vector<unsigned char>> title_frames;
    for (unsigned id = 0; id < 15; ++id) {
        bytes[4] = static_cast<unsigned char>(id);
        CHECK(retro_load_game(&game)); retro_run();
        for (const auto& existing : title_frames) CHECK(!same_menu(existing, frame));
        title_frames.push_back(frame);
        retro_unload_game();
    }
    bytes[4] = 14; CHECK(retro_load_game(&game)); retro_run();
    CHECK(frames == 16 && polls == 16 && audio_frames == 16 * 800);
    auto initial = frame;
    auto at = [](const std::vector<unsigned char>& image, int x, int y) {
        std::uint32_t p; std::memcpy(&p, image.data() + (y * 320 + x) * 4, sizeof p); return p;
    };
    // The first glyph of Variant or Original appears at the expected centered position.
    for (unsigned id = 0; id < 12; ++id) {
        const int sport_length = id < 8 ? 6 : 10;
        const bool variant = id % 4 < 2;
        const int length = sport_length + 1 + (variant ? 7 : 8);
        const int x = (320 - length * 6) / 2 + (sport_length + 1) * 6;
        const char* top = variant ? "10001" : "01110";
        for (int col = 0; col < 5; ++col)
            CHECK((at(title_frames[id], x + col, 58) == 0xffffff) == (top[col] == '1'));
    }
    // Verify the centered title's first glyph row spells SHOOTING GAME.
        const char* shooting_top[] = {"01111", "10001", "01110", "01110", "11111", "11111", "10001", "01111", "00000", "01111", "01110", "10001", "11111"};
        for (int x = 0; x < 320; ++x) {
            const int offset = x - 121;
            const bool ink = offset >= 0 && offset < 78 && offset % 6 < 5 && shooting_top[offset / 6][offset % 6] == '1';
            CHECK((at(initial, x, 58) == 0xffffff) == ink);
            for (int y = 70; y < 77; ++y) CHECK(at(initial, x, y) != 0xffffff);
        }
        CHECK(at(initial, 60, 150) == ((0x46a79a & 0xfefefe) >> 1));
    CHECK(at(initial, 60, 200) == ((0x46a79a & 0xfefefe) >> 1));
    CHECK(at(initial, 60, 204) == 0x46a79a);
    CHECK(at(initial, 75, 103) == 0x295166);
    CHECK(at(initial, 151, 124) == 0xffffff && at(initial, 222, 124) == 0xffffff);
    CHECK(at(initial, 130, 94) == 0xffffff);
    keys[RETROK_p] = true; retro_run(); CHECK(same_menu(frame, initial));
    keys[RETROK_p] = false; retro_run();
    auto press = [&](unsigned id) { buttons = 0; retro_run(); buttons = 1u << id; retro_run(); };
    auto exit_pause = [&]() { press(RETRO_DEVICE_ID_JOYPAD_START); press(RETRO_DEVICE_ID_JOYPAD_UP); press(RETRO_DEVICE_ID_JOYPAD_A); };
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, initial));
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); auto option = frame; CHECK(!same_menu(option, initial));
    CHECK(at(option, 75, 118) == 0x295166 && at(option, 151, 124) == 0x70e0ff && at(option, 222, 124) == 0x70e0ff);
    press(RETRO_DEVICE_ID_JOYPAD_A); CHECK(same_menu(frame, option));
    keys[RETROK_p] = true; retro_run(); CHECK(same_menu(frame, option));
    keys[RETROK_p] = false; retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(!same_menu(frame, option));
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, option));
    press(RETRO_DEVICE_ID_JOYPAD_UP); CHECK(same_menu(frame, initial));
    buttons = 0; retro_run(); keys[RETROK_RETURN] = true; retro_run();
    auto detail = frame; CHECK(detail != initial);
    retro_run(); CHECK(frame == detail); // Held confirmation does not fire a shot.
    keys[RETROK_RETURN] = false; retro_run();
    keys[RETROK_p] = true; retro_run(); auto keyboard_pause = frame; CHECK(keyboard_pause != detail);
    CHECK(at(keyboard_pause, 60, 150) == ((at(detail, 60, 150) & 0xfefefe) >> 1));
    CHECK(at(keyboard_pause, 75, 99) == 0x295166);
    retro_run(); CHECK(frame == keyboard_pause);
    keys[RETROK_p] = false; retro_run();
    keys[RETROK_DOWN] = true; retro_run(); CHECK(frame != keyboard_pause);
    keys[RETROK_DOWN] = false; keys[RETROK_p] = true; retro_run(); CHECK(frame == detail);
    keys[RETROK_p] = false; retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_B); CHECK(!same_menu(frame, initial));
    press(RETRO_DEVICE_ID_JOYPAD_START); auto pause = frame;
    unsigned silent = nonzero_audio;
    for (int i = 0; i < 10; ++i) retro_run();
    CHECK(frame == pause && nonzero_audio == silent);
    keys[RETROK_DOWN] = true; retro_run(); CHECK(frame != pause);
    keys[RETROK_DOWN] = false; retro_run(); keys[RETROK_UP] = true; retro_run(); CHECK(frame == pause);
    keys[RETROK_UP] = false; retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_UP); CHECK(at(frame, 75, 131) == 0x295166);
    press(RETRO_DEVICE_ID_JOYPAD_START); CHECK(frame != pause && !same_menu(frame, initial));
    press(RETRO_DEVICE_ID_JOYPAD_START); CHECK(frame == pause);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); CHECK(at(frame, 75, 115) == 0x295166);
    press(RETRO_DEVICE_ID_JOYPAD_START); CHECK(frame != pause && !same_menu(frame, initial));
    press(RETRO_DEVICE_ID_JOYPAD_START); CHECK(frame == pause);
    press(RETRO_DEVICE_ID_JOYPAD_A); CHECK(frame == detail);
    press(RETRO_DEVICE_ID_JOYPAD_START); CHECK(frame == pause);
    press(RETRO_DEVICE_ID_JOYPAD_B); CHECK(frame == pause);
    press(RETRO_DEVICE_ID_JOYPAD_UP); press(RETRO_DEVICE_ID_JOYPAD_A); CHECK(same_menu(frame, initial));
    press(RETRO_DEVICE_ID_JOYPAD_B); CHECK(!same_menu(frame, initial) && shutdowns == 0);
    retro_reset(); buttons = 0; retro_run(); CHECK(same_menu(frame, initial));
    CHECK(retro_serialize_size() == 0 && !retro_serialize(nullptr, 0) && !retro_unserialize(nullptr, 0));
    CHECK(!retro_get_memory_data(0) && retro_get_memory_size(0) == 0);
    retro_unload_game();
    keys.fill(false); buttons = buttons2 = 0;
    bytes[4] = 2; CHECK(retro_load_game(&game)); retro_run();
    auto menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, menu));
    auto tennis_pixel = [](int x, int y) { std::uint32_t p; std::memcpy(&p, frame.data() + (y * 320 + x) * 4, sizeof p); return p; };
    CHECK(tennis_pixel(74, 42) == 0xcdf0ff);
    auto tennis_start = frame;
    press(RETRO_DEVICE_ID_JOYPAD_UP); CHECK(frame != tennis_start);
    press(RETRO_DEVICE_ID_JOYPAD_START);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); CHECK(at(frame, 75, 115) == 0x295166);
    press(RETRO_DEVICE_ID_JOYPAD_A); CHECK(frame == tennis_start);
    buttons = 0;
    for (int i = 0; i < 130; ++i) retro_run();
    CHECK(tennis_pixel(74, 42) == 0xc07645);
    retro_reset(); retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    auto paddle = [](int x, int y) { std::uint32_t pixel; std::memcpy(&pixel, frame.data() + (y * 320 + x) * 4, sizeof pixel); return pixel == 0xd0ecb2; };
    CHECK(paddle(48, 101) && paddle(266, 101));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    CHECK(paddle(48, 97) && !paddle(48, 133));
    CHECK(!paddle(266, 101) && paddle(266, 136));
    buttons = buttons2 = 0; axes = {{-20000, 20000}}; retro_run();
    CHECK(paddle(48, 94) && paddle(266, 139));
    axes = {{0, 0}}; keys[RETROK_s] = true; keys[RETROK_UP] = true; retro_run();
    CHECK(paddle(48, 94) && paddle(266, 139));
    // Simulate the frontend mapping W/S to P1 and arrows to P2 RetroPad input.
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
    buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_UP; retro_run();
    CHECK(!paddle(48, 94) && !paddle(266, 139));
    buttons = buttons2 = 0;
    keys.fill(false); exit_pause(); CHECK(same_menu(frame, menu));
    keys[RETROK_ESCAPE] = false; buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
    CHECK(!same_menu(frame, menu)); retro_reset(); retro_run(); CHECK(same_menu(frame, menu));
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run(); buttons = 0;
    buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    for (int i = 0; i < 30000; ++i) retro_run();
    buttons = 0;
    CHECK(nonzero_audio > 0);
    auto won = frame;
    for (int i = 0; i < 10; ++i) retro_run();
    CHECK(frame == won);
    buttons2 = 0; buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
    CHECK(frame != won);
    retro_unload_game();
    buttons = buttons2 = 0; keys.fill(false);
    bytes[4] = 3; CHECK(retro_load_game(&game)); retro_run();
    auto doubles_menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, doubles_menu));
    CHECK(tennis_pixel(74, 42) == 0xcdf0ff);
    buttons = 0;
    for (int i = 0; i < 130; ++i) retro_run();
    CHECK(tennis_pixel(74, 42) == 0xc07645);
    retro_reset(); retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(paddle(48, 101) && paddle(108, 101) && paddle(205, 101) && paddle(266, 101));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    CHECK(paddle(48, 97) && paddle(108, 97));
    CHECK(paddle(205, 136) && paddle(266, 136));
    buttons = 0; buttons2 = 0; retro_reset(); retro_run(); CHECK(same_menu(frame, doubles_menu));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
    CHECK(paddle(108, 101) && paddle(205, 101));
    retro_unload_game();
    buttons = 0; bytes[4] = 2; CHECK(retro_load_game(&game)); retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!paddle(108, 101) && !paddle(205, 101));
    retro_unload_game();
    buttons = 0; bytes[4] = 4; CHECK(retro_load_game(&game)); retro_run();
    auto hockey_a_menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, hockey_a_menu));
    auto a_pixel = [](int x, int y) { std::uint32_t p; std::memcpy(&p, frame.data() + (y * 320 + x) * 4, sizeof p); return p; };
    CHECK(a_pixel(160, 30) == 0x98ff8d && a_pixel(160, 62) == 0x98ff8d);
    CHECK(a_pixel(160, 94) == 0x2cc94c && a_pixel(160, 126) == 0x2cc94c);
    CHECK(a_pixel(160, 158) == 0x98ff8d && a_pixel(160, 190) == 0x98ff8d);
    CHECK(a_pixel(48, 105) == 0xff9fff && a_pixel(266, 105) == 0xd5e87c);
    buttons = 0;
    bool closed_seen = false, reopened_seen = false;
    for (int i = 0; i < 10000 && !reopened_seen; ++i) {
        retro_run();
        if (a_pixel(160, 94) == 0x98ff8d) closed_seen = true;
        else if (closed_seen && a_pixel(160, 94) == 0x2cc94c) reopened_seen = true;
    }
    CHECK(closed_seen && reopened_seen);
    retro_reset(); buttons = 0; retro_run(); CHECK(same_menu(frame, hockey_a_menu));
    retro_unload_game();
    buttons = 0; bytes[4] = 5; CHECK(retro_load_game(&game)); retro_run();
    auto hockey_a_doubles_menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, hockey_a_doubles_menu));
    CHECK(a_pixel(160, 30) == 0x98ff8d && a_pixel(160, 94) == 0x2cc94c);
    CHECK(a_pixel(48, 105) == 0xff9fff && a_pixel(194, 105) == 0xff9fff);
    CHECK(a_pixel(119, 105) == 0xd5e87c && a_pixel(266, 105) == 0xd5e87c);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
    for (int i = 0; i < 40; ++i) retro_run();
    CHECK(a_pixel(48, 21) == 0xff9fff && a_pixel(194, 21) == 0xff9fff);
    CHECK(a_pixel(119, 193) == 0xd5e87c && a_pixel(266, 193) == 0xd5e87c);
    buttons = buttons2 = 0; axes = {{20000, -20000}};
    for (int i = 0; i < 10; ++i) retro_run();
    CHECK(a_pixel(48, 36) == 0xff9fff && a_pixel(194, 36) == 0xff9fff);
    CHECK(a_pixel(119, 161) == 0xd5e87c && a_pixel(266, 161) == 0xd5e87c);
    axes = {{0, 0}}; retro_reset(); retro_run(); CHECK(same_menu(frame, hockey_a_doubles_menu));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(a_pixel(160, 94) == 0x2cc94c && a_pixel(119, 105) == 0xd5e87c);
    buttons = 0; closed_seen = reopened_seen = false;
    for (int i = 0; i < 20000 && !reopened_seen; ++i) {
        retro_run();
        if (a_pixel(160, 94) == 0x98ff8d) closed_seen = true;
        else if (closed_seen && a_pixel(160, 94) == 0x2cc94c) reopened_seen = true;
    }
    CHECK(closed_seen && reopened_seen);
    retro_unload_game();
    buttons = 0; bytes[4] = 4; CHECK(retro_load_game(&game)); retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(a_pixel(160, 30) == 0x98ff8d && a_pixel(160, 94) == 0x2cc94c);
    CHECK(a_pixel(119, 105) == 0x2cc94c && a_pixel(194, 105) == 0x2cc94c);
    retro_unload_game();
    buttons = 0; bytes[4] = 6; CHECK(retro_load_game(&game)); retro_run();
    auto hockey_menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, hockey_menu));
    auto pixel = [](int x, int y) { std::uint32_t p; std::memcpy(&p, frame.data() + (y * 320 + x) * 4, sizeof p); return p; };
    CHECK(pixel(0, 0) == 0x505068 && pixel(100, 100) == 0x2cc94c);
    CHECK(pixel(36, 40) == 0x99ff8f && pixel(280, 40) == 0xb1ffbf);
    CHECK(pixel(36, 120) == 0x3a687f && pixel(283, 120) == 0x51bc29);
    CHECK(pixel(44, 40) == 0x2cc94c && pixel(275, 40) == 0x2cc94c);
    CHECK(pixel(160, 30) == 0x2cc94c);
    CHECK(pixel(48, 105) == 0xff9fff && pixel(266, 105) == 0xd5e87c);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
    for (int i = 0; i < 40; ++i) retro_run();
    CHECK(pixel(48, 20) != 0xff9fff && pixel(48, 21) == 0xff9fff);
    CHECK(pixel(48, 37) == 0xff9fff && pixel(48, 38) != 0xff9fff);
    CHECK(pixel(266, 192) != 0xd5e87c && pixel(266, 193) == 0xd5e87c);
    CHECK(pixel(266, 213) == 0xd5e87c && pixel(266, 214) != 0xd5e87c);
    buttons = buttons2 = 0; axes = {{20000, -20000}};
    for (int i = 0; i < 10; ++i) retro_run();
    CHECK(pixel(48, 35) != 0xff9fff && pixel(48, 36) == 0xff9fff && pixel(48, 67) == 0xff9fff);
    CHECK(pixel(266, 161) == 0xd5e87c && pixel(266, 213) != 0xd5e87c);
    axes = {{0, 0}}; retro_reset(); retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    auto count_color = [](std::uint32_t color) {
        unsigned count = 0;
        for (size_t i = 0; i < frame.size(); i += 4) {
            std::uint32_t p; std::memcpy(&p, frame.data() + i, 4);
            count += p == color;
        }
        return count;
    };
    CHECK(count_color(0xaefbff) > 0 && count_color(0x98ff8d) == 0);
    buttons = 0;
    for (int i = 0; i < 129; ++i) {
        retro_run();
        CHECK(count_color(0xaefbff) > 0 && count_color(0x98ff8d) == 0);
    }
    retro_run();
    CHECK(count_color(0xaefbff) == 0 && count_color(0x98ff8d) == 35);
    int serve_min_x = 320, serve_max_x = -1;
    for (int y = 0; y < 240; ++y) for (int x = 0; x < 320; ++x) {
        if (pixel(x, y) == 0x98ff8d) {
            if (x < serve_min_x) serve_min_x = x;
            if (x > serve_max_x) serve_max_x = x;
        }
    }
    CHECK(serve_min_x == 44 && serve_max_x == 50);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    bool saw_point = false;
    for (int i = 0; i < 10000 && !saw_point; ++i) {
        retro_run();
        saw_point = count_color(0xaefbff) > 0;
    }
    CHECK(saw_point && count_color(0x98ff8d) == 0);
    for (int i = 0; i < 129; ++i) {
        retro_run();
        CHECK(count_color(0xaefbff) > 0 && count_color(0x98ff8d) == 0);
    }
    retro_run();
    CHECK(count_color(0xaefbff) == 0 && count_color(0x98ff8d) > 0);
    // Exercise both court boundaries and framebuffer edges independently of rally timing.
    for (int ball_x : {-2, 0, 31, 34, 35, 37, 38, 160, 281, 282, 284, 285, 288, 319, 321}) {
        for (int ball_y : {-1, 0, 120, 239, 240}) {
            std::array<std::uint32_t, 320 * 240> image;
            image.fill(0x123456);
            ctg::draw_hockey_ball(image, ball_x, ball_y);
            for (int y = 0; y < 240; ++y) for (int x = 0; x < 320; ++x) {
                const bool in_ball = x >= ball_x - 3 && x < ball_x + 4 &&
                                     y >= ball_y - 2 && y < ball_y + 3;
                const std::uint32_t expected = !in_ball ? 0x123456 :
                    (x >= 35 && x < 285 ? 0x98ff8d : 0xa098ac);
                CHECK(image[y * 320 + x] == expected);
            }
        }
    }
    // Move away from the visible ball, allowing either side to score.
    for (int i = 0; i < 30000 && pixel(75, 99) != 0x295166; ++i) {
        int ball_y = -1;
        for (int y = 0; y < 240 && ball_y < 0; ++y)
            for (int x = 35; x < 285; ++x)
                if (pixel(x, y) == 0x98ff8d) { ball_y = y + 2; break; }
        if (ball_y >= 0) {
            buttons = buttons2 = 1u << (ball_y < 115 ? RETRO_DEVICE_ID_JOYPAD_DOWN : RETRO_DEVICE_ID_JOYPAD_UP);
        }
        retro_run();
    }
    CHECK(pixel(75, 99) == 0x295166 && count_color(0xaefbff) > 0);
    auto hockey_won = frame;
    for (int i = 0; i < 10; ++i) retro_run();
    CHECK(frame == hockey_won);
    buttons2 = 0; buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
    CHECK(frame != hockey_won);
    retro_unload_game();
    buttons = buttons2 = 0; axes = {{0, 0}};
    bytes[4] = 7; CHECK(retro_load_game(&game)); retro_run();
    auto hockey_doubles_menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(pixel(48, 105) == 0xff9fff && pixel(119, 105) == 0xd5e87c);
    CHECK(pixel(194, 105) == 0xff9fff && pixel(266, 105) == 0xd5e87c);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
    for (int i = 0; i < 40; ++i) retro_run();
    for (int x : {48, 194}) {
        CHECK(pixel(x, 21) == 0xff9fff && pixel(x, 38) != 0xff9fff);
        CHECK(pixel(x, 20) != 0xff9fff);
    }
    for (int x : {119, 266}) {
        CHECK(pixel(x, 193) == 0xd5e87c && pixel(x, 213) == 0xd5e87c);
        CHECK(pixel(x, 214) != 0xd5e87c);
    }
    buttons = buttons2 = 0; axes = {{20000, -20000}};
    for (int i = 0; i < 10; ++i) retro_run();
    CHECK(pixel(48, 36) == 0xff9fff && pixel(194, 36) == 0xff9fff);
    CHECK(pixel(119, 161) == 0xd5e87c && pixel(266, 161) == 0xd5e87c);
    axes = {{0, 0}}; retro_reset(); retro_run(); CHECK(same_menu(frame, hockey_doubles_menu));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(pixel(119, 105) == 0xd5e87c && pixel(194, 105) == 0xff9fff);
    retro_unload_game();
    buttons = 0; bytes[4] = 6; CHECK(retro_load_game(&game)); retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(pixel(119, 105) == 0x2cc94c && pixel(194, 105) == 0x2cc94c);
    retro_unload_game();
    buttons = 0; bytes[4] = 3; CHECK(retro_load_game(&game)); retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(paddle(108, 101) && paddle(205, 101));
    retro_unload_game();
    buttons = buttons2 = 0; bytes[4] = 8; CHECK(retro_load_game(&game)); retro_run();
    auto volleyball_a_menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, volleyball_a_menu));
    CHECK(pixel(0, 0) == 0x19b537 && pixel(100, 100) == 0x6d70f9);
    CHECK(pixel(139, 40) == 0xc84458 && pixel(163, 40) == 0xc84458);
    CHECK(pixel(150, 24) == 0x53c178 && pixel(174, 24) == 0x53c178);
    CHECK(pixel(160, 30) == 0x6d70f9 && pixel(48, 105) == 0xa6e6ff && pixel(266, 105) == 0xa6e6ff);
    CHECK(pixel(108, 105) == 0x6d70f9 && count_color(0xedfd84) > 0);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    CHECK(pixel(48, 97) == 0xa6e6ff && pixel(266, 136) == 0xa6e6ff);
    buttons = buttons2 = 0; for (int i = 0; i < 129; ++i) retro_run();
    CHECK(count_color(0xedfd84) == 0);
    exit_pause(); CHECK(same_menu(frame, volleyball_a_menu));
    retro_reset(); retro_run(); CHECK(same_menu(frame, volleyball_a_menu));
    retro_unload_game();
    buttons = buttons2 = 0; bytes[4] = 9; CHECK(retro_load_game(&game)); retro_run();
    auto volleyball_a_doubles_menu = frame;
    CHECK(!same_menu(volleyball_a_doubles_menu, volleyball_a_menu));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, volleyball_a_doubles_menu));
    CHECK(pixel(0, 0) == 0x19b537 && pixel(100, 100) == 0x6d70f9);
    CHECK(pixel(139, 40) == 0xc84458 && pixel(163, 40) == 0xc84458);
    CHECK(pixel(150, 24) == 0x53c178 && pixel(174, 24) == 0x53c178);
    CHECK(pixel(160, 30) == 0x6d70f9 && count_color(0xedfd84) > 0);
    for (int x : {48, 108, 205, 266}) CHECK(pixel(x, 105) == 0xa6e6ff);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    for (int x : {48, 108}) CHECK(pixel(x, 97) == 0xa6e6ff && pixel(x, 133) != 0xa6e6ff);
    for (int x : {205, 266}) CHECK(pixel(x, 136) == 0xa6e6ff && pixel(x, 101) != 0xa6e6ff);
    buttons = buttons2 = 0; axes = {{-20000, 20000}}; retro_run();
    for (int x : {48, 108}) CHECK(pixel(x, 94) == 0xa6e6ff);
    for (int x : {205, 266}) CHECK(pixel(x, 139) == 0xa6e6ff);
    axes = {{0, 0}}; for (int i = 0; i < 128; ++i) retro_run();
    CHECK(count_color(0xedfd84) == 0);
    exit_pause(); CHECK(same_menu(frame, volleyball_a_doubles_menu));
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(count_color(0xedfd84) > 0);
    for (int x : {48, 108, 205, 266}) CHECK(pixel(x, 105) == 0xa6e6ff);
    retro_reset(); retro_run(); CHECK(same_menu(frame, volleyball_a_doubles_menu));
    retro_unload_game();
    buttons = buttons2 = 0; bytes[4] = 10; CHECK(retro_load_game(&game)); retro_run();
    auto volleyball_menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, volleyball_menu));
    CHECK(pixel(0, 0) == 0x19b537 && pixel(100, 100) == 0x6d70f9);
    CHECK(pixel(160, 30) == 0xc7dbfd && pixel(160, 45) == 0x6d70f9);
    CHECK(pixel(48, 105) == 0xa6e6ff && pixel(266, 105) == 0xa6e6ff);
    CHECK(count_color(0xedfd84) > 0);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
    retro_run();
    CHECK(pixel(48, 97) == 0xa6e6ff && pixel(266, 136) == 0xa6e6ff);
    buttons = buttons2 = 0;
    for (int i = 0; i < 129; ++i) retro_run();
    CHECK(count_color(0xedfd84) == 0);
    exit_pause(); CHECK(same_menu(frame, volleyball_menu));
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(count_color(0xedfd84) > 0);
    retro_reset(); retro_run(); CHECK(same_menu(frame, volleyball_menu));
    retro_unload_game();
    buttons = buttons2 = 0; bytes[4] = 11; CHECK(retro_load_game(&game)); retro_run();
    auto volleyball_doubles_menu = frame;
    CHECK(!same_menu(volleyball_doubles_menu, volleyball_menu));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(pixel(0, 0) == 0x19b537 && pixel(100, 100) == 0x6d70f9);
    CHECK(pixel(160, 30) == 0xc7dbfd && count_color(0xedfd84) > 0);
    for (int x : {48, 108, 205, 266}) CHECK(pixel(x, 105) == 0xa6e6ff);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
    retro_run();
    for (int x : {48, 108}) CHECK(pixel(x, 97) == 0xa6e6ff && pixel(x, 133) != 0xa6e6ff);
    for (int x : {205, 266}) CHECK(pixel(x, 136) == 0xa6e6ff && pixel(x, 101) != 0xa6e6ff);
    buttons = buttons2 = 0; axes = {{-20000, 20000}}; retro_run();
    for (int x : {48, 108}) CHECK(pixel(x, 94) == 0xa6e6ff);
    for (int x : {205, 266}) CHECK(pixel(x, 139) == 0xa6e6ff);
    axes = {{0, 0}};
    for (int i = 0; i < 128; ++i) retro_run();
    CHECK(count_color(0xedfd84) == 0);
    exit_pause(); CHECK(same_menu(frame, volleyball_doubles_menu));
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(count_color(0xedfd84) > 0 && pixel(108, 105) == 0xa6e6ff);
    retro_reset(); retro_run(); CHECK(same_menu(frame, volleyball_doubles_menu));
    retro_unload_game();
    buttons = buttons2 = 0; bytes[4] = 0; CHECK(retro_load_game(&game)); retro_run();
    auto tennis_a_menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, tennis_a_menu));
    CHECK(pixel(139, 40) == 0xc84458 && pixel(163, 40) == 0xc84458);
    CHECK(pixel(150, 24) == 0x53c178 && pixel(174, 24) == 0x53c178);
    CHECK(pixel(160, 30) == 0xc07645 && pixel(100, 100) == 0xc07645);
    CHECK(pixel(48, 105) == 0xd0ecb2 && pixel(266, 105) == 0xd0ecb2);
    exit_pause(); CHECK(same_menu(frame, tennis_a_menu));
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    retro_reset(); retro_run(); CHECK(same_menu(frame, tennis_a_menu));
    retro_unload_game();
    buttons = buttons2 = 0; bytes[4] = 1; CHECK(retro_load_game(&game)); retro_run();
    auto tennis_a_doubles_menu = frame;
    CHECK(!same_menu(tennis_a_doubles_menu, tennis_a_menu));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, tennis_a_doubles_menu));
    CHECK(pixel(139, 40) == 0xc84458 && pixel(150, 24) == 0x53c178);
    CHECK(pixel(160, 30) == 0xc07645);
    for (int x : {48, 108, 205, 266}) CHECK(paddle(x, 101));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    for (int x : {48, 108}) CHECK(paddle(x, 97) && !paddle(x, 133));
    for (int x : {205, 266}) CHECK(paddle(x, 136) && !paddle(x, 101));
    buttons = buttons2 = 0; axes = {{-20000, 20000}}; retro_run();
    for (int x : {48, 108}) CHECK(paddle(x, 94));
    for (int x : {205, 266}) CHECK(paddle(x, 139));
    axes = {{0, 0}}; exit_pause(); CHECK(same_menu(frame, tennis_a_doubles_menu));
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
    for (int x : {48, 108, 205, 266}) CHECK(paddle(x, 101));
    retro_reset(); retro_run(); CHECK(same_menu(frame, tennis_a_doubles_menu));
    retro_unload_game();
    buttons = buttons2 = 0; bytes[4] = 12; CHECK(retro_load_game(&game)); retro_run();
    auto ping_menu = frame;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, ping_menu));
    CHECK(pixel(100, 60) == 0 && pixel(160, 155) == 0xffffff);
    CHECK(pixel(160, 177) == 0 && pixel(160, 187) == 0xffffff);
    CHECK(pixel(100, 215) == 0xffffff && pixel(48, 105) == 0x9b8cff);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    CHECK(pixel(48, 98) == 0x9b8cff && pixel(266, 135) == 0x9b8cff);
    exit_pause(); CHECK(same_menu(frame, ping_menu));
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    retro_reset(); retro_run(); CHECK(same_menu(frame, ping_menu));
    retro_unload_game();
    buttons = buttons2 = 0; bytes[4] = 13; CHECK(retro_load_game(&game)); retro_run();
    auto ping_doubles_menu = frame;
    CHECK(!same_menu(ping_doubles_menu, ping_menu));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(!same_menu(frame, ping_doubles_menu));
    CHECK(pixel(100, 60) == 0 && pixel(160, 155) == 0xffffff);
    CHECK(pixel(160, 177) == 0 && pixel(160, 187) == 0xffffff && pixel(100, 215) == 0xffffff);
    CHECK(count_color(0x8affa1) > 0);
    for (int x : {48, 108, 205, 266}) CHECK(pixel(x, 105) == 0x9b8cff);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    for (int x : {48, 108}) CHECK(pixel(x, 97) == 0x9b8cff && pixel(x, 133) != 0x9b8cff);
    for (int x : {205, 266}) CHECK(pixel(x, 136) == 0x9b8cff && pixel(x, 101) != 0x9b8cff);
    buttons = buttons2 = 0; axes = {{-20000, 20000}}; retro_run();
    for (int x : {48, 108}) CHECK(pixel(x, 94) == 0x9b8cff);
    for (int x : {205, 266}) CHECK(pixel(x, 139) == 0x9b8cff);
    axes = {{0, 0}};
    for (int i = 0; i < 128; ++i) retro_run();
    CHECK(count_color(0x8affa1) == 0);
    exit_pause(); CHECK(same_menu(frame, ping_doubles_menu));
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(count_color(0x8affa1) > 0);
    for (int x : {48, 108, 205, 266}) CHECK(pixel(x, 105) == 0x9b8cff);
    retro_reset(); retro_run(); CHECK(same_menu(frame, ping_doubles_menu));
    retro_unload_game();
    buttons = buttons2 = 0; keys.fill(false); bytes[4] = 14; CHECK(retro_load_game(&game)); retro_run();
    auto shooting_menu = frame;
    bool p2_up = false, p2_down = false, p2_axis = false;
    for (const auto& d : descriptors) {
        if (d.port != 1) continue;
        p2_up |= d.device == RETRO_DEVICE_JOYPAD && d.id == RETRO_DEVICE_ID_JOYPAD_UP && std::strstr(d.description, "target");
        p2_down |= d.device == RETRO_DEVICE_JOYPAD && d.id == RETRO_DEVICE_ID_JOYPAD_DOWN && std::strstr(d.description, "target");
        p2_axis |= d.device == RETRO_DEVICE_ANALOG && d.id == RETRO_DEVICE_ID_ANALOG_Y && std::strstr(d.description, "target");
    }
    CHECK(p2_up && p2_down && p2_axis);
    buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_Y; retro_run(); CHECK(same_menu(frame, shooting_menu));
    buttons2 = 0;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(pixel(265, 101) == 0xf3d66b);
    buttons = 0; retro_run(); CHECK(pixel(265, 101) == 0xf3d66b && pixel(265, 133) == 0xf3d66b);
    keys[RETROK_DOWN] = true; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    CHECK(pixel(265, 101) == 0x46a79a && pixel(265, 136) == 0xf3d66b && pixel(48, 101) == 0xf3d66b && pixel(48, 133) == 0xf3d66b);
    keys[RETROK_DOWN] = false; keys[RETROK_UP] = true; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_UP; retro_run();
    CHECK(pixel(265, 101) == 0xf3d66b && pixel(265, 136) == 0x46a79a && pixel(48, 101) == 0xf3d66b && pixel(48, 133) == 0xf3d66b);
    keys[RETROK_UP] = false; buttons2 = 0; axes[1] = -20000; retro_run(); axes[1] = 0;
    CHECK(pixel(265, 98) == 0xf3d66b && pixel(265, 130) == 0xf3d66b);
    retro_set_controller_port_device(1, RETRO_DEVICE_NONE);
    buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    CHECK(pixel(265, 98) == 0xf3d66b && pixel(265, 130) == 0xf3d66b);
    retro_set_controller_port_device(1, RETRO_DEVICE_JOYPAD); buttons2 = 0;
    CHECK(pixel(100, 100) == 0x46a79a && pixel(48, 110) == 0xf3d66b);
    buttons = 0; keys[RETROK_w] = true; retro_run();
    CHECK(pixel(48, 99) == 0xf3d66b && pixel(48, 132) == 0x46a79a);
    keys[RETROK_w] = false; keys[RETROK_s] = true; retro_run();
    CHECK(pixel(48, 101) == 0xf3d66b && pixel(48, 133) == 0xf3d66b);
    keys[RETROK_s] = false;
    unsigned before_sound = nonzero_audio;
    keys[RETROK_x] = true; retro_run();
    CHECK(nonzero_audio > before_sound && count_color(0x8ddade) > 0);
    keys[RETROK_x] = false;
    for (int i = 0; i < 100; ++i) retro_run();
    before_sound = nonzero_audio;
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_Y; retro_run();
    CHECK(nonzero_audio > before_sound && count_color(0x8ddade) > 0);
    exit_pause(); CHECK(same_menu(frame, shooting_menu));
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    retro_reset(); retro_run(); CHECK(same_menu(frame, shooting_menu));
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); auto auto_option = frame;
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(!same_menu(frame, auto_option));
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, auto_option));
    press(RETRO_DEVICE_ID_JOYPAD_UP);
    auto auto_menu = frame; CHECK(!same_menu(auto_menu, shooting_menu));
    press(RETRO_DEVICE_ID_JOYPAD_START);
    auto auto_start = frame;
    buttons = 0; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_UP; retro_run();
    CHECK(pixel(265, 101) == 0x46a79a && pixel(265, 135) == 0xf3d66b);
    buttons2 = 0;
    keys[RETROK_p] = true; retro_run(); keys[RETROK_p] = false; retro_run();
    keys[RETROK_DOWN] = true; retro_run(); CHECK(pixel(75, 115) == 0x295166);
    keys[RETROK_DOWN] = false; retro_run(); keys[RETROK_RETURN] = true; retro_run();
    CHECK(frame == auto_start);
    keys[RETROK_RETURN] = false; retro_run();
    CHECK(pixel(265, 101) == 0x46a79a);
    for (int shot = 0; shot < 15; ++shot) {
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_Y; retro_run();
        buttons = 0; for (int i = 0; i < 100; ++i) retro_run();
    }
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
    CHECK(pixel(265, 101) == 0xf3d66b);
    buttons = 0; retro_run(); CHECK(pixel(265, 101) == 0x46a79a);
    exit_pause(); CHECK(same_menu(frame, auto_menu));
    retro_reset(); buttons = 0; retro_run(); CHECK(same_menu(frame, auto_menu));
    retro_unload_game(); CHECK(retro_load_game(&game)); retro_run(); CHECK(same_menu(frame, shooting_menu));
    // Finish with A held: the scoring input must not confirm the result.
    press(RETRO_DEVICE_ID_JOYPAD_START);
    for (int shot = 0; shot < 15; ++shot) {
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
        if (shot != 14) { buttons = 0; for (int i = 0; i < 100; ++i) retro_run(); }
    }
    for (int i = 0; i < 100; ++i) retro_run();
    CHECK(pixel(75, 99) == 0x295166);
    auto shooting_result = frame;
    retro_run(); CHECK(frame == shooting_result);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run(); CHECK(pixel(75, 115) == 0x295166);
    for (int i = 0; i < 23; ++i) retro_run();
    CHECK(pixel(75, 115) == 0x295166);
    retro_run(); CHECK(frame == shooting_result);
    buttons = 0; retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_START); CHECK(frame == shooting_result);
    keys[RETROK_p] = true; buttons = 0; retro_run(); keys[RETROK_p] = false; retro_run(); CHECK(frame == shooting_result);
    keys[RETROK_DOWN] = true; retro_run(); CHECK(pixel(75, 115) == 0x295166);
    keys[RETROK_DOWN] = false; retro_run(); keys[RETROK_UP] = true; retro_run(); CHECK(frame == shooting_result);
    keys[RETROK_UP] = false; retro_run(); keys[RETROK_RETURN] = true; retro_run();
    CHECK(pixel(75, 99) != 0x295166);
    keys[RETROK_RETURN] = false; retro_run();
    for (int shot = 0; shot < 15; ++shot) {
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_Y; retro_run();
        buttons = 0; for (int i = 0; i < 100; ++i) retro_run();
    }
    CHECK(pixel(75, 99) == 0x295166);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A); CHECK(same_menu(frame, shooting_menu));
    retro_unload_game();

    // Tennis B doubles reaches a winner with both paddles held away from play.
    buttons = buttons2 = 0; bytes[4] = 3; CHECK(retro_load_game(&game)); retro_run();
    auto result_tennis_menu = frame;
    press(RETRO_DEVICE_ID_JOYPAD_START);
    for (int round = 0; round < 2; ++round) {
        buttons = buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
        int remaining = 20000;
        do { retro_run(); } while (pixel(75, 99) != 0x295166 && --remaining);
        CHECK(remaining > 0);
        auto tennis_result = frame;
        retro_run(); CHECK(frame == tennis_result);
        buttons = buttons2 = 0; retro_run();
        press(RETRO_DEVICE_ID_JOYPAD_START); CHECK(frame == tennis_result);
        press(RETRO_DEVICE_ID_JOYPAD_DOWN); CHECK(pixel(75, 115) == 0x295166);
        if (round == 0) {
            press(RETRO_DEVICE_ID_JOYPAD_UP); CHECK(frame == tennis_result);
            press(RETRO_DEVICE_ID_JOYPAD_A); CHECK(pixel(75, 99) != 0x295166);
            CHECK(paddle(108, 101) && paddle(205, 101));
        } else {
            keys[RETROK_RETURN] = true; buttons = 0; retro_run(); keys[RETROK_RETURN] = false;
            CHECK(same_menu(frame, result_tennis_menu));
        }
    }
    retro_unload_game();

    // Display positions map to stable content IDs, independently of their numeric order.
    const std::array<unsigned, 15> display_ids{{2, 3, 6, 7, 10, 11, 0, 1, 4, 5, 8, 9, 12, 13, 14}};
    // Every selector entry opens the same game as its direct-content launch.
    buttons = buttons2 = 0; axes = {{0, 0}}; keys.fill(false);
    CHECK(retro_load_game(nullptr)); retro_run();
    auto first_selector = frame;
    CHECK(at(first_selector, 160, 185) == 0xffffff && at(first_selector, 156, 187) == 0xffffff);
    CHECK(at(first_selector, 160, 191) == 0xffffff && at(first_selector, 156, 191) != 0xffffff);
    CHECK(at(first_selector, 55, 88) == 0x295166 && at(first_selector, 160, 77) != 0xffffff);
    keys[RETROK_p] = true; retro_run(); CHECK(same_menu(frame, first_selector));
    keys[RETROK_p] = false; retro_run();
    // Escape at the selector is a no-op; no frontend shutdown is requested.
    keys[RETROK_ESCAPE] = true; retro_run(); keys[RETROK_ESCAPE] = false; retro_run();
    CHECK(same_menu(frame, first_selector) && shutdowns == 0);
    for (unsigned id = 0; id < 15; ++id) {
        auto selected = frame;
        const unsigned first = id > 2 ? (id - 2 < 9 ? id - 2 : 9) : 0;
        CHECK(at(selected, 55, 88 + (id - first) * 16) == 0x295166);
        CHECK((at(selected, 160, 77) == 0xffffff) == (first != 0));
        CHECK((at(selected, 160, 191) == 0xffffff) == (first + 6 < 15));
        for (unsigned row = 0; row < 6; ++row) {
            bool ink = false;
            for (unsigned y = 91 + row * 16; y < 98 + row * 16; ++y)
                for (unsigned x = 56; x < 264; ++x)
                    ink |= at(selected, x, y) == (first + row == id ? 0x70e0ff : 0xffffff);
            CHECK(ink);
        }
        retro_run(); CHECK(same_menu(frame, selected)); // The selector UI stays unchanged as the preview animates.
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
        auto launch = frame;
        buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
        auto started = frame;
        retro_reset(); retro_run(); CHECK(same_menu(frame, launch));
        buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_B; retro_run(); CHECK(same_menu(frame, selected));
        buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
        auto next = frame;
        retro_run(); CHECK(same_menu(frame, next)); // Held direction only moves once.
        buttons = 0; retro_run();
        retro_unload_game(); bytes[4] = static_cast<unsigned char>(display_ids[id]);
        CHECK(retro_load_game(&game)); retro_run();
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run(); CHECK(frame == started);
        retro_unload_game(); buttons = 0;
        CHECK(retro_load_game(nullptr)); retro_run();
        for (unsigned n = 0; n <= id; ++n) {
            buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
            buttons = 0; retro_run();
        }
        CHECK(same_menu(frame, next));
    }
    CHECK(same_menu(frame, first_selector));
    keys[RETROK_UP] = true; retro_run(); keys[RETROK_UP] = false; retro_run();
    auto last_selector = frame; CHECK(!same_menu(last_selector, first_selector));
    keys[RETROK_DOWN] = true; retro_run(); keys[RETROK_DOWN] = false; retro_run();
    CHECK(same_menu(frame, first_selector));
    keys[RETROK_UP] = true; retro_run(); keys[RETROK_UP] = false; retro_run();
    CHECK(same_menu(frame, last_selector));
    CHECK(at(last_selector, 160, 77) == 0xffffff && at(last_selector, 156, 77) != 0xffffff);
    CHECK(at(last_selector, 156, 81) == 0xffffff && at(last_selector, 160, 83) == 0xffffff);
    CHECK(at(last_selector, 55, 168) == 0x295166 && at(last_selector, 160, 191) != 0xffffff);

    axes[0] = 20000; retro_run(); CHECK(same_menu(frame, first_selector));
    retro_run(); CHECK(same_menu(frame, first_selector));
    axes[0] = 0; retro_run();
    keys[RETROK_DOWN] = true; retro_run(); keys[RETROK_DOWN] = false; retro_run();
    keys[RETROK_RETURN] = true; retro_run(); keys[RETROK_RETURN] = false; retro_run();
    auto selected_menu = frame;
    keys[RETROK_UP] = true; retro_run(); keys[RETROK_UP] = false; retro_run();
    auto return_selected = frame; CHECK(!same_menu(return_selected, selected_menu));
    keys[RETROK_RIGHT] = true; retro_run(); keys[RETROK_RIGHT] = false; retro_run(); CHECK(same_menu(frame, return_selected));
    keys[RETROK_DOWN] = true; retro_run(); keys[RETROK_DOWN] = false; retro_run(); CHECK(same_menu(frame, selected_menu));
    keys[RETROK_ESCAPE] = true; retro_run(); keys[RETROK_ESCAPE] = false; retro_run(); CHECK(!same_menu(frame, selected_menu));
    retro_reset(); retro_run(); CHECK(same_menu(frame, selected_menu));
    retro_unload_game();

    // Direction repeat: immediate, 24-frame delay, then every 6 frames.
    buttons = 0; keys.fill(false); axes = {{0, 0}};
    CHECK(retro_load_game(nullptr)); retro_run(); CHECK(same_menu(frame, first_selector));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; retro_run(); CHECK(same_menu(frame, last_selector));
    for (int i = 0; i < 23; ++i) retro_run();
    CHECK(same_menu(frame, last_selector));
    retro_run(); auto before_last = frame; CHECK(!same_menu(before_last, last_selector));
    for (int i = 0; i < 5; ++i) retro_run();
    CHECK(same_menu(frame, before_last));
    retro_run(); CHECK(!same_menu(frame, before_last));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run(); CHECK(same_menu(frame, before_last));
    buttons = 0; retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run(); CHECK(same_menu(frame, last_selector));
    retro_run(); CHECK(same_menu(frame, last_selector));
    buttons = 0; retro_run();
    axes[0] = 20000; retro_run(); CHECK(same_menu(frame, first_selector));
    for (int i = 0; i < 24; ++i) retro_run();
    CHECK(!same_menu(frame, first_selector));
    axes[0] = 0; retro_run();
    retro_unload_game();

    bytes[4] = 14; CHECK(retro_load_game(&game)); retro_run();
    keys[RETROK_DOWN] = true; retro_run(); CHECK(same_menu(frame, option));
    for (int i = 0; i < 24; ++i) retro_run();
    CHECK(!same_menu(frame, option));
    keys[RETROK_DOWN] = false; retro_run();
    retro_reset(); retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run(); auto moving = frame;
    retro_run(); CHECK(frame != moving); moving = frame;
    retro_run(); CHECK(frame != moving); // Gameplay continues moving every frame, without menu repeat timing.
    buttons |= 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    auto repeat_pause = frame;
    for (int i = 0; i < 30; ++i) retro_run();
    CHECK(frame == repeat_pause); // A direction held when entering pause does not navigate it.
    buttons = 0; retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run(); CHECK(at(frame, 75, 115) == 0x295166);
    for (int i = 0; i < 24; ++i) retro_run();
    CHECK(at(frame, 75, 131) == 0x295166);
    for (int i = 0; i < 6; ++i) retro_run();
    CHECK(frame == repeat_pause);
    buttons = 0; retro_run();
    retro_unload_game();

    // Each title exposes speed and both sizes; shooting also keeps its target mode.
    for (unsigned id = 0; id < 15; ++id) {
        buttons = buttons2 = 0; keys.fill(false); axes = {{0, 0}};
        bytes[4] = static_cast<unsigned char>(id);
        CHECK(retro_load_game(&game)); retro_run();
        auto defaults = frame;
        // All choices stay in place and no vertical scroll arrows appear.
        const unsigned count = 7, y = 88, spacing = 15;
        for (unsigned row = 0; row < count; ++row) {
            CHECK(at(frame, 75, y + ((row + 1) % count) * spacing) == 0x295166);
            CHECK(at(frame, 160, 87) != 0xffffff && at(frame, 156, 181) != 0xffffff);
            press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        }
        CHECK(same_menu(frame, defaults));
        press(RETRO_DEVICE_ID_JOYPAD_DOWN); // Mode or target.
        press(RETRO_DEVICE_ID_JOYPAD_DOWN); // Speed.
        auto low = frame; press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(!same_menu(frame, low));
        press(RETRO_DEVICE_ID_JOYPAD_DOWN); // Left size.
        if (id != 14) CHECK(at(frame, 117, 151) == 0x70e0ff); // Z in P1 SIZE.
        auto large_left = frame; press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(!same_menu(frame, large_left));
        press(RETRO_DEVICE_ID_JOYPAD_DOWN); // Right size.
        auto large_right = frame; press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(!same_menu(frame, large_right));

        retro_reset(); retro_run(); CHECK(!same_menu(frame, defaults)); // Switches survive reset.
        press(RETRO_DEVICE_ID_JOYPAD_START);
        const auto paddle_color = id == 14 ? 0xf3d66b : id >= 12 ? 0x9b8cff : id >= 8 ? 0xa6e6ff : id >= 4 ? 0xff9fff : 0xd0ecb2;
        CHECK(at(frame, 48, 117) == paddle_color);
        CHECK(at(frame, 48, 103) != paddle_color);
        press(RETRO_DEVICE_ID_JOYPAD_START); // Pause, then restart.
        press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
        CHECK(at(frame, 48, 103) != paddle_color);
        retro_unload_game(); buttons = 0;
        CHECK(retro_load_game(&game)); retro_run(); CHECK(same_menu(frame, defaults));
        retro_unload_game();
    }
    // Competitive score cycles through all targets and survives reset, restart and game switching.
    buttons = buttons2 = 0; bytes[4] = 2; CHECK(retro_load_game(&game)); retro_run();
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    std::vector<std::vector<unsigned char>> scores{frame};
    for (int i = 0; i < 5; ++i) {
        press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        for (const auto& score : scores) CHECK(!same_menu(frame, score));
        scores.push_back(frame);
    }
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, scores[0]));
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, scores[5])); // 10
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, scores[4])); // 5
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, scores[3])); // 30
    retro_reset(); retro_run();
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, scores[3]));
    press(RETRO_DEVICE_ID_JOYPAD_B); // selector
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_UP);
    press(RETRO_DEVICE_ID_JOYPAD_A);
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, scores[3]));
    press(RETRO_DEVICE_ID_JOYPAD_UP); // Select AI without disturbing target.
    for (int i = 0; i < 3; ++i) press(RETRO_DEVICE_ID_JOYPAD_UP);
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    press(RETRO_DEVICE_ID_JOYPAD_UP); press(RETRO_DEVICE_ID_JOYPAD_START);
    press(RETRO_DEVICE_ID_JOYPAD_START); press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    retro_reset(); retro_run();
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(!same_menu(frame, scores[0])); // AI mode remains configured too.
    for (int i = 0; i < 4; ++i) press(RETRO_DEVICE_ID_JOYPAD_UP);
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); // Restore two-player mode.
    for (int i = 0; i < 4; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, scores[3]));
    auto retained_score = frame;
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_UP);
    press(RETRO_DEVICE_ID_JOYPAD_START);
    for (int round = 0; round < 2; ++round) {
        buttons = buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
        int remaining = 30000;
        do { retro_run(); } while (pixel(75, 99) != 0x295166 && --remaining);
        CHECK(remaining > 0);
        // The winner's two digits are 30, not the former fixed 15.
        CHECK((pixel(63, 42) == 0xcdf0ff && pixel(85, 42) == 0xcdf0ff) ||
              (pixel(223, 42) == 0xcdf0ff && pixel(245, 42) == 0xcdf0ff));
        buttons = buttons2 = 0; retro_run();
        press(RETRO_DEVICE_ID_JOYPAD_A); // Result restart retains 30.
    }
    retro_unload_game(); buttons = 0; CHECK(retro_load_game(&game)); retro_run();
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, scores[0]) && !same_menu(frame, retained_score));
    retro_unload_game();

    // SHOTS cycles independently of SCORE and survives switches, reset, and both restarts.
    buttons = buttons2 = 0; bytes[4] = 14; CHECK(retro_load_game(&game)); retro_run();
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    std::vector<std::vector<unsigned char>> shots{frame};
    for (int i = 0; i < 5; ++i) {
        press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        for (const auto& value : shots) CHECK(!same_menu(frame, value));
        shots.push_back(frame);
    }
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, shots[0]));
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, shots[5]));
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, shots[4])); // 5
    retro_reset(); buttons = 0; retro_run();
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, shots[4]));
    press(RETRO_DEVICE_ID_JOYPAD_B); press(RETRO_DEVICE_ID_JOYPAD_UP); press(RETRO_DEVICE_ID_JOYPAD_A);
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    auto competitive_default = frame;
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); auto competitive_changed = frame;
    press(RETRO_DEVICE_ID_JOYPAD_B); press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, shots[4]));
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_UP);
    press(RETRO_DEVICE_ID_JOYPAD_START);
    press(RETRO_DEVICE_ID_JOYPAD_START); press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    for (int round = 0; round < 2; ++round) {
        for (int shot = 0; shot < 5; ++shot) {
            press(RETRO_DEVICE_ID_JOYPAD_Y);
            buttons = 0; for (int i = 0; i < 100; ++i) retro_run();
            CHECK((pixel(75, 99) == 0x295166) == (shot == 4));
        }
        if (round == 0) press(RETRO_DEVICE_ID_JOYPAD_A);
    }
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, shots[4]));
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, shots[3])); // 30
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_UP);
    press(RETRO_DEVICE_ID_JOYPAD_START);
    for (int shot = 0; shot < 30; ++shot) {
        press(RETRO_DEVICE_ID_JOYPAD_Y);
        buttons = 0; for (int i = 0; i < 100; ++i) retro_run();
        CHECK((pixel(75, 99) == 0x295166) == (shot == 29));
    }
    CHECK(pixel(63, 42) == 0xffc9fa && pixel(85, 42) == 0xffc9fa);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    press(RETRO_DEVICE_ID_JOYPAD_B); press(RETRO_DEVICE_ID_JOYPAD_UP); press(RETRO_DEVICE_ID_JOYPAD_A);
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, competitive_changed) && !same_menu(frame, competitive_default));
    retro_unload_game(); buttons = 0; CHECK(retro_load_game(&game)); retro_run();
    for (int i = 0; i < 5; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, shots[0]));
    retro_unload_game();

    // P2 modes are available for every tennis-family title and cycle in both directions.
    for (unsigned id = 0; id < 14; ++id) {
        buttons = buttons2 = 0; axes = {{0, 0}};
        bytes[4] = static_cast<unsigned char>(id);
        CHECK(retro_load_game(&game)); retro_run();
        press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        CHECK(at(frame, 75, 118) == 0x295166 && at(frame, 91, 124) == 0x70e0ff && at(frame, 220, 124) == 0x70e0ff);
        std::vector<std::vector<unsigned char>> modes{frame};
        for (int i = 0; i < 3; ++i) {
            press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
            for (const auto& mode : modes) CHECK(!same_menu(frame, mode));
            modes.push_back(frame);
        }
        press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, modes[0]));
        press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, modes[3]));
        press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, modes[0]));
        retro_unload_game();
    }

    // AI ignores both digital and analog P2 input, while classic responds to it.
    // Replaying after reset must reproduce the same frame sequence for every mode.
    for (unsigned mode = 0; mode < 4; ++mode) {
        buttons = buttons2 = 0; axes = {{0, 0}}; bytes[4] = 2;
        CHECK(retro_load_game(&game)); retro_run();
        press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        for (unsigned i = 0; i < mode; ++i) press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        retro_reset(); buttons = 0; retro_run();
        press(RETRO_DEVICE_ID_JOYPAD_START);
        std::vector<std::vector<unsigned char>> baseline;
        buttons = 0;
        for (int i = 0; i < 180; ++i) { retro_run(); baseline.push_back(frame); }
        retro_reset(); buttons = 0; retro_run();
        press(RETRO_DEVICE_ID_JOYPAD_START);
        buttons = 0; buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; axes[1] = -20000;
        bool differs = false;
        for (int i = 0; i < 180; ++i) {
            retro_run();
            if (frame != baseline[i]) differs = true;
        }
        CHECK(differs == (mode == 0));
        if (mode != 0) {
            buttons = buttons2 = 0; axes[1] = 0;
            retro_reset(); retro_run(); press(RETRO_DEVICE_ID_JOYPAD_START);
            buttons = 0;
            for (int i = 0; i < 60; ++i) { retro_run(); CHECK(frame == baseline[i]); }
            buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
            auto paused_frame = frame;
            buttons = 0;
            // Longer than the initial Hard phase and a full Normal/Easy loop.
            // Paused frames must not advance the clock.
            for (int i = 0; i < 3660; ++i) { retro_run(); CHECK(frame == paused_frame); }
            buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
            buttons = 0;
            for (int i = 60; i < 180; ++i) { retro_run(); CHECK(frame == baseline[i]); }
            // Pause-menu restart resets the AI sequence as well as the match.
            buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
            press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
            buttons = 0;
            for (int i = 0; i < 180; ++i) { retro_run(); CHECK(frame == baseline[i]); }
        }
        retro_unload_game();
    }

    // A selection belongs to its content ID, survives a game change and reset,
    // and is cleared by loading content again (including no-content loads).
    buttons = buttons2 = 0; axes = {{0, 0}};
    CHECK(retro_load_game(nullptr)); retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_A);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    auto classic_p2 = frame;
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); auto easy_p2 = frame;
    press(RETRO_DEVICE_ID_JOYPAD_B);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_A);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    auto other_classic = frame;
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(!same_menu(frame, other_classic));
    press(RETRO_DEVICE_ID_JOYPAD_B);
    press(RETRO_DEVICE_ID_JOYPAD_UP); press(RETRO_DEVICE_ID_JOYPAD_A);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, easy_p2));
    retro_reset(); buttons = 0; retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, easy_p2));
    retro_unload_game(); buttons = 0;
    CHECK(retro_load_game(nullptr)); retro_run(); press(RETRO_DEVICE_ID_JOYPAD_A);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, classic_p2));
    retro_unload_game();
    retro_deinit();

    // Startup selection applies only to no-content launches. Keep this fixture
    // separate from the older tests and their system-directory fixture.
    const auto save_temp = fs::temp_directory_path() / ("ctg-start-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    CHECK(fs::create_directory(save_temp));
    save_directory = save_temp.u8string(); save_available = true;
    const auto last_game = save_temp / "colortvgame-last-game";
    buttons = buttons2 = 0; axes = {{0, 0}}; keys.fill(false);
    start_game_value = "First";
    retro_set_environment(env); retro_init();
    CHECK(start_game_definition.find("; Last used|First|Random") != std::string::npos);

    // Capture each selector and direct-content menu as independent UI oracles.
    std::vector<std::vector<unsigned char>> selectors, direct_menus;
    CHECK(retro_load_game(nullptr)); retro_run();
    for (unsigned id = 0; id < 15; ++id) {
        selectors.push_back(frame);
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
        buttons = 0; retro_run();
        { std::ifstream in(last_game, std::ios::binary);
          CHECK(in.get() == static_cast<int>(display_ids[(id + 1) % display_ids.size()]));
          CHECK(in.get() == std::char_traits<char>::eof()); }
    }
    retro_unload_game();
    for (unsigned id = 0; id < 15; ++id) {
        bytes[4] = static_cast<unsigned char>(id);
        CHECK(retro_load_game(&game)); retro_run(); direct_menus.push_back(frame);
        retro_unload_game();
    }

    // A highlighted selection survives unload, deinit, and a fresh startup without confirmation.
    CHECK(retro_load_game(nullptr)); retro_run();
    for (unsigned i = 0; i < 7; ++i) {
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
        buttons = 0; retro_run();
    }
    { std::ifstream in(last_game, std::ios::binary); CHECK(in.get() == 1 && in.get() == std::char_traits<char>::eof()); }
    CHECK(same_menu(frame, selectors[7]));
    retro_unload_game();
    start_game_value.clear(); // Frontend has no value: use the declared default.
    CHECK(retro_load_game(nullptr)); retro_run(); CHECK(same_menu(frame, selectors[7]));
    retro_unload_game(); retro_deinit(); retro_init();
    CHECK(retro_load_game(nullptr)); retro_run(); CHECK(same_menu(frame, selectors[7]));
    retro_unload_game();

    // First overrides saved state; direct content overrides every startup mode.
    for (const char* mode : {"Last used", "First", "Random"}) {
        start_game_value = mode;
        for (unsigned id : {0u, 7u, 14u}) {
            bytes[4] = static_cast<unsigned char>(id);
            CHECK(retro_load_game(&game)); retro_run();
            CHECK(same_menu(frame, direct_menus[id])); retro_unload_game();
        }
    }
    start_game_value = "First";
    CHECK(retro_load_game(nullptr)); retro_run(); CHECK(same_menu(frame, selectors[0]));
    retro_unload_game();

    // Last used reads the persisted binary ID on startup; absent, out-of-range,
    // empty and oversized records all fall back safely to the first entry.
    start_game_value = "Last used";
    for (const std::vector<unsigned char>& record : std::vector<std::vector<unsigned char>>{{7}, {}, {15}, {255}, {7, 0}}) {
        { std::ofstream out(last_game, std::ios::binary | std::ios::trunc);
          CHECK(out.good());
          if (!record.empty()) out.write(reinterpret_cast<const char*>(record.data()), record.size());
          CHECK(out.good()); }
        retro_deinit(); retro_init();
        CHECK(retro_load_game(nullptr)); retro_run();
        CHECK(same_menu(frame, selectors[record == std::vector<unsigned char>{7} ? 3 : 0]));
        retro_unload_game();
    }
    for (unsigned position = 0; position < display_ids.size(); ++position) {
        { std::ofstream out(last_game, std::ios::binary | std::ios::trunc); out.put(display_ids[position]); CHECK(out.good()); }
        CHECK(retro_load_game(nullptr)); retro_run(); CHECK(same_menu(frame, selectors[position]));
        retro_unload_game();
    }
    fs::remove(last_game); retro_deinit(); retro_init();
    CHECK(retro_load_game(nullptr)); retro_run(); CHECK(same_menu(frame, selectors[0]));
    retro_unload_game();
    save_available = false; retro_deinit(); retro_init();
    CHECK(retro_load_game(nullptr)); retro_run(); CHECK(same_menu(frame, selectors[0]));
    retro_unload_game(); save_available = true;

    // Random must choose valid entries and vary across launches. The probability
    // of a uniform random implementation choosing one ID 128 times is negligible.
    start_game_value = "Random";
    std::array<bool, 15> seen{};
    for (unsigned attempt = 0; attempt < 128; ++attempt) {
        CHECK(retro_load_game(nullptr)); retro_run();
        unsigned id = 0;
        while (id < selectors.size() && !same_menu(frame, selectors[id])) ++id;
        CHECK(id < selectors.size()); seen[id] = true;
        retro_unload_game();
    }
    unsigned distinct = 0; for (bool selected : seen) if (selected) ++distinct;
    CHECK(distinct > 1);
    retro_deinit(); fs::remove_all(save_temp);

    // Rotation has its own save directory and fresh core state; the UI oracles
    // above were captured with rotation Off and startup set to First.
    const auto rotation_temp = fs::temp_directory_path() / ("ctg-rotation-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    CHECK(fs::create_directory(rotation_temp));
    save_directory = rotation_temp.u8string(); save_available = true;
    start_game_value = "First"; demo_rotation_value = "Off";
    buttons = buttons2 = 0; axes = {{0, 0}}; keys.fill(false);
    retro_set_environment(env); retro_init();
    CHECK(demo_rotation_definition.find("; Random every 15 seconds|") != std::string::npos);
    CHECK(demo_rotation_definition.find("|Off") != std::string::npos);
    CHECK(demo_rotation_definition.find("|Every 15 seconds") != std::string::npos);

    // Capture hidden titles independently from direct-content launch menus.
    auto same_bottom_ink = [&](const std::vector<unsigned char>& a, const std::vector<unsigned char>& b) {
        for (int y = 220; y < 240; ++y) for (int x = 0; x < 320; ++x)
            if ((demo_pixel(a, x, y) == 0xffffff) != (demo_pixel(b, x, y) == 0xffffff)) return false;
        return true;
    };
    std::vector<std::vector<unsigned char>> hidden_titles;
    for (unsigned id : {2u, 3u, 14u}) {
        bytes[4] = static_cast<unsigned char>(id);
        CHECK(retro_load_game(&game));
        for (int i = 0; i < 600; ++i) retro_run();
        hidden_titles.push_back(frame);
        retro_unload_game();
    }
    // The full first glyph row of SHOOTING GAME must appear at the bottom.
    const char* hidden_shooting_top[] = {"01111", "10001", "01110", "01110", "11111", "11111", "10001", "01111", "00000", "01111", "01110", "10001", "11111"};
    bool shooting_title_found = false;
    for (int y = 220; y < 240; ++y) {
        bool matches = true;
        for (int x = 0; x < 320; ++x) {
            const int offset = x - 121;
            const bool ink = offset >= 0 && offset < 78 && offset % 6 < 5 && hidden_shooting_top[offset / 6][offset % 6] == '1';
            if ((demo_pixel(hidden_titles[2], x, y) == 0xffffff) != ink) matches = false;
        }
        shooting_title_found |= matches;
    }
    CHECK(shooting_title_found);
    CHECK(!same_bottom_ink(hidden_titles[0], hidden_titles[1]));

    // Explicit Off leaves the first entry alone.
    CHECK(retro_load_game(nullptr));
    for (int i = 0; i < 1801; ++i) retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
    CHECK(same_menu(frame, selectors[0])); // Wake must not confirm.
    buttons = 0; retro_unload_game();

    // Both the explicit random mode and a missing frontend value rotate at
    // 600 frames to hide the menu, then 900 hidden frames per rotation.
    const auto rotation_last_game = rotation_temp / "colortvgame-last-game";
    for (const char* mode : {"Random every 15 seconds", ""}) {
        demo_rotation_value = mode; variable_updated = true;
        { std::ofstream out(rotation_last_game, std::ios::binary | std::ios::trunc);
          out.put(7); CHECK(out.good()); }
        CHECK(retro_load_game(nullptr));
        for (int i = 0; i < 1499; ++i) retro_run();
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
        CHECK(same_menu(frame, selectors[0]));
        buttons = 0; retro_unload_game();

        std::array<bool, 15> rotated_seen{};
        for (unsigned attempt = 0; attempt < 16; ++attempt) {
            CHECK(retro_load_game(nullptr));
            for (int i = 0; i < 1500; ++i) retro_run();
            buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
            unsigned id = 0;
            while (id < selectors.size() && !same_menu(frame, selectors[id])) ++id;
            CHECK(id < selectors.size()); // Wake shows a valid selector, not a launch menu.
            CHECK(id != 0);
            rotated_seen[id] = true;
            { std::ifstream in(rotation_last_game, std::ios::binary);
              CHECK(in.get() == 7 && in.get() == std::char_traits<char>::eof()); }
            buttons = 0; retro_unload_game();
        }
        unsigned rotated_distinct = 0;
        for (bool selected : rotated_seen) if (selected) ++rotated_distinct;
        CHECK(rotated_distinct > 1);
    }
    fs::remove(rotation_last_game);

    demo_rotation_value = "Every 15 seconds"; variable_updated = true;
    CHECK(retro_load_game(nullptr));
    for (int i = 0; i < 1499; ++i) retro_run();
    CHECK(same_bottom_ink(frame, hidden_titles[0]));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
    CHECK(same_menu(frame, selectors[0])); // No rotation before 10 + 15 seconds.
    buttons = 0; retro_unload_game();

    CHECK(retro_load_game(nullptr));
    for (int i = 0; i < 1500; ++i) retro_run();
    CHECK(same_bottom_ink(frame, hidden_titles[1])); // The hidden title follows rotation.
    CHECK(!fs::exists(rotation_temp / "colortvgame-last-game"));
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
    CHECK(same_menu(frame, selectors[1])); // Wake on the rotated entry, without confirming.
    CHECK(!fs::exists(rotation_temp / "colortvgame-last-game"));
    buttons = 0; retro_unload_game();

    // Sequential rotation visits every display position and wraps without saving.
    { std::ofstream out(rotation_last_game, std::ios::binary | std::ios::trunc); out.put(0); CHECK(out.good()); }
    CHECK(retro_load_game(nullptr));
    for (unsigned position = 1; position <= display_ids.size(); ++position) {
        for (int i = 0; i < (position == 1 ? 1500 : 900); ++i) retro_run();
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
        CHECK(same_menu(frame, selectors[position % display_ids.size()]));
        buttons = 0; retro_run();
        if (position < display_ids.size()) {
            for (int i = 0; i < 599; ++i) retro_run();
        }
    }
    { std::ifstream in(rotation_last_game, std::ios::binary); CHECK(in.get() == 0 && in.get() == std::char_traits<char>::eof()); }
    retro_unload_game(); fs::remove(rotation_last_game);

    // Input resets the full idle interval, including input from the second port.
    CHECK(retro_load_game(nullptr));
    for (int i = 0; i < 700; ++i) retro_run();
    buttons2 = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run(); buttons2 = 0;
    for (int i = 0; i < 1499; ++i) retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
    CHECK(same_menu(frame, selectors[0]));
    buttons = 0; retro_unload_game();

    // Repeated input keeps the menu visible and prevents rotation indefinitely.
    CHECK(retro_load_game(nullptr));
    for (int i = 0; i < 1800; ++i) {
        buttons2 = (i % 300 == 299) ? 1u << RETRO_DEVICE_ID_JOYPAD_A : 0;
        retro_run();
    }
    buttons2 = 0;
    CHECK(same_menu(frame, selectors[0]));
    retro_unload_game();

    // Neither a direct-content launch menu nor gameplay rotates to another title.
    bytes[4] = 0; CHECK(retro_load_game(&game));
    for (int i = 0; i < 1800; ++i) retro_run();
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(same_menu(frame, direct_menus[0]));
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    buttons = 0; for (int i = 0; i < 1800; ++i) retro_run();
    retro_reset(); retro_run(); CHECK(same_menu(frame, direct_menus[0]));
    retro_unload_game(); retro_deinit();
    demo_rotation_value.clear(); save_available = false; fs::remove_all(rotation_temp);

    // Compare deterministic gameplay frame hashes after selecting a game. Changing
    // the option before launch also exercises construction of the selected engine.
    start_game_value = "First"; demo_rotation_value = "Off"; demo_sounds_value = "Off";
    save_available = false; set_color("On");
    auto rebound_replay = [&](unsigned id, const char* initial, bool selector, bool switch_live) {
        set_rebound(initial); retro_init(); buttons = buttons2 = 0; axes = {{0, 0}}; keys.fill(false);
        bytes[4] = static_cast<unsigned char>(id);
        if (!retro_load_game(selector ? nullptr : &game)) return std::vector<std::uint64_t>{};
        retro_run();
        if (selector) {
            for (unsigned i = 0; display_ids[i] != id; ++i) {
                buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run(); buttons = 0; retro_run();
            }
            buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run(); buttons = 0; retro_run();
        }
        if (id != 14) {
            buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
            buttons = 1u << RETRO_DEVICE_ID_JOYPAD_RIGHT; retro_run();
            buttons = 1u << RETRO_DEVICE_ID_JOYPAD_UP; retro_run();
        }
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
        buttons = 0;
        std::vector<std::uint64_t> result;
        for (int i = 0; i < 700; ++i) {
            if (switch_live && i == 10) set_rebound("Paddle movement");
            buttons = 1u << ((i % 2) ? RETRO_DEVICE_ID_JOYPAD_UP : RETRO_DEVICE_ID_JOYPAD_DOWN);
            buttons2 = 1u << ((i % 2) ? RETRO_DEVICE_ID_JOYPAD_DOWN : RETRO_DEVICE_ID_JOYPAD_UP);
            retro_run();
            std::uint64_t hash = 14695981039346656037ull;
            for (auto byte : frame) { hash ^= byte; hash *= 1099511628211ull; }
            result.push_back(hash);
        }
        retro_unload_game(); retro_deinit(); return result;
    };
    const auto hit = rebound_replay(2, "Hit position", false, false);
    CHECK(hit.size() == 700);
    CHECK(rebound_replay(2, "", false, false) == hit);
    const auto movement = rebound_replay(2, "Paddle movement", false, false);
    CHECK(movement.size() == hit.size() && movement != hit);
    CHECK(rebound_replay(2, "Hit position", false, true) == movement);
    CHECK(rebound_replay(2, "Paddle movement", true, false) == movement);
    const auto shooting_hit = rebound_replay(14, "Hit position", false, false);
    CHECK(shooting_hit.size() == 700);
    CHECK(rebound_replay(14, "Paddle movement", false, false) == shooting_hit);
    CHECK(rebound_replay(14, "Hit position", false, true) == shooting_hit);
    set_rebound("Hit position"); buttons = buttons2 = 0;

    // Replay identical inputs with live color changes: output conversion must not
    // change the selected game or the progression of the underlying match.
    CHECK(color_definition.find("; On|Off") != std::string::npos);
    start_game_value = "First"; demo_rotation_value = "Off"; demo_sounds_value = "Off";
    auto grayscale = [](const std::vector<unsigned char>& image) {
        if (image.size() != 320u * 240u * 4u) return false;
        for (size_t i = 0; i < image.size(); i += 4) {
            std::uint32_t pixel; std::memcpy(&pixel, image.data() + i, sizeof pixel);
            if (((pixel >> 16) & 255) != ((pixel >> 8) & 255) ||
                ((pixel >> 8) & 255) != (pixel & 255)) return false;
        }
        return true;
    };
    const std::array<unsigned, 9> sequence = {{
        0, 0, 0, 1u << RETRO_DEVICE_ID_JOYPAD_START, 0,
        1u << RETRO_DEVICE_ID_JOYPAD_START, 0, 0, 0
    }};
    std::vector<std::vector<unsigned char>> color_frames;
    set_color("On"); retro_init(); buttons = buttons2 = 0; axes = {{0, 0}}; keys.fill(false);
    CHECK(retro_load_game(nullptr));
    for (auto input_buttons : sequence) { buttons = input_buttons; retro_run(); color_frames.push_back(frame); }
    retro_unload_game(); retro_deinit();
    CHECK(!grayscale(color_frames[0]) && !grayscale(color_frames[3]) && !grayscale(color_frames[6]));

    retro_init(); buttons = 0; CHECK(retro_load_game(nullptr));
    for (size_t i = 0; i < sequence.size(); ++i) {
        set_color((i == 1 || i == 3 || i == 4 || i == 6 || i == 7) ? "Off" : "On");
        buttons = sequence[i]; retro_run();
        if (color_value == "Off") CHECK(grayscale(frame));
        else CHECK(frame == color_frames[i]);
    }
    retro_unload_game(); retro_deinit();

    // A frontend that cannot supply the option must behave like the On default.
    color_value.clear(); retro_init(); buttons = 0; CHECK(retro_load_game(nullptr));
    for (size_t i = 0; i < sequence.size(); ++i) {
        buttons = sequence[i]; retro_run(); CHECK(frame == color_frames[i]);
    }
    retro_unload_game(); retro_deinit();

    // Starting with Off must also convert a direct-content launch and gameplay.
    bytes[4] = 14; set_color("On"); retro_init(); buttons = 0;
    CHECK(retro_load_game(&game)); color_frames.clear();
    for (auto input_buttons : sequence) { buttons = input_buttons; retro_run(); color_frames.push_back(frame); }
    retro_unload_game(); retro_deinit();
    CHECK(!grayscale(color_frames[0]) && !grayscale(color_frames[4]));
    set_color("Off"); retro_init(); buttons = 0; CHECK(retro_load_game(&game));
    for (size_t i = 0; i < sequence.size(); ++i) {
        if (i == sequence.size() - 1) set_color("On");
        buttons = sequence[i]; retro_run();
        if (color_value == "Off") CHECK(grayscale(frame));
        else CHECK(frame == color_frames[i]);
    }
    retro_unload_game(); retro_deinit();

    // Monochrome menus outline the selected row without changing color-mode rendering.
    set_color("Off"); buttons = 0; retro_init(); CHECK(retro_load_game(nullptr)); retro_run();
    CHECK(demo_pixel(frame, 55, 88) == 0xffffff && demo_pixel(frame, 264, 100) == 0xffffff);
    for (unsigned id = 1; id < 15; ++id) {
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
        buttons = 0; retro_run();
        const unsigned first = id > 2 ? (id - 2 < 9 ? id - 2 : 9) : 0;
        const unsigned y = 88 + (id - first) * 16;
        CHECK(demo_pixel(frame, 55, y) == 0xffffff && demo_pixel(frame, 264, y + 12) == 0xffffff);
    }
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
    buttons = 0; retro_run();
    CHECK(demo_pixel(frame, 55, 88) == 0xffffff && demo_pixel(frame, 264, 100) == 0xffffff);
    buttons = 1u << RETRO_DEVICE_ID_JOYPAD_A; retro_run();
    CHECK(demo_pixel(frame, 53, 103) == 0xffffff && demo_pixel(frame, 266, 116) == 0xffffff);
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    buttons = 0; retro_run(); buttons = 1u << RETRO_DEVICE_ID_JOYPAD_START; retro_run();
    CHECK(demo_pixel(frame, 75, 99) == 0xffffff && demo_pixel(frame, 244, 112) == 0xffffff);
    retro_unload_game(); retro_deinit();

    // Replay the same selector and direct-content inputs with live RF changes.
    // Comparing the unfiltered frames after switching back also checks that the
    // filter never changes game selection or simulation state.
    set_color("On"); start_game_value = "First"; demo_rotation_value = "Off";
    demo_sounds_value = "Off"; save_available = false;
    bytes[4] = 14;
    for (bool direct : {false, true}) {
        std::vector<std::vector<unsigned char>> baseline;
        set_rf_filter("Off"); retro_init(); buttons = buttons2 = 0;
        axes = {{0, 0}}; keys.fill(false);
        CHECK(retro_load_game(direct ? &game : nullptr));
        for (auto input_buttons : sequence) {
            buttons = input_buttons; const unsigned before = frames; retro_run();
            CHECK(frames == before + 1 && frame.size() == 320u * 240u * 4u);
            baseline.push_back(frame);
        }
        retro_unload_game(); retro_deinit();

        // An unavailable value must be identical to explicit Off, frame for frame.
        rf_filter_value.clear(); retro_init(); buttons = 0;
        CHECK(retro_load_game(direct ? &game : nullptr));
        for (size_t i = 0; i < sequence.size(); ++i) {
            buttons = sequence[i]; const unsigned before = frames; retro_run();
            CHECK(frames == before + 1 && frame == baseline[i]);
        }
        retro_unload_game(); retro_deinit();

        retro_init(); buttons = 0; CHECK(retro_load_game(direct ? &game : nullptr));
        for (size_t i = 0; i < sequence.size(); ++i) {
            const bool filtered = i == 0 || i == 1 || i == 3 || i == 4 || i == 6 || i == 7;
            set_rf_filter(filtered ? "On" : "Off");
            buttons = sequence[i]; const unsigned before = frames; retro_run();
            CHECK(frames == before + 1 && frame.size() == baseline[i].size());
            if (filtered) CHECK(frame != baseline[i]);
            else CHECK(frame == baseline[i]);
        }
        retro_unload_game(); retro_deinit();

        // RF must not reintroduce color when monochrome output is requested.
        set_color("Off"); set_rf_filter("On"); retro_init(); buttons = 0;
        CHECK(retro_load_game(direct ? &game : nullptr));
        for (auto input_buttons : sequence) {
            buttons = input_buttons; const unsigned before = frames; retro_run();
            CHECK(frames == before + 1 && grayscale(frame));
        }
        retro_unload_game(); retro_deinit(); set_color("On");
    }
    CHECK(rf_filter_definition.find("; Off|On") != std::string::npos);

    // Capture the existing clean RF output with the quality option unavailable.
    const std::array<const char*, 11> qualities = {{
        "100%", "90%", "80%", "70%", "60%", "50%",
        "40%", "30%", "20%", "10%", "0%"
    }};
    set_rf_filter("On"); rf_quality_value.clear(); retro_init(); buttons = 0;
    CHECK(retro_load_game(&game));
    std::vector<std::vector<unsigned char>> clean_rf;
    for (auto input_buttons : sequence) {
        buttons = input_buttons; retro_run(); clean_rf.push_back(frame);
    }
    retro_unload_game(); retro_deinit();

    // Identical inputs isolate the video effect; aggregate RGB error over several
    // frames rather than depending on a particular random pixel or noise pattern.
    std::array<unsigned long long, 11> errors{};
    for (size_t q = 0; q < qualities.size(); ++q) {
        set_rf_quality(qualities[q]); retro_init(); buttons = 0;
        CHECK(retro_load_game(&game));
        bool noise_changes = false;
        std::vector<unsigned char> previous_noise;
        for (size_t i = 0; i < sequence.size(); ++i) {
            buttons = sequence[i]; retro_run();
            CHECK(frame.size() == clean_rf[i].size());
            std::vector<unsigned char> noise;
            for (size_t p = 0; p < frame.size(); p += 4) {
                std::uint32_t actual, clean;
                std::memcpy(&actual, frame.data() + p, 4);
                std::memcpy(&clean, clean_rf[i].data() + p, 4);
                for (unsigned shift : {0u, 8u, 16u}) {
                    const int delta = int((actual >> shift) & 255) - int((clean >> shift) & 255);
                    errors[q] += delta < 0 ? -delta : delta;
                    noise.push_back(static_cast<unsigned char>(delta));
                }
            }
            if (!previous_noise.empty() && noise != previous_noise) noise_changes = true;
            previous_noise = std::move(noise);
        }
        retro_unload_game(); retro_deinit();
        if (q == 0) CHECK(errors[q] == 0);
        else CHECK(errors[q] > 0 && noise_changes);
    }
    CHECK(errors.back() > errors[1]);

    // Quality changes cannot affect unfiltered output or gameplay progression.
    set_rf_filter("Off"); set_rf_quality("100%"); retro_init(); buttons = 0;
    CHECK(retro_load_game(&game));
    std::vector<std::vector<unsigned char>> unfiltered;
    for (auto input_buttons : sequence) { buttons = input_buttons; retro_run(); unfiltered.push_back(frame); }
    retro_unload_game(); retro_deinit();
    set_rf_quality("0%"); retro_init(); buttons = 0; CHECK(retro_load_game(&game));
    for (size_t i = 0; i < sequence.size(); ++i) {
        buttons = sequence[i]; retro_run(); CHECK(frame == unfiltered[i]);
    }
    retro_unload_game(); retro_deinit();

    set_rf_filter("On"); set_rf_quality("0%"); retro_init(); buttons = 0;
    CHECK(retro_load_game(&game));
    for (size_t i = 0; i < sequence.size(); ++i) {
        if (i == sequence.size() - 1) set_rf_filter("Off");
        buttons = sequence[i]; retro_run();
        if (i == sequence.size() - 1) CHECK(frame == unfiltered[i]);
    }
    retro_unload_game(); retro_deinit();
    set_color("Off"); set_rf_filter("On"); retro_init(); buttons = 0;
    CHECK(retro_load_game(&game));
    for (auto input_buttons : sequence) { buttons = input_buttons; retro_run(); CHECK(grayscale(frame)); }
    retro_unload_game(); retro_deinit();

    CHECK(rf_quality_definition.find("; 100%|90%|80%|70%|60%|50%|40%|30%|20%|10%|0%") != std::string::npos);
    return 0;
}
