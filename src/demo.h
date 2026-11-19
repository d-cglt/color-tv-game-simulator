#pragma once
#include "content.h"
#include "opponent.h"
#include "racing.h"
#include "shooting.h"
#include "tennis_b.h"
#include <array>
#include <random>

namespace ctg {
struct DemoOptions {
    bool high_speed = false;
    std::array<bool, 2> small_paddle{};
    unsigned shooting_target = 2, shooting_direction = 0, shooting_size = 0, shooting_speed = 0;
    unsigned racing_mode = 0;
};

class Demo {
public:
    TennisB tennis;
    Shooting shooting;
    Racing racing;

    unsigned current_id() const { return id_; }
    void reset() { id_ = slot_count; frame_ = 0; }
    unsigned step(unsigned id, bool rebound, bool anti, const DemoOptions& options = {});

private:
    std::array<Opponent, 2> opponents_;
    unsigned id_ = slot_count, frame_ = 0;
    DemoOptions options_{};
    std::mt19937 shooting_random_{1};
    void resolve_shooting_settings(const DemoOptions& options);
    static constexpr unsigned sound_mask = sound_wall | sound_paddle | sound_point | sound_win | sound_shot;
};
}
