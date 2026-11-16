#include "libretro.h"
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return false; } } while (0)

namespace {
struct Expected {
    const char* key;
    const char* label;
    const char* description;
    std::vector<const char*> values;
    const char* default_value;
    const char* legacy;
};
const Expected expected[] = {
    {"colortvgame_generate_files", "Generate game files",
     "Generate CTG game files in system/Nintendo Color TV-Game. Files can be moved and used like ROMs. Select Generate, then resume the core.",
     {"Ready", "Generate"}, "Ready", "Generate game files; Ready|Generate"},
    {"colortvgame_vibration", "Vibration", "Vibrate controllers during gameplay.",
     {"Off", "On"}, "Off", "Vibration; Off|On"},
    {"colortvgame_shooting_fire_sound", "Shooting fire sound", "Play a firing sound in Shooting gameplay and previews. Off matches the original console's silent firing.",
     {"On", "Off"}, "On", "Shooting fire sound; On|Off"},
    {"colortvgame_demo_sounds", "Demo sounds", "Play sounds during menu previews.",
     {"On", "Off"}, "On", "Demo sounds; On|Off"},
    {"colortvgame_start_game", "Selected game at start", "Choose the initial game in the no-content selector.",
     {"Last used", "First", "Random"}, "Last used", "Selected game at start; Last used|First|Random"},
    {"colortvgame_demo_rotation", "Demo game rotation", "Rotate games while the selector is idle.",
     {"Random every 15 seconds", "Off", "Every 15 seconds"}, "Random every 15 seconds",
     "Demo game rotation; Random every 15 seconds|Off|Every 15 seconds"},
    {"colortvgame_rebound", "Rebound behavior", "Choose the approximate paddle rebound model.",
     {"Hit position", "Paddle movement"}, "Hit position", "Rebound behavior; Hit position|Paddle movement"},
    {"colortvgame_anti_stalemate", "Anti-stalemate",
     "On the third consecutive horizontal paddle rebound, add a small vertical angle at the hit. Modern approximation, not proven original hardware behavior. Applies to competitive games and demos; Off disables it.",
     {"On", "Off"}, "On", "Anti-stalemate; On|Off"},
    {"colortvgame_color", "Color", "Display the rendered image in color or grayscale.",
     {"On", "Off"}, "On", "Color; On|Off"},
    {"colortvgame_rf_filter", "RF filter", "Approximate composite signal color and bandwidth artifacts.",
     {"Off", "On"}, "Off", "RF filter; Off|On"},
    {"colortvgame_rf_quality", "RF signal quality", "Lower quality adds color bleed and noise. No effect when the RF filter is off.",
     {"0%", "10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%"}, "100%",
     "RF signal quality (no effect when RF filter is off); 100%|0%|10%|20%|30%|40%|50%|60%|70%|80%|90%"}
};
constexpr unsigned option_count = sizeof(expected) / sizeof(expected[0]);
static_assert(option_count == 11, "Golden option count");
bool equal(const char* actual, const char* golden) {
    return actual && std::strcmp(actual, golden) == 0;
}
struct Registration {
    unsigned command;
    const void* data;
};
std::vector<Registration> registrations;
unsigned reported_version = 0, queries = 0, support_calls = 0;
bool query_supported = true, write_version = true, support_valid = true;
bool environment(unsigned command, void* data) {
    if (command == RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME) {
        ++support_calls;
        support_valid = support_valid && data && *static_cast<bool*>(data);
        return true;
    }
    if (command == RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION) {
        ++queries;
        if (write_version) *static_cast<unsigned*>(data) = reported_version;
        return query_supported;
    }
    if (command == RETRO_ENVIRONMENT_SET_VARIABLES ||
        command == RETRO_ENVIRONMENT_SET_CORE_OPTIONS ||
        command == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2) {
        registrations.push_back({command, data});
        return true;
    }
    return false;
}
template<class Definition>
bool modern(const Definition* definitions) {
    CHECK(definitions);
    for (unsigned i = 0; i < option_count; ++i) {
        const auto& d = definitions[i];
        const auto& e = expected[i];
        CHECK(equal(d.key, e.key));
        CHECK(equal(d.desc, e.label));
        CHECK(equal(d.info, e.description));
        CHECK(equal(d.default_value, e.default_value));
        for (unsigned j = 0; j < RETRO_NUM_CORE_OPTION_VALUES_MAX; ++j) {
            if (j < e.values.size()) CHECK(equal(d.values[j].value, e.values[j]));
            else CHECK(d.values[j].value == nullptr);
            CHECK(d.values[j].label == nullptr);
        }
    }
    const auto& end = definitions[option_count];
    CHECK(end.key == nullptr && end.desc == nullptr && end.info == nullptr && end.default_value == nullptr);
    for (const auto& value : end.values) CHECK(value.value == nullptr && value.label == nullptr);
    return true;
}
bool validate(const Registration& registration) {
    CHECK(registration.data);
    if (registration.command == RETRO_ENVIRONMENT_SET_VARIABLES) {
        const auto* variables = static_cast<const retro_variable*>(registration.data);
        for (unsigned i = 0; i < option_count; ++i) {
            CHECK(equal(variables[i].key, expected[i].key));
            CHECK(equal(variables[i].value, expected[i].legacy));
        }
        CHECK(variables[option_count].key == nullptr && variables[option_count].value == nullptr);
    } else if (registration.command == RETRO_ENVIRONMENT_SET_CORE_OPTIONS) {
        CHECK(modern(static_cast<const retro_core_option_definition*>(registration.data)));
    } else {
        CHECK(registration.command == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2);
        const auto* options = static_cast<const retro_core_options_v2*>(registration.data);
        CHECK(options->categories == nullptr);
        CHECK(modern(options->definitions));
        for (unsigned i = 0; i <= option_count; ++i) {
            const auto& d = options->definitions[i];
            CHECK(d.desc_categorized == nullptr && d.info_categorized == nullptr && d.category_key == nullptr);
        }
    }
    return true;
}
bool run() {
    retro_set_environment(nullptr);
    CHECK(registrations.empty() && queries == 0 && support_calls == 0);
    struct Case { bool supported; bool writes; unsigned version; unsigned command; };
    const Case cases[] = {
        {false, false, 0, RETRO_ENVIRONMENT_SET_VARIABLES},
        {false, true, 2, RETRO_ENVIRONMENT_SET_VARIABLES},
        {true, true, 0, RETRO_ENVIRONMENT_SET_VARIABLES},
        {true, true, 1, RETRO_ENVIRONMENT_SET_CORE_OPTIONS},
        {true, true, 2, RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2},
        {true, true, 3, RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2},
        {true, true, std::numeric_limits<unsigned>::max(), RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2}
    };
    for (unsigned repeat = 0; repeat < 3; ++repeat) {
        for (const auto& test : cases) {
            query_supported = test.supported;
            write_version = test.writes;
            reported_version = test.version;
            const auto previous_size = registrations.size();
            const auto previous_queries = queries;
            const auto previous_support = support_calls;
            retro_set_environment(environment);
            CHECK(queries == previous_queries + 1);
            CHECK(support_calls == previous_support + 1 && support_valid);
            CHECK(registrations.size() == previous_size + 1);
            CHECK(registrations.back().command == test.command);
            // Validate retained raw pointers after return and after other API versions registered.
            for (const auto& saved : registrations) CHECK(validate(saved));
            retro_set_environment(nullptr);
            CHECK(queries == previous_queries + 1 && support_calls == previous_support + 1);
            CHECK(registrations.size() == previous_size + 1);
            for (const auto& saved : registrations) CHECK(validate(saved));
        }
    }
    return true;
}
}
int main() {
    if (!run()) return 1;
    std::puts("core_options_tests passed");
    return 0;
}
