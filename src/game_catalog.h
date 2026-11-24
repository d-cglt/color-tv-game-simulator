#pragma once

#include "content.h"
#include "tennis_b.h"
#include <array>

namespace ctg {
inline TennisB make_rally_game(unsigned id) {
    return TennisB(id < 14 && id % 2 == 1,
                   id >= 4 && id <= 7, id >= 8 && id <= 11,
                   id <= 1, id == 4 || id == 5,
                   id == 8 || id == 9, id == 12 || id == 13);
}

inline constexpr std::array<unsigned, ctg::slot_count> selector_order{{
    2, 3, 6, 7, 10, 11, 0, 1, 4, 5, 8, 9, 12, 13, 14, 15
}};
struct Title { const char* name; const char* variant; };
inline constexpr std::array<Title, ctg::slot_count> titles{{
    {"TENNIS VARIANT", "SINGLES"}, {"TENNIS VARIANT", "DOUBLES"},
    {"TENNIS ORIGINAL", "SINGLES"}, {"TENNIS ORIGINAL", "DOUBLES"},
    {"HOCKEY VARIANT", "SINGLES"}, {"HOCKEY VARIANT", "DOUBLES"},
    {"HOCKEY ORIGINAL", "SINGLES"}, {"HOCKEY ORIGINAL", "DOUBLES"},
    {"VOLLEYBALL VARIANT", "SINGLES"}, {"VOLLEYBALL VARIANT", "DOUBLES"},
    {"VOLLEYBALL ORIGINAL", "SINGLES"}, {"VOLLEYBALL ORIGINAL", "DOUBLES"},
    {"PING PONG", "SINGLES"}, {"PING PONG", "DOUBLES"},
    {"SHOOTING GAME", ""}, {"RACING 112", ""}
}};
constexpr unsigned title_length(const char* s) {
    unsigned length = 0;
    while (*s++) ++length;
    return length;
}
constexpr bool titles_fit_selector() {
    for (const auto& title : titles) {
        const unsigned length = title_length(title.name) + title_length(title.variant) + (title.variant[0] ? 1 : 0);
        if (length * 6 > 210) return false;
    }
    return true;
}
static_assert(titles_fit_selector(), "Game titles must fit the selector highlight");
}
