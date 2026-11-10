#pragma once
#include "tennis_b.h"
#include <cstdint>

namespace ctg {
// Tracks completed active frames; call advance after each gameplay step with
// the pre-step state and resulting events. Paused frames must not be advanced.
class RallyDifficulty {
public:
    void reset() { active_frames_ = 0; hard_cycle_frames_ = 0; }
    unsigned effective(unsigned selected) const {
        if (selected == 2 && active_frames_ >= 900) return 1;
        if (selected == 3 && hard_cycle_frames_ >= 1800)
            return hard_cycle_frames_ < 2700 ? 2 : 1;
        return selected;
    }
    void advance(const TennisState& state, unsigned events) {
        if (state.serve_wait || state.winner != -1 || (events & sound_point)) reset();
        else {
            if (active_frames_ < 900) ++active_frames_;
            if (++hard_cycle_frames_ == 3600) hard_cycle_frames_ = 1800;
        }
    }

private:
    unsigned active_frames_ = 0;
    unsigned hard_cycle_frames_ = 0;
};

// Controls player 2 (TennisInput::right). Call once per frame before TennisB::step.
// Difficulty is clamped to 1..3; reset restores the deterministic sequence.
// Set explicitly_selected_easy to enable the hockey Easy behavior, including on fallback.
// Prediction includes horizontal walls, but approximates nets, blocks, barriers,
// other paddles, and hockey side walls by replanning after their collisions.
class Opponent {
public:
    void reset();
    int step(const TennisState& state, unsigned difficulty, bool explicitly_selected_easy = false);

private:
    unsigned countdown_ = 0;
    unsigned difficulty_ = 0;
    bool hockey_easy_ = false;
    std::uint32_t random_ = 1;
    int target_ = TennisB::height / 2;
};
}
