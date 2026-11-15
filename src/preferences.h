#pragma once

#include <array>
#include <filesystem>
#include <random>

namespace ctg::preferences {
// CTGP, version 1, then eight single-byte settings and one mode per rally game ID.
using Data = std::array<unsigned char, 27>;

// Invalid or unavailable records leave the caller's current settings intact.
bool read(const std::filesystem::path& path, Data& data) noexcept;
bool write(const std::filesystem::path& path, const Data& data, std::mt19937& random) noexcept;
unsigned read_last_game(const std::filesystem::path& path, unsigned fallback) noexcept;
bool write_last_game(const std::filesystem::path& path, unsigned id, std::mt19937& random) noexcept;
}
