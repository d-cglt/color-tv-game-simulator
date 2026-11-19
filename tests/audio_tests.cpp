#include "audio.h"
#include "tennis_b.h"
#include <cstdio>
#include <utility>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

// Independent scalar oracle: literal timings, no Audio constants or helpers.
// Each call advances a single mono sample; frame tests compare both channels.
struct Baseline {
    struct Tone {
        unsigned left = 0, phase = 0, period = 0, total = 0;
        bool shaped = false, point = false;
        double filtered = 0, previous_input = 0;
    } game, demo;
    unsigned crash = 0, crash_phase = 0, bonus = 62976, bonus_phase = 0;
    unsigned start = 192240, start_phase = 0, engine_phase = 0;

    void trigger(unsigned events, bool shooting, bool preview) {
        events &= 31;
        if (!events) return;
        Tone& t = preview ? demo : game;
        const bool point = events & 12;
        const bool shaped = !(events & 12) && (!shooting || (events & (2 | 16)));
        const unsigned total = point ? ((events & 8) ? 5760u : 12240u) : shaped ? 3120u : (events & 8) ? 7200u : 2400u;
        t = {total, 0,
             shaped ? ((events & 2) ? 100u : 200u) :
                 (events & 8) ? 80u : (events & 4) ? 120u : (shooting || (events & 2)) ? 60u : 100u,
             total, shaped, point};
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
            if (t.shaped || t.point) {
                const unsigned midpoint = t.point ? 24000 : t.period / 2;
                const double input = t.left > 960 ? (t.phase < midpoint ? -6000.0 : 6000.0) : t.previous_input;
                t.filtered = 0.9974 * (t.filtered + input - t.previous_input);
                t.previous_input = input;
                value = static_cast<int>(t.filtered * (t.left <= 960 ? t.left / 960.0 : 1.0));
            } else {
                value = t.phase < t.period / 2 ? 6000 : -6000;
            }
            if (t.point) {
                const unsigned elapsed = t.total - t.left;
                const unsigned frequency = (elapsed / 720) % 2 == 0 ? 480 : 240;
                t.phase = (t.phase + frequency) % 48000;
            } else {
                t.phase = (t.phase + 1) % t.period;
            }
            --t.left;
        }
        if (id != 15 || preview || finished) return value;
        if (start < 192240) {
            if (start >= 153600 || start % 51200 < 25440)
                value = pulse(start_phase, start >= 153600 ? 484 : 164);
            ++start;
        } else {
            const unsigned hz = speed <= 136 ? 128 + 65 * (speed - 30) / 106 :
                                speed <= 166 ? 193 + 50 * (speed - 136) / 30 : 243 + 150 * (speed - 166) / 67;
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
            for (unsigned f = 0; f < 18; ++f)
                CHECK(frame(audio, baseline, false, f == 3, f == 1 || f == 2));
        }
    }
    // Check oscillation edges, the exact tail boundary, silence, and reset on
    // retrigger directly against rendered samples (not just the oracle).
    for (unsigned events : {1u, 2u, 3u}) {
        ctg::Audio audio;
        audio.trigger_game(events, false);
        ctg::Audio::Samples frames[4];
        for (auto& samples : frames) samples = audio.render(false, false, false, 0, 30);
        const auto at = [&](unsigned i) { return frames[i / 800][2 * (i % 800)]; };
        const unsigned half_period = events & 2 ? 50 : 100;
        CHECK(at(0) == -5984);
        CHECK(at(half_period - 1) < 0);
        CHECK(at(half_period) > 0);
        CHECK(at(2 * half_period - 1) > 0);
        CHECK(at(2 * half_period) < 0);
        CHECK(at(2159) > 0);
        CHECK(at(2160) > 0);
        CHECK(at(2160) <= at(2159));
        CHECK(at(2160) >= at(2159) * 9974 / 10000 - 1);
        CHECK(at(2400) > 0 && at(2400) < at(2160));
        CHECK(at(3119) == 0); // Final scaled sample truncates toward zero.
        CHECK(at(3120) == 0);
        CHECK(at(3199) == 0);
        audio.trigger_game(events, false);
        const auto retriggered = audio.render(false, false, false, 0, 30);
        CHECK(retriggered == frames[0]);
        audio.trigger_game(events, false);
        CHECK(audio.render(false, false, false, 0, 30) == frames[0]);
    }
    // Point frequency changes retain phase across 720-sample boundaries.
    // Compare the entire 235 ms tone and 20 ms held-input fade, including
    // the partial last frame and subsequent silence.
    for (unsigned events : {4u, 5u, 6u, 7u}) {
        ctg::Audio audio;
        Baseline baseline;
        audio.trigger_game(events, false); baseline.trigger(events, false, false);
        ctg::Audio::Samples frames[16];
        for (unsigned f = 0; f < 16; ++f) {
            frames[f] = audio.render(false, false, false, 0, 30);
            for (unsigned i = 0; i < 800; ++i) {
                const int expected = baseline.sample(false, false, false, 0, 30);
                CHECK(frames[f][2 * i] == expected);
                CHECK(frames[f][2 * i + 1] == expected);
                if (f * 800 + i == 12238) CHECK(baseline.game.left == 1);
                if (f * 800 + i == 12239) CHECK(baseline.game.left == 0);
                if (f * 800 + i >= 12240) CHECK(expected == 0);
            }
        }
        const auto at = [&](unsigned i) { return frames[i / 800][2 * (i % 800)]; };
        CHECK(at(0) == -5984);
        CHECK(at(49) < 0 && at(50) > 0);
        CHECK(at(719) < 0 && at(720) < 0);
        CHECK(at(779) < 0 && at(780) > 0);
        CHECK(at(1439) > 0 && at(1440) > 0);
        CHECK(at(1459) > 0 && at(1460) < 0);
        CHECK(at(11279) != 0 && at(11280) != 0);
        CHECK(at(11280) * at(11279) > 0);
        audio.trigger_game(events, false);
        CHECK(audio.render(false, false, false, 0, 30) == frames[0]);
    }
    // Pausing freezes the point oscillator, segment clock and fade.
    for (unsigned pause_after : {1u, 14u, 15u}) {
        ctg::Audio paused, uninterrupted;
        paused.trigger_game(4, false); uninterrupted.trigger_game(4, false);
        for (unsigned f = 0; f < 16; ++f) {
            if (f == pause_after) {
                const auto silence = paused.render(false, false, true, 0, 30);
                for (auto sample : silence) CHECK(sample == 0);
            }
            CHECK(paused.render(false, false, false, 0, 30) ==
                  uninterrupted.render(false, false, false, 0, 30));
        }
    }
    // The match-ending tone shares the point tone for 100 ms, then fades.
    {
        ctg::Audio point, win;
        point.trigger_game(ctg::sound_point, false);
        win.trigger_game(ctg::sound_point | ctg::sound_win, false);
        for (unsigned f = 0; f < 6; ++f)
            CHECK(point.render(false, false, false, 0, 30) == win.render(false, false, false, 0, 30));
        const auto fade1 = win.render(false, true, false, 0, 30);
        const auto fade2 = win.render(false, true, false, 0, 30);
        CHECK(fade1[0] != 0);
        for (unsigned i = 160; i < 800; ++i) CHECK(fade2[2 * i] == 0 && fade2[2 * i + 1] == 0);
    }
    // End-of-game sound overrides the point/shot-exit sound in both modes.
    for (bool shooting : {false, true}) {
        ctg::Audio combined, expected;
        combined.trigger_game(15, shooting);
        expected.trigger_game(ctg::sound_win, shooting);
        for (unsigned f = 0; f < 18; ++f)
            CHECK(combined.render(false, false, false, 0, 30) ==
                  expected.render(false, false, false, 0, 30));
    }
    {
        ctg::Audio shot;
        Baseline baseline;
        shot.trigger_game(ctg::sound_shot, true);
        baseline.trigger(ctg::sound_shot, true, false);
        ctg::Audio wall;
        wall.trigger_game(ctg::sound_wall, false);
        for (unsigned f = 0; f < 5; ++f) {
            CHECK(frame(shot, baseline));
        }
        shot.trigger_game(ctg::sound_shot, true);
        for (unsigned f = 0; f < 5; ++f)
            CHECK(shot.render(false, false, false, 14, 30) == wall.render(false, false, false, 0, 30));
    }
    for (unsigned id : {0u, 13u, 14u, 15u}) {
        for (unsigned events = 0; events < 16; ++events) {
            ctg::Audio audio;
            Baseline baseline;
            audio.trigger_game(8, false); baseline.trigger(8, false, false);
            audio.trigger_demo(events, id); baseline.trigger(events, id == 14, true);
            for (unsigned f = 0; f < 18; ++f)
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
            CHECK(frame(audio, baseline, false, false, f == 31 || f == 193, 15, 233));
        }
        CHECK(!audio.racing_starting());
        // Every valid speed, preserving oscillator phase across changes.
        for (unsigned speed = 30; speed <= 233; ++speed)
            CHECK(frame(audio, baseline, false, false, false, 15, speed));
    }
    // Count oscillator periods over exactly one second at each steady speed.
    for (const auto test : {std::pair<unsigned, unsigned>{30, 128}, {136, 193}, {166, 243}, {233, 393}}) {
        ctg::Audio audio;
        audio.start_racing();
        for (unsigned f = 0; f < 241; ++f) audio.render(false, false, false, 15, test.first);
        unsigned rising = 0;
        int previous = 0;
        for (unsigned f = 0; f < 60; ++f) {
            const auto samples = audio.render(false, false, false, 15, test.first);
            for (unsigned i = 0; i < 800; ++i) {
                const int sample = samples[2 * i];
                if (previous < 0 && sample > 0) ++rising;
                previous = sample;
            }
        }
        CHECK(rising == test.second);
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
