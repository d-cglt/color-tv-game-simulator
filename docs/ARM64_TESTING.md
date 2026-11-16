# Experimental Linux ARM64 volunteer test

This is an experimental CI build, not a confirmed Raspberry Pi, Recalbox or RetroPie release. The core was built and its automated tests passed on a GitHub-hosted Ubuntu ARM64 runner; it has not been validated on a Raspberry Pi or in those frontends. Testing does not require any ROMs.

## What to use

A Raspberry Pi 4 or 5 running a **64-bit** Linux system and 64-bit RetroArch could be a useful starting point. These are suggestions, not a list of supported or minimum models; other ARM64 devices and configurations are welcome. A 32-bit OS or frontend cannot load this ARM64 core, even on a 64-bit processor. Recalbox and RetroPie configurations may differ; do not assume either can load a custom core without additional steps.

The package is based on [CI run 43856733238](https://github.com/d-cglt/color-tv-game-simulator/actions/runs/43856733238), commit `0f35ad16e57235677058a3ab7c94f5673b042407`. The core is an AArch64 ELF shared library, built on Ubuntu 24.04 ARM64. Its dynamic symbol requirements include `GLIBC_2.17`, `GLIBCXX_3.4.26` and `CXXABI_1.3.9`; compatible ARM64 runtime libraries are necessary but do not guarantee compatibility. The core and frontend architectures must match, and the frontend must support XRGB8888 video.

## Try the core

1. Back up frontend configuration and saves if needed. Use a test environment; avoid replacing an existing installed core. Frontend operations can update configuration, content history or other files.
2. Extract the package. In a frontend that supports custom cores, use RetroArch's **Load Core** to select `colortvgame_libretro.so`. Custom-core installation and permissions vary by distribution; do not overwrite system files or change permissions unless you understand the distribution's instructions.
3. Start the core without content to open its built-in game selector. The package does not contain `.ctg` selectors; they are optional, not ROMs. If direct-content testing is desired, use the separate selector archive from a release or generate selectors through the core option.
4. Record whether each check passes, fails or was not tested:
   - Core loads and identifies itself; the no-content selector opens.
   - Menus display correctly and respond to controller or keyboard input.
   - A paddle game starts and plays; controls, video and sound work.
   - Shooting and the Racing prototype start and play.
   - Pause, resume, restart and exit work without crashing.
   - If feasible, change a game setting, exit the frontend, reopen it and check whether it persists.
   - If using selectors, load one directly and confirm it opens the expected game.
5. If any check fails, record the exact error and relevant frontend logs. Stop rather than changing system files to force a test.

## Report

Please include the device model and RAM, OS and version, 64-bit OS/frontend confirmation, frontend name and version, whether it is a standalone RetroArch, Recalbox, RetroPie or another setup, the package or CI run tested, and results for each check. Include display/audio and controller details if relevant. Share logs only after checking them for private paths or other personal information. Avoid sharing save files or private recordings.

A successful result applies only to the tested configuration; broader support requires additional testing.
