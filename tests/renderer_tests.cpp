// Golden frames captured from build/core-before-refactor.cpp, except Racing frames
// updated for the 49-column road, five-pixel borders, score alignment, Racing grid,
// and Ping-Pong's gray background.
// Baseline: g++ -std=c++17 -O2 -I src -I third_party/libretro
// -DCORE_VERSION="renderer-baseline" build/renderer_baseline.cpp
// src/{preferences,content,generator,tennis_b,shooting,racing,opponent}.cpp
// -o build/renderer_baseline; build/renderer_baseline > build/renderer_goldens.inc
// Explicit fixtures below are shared verbatim with that temporary baseline runner.
// Hashes cover all 320x240 pixels (including the unused high byte).
#include "renderer.h"
#include "score_digits.h"
#include "tennis_b.h"
#include "shooting.h"
#include "racing.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

namespace fixtures {
enum Kind { serve, inplay, select, launch, pause, result_menu, hidden, crash };
enum Effect { plain, fixed, mono, grid, rf100, rf50, rf0, combined };
struct Case { unsigned id; Kind kind; unsigned choice; Effect effect; };
struct Fixture {
    ctg::TennisB tennis, demo_tennis;
    ctg::Shooting shooting, demo_shooting;
    ctg::Racing racing, demo_racing;
    std::array<unsigned, 14> modes{};
    explicit Fixture(const Case& c)
        : tennis(c.id == 1 || c.id == 3 || c.id == 5 || c.id == 7 || c.id == 9 || c.id == 11 || c.id == 13,
                 c.id >= 4 && c.id <= 7, c.id >= 8 && c.id <= 11,
                 c.id <= 1, c.id == 4 || c.id == 5, c.id == 8 || c.id == 9, c.id == 12 || c.id == 13),
          demo_tennis(tennis) {
        for (unsigned i = 0; i < modes.size(); ++i) modes[i] = i % 4;
        // These are non-const owned games; edit their public state views to isolate
        // drawing from simulation changes. No step(), clocks, or RNG are involved.
        auto& t = const_cast<ctg::TennisState&>(tennis.state());
        t.score = {12, 9};
        t.paddle_y = {63, 177};
        t.small_paddle = {true, false};
        t.high_speed = true;
        t.ball_x = 187; t.ball_y = 83;
        t.ball_vx = -4; t.ball_vy = 2;
        t.serve_wait = c.kind == serve ? 73 : 0;
        t.hockey_barrier_closed = c.kind == serve;
        t.winner = c.kind == result_menu ? static_cast<int>(c.choice % 2) : -1;
        auto& dt = const_cast<ctg::TennisState&>(demo_tennis.state());
        dt.ball_x = 113; dt.ball_y = 159;
        dt.paddle_y = {96, 144}; dt.serve_wait = 0;
        shooting.set_options(true, {true, false});
        shooting.set_target_behavior(11, -1, true);
        auto& s = const_cast<ctg::ShootingState&>(shooting.state());
        s.shooter_y = 51; s.target_y = 192;
        s.shots = 12; s.hits = 9;
        s.frame = c.kind == serve ? 28 : 19;
        s.flying = c.kind != serve; s.shot_x = 181; s.shot_y = 73;
        s.finished = c.kind == result_menu;
        auto& ds = const_cast<ctg::ShootingState&>(demo_shooting.state());
        ds.shooter_y = 170; ds.target_y = 45;
        ds.flying = true; ds.shot_x = 119; ds.shot_y = 171;
        ctg::RacingState layout;
        layout.player = {206, 194, ctg::racing_car_width, ctg::racing_car_height};
        layout.traffic[0].rect = {80, c.kind == serve ? -7 : 213, ctg::racing_car_width, ctg::racing_car_height};
        layout.road_scroll = c.kind == serve ? 0 : 251;
        racing.reset_from_layout(layout);
        auto& r = const_cast<ctg::RacingState&>(racing.state());
        r.score = 47; r.finished = c.kind == result_menu;
        r.traffic[0].collided = c.kind == crash;
        layout.player.x = 144; layout.traffic[0].rect = {232, 79, ctg::racing_car_width, ctg::racing_car_height};
        layout.road_scroll = 107;
        demo_racing.reset_from_layout(layout);
    }
};
inline std::vector<Case> cases() {
    std::vector<Case> out;
    for (unsigned id = 0; id < 16; ++id) {
        out.push_back({id, serve, 0, plain});
        out.push_back({id, inplay, 0, plain});
        out.push_back({id, inplay, 0, fixed});
        out.push_back({id, select, id, plain});
        out.push_back({id, hidden, 0, plain});
        for (unsigned i = 0; i < (id == 15 ? 2u : id == 14 ? 9u : 7u); ++i)
            out.push_back({id, launch, i, plain});
        for (unsigned i = 0; i < 3; ++i) out.push_back({id, pause, i, plain});
        for (unsigned i = 0; i < 2; ++i) out.push_back({id, result_menu, i, plain});
    }
    for (const auto c : std::array<Case, 8>{{
             {0, select, 0, mono}, {7, select, 15, mono},
             {8, pause, 2, mono}, {14, result_menu, 1, mono},
             {14, launch, 0, mono}, {14, launch, 4, mono},
             {14, launch, 8, mono}, {15, launch, 1, mono}}}) out.push_back(c);
    for (auto effect : {grid, rf100, rf50, rf0, combined})
        for (unsigned id : {0u, 6u, 10u, 12u, 14u, 15u})
            out.push_back({id, inplay, 0, effect});
    out.push_back({15, crash, 0, plain});
    return out;
}
template<class Frame> std::uint64_t hash(const Frame& frame) {
    // FNV-1a over explicit low-to-high bytes, independent of host endianness.
    std::uint64_t value = UINT64_C(14695981039346656037);
    for (auto pixel : frame)
        for (unsigned shift = 0; shift < 32; shift += 8) {
            value ^= (pixel >> shift) & 255;
            value *= UINT64_C(1099511628211);
        }
    return value;
}
}

namespace {
constexpr std::uint64_t expected[] = {
    UINT64_C(0xde91ce5585951b45), // id=0 kind=0 choice=0 effect=0
    UINT64_C(0x0468239e93d6d079), // id=0 kind=1 choice=0 effect=0
    UINT64_C(0xc9254751298f3da9), // id=0 kind=1 choice=0 effect=1
    UINT64_C(0xf941c620922fc5df), // id=0 kind=2 choice=0 effect=0
    UINT64_C(0x63dda589905eb18d), // id=0 kind=6 choice=0 effect=0
    UINT64_C(0x20ded4c36b9c309e), // id=0 kind=3 choice=0 effect=0
    UINT64_C(0x40edda6f82435227), // id=0 kind=3 choice=1 effect=0
    UINT64_C(0x08569de72ee74bf9), // id=0 kind=3 choice=2 effect=0
    UINT64_C(0x7e37d5e13c6c3cf6), // id=0 kind=3 choice=3 effect=0
    UINT64_C(0xae7209151370491f), // id=0 kind=3 choice=4 effect=0
    UINT64_C(0x8c4c3351527d710f), // id=0 kind=3 choice=5 effect=0
    UINT64_C(0x9a59e4c8567538b4), // id=0 kind=3 choice=6 effect=0
    UINT64_C(0xd013a682cc18cef4), // id=0 kind=4 choice=0 effect=0
    UINT64_C(0xa4cd1d0f3a9f5e7a), // id=0 kind=4 choice=1 effect=0
    UINT64_C(0x3dbfd47715713589), // id=0 kind=4 choice=2 effect=0
    UINT64_C(0xd3dd8b3d17443a2d), // id=0 kind=5 choice=0 effect=0
    UINT64_C(0x01783d498b687e5b), // id=0 kind=5 choice=1 effect=0
    UINT64_C(0x1780799079132a85), // id=1 kind=0 choice=0 effect=0
    UINT64_C(0x50e13d74cb2b4019), // id=1 kind=1 choice=0 effect=0
    UINT64_C(0x7c0fe92a0d2e8089), // id=1 kind=1 choice=0 effect=1
    UINT64_C(0x5edc86f2c3112afa), // id=1 kind=2 choice=1 effect=0
    UINT64_C(0x45e528a7125d1ea5), // id=1 kind=6 choice=0 effect=0
    UINT64_C(0xa3280ce315434012), // id=1 kind=3 choice=0 effect=0
    UINT64_C(0x91d47ef0b2c3736b), // id=1 kind=3 choice=1 effect=0
    UINT64_C(0x133b4b0ad71fec96), // id=1 kind=3 choice=2 effect=0
    UINT64_C(0x4b96f0f31d50ea92), // id=1 kind=3 choice=3 effect=0
    UINT64_C(0xded5e2c1e4bb108b), // id=1 kind=3 choice=4 effect=0
    UINT64_C(0x5a09697b72e2d33b), // id=1 kind=3 choice=5 effect=0
    UINT64_C(0x51799d14e27ca0e0), // id=1 kind=3 choice=6 effect=0
    UINT64_C(0xff7ff048b60d02c4), // id=1 kind=4 choice=0 effect=0
    UINT64_C(0x2aa3669057af8d0a), // id=1 kind=4 choice=1 effect=0
    UINT64_C(0x14c9fbba8e1bdfb9), // id=1 kind=4 choice=2 effect=0
    UINT64_C(0x733fa9f8dfa8f9a5), // id=1 kind=5 choice=0 effect=0
    UINT64_C(0x19fef566f708bac3), // id=1 kind=5 choice=1 effect=0
    UINT64_C(0xddadc2042ffa5955), // id=2 kind=0 choice=0 effect=0
    UINT64_C(0x4337b436ed8a93a9), // id=2 kind=1 choice=0 effect=0
    UINT64_C(0x23aa3a8cc5005a59), // id=2 kind=1 choice=0 effect=1
    UINT64_C(0xcc09ef7ead8b6f27), // id=2 kind=2 choice=2 effect=0
    UINT64_C(0xd7ff53ce364dd6e1), // id=2 kind=6 choice=0 effect=0
    UINT64_C(0xe39b84ee574b0bca), // id=2 kind=3 choice=0 effect=0
    UINT64_C(0xfae38b44f7841773), // id=2 kind=3 choice=1 effect=0
    UINT64_C(0xfe0274c45ccf6703), // id=2 kind=3 choice=2 effect=0
    UINT64_C(0x1349670604161332), // id=2 kind=3 choice=3 effect=0
    UINT64_C(0xad46fc46d0d25392), // id=2 kind=3 choice=4 effect=0
    UINT64_C(0xa5797a67e52d5faf), // id=2 kind=3 choice=5 effect=0
    UINT64_C(0xc38999242fcaf4d7), // id=2 kind=3 choice=6 effect=0
    UINT64_C(0xd2e550e0cf4c4156), // id=2 kind=4 choice=0 effect=0
    UINT64_C(0x96c42cbd01740057), // id=2 kind=4 choice=1 effect=0
    UINT64_C(0x038912b37774677b), // id=2 kind=4 choice=2 effect=0
    UINT64_C(0x0a1ac4479692da2b), // id=2 kind=5 choice=0 effect=0
    UINT64_C(0x28edeb554579fd1f), // id=2 kind=5 choice=1 effect=0
    UINT64_C(0x889e62ae2b061ec5), // id=3 kind=0 choice=0 effect=0
    UINT64_C(0xaacc1b1b29500c99), // id=3 kind=1 choice=0 effect=0
    UINT64_C(0x30a43c7c50d66049), // id=3 kind=1 choice=0 effect=1
    UINT64_C(0xe9454c6b89764e48), // id=3 kind=2 choice=3 effect=0
    UINT64_C(0x989793735674e979), // id=3 kind=6 choice=0 effect=0
    UINT64_C(0x79ea3418d2519a58), // id=3 kind=3 choice=0 effect=0
    UINT64_C(0x89a2444305c4b951), // id=3 kind=3 choice=1 effect=0
    UINT64_C(0x27f72436ceded580), // id=3 kind=3 choice=2 effect=0
    UINT64_C(0xf826f881cb33cf5c), // id=3 kind=3 choice=3 effect=0
    UINT64_C(0xf59fe13b20bed7fc), // id=3 kind=3 choice=4 effect=0
    UINT64_C(0x09570784bd012089), // id=3 kind=3 choice=5 effect=0
    UINT64_C(0x58fb14d8eaa93ea5), // id=3 kind=3 choice=6 effect=0
    UINT64_C(0xca59fc73f8e63fe6), // id=3 kind=4 choice=0 effect=0
    UINT64_C(0x98a3551537b4b8c7), // id=3 kind=4 choice=1 effect=0
    UINT64_C(0x76760c3f0629643b), // id=3 kind=4 choice=2 effect=0
    UINT64_C(0x7de3b80fbbf62df3), // id=3 kind=5 choice=0 effect=0
    UINT64_C(0x1c9cfba3c0290967), // id=3 kind=5 choice=1 effect=0
    UINT64_C(0xe293ea217e85ecc0), // id=4 kind=0 choice=0 effect=0
    UINT64_C(0x2b46561bd0a1ba0b), // id=4 kind=1 choice=0 effect=0
    UINT64_C(0x8429e921cbfcd22b), // id=4 kind=1 choice=0 effect=1
    UINT64_C(0xe024cc73e759c05c), // id=4 kind=2 choice=4 effect=0
    UINT64_C(0x89046f30ad0dfb8e), // id=4 kind=6 choice=0 effect=0
    UINT64_C(0x618f684f1365c6cb), // id=4 kind=3 choice=0 effect=0
    UINT64_C(0x5b3c46ff37879741), // id=4 kind=3 choice=1 effect=0
    UINT64_C(0x0f745d0649b2a0e1), // id=4 kind=3 choice=2 effect=0
    UINT64_C(0x77942724344c0389), // id=4 kind=3 choice=3 effect=0
    UINT64_C(0xf326a2f66ebef35d), // id=4 kind=3 choice=4 effect=0
    UINT64_C(0xca216ad68fa26c56), // id=4 kind=3 choice=5 effect=0
    UINT64_C(0x25c9ce572fb671eb), // id=4 kind=3 choice=6 effect=0
    UINT64_C(0x81e0cd6ce9447446), // id=4 kind=4 choice=0 effect=0
    UINT64_C(0xe7c7aa25ab8f9510), // id=4 kind=4 choice=1 effect=0
    UINT64_C(0x3b6fffd3a67fc5b0), // id=4 kind=4 choice=2 effect=0
    UINT64_C(0x929fe345d50ea48c), // id=4 kind=5 choice=0 effect=0
    UINT64_C(0x1a5bea51276970ac), // id=4 kind=5 choice=1 effect=0
    UINT64_C(0x6e41fb6bde4ea6c0), // id=5 kind=0 choice=0 effect=0
    UINT64_C(0x2829283bee37a54b), // id=5 kind=1 choice=0 effect=0
    UINT64_C(0xaf9eaddaac0ff26b), // id=5 kind=1 choice=0 effect=1
    UINT64_C(0x95f26e3dec6e47ff), // id=5 kind=2 choice=5 effect=0
    UINT64_C(0xac06c3ab920fbc0e), // id=5 kind=6 choice=0 effect=0
    UINT64_C(0xaa908a8d075c087e), // id=5 kind=3 choice=0 effect=0
    UINT64_C(0x8e1daca2449dea78), // id=5 kind=3 choice=1 effect=0
    UINT64_C(0xe026c71e607efc5e), // id=5 kind=3 choice=2 effect=0
    UINT64_C(0xd88a2be4a2258af0), // id=5 kind=3 choice=3 effect=0
    UINT64_C(0x05b5da947b5b36e4), // id=5 kind=3 choice=4 effect=0
    UINT64_C(0xcc7a8b688b36ec9f), // id=5 kind=3 choice=5 effect=0
    UINT64_C(0xa53a3516c8731c4e), // id=5 kind=3 choice=6 effect=0
    UINT64_C(0xc796ea32657d12b6), // id=5 kind=4 choice=0 effect=0
    UINT64_C(0x51dbe6625ceae6f0), // id=5 kind=4 choice=1 effect=0
    UINT64_C(0x109179d2331c8f70), // id=5 kind=4 choice=2 effect=0
    UINT64_C(0xe77bbde40f576b3d), // id=5 kind=5 choice=0 effect=0
    UINT64_C(0x8fbe3bbfe49a4e3d), // id=5 kind=5 choice=1 effect=0
    UINT64_C(0x0debe5d9760d36c0), // id=6 kind=0 choice=0 effect=0
    UINT64_C(0x088e4fec8bf4a88b), // id=6 kind=1 choice=0 effect=0
    UINT64_C(0x14f65b1cd6ff74eb), // id=6 kind=1 choice=0 effect=1
    UINT64_C(0x3d670a9a3b30ff87), // id=6 kind=2 choice=6 effect=0
    UINT64_C(0x3526c9645915d9ee), // id=6 kind=6 choice=0 effect=0
    UINT64_C(0x656822d97d8c30af), // id=6 kind=3 choice=0 effect=0
    UINT64_C(0x14fa7cff9840d7cd), // id=6 kind=3 choice=1 effect=0
    UINT64_C(0xfe79a86d9cd6271d), // id=6 kind=3 choice=2 effect=0
    UINT64_C(0xc7cf8057d27825a5), // id=6 kind=3 choice=3 effect=0
    UINT64_C(0x496a1b45a1b00119), // id=6 kind=3 choice=4 effect=0
    UINT64_C(0x4b70c34eb8029fcd), // id=6 kind=3 choice=5 effect=0
    UINT64_C(0xc79d630ede91a24f), // id=6 kind=3 choice=6 effect=0
    UINT64_C(0x2389e28d6942e99a), // id=6 kind=4 choice=0 effect=0
    UINT64_C(0xe8271f9cc59fdd24), // id=6 kind=4 choice=1 effect=0
    UINT64_C(0x8475d4e5051cf534), // id=6 kind=4 choice=2 effect=0
    UINT64_C(0x1b6383dc79c75f7c), // id=6 kind=5 choice=0 effect=0
    UINT64_C(0x7aefc08f2404f6dc), // id=6 kind=5 choice=1 effect=0
    UINT64_C(0x07677fae65fc1280), // id=7 kind=0 choice=0 effect=0
    UINT64_C(0x2e6e885e3834ac4b), // id=7 kind=1 choice=0 effect=0
    UINT64_C(0xef26c79bfe2471eb), // id=7 kind=1 choice=0 effect=1
    UINT64_C(0xd5a71bac97795529), // id=7 kind=2 choice=7 effect=0
    UINT64_C(0x4066da80247b75ce), // id=7 kind=6 choice=0 effect=0
    UINT64_C(0x0a270f0d8d01cd0a), // id=7 kind=3 choice=0 effect=0
    UINT64_C(0x4926705b633581d4), // id=7 kind=3 choice=1 effect=0
    UINT64_C(0x07bc43079c4a07ea), // id=7 kind=3 choice=2 effect=0
    UINT64_C(0x8d57ebd57f428e5c), // id=7 kind=3 choice=3 effect=0
    UINT64_C(0x17f36d65b293d3e0), // id=7 kind=3 choice=4 effect=0
    UINT64_C(0xd7d20f039219d3dc), // id=7 kind=3 choice=5 effect=0
    UINT64_C(0xa0d14d42bb2a494a), // id=7 kind=3 choice=6 effect=0
    UINT64_C(0xaa8ebf06955e062a), // id=7 kind=4 choice=0 effect=0
    UINT64_C(0x11540be7f305b224), // id=7 kind=4 choice=1 effect=0
    UINT64_C(0xb8d4b716ae20e714), // id=7 kind=4 choice=2 effect=0
    UINT64_C(0x86e27af44a0d384d), // id=7 kind=5 choice=0 effect=0
    UINT64_C(0xec92bafad7f4d7cd), // id=7 kind=5 choice=1 effect=0
    UINT64_C(0xe1407a863372c255), // id=8 kind=0 choice=0 effect=0
    UINT64_C(0x3f678511d73b159e), // id=8 kind=1 choice=0 effect=0
    UINT64_C(0x7d90413fb7524a8e), // id=8 kind=1 choice=0 effect=1
    UINT64_C(0xd3a62ea7bd1c2d7e), // id=8 kind=2 choice=8 effect=0
    UINT64_C(0x1f21b4296b56ce0a), // id=8 kind=6 choice=0 effect=0
    UINT64_C(0x84158ed0abad5d01), // id=8 kind=3 choice=0 effect=0
    UINT64_C(0x2ae2e2b784e502c9), // id=8 kind=3 choice=1 effect=0
    UINT64_C(0x4148e08aba7b343e), // id=8 kind=3 choice=2 effect=0
    UINT64_C(0x9072026ea09f4c09), // id=8 kind=3 choice=3 effect=0
    UINT64_C(0x0407b3a7e7b9c921), // id=8 kind=3 choice=4 effect=0
    UINT64_C(0xa13a641be6ee609d), // id=8 kind=3 choice=5 effect=0
    UINT64_C(0xd4f22ce05c30f5a6), // id=8 kind=3 choice=6 effect=0
    UINT64_C(0x3323f5da82fc7ba1), // id=8 kind=4 choice=0 effect=0
    UINT64_C(0x0a5f2d26c0d025b2), // id=8 kind=4 choice=1 effect=0
    UINT64_C(0xa046fefe2d2b8ff9), // id=8 kind=4 choice=2 effect=0
    UINT64_C(0x04acf054901b168d), // id=8 kind=5 choice=0 effect=0
    UINT64_C(0x512f039f1f43aaaa), // id=8 kind=5 choice=1 effect=0
    UINT64_C(0x0817ad55bb3ae585), // id=9 kind=0 choice=0 effect=0
    UINT64_C(0xbbbeadda1c8cfa5e), // id=9 kind=1 choice=0 effect=0
    UINT64_C(0x376d3456889b302e), // id=9 kind=1 choice=0 effect=1
    UINT64_C(0x4a27007da3462b16), // id=9 kind=2 choice=9 effect=0
    UINT64_C(0xfaeba40cd7d9071e), // id=9 kind=6 choice=0 effect=0
    UINT64_C(0x6cbd38d027f6aa41), // id=9 kind=3 choice=0 effect=0
    UINT64_C(0xf1feba7243219ee1), // id=9 kind=3 choice=1 effect=0
    UINT64_C(0x832c2558cb88e761), // id=9 kind=3 choice=2 effect=0
    UINT64_C(0xbc0d76574e537e79), // id=9 kind=3 choice=3 effect=0
    UINT64_C(0xf6a82fa087bfd0f9), // id=9 kind=3 choice=4 effect=0
    UINT64_C(0x1e44a589016dd52d), // id=9 kind=3 choice=5 effect=0
    UINT64_C(0x646ad50f474b8616), // id=9 kind=3 choice=6 effect=0
    UINT64_C(0xe7bad0bd206688d1), // id=9 kind=4 choice=0 effect=0
    UINT64_C(0xbf89b76eb10ecf62), // id=9 kind=4 choice=1 effect=0
    UINT64_C(0x60d7bfb8eb533279), // id=9 kind=4 choice=2 effect=0
    UINT64_C(0xee1e1edb8c1bace5), // id=9 kind=5 choice=0 effect=0
    UINT64_C(0xa85ac805915f8402), // id=9 kind=5 choice=1 effect=0
    UINT64_C(0x56d777f72dffb9c5), // id=10 kind=0 choice=0 effect=0
    UINT64_C(0xb94ce9ca2c79ca4e), // id=10 kind=1 choice=0 effect=0
    UINT64_C(0x1892f868d73d709e), // id=10 kind=1 choice=0 effect=1
    UINT64_C(0x0faf8b58a54abe7d), // id=10 kind=2 choice=10 effect=0
    UINT64_C(0x722a7b0299c1cfb6), // id=10 kind=6 choice=0 effect=0
    UINT64_C(0x64ca62b2f6652e36), // id=10 kind=3 choice=0 effect=0
    UINT64_C(0xf2ef875b3483494e), // id=10 kind=3 choice=1 effect=0
    UINT64_C(0x55a9e9126242a70e), // id=10 kind=3 choice=2 effect=0
    UINT64_C(0x1a785e0ba9dbd00a), // id=10 kind=3 choice=3 effect=0
    UINT64_C(0xf9213d17a677c18e), // id=10 kind=3 choice=4 effect=0
    UINT64_C(0xaf62fc699fc4320e), // id=10 kind=3 choice=5 effect=0
    UINT64_C(0xd45dd155fbc28812), // id=10 kind=3 choice=6 effect=0
    UINT64_C(0x54967c6c0c224eba), // id=10 kind=4 choice=0 effect=0
    UINT64_C(0xdf31b6248087e25e), // id=10 kind=4 choice=1 effect=0
    UINT64_C(0x7460b0a0af273a06), // id=10 kind=4 choice=2 effect=0
    UINT64_C(0xf6a2e18defdeabd6), // id=10 kind=5 choice=0 effect=0
    UINT64_C(0x04acd64c9a17ebc6), // id=10 kind=5 choice=1 effect=0
    UINT64_C(0x72196e39f7f7dce5), // id=11 kind=0 choice=0 effect=0
    UINT64_C(0xad9db0c4a55bf7fe), // id=11 kind=1 choice=0 effect=0
    UINT64_C(0x712f3c316975b26e), // id=11 kind=1 choice=0 effect=1
    UINT64_C(0xc5351ffb0d5d05da), // id=11 kind=2 choice=11 effect=0
    UINT64_C(0x39f4446141190312), // id=11 kind=6 choice=0 effect=0
    UINT64_C(0xd530c706f3936175), // id=11 kind=3 choice=0 effect=0
    UINT64_C(0x723f104ef8d50305), // id=11 kind=3 choice=1 effect=0
    UINT64_C(0x3122f6f43bc21731), // id=11 kind=3 choice=2 effect=0
    UINT64_C(0xcf1fb592337e6c2d), // id=11 kind=3 choice=3 effect=0
    UINT64_C(0xb56ff4c349888039), // id=11 kind=3 choice=4 effect=0
    UINT64_C(0x05c0bf02ce9ae219), // id=11 kind=3 choice=5 effect=0
    UINT64_C(0x31abcca721c37459), // id=11 kind=3 choice=6 effect=0
    UINT64_C(0x78380c2bd6491aba), // id=11 kind=4 choice=0 effect=0
    UINT64_C(0xb48e002049b9b15e), // id=11 kind=4 choice=1 effect=0
    UINT64_C(0x4294962b399e91b6), // id=11 kind=4 choice=2 effect=0
    UINT64_C(0x23b04fd09a42c8ee), // id=11 kind=5 choice=0 effect=0
    UINT64_C(0x6d5e61328a16943e), // id=11 kind=5 choice=1 effect=0
    UINT64_C(0x3adefa7197222205), // id=12 kind=0 choice=0 effect=0
    UINT64_C(0xf5f3c96d1d3cd7bc), // id=12 kind=1 choice=0 effect=0
    UINT64_C(0x92b3f1b39487442c), // id=12 kind=1 choice=0 effect=1
    UINT64_C(0x500a5ac2a3f6b6be), // id=12 kind=2 choice=12 effect=0
    UINT64_C(0xb7c590bc3c54c5b8), // id=12 kind=6 choice=0 effect=0
    UINT64_C(0x5b5e145396051142), // id=12 kind=3 choice=0 effect=0
    UINT64_C(0xf6bd36dd47b06bf2), // id=12 kind=3 choice=1 effect=0
    UINT64_C(0xa44c70eccceb8ba6), // id=12 kind=3 choice=2 effect=0
    UINT64_C(0xcef124ca5d468a9e), // id=12 kind=3 choice=3 effect=0
    UINT64_C(0xe54c11f0a1ee895e), // id=12 kind=3 choice=4 effect=0
    UINT64_C(0x947626f47e54f5ad), // id=12 kind=3 choice=5 effect=0
    UINT64_C(0xd31e8d50dd088b06), // id=12 kind=3 choice=6 effect=0
    UINT64_C(0x0eddc55a2869effd), // id=12 kind=4 choice=0 effect=0
    UINT64_C(0x82f64818b6a0e4fd), // id=12 kind=4 choice=1 effect=0
    UINT64_C(0x710827e23cbea351), // id=12 kind=4 choice=2 effect=0
    UINT64_C(0x82a50f93db38cdc5), // id=12 kind=5 choice=0 effect=0
    UINT64_C(0x946385c13ac63a79), // id=12 kind=5 choice=1 effect=0
    UINT64_C(0x78ac1018ad338d25), // id=13 kind=0 choice=0 effect=0
    UINT64_C(0xf6b935e52d339fbc), // id=13 kind=1 choice=0 effect=0
    UINT64_C(0x70b3cf25714e1eec), // id=13 kind=1 choice=0 effect=1
    UINT64_C(0xf0dd9bdf4ca14aa7), // id=13 kind=2 choice=13 effect=0
    UINT64_C(0xd8830f562da9af14), // id=13 kind=6 choice=0 effect=0
    UINT64_C(0x0288671a49bb1bb9), // id=13 kind=3 choice=0 effect=0
    UINT64_C(0xd0135e3239f3b835), // id=13 kind=3 choice=1 effect=0
    UINT64_C(0x05ffe01ba874eabd), // id=13 kind=3 choice=2 effect=0
    UINT64_C(0x9c009d72f28fbd31), // id=13 kind=3 choice=3 effect=0
    UINT64_C(0xb11496dad895dbf5), // id=13 kind=3 choice=4 effect=0
    UINT64_C(0x615d40cdd9ef7746), // id=13 kind=3 choice=5 effect=0
    UINT64_C(0x9b94bcfd086f0375), // id=13 kind=3 choice=6 effect=0
    UINT64_C(0x206c12b2a7d30f9d), // id=13 kind=4 choice=0 effect=0
    UINT64_C(0x50adbe4f92a5525d), // id=13 kind=4 choice=1 effect=0
    UINT64_C(0xfb61ee2bc82076a9), // id=13 kind=4 choice=2 effect=0
    UINT64_C(0xef45b51d5d9cc5ed), // id=13 kind=5 choice=0 effect=0
    UINT64_C(0xdab3e5113ad11fa1), // id=13 kind=5 choice=1 effect=0
    UINT64_C(0xf52c9bbf95bd28e5), // id=14 kind=0 choice=0 effect=0
    UINT64_C(0x577ae7e1f856f7e1), // id=14 kind=1 choice=0 effect=0
    UINT64_C(0xf07708fb422eb571), // id=14 kind=1 choice=0 effect=1
    UINT64_C(0xd8bd37c640f9e893), // id=14 kind=2 choice=14 effect=0
    UINT64_C(0xa1b31116b3cf593d), // id=14 kind=6 choice=0 effect=0
    UINT64_C(0x6d36be7fc1fa3ffc), // id=14 kind=3 choice=0 effect=0
    UINT64_C(0xc80fa7b8b6da4a49), // id=14 kind=3 choice=1 effect=0
    UINT64_C(0x856de9bdc8dc5390), // id=14 kind=3 choice=2 effect=0
    UINT64_C(0xe2897fc4383fd15e), // id=14 kind=3 choice=3 effect=0
    UINT64_C(0x604047f50d503d93), // id=14 kind=3 choice=4 effect=0
    UINT64_C(0x4a17223eb7ca2ebe), // id=14 kind=3 choice=5 effect=0
    UINT64_C(0x624cc9274cdcb83a), // id=14 kind=3 choice=6 effect=0
    UINT64_C(0x91d261c9e57fb9ba), // id=14 kind=3 choice=7 effect=0
    UINT64_C(0xcef96a30103cb885), // id=14 kind=3 choice=8 effect=0
    UINT64_C(0x8061322b569d086e), // id=14 kind=4 choice=0 effect=0
    UINT64_C(0xd793337e27afd7bb), // id=14 kind=4 choice=1 effect=0
    UINT64_C(0x20ecd0f502a6e5db), // id=14 kind=4 choice=2 effect=0
    UINT64_C(0xcada23369030af57), // id=14 kind=5 choice=0 effect=0
    UINT64_C(0xc424c74e8e544507), // id=14 kind=5 choice=1 effect=0
    UINT64_C(0xc1f3757c19b62c1d), // id=15 kind=0 choice=0 effect=0
    UINT64_C(0x727fdfd8ab266a95), // id=15 kind=1 choice=0 effect=0
    UINT64_C(0xde751f7a35db13b5), // id=15 kind=1 choice=0 effect=1
    UINT64_C(0x3a7c140c6a118ece), // id=15 kind=2 choice=15 effect=0
    UINT64_C(0x106fe5a6a6e31eef), // id=15 kind=6 choice=0 effect=0
    UINT64_C(0x8dcec1402b135605), // id=15 kind=3 choice=0 effect=0
    UINT64_C(0xdb0b62677151f4c7), // id=15 kind=3 choice=1 effect=0
    UINT64_C(0xf6c82202721762fc), // id=15 kind=4 choice=0 effect=0
    UINT64_C(0x87d5a411283dd6f6), // id=15 kind=4 choice=1 effect=0
    UINT64_C(0xeb7f31edf90cf526), // id=15 kind=4 choice=2 effect=0
    UINT64_C(0x2f786d329c7daf12), // id=15 kind=5 choice=0 effect=0
    UINT64_C(0xf84ea2f85611b096), // id=15 kind=5 choice=1 effect=0
    UINT64_C(0x1c2528de9edb0a81), // id=0 kind=2 choice=0 effect=2
    UINT64_C(0x8cf87db58543b36d), // id=7 kind=2 choice=15 effect=2
    UINT64_C(0x209cdf643581e1ff), // id=8 kind=4 choice=2 effect=2
    UINT64_C(0xf8c6221bff0a4c78), // id=14 kind=5 choice=1 effect=2
    UINT64_C(0x3b428475c55920bb), // id=14 kind=3 choice=0 effect=2
    UINT64_C(0xe781040baeeb57d0), // id=14 kind=3 choice=4 effect=2
    UINT64_C(0xbc1d691dd8fb09d5), // id=14 kind=3 choice=8 effect=2
    UINT64_C(0x6b592d2e3b714341), // id=15 kind=3 choice=1 effect=2
    UINT64_C(0xb57ff59664dde289), // id=0 kind=1 choice=0 effect=3
    UINT64_C(0xba8246fc5648a6de), // id=6 kind=1 choice=0 effect=3
    UINT64_C(0x0b8fde84ced97dd6), // id=10 kind=1 choice=0 effect=3
    UINT64_C(0xda9983a375f59cc9), // id=12 kind=1 choice=0 effect=3
    UINT64_C(0x578d1d54e7181abb), // id=14 kind=1 choice=0 effect=3
    UINT64_C(0xa4edb8eb73a065a6), // id=15 kind=1 choice=0 effect=3
    UINT64_C(0x65a5d4b57100d86f), // id=0 kind=1 choice=0 effect=4
    UINT64_C(0xa850f99965988449), // id=6 kind=1 choice=0 effect=4
    UINT64_C(0xbea2dd0f1fd72421), // id=10 kind=1 choice=0 effect=4
    UINT64_C(0xc758a770648e7659), // id=12 kind=1 choice=0 effect=4
    UINT64_C(0x253e8a917aea4b1b), // id=14 kind=1 choice=0 effect=4
    UINT64_C(0xcf2a96dd6894dc6c), // id=15 kind=1 choice=0 effect=4
    UINT64_C(0x2c1416a774c02d63), // id=0 kind=1 choice=0 effect=5
    UINT64_C(0x5db8294aac3579e9), // id=6 kind=1 choice=0 effect=5
    UINT64_C(0xb5e8687612b0a299), // id=10 kind=1 choice=0 effect=5
    UINT64_C(0x3585c82bb59ce754), // id=12 kind=1 choice=0 effect=5
    UINT64_C(0x0835046b243a2086), // id=14 kind=1 choice=0 effect=5
    UINT64_C(0x9837a8447dd4105e), // id=15 kind=1 choice=0 effect=5
    UINT64_C(0x62b0de5cfd965bc8), // id=0 kind=1 choice=0 effect=6
    UINT64_C(0xa3960a954132db90), // id=6 kind=1 choice=0 effect=6
    UINT64_C(0x87f2321f02c04487), // id=10 kind=1 choice=0 effect=6
    UINT64_C(0x70f02760422bdaad), // id=12 kind=1 choice=0 effect=6
    UINT64_C(0xccadb622ea6d1449), // id=14 kind=1 choice=0 effect=6
    UINT64_C(0x99dccab5454ba445), // id=15 kind=1 choice=0 effect=6
    UINT64_C(0x07ab351da28c2619), // id=0 kind=1 choice=0 effect=7
    UINT64_C(0x2f4df1355f1e3226), // id=6 kind=1 choice=0 effect=7
    UINT64_C(0x18619a684cc523e1), // id=10 kind=1 choice=0 effect=7
    UINT64_C(0x5c4c0a77f0ffa64f), // id=12 kind=1 choice=0 effect=7
    UINT64_C(0xdb182049acf28490), // id=14 kind=1 choice=0 effect=7
    UINT64_C(0x7aff4992ccf804b9), // id=15 kind=1 choice=0 effect=7
    UINT64_C(0x5110d709eb83ed65), // id=15 kind=7 choice=0 effect=0

};

std::uint64_t draw(const fixtures::Case& c, bool diagnostics = false, bool rally_score_visible = true) {
    fixtures::Fixture f(c);
    const bool detail = c.kind != fixtures::select && c.kind != fixtures::launch && c.kind != fixtures::hidden;
    const ctg::RenderState state{
        c.id, c.kind == fixtures::select, detail, c.kind == fixtures::pause,
        c.kind == fixtures::result_menu, c.kind == fixtures::hidden,
        c.choice, c.choice, c.choice, c.choice,
        true, {true, false}, 21, 30, c.choice % 5, c.choice % 3, (c.choice + 1) % 3,
        f.modes, f.tennis, f.demo_tennis, f.shooting, f.demo_shooting,
        f.racing, f.demo_racing, c.kind != fixtures::crash,
        {c.effect == fixtures::fixed, 87, 65}, rally_score_visible
    };
    auto renderer_storage = std::make_unique<ctg::Renderer>();
    auto& renderer = *renderer_storage;
    if (c.kind == fixtures::serve || c.kind == fixtures::inplay || c.kind == fixtures::crash) {
        if (c.id == 15) renderer.render_racing(f.racing, true, state.racing_player_visible, state.scores);
        else if (c.id == 14) renderer.render_shooting(f.shooting, true, state.scores);
        else renderer.render_tennis(f.tennis, true, state.scores);
    } else renderer.render(state);
    if (c.effect == fixtures::rf100 || c.effect == fixtures::rf50 || c.effect == fixtures::rf0 || c.effect == fixtures::combined)
        renderer.apply_rf_filter(c.effect == fixtures::rf100 ? 100 : c.effect == fixtures::rf0 ? 0 : 50);
    if (c.effect == fixtures::mono || c.effect == fixtures::combined) {
        renderer.grayscale();
        if (c.effect == fixtures::mono) renderer.monochrome_selection_outline(state);
    }
    if (c.effect == fixtures::grid || c.effect == fixtures::combined) renderer.test_grid_overlay(c.id == 15);
    const auto frame_hash = fixtures::hash(renderer.pixels);
    if (diagnostics) {
        std::cerr << "pause diagnostic id=" << c.id << " kind=" << static_cast<unsigned>(c.kind)
                  << " choice=" << c.choice << " effect=" << static_cast<unsigned>(c.effect)
                  << " state={content_id=" << state.content_id << " selector=" << state.selector
                  << " detail=" << state.detail << " paused=" << state.paused
                  << " finished=" << state.finished << " menu_hidden=" << state.menu_hidden
                  << " selector_choice=" << state.selector_choice << " menu_choice=" << state.menu_choice
                  << " pause_choice=" << state.pause_choice << " result_choice=" << state.result_choice
                  << "} frame_fnv1a=" << std::hex << std::setfill('0') << std::setw(16) << frame_hash
                  << std::dec << '\n';
        const char* labels[] = {"RESUME", "RESTART", "EXIT"};
        const unsigned label_x[] = {142, 139, 148};
        for (unsigned row = 0; row < 3; ++row) {
            const unsigned y = 99 + row * 16;
            std::uint64_t hash = UINT64_C(14695981039346656037);
            for (unsigned py = y; py < y + 14; ++py)
                for (unsigned px = 75; px < 245; ++px) {
                    const auto pixel = renderer.pixels[py * ctg::Graphics::width + px];
                    for (unsigned shift = 0; shift < 32; shift += 8) {
                        hash ^= (pixel >> shift) & 255;
                        hash *= UINT64_C(1099511628211);
                    }
                }
            const unsigned text_y = y + 3;
            std::cerr << "  row=" << row << " label=" << labels[row] << " selected="
                      << (row == state.pause_choice) << " region=[75," << y << ",170,14] fnv1a="
                      << std::hex << std::setfill('0') << std::setw(16) << hash
                      << " background(75," << std::dec << y << ")=" << std::hex << std::setw(8)
                      << renderer.pixels[y * ctg::Graphics::width + 75]
                      << " label_pixel(" << std::dec << label_x[row] << ',' << text_y << ")="
                      << std::hex << std::setw(8)
                      << renderer.pixels[text_y * ctg::Graphics::width + label_x[row]]
                      << std::dec << '\n';
        }
    }
    return frame_hash;
}
}

int main() {
    // Check every sprite pixel, including the two-pixel wheel bridges, for both colors.
    {
        ctg::Racing game;
        auto& state = const_cast<ctg::RacingState&>(game.state());
        state.player = {100, 100, ctg::racing_car_width, ctg::racing_car_height};
        state.traffic[0].rect = {200, 100, ctg::racing_car_width, ctg::racing_car_height};
        auto renderer = std::make_unique<ctg::Renderer>();
        renderer->render_racing(game, false);
        const char* rows[] = {"--++--", "+-++-+", "+-++-+", "--++--", "+-++-+", "+-++-+"};
        for (int y = 0; y < ctg::racing_car_height; ++y) for (int x = 0; x < ctg::racing_car_width; ++x) {
            const int col = x / 4, row = y / 5;
            const bool bridge = (col == 1 || col == 4) && (y == 9 || y == 10 || y == 24 || y == 25);
            const bool lit = rows[row][col] == '+' || bridge;
            for (int car = 0; car < 2; ++car) {
                const auto expected_pixel = lit ? (car == 0 ? 0xeb65e7u : 0xf5ed48u) : 0x39b800u;
                if (renderer->pixels[(100 + y) * 320 + (car == 0 ? 100 : 200) + x] != expected_pixel) {
                    std::fprintf(stderr, "Racing sprite mismatch car=%d x=%d y=%d\n", car, x, y);
                    return 1;
                }
            }
        }
    }
    // Opponents are clipped at both edges of the gameplay window, even when partially visible.
    for (int top : {-30, -7, 8, 9, 194, 213, 223, 224, 240}) {
        ctg::Racing game;
        auto& state = const_cast<ctg::RacingState&>(game.state());
        state.player.x = 100;
        state.traffic[0].rect = {200, top, ctg::racing_car_width, ctg::racing_car_height};
        auto renderer = std::make_unique<ctg::Renderer>();
        renderer->render_racing(game, false);
        for (int y = 0; y < 240; ++y) {
            const bool visible = y >= 9 && y < 224 && y >= top && y < top + 30;
            if ((renderer->pixels[y * 320 + 208] == 0xf5ed48u) != visible) {
                std::fprintf(stderr, "Racing opponent clipping mismatch top=%d y=%d\n", top, y);
                return 1;
            }
        }
    }
    // Racing labels must remain legible over both dark and bright game pixels.
    const char* grid_digits[10] = {"111101101101111", "001001001001001", "111001111100111",
        "111001111001111", "101101111001001", "111100111001111", "111100111101111",
        "111001001001001", "111101111101111", "111101111001111"};
    for (std::uint32_t background : {0u, 0xffffffu, 0x13579bu}) {
        auto renderer = std::make_unique<ctg::Renderer>();
        renderer->pixels.fill(background);
        renderer->test_grid_overlay(true);
        for (int col = 0; col < 53; ++col) for (int y = 0; y < 11; ++y)
            for (int x = 0; x < 4; ++x) {
                const int digit_y = y < 5 ? y : y - 6;
                const int value = y < 5 ? col / 10 : col % 10;
                const bool ink = y != 5 && x < 3 &&
                    grid_digits[value][digit_y * 3 + x] == '1';
                const auto blend = [](unsigned c) { return (c * 3 + 255) / 4; };
                const auto tinted = x < 3 && y % 5 < 4
                    ? (blend((background >> 16) & 255) << 16) |
                      (blend((background >> 8) & 255) << 8) | blend(background & 255)
                    : background;
                const auto expected = ink ? 0xffffffu : (tinted & 0xfefefeu) >> 1;
                if (renderer->pixels[y * 320 + 54 + col * 4 + x] != expected) {
                    std::fprintf(stderr, "Racing column label mismatch col=%d x=%d y=%d\n", col, x, y);
                    return 1;
                }
            }
        for (int row = 0; row < 48; row += 2) for (int y = 0; y < 5; ++y)
            for (int x = 0; x < 7; ++x) {
                const bool ink = x != 3 && grid_digits[x < 3 ? row / 10 : row % 10][y * 3 + (x < 3 ? x : x - 4)] == '1';
                const auto expected = ink ? 0xffffffu : (background & 0xfefefeu) >> 1;
                if (renderer->pixels[(row * 5 + y) * 320 + 46 + x] != expected) {
                    std::fprintf(stderr, "Racing row label mismatch row=%d x=%d y=%d\n", row, x, y);
                    return 1;
                }
            }
    }
    // Check every pixel, including the spaces around and inside both digits.
    for (unsigned value = 0; value < 100; ++value) {
        auto renderer = std::make_unique<ctg::Renderer>();
        renderer->pixels.fill(0);
        ctg::RacingState state;
        state.score = value;
        renderer->render_racing_score(state);
        for (int y = 0; y < 240; ++y) for (int x = 0; x < 320; ++x) {
            bool lit = false;
            for (int digit = 0; digit < 2; ++digit) {
                const int left = digit == 0 ? 70 : 102;
                const unsigned number = digit == 0 ? value / 10 : value % 10;
                if (x >= left && x < left + 16 && y >= 20 && y < 53) {
                    for (int row = 0; row < 8; ++row)
                        if (y >= 20 + row * 33 / 8 && y < 20 + (row + 1) * 33 / 8)
                            lit = (ctg::score_digits[number][row] & (8u >> ((x - left) / 4))) != 0;
                }
            }
            if (renderer->pixels[y * 320 + x] != (lit ? 0xffffffu : 0u)) {
                std::fprintf(stderr, "Racing score %u mismatch at %d,%d\n", value, x, y);
                return 1;
            }
        }
    }
    // Exercise actual serve state through both ball rendering paths, comparing
    // with the same frame with its ball hidden to measure its exact bounds.
    for (int mode = 0; mode < 7; ++mode) for (int side = 0; side < 2; ++side) {
        ctg::TennisB game(false, mode == 2 || mode == 3, mode == 4 || mode == 5,
                          mode == 1, mode == 3, mode == 5, mode == 6);
        auto& s = const_cast<ctg::TennisState&>(game.state());
        if (side == 1) {
            s.serve_wait = 0; s.ball_x = -60; s.ball_y = 120;
            s.ball_vx = -4; s.ball_vy = 2;
            if (!(game.step() & ctg::sound_point)) return 1;
        }
        for (unsigned i = 0; i < ctg::TennisB::hockey_serve_delay; ++i) game.step();
        auto renderer = std::make_unique<ctg::Renderer>();
        renderer->render_tennis(game, false);
        const auto visible = renderer->pixels;
        s.serve_wait = 1;
        renderer->render_tennis(game, false);
        const int left = side == 0 ? 61 : 253;
        const int top = 21 + s.ball_y * 188 / 240 - 2;
        for (int y = 0; y < 240; ++y) for (int x = 0; x < 320; ++x) {
            const bool ball = x >= left && x < left + 7 && y >= top && y < top + 5;
            if ((visible[y * 320 + x] != renderer->pixels[y * 320 + x]) != ball) {
                std::fprintf(stderr, "Serve pixels mismatch mode=%d side=%d at %d,%d\n", mode, side, x, y);
                return 1;
            }
        }
    }
    static_assert(ctg::menu::option_count(14) == ctg::menu::shooting_row_count);
    static_assert(ctg::menu::shooting_shots_row == 8);
    static_assert(ctg::menu::launch_layout(ctg::menu::shooting_row_count, ctg::menu::shooting_shots_row).first == 4);
    for (unsigned id : {0u, 14u, 15u}) {
        for (auto effect : {fixtures::plain, fixtures::fixed}) {
            const fixtures::Case c{id, fixtures::result_menu, 0, effect};
            const bool should_change = id != 15 && effect == fixtures::plain;
            if ((draw(c) != draw(c, false, false)) != should_change) {
                std::fprintf(stderr, "Unexpected result score visibility for id=%u effect=%u\n", id, static_cast<unsigned>(effect));
                return 1;
            }
        }
    }
    for (auto kind : {fixtures::inplay, fixtures::select, fixtures::launch, fixtures::pause}) {
        for (auto effect : {fixtures::plain, fixtures::fixed}) {
            const fixtures::Case c{14, kind, 0, effect};
            if (draw(c) != draw(c, false, false)) {
                std::fprintf(stderr, "Unexpected shooting score visibility change for kind=%u effect=%u\n", static_cast<unsigned>(kind), static_cast<unsigned>(effect));
                return 1;
            }
        }
    }
    const auto cases = fixtures::cases();
    if (cases.size() != std::size(expected)) {
        std::fprintf(stderr, "Fixture count changed: %zu cases, %zu original goldens\n", cases.size(), std::size(expected));
        return 1;
    }
    unsigned failures = 0;
    for (std::size_t i = 0; i < cases.size(); ++i) {
        const auto actual = draw(cases[i]);
        if (actual != expected[i]) {
            const auto& c = cases[i];
            std::fprintf(stderr, "Frame %zu id=%u kind=%u choice=%u effect=%u: expected %016llx, got %016llx\n",
                         i, c.id, static_cast<unsigned>(c.kind), c.choice, static_cast<unsigned>(c.effect),
                         static_cast<unsigned long long>(expected[i]), static_cast<unsigned long long>(actual));
            ++failures;
        }
    }
    const char* diagnostics = std::getenv("CTG_RENDER_DIAGNOSTICS");
    if (failures || (diagnostics && std::strcmp(diagnostics, "1") == 0)) {
        for (std::size_t i = 0; i < cases.size(); ++i) {
            const auto& c = cases[i];
            if (c.id == 0 && c.kind == fixtures::pause && c.effect == fixtures::plain) {
                std::cerr << "fixture index=" << i << " expected=" << std::hex << std::setfill('0')
                          << std::setw(16) << expected[i] << std::dec << '\n';
                draw(c, true);
            }
        }
    }
    if (failures) {
        std::fprintf(stderr, "%u renderer golden frames failed\n", failures);
        return 1;
    }
    std::printf("Renderer: %zu original-core golden frames passed\n", cases.size());
}
