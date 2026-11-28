#pragma once

#include "libretro.h"
#include <array>
#include <cstddef>
#include <cstring>
#include <string>

namespace ctg {
namespace core_options {
struct Option {
    const char* key;
    const char* description;
    const char* information;
    retro_core_option_value values[RETRO_NUM_CORE_OPTION_VALUES_MAX];
    const char* default_value;
    const char* legacy_description = nullptr;
    const char* category = nullptr;
};

inline constexpr Option definitions[] = {
    {"colortvgame_color", "Color", "Display the rendered image in color or grayscale.",
     {{"On", nullptr}, {"Off", nullptr}}, "On", nullptr, "video"},
    {"colortvgame_rf_filter", "RF filter", "Approximate composite signal color and bandwidth artifacts.",
     {{"Off", nullptr}, {"On", nullptr}}, "Off", nullptr, "video"},
    {"colortvgame_rf_quality", "RF signal quality", "Lower quality adds color bleed and noise. No effect when the RF filter is off.",
     {{"0%", nullptr}, {"10%", nullptr}, {"20%", nullptr}, {"30%", nullptr}, {"40%", nullptr}, {"50%", nullptr},
      {"60%", nullptr}, {"70%", nullptr}, {"80%", nullptr}, {"90%", nullptr}, {"100%", nullptr}}, "100%",
     "RF signal quality (no effect when RF filter is off)", "video"},
    {"colortvgame_shooting_fire_sound", "Shooting fire sound", "Play a firing sound in Shooting gameplay and previews. Off matches the original console's silent firing.",
     {{"On", nullptr}, {"Off", nullptr}}, "On", nullptr, "audio"},
    {"colortvgame_demo_sounds", "Demo sounds", "Play sounds during menu previews.",
     {{"On", nullptr}, {"Off", nullptr}}, "On", nullptr, "audio"},
    {"colortvgame_stereo", "Stereo", "Pan positional game sounds across stereo channels.",
     {{"No", nullptr}, {"Yes", nullptr}}, "No", nullptr, "audio"},
    {"colortvgame_vibration", "Vibration", "Vibrate controllers during gameplay.",
     {{"Off", nullptr}, {"On", nullptr}}, "Off", nullptr, "gameplay"},
    {"colortvgame_original_features", "Original features mode", "Restrict features to original consoles.",
     {{"Off", nullptr}, {"On", nullptr}}, "Off", nullptr, "gameplay"},
    {"colortvgame_rebound", "Rebound behavior", "Choose the approximate paddle rebound model.",
     {{"Hit position", nullptr}, {"Paddle movement", nullptr}}, "Hit position", nullptr, "gameplay"},
    {"colortvgame_anti_stalemate", "Anti-stalemate",
     "Add a small vertical angle on the 3rd consecutive horizontal rebound.",
     {{"On", nullptr}, {"Off", nullptr}}, "On", nullptr, "gameplay"},
    {"colortvgame_start_game", "Selected game at start", "Choose the initial game in the no-content selector.",
     {{"Last used", nullptr}, {"First", nullptr}, {"Random", nullptr}}, "Last used"},
    {"colortvgame_direct_launch_lock", "Lock direct-launched game", "Restrict a direct .ctg session to its loaded game. No-content sessions are unaffected.",
     {{"Off", nullptr}, {"On", nullptr}}, "Off"},
    {"colortvgame_demo_rotation", "Demo game rotation", "Rotate games while the selector is idle.",
     {{"Random every 15 seconds", nullptr}, {"Off", nullptr}, {"Every 15 seconds", nullptr}}, "Random every 15 seconds"},
    {"colortvgame_generate_files", "Generate game files",
     "Generate CTG game files in system/Nintendo Color TV-Game. Files can be moved and used like ROMs. Select Generate, then resume the core.",
     {{"Ready", nullptr}, {"Generate", nullptr}}, "Ready"}
};

inline constexpr size_t count = sizeof(definitions) / sizeof(definitions[0]);

struct Registration {
    std::array<retro_core_option_v2_category, 4> categories{{
        {"video", "Video", nullptr},
        {"audio", "Audio", nullptr},
        {"gameplay", "Gameplay", nullptr},
        {nullptr, nullptr, nullptr}
    }};
    std::array<retro_core_option_v2_definition, count + 1> v2_definitions{};
    std::array<retro_core_option_definition, count + 1> v1_definitions{};
    std::array<std::string, count> legacy_strings;
    std::array<retro_variable, count + 1> variables{};
    retro_core_options_v2 v2{categories.data(), v2_definitions.data()};

    Registration() {
        for (size_t i = 0; i < count; ++i) {
            const auto& option = definitions[i];
            auto& modern = v2_definitions[i];
            auto& previous = v1_definitions[i];
            modern.key = previous.key = option.key;
            modern.desc = previous.desc = option.description;
            modern.info = previous.info = option.information;
            modern.category_key = option.category;
            modern.default_value = previous.default_value = option.default_value;
            for (size_t j = 0; j < RETRO_NUM_CORE_OPTION_VALUES_MAX; ++j)
                modern.values[j] = previous.values[j] = option.values[j];

            auto& legacy = legacy_strings[i];
            legacy = option.legacy_description ? option.legacy_description : option.description;
            legacy += "; ";
            // Legacy frontends use the first value as the default, unlike v1/v2.
            legacy += option.default_value;
            for (const auto& value : option.values) {
                if (!value.value) break;
                if (std::strcmp(value.value, option.default_value) == 0) continue;
                legacy += '|';
                legacy += value.value;
            }
            variables[i] = {option.key, legacy.c_str()};
        }
    }

};

inline void register_options(retro_environment_t environment) {
    // Frontends may retain these pointers, so both definitions and generated strings
    // must outlive registration and remain unchanged across frontend callbacks.
    static Registration registration;
    unsigned version = 0;
    if (!environment(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version)) version = 0;
    if (version >= 2)
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &registration.v2);
    else if (version == 1)
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, registration.v1_definitions.data());
    else
        environment(RETRO_ENVIRONMENT_SET_VARIABLES, registration.variables.data());
}
}
}
