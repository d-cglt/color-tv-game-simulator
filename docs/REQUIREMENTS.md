# Requirements and compatibility

This document describes known requirements and observations, **not minimum hardware specifications**. Performance, frontend support and library compatibility depend on the particular build and device. Investigation performed on 2026-11-19 against the working tree based on `f0baeca`, including uncommitted changes; released binaries may differ.

## Software requirements

- **Build:** CMake 3.16 or newer and a compiler and C++ standard library supporting **C++17**, including `std::filesystem`. CMake does not specify a minimum compiler version. A CMake-supported build tool is needed. The Libretro header is vendored; no separate Libretro library is required. Python 3 is optional for the selector archive and Python-based tests; the default test build also requires thread support. `-DBUILD_TESTING=OFF` builds without the test sources.
- **Runtime:** a compatible Libretro frontend capable of loading a core for the same operating system and CPU architecture/ABI, with compatible runtime libraries. The core requires **XRGB8888** video: it refuses to load if the frontend does not accept that pixel format. It reports 320 × 240 video at 60 Hz and 48 kHz audio. GPU, display, audio and input device requirements depend on the frontend.
- **Linux runtime example:** the locally built Fedora x86-64 core declares `libstdc++.so.6`, `libm.so.6`, `libgcc_s.so.1` and `libc.so.6` as dynamic dependencies. Exact library versions and compatibility depend on the build environment; this is not a universal baseline. The [experimental ARM64 test guide](ARM64_TESTING.md) records symbol requirements for one earlier Ubuntu-built artifact, not every ARM64 build.
- **Content and storage:** no ROM or BIOS is needed; the core can start without content. Optional `.ctg` files are game selectors. Persisting preferences requires a frontend-provided writable save directory; generating selectors requires a writable system directory. The frontend must expose the relevant core options to change them. Rumble is optional and requires frontend and controller support. Save states and rewind are unsupported.
- **Local launcher:** the supplied Linux launcher additionally needs a native RetroArch executable or its documented RetroDECK Flatpak setup, the built core and local metadata. These launcher requirements are not prerequisites for loading a core directly in a compatible frontend.

## What has been tested

| Configuration | Evidence | Limitations |
| --- | --- | --- |
| Linux x86-64 | Local Release automated tests and interactive RetroArch play have been reported; the published 0.1.7 Linux package was used for a 96-point Racing run. The measurements below use a simulated frontend. | These checks do not establish compatibility or performance on every Linux distribution or device. |
| Linux ARM64 | The GitHub Actions workflow runs a native Ubuntu 24.04 ARM64 build, CTest and an AArch64 ELF check. Release 0.1.7 passed its four-platform CI run ([44079062775](https://github.com/d-cglt/color-tv-game-simulator/actions/runs/44079062775)). | Experimental; no Raspberry Pi, RetroPie or Recalbox frontend or performance test has been reported. CI is not a device benchmark. |
| Windows x86-64; macOS Intel | Release 0.1.7 passed the configured CI builds and tests; CI also checks the macOS architecture. | Experimental; native frontend compatibility and performance are not established. Earlier Wine or Darling checks do not prove native compatibility. |
| Apple Silicon; x86 32-bit; ARM 32-bit | No corresponding build is configured in the current GitHub Actions matrix. | Build, tests, frontend compatibility and performance are unverified. See the 32-bit investigation below. |

The CI result above applies to the released commit, not automatically to the present uncommitted working tree. See [ARM64_TESTING.md](ARM64_TESTING.md) for a volunteer test checklist and [LIBRETRO_INTEGRATION.md](LIBRETRO_INTEGRATION.md) for buildbot limitations. The draft buildbot configuration does not establish Core Downloader availability.

## CPU and memory observations

On 2026-11-19, three runs per scenario were measured on an Intel Core i5-12450H (8 cores, 12 logical CPUs), 15.4 GiB RAM, Fedora 45 x86-64, Linux `7.3.4-300.fc45.x86_64`, glibc 2.44. The current modified source was built with GCC 16.2.1, C++17 and CMake Release (`-O3 -DNDEBUG`).

A temporary headless driver linked the built core and `tests/core_test_harness.cpp`. It loaded ID 2 (Tennis Original Singles, provisionally associated with Color TV-Game 6 and 15) or ID 15 (Racing 112 prototype), ran one menu frame, started the game, released Start, then ran 3,000 frames with alternating directional input. Color was on; RF was either off or on at 0% quality; demo rotation was off. Other settings were fresh-process defaults, without a save directory or rumble. Each run produced 3,003 video callbacks, 3,003 input polls and 2,402,400 audio samples per channel. `/usr/bin/time` measured the entire process, including the harness, frame copying and audio scanning.

| Scenario | Wall time, three runs | User CPU time | Peak process RSS |
| --- | --- | --- | --- |
| Tennis, RF off | 0.07–0.08 s | 0.06–0.07 s | 5,384–5,584 KiB |
| Tennis, RF on, 0% quality | 2.48–2.53 s | 2.47–2.50 s | 5,384–5,456 KiB |
| Racing, RF off | 0.07–0.08 s | 0.07–0.08 s | 5,432–5,712 KiB |
| Racing, RF on, 0% quality | 2.51–2.55 s | 2.48–2.53 s | 5,384–5,584 KiB |

These were **unpaced, as-fast-as-possible** runs, not real-time CPU utilization. The RF-on runs processed approximately 50 nominal seconds of frames in roughly 2.5 seconds on this machine; RF-off timings are especially sensitive to the timer's 0.01-second resolution. RSS is the whole process peak, not core-only memory. The driver checked that Start changed the frame but did not independently track every gameplay state; Tennis might have reached a result screen. CPU frequency and other system activity were uncontrolled. These numbers do not cover all filter qualities, frontend overhead, GPU/display/audio devices, long sessions, power use, other games or slower hardware. They do not establish a minimum CPU or RAM requirement.

## 32-bit investigation

Neither 32-bit target has been validated. The current source uses fixed-width pixel and serialized-data types in relevant paths, and inspection found no architecture-specific assembly or pointer-to-integer casts, but that is not a complete portability audit. The frontend and core must match in architecture and ABI; a 64-bit ARM core cannot be loaded by a 32-bit frontend. Differences in floating-point behavior, filesystem support and performance still need tests on each target.

- **x86 (32-bit):** the installed GCC advertises `-m32`, but a C++17 `std::filesystem` compile-and-link probe failed because the linker could not find `/usr/lib/libatomic.so.1.2.0`. A 32-bit C++ runtime/loader was present, but the multilib setup was incomplete. No project build, tests or 32-bit frontend run was completed.
- **ARM (32-bit):** no `arm-linux-gnueabihf-g++` or `arm-linux-gnueabi-g++` cross-compiler was available. Clang had an ARM backend, but an ARM C++17 probe could not find the `<filesystem>` header: no suitable target C++ toolchain and sysroot were found. No ARM32 project build, tests, frontend run or physical-device test was completed. Installed QEMU user emulators do not provide the missing toolchain or demonstrate real-device performance.

## What can be communicated to RetroPie

An **experimental Linux ARM64 build** has passed native ARM64 CI build and automated tests, but RetroPie support, installation, frontend loading, controls, audio and performance are **not confirmed**. RetroPie installations can use different OS architectures and frontend builds; identify the actual frontend ABI and compatible runtime libraries before selecting or building a core. The existing ARM64 artifact cannot serve an ARM32 frontend. No minimum Raspberry Pi model or specification can be claimed from the present evidence.

Suggested next tests: obtain a suitable ARM32 toolchain/sysroot and run a clean build and applicable tests; independently test ARM64 and ARM32 on real devices with matching frontend architectures. For each, record device/model, OS and frontend versions and ABI, core commit/build, load and no-content startup, representative paddle/Shooting/Racing play, controls, sound, video, persistence and direct selectors. Measure frame timing, CPU and memory with RF off and on, including the frontend, and report failures or untested steps rather than extrapolating. A Raspberry Pi 4 or 5 is a possible ARM64 starting point, not a supported minimum.
