#include "content.h"
#include <cstring>

namespace ctg {
std::optional<std::uint8_t> parse_content(const void* data, std::size_t size) noexcept {
    if (!data || size != 5 || std::memcmp(data, "CTG1", 4) != 0)
        return std::nullopt;
    const auto id = static_cast<const std::uint8_t*>(data)[4];
    if (id >= slot_count) return std::nullopt;
    return id;
}
}
