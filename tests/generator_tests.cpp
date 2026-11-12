#include "content.h"
#include "generator.h"
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

int main() {
    namespace fs = std::filesystem;
    const auto base = fs::temp_directory_path() / ("ctg-generator-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    CHECK(fs::create_directory(base));
    const auto system = base / "system with spaces";
    fs::create_directory(system);
    const auto system_string = system.u8string();
    auto invalid = ctg::generate_game_files(nullptr);
    CHECK(invalid.failed == ctg::slot_count && !invalid.created && !invalid.existing);
    CHECK(ctg::generate_game_files("").failed == ctg::slot_count);

    const char* const old_names[] = {
        "Tennis B - Singles (Color TV Game 15).ctg", "Tennis B - Doubles (Color TV Game 15).ctg",
        "Tennis A - Singles (Color TV Game 6 + 15).ctg", "Tennis A - Doubles (Color TV Game 6 + 15).ctg",
        "Hockey B - Singles (Color TV Game 15).ctg", "Hockey B - Doubles (Color TV Game 15).ctg",
        "Hockey A - Singles (Color TV Game 6 + 15).ctg", "Hockey A - Doubles (Color TV Game 6 + 15).ctg",
        "Volleyball B - Singles (Color TV Game 15).ctg", "Volleyball B - Doubles (Color TV Game 15).ctg",
        "Volleyball A - Singles (Color TV Game 6 + 15).ctg", "Volleyball A - Doubles (Color TV Game 6 + 15).ctg"
    };
    const char* const restored_names[] = {
        "Tennis A - Singles (Color TV Game 15).ctg", "Tennis A - Doubles (Color TV Game 15).ctg",
        "Tennis B - Singles (Color TV Game 6 + 15).ctg", "Tennis B - Doubles (Color TV Game 6 + 15).ctg",
        "Hockey A - Singles (Color TV Game 15).ctg", "Hockey A - Doubles (Color TV Game 15).ctg",
        "Hockey B - Singles (Color TV Game 6 + 15).ctg", "Hockey B - Doubles (Color TV Game 6 + 15).ctg",
        "Volleyball A - Singles (Color TV Game 15).ctg", "Volleyball A - Doubles (Color TV Game 15).ctg",
        "Volleyball B - Singles (Color TV Game 6 + 15).ctg", "Volleyball B - Doubles (Color TV Game 6 + 15).ctg"
    };
    unsigned char descriptor[] = {'C', 'T', 'G', '1', 0};
    for (unsigned id = 0; id < 256; ++id) {
        descriptor[4] = static_cast<unsigned char>(id);
        const auto parsed = ctg::parse_content(descriptor, sizeof(descriptor));
        CHECK(bool(parsed) == (id < 16));
        if (parsed) CHECK(*parsed == id);
    }
    const char* const expected_names[] = {
        "Tennis Variant - Singles (Color TV Game 15).ctg", "Tennis Variant - Doubles (Color TV Game 15).ctg",
        "Tennis Original - Singles (Color TV Game 6 + 15).ctg", "Tennis Original - Doubles (Color TV Game 6 + 15).ctg",
        "Hockey Variant - Singles (Color TV Game 15).ctg", "Hockey Variant - Doubles (Color TV Game 15).ctg",
        "Hockey Original - Singles (Color TV Game 6 + 15).ctg", "Hockey Original - Doubles (Color TV Game 6 + 15).ctg",
        "Volleyball Variant - Singles (Color TV Game 15).ctg", "Volleyball Variant - Doubles (Color TV Game 15).ctg",
        "Volleyball Original - Singles (Color TV Game 6 + 15).ctg", "Volleyball Original - Doubles (Color TV Game 6 + 15).ctg",
        "Ping-Pong - Singles (Color TV Game 15).ctg", "Ping-Pong - Doubles (Color TV Game 15).ctg",
        "Shooting Game (Color TV Game 15).ctg",
        "Racing 112 (Color TV Game Racing 112).ctg"
    };
    static_assert(sizeof(expected_names) / sizeof(expected_names[0]) == ctg::slot_count);
    const auto former_dir = base / "old-save" / "color-tv-game" / "content";
    fs::create_directories(former_dir);
    const auto former_file = former_dir / "old-only.ctg";
    { std::ofstream file(former_file, std::ios::binary); file << "old location"; }
    const auto dir = system / "Nintendo Color TV-Game";
    fs::create_directories(dir);
    for (unsigned id = 0; id < 12; ++id) {
        std::ofstream old(dir / old_names[id], std::ios::binary);
        const char bytes[] = {'C', 'T', 'G', '1', static_cast<char>(id)};
        old.write(bytes, sizeof(bytes));
    }
    for (unsigned id = 0; id < 12; ++id) {
        std::ofstream old(dir / restored_names[id], std::ios::binary);
        const char bytes[] = {'C', 'T', 'G', '1', static_cast<char>(id)};
        old.write(bytes, sizeof(bytes));
    }
    const auto legacy_file = dir / "tennis-a-singles.ctg";
    { std::ofstream file(legacy_file, std::ios::binary); file << "legacy data"; }
        const auto old_shooting_file = dir / "Shooting Game - Original (Color TV Game 15).ctg";
        { std::ofstream file(old_shooting_file, std::ios::binary); file << "old shooting data"; }

    ctg::GenerationResult a, b;
    std::thread first([&] { a = ctg::generate_game_files(system_string.c_str()); });
    std::thread second([&] { b = ctg::generate_game_files(system_string.c_str()); });
    first.join(); second.join();
    CHECK(a.failed == 0 && b.failed == 0);
    CHECK(a.created + b.created == ctg::slot_count);
    CHECK(a.existing + b.existing == ctg::slot_count);
    unsigned count = 0;
    for (const auto& entry : fs::directory_iterator(dir)) { (void)entry; ++count; }
    CHECK(count == ctg::slot_count + 24 + 2);
    for (unsigned id = 0; id < 12; ++id) {
        std::ifstream old(dir / old_names[id], std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(old)), {});
        const auto parsed = ctg::parse_content(bytes.data(), bytes.size());
        CHECK(parsed && *parsed == id);
    }
    for (unsigned id = 0; id < 12; ++id) {
        std::ifstream old(dir / restored_names[id], std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(old)), {});
        const auto parsed = ctg::parse_content(bytes.data(), bytes.size());
        CHECK(parsed && *parsed == id);
    }
    CHECK(!fs::exists(dir / "old-only.ctg") && fs::exists(former_file));
    for (unsigned id = 0; id < ctg::slot_count; ++id) {
        CHECK(std::string(ctg::generated_filenames[id]) == expected_names[id]);
        const fs::path name(expected_names[id]);
        CHECK(name == name.filename() && name.extension() == ".ctg");
        std::ifstream file(dir / name, std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(file)), {});
        auto parsed = ctg::parse_content(bytes.data(), bytes.size());
        CHECK(parsed && *parsed == id);
    }
    const auto protected_file = dir / ctg::generated_filenames[0];
    { std::ofstream file(protected_file, std::ios::binary | std::ios::trunc); file << "user data"; }
    auto again = ctg::generate_game_files(system_string.c_str());
    CHECK(again.created == 0 && again.existing == ctg::slot_count && again.failed == 0);
    std::ifstream file(protected_file, std::ios::binary);
    CHECK(std::string((std::istreambuf_iterator<char>(file)), {}) == "user data");
    file.close();
    std::ifstream legacy(legacy_file, std::ios::binary);
    CHECK(std::string((std::istreambuf_iterator<char>(legacy)), {}) == "legacy data");
    legacy.close();
        std::ifstream old_shooting(old_shooting_file, std::ios::binary);
        CHECK(std::string((std::istreambuf_iterator<char>(old_shooting)), {}) == "old shooting data");
        old_shooting.close();
    // Existing files at current names must not be overwritten.
    const auto collision_system = base / "collision-system";
    const auto collision_dir = collision_system / "Nintendo Color TV-Game";
    fs::create_directories(collision_dir);
    for (unsigned id = 0; id < 12; ++id) {
        std::ofstream existing(collision_dir / expected_names[id], std::ios::binary);
        const char bytes[] = {'C', 'T', 'G', '1', static_cast<char>(id)};
        existing.write(bytes, sizeof(bytes));
    }
    const auto collisions = ctg::generate_game_files(collision_system.u8string().c_str());
    CHECK(collisions.created == 4 && collisions.existing == 12 && collisions.failed == 0);
    for (unsigned id = 0; id < 12; ++id) {
        std::ifstream existing(collision_dir / expected_names[id], std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(existing)), {});
        const auto parsed = ctg::parse_content(bytes.data(), bytes.size());
        CHECK(parsed && *parsed == id);
    }
    const auto blocker = base / "blocker";
    { std::ofstream out(blocker); out << "not a directory"; }
    CHECK(ctg::generate_game_files(blocker.u8string().c_str()).failed == ctg::slot_count);
    fs::remove_all(base);
    return 0;
}
