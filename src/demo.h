#pragma once
#include "content.h"
#include "opponent.h"
#include "racing.h"
#include "shooting.h"
#include "tennis_b.h"
#include <array>

namespace ctg {
class Demo {
public:
    TennisB tennis;
    Shooting shooting;
    Racing racing;

    unsigned current_id() const { return id_; }
    void reset() { id_ = slot_count; frame_ = 0; }
    unsigned step(unsigned id, bool rebound, bool anti);

private:
    std::array<Opponent, 2> opponents_;
    unsigned id_ = slot_count, frame_ = 0;
    static constexpr unsigned sound_mask = sound_wall | sound_paddle | sound_point | sound_win;
};
}
