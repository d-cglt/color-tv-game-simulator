// Full-frame renderer regression hashes; see docs/RENDERER_TESTS.md for their
// provenance, scenario ordering, limitations, and safe update procedure.
#include "renderer.h"
#include "hockey_ball.h"
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
// The order here must match the expected[] table, including the extra effect cases.
inline std::vector<Case> cases() {
    std::vector<Case> out;
    for (unsigned id = 0; id < 16; ++id) {
        out.push_back({id, serve, 0, plain});
        out.push_back({id, inplay, 0, plain});
        out.push_back({id, inplay, 0, fixed});
        out.push_back({id, select, id, plain});
        out.push_back({id, hidden, 0, plain});
        for (unsigned i = 0; i < (id == 15 ? 8u : 7u); ++i)
            out.push_back({id, launch, i, plain});
        for (unsigned i = 0; i < 3; ++i) out.push_back({id, pause, i, plain});
        for (unsigned i = 0; i < 2; ++i) out.push_back({id, result_menu, i, plain});
    }
    for (const auto c : std::array<Case, 8>{{
             {0, select, 0, mono}, {7, select, 15, mono},
             {8, pause, 2, mono}, {14, result_menu, 1, mono},
             {14, launch, 0, mono}, {14, launch, 4, mono},
             {14, launch, 6, mono}, {15, launch, 1, mono}}}) out.push_back(c);
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
// One FNV-1a hash per case, in fixtures::cases() order. Comments encode the
// Case fields; kind and effect numbers correspond to the enums above.
constexpr std::uint64_t expected[] = {
    UINT64_C(0xde91ce5585951b45), // id=0 kind=0 choice=0 effect=0
    UINT64_C(0x0468239e93d6d079), // id=0 kind=1 choice=0 effect=0
    UINT64_C(0xc9254751298f3da9), // id=0 kind=1 choice=0 effect=1
    UINT64_C(0xf941c620922fc5df), // id=0 kind=2 choice=0 effect=0
    UINT64_C(0x63dda589905eb18d), // id=0 kind=6 choice=0 effect=0
    UINT64_C(0x1abed858c7753626), // id=0 kind=3 choice=0 effect=0
    UINT64_C(0x3338437c7465ba97), // id=0 kind=3 choice=1 effect=0
    UINT64_C(0x9299d50dce649011), // id=0 kind=3 choice=2 effect=0
    UINT64_C(0x17bb1c6c41c091b3), // id=0 kind=3 choice=3 effect=0
    UINT64_C(0xd3b259b6dba67c07), // id=0 kind=3 choice=4 effect=0
    UINT64_C(0x3871e570ff88a5ca), // id=0 kind=3 choice=5 effect=0
    UINT64_C(0xa60efae9b1a1a31c), // id=0 kind=3 choice=6 effect=0
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
    UINT64_C(0x16b2a9f716c4cde2), // id=1 kind=3 choice=0 effect=0
    UINT64_C(0xe4948244906a3c7b), // id=1 kind=3 choice=1 effect=0
    UINT64_C(0xafc405b21c541b06), // id=1 kind=3 choice=2 effect=0
    UINT64_C(0x11432163d325cc47), // id=1 kind=3 choice=3 effect=0
    UINT64_C(0x64b4fd3bd3ac36eb), // id=1 kind=3 choice=4 effect=0
    UINT64_C(0x371b6e841fbf2bde), // id=1 kind=3 choice=5 effect=0
    UINT64_C(0x45a9f20ffc9392f0), // id=1 kind=3 choice=6 effect=0
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
    UINT64_C(0x15ffeda7d13c7e6f), // id=2 kind=3 choice=0 effect=0
    UINT64_C(0xb869b7f9405a8a06), // id=2 kind=3 choice=1 effect=0
    UINT64_C(0x257e5db6b57f4913), // id=2 kind=3 choice=2 effect=0
    UINT64_C(0xa1e2a9f18f41e883), // id=2 kind=3 choice=3 effect=0
    UINT64_C(0x117c0951cf78c66b), // id=2 kind=3 choice=4 effect=0
    UINT64_C(0x6dbf0a3abb52a72e), // id=2 kind=3 choice=5 effect=0
    UINT64_C(0xc6042a3f3ca693e2), // id=2 kind=3 choice=6 effect=0
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
    UINT64_C(0xc78faa627c540f35), // id=3 kind=3 choice=0 effect=0
    UINT64_C(0xd17c7a74b7be73bc), // id=3 kind=3 choice=1 effect=0
    UINT64_C(0xf0b5642e21e8ec20), // id=3 kind=3 choice=2 effect=0
    UINT64_C(0x9086798787da8b0d), // id=3 kind=3 choice=3 effect=0
    UINT64_C(0xea62657f01aba515), // id=3 kind=3 choice=4 effect=0
    UINT64_C(0x595dd621f31eee38), // id=3 kind=3 choice=5 effect=0
    UINT64_C(0x8b9e66803e9bc428), // id=3 kind=3 choice=6 effect=0
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
    UINT64_C(0x0504837b18ca5c07), // id=4 kind=3 choice=0 effect=0
    UINT64_C(0x2fdb326b30bb42d5), // id=4 kind=3 choice=1 effect=0
    UINT64_C(0x101eed0a95219c32), // id=4 kind=3 choice=2 effect=0
    UINT64_C(0xb2e3e0f9b0f6d4d1), // id=4 kind=3 choice=3 effect=0
    UINT64_C(0xdf2931d6745fbe82), // id=4 kind=3 choice=4 effect=0
    UINT64_C(0x1d44f5eeda09bbe2), // id=4 kind=3 choice=5 effect=0
    UINT64_C(0x5c756a0b127d044d), // id=4 kind=3 choice=6 effect=0
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
    UINT64_C(0x654cf2505ed624a2), // id=5 kind=3 choice=0 effect=0
    UINT64_C(0x1bf6216897060294), // id=5 kind=3 choice=1 effect=0
    UINT64_C(0x13b547ab7c8cbd69), // id=5 kind=3 choice=2 effect=0
    UINT64_C(0x02ccf63a420ba680), // id=5 kind=3 choice=3 effect=0
    UINT64_C(0x4ae89fdb854cafbb), // id=5 kind=3 choice=4 effect=0
    UINT64_C(0x0d3aad8b4542704b), // id=5 kind=3 choice=5 effect=0
    UINT64_C(0x411e208eca8531e1), // id=5 kind=3 choice=6 effect=0
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
    UINT64_C(0x0a9af52df28afc83), // id=6 kind=3 choice=0 effect=0
    UINT64_C(0xf2fbcbe87a3a9e51), // id=6 kind=3 choice=1 effect=0
    UINT64_C(0x387aafed8640ddae), // id=6 kind=3 choice=2 effect=0
    UINT64_C(0xcb522914da9d1185), // id=6 kind=3 choice=3 effect=0
    UINT64_C(0x368a947a67c75d35), // id=6 kind=3 choice=4 effect=0
    UINT64_C(0x3da7e8bd39683ead), // id=6 kind=3 choice=5 effect=0
    UINT64_C(0x947d03f345e9a1a2), // id=6 kind=3 choice=6 effect=0
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
    UINT64_C(0xb230c97f5e2c1146), // id=7 kind=3 choice=0 effect=0
    UINT64_C(0x2e743552d1f91f70), // id=7 kind=3 choice=1 effect=0
    UINT64_C(0x4cfb749c13ec827d), // id=7 kind=3 choice=2 effect=0
    UINT64_C(0x603dc63a37913264), // id=7 kind=3 choice=3 effect=0
    UINT64_C(0xa60dd4367d4be9f4), // id=7 kind=3 choice=4 effect=0
    UINT64_C(0x8fb41caa208f356c), // id=7 kind=3 choice=5 effect=0
    UINT64_C(0x43eab9edb887c067), // id=7 kind=3 choice=6 effect=0
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
    UINT64_C(0x1f9b39864be29731), // id=8 kind=3 choice=0 effect=0
    UINT64_C(0x3b05c6505b3d7559), // id=8 kind=3 choice=1 effect=0
    UINT64_C(0x92c40c61377c8d16), // id=8 kind=3 choice=2 effect=0
    UINT64_C(0x02a3829f7c7d799d), // id=8 kind=3 choice=3 effect=0
    UINT64_C(0xdd6465a34858bf79), // id=8 kind=3 choice=4 effect=0
    UINT64_C(0x2d5d4e5018134829), // id=8 kind=3 choice=5 effect=0
    UINT64_C(0x247af7e6c9596586), // id=8 kind=3 choice=6 effect=0
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
    UINT64_C(0xaae7e07617ee6055), // id=9 kind=3 choice=0 effect=0
    UINT64_C(0x8f9f5ece1173eff5), // id=9 kind=3 choice=1 effect=0
    UINT64_C(0xe2120994202a4909), // id=9 kind=3 choice=2 effect=0
    UINT64_C(0x0f2d4fed085b22e9), // id=9 kind=3 choice=3 effect=0
    UINT64_C(0x7d312d28540c32ed), // id=9 kind=3 choice=4 effect=0
    UINT64_C(0xd72faea293e419d5), // id=9 kind=3 choice=5 effect=0
    UINT64_C(0xf6d8a0ae1213e442), // id=9 kind=3 choice=6 effect=0
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
    UINT64_C(0x77a7d49be25073fa), // id=10 kind=3 choice=0 effect=0
    UINT64_C(0xa68dc278907721f2), // id=10 kind=3 choice=1 effect=0
    UINT64_C(0x73683c61daf91c1e), // id=10 kind=3 choice=2 effect=0
    UINT64_C(0xeb1bfad1391b39b6), // id=10 kind=3 choice=3 effect=0
    UINT64_C(0xa0b51404598984ba), // id=10 kind=3 choice=4 effect=0
    UINT64_C(0x05ccd5490ba754ba), // id=10 kind=3 choice=5 effect=0
    UINT64_C(0x6664a8a78d9df5b6), // id=10 kind=3 choice=6 effect=0
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
    UINT64_C(0x3f6f4cf7ba957689), // id=11 kind=3 choice=0 effect=0
    UINT64_C(0x02ff5f2fdb1fc1d9), // id=11 kind=3 choice=1 effect=0
    UINT64_C(0x104e276480ef65a1), // id=11 kind=3 choice=2 effect=0
    UINT64_C(0xdc36f45d0a887309), // id=11 kind=3 choice=3 effect=0
    UINT64_C(0xb94e1508df26f345), // id=11 kind=3 choice=4 effect=0
    UINT64_C(0x7d45e059e63e5af5), // id=11 kind=3 choice=5 effect=0
    UINT64_C(0x0b0444a1eb35ff9d), // id=11 kind=3 choice=6 effect=0
    UINT64_C(0x78380c2bd6491aba), // id=11 kind=4 choice=0 effect=0
    UINT64_C(0xb48e002049b9b15e), // id=11 kind=4 choice=1 effect=0
    UINT64_C(0x4294962b399e91b6), // id=11 kind=4 choice=2 effect=0
    UINT64_C(0x23b04fd09a42c8ee), // id=11 kind=5 choice=0 effect=0
    UINT64_C(0x6d5e61328a16943e), // id=11 kind=5 choice=1 effect=0
    UINT64_C(0x3adefa7197222205), // id=12 kind=0 choice=0 effect=0
    UINT64_C(0xf5f3c96d1d3cd7bc), // id=12 kind=1 choice=0 effect=0
    UINT64_C(0x92b3f1b39487442c), // id=12 kind=1 choice=0 effect=1
    UINT64_C(0xcfa0a0e6d74163b9), // id=12 kind=2 choice=12 effect=0
    UINT64_C(0xb7c590bc3c54c5b8), // id=12 kind=6 choice=0 effect=0
    UINT64_C(0x06a9189c45117e22), // id=12 kind=3 choice=0 effect=0
    UINT64_C(0xee65d43db296b9d2), // id=12 kind=3 choice=1 effect=0
    UINT64_C(0xc32409db4f6bc766), // id=12 kind=3 choice=2 effect=0
    UINT64_C(0x9684784e5472cdc6), // id=12 kind=3 choice=3 effect=0
    UINT64_C(0x1208cf23dc2b6b36), // id=12 kind=3 choice=4 effect=0
    UINT64_C(0xf02ff9e7ecc47ba5), // id=12 kind=3 choice=5 effect=0
    UINT64_C(0xe7f25197fd3a487e), // id=12 kind=3 choice=6 effect=0
    UINT64_C(0x0eddc55a2869effd), // id=12 kind=4 choice=0 effect=0
    UINT64_C(0x82f64818b6a0e4fd), // id=12 kind=4 choice=1 effect=0
    UINT64_C(0x710827e23cbea351), // id=12 kind=4 choice=2 effect=0
    UINT64_C(0x82a50f93db38cdc5), // id=12 kind=5 choice=0 effect=0
    UINT64_C(0x946385c13ac63a79), // id=12 kind=5 choice=1 effect=0
    UINT64_C(0x78ac1018ad338d25), // id=13 kind=0 choice=0 effect=0
    UINT64_C(0xf6b935e52d339fbc), // id=13 kind=1 choice=0 effect=0
    UINT64_C(0x70b3cf25714e1eec), // id=13 kind=1 choice=0 effect=1
    UINT64_C(0x05a48a94d671da54), // id=13 kind=2 choice=13 effect=0
    UINT64_C(0xd8830f562da9af14), // id=13 kind=6 choice=0 effect=0
    UINT64_C(0xba282b432dc2efc1), // id=13 kind=3 choice=0 effect=0
    UINT64_C(0xb4ab1af983f999fd), // id=13 kind=3 choice=1 effect=0
    UINT64_C(0xcda3f16e2e9c00fd), // id=13 kind=3 choice=2 effect=0
    UINT64_C(0xc71b614b614a4e31), // id=13 kind=3 choice=3 effect=0
    UINT64_C(0x745e97a8a0ad9205), // id=13 kind=3 choice=4 effect=0
    UINT64_C(0x80704a9b5579b656), // id=13 kind=3 choice=5 effect=0
    UINT64_C(0xc33520396e918f65), // id=13 kind=3 choice=6 effect=0
    UINT64_C(0x206c12b2a7d30f9d), // id=13 kind=4 choice=0 effect=0
    UINT64_C(0x50adbe4f92a5525d), // id=13 kind=4 choice=1 effect=0
    UINT64_C(0xfb61ee2bc82076a9), // id=13 kind=4 choice=2 effect=0
    UINT64_C(0xef45b51d5d9cc5ed), // id=13 kind=5 choice=0 effect=0
    UINT64_C(0xdab3e5113ad11fa1), // id=13 kind=5 choice=1 effect=0
    UINT64_C(0xfe0bd81cdb27abe5), // id=14 kind=0 choice=0 effect=0
    UINT64_C(0x577ae7e1f856f7e1), // id=14 kind=1 choice=0 effect=0
    UINT64_C(0xf07708fb422eb571), // id=14 kind=1 choice=0 effect=1
    UINT64_C(0x040c029a50b5c3bf), // id=14 kind=2 choice=14 effect=0
    UINT64_C(0xa1b31116b3cf593d), // id=14 kind=6 choice=0 effect=0
    UINT64_C(0x0c4cd66c568619da), // id=14 kind=3 choice=0 effect=0
    UINT64_C(0x0ea3e9785a2db39f), // id=14 kind=3 choice=1 effect=0
    UINT64_C(0xf4381a99ae101865), // id=14 kind=3 choice=2 effect=0
    UINT64_C(0x2267281dc9cfa620), // id=14 kind=3 choice=3 effect=0
    UINT64_C(0x6a101e6bbdfbdf6a), // id=14 kind=3 choice=4 effect=0
    UINT64_C(0x7e255b7f9f4e2a0d), // id=14 kind=3 choice=5 effect=0
    UINT64_C(0x01ce331328ac8658), // id=14 kind=3 choice=6 effect=0

    UINT64_C(0x8061322b569d086e), // id=14 kind=4 choice=0 effect=0
    UINT64_C(0xd793337e27afd7bb), // id=14 kind=4 choice=1 effect=0
    UINT64_C(0x20ecd0f502a6e5db), // id=14 kind=4 choice=2 effect=0
    UINT64_C(0xcada23369030af57), // id=14 kind=5 choice=0 effect=0
    UINT64_C(0xc424c74e8e544507), // id=14 kind=5 choice=1 effect=0
    UINT64_C(0xc1f3757c19b62c1d), // id=15 kind=0 choice=0 effect=0
    UINT64_C(0xadd4ec5dc1cbf9d5), // id=15 kind=1 choice=0 effect=0
    UINT64_C(0xde85234b8f7f40f5), // id=15 kind=1 choice=0 effect=1
    UINT64_C(0xe330fabd45fa461e), // id=15 kind=2 choice=15 effect=0
    UINT64_C(0x64d7345123cd7e1f), // id=15 kind=6 choice=0 effect=0
    UINT64_C(0x440c1bfdb6f74d0c), // id=15 kind=3 choice=0 effect=0
    UINT64_C(0xde972a9f53ef6732), // id=15 kind=3 choice=1 effect=0
    UINT64_C(0xea8d48161454ba5a), // id=15 kind=3 choice=2 effect=0
    UINT64_C(0x95fb967ec3fe11f2), // id=15 kind=3 choice=3 effect=0
    UINT64_C(0x3f8e384f1131debc), // id=15 kind=3 choice=4 effect=0
    UINT64_C(0x7303b0be302403d2), // id=15 kind=3 choice=5 effect=0
    UINT64_C(0x0e6a4f6192376cc9), // id=15 kind=3 choice=6 effect=0
    UINT64_C(0x8fb91683f1b7dcee), // id=15 kind=3 choice=7 effect=0
    UINT64_C(0x582794b47a6cb7fc), // id=15 kind=4 choice=0 effect=0
    UINT64_C(0xc9cad7302c236436), // id=15 kind=4 choice=1 effect=0
    UINT64_C(0xf0c95b87644ee6e6), // id=15 kind=4 choice=2 effect=0
    UINT64_C(0x59b52d6aad097b12), // id=15 kind=5 choice=0 effect=0
    UINT64_C(0x228b6330669d7c96), // id=15 kind=5 choice=1 effect=0
    UINT64_C(0x1c2528de9edb0a81), // id=0 kind=2 choice=0 effect=2
    UINT64_C(0xa8dc2ea4f11cca6d), // id=7 kind=2 choice=15 effect=2
    UINT64_C(0x209cdf643581e1ff), // id=8 kind=4 choice=2 effect=2
    UINT64_C(0xf8c6221bff0a4c78), // id=14 kind=5 choice=1 effect=2
    UINT64_C(0xe9f4bc22a40c5c72), // id=14 kind=3 choice=0 effect=2
    UINT64_C(0x37b967ec3722b6f2), // id=14 kind=3 choice=4 effect=2
    UINT64_C(0x1d82c75a5d1a2c1b), // id=14 kind=3 choice=6 effect=2
    UINT64_C(0x868416a532b19542), // id=15 kind=3 choice=1 effect=2
    UINT64_C(0xb57ff59664dde289), // id=0 kind=1 choice=0 effect=3
    UINT64_C(0xba8246fc5648a6de), // id=6 kind=1 choice=0 effect=3
    UINT64_C(0x0b8fde84ced97dd6), // id=10 kind=1 choice=0 effect=3
    UINT64_C(0xda9983a375f59cc9), // id=12 kind=1 choice=0 effect=3
    UINT64_C(0x578d1d54e7181abb), // id=14 kind=1 choice=0 effect=3
    UINT64_C(0xe98cd1466f71f286), // id=15 kind=1 choice=0 effect=3
    UINT64_C(0x65a5d4b57100d86f), // id=0 kind=1 choice=0 effect=4
    UINT64_C(0xa850f99965988449), // id=6 kind=1 choice=0 effect=4
    UINT64_C(0xbea2dd0f1fd72421), // id=10 kind=1 choice=0 effect=4
    UINT64_C(0xc758a770648e7659), // id=12 kind=1 choice=0 effect=4
    UINT64_C(0x253e8a917aea4b1b), // id=14 kind=1 choice=0 effect=4
    UINT64_C(0x600b3d3868fdb480), // id=15 kind=1 choice=0 effect=4
    UINT64_C(0x2c1416a774c02d63), // id=0 kind=1 choice=0 effect=5
    UINT64_C(0x5db8294aac3579e9), // id=6 kind=1 choice=0 effect=5
    UINT64_C(0xb5e8687612b0a299), // id=10 kind=1 choice=0 effect=5
    UINT64_C(0x3585c82bb59ce754), // id=12 kind=1 choice=0 effect=5
    UINT64_C(0x0835046b243a2086), // id=14 kind=1 choice=0 effect=5
    UINT64_C(0x69a441be048b49fe), // id=15 kind=1 choice=0 effect=5
    UINT64_C(0x62b0de5cfd965bc8), // id=0 kind=1 choice=0 effect=6
    UINT64_C(0xa3960a954132db90), // id=6 kind=1 choice=0 effect=6
    UINT64_C(0x87f2321f02c04487), // id=10 kind=1 choice=0 effect=6
    UINT64_C(0x70f02760422bdaad), // id=12 kind=1 choice=0 effect=6
    UINT64_C(0xccadb622ea6d1449), // id=14 kind=1 choice=0 effect=6
    UINT64_C(0x61cf629b5e47f804), // id=15 kind=1 choice=0 effect=6
    UINT64_C(0x07ab351da28c2619), // id=0 kind=1 choice=0 effect=7
    UINT64_C(0x2f4df1355f1e3226), // id=6 kind=1 choice=0 effect=7
    UINT64_C(0x18619a684cc523e1), // id=10 kind=1 choice=0 effect=7
    UINT64_C(0x5c4c0a77f0ffa64f), // id=12 kind=1 choice=0 effect=7
    UINT64_C(0xdb182049acf28490), // id=14 kind=1 choice=0 effect=7
    UINT64_C(0xd5e9047ce9532aa8), // id=15 kind=1 choice=0 effect=7
    UINT64_C(0xdbbceef4a4c524a5), // id=15 kind=7 choice=0 effect=0

};

std::uint64_t draw(const fixtures::Case& c, bool diagnostics = false, bool rally_score_visible = true, unsigned racing_mode = 0) {
    fixtures::Fixture f(c);
    const bool detail = c.kind != fixtures::select && c.kind != fixtures::launch && c.kind != fixtures::hidden;
    const ctg::RenderState state{
        c.id, c.kind == fixtures::select, detail, c.kind == fixtures::pause,
        c.kind == fixtures::result_menu, c.kind == fixtures::hidden,
        c.choice, c.choice, c.choice, c.choice,
        true, {true, false}, 21, 30, c.choice % 5, c.choice % 4, c.choice % 3, c.choice % 3, racing_mode,
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
    // The ice palette must cover walls, the variant barrier, and both score digits.
    for (bool ice : {false, true}) for (bool fixed : {false, true}) {
        ctg::TennisB game(false, true, false, false, true);
        game.set_ice_hockey(ice);
        auto renderer = std::make_unique<ctg::Renderer>();
        renderer->render_tennis(game, true, {fixed, 0, 0});
        const auto pixel = [&](int x, int y) { return renderer->pixels[y * 320 + x]; };
        const unsigned ink = ice ? 0x245574 : 0xaefbff;
        if (pixel(100, 20) != (ice ? 0x285c9cu : 0xb1ffbfu) ||
            pixel(157, 28) != (ice ? 0x285c9cu : ctg::hockey_boundary) ||
            pixel(0, 0) != 0x505068u ||
            pixel(73, 44) != ink || pixel(247, 44) != ink ||
            pixel(100, 100) != (ice ? 0xb9e4edu : 0x2cc94cu) ||
            pixel(49, game.paddle_visible_top(0)) != (ice ? 0xe67fdbu : 0xff9fffu) ||
            pixel(265, game.paddle_visible_top(1)) != (ice ? 0xb5cf60u : 0xd5e87cu)) {
            std::fprintf(stderr, "Hockey palette mismatch ice=%d fixed=%d\n", ice, fixed);
            return 1;
        }
    }
    // Doubles inner paddles use the same owner colors as the outer paddles.
    for (bool ice : {false, true}) {
        ctg::TennisB game(true, true);
        game.set_ice_hockey(ice);
        auto renderer = std::make_unique<ctg::Renderer>();
        renderer->render_tennis(game, false);
        if (renderer->pixels[game.paddle_visible_top(1) * 320 + 121] != (ice ? 0xb5cf60u : 0xd5e87cu) ||
            renderer->pixels[game.paddle_visible_top(0) * 320 + 193] != (ice ? 0xe67fdbu : 0xff9fffu)) {
            std::fprintf(stderr, "Hockey doubles palette mismatch ice=%d\n", ice);
            return 1;
        }
    }
    // Check the puck both on the court and across the exterior boundary.
    for (bool ice : {false, true}) for (int x : {160, -8}) {
        ctg::TennisB game(false, true);
        game.set_ice_hockey(ice);
        auto& state = const_cast<ctg::TennisState&>(game.state());
        state.serve_wait = 0; state.ball_x = x; state.ball_y = 120;
        auto renderer = std::make_unique<ctg::Renderer>();
        renderer->render_tennis(game, false);
        const int px = ctg::hockey_x(x), py = ctg::hockey_y(120);
        const unsigned expected = ice ? 0x243e70 : x == 160 ? ctg::hockey_puck_color : ctg::hockey_exterior_puck_color;
        if (renderer->pixels[py * 320 + px] != expected) {
            std::fprintf(stderr, "Hockey puck palette mismatch ice=%d x=%d\n", ice, x);
            return 1;
        }
    }
    // Hidden previews use the demo game's ice palette, including fixed test scores.
    {
        fixtures::Case c{4, fixtures::hidden, 0, fixtures::plain};
        fixtures::Fixture f(c);
        f.demo_tennis.set_ice_hockey(true);
        ctg::RenderState state{4, false, false, false, false, true,
            0, 0, 0, 0, false, {}, 15, 15, 0, 0, 0, 0, 0,
            f.modes, f.tennis, f.demo_tennis, f.shooting, f.demo_shooting,
            f.racing, f.demo_racing, true, {true, 0, 0}, true};
        auto renderer = std::make_unique<ctg::Renderer>();
        renderer->render(state);
        if (renderer->pixels[20 * 320 + 100] != 0x285c9cu ||
                    renderer->pixels[0] != 0x505068u ||
            renderer->pixels[44 * 320 + 73] != 0x245574u ||
            renderer->pixels[f.demo_tennis.paddle_visible_top(0) * 320 + 49] != 0xe67fdbu ||
            renderer->pixels[f.demo_tennis.paddle_visible_top(1) * 320 + 265] != 0xb5cf60u) {
            std::fprintf(stderr, "Ice preview palette mismatch\n");
            return 1;
        }
    }
    // Mixed surfaces split visible dashes at the exact dirt transition in every layout.
    for (int mode = 0; mode < 3; ++mode) for (int position : {-125, -1475})
        for (unsigned phase : {0u, 100u, 239u}) for (bool show_score : {false, true}) {
            ctg::Racing game;
            auto& state = const_cast<ctg::RacingState&>(game.state());
            state.narrow = mode == 1;
            state.two_players = mode == 2;
            state.bad_road = true;
            state.road_seed = 0;
            state.road_position = position;
            state.road_scroll = phase;
            state.player.y = state.player2.y = -100;
            state.traffic[0].rect.y = state.traffic2[0].rect.y = -100;
            auto renderer = std::make_unique<ctg::Renderer>();
            renderer->render_racing(game, show_score);
            bool saw_dirt = false, saw_normal = false;
            for (int y = 0; y < 240; ++y) {
                const bool dirt = ctg::racing_dirt_at(state, y);
                saw_dirt |= dirt; saw_normal |= !dirt;
                const bool dash = (y - static_cast<int>(phase) + 270) % 270 < 240;
                const unsigned road = dirt ? 0x965b2cu : mode == 1 ? 0x6514a4u : mode == 2 ? 0x287b53u : 0x39b800u;
                const unsigned border = dirt ? 0xf5b84bu : mode == 1 ? 0xd9b5f5u : 0xc3f5dfu;
                for (int x : {54, 57, 158, 161, 262, 265}) {
                    const bool line = x < 58 || x >= 262 || mode != 0;
                    const unsigned expected = line && dash ? border : road;
                    if (renderer->pixels[y * 320 + x] != expected) {
                        std::fprintf(stderr, "Racing dirt line mismatch mode=%d position=%d phase=%u score=%d x=%d y=%d\n", mode, position, phase, show_score, x, y);
                        return 1;
                    }
                }
            }
            if (!saw_dirt || !saw_normal) return 1;
        }
    // Border gaps are six cells high, including when clipped by screen edges.
    for (unsigned phase : {0u, 15u, 100u, 239u, 269u}) {
        ctg::Racing game;
        auto& state = const_cast<ctg::RacingState&>(game.state());
        state.road_scroll = phase;
        auto renderer = std::make_unique<ctg::Renderer>();
        renderer->render_racing(game, false);
        for (int y = 0; y < 240; ++y) for (int x : {54, 265}) {
            const int relative = (y - static_cast<int>(phase) + 270) % 270;
            const unsigned expected = relative < 240 ? 0xc3f5dfu : 0x39b800u;
            if (renderer->pixels[y * 320 + x] != expected) {
                std::fprintf(stderr, "Racing border mismatch phase=%u x=%d y=%d\n", phase, x, y);
                return 1;
            }
        }
    }
    // Narrow mode uses the same scrolling gaps for borders and the center divider.
    for (unsigned phase : {0u, 15u, 100u, 239u, 269u}) {
        ctg::Racing game;
        auto& state = const_cast<ctg::RacingState&>(game.state());
        state.narrow = true;
        state.road_scroll = phase;
        state.player = {100, 100, ctg::racing_car_width, ctg::racing_car_height};
        state.traffic[0].rect = {200, 100, ctg::racing_car_width, ctg::racing_car_height};
        auto renderer = std::make_unique<ctg::Renderer>();
        renderer->render_racing(game, false);
        for (int y = 0; y < 240; ++y) {
            const int relative = (y - static_cast<int>(phase) + 270) % 270;
            const unsigned color = relative < 240 ? 0xd9b5f5u : 0x6514a4u;
            for (int x : {54, 57, 158, 159, 160, 161, 262, 265}) {
                if (renderer->pixels[y * 320 + x] != color) {
                    std::fprintf(stderr, "Narrow racing line mismatch phase=%u x=%d y=%d\n", phase, x, y);
                    return 1;
                }
            }
            for (int x : {157, 162}) if (renderer->pixels[y * 320 + x] != 0x6514a4u) return 1;
            if (renderer->pixels[y * 320 + 53] != 0x66594au || renderer->pixels[y * 320 + 266] != 0x66594au) return 1;
        }
        if (renderer->pixels[100 * 320 + 108] != 0xffa0cdu ||
            renderer->pixels[100 * 320 + 208] != 0xf5ed48u) return 1;
    }
    // Two-player lines share the edge scroll phase.
    for (unsigned phase : {0u, 15u, 100u, 239u, 269u}) {
        ctg::Racing game;
        game.set_two_players(true);
        auto& state = const_cast<ctg::RacingState&>(game.state());
        state.road_scroll = phase;
        state.player = {80, 100, ctg::racing_car_width, ctg::racing_car_height};
        state.player2 = {200, 100, ctg::racing_car_width, ctg::racing_car_height};
        state.traffic[0].rect = {120, 150, ctg::racing_car_width, ctg::racing_car_height};
        state.traffic2[0].rect = {220, 150, ctg::racing_car_width, ctg::racing_car_height};
        auto renderer = std::make_unique<ctg::Renderer>();
        renderer->render_racing(game, false);
        for (int y = 0; y < 240; ++y) {
            const int relative = (y - static_cast<int>(phase) + 270) % 270;
            const unsigned color = relative < 240 ? 0xc3f5dfu : 0x287b53u;
            for (int x : {54, 57, 158, 159, 160, 161, 262, 265})
                if (renderer->pixels[y * 320 + x] != color) return 1;
        }
        for (int x : {88, 208}) if (renderer->pixels[100 * 320 + x] != 0x98ff8du) return 1;
        for (int x : {128, 228}) if (renderer->pixels[150 * 320 + x] != 0xf5ed48u) return 1;
        state.traffic2[0].collided = true;
        state.traffic2[0].rect.y = 0;
        renderer->render_racing(game, true, false);
        if (renderer->pixels[100 * 320 + 88] != 0x98ff8du ||
            renderer->pixels[100 * 320 + 208] != 0x98ff8du ||
            renderer->pixels[8 * 320 + 228] != 0x287b53u ||
            renderer->pixels[9 * 320 + 228] != 0xffc782u) return 1;
        state.blink_samples = ctg::racing_blink_total_samples - ctg::racing_blink_segment_samples;
        renderer->render_racing(game, true, false);
        if (renderer->pixels[100 * 320 + 88] != 0x287b53u ||
            renderer->pixels[100 * 320 + 208] != 0x98ff8du) return 1;
        state.blink_samples = 0;
        state.blink_samples2 = ctg::racing_blink_total_samples - ctg::racing_blink_segment_samples;
        renderer->render_racing(game, true, false);
        if (renderer->pixels[100 * 320 + 88] != 0x98ff8du ||
            renderer->pixels[100 * 320 + 208] != 0x287b53u) return 1;
        renderer->render_racing(game, false, false);
        if (renderer->pixels[100 * 320 + 208] != 0x98ff8du) return 1;
    }
    // Extra traffic is active only when enabled, and the second lane only in two-player mode.
    for (int mode = 0; mode < 3; ++mode) for (bool more : {false, true}) {
        ctg::Racing game;
        auto& state = const_cast<ctg::RacingState&>(game.state());
        state.narrow = mode == 1;
        state.two_players = mode == 2;
        state.more_cars = more;
        state.player = {70, 180, ctg::racing_car_width, ctg::racing_car_height};
        state.player2 = {200, 180, ctg::racing_car_width, ctg::racing_car_height};
        state.traffic[0].rect = {80, 60, ctg::racing_car_width, ctg::racing_car_height};
        state.traffic[1].rect = {110, 60, ctg::racing_car_width, ctg::racing_car_height};
        state.traffic[1].collided = true;
        state.traffic2[0].rect = {180, 60, ctg::racing_car_width, ctg::racing_car_height};
        state.traffic2[1].rect = {210, 60, ctg::racing_car_width, ctg::racing_car_height};
        auto renderer = std::make_unique<ctg::Renderer>();
        renderer->render_racing(game, false);
        const auto pixel = [&](int x) { return renderer->pixels[60 * 320 + x + 8]; };
        const unsigned road = mode == 1 ? 0x6514a4u : mode == 2 ? 0x287b53u : 0x39b800u;
        if (pixel(80) != 0xf5ed48u || pixel(110) != (more ? 0xffc782u : road) ||
            pixel(180) != (mode == 2 ? 0xf5ed48u : road) ||
            pixel(210) != (mode == 2 && more ? 0xf5ed48u : road)) {
            std::fprintf(stderr, "Racing extra traffic mismatch mode=%d more=%d\n", mode, more);
            return 1;
        }
    }
    // Extra opponents use the same top and bottom clipping as the first opponent.
    for (int top : {-30, -7, 8, 9, 194, 213, 223, 224, 240}) {
        ctg::Racing game;
        auto& state = const_cast<ctg::RacingState&>(game.state());
        state.two_players = true;
        state.more_cars = true;
        state.player.x = 70;
        state.player2.x = 180;
        state.traffic[0].rect.x = 80;
        state.traffic2[0].rect.x = 180;
        state.traffic[1].rect = {110, top, ctg::racing_car_width, ctg::racing_car_height};
        state.traffic2[1].rect = {210, top, ctg::racing_car_width, ctg::racing_car_height};
        state.traffic2[1].collided = true;
        auto renderer = std::make_unique<ctg::Renderer>();
        renderer->render_racing(game, false);
        for (int y = 0; y < 240; ++y) {
            const bool visible = y >= 9 && y < 224 && y >= top && y < top + ctg::racing_car_height;
            if ((renderer->pixels[y * 320 + 118] == 0xf5ed48u) != visible ||
                (renderer->pixels[y * 320 + 218] == 0xffc782u) != visible) {
                std::fprintf(stderr, "Racing extra opponent clipping mismatch top=%d y=%d\n", top, y);
                return 1;
            }
        }
    }
    // Each lane has an independent two-digit score and override.
    {
        ctg::RacingState state;
        state.two_players = true;
        state.score = 12;
        state.score2 = 34;
        auto actual = std::make_unique<ctg::Renderer>();
        auto expected_scores = std::make_unique<ctg::Renderer>();
        for (bool override_enabled : {false, true}) {
            actual->pixels.fill(0);
            expected_scores->pixels.fill(0);
            actual->render_racing_score(state, {override_enabled, 56, 78});
            const unsigned values[] = {override_enabled ? 56u : 12u, override_enabled ? 78u : 34u};
            for (int lane = 0; lane < 2; ++lane) {
                expected_scores->score_digit(ctg::racing_x(4 + lane * 26), 20, values[lane] / 10, 0xffffff, 4);
                expected_scores->score_digit(ctg::racing_x(12 + lane * 26), 20, values[lane] % 10, 0xffffff, 4);
            }
            if (actual->pixels != expected_scores->pixels) return 1;
        }
    }
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
    // Check the actual title pixels, including the area where an old suffix
    // would appear, in all three Racing title presentation paths.
    for (auto kind : {fixtures::select, fixtures::launch, fixtures::hidden}) {
        fixtures::Case c{15, kind, kind == fixtures::select ? 15u : 1u, fixtures::plain};
        fixtures::Fixture f(c);
        ctg::RenderState state{15, kind == fixtures::select, false, false, false,
            kind == fixtures::hidden, 15, 1, 0, 0, true, {true, false},
            21, 30, 0, 0, 0, 0, 0, f.modes, f.tennis, f.demo_tennis,
            f.shooting, f.demo_shooting, f.racing, f.demo_racing, true, {}, true};
        auto actual = std::make_unique<ctg::Renderer>();
        auto expected_title = std::make_unique<ctg::Renderer>();
        actual->render(state);
        expected_title->render_racing(f.demo_racing, false, true, {});
        if (kind != fixtures::hidden) expected_title->darken_panel();
        const unsigned y = kind == fixtures::select ? 171 : kind == fixtures::launch ? 58 : 226;
        if (kind == fixtures::select) expected_title->rect(55, 168, 210, 13, 0x295166);
        expected_title->centered_text(y, "RACING 112", kind == fixtures::select ? 0x70e0ff : 0xffffff);
        for (unsigned py = y; py < y + 7; ++py)
            for (unsigned x = 0; x < ctg::Graphics::width; ++x)
                if (actual->pixels[py * ctg::Graphics::width + x] != expected_title->pixels[py * ctg::Graphics::width + x]) {
                    std::fprintf(stderr, "Racing title pixels differ in path %u at %u,%u\n", kind, x, py);
                    return 1;
                }
    }
    const fixtures::Case racing_mode_menu{15, fixtures::launch, 2, fixtures::plain};
    if (draw(racing_mode_menu) == draw(racing_mode_menu, false, true, 1)) {
        std::fprintf(stderr, "Racing mode menu did not change for narrow mode\n");
        return 1;
    }
    static_assert(ctg::menu::option_count(15) == ctg::menu::racing_row_count);
    static_assert(ctg::menu::racing_speed_row == 3 && ctg::menu::racing_row_count == 8);
    static_assert(ctg::menu::launch_layout(ctg::menu::racing_row_count, 7).scroll);
    static_assert(ctg::menu::launch_layout(ctg::menu::racing_row_count, 7).first == 3);
    static_assert(ctg::menu::option_count(14) == ctg::menu::shooting_row_count);
    static_assert(ctg::menu::shooting_target_size_row == 4 && ctg::menu::shooting_speed_row == 5 && ctg::menu::shooting_shots_row == 6);
    static_assert(ctg::menu::shooting_row_count == 7);
    static_assert(!ctg::menu::launch_layout(ctg::menu::shooting_row_count, ctg::menu::shooting_shots_row).scroll);
    // Compare complete selected option rows with independently positioned glyphs and arrows.
    // Exercise each viewport, feature/lock combination, value, and monochrome outline.
    for (unsigned id : {0u, 4u, 14u, 15u}) for (bool original : {false, true})
    for (bool locked : {false, true}) for (bool mono : {false, true})
    for (unsigned choice = 2; choice < ctg::menu::option_count(id); ++choice) {
        if (!ctg::menu::row_visible(id, choice, original, locked)) continue;
        for (unsigned v = 0; v < 60; ++v) {
            fixtures::Case c{id, fixtures::launch, choice, fixtures::plain};
            fixtures::Fixture f(c);
            f.modes[id < 14 ? id : 0] = v % 4;
            ctg::RenderState state{id, false, false, false, false, false,
                choice, choice, choice, choice, bool(v % 2), {bool(v % 2), bool(v % 2)}, 30, 30,
                v % 5, v % 4, v % 3, v % 3, v % 3, f.modes, f.tennis, f.demo_tennis,
                f.shooting, f.demo_shooting, f.racing, f.demo_racing, true, {}, true};
            state.racing_speed = v % 3;
            state.racing_more_cars = state.racing_wall_crash = state.racing_zigzag = state.racing_bad_road = v % 2;
            state.ice_hockey = v % 2;
            state.original_features = original;
            state.direct_launch_locked = locked;
            const char* label = nullptr;
            const char* value = nullptr;
            const char* modes[] = {"2 PLAYERS", "1P EASY", "1P NORMAL", "1P HARD"};
            const char* racing_modes[] = {"1P SPACIOUS", "1P NARROW", "2 PLAYERS"};
            const char* speeds[] = {"LOW", "MEDIUM", "HIGH"};
            const char* targets[] = {"MANUAL", "SLOW", "NORMAL", "FAST", "RANDOM"};
            const char* directions[] = {"DOWN", "UP", "RANDOM", "BOUNCE"};
            const char* sizes[] = {"LARGE", "SMALL", "RANDOM"};
            const char* shot_speeds[] = {"LOW", "HIGH", "RANDOM"};
            if (id == 15) {
                const char* labels[] = {"MODE", "SPEED", "BAD ROAD", "WALL CRASH", "MORE CARS", "CAR PATH"};
                label = labels[choice - 2];
                value = choice == 2 ? racing_modes[v % 3] : choice == 3 ? speeds[v % 3] :
                        choice == 7 ? (v % 2 ? "ZIGZAG" : "STRAIGHT") : (v % 2 ? "ON" : "OFF");
            } else if (id == 14) {
                const char* labels[] = {"TARGET MOVE", "DIRECTION", "TARGET SIZE", "SHOTS SPEED", "SHOTS"};
                label = labels[choice - 2];
                value = choice == 2 ? targets[v % 5] : choice == 3 ? directions[v % 4] :
                        choice == 4 ? sizes[v % 3] : choice == 5 ? shot_speeds[v % 3] : "30";
            } else {
                const char* labels[] = {"MODE", "SPEED", "P1 SIZE", "P2 SIZE", "SCORE", "ICE HOCKEY"};
                label = labels[choice - 2];
                value = choice == 2 ? modes[v % 4] : choice == 3 ? (v % 2 ? "HIGH" : "LOW") :
                        choice == 6 ? "30" : choice == 7 ? (v % 2 ? "ON" : "OFF") : (v % 2 ? "SMALL" : "LARGE");
            }
            if (75 + std::strlen(label) * 6 - 1 >= 151 || 163 + std::strlen(value) * 6 - 1 >= 234) {
                std::fprintf(stderr, "Option columns overlap: %s / %s\n", label, value);
                return 1;
            }
            const auto layout = ctg::menu::launch_layout(ctg::menu::visible_count(id, original, locked),
                ctg::menu::visible_index(id, choice, original, locked));
            const unsigned y = layout.y + (ctg::menu::visible_index(id, choice, original, locked) - layout.first) * layout.spacing;
            auto actual = std::make_unique<ctg::Renderer>();
            auto expected_row = std::make_unique<ctg::Graphics>();
            actual->render(state);
            expected_row->pixels.fill(0x295166);
            const auto ink = id == 14 && choice == 3 && v % 5 == 0 ? 0x888888u : 0x70e0ffu;
            expected_row->text(75, y + 3, label, ink);
            expected_row->horizontal_arrow(151, y + 6, true, ink);
            expected_row->text(163, y + 3, value, ink);
            expected_row->horizontal_arrow(236, y + 6, false, ink);
            if (mono) { actual->grayscale(); actual->monochrome_selection_outline(state); expected_row->grayscale(); }
            for (unsigned py = y + 2; py <= y + 10; ++py) for (unsigned x = 54; x < 266; ++x)
                if (actual->pixels[py * 320 + x] != expected_row->pixels[py * 320 + x]) {
                    std::fprintf(stderr, "Option row mismatch id=%u row=%u value=%u original=%d locked=%d mono=%d at %u,%u\n", id, choice, v, original, locked, mono, x, py);
                    return 1;
                }
        }
    }
    // The active MORE CARS row displays the selected OFF/ON value and arrows.
    {
        constexpr unsigned choice = ctg::menu::racing_opponents_row;
        fixtures::Case c{15, fixtures::launch, choice, fixtures::plain};
        fixtures::Fixture f(c);
        ctg::RenderState state{15, false, false, false, false, false,
            choice, choice, choice, choice, true, {true, false}, 21, 30,
            0, 0, 0, 0, 0, f.modes, f.tennis, f.demo_tennis, f.shooting,
            f.demo_shooting, f.racing, f.demo_racing, true, {}, true};
        const auto layout = ctg::menu::launch_layout(8, choice);
        const unsigned y = layout.y + (choice - layout.first) * layout.spacing;
        auto renderer = std::make_unique<ctg::Renderer>();
        auto expected_row = std::make_unique<ctg::Graphics>();
        for (bool enabled : {false, true}) {
            state.racing_more_cars = enabled;
            renderer->render(state);
            expected_row->pixels.fill(0x295166);
            expected_row->text(75, y + 3, "MORE CARS", 0x70e0ff);
            expected_row->horizontal_arrow(151, y + 6, true, 0x70e0ff);
            expected_row->text(163, y + 3, enabled ? "ON" : "OFF", 0x70e0ff);
            expected_row->horizontal_arrow(236, y + 6, false, 0x70e0ff);
            for (unsigned py = y + 2; py < y + 11; ++py)
                for (unsigned px = 75; px < 241; ++px)
                    if (renderer->pixels[py * ctg::Graphics::width + px] !=
                        expected_row->pixels[py * ctg::Graphics::width + px]) {
                        std::fprintf(stderr, "MORE CARS row mismatch enabled=%d x=%u y=%u\n", enabled, px, py);
                        return 1;
                    }
        }
    }

    // WALL CRASH shows its value and arrows in both selected and unselected colors.
    for (bool selected : {false, true}) for (bool enabled : {false, true}) {
        const unsigned choice = selected ? ctg::menu::racing_wall_crash_row : ctg::menu::racing_opponents_row;
        fixtures::Case c{15, fixtures::launch, choice, fixtures::plain};
        fixtures::Fixture f(c);
        ctg::RenderState state{15, false, false, false, false, false,
            choice, choice, choice, choice, true, {true, false}, 21, 30,
            0, 0, 0, 0, 0, f.modes, f.tennis, f.demo_tennis, f.shooting,
            f.demo_shooting, f.racing, f.demo_racing, true, {}, true};
        state.racing_wall_crash = enabled;
        const auto layout = ctg::menu::launch_layout(8, choice);
        const unsigned y = layout.y + (ctg::menu::racing_wall_crash_row - layout.first) * layout.spacing;
        const std::uint32_t color = selected ? 0x70e0ff : 0xffffff;
        auto renderer = std::make_unique<ctg::Renderer>();
        auto expected_row = std::make_unique<ctg::Graphics>();
        renderer->render(state);
        // Compare glyph and arrow pixels against an independently drawn mask.
        expected_row->pixels.fill(0);
        expected_row->text(75, y + 3, "WALL CRASH", color);
        expected_row->horizontal_arrow(151, y + 6, true, color);
        expected_row->text(163, y + 3, enabled ? "ON" : "OFF", color);
        expected_row->horizontal_arrow(236, y + 6, false, color);
        for (unsigned py = y + 2; py < y + 11; ++py)
            for (unsigned px = 75; px < 241; ++px)
                if (expected_row->pixels[py * ctg::Graphics::width + px] == color &&
                    renderer->pixels[py * ctg::Graphics::width + px] != color) {
                    std::fprintf(stderr, "WALL CRASH row mismatch selected=%d enabled=%d x=%u y=%u\n", selected, enabled, px, py);
                    return 1;
                }
    }

    // Road pixels and simulation use the same world-space dirt boundary.
    {
        fixtures::Case c{15, fixtures::launch, 1, fixtures::plain};
        fixtures::Fixture f(c);
        f.racing.set_bad_road(true);
        auto& road = const_cast<ctg::RacingState&>(f.racing.state());
        while (!(ctg::racing_dirt_at(road, 100) && !ctg::racing_dirt_at(road, 120))) ++road.road_position;
        auto renderer = std::make_unique<ctg::Renderer>();
        renderer->render_racing(f.racing);
        if (renderer->pixels[100 * 320 + 70] != 0x965b2c || renderer->pixels[120 * 320 + 70] != 0x39b800) {
            std::fprintf(stderr, "BAD ROAD pixels disagree with dirt geometry: %06x %06x\n", renderer->pixels[100 * 320 + 70], renderer->pixels[120 * 320 + 70]);
            return 1;
        }
    }
    // BAD ROAD is an active switch, including its selected value and arrows.
    for (unsigned choice : {4u}) {
        fixtures::Case c{15, fixtures::launch, choice, fixtures::plain};
        fixtures::Fixture f(c);
        const ctg::RenderState state{15, false, false, false, false, false,
            choice, choice, choice, choice, true, {true, false}, 21, 30,
            0, 0, 0, 0, 0, f.modes, f.tennis, f.demo_tennis, f.shooting,
            f.demo_shooting, f.racing, f.demo_racing, true, {}, true};
        auto renderer = std::make_unique<ctg::Renderer>();
        renderer->render(state);
        const auto layout = ctg::menu::launch_layout(8, choice);
        const unsigned y = layout.y + (choice - layout.first) * layout.spacing + 3;
        const auto contains_gray_text = [&](const char* label) {
            auto mask = std::make_unique<ctg::Graphics>();
            mask->text(0, 0, label, 0xffffff);
            const unsigned width = static_cast<unsigned>(std::strlen(label)) * 6 - 1;
            for (unsigned x = 53; x + width <= 267; ++x) {
                std::uint32_t color = 0;
                bool matches = true;
                for (unsigned py = 0; py < 7 && matches; ++py)
                    for (unsigned px = 0; px < width; ++px) {
                        if (mask->pixels[py * ctg::Graphics::width + px] != 0xffffff) continue;
                        const auto pixel = renderer->pixels[(y + py) * ctg::Graphics::width + x + px];
                        if (!color) color = pixel;
                        if (pixel != color) { matches = false; break; }
                    }
                if (matches && color == 0x70e0ff) return true;
            }
            return false;
        };
        if (!contains_gray_text("BAD ROAD") || !contains_gray_text("OFF")) {
            std::fprintf(stderr, "Racing row %u missing selected BAD ROAD or OFF value\n", choice);
            return 1;
        }
    }
    // Every scroll position has a distinct selection and viewport.
    for (unsigned row = 0; row < 8; ++row) {
        const fixtures::Case current{15, fixtures::launch, row, fixtures::plain};
        for (unsigned previous = 0; previous < row; ++previous) {
            const fixtures::Case other{15, fixtures::launch, previous, fixtures::plain};
            if (draw(current) == draw(other)) {
                std::fprintf(stderr, "Racing menu rows %u and %u rendered identically\n", row, previous);
                return 1;
            }
        }
    }
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
    // Equal counts catch missing entries; changing case order still requires
    // updating the corresponding table entries and reviewing rendered output.
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
