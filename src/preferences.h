#pragma once

#include <array>
#include <filesystem>
#include <random>
#include <vector>

namespace ctg::preferences {
// CTGP version 8 adds Racing car path (0 straight, 1 zigzag).
// CTGP version 7 adds Racing wall crash (0 off, 1 on); older records default to off.
// CTGP version 6 adds Racing opponents (0 off, 1 on); older records default to off.
// CTGP version 5 adds Racing speed (0 low, 1 medium, 2 high); older records default to high.
// Version 4 adds Racing mode (0 spacious, 1 narrow, 2 two players); older records default to spacious.
// Versions before 3 retain Shooting shot speed from the rally speed setting.
using Data = std::array<unsigned char, 34>;
// CTGD, version 1, grid, fixed score, left score, right score, car collision.
using DiagnosticData = std::array<unsigned char, 10>;

// Invalid or unavailable records leave the caller's current settings intact.
// Opaque, complete v9 entries. Keep this alongside Data across all saves.
using Unknown = std::vector<unsigned char>;
bool read(const std::filesystem::path& path, Data& data, Unknown& unknown) noexcept;
bool write(const std::filesystem::path& path, const Data& data, const Unknown& unknown, std::mt19937& random) noexcept;
bool read_diagnostic(const std::filesystem::path& path, DiagnosticData& data) noexcept;
bool write_diagnostic(const std::filesystem::path& path, const DiagnosticData& data, std::mt19937& random) noexcept;
unsigned read_last_game(const std::filesystem::path& path, unsigned fallback) noexcept;
bool write_last_game(const std::filesystem::path& path, unsigned id, std::mt19937& random) noexcept;
}
