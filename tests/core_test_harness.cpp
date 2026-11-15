#include "core_test_harness.h"

#include <cstring>

namespace core_test {
bool support = false, accept_format = true;
bool system_available = true;
std::string system_directory, last_message;
unsigned system_queries = 0, set_variable_calls = 0;
bool set_variable_supported = true, variable_updated = false, variable_update_supported = true;
std::string generate_value = "Ready", last_set_key, last_set_value, generate_definition;
std::string demo_sounds_value, demo_sounds_definition;
std::string rebound_value, rebound_definition;
std::string anti_stalemate_value, anti_stalemate_definition;
unsigned core_options_version = 0;
bool anti_stalemate_modern_definition = false, vibration_modern_definition = false, rf_quality_modern_definition = false;
std::string vibration_value, vibration_definition;
std::vector<RumbleCall> rumble_calls;
bool rumble_available = false, rumble_accept = true;
unsigned rumble_queries = 0;
std::string color_value = "On", color_definition;
std::string rf_filter_value, rf_filter_definition;
std::string rf_quality_value = "100%", rf_quality_definition;
std::string demo_rotation_value = "Off", demo_rotation_definition;
std::string start_game_value = "First", start_game_definition, save_directory;
bool save_available = false;
unsigned buttons = 0, buttons2 = 0, frames = 0, polls = 0, audio_frames = 0, messages = 0, shutdowns = 0, nonzero_audio = 0;
std::array<int16_t, 2> axes{};
std::array<bool, RETROK_LAST> keys{};
std::vector<unsigned char> frame;
std::vector<retro_input_descriptor> descriptors;

void set_vibration(const char* value) { vibration_value = value; variable_updated = true; }
bool rumble(unsigned port, retro_rumble_effect effect, uint16_t strength) {
    rumble_calls.push_back({port, effect, strength}); return rumble_accept;
}
void set_rebound(const char* value) { rebound_value = value; variable_updated = true; }
void set_rf_quality(const char* value) { rf_quality_value = value; variable_updated = true; }
void set_rf_filter(const char* value) { rf_filter_value = value; variable_updated = true; }
void set_color(const char* value) { color_value = value; variable_updated = true; }
void set_demo_sounds(const char* value) { demo_sounds_value = value; variable_updated = true; }
void set_generate_value(const char* value) { generate_value = value; variable_updated = true; }
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
        auto check_vibration = [](const auto* definitions) {
            for (auto* d = definitions; d->key; ++d) {
                if (std::strcmp(d->key, "colortvgame_vibration") == 0)
                    return std::strcmp(d->default_value, "Off") == 0 &&
                           std::strcmp(d->values[0].value, "Off") == 0 &&
                           std::strcmp(d->values[1].value, "On") == 0 && !d->values[2].value;
            }
            return false;
        };
        auto check_quality = [](const auto* definitions) {
            const char* values[] = {"0%", "10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%"};
            for (auto* d = definitions; d->key; ++d) {
                if (std::strcmp(d->key, "colortvgame_rf_quality") != 0) continue;
                if (std::strcmp(d->default_value, "100%") != 0) return false;
                for (unsigned i = 0; i < 11; ++i)
                    if (!d->values[i].value || std::strcmp(d->values[i].value, values[i]) != 0) return false;
                return !d->values[11].value;
            }
            return false;
        };
        rf_quality_modern_definition = cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS
            ? check_quality(static_cast<retro_core_option_definition*>(data))
            : check_quality(static_cast<retro_core_options_v2*>(data)->definitions);
        vibration_modern_definition = cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS
            ? check_vibration(static_cast<retro_core_option_definition*>(data))
            : check_vibration(static_cast<retro_core_options_v2*>(data)->definitions);
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
            if (std::strcmp(variable->key, "colortvgame_vibration") == 0) vibration_definition = variable->value;
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
        if (!variable_update_supported) return false;
        *static_cast<bool*>(data) = variable_updated; variable_updated = false; return true;
    }
    if (cmd == RETRO_ENVIRONMENT_GET_VARIABLE) {
        auto* variable = static_cast<retro_variable*>(data);
        if (std::strcmp(variable->key, "colortvgame_generate_files") == 0) variable->value = generate_value.c_str();
        else if (std::strcmp(variable->key, "colortvgame_demo_sounds") == 0 && !demo_sounds_value.empty()) variable->value = demo_sounds_value.c_str();
        else if (std::strcmp(variable->key, "colortvgame_rebound") == 0 && !rebound_value.empty()) variable->value = rebound_value.c_str();
        else if (std::strcmp(variable->key, "colortvgame_vibration") == 0 && !vibration_value.empty()) variable->value = vibration_value.c_str();
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
    if (cmd == RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE) {
        ++rumble_queries;
        if (!rumble_available) return false;
        static_cast<retro_rumble_interface*>(data)->set_rumble_state = rumble;
        return true;
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

bool same_menu(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b) {
        for (int y = 48; y < 204; ++y) for (int x = 48; x < 272; ++x) {
            std::uint32_t p, q;
            std::memcpy(&p, a.data() + (y * 320 + x) * 4, 4);
            std::memcpy(&q, b.data() + (y * 320 + x) * 4, 4);
            auto ink = [](std::uint32_t c) { return c == 0xffffff || c == 0x70e0ff || c == 0x295166 ? c : 0u; };
            if (ink(p) != ink(q)) return false;
        }
        return true;
    }
std::uint32_t demo_pixel(const std::vector<unsigned char>& image, int x, int y) {
        std::uint32_t value; std::memcpy(&value, image.data() + (y * 320 + x) * 4, sizeof value); return value;
    }
std::uint32_t at(const std::vector<unsigned char>& image, int x, int y) {
        std::uint32_t p; std::memcpy(&p, image.data() + (y * 320 + x) * 4, sizeof p); return p;
    }
void press(unsigned id) { buttons = 0; retro_run(); buttons = 1u << id; retro_run(); }
void exit_pause() { press(RETRO_DEVICE_ID_JOYPAD_START); press(RETRO_DEVICE_ID_JOYPAD_UP); press(RETRO_DEVICE_ID_JOYPAD_A); }
std::uint32_t pixel(int x, int y) { std::uint32_t p; std::memcpy(&p, frame.data() + (y * 320 + x) * 4, sizeof p); return p; }
} // namespace core_test
