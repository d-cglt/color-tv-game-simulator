#pragma once
#include <array>

namespace ctg {
struct ShootingState {
    int shooter_y = 117;
    int target_y = 101;
    int shot_x = 0, shot_y = 0;
    unsigned shots = 0, hits = 0, frame = 0;
    bool flying = false, hit = false, finished = false;
};

class Shooting {
public:
    void reset() { state_ = {}; target_position_ = 405; shot_position_ = 0; }
    static constexpr unsigned default_shot_limit = 15;
    void set_shot_limit(unsigned limit) { shot_limit_ = limit; }
    void set_options(bool high_speed, std::array<bool, 2> small_paddle) { high_speed_ = high_speed; small_paddle_ = small_paddle; }
    int paddle_height(int owner) const { return small_paddle_[owner] ? 16 : 33; }
    int shooter_top() const { return state_.shooter_y - paddle_height(0) / 2; }
    int target_top() const { return state_.target_y + (33 - paddle_height(1)) / 2; }
    unsigned step(int direction, bool fire, int target_direction = 0, bool automatic_target = false);
    const ShootingState& state() const { return state_; }
    bool target_visible() const { return state_.frame % 32 < 26; }
private:
    ShootingState state_{};
    unsigned shot_limit_ = default_shot_limit;
    bool high_speed_ = false;
    std::array<bool, 2> small_paddle_{};
    int target_position_ = 405, shot_position_ = 0;
};
}
