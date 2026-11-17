#include "preferences.h"
#include "content.h"
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <random>
#include <string>
#include <vector>
#ifndef _WIN32
#include <sys/stat.h>
#endif

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

namespace fs = std::filesystem;
namespace pref = ctg::preferences;
using Bytes = std::vector<unsigned char>;

static bool put(const fs::path& path, const Bytes& bytes) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!bytes.empty()) file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    file.close();
    return !file.fail();
}

static Bytes get(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    return Bytes(std::istreambuf_iterator<char>(file), {});
}

static fs::path temporary(fs::path path, std::mt19937& rng) {
    path += ".tmp-" + std::to_string(rng());
    return path;
}

struct Cleanup {
    fs::path path;
    ~Cleanup() { std::error_code error; fs::remove_all(path, error); }
};

int main() {
    const auto base = fs::temp_directory_path() / ("ctg-preferences-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    CHECK(fs::create_directory(base));
    const Cleanup cleanup{base};
    const auto path = base / "preferences";
    const pref::Data golden{{'C', 'T', 'G', 'P', 1, 1, 0, 1, 15, 30, 4, 2, 1,
                            0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1}};
    const Bytes golden_bytes(golden.begin(), golden.end());
    pref::Data sentinel;
    sentinel.fill(0xa5);
    auto output = sentinel;
    std::mt19937 rng(12345);
    CHECK(!pref::read({}, output));
    CHECK(output == sentinel);
    CHECK(!pref::read(path, output));
    CHECK(output == sentinel);
    CHECK(!pref::read(base, output));
    CHECK(output == sentinel);
    CHECK(pref::write(path, golden, rng));
    CHECK(get(path) == golden_bytes);
    CHECK(pref::read(path, output));
    CHECK(output == golden);

    // Exhaust every field independently: this includes both boundaries and holes
    // in the score fields, without relying on write() to validate the fixture.
    for (std::size_t field = 0; field < golden.size(); ++field) {
        for (unsigned value = 0; value < 256; ++value) {
            auto bytes = golden_bytes;
            bytes[field] = static_cast<unsigned char>(value);
            bool valid;
            if (field < 5) valid = value == golden[field];
            else if (field < 8) valid = value <= 1;
            else if (field < 10) valid = value >= 5 && value <= 30 && value % 5 == 0;
            else if (field == 10) valid = value <= 4;
            else if (field < 13) valid = value <= 2;
            else valid = value <= 3;
            CHECK(put(path, bytes));
            output = sentinel;
            CHECK(pref::read(path, output) == valid);
            auto expected = golden;
            expected[field] = static_cast<unsigned char>(value);
            CHECK(output == (valid ? expected : sentinel));
        }
    }
    for (std::size_t size = 0; size < golden.size(); ++size) {
        CHECK(put(path, Bytes(golden_bytes.begin(), golden_bytes.begin() + size)));
        output = sentinel;
        CHECK(!pref::read(path, output));
        CHECK(output == sentinel);
    }
    for (const auto size : {28u, 29u, 54u, 4096u}) {
        auto bytes = golden_bytes;
        bytes.resize(size, 0);
        CHECK(put(path, bytes));
        output = sentinel;
        CHECK(!pref::read(path, output));
        CHECK(output == sentinel);
    }
    auto arbitrary = sentinel;
    CHECK(pref::write(path, arbitrary, rng));
    CHECK(get(path) == Bytes(arbitrary.begin(), arbitrary.end()));
    output = golden;
    CHECK(!pref::read(path, output));
    CHECK(output == golden);

    const auto diagnostic = base / "diagnostic";
    const pref::DiagnosticData diag{{'C', 'T', 'G', 'D', 1, 1, 0, 0, 30, 1}};
    pref::DiagnosticData diag_sentinel; diag_sentinel.fill(0xa5);
    auto diag_output = diag_sentinel;
    CHECK(!pref::read_diagnostic({}, diag_output));
    CHECK(!pref::read_diagnostic(base, diag_output));
    CHECK(diag_output == diag_sentinel);
    CHECK(pref::write_diagnostic(diagnostic, diag, rng));
    CHECK(pref::read_diagnostic(diagnostic, diag_output) && diag_output == diag);
    for (std::size_t field = 0; field < diag.size(); ++field) {
        for (unsigned value = 0; value < 256; ++value) {
            Bytes bytes(diag.begin(), diag.end()); bytes[field] = static_cast<unsigned char>(value);
            const bool valid = field < 5 ? value == diag[field] :
                               (field == 7 || field == 8) ? value <= 30 : value <= 1;
            CHECK(put(diagnostic, bytes)); diag_output = diag_sentinel;
            CHECK(pref::read_diagnostic(diagnostic, diag_output) == valid);
            auto expected = diag; expected[field] = static_cast<unsigned char>(value);
            CHECK(diag_output == (valid ? expected : diag_sentinel));
        }
    }
    for (std::size_t size = 0; size < diag.size(); ++size) {
        CHECK(put(diagnostic, Bytes(diag.begin(), diag.begin() + size)));
        diag_output = diag_sentinel;
        CHECK(!pref::read_diagnostic(diagnostic, diag_output) && diag_output == diag_sentinel);
    }
    CHECK(put(diagnostic, Bytes(11, 0)));
    CHECK(!pref::read_diagnostic(diagnostic, diag_output));

    const auto last = base / "last-game";
    constexpr unsigned fallback = 1234;
    static_assert(ctg::slot_count == 16);
    CHECK(pref::read_last_game({}, fallback) == fallback);
    CHECK(pref::read_last_game(last, fallback) == fallback);
    CHECK(pref::read_last_game(base, fallback) == fallback);
    for (unsigned id = 0; id < 256; ++id) {
        CHECK(pref::write_last_game(last, id, rng));
        CHECK(get(last) == Bytes{static_cast<unsigned char>(id)});
        CHECK(pref::read_last_game(last, fallback) == (id < ctg::slot_count ? id : fallback));
    }
    for (const unsigned id : {256u, 271u, 511u, std::numeric_limits<unsigned>::max()}) {
        CHECK(pref::write_last_game(last, id, rng));
        const auto byte = static_cast<unsigned char>(id);
        CHECK(get(last) == Bytes{byte});
        CHECK(pref::read_last_game(last, fallback) == (byte < ctg::slot_count ? byte : fallback));
    }
    for (const auto size : {0u, 2u, 3u, 27u, 4096u}) {
        CHECK(put(last, Bytes(size, 0)));
        CHECK(pref::read_last_game(last, fallback) == fallback);
    }

    const auto unicode_dir = base / fs::u8path(u8"préférences-日本語");
    CHECK(fs::create_directory(unicode_dir));
    const auto unicode_path = unicode_dir / fs::u8path(u8"réglages-遊戯");
    CHECK(pref::write(unicode_path, golden, rng));
    CHECK(get(unicode_path) == golden_bytes);
    CHECK(pref::read(unicode_path, output));
    CHECK(output == golden);
    CHECK(pref::write_last_game(unicode_path, 15, rng));
    CHECK(get(unicode_path) == Bytes{15});
    CHECK(pref::read_last_game(unicode_path, fallback) == 15);

    // Exercise the same atomic-writer contract through both public entry points.
    for (unsigned kind = 0; kind < 3; ++kind) {
        const auto dir = base / ("atomic-" + std::to_string(kind));
        CHECK(fs::create_directory(dir));
        const auto target = dir / "target";
        const Bytes expected = kind == 1 ? Bytes{7} : kind == 2 ? Bytes(diag.begin(), diag.end()) : golden_bytes;
        const Bytes original{9, 8, 7};
        auto write = [&](const fs::path& destination, std::mt19937& engine) {
            return kind == 1 ? pref::write_last_game(destination, 7, engine)
                 : kind == 2 ? pref::write_diagnostic(destination, diag, engine)
                             : pref::write(destination, golden, engine);
        };
        CHECK(!write({}, rng));
        CHECK(put(target, original));
        CHECK(write(target, rng));
        CHECK(get(target) == expected);
        CHECK(std::distance(fs::directory_iterator(dir), fs::directory_iterator{}) == 1);
#ifndef _WIN32
        // Only assert mode bits on a filesystem that honors POSIX permissions.
        const auto probe = dir / "mode-probe";
        CHECK(put(probe, {}));
        struct stat info{};
        if (::chmod(probe.c_str(), 0600) == 0 && ::stat(probe.c_str(), &info) == 0 &&
            (info.st_mode & 0777) == 0600) {
            CHECK(::stat(target.c_str(), &info) == 0);
            CHECK((info.st_mode & 0777) == 0600);
        }
        CHECK(fs::remove(probe));
#endif
        const auto directory_target = dir / "directory";
        CHECK(fs::create_directory(directory_target));
        CHECK(put(directory_target / "keep", original));
        CHECK(!write(directory_target, rng));
        CHECK(get(directory_target / "keep") == original);
        CHECK(std::distance(fs::directory_iterator(dir), fs::directory_iterator{}) == 2);
        CHECK(!write(dir / "missing" / "target", rng));
        CHECK(!fs::exists(dir / "missing"));
        CHECK(std::distance(fs::directory_iterator(dir), fs::directory_iterator{}) == 2);

        std::mt19937 collision_rng(2468);
        auto prediction = collision_rng;
        const auto collision = temporary(target, prediction);
        CHECK(put(collision, original));
        CHECK(write(target, collision_rng));
        CHECK(get(target) == expected);
        CHECK(get(collision) == original);
        CHECK(std::distance(fs::directory_iterator(dir), fs::directory_iterator{}) == 3);
        CHECK(fs::remove(collision));

        CHECK(put(target, original));
        std::mt19937 exhausted_rng(9876);
        prediction = exhausted_rng;
        std::vector<fs::path> collisions;
        for (unsigned attempt = 0; attempt < 16; ++attempt) {
            collisions.push_back(temporary(target, prediction));
            CHECK(put(collisions.back(), original));
        }
        CHECK(!write(target, exhausted_rng));
        CHECK(get(target) == original);
        for (const auto& entry : collisions) CHECK(get(entry) == original);
        CHECK(std::distance(fs::directory_iterator(dir), fs::directory_iterator{}) == 18);
        // Exactly sixteen names were consumed; no seventeenth attempt is made.
        CHECK(exhausted_rng == prediction);
    }
    return 0;
}
