#include "../src/core.cpp"
#include <cstdio>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

int main() {
    CHECK(shooting_target == 2 && shooting_direction == 0 && shooting_size == 0 && shooting_speed == 0);
    retro_init();
    CHECK(shooting_target == 2 && shooting_direction == 0 && shooting_size == 0 && shooting_speed == 0);
    loaded = detail = true;
    selector = paused = false;
    content_id = 14;
    shooting.set_options(false, {});
    shooting.set_shot_limit(15);

    // Check both complete borders and the immediately adjacent background.
    {
        for (bool small_target : {false, true}) {
            ctg::Shooting field;
            field.set_options(false, {false, small_target});
            renderer.render_shooting(field, false);
            for (int y : {19, 20, 21, 22, 23, 24, 215, 216, 217, 218, 219, 220}) {
                for (int x = 36; x <= 283; ++x) {
                    const bool border = x >= 37 && x <= 282 &&
                        ((y >= 20 && y <= 23) || (y >= 216 && y <= 219));
                    CHECK(pixels[y * 320 + x] == (border ? 0x86ffffu : 0x46a79au));
                }
            }
        }
    }

    // The projectile blinks for 200 ms on and 125 ms off, quantized at 60 Hz.
    {
        ctg::Shooting field;
        field.set_target_behavior(11, 1, false);
        field.step(0, true);
        for (unsigned frame = 0; frame < 39; ++frame) {
            const bool visible = (frame * 2) % 39 < 24;
            CHECK(field.shot_visible() == visible);
            renderer.render_shooting(field, false);
            const auto& shot = field.state();
            CHECK(pixels[shot.shot_y * 320 + shot.shot_x] == (visible ? 0x8ddadeu : 0x46a79au));
            field.step(0, false);
        }
        CHECK(field.shot_visible());
    }

    // Paddles may cover the border but never draw outside the field or disappear.
    for (bool small_target : {false, true}) for (bool automatic : {false, true}) for (int direction : {-1, 1}) {
        ctg::Shooting field;
        field.set_options(false, {false, small_target});
        field.set_target_behavior(11, direction, false);
        for (int i = 0; i < 300; ++i) {
            field.step(direction, false, direction, automatic);
            renderer.render_shooting(field, false);
            for (int x : {ctg::rally_x(2), ctg::rally_x(38)}) {
                int visible = 0;
                for (int y = 0; y < 240; ++y) {
                    const bool object = pixels[y * 320 + x] == 0xf3d66bu;
                    if (y < 20 || y >= 220) CHECK(!object);
                    else visible += object;
                }
                CHECK(visible >= 4);
            }
        }
    }

    // Exercise every mix of random and fixed settings, including manual direction.
    bool saw_random_large = false, saw_random_small = false;
    bool saw_random_low = false, saw_random_high = false;
    for (unsigned target : {0u, 1u, 2u, 3u, 4u}) {
        for (unsigned direction : {0u, 1u, 2u, 3u}) {
            for (unsigned size : {0u, 1u, 2u}) for (unsigned speed : {0u, 1u, 2u}) {
                shooting_target = target; shooting_direction = direction; shooting_size = size; shooting_speed = speed;
                shooting_random.seed(1234);
                std::mt19937 expected = shooting_random;
                bool expected_small = false, expected_high = false;
                auto advance_expected = [&] {
                    if (target == 4) std::uniform_int_distribution<unsigned>(1, 3)(expected);
                    if (target != 0 && direction == 2) std::uniform_int_distribution<unsigned>(0, 1)(expected);
                    expected_small = size == 2 ? std::uniform_int_distribution<unsigned>(0, 1)(expected) != 0 : size == 1;
                    expected_high = speed == 2 ? std::uniform_int_distribution<unsigned>(0, 1)(expected) != 0 : speed == 1;
                    if (speed == 2) {
                        saw_random_high |= expected_high;
                        saw_random_low |= !expected_high;
                    }
                    if (size == 2) {
                        saw_random_small |= expected_small;
                        saw_random_large |= !expected_small;
                    }
                };
                auto check_size = [&] {
                    return shooting_resolved_small == expected_small && shooting_resolved_high_speed == expected_high &&
                        shooting.paddle_height(0) == (expected_high ? 32 : 16) &&
                        shooting.paddle_height(1) == (expected_small ? 16 : 33) &&
                        shooting_size == size && shooting_speed == speed;
                };
                start_shooting_match(); advance_expected();
                CHECK(shooting_random == expected && check_size());
                // Applying either generic paddle preference must not change Shooting,
                // consume randomness, or overwrite those preferences.
                for (bool rally_speed : {false, true}) for (bool p1 : {false, true}) for (bool p2 : {false, true}) {
                    high_speed = rally_speed;
                    small_paddle = {p1, p2};
                    apply_options();
                    CHECK(high_speed == rally_speed && small_paddle[0] == p1 && small_paddle[1] == p2);
                    CHECK(shooting_random == expected && check_size());
                    shooting.reset();
                    shooting.step(0, true);
                    const int shot_x = shooting.state().shot_x;
                    shooting.step(0, false);
                    CHECK(shooting.state().shot_x - shot_x == (expected_high ? 6 : 3));
                }
                shooting.reset();
                for (int i = 0; i < 20; ++i) retro_run();
                CHECK(shooting_random == expected && check_size()); // No per-frame rerolls.

                auto prepare_shot = [&](bool hit) {
                    shooting.reset();
                    shooting.step(0, true);
                    while (shooting.state().shot_x < 256) shooting.step(0, false);
                    auto& s = const_cast<ctg::ShootingState&>(shooting.state());
                    s.frame = 0;
                    s.shot_y = hit ? shooting.target_top() + shooting.paddle_height(1) / 2 : 24;
                };
                prepare_shot(false);
                retro_run();
                CHECK(shooting.state().hits == 0 && shooting_random == expected && check_size());
                for (int i = 0; i < 25; ++i) retro_run();
                CHECK(shooting_random == expected && check_size()); // A completed miss does not reroll.
                prepare_shot(true);
                retro_run(); advance_expected();
                CHECK(shooting.state().hits == 1 && shooting_random == expected && check_size());
                while (shooting.state().flying) {
                    retro_run();
                    CHECK(shooting_random == expected && check_size()); // No reroll during flight or on exit.
                }
                prepare_shot(true);
                retro_run(); advance_expected();
                CHECK(shooting.state().hits == 1 && shooting_random == expected && check_size());
                start_shooting_match(); advance_expected();
                CHECK(shooting_random == expected && check_size()); // Restart resolves again.
                CHECK(small_paddle[0] && small_paddle[1]);
            }
        }
    }
    CHECK(saw_random_large && saw_random_small && saw_random_low && saw_random_high);
    small_paddle = {};
    high_speed = false;
    shooting_size = shooting_speed = 0;
    // Fixed target speed and direction remain unchanged through a hit.
    shooting.set_options(false, {});
    shooting_target = 3; shooting_direction = 1;
    start_shooting_match();
    shooting.step(0, true);
    while (shooting.state().shot_x < 256) shooting.step(0, false);
    auto& s = const_cast<ctg::ShootingState&>(shooting.state());
    s.frame = 0; s.shot_y = shooting.target_top() + shooting.paddle_height(1) / 2;
    retro_run(); CHECK(s.hits == 1);
    const int before = s.target_y;
    for (int i = 0; i < 10; ++i) retro_run();
    CHECK(s.target_y == before - 40);

    // Preview runs its own simulation and must not consume gameplay randomness.
    shooting_target = 4; shooting_direction = 2; shooting_size = 2; shooting_speed = 2;
    shooting_random.seed(9876);
    const auto before_demo = shooting_random;
    demo.reset();
    step_demo(14); // Initial random settings.
    CHECK(shooting_random == before_demo);
    shooting_size = 0; step_demo(14);
    shooting_size = 2; step_demo(14); // Changing preferences resolves again.
    CHECK(shooting_random == before_demo);

    demo.shooting.reset();
    demo.shooting.step(0, true);
    while (demo.shooting.state().shot_x < 256) demo.shooting.step(0, false);
    auto& demo_state = const_cast<ctg::ShootingState&>(demo.shooting.state());
    demo_state.frame = 0;
    demo_state.shot_y = demo.shooting.target_top() + demo.shooting.paddle_height(1) / 2;
    const auto hits = demo_state.hits;
    step_demo(14); // A hit rerolls the demo settings.
    CHECK(demo_state.hits == hits + 1 && shooting_random == before_demo);
    demo_state.finished = true;
    step_demo(14); // A finished demo resolves settings and resets.
    CHECK(!demo_state.finished && shooting_random == before_demo);
    for (int i = 0; i < 2000; ++i) step_demo(14);
    CHECK(shooting_random == before_demo);
    return 0;
}
