#include "core_test_harness.h"
#include "bundled_selectors.h"

#include <chrono>

#include <filesystem>
#include <fstream>
#include <iterator>

namespace core_test {
namespace fs = std::filesystem;
int persistence_selector_demo_scenarios(ContentContext& context) {
    auto& bytes = context.bytes;
    auto& game = context.game;
    // Persist menu settings independently of the selected-game startup setting.
    // Compare rendered rows rather than depending on the settings file format.
    const auto settings_temp = fs::temp_directory_path() / ("ctg-settings-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    CHECK(fs::create_directory(settings_temp));
    const auto settings_file = settings_temp / "colortvgame-preferences";
    save_directory = settings_temp.u8string(); save_available = true;
    start_game_value = "First";
    buttons = buttons2 = 0; axes = {{0, 0}}; keys.fill(false);
    retro_init();
    auto load_direct = [&](unsigned id) {
        const auto path = (fs::path(CONTENT_DIR) / bundled_selectors.at(id)).u8string();
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;
        std::vector<unsigned char> content((std::istreambuf_iterator<char>(in)), {});
        retro_game_info direct{}; direct.path = path.c_str(); direct.data = content.data(); direct.size = content.size();
        buttons = 0;
        if (!retro_load_game(&direct)) return false;
        retro_run(); return true;
    };
    // Direct Hockey launch previews and gameplay use the saved ice setting.
    CHECK(load_direct(6));
    const auto normal_court = at(frame, 60, 30);
    for (int i = 0; i < 6; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    const auto icy_court = at(frame, 60, 30);
    CHECK(icy_court != normal_court);
    retro_unload_game(); retro_deinit(); retro_init();
    CHECK(load_direct(7));
    CHECK(at(frame, 60, 30) == icy_court);
    press(RETRO_DEVICE_ID_JOYPAD_START);
    CHECK(at(frame, 60, 30) == 0xb9e4ed);
    retro_unload_game();
    CHECK(load_direct(6));
    for (int i = 0; i < 6; ++i) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    retro_unload_game();
    auto saved_opponents = [&]() {
        std::ifstream in(settings_file, std::ios::binary);
        std::vector<unsigned char> record((std::istreambuf_iterator<char>(in)), {});
        return record.size() >= 150 && record[4] == 9 ? record[5 + (31 - 5) * 5 + 4] : -1;
    };
    auto saved_wall_crash = [&]() {
        std::ifstream in(settings_file, std::ios::binary);
        std::vector<unsigned char> record((std::istreambuf_iterator<char>(in)), {});
        return record.size() >= 150 && record[4] == 9 ? record[5 + (32 - 5) * 5 + 4] : -1;
    };
    auto menu_rows = [&](unsigned id) {
        std::vector<std::vector<unsigned char>> rows{frame};
        for (unsigned row = 1; row < 7u; ++row) {
            press(RETRO_DEVICE_ID_JOYPAD_DOWN); rows.push_back(frame);
        }
        return rows;
    };
    auto check_rows = [&](unsigned id, const std::vector<std::vector<unsigned char>>& expected) {
        const auto actual = menu_rows(id);
        if (actual.size() != expected.size()) return false;
        for (size_t row = 0; row < actual.size(); ++row)
            if (!same_menu(actual[row], expected[row])) return false;
        return true;
    };
    std::vector<std::vector<std::vector<unsigned char>>> default_rows, saved_rows;
    for (unsigned id = 0; id < 15; ++id) {
        CHECK(load_direct(id)); default_rows.push_back(menu_rows(id)); retro_unload_game();
    }
    // Set all shared competitive controls to non-default values.
    CHECK(load_direct(2));
    for (int row = 0; row < 2; ++row) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    for (int row = 0; row < 4; ++row) {
        press(RETRO_DEVICE_ID_JOYPAD_RIGHT); press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    }
    retro_unload_game();
    // Give every competitive title its own mode, including untouched defaults.
    for (unsigned id = 0; id < 14; ++id) {
        CHECK(load_direct(id)); press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        for (unsigned choice = 0; choice < id % 4; ++choice) press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        retro_unload_game();
    }
    CHECK(load_direct(14));
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_RIGHT); // target
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_RIGHT); // direction
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_LEFT); // random target size
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_LEFT); // random shots speed, independent of rally speed
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); press(RETRO_DEVICE_ID_JOYPAD_LEFT); // shots, independent of score
    retro_unload_game();
    for (unsigned id = 0; id < 15; ++id) {
        CHECK(load_direct(id)); saved_rows.push_back(menu_rows(id)); retro_unload_game();
        CHECK(!same_menu(saved_rows[id][0], default_rows[id][0]));
    }
    // Racing has three modes; save narrow independently of the other games.
    CHECK(load_direct(15));
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    auto racing_spacious = frame;
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    auto racing_narrow = frame;
    CHECK(!same_menu(racing_spacious, racing_narrow));
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    const auto speed_high = frame;
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    const auto speed_low = frame;
    CHECK(!same_menu(speed_high, speed_low));
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    auto speed_medium = frame;
    CHECK(!same_menu(speed_low, speed_medium) && !same_menu(speed_high, speed_medium));
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, speed_low));
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, speed_high));
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(same_menu(frame, speed_medium));
    // BAD ROAD and WALL CRASH toggle in both directions.
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    const auto bad_road = frame;
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(!same_menu(frame, bad_road));
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, bad_road));
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); const auto bad_road_on = frame;
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(saved_wall_crash() == 0);
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(saved_wall_crash() == 1);
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(saved_wall_crash() == 0);
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(saved_wall_crash() == 1);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    const auto opponents_off = frame;
    press(RETRO_DEVICE_ID_JOYPAD_LEFT);
    const auto opponents_on = frame;
    CHECK(saved_opponents() == 1);
    CHECK(!same_menu(opponents_off, opponents_on));
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(saved_opponents() == 0); CHECK(same_menu(frame, opponents_off));
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(saved_opponents() == 1); CHECK(same_menu(frame, opponents_on));
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    const auto straight = frame;
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(!same_menu(frame, straight));
    {
        std::ifstream in(settings_file, std::ios::binary);
        std::vector<unsigned char> record((std::istreambuf_iterator<char>(in)), {});
        CHECK(record.size() >= 150 && record[4] == 9 && record[149] == 1);
    }
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, straight));
    for (unsigned row = 0; row < 4; ++row) press(RETRO_DEVICE_ID_JOYPAD_UP);
    speed_medium = frame;
    press(RETRO_DEVICE_ID_JOYPAD_UP);
    racing_narrow = frame;
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); const auto racing_two_player = frame;
    CHECK(!same_menu(racing_two_player, racing_narrow) && !same_menu(racing_two_player, racing_spacious));
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); racing_spacious = frame;
    CHECK(!same_menu(racing_spacious, racing_two_player) && !same_menu(racing_spacious, racing_narrow));
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, racing_narrow));
    retro_reset(); buttons = 0; retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); CHECK(same_menu(frame, racing_narrow));
    retro_unload_game();
    CHECK(load_direct(15)); press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, racing_narrow)); retro_unload_game();
    CHECK(fs::exists(settings_file));
    const std::vector<unsigned char> future_entry{0x34, 0x12, 3, 0, 0, 0xff, 42};
    {
        std::ofstream out(settings_file, std::ios::binary | std::ios::app);
        out.write(reinterpret_cast<const char*>(future_entry.data()), future_entry.size());
        CHECK(out.good());
    }
    // Reset and reload must preserve every row, including random Shooting size and speed.
    for (unsigned id = 0; id < 15; ++id) {
        CHECK(load_direct(id)); retro_reset(); buttons = 0; retro_run();
        CHECK(check_rows(id, saved_rows[id])); retro_unload_game();
    }
    retro_deinit(); retro_init();
    for (unsigned id = 0; id < 15; ++id) {
        CHECK(load_direct(id)); CHECK(check_rows(id, saved_rows[id])); retro_unload_game();
    }
    CHECK(load_direct(15)); press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, racing_narrow));
    CHECK(saved_wall_crash() == 1);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); CHECK(same_menu(frame, speed_medium));
    press(RETRO_DEVICE_ID_JOYPAD_DOWN); CHECK(same_menu(frame, bad_road_on));
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    // A failed save is retried on unload without dropping unknown entries.
    save_available = false;
    press(RETRO_DEVICE_ID_JOYPAD_LEFT); CHECK(saved_wall_crash() == 1);
    save_available = true;
    retro_unload_game(); CHECK(saved_wall_crash() == 0);
    CHECK(load_direct(15));
    for (unsigned row = 0; row < 4; ++row) press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(saved_wall_crash() == 1);
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, opponents_on));
    for (unsigned row = 0; row < 3; ++row) press(RETRO_DEVICE_ID_JOYPAD_UP);
    press(RETRO_DEVICE_ID_JOYPAD_UP);
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, racing_two_player));
    retro_unload_game(); retro_deinit(); retro_init();
    CHECK(load_direct(15)); press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    CHECK(same_menu(frame, racing_two_player));
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT); CHECK(same_menu(frame, racing_spacious)); retro_unload_game();
    {
        std::ifstream in(settings_file, std::ios::binary);
        std::vector<unsigned char> record((std::istreambuf_iterator<char>(in)), {});
        CHECK(record.size() >= future_entry.size());
        CHECK(std::equal(future_entry.begin(), future_entry.end(), record.end() - future_entry.size()));
    }
    // Selector launches and direct launches share the same persisted settings.
    CHECK(retro_load_game(nullptr)); buttons = 0; retro_run();
    for (unsigned position = 0; position < 15; ++position) {
        const unsigned id = display_ids[position];
        press(RETRO_DEVICE_ID_JOYPAD_A); CHECK(check_rows(id, saved_rows[id]));
        press(RETRO_DEVICE_ID_JOYPAD_B); press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    }
    retro_unload_game(); retro_deinit();
    // Missing, unavailable and malformed settings must never inherit stale values.
    for (int invalid = 0; invalid < 4; ++invalid) {
        fs::remove(settings_file);
        if (invalid >= 2) {
            std::ofstream out(settings_file, std::ios::binary | std::ios::trunc);
            if (invalid == 3) out << "not a valid settings record\n";
            CHECK(out.good());
        }
        save_available = invalid != 1;
        retro_init();
        for (unsigned id = 0; id < 15; ++id) {
            CHECK(load_direct(id)); CHECK(check_rows(id, default_rows[id])); retro_unload_game();
        }
        retro_deinit();
    }
    fs::remove_all(settings_temp); save_available = true;

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
    for (unsigned id = 0; id < display_ids.size(); ++id) {
        selectors.push_back(frame);
        buttons = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN; retro_run();
        buttons = 0; retro_run();
        { std::ifstream in(last_game, std::ios::binary);
          CHECK(in.get() == static_cast<int>(display_ids[(id + 1) % display_ids.size()]));
          CHECK(in.get() == std::char_traits<char>::eof()); }
    }
    retro_unload_game();
    for (unsigned id = 0; id < display_ids.size(); ++id) {
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
        for (unsigned id : {0u, 7u, 14u, 15u}) {
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
    for (const std::vector<unsigned char>& record : std::vector<std::vector<unsigned char>>{{7}, {}, {16}, {255}, {7, 0}}) {
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
    std::array<bool, 16> seen{};
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

        std::array<bool, 16> rotated_seen{};
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

    return 0;
}
} // namespace core_test
