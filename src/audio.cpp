#include "audio.h"
#include "racing.h"
#include "tennis_b.h"

namespace ctg {
void Audio::trigger(Tone& tone, unsigned events, bool shooting) {
    events &= sound_wall | sound_paddle | sound_point | sound_win | sound_shot;
    if (!events) return;
    tone = {};
    tone.collision = !(events & (sound_win | sound_point)) &&
                     (!shooting || (events & (sound_paddle | sound_shot)));
    tone.alternating = events & (sound_point | sound_win);
    if (tone.alternating) {
        tone.total = tone.remaining = ((events & sound_win) ? 4800 : 11280) + 960;
    } else if (tone.collision) {
        tone.period = (events & sound_paddle) ? 100 : 200;
        tone.remaining = 2160 + 960;
    } else {
        tone.period = (events & sound_win) ? 80 : (events & sound_point) ? 120 : 60;
        tone.remaining = (events & sound_win) ? 7200 : 2400;
    }
}
void Audio::trigger_game(unsigned events, bool shooting) { trigger(game_, events, shooting); }
void Audio::trigger_demo(unsigned events, unsigned id) { trigger(demo_, events, id == 14); }
void Audio::start_racing() {
    start_sample_ = start_phase_ = 0;
    engine_phase_ = 0;
    crash_sample_ = racing_crash_total_samples;
    bonus_sample_ = racing_bonus_total_samples;
    bonus_phase_ = 0;
}
void Audio::collision() { crash_sample_ = crash_phase_ = 0; }
void Audio::bonus() { bonus_sample_ = bonus_phase_ = 0; }

Audio::Samples Audio::render(bool preview, bool finished, bool paused, unsigned id, unsigned racing_speed) {
    Samples samples{};
    if (preview || finished) {
        crash_sample_ = racing_crash_total_samples;
        bonus_sample_ = racing_bonus_total_samples;
        start_sample_ = racing_start_total;
    }
    Tone& tone = preview ? demo_ : game_;
    for (unsigned i = 0; i < 800 && tone.remaining && !paused; ++i, --tone.remaining) {
        if (tone.collision || tone.alternating) {
            const unsigned midpoint = tone.alternating ? 24000 : tone.period / 2;
            const double input = tone.remaining > 960
                ? (tone.phase < midpoint ? -6000.0 : 6000.0)
                : tone.previous_input;
            tone.filtered = 0.9974 * (tone.filtered + input - tone.previous_input);
            tone.previous_input = input;
            const double sample = tone.remaining <= 960
                ? tone.filtered * tone.remaining / 960 : tone.filtered;
            samples[2 * i] = samples[2 * i + 1] = static_cast<std::int16_t>(sample);
        } else {
            samples[2 * i] = samples[2 * i + 1] = tone.phase < tone.period / 2 ? 6000 : -6000;
        }
        if (tone.alternating) {
            const unsigned elapsed = tone.total - tone.remaining;
            const unsigned frequency = (elapsed / 720) % 2 == 0 ? 480 : 240;
            tone.phase = (tone.phase + frequency) % 48000;
        } else {
            tone.phase = (tone.phase + 1) % tone.period;
        }
    }
    if (id == 15 && !preview && !finished && !paused) {
        for (unsigned i = 0; i < 800; ++i) {
            if (start_sample_ < racing_start_total) {
                const bool final_tone = start_sample_ >= 3 * racing_start_interval;
                if (final_tone || start_sample_ % racing_start_interval < racing_start_pulse) {
                    samples[2 * i] = samples[2 * i + 1] = start_phase_ < 4800 ? 6000 : -667;
                    start_phase_ = (start_phase_ + (final_tone ? 484 : 164)) % 48000;
                }
                ++start_sample_;
            } else {
                const unsigned frequency = racing_speed <= Racing::low_speed
                    ? 128 + 65 * (racing_speed - Racing::start_speed) / (Racing::low_speed - Racing::start_speed)
                    : 193 + 200 * (racing_speed - Racing::low_speed) / (Racing::max_speed - Racing::low_speed);
                samples[2 * i] = samples[2 * i + 1] = engine_phase_ < 4800 ? 6000 : -667;
                engine_phase_ = (engine_phase_ + frequency) % 48000;
            }
            if (crash_sample_ >= racing_crash_total_samples && bonus_sample_ < racing_bonus_total_samples) {
                const unsigned tone_index = bonus_sample_ / racing_bonus_tone_samples;
                const unsigned frequency = tone_index >= 8 ? 874 : tone_index % 3 == 0 ? 475 : tone_index % 3 == 1 ? 627 : 874;
                samples[2 * i] = samples[2 * i + 1] = bonus_phase_ < 4800 ? 6000 : -667;
                bonus_phase_ = (bonus_phase_ + frequency) % 48000;
                ++bonus_sample_;
            }
            if (crash_sample_ < racing_crash_total_samples) {
                const unsigned frequency = (crash_sample_ / racing_crash_segment_samples) % 2 == 0 ? 320 : 627;
                samples[2 * i] = samples[2 * i + 1] = crash_phase_ < 4800 ? 6000 : -667;
                crash_phase_ = (crash_phase_ + frequency) % 48000;
                ++crash_sample_;
            }
        }
    }
    return samples;
}
}
