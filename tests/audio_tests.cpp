#include "audio.h"
#include "tennis_b.h"
#include <cstdio>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

// Independent scalar oracle: literal legacy timings, no Audio constants or helpers.
// Each call advances a single mono sample; frame tests compare both channels.
struct Baseline {
    struct Tone { unsigned left = 0, phase = 0, period = 0; } game, demo;
    unsigned crash = 0, crash_phase = 0, bonus = 62976, bonus_phase = 0;
    unsigned start = 192240, start_phase = 0, engine_phase = 0;

    void trigger(unsigned events, bool shooting, bool preview) {
        events &= 15;
        if (!events) return;
        Tone& t = preview ? demo : game;
        t = {(events & 8) ? 7200u : 2400u, 0,
             (events & 8) ? 80u : (events & 4) ? 120u : (shooting || (events & 2)) ? 60u : 100u};
    }
    void restart() { start = start_phase = engine_phase = bonus_phase = 0; crash = 77760; bonus = 62976; }
    static int pulse(unsigned& phase, unsigned frequency) {
        const int sample = phase < 4800 ? 6000 : -667;
        phase = (phase + frequency) % 48000;
        return sample;
    }
    int sample(bool preview, bool finished, bool paused, unsigned id, unsigned speed) {
        if (preview || finished) { crash = 77760; bonus = 62976; start = 192240; }
        if (paused) return 0;
        Tone& t = preview ? demo : game;
        int value = 0;
        if (t.left) {
            value = t.phase < t.period / 2 ? 6000 : -6000;
            t.phase = (t.phase + 1) % t.period;
            --t.left;
        }
        if (id != 15 || preview || finished) return value;
        if (start < 192240) {
            if (start >= 153600 || start % 51200 < 25440)
                value = pulse(start_phase, start >= 153600 ? 484 : 164);
            ++start;
        } else {
            const unsigned hz = speed <= 120 ? 128 + 65 * (speed - 30) / 90 : 193 + 200 * (speed - 120) / 124;
            value = pulse(engine_phase, hz);
        }
        if (crash >= 77760 && bonus < 62976) {
            const unsigned note = bonus / 6336;
            value = pulse(bonus_phase, note >= 8 ? 874 : note % 3 == 0 ? 475 : note % 3 == 1 ? 627 : 874);
            ++bonus;
        }
        if (crash < 77760) {
            value = pulse(crash_phase, (crash / 12960) % 2 == 0 ? 320 : 627);
            ++crash;
        }
        return value;
    }
};

static bool frame(ctg::Audio& audio, Baseline& baseline, bool preview = false,
                  bool finished = false, bool paused = false, unsigned id = 0, unsigned speed = 30) {
    const auto actual = audio.render(preview, finished, paused, id, speed);
    for (unsigned i = 0; i < 800; ++i) {
        const int expected = baseline.sample(preview, finished, paused, id, speed);
        if (actual[2 * i] != expected || actual[2 * i + 1] != expected) {
            std::fprintf(stderr, "PCM mismatch at pair %u: %d/%d expected %d\n",
                         i, actual[2 * i], actual[2 * i + 1], expected);
            return false;
        }
    }
    return audio.racing_recovering() == (baseline.crash < 77760) &&
           audio.racing_starting() == (baseline.start < 192240) &&
           audio.racing_player_visible() == (baseline.crash >= 77760 || (baseline.crash / 12960) % 2 == 0);
}

int main() {
    static_assert(ctg::Audio::racing_crash_segment_samples == 12960);
    static_assert(ctg::Audio::racing_crash_total_samples == 77760);
    static_assert(ctg::Audio::racing_bonus_tone_samples == 6336);
    static_assert(ctg::Audio::racing_bonus_total_samples == 62976);
    static_assert(ctg::Audio::racing_start_interval == 51200);
    static_assert(ctg::Audio::racing_start_pulse == 25440);
    static_assert(ctg::Audio::racing_start_total == 192240);
    // Exhaust all event combinations, shooting selection, duration, priority,
    // owner-only events, stereo identity, pause/resume and silent tail.
    for (unsigned events = 0; events < 16; ++events) {
        for (bool shooting : {false, true}) {
            ctg::Audio audio;
            Baseline baseline;
            audio.trigger_game(events | ctg::event_player1_hit, shooting);
            baseline.trigger(events, shooting, false);
            for (unsigned f = 0; f < 12; ++f)
                CHECK(frame(audio, baseline, false, f == 3, f == 1 || f == 2));
        }
    }
    for (unsigned id : {0u, 13u, 14u, 15u}) {
        for (unsigned events = 0; events < 16; ++events) {
            ctg::Audio audio;
            Baseline baseline;
            audio.trigger_game(8, false); baseline.trigger(8, false, false);
            audio.trigger_demo(events, id); baseline.trigger(events, id == 14, true);
            for (unsigned f = 0; f < 12; ++f)
                CHECK(frame(audio, baseline, true, false, f == 2, id));
            // Preview never consumes the live tone.
            CHECK(frame(audio, baseline));
            audio.clear_demo(); baseline.demo = {};
            CHECK(frame(audio, baseline, true));
        }
    }
    {
        ctg::Audio audio;
        Baseline baseline;
        audio.trigger_game(8, false); baseline.trigger(8, false, false);
        CHECK(frame(audio, baseline));
        audio.trigger_game(ctg::event_player1_shot, true);
        CHECK(frame(audio, baseline)); // Non-audio event must not retrigger.
        audio.trigger_game(1, false); baseline.trigger(1, false, false);
        CHECK(frame(audio, baseline)); // Replaces and resets previous phase.
        audio.clear_game_tone(); baseline.game.left = 0;
        CHECK(frame(audio, baseline));
        audio.trigger_demo(8, 0); baseline.trigger(8, false, true);
        audio.clear_game_tone();
        CHECK(frame(audio, baseline, true)); // Clearing game leaves demo alone.
    }
    // Default crash_sample is zero, not the crash total. Exercise all six
    // crash segments, blinking boundaries and the partial final crash frame.
    {
        ctg::Audio audio;
        Baseline baseline;
        CHECK(audio.racing_recovering());
        CHECK(!audio.racing_starting());
        for (unsigned f = 0; f < 110; ++f)
            CHECK(frame(audio, baseline, false, false, f == 17, 15));
        CHECK(!audio.racing_recovering());
    }
    // Complete countdown including all gaps and the sample-exact transition
    // to engine within frame 240. Generic sound survives in countdown gaps.
    {
        ctg::Audio audio;
        Baseline baseline;
        audio.start_racing(); baseline.restart();
        CHECK(!audio.racing_recovering());
        CHECK(audio.racing_starting());
        for (unsigned f = 0; f < 260; ++f) {
            if (f % 7 == 0) {
                audio.trigger_game(8, false); baseline.trigger(8, false, false);
            }
            CHECK(frame(audio, baseline, false, false, f == 31 || f == 193, 15, 244));
        }
        CHECK(!audio.racing_starting());
        // Every valid speed, preserving oscillator phase across changes.
        for (unsigned speed = 30; speed <= 244; ++speed)
            CHECK(frame(audio, baseline, false, false, false, 15, speed));
    }
    // Bonus note boundaries, crash-over-bonus priority, suspended bonus clock,
    // crash retrigger, and underlying countdown/engine advancement.
    {
        ctg::Audio audio;
        Baseline baseline;
        audio.start_racing(); baseline.restart();
        audio.bonus(); baseline.bonus = baseline.bonus_phase = 0;
        for (unsigned f = 0; f < 500; ++f) {
            if (f == 9 || f == 40 || f == 249) {
                audio.collision(); baseline.crash = baseline.crash_phase = 0;
            }
            if (f == 130 || f == 250 || f == 380) {
                audio.bonus(); baseline.bonus = baseline.bonus_phase = 0;
            }
            CHECK(frame(audio, baseline, false, false, f == 19 || f == 300, 15, 120));
        }
    }
    // Preview/result cancellation happens even when paused. Restart resets only
    // its legacy subset, not generic/demo tones. Non-racing frames freeze racing.
    for (bool preview : {false, true}) {
        ctg::Audio audio;
        Baseline baseline;
        audio.start_racing(); baseline.restart();
        audio.collision(); baseline.crash = baseline.crash_phase = 0;
        audio.bonus(); baseline.bonus = baseline.bonus_phase = 0;
        CHECK(frame(audio, baseline, preview, !preview, true, 15));
        CHECK(!audio.racing_starting() && !audio.racing_recovering());
        CHECK(frame(audio, baseline, false, false, false, 15));
        audio.trigger_game(8, false); baseline.trigger(8, false, false);
        audio.trigger_demo(4, 14); baseline.trigger(4, true, true);
        audio.start_racing(); baseline.restart();
        CHECK(frame(audio, baseline, false, false, false, 0));
        CHECK(frame(audio, baseline, false, false, true, 15));
        CHECK(frame(audio, baseline, true, false, false, 14));
    }
    std::puts("audio tests passed");
    return 0;
}
