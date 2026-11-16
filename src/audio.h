#pragma once
#include <array>
#include <cstdint>

namespace ctg {
class Audio {
public:
    using Samples = std::array<std::int16_t, 1600>;
    static constexpr unsigned racing_crash_segment_samples = 12960;
    static constexpr unsigned racing_crash_total_samples = 6 * racing_crash_segment_samples;
    static constexpr unsigned racing_bonus_tone_samples = 6336;
    static constexpr unsigned racing_bonus_total_samples = 8 * racing_bonus_tone_samples + 12288;
    static constexpr unsigned racing_start_interval = 51200;
    static constexpr unsigned racing_start_pulse = 25440;
    static constexpr unsigned racing_start_total = 3 * racing_start_interval + 38640;

    void trigger_game(unsigned events, bool shooting);
    // Caller gates this on demo_sounds; id changes must call clear_demo().
    void trigger_demo(unsigned events, unsigned id);
    void clear_demo() { demo_ = {}; }
    // Unlike clear_demo(), preserves the oscillator phase and period.
    void clear_game_tone() { game_.remaining = 0; }
    void start_racing();
    void collision();
    void bonus();
    bool racing_recovering() const { return crash_sample_ < racing_crash_total_samples; }
    bool racing_starting() const { return start_sample_ < racing_start_total; }
    bool racing_player_visible() const {
        return !racing_recovering() || (crash_sample_ / racing_crash_segment_samples) % 2 == 0;
    }
    // One 48 kHz stereo frame (800 sample pairs). Speed uses Racing's units.
    Samples render(bool preview, bool finished, bool paused, unsigned id, unsigned racing_speed);

private:
    struct Tone {
        unsigned remaining = 0, phase = 0, period = 0, total = 0;
        bool collision = false, alternating = false;
        double filtered = 0, previous_input = 0;
    };
    static void trigger(Tone& tone, unsigned events, bool shooting);
    Tone game_, demo_;
    // Static core initialization deliberately starts crash recovery at zero.
    unsigned crash_sample_ = 0, crash_phase_ = 0;
    unsigned bonus_sample_ = racing_bonus_total_samples, bonus_phase_ = 0;
    unsigned start_sample_ = racing_start_total, start_phase_ = 0;
    unsigned engine_phase_ = 0;
};
}
