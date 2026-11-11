#include "../src/core.cpp"
#include <cstdio>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

int main() {
    loaded = detail = true;
    selector = paused = false;
    content_id = 14;
    shooting.set_options(false, {});
    shooting.set_shot_limit(15);

    // Exercise every mix of random and fixed settings, including manual direction.
    for (unsigned target : {0u, 1u, 2u, 3u, 4u}) {
        for (unsigned direction : {0u, 1u, 2u}) {
            for (unsigned blinking : {0u, 1u, 2u}) {
                shooting_target = target; shooting_direction = direction; shooting_blinking = blinking;
                shooting_random.seed(1234);
                std::mt19937 expected = shooting_random;
                auto advance_expected = [&] {
                    if (target == 4) std::uniform_int_distribution<unsigned>(1, 3)(expected);
                    if (target != 0 && direction == 2) std::uniform_int_distribution<unsigned>(0, 1)(expected);
                    if (blinking == 2) std::uniform_int_distribution<unsigned>(0, 1)(expected);
                };
                start_shooting_match(); advance_expected();
                CHECK(shooting_random == expected);
                for (int i = 0; i < 20; ++i) retro_run();
                CHECK(shooting_random == expected); // No per-frame rerolls.

                auto prepare_shot = [&](bool hit) {
                    shooting.reset();
                    shooting.step(0, true);
                    while (shooting.state().shot_x < 256) shooting.step(0, false);
                    auto& s = const_cast<ctg::ShootingState&>(shooting.state());
                    s.frame = 0;
                    s.shot_y = hit ? s.target_y + 16 : 24;
                };
                prepare_shot(false);
                retro_run();
                CHECK(shooting.state().hits == 0 && shooting_random == expected);
                for (int i = 0; i < 25; ++i) retro_run();
                CHECK(shooting_random == expected); // A completed miss does not reroll.
                prepare_shot(true);
                retro_run(); advance_expected();
                CHECK(shooting.state().hits == 1 && shooting_random == expected);
                for (int i = 0; i < 10; ++i) retro_run();
                CHECK(shooting_random == expected); // One reroll per hit, not per frame.
                prepare_shot(true);
                retro_run(); advance_expected();
                CHECK(shooting.state().hits == 1 && shooting_random == expected);
            }
        }
    }
    // Fixed behavior remains fixed through a hit: fast, up, and never blinking.
    shooting_target = 3; shooting_direction = 1; shooting_blinking = 0;
    start_shooting_match();
    shooting.step(0, true);
    while (shooting.state().shot_x < 256) shooting.step(0, false);
    auto& s = const_cast<ctg::ShootingState&>(shooting.state());
    s.frame = 0; s.shot_y = s.target_y + 16;
    retro_run(); CHECK(s.hits == 1);
    const int before = s.target_y;
    for (int i = 0; i < 10; ++i) { retro_run(); CHECK(shooting.target_visible()); }
    CHECK(s.target_y == before - 40);
    for (int i = 0; i < 32; ++i) { retro_run(); CHECK(shooting.target_visible()); }

    // Preview runs its own simulation and must not consume gameplay randomness.
    shooting_target = 4; shooting_direction = shooting_blinking = 2;
    const auto before_demo = shooting_random;
    for (int i = 0; i < 2000; ++i) step_demo(14);
    CHECK(shooting_random == before_demo);
    return 0;
}
