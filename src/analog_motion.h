#pragma once
#include <algorithm>
#include <cstdlib>

namespace ctg {
// Signed fixed-point remainder is discarded on stop or reversal.
inline int analog_motion(int axis, int speed, int& remainder) {
    constexpr int deadzone = 8192, range = 32767 - deadzone;
    axis = std::clamp(axis, -32767, 32767);
    if (std::abs(axis) <= deadzone) { remainder = 0; return 0; }
    const int deflection = axis > 0 ? axis - deadzone : axis + deadzone;
    if ((remainder > 0 && deflection < 0) || (remainder < 0 && deflection > 0)) remainder = 0;
    remainder += deflection * speed;
    const int movement = remainder / range;
    remainder %= range;
    return movement;
}
}
