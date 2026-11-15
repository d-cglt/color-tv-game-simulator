#pragma once

#include "libretro.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#define CHECK(x) do { if (core_test::check_failed(!(x), __FILE__, __LINE__, #x)) return 1; } while (0)

namespace core_test {
inline bool check_failed(bool failed, const char* file, int line, const char* expression) {
    if (failed) std::fprintf(stderr, "%s:%d: %s\n", file, line, expression);
    return failed;
}
struct RumbleCall { unsigned port; retro_rumble_effect effect; uint16_t strength; };
extern bool support, accept_format;
extern bool system_available;
extern std::string system_directory, last_message;
extern unsigned system_queries, set_variable_calls;
extern bool set_variable_supported, variable_updated, variable_update_supported;
extern std::string generate_value, last_set_key, last_set_value, generate_definition;
extern std::string demo_sounds_value, demo_sounds_definition;
extern std::string rebound_value, rebound_definition;
extern std::string anti_stalemate_value, anti_stalemate_definition;
extern unsigned core_options_version;
extern bool anti_stalemate_modern_definition, vibration_modern_definition, rf_quality_modern_definition;
extern std::string vibration_value, vibration_definition;
extern std::vector<RumbleCall> rumble_calls;
extern bool rumble_available, rumble_accept;
extern unsigned rumble_queries;
extern std::string color_value, color_definition;
extern std::string rf_filter_value, rf_filter_definition;
extern std::string rf_quality_value, rf_quality_definition;
extern std::string demo_rotation_value, demo_rotation_definition;
extern std::string start_game_value, start_game_definition, save_directory;
extern bool save_available;
extern unsigned buttons, buttons2, frames, polls, audio_frames, messages, shutdowns, nonzero_audio;
extern std::array<int16_t, 2> axes;
extern std::array<bool, RETROK_LAST> keys;
extern std::vector<unsigned char> frame;
extern std::vector<retro_input_descriptor> descriptors;

// Callback state is shared by the single frontend; scenario-only state stays local.
// The content buffer must outlive game.data across the ordered scenarios.
struct ContentContext {
    std::array<unsigned char, 6> bytes{};
    retro_game_info game{};
};
void set_vibration(const char* value);
bool rumble(unsigned port, retro_rumble_effect effect, uint16_t strength);
void set_rebound(const char* value);
void set_rf_quality(const char* value);
void set_rf_filter(const char* value);
void set_color(const char* value);
void set_demo_sounds(const char* value);
void set_generate_value(const char* value);
bool env(unsigned cmd, void* data);
void video(const void* data, unsigned w, unsigned h, size_t pitch);
void poll();
int16_t input(unsigned port, unsigned device, unsigned index, unsigned id);
size_t audio(const int16_t* data, size_t n);
bool same_menu(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b);
std::uint32_t demo_pixel(const std::vector<unsigned char>& image, int x, int y);
std::uint32_t at(const std::vector<unsigned char>& image, int x, int y);
void press(unsigned id);
void exit_pause();
std::uint32_t pixel(int x, int y);
// Display positions map to stable content IDs, independently of numeric order.
inline constexpr std::array<unsigned, 16> display_ids{{2, 3, 6, 7, 10, 11, 0, 1, 4, 5, 8, 9, 12, 13, 14, 15}};
int boot_generator_scenarios(ContentContext& context);
int demo_preview_scenarios(ContentContext& context);
struct MenuSnapshot { std::vector<unsigned char> option; };
int gameplay_rendering_scenarios(ContentContext& context, MenuSnapshot& snapshot);
int menus_options_scenarios(ContentContext& context, const MenuSnapshot& snapshot);
int persistence_selector_demo_scenarios(ContentContext& context);
int video_rumble_scenarios(ContentContext& context);
} // namespace core_test
