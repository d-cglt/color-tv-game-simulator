#include "core_test_harness.h"
#include "bundled_selectors.h"
#include "content.h"

#include <chrono>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace core_test {
namespace fs = std::filesystem;
int boot_generator_scenarios(ContentContext& context) {
    auto& bytes = context.bytes;
    auto& game = context.game;
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
    CHECK(vibration_definition.find("; Off|On") != std::string::npos);
    for (unsigned version : {1u, 2u}) {
        core_options_version = version; anti_stalemate_modern_definition = false; vibration_modern_definition = false; rf_quality_modern_definition = false;
        retro_set_environment(env);
        CHECK(anti_stalemate_modern_definition && vibration_modern_definition && rf_quality_modern_definition);
    }
    core_options_version = 0; retro_set_environment(env);
    CHECK(retro_load_game(nullptr)); retro_run();
    CHECK(system_queries == 0 && set_variable_calls == 0 && !fs::exists(temp / "Nintendo Color TV-Game"));
    for (int i = 0; i < 3; ++i) retro_run();
    CHECK(system_queries == 0);
    set_generate_value("Ready"); retro_run();
    set_generate_value("Generate"); retro_run();
    CHECK(system_queries == 1 && last_message.find("16 created, 0 existing, 0 failed") != std::string::npos);
    CHECK(fs::exists(temp / "Nintendo Color TV-Game" / "Tennis Variant - Singles (Color TV Game 15).ctg"));
        CHECK(fs::exists(temp / "Nintendo Color TV-Game" / "Racing 112 (Color TV Game Racing 112).ctg"));
    CHECK(set_variable_calls == 1 && last_set_key == "colortvgame_generate_files" && last_set_value == "Ready" && generate_value == "Ready");
    for (int i = 0; i < 30; ++i) retro_run();
    CHECK(system_queries == 1 && set_variable_calls == 1);
    set_generate_value("Generate"); retro_run();
    CHECK(system_queries == 2 && last_message.find("0 created, 16 existing, 0 failed") != std::string::npos);
    CHECK(set_variable_calls == 2 && generate_value == "Ready");
    retro_run();
    system_available = false;
    set_generate_value("Generate"); retro_run();
    CHECK(system_queries == 3 && last_message.find("16 failed") != std::string::npos && last_message.find("system directory unavailable") != std::string::npos);
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
    CHECK(last_message.find("0 created, 16 existing, 0 failed") != std::string::npos);
    set_variable_supported = true; set_generate_value("Ready");
    retro_run();
    // Frontends without update notifications must still detect option changes.
    variable_update_supported = false;
    set_generate_value("Generate"); retro_run();
    CHECK(system_queries == 6 && last_message.find("0 created, 16 existing, 0 failed") != std::string::npos);
    for (int i = 0; i < 30; ++i) retro_run();
    CHECK(system_queries == 6);
    variable_update_supported = true;
    retro_run();
    // Some frontends return a changed value without setting the update flag.
    generate_value = "Generate"; variable_updated = false;
    retro_run();
    CHECK(system_queries == 7 && last_message.find("0 created, 16 existing, 0 failed") != std::string::npos);
    for (int i = 0; i < 30; ++i) retro_run();
    CHECK(system_queries == 7);
    retro_unload_game(); retro_deinit(); fs::remove_all(temp);
    frames = polls = audio_frames = messages = 0;
    bytes = {{'C','T','G','1',0,0}};
    CHECK(!ctg::parse_content(nullptr, 5));
    for (size_t n = 0; n <= bytes.size(); ++n) CHECK(bool(ctg::parse_content(bytes.data(), n)) == (n == 5));
    for (unsigned id = 0; id < 256; ++id) {
        bytes[4] = static_cast<unsigned char>(id);
        auto parsed = ctg::parse_content(bytes.data(), 5);
        CHECK(bool(parsed) == (id < 16));
        if (parsed) CHECK(*parsed == id);
    }
    bytes[4] = 0;
    for (unsigned i = 0; i < 4; ++i) { bytes[i] ^= 1; CHECK(!ctg::parse_content(bytes.data(), 5)); bytes[i] ^= 1; }
    for (unsigned id = 0; id < bundled_selectors.size(); ++id) {
        const auto path = fs::path(CONTENT_DIR) / bundled_selectors[id];
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
    CHECK(info.library_version && std::strcmp(info.library_version, "0.2.0") == 0);
    CHECK(std::strlen(info.library_name) == 23);
    CHECK(!info.need_fullpath && std::strcmp(info.valid_extensions, "ctg") == 0);
    retro_system_av_info av{}; retro_get_system_av_info(&av);
    CHECK(av.geometry.base_width == 320 && av.timing.fps == 60.0);
    game = {}; game.data = bytes.data(); game.size = 6;
    CHECK(!retro_load_game(&game));
    game.size = 5; accept_format = false; CHECK(!retro_load_game(&game)); accept_format = true;
    CHECK(messages >= 2);

    // Compare menu ink and selection highlights, not the moving scenery behind them.

    return 0;
}
} // namespace core_test
