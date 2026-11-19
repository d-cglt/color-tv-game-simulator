#pragma once

#include <array>
#include <filesystem>
#include <random>

namespace ctg::preferences {
// CTGP version 5 adds Racing speed (0 low, 1 medium, 2 high); older records default to high.
// Version 4 adds Racing mode (0 spacious, 1 narrow, 2 two players); older records default to spacious.
// Versions before 3 retain Shooting shot speed from the rally speed setting.
using Data = std::array<unsigned char, 31>;
// CTGD, version 1, grid, fixed score, left score, right score, car collision.
using DiagnosticData = std::array<unsigned char, 10>;

// Invalid or unavailable records leave the caller's current settings intact.
bool read(const std::filesystem::path& path, Data& data) noexcept;
bool write(const std::filesystem::path& path, const Data& data, std::mt19937& random) noexcept;
bool read_diagnostic(const std::filesystem::path& path, DiagnosticData& data) noexcept;
bool write_diagnostic(const std::filesystem::path& path, const DiagnosticData& data, std::mt19937& random) noexcept;
unsigned read_last_game(const std::filesystem::path& path, unsigned fallback) noexcept;
bool write_last_game(const std::filesystem::path& path, unsigned id, std::mt19937& random) noexcept;
}
