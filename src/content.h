#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>

namespace ctg {
constexpr unsigned slot_count = 16;
std::optional<std::uint8_t> parse_content(const void* data, std::size_t size) noexcept;
}
