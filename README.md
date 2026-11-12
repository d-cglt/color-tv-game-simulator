# Color TV-Game Simulator

Color TV-Game Simulator is a libretro core with a `.ctg` content loader and a launch menu showing the selected game's provisional title. The 15 provisional Color TV-Game 6/15 variants (IDs 0–14) have playable implementations. A first Racing 112 implementation is in progress as a single additional console entry, ID 15: its scope is one player on a wide road, with approximate gameplay, not all 112 combinations. Racing is integrated into the selector and direct-content launch; the user confirmed Racing play and steering in a frontend. Physics and presentation are approximations, not verified original hardware behavior. In Racing, L2/LT selects speed 1 and R2/RT selects speed 2; the selected speed persists after release. The prototype automatically speeds up after startup and impacts. These mappings and speed transitions are approximations of a documented two-position gear lever, not verified original hardware behavior. Historical details and game assignments remain unverified.

Public labels use **Original** for former B IDs 2, 3, 6, 7, 10 and 11, and **Variant** for former A IDs 0, 1, 4, 5, 8 and 9. IDs, gameplay, selector order and suffixes are unchanged. Original is a project label, not proof of historical primacy or CTG-6 availability; the `Color TV Game 6 + 15` suffix remains an unverified hypothesis. See [content format](docs/CONTENT_FORMAT.md) for the complete mapping.

The core implementation and headless tests build and pass on Linux. A bounded RetroArch 1.22.2 smoke test loaded ID 2, initialized video and audio, ran 120 frames and unloaded successfully. In a separate launcher session, a user played ID 2 (Tennis Original Singles) in RetroArch with an Xbox controller for player 1 and keyboard arrows for player 2; the match reached 15 and displayed the win screen. Restart and play with two separate controllers were not confirmed. ID 2 uses approximate procedural sound. By design, the core does not support save states or rewind and requires frontend support for XRGB8888 video.

**Disclaimer:** Color TV-Game Simulator is an independently developed project. It is not affiliated with, endorsed by, or sponsored by Nintendo.

## License

The project is licensed under the [MIT License](LICENSE). The vendored Libretro API header has its own copyright and MIT license notice in `third_party/libretro/libretro.h`; retain that notice when distributing it or applicable portions of the software.

## Relationship to the reference simulator

This project was originally conceived as a port of [Thomas Visvader's Python simulator](https://github.com/ThomasVisvader/Pong/tree/e654c5ff909851dac9360000c2c269562cf0c735/Nintendo%20Color%20TV-Game%2015). Because no applicable license was identified for that repository, and to support planned improvements and additional features, it evolved into an independently developed implementation written from scratch. Thomas's simulator, including its Python source code, was examined as a behavioral reference, not used as a source-code dependency: none of its Python source code has been copied, translated, ported, or incorporated into this core.

Racing 112 follows the same research-driven approach, using original-hardware footage, electronic schematics, and historical documentation to inform an independent implementation. The unresolved reuse permissions concern the external Python repository, not this core's MIT license. See [Research](docs/RESEARCH.md#reference-repository-and-licensing) for the sources and licensing findings.

## Game selection at startup

The **Selected game at start** core option controls the initial highlight when starting without content: **Last used** (default), **First**, or **Random**. Last used remembers the last game highlighted while navigating the selector, without requiring confirmation; games opened directly as `.ctg` files do not change it. The core stores that selection in `colortvgame-last-game` under the frontend's save directory, so it can survive a full restart if the directory is available and writable. If it cannot be read, the selector starts on the first game; if saving fails, the core reports a message. Direct `.ctg` launches always open their specified game.

## Rebound behavior

The **Rebound behavior** core option selects an approximate paddle rebound model for competitive games and their demos. **Hit position** is the default and retains the existing behavior; **Paddle movement** instead uses each player's last actual paddle movement, even after stopping, to select the vertical rebound direction; contact position selects its magnitude. Before a paddle has moved, it falls back to hit-position behavior. Movement history resets when the match is reset. Changes apply during play and previews. The original hardware's behavior is uncertain, so neither model should be considered verified; the movement-based model can also produce long, nearly horizontal rallies.

## Vibration

The **Vibration** core option defaults to **Off**. When enabled, a supported controller briefly vibrates when its player's paddle hits the ball or when the player fires in Shooting. Automatic serves do not trigger vibration. AI-controlled players and menu demos do not trigger vibration. This is a modern convenience, not a feature of the original controllers; it requires frontend and controller rumble support. The user confirmed vibration works with a controller in RetroArch; the Off default has not been separately confirmed there.

## Anti-stalemate

The **Anti-stalemate** core option is **On** by default. On the third consecutive horizontal paddle rebound, it adds a small vertical angle at the hit to help break a stalemate; there is no timer. It applies to competitive games and their demos, and changes take effect during play and previews. Set it to **Off** to disable the intervention. This is a modern approximation, not proven behavior of the original hardware.

## Display

The **Color** core option is On by default. Set it to **Off** for a grayscale picture resembling a black-and-white TV. It applies to the entire core image, including game menus and demos; RetroArch's own interface is unaffected. Changes take effect when the core resumes. The optional **RF filter** is Off by default. When enabled, it applies a provisional NTSC-like color and bandwidth effect to the core image before grayscale conversion. **RF signal quality** ranges from 100% (default) to 0% in 10% steps; lower values add color bleed and moving noise. It has no effect while the RF filter is Off. This is a visual approximation, not a verified model of the console's RF signal.

## Attract/demo mode

The game selector and each game's launch menu show an animated, scoreless preview of the selected game. Competitive previews alternate both players between Hard and Easy AI every three seconds, starting with Hard; Shooting uses its own automated preview. After 10 seconds without input, the menu disappears, a demo prompt appears at the top, and the game title and variant appear at the bottom. Input restores the same menu without navigating, changing a setting, or starting a game; a subsequent input can be used normally. Preview activity does not change the settings, score, or initial state of a real game. The **Demo sounds** core option is On by default and can be switched Off; changes take effect when the core resumes. Preview sounds are independent of gameplay audio. **Demo game rotation** defaults to **Random every 15 seconds**, choosing a different game every 15 seconds while the selector menu is hidden. Choose **Every 15 seconds** for sequential rotation or **Off** to disable it. A previously saved setting may override the new default. Input stops the rotation and restores the menu on the displayed game; the menu hides again after 10 seconds of inactivity, and rotation resumes 15 seconds later. Automatic rotation does not change the saved last selection, and individual game launch menus do not rotate. The user confirmed demo rotation, the hidden-screen title, wake behavior and last-selection preservation in RetroArch.

## Build and test

Install CMake 3.16 or newer and a C++17 compiler, then from the repository root run:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

On Linux the shared core is `build/colortvgame_libretro.so`; the extension and output location can differ by platform or build configuration. For multi-configuration generators such as Visual Studio, use `cmake --build build --config Release` and `ctest --test-dir build -C Release --output-on-failure`. Tests use a simulated frontend, not RetroArch.

## GitHub Actions builds

The [build workflow](.github/workflows/build.yml) is configured to build Release and run the applicable CTest suite on GitHub-hosted Linux x86-64, Windows x86-64 and macOS x86-64 runners. The macOS job requests x86-64 and checks the built Mach-O architecture with `lipo`. Successful jobs upload separately named artifacts containing the core, both license notices and [usage instructions](docs/CI_ARTIFACT.md). The Linux-only package test and Unix launcher test run only on their applicable platforms.

[GitHub Actions run 42813010356](https://github.com/d-cglt/color-tv-game-simulator/actions/runs/42813010356) succeeded on all three runners and uploaded their artifacts, including a macOS x86-64 architecture check. That run predates the 0.1.1 source changes; runner images and availability can change. CI artifacts are not the packaged Linux release and have not themselves been validated in a real frontend. Windows x86-64 and macOS Intel remain experimental targets for 0.1.3; frontend compatibility, runtime requirements and distribution readiness remain unverified. Apple Silicon is not built; matching architecture alone does not guarantee that a frontend can load the core.

## Linux release package

Versions **0.1.0**, **0.1.1**, and **0.1.2** have been released; the current source is version **0.1.3**. See the [0.1.3 release notes](docs/RELEASE_NOTES_0.1.3.md). This does not imply a 0.1.3 package has been published or frontend-tested. Build and test a Release configuration, then assemble the versioned package from that exact core:

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
ctest --test-dir build-release --output-on-failure
sh scripts/package-linux.sh build-release/colortvgame_libretro.so build-release/colortvgame-0.1.3-linux.tar.gz
```

The archive contains the Color TV-Game Simulator core, project license, vendored libretro header's MIT notice, and [end-user instructions](docs/LINUX_RELEASE.md). The script packages the supplied binary as-is; build it on an appropriate Linux target and verify runtime compatibility for distribution. The CMake project version controls the version reported by the core; choose an archive name matching the built binary, since the script does not verify its version or revision. It does not guarantee byte-identical archives or bundle RetroArch, metadata, launchers, or content. CTest assembles and extracts a package, checks its payload and notices, and dynamically loads the packaged core; this is not a substitute for frontend validation.

## Linux launch commands

Build the core as above. To open the game selector without a content file, run `./scripts/no-content.sh`. Alternatively, invoke a named game through the launcher:

```sh
./scripts/launch.sh tennis-original-singles
./scripts/launch.sh volleyball-variant-doubles
```

Use `tennis`, `hockey`, or `volleyball`, followed by `original` or `variant`, and `singles` or `doubles`. Other names are `ping-pong-singles`, `ping-pong-doubles`, and `shooting-game-original`. Existing A/B names and individual scripts remain supported: A selects Variant and B selects Original (for example, `./scripts/tennis-b-singles.sh` selects ID 2). All selectors open playable implementations; fidelity and interactive validation remain in progress. The launcher prefers `retroarch` on `PATH`, otherwise it tries the RetroDECK Flatpak (`net.retrodeck.retrodeck`). Set `CTG_FRONTEND=native` or `CTG_FRONTEND=retrodeck` to force a choice. RetroDECK runs its bundled `retroarch` with temporary read-only filesystem access to the project; no persistent Flatpak permissions or frontend configuration are changed. Flatpak must be installed and RetroDECK must include its RetroArch executable at `/app/retrodeck/components/retroarch/bin/retroarch`. If the sandbox or installation cannot load the core, use native RetroArch instead. Paths with spaces are supported; Flatpak filesystem syntax does not support a colon in the project path. The launcher expects `build/colortvgame_libretro.so` and reports missing files or frontends.

The project launchers append `scripts/launcher-input.cfg` for that RetroArch session only: player 1 can use an assigned gamepad (including Xbox-compatible controllers), while keyboard Up/Down control player 2. Keyboard Up/Down are unbound for player 1 to prevent overlap. The override does not change joypad assignments or other gamepad bindings; those remain subject to your RetroArch configuration. It also disables rewind, automatic state saving/loading, and saving frontend settings on exit for the launched session, so changes made in RetroArch during that session will not be saved to its main configuration. When loading the core manually, the frontend retains its own settings, but the core reports that serialization is unsupported. The launchers do not modify global configuration. Manually loading the core and content without the launcher does not apply these settings.

## Generate individual game files

Start the core, open RetroArch's **Quick Menu → Core Options**, change **Generate game files** from **Ready** to **Generate**, then resume the core. The updated generator targets 16 five-byte `.ctg` files under `<frontend system directory>/Nintendo Color TV-Game/`: the existing 15 and one Racing 112 console descriptor (ID 15). The core reports created, existing and failed counts. The directory depends on frontend configuration and sandbox permissions and must be available and writable; there is no fallback. Files in the former `system/ctggames` and save-directory locations are not copied or moved. Generated files can be moved and used like traditional ROM files. The generator uses the updated labels, for example `Tennis Variant - Singles (Color TV Game 15).ctg` for ID 0 and `Tennis Original - Singles (Color TV Game 6 + 15).ctg` for ID 2. Shooting Game is generated as `Shooting Game (Color TV Game 15).ctg`; these availability assignments are not historically verified. Existing files are never overwritten, and previously generated files retain their old names and encoded IDs; they are not removed or renamed. Older A/B filenames from both before and during the temporary inversion remain untouched; their labels can refer to different IDs, so inspect the encoded ID rather than relying on the name. Existing bundled slot files are unchanged; `content/slot-15.ctg` is the new Racing descriptor. It identifies the console entry, not 112 separate configurations. Released older cores reject ID 15; use a core with Racing integration, such as 0.1.3. IDs 0–14, their descriptors and existing settings retain their meanings. A file that could not be completely written may remain partial and must be removed manually before retrying. After generation the core asks the frontend to reset the option to **Ready**. If the frontend cannot do so, switch it to **Ready** manually before generating again. A saved **Generate** value does not trigger generation at startup; change it to **Ready** and back to **Generate**.

## Try in RetroArch

ID 2 (Tennis Original Singles) has been played through to the win screen using the launcher configuration described above. The following manual loading and configuration steps, including restart, have not been confirmed by that session:

1. In RetroArch, use **Load Core** to select the built shared library.
2. Use **Load Content** to open a bundled file such as `content/slot-00.ctg`, or start the core without content to choose a game in its selector. No-content launches have automated coverage and were used in the user's settings-persistence check.
3. Navigate the game list and select a game. For the existing rally games, use the inline **SPEED**, **P1 SIZE**, **P2 SIZE**, and competitive-game **SCORE** rows to change settings with left/right, then choose **START**. The winning score can be 5, 10, 15, 20, 25 or 30, with a default of 15. Shooting has an independent SHOTS setting with the same choices and default. Low speed and large paddles are the defaults; game-menu settings are saved as described below. For two-player games, the MODE row selects 2 PLAYERS or 1 PLAYER with EASY, NORMAL, or HARD computer control. During an active rally, Normal switches to Easy after 15 seconds. Hard starts with 30 seconds of Hard, then alternates 15 seconds of Normal and 15 seconds of Easy until the point ends. The selected difficulty returns on the next point; pauses and serve waits do not count. Hockey's Easy AI has extra targeting error to account for its narrow goals, including when another mode falls back to Easy. Opponent selections are remembered per game; Shooting has separate target controls. During gameplay, RetroPad **Start** or keyboard **P** opens a pause menu with **RESUME**, **RESTART** and **EXIT**. RetroArch Reset returns to the game's launch menu. In ID 2's Tennis Original Singles, each player's RetroPad directions or left stick moves their paddle; the first to the selected score wins. Physical key bindings and frontend hotkeys depend on frontend configuration.

Game-menu preferences persist across reset, exit and subsequent launches, whether starting directly with a `.ctg` file or without content. This includes speed, paddle sizes, score and shot limits, per-game opponent mode, and Shooting target, direction and blinking choices. A Random preference is saved as Random, not as a particular randomly resolved value. Persistence depends on the frontend providing an available, writable save directory; a failed save is reported and values remain in memory for the current session. The user confirmed persistence in RetroArch across reset, full exits and direct `.ctg` / no-content launches. These preferences are separate from frontend-managed core options and from the last-game selection described above; they are not save states, which remain unsupported.

For Shooting Game, the launch menu defaults to manual target control. Select the **TARGET** row in the game menu and use left/right to switch between manual P2 and the reference-inspired automatic target. Move the shooter with P1's left stick or D-pad or keyboard W/S; fire with RetroPad A/Y or keyboard X/Space. In manual mode, P2 controls the target with its left stick or D-pad (the project launcher maps keyboard Up/Down to P2). Manual target control follows the manual's broad description, but its precise behavior is not hardware-verified.

Shooting settings include **TARGET** choices **Manual**, **Slow**, **Normal**, **Fast**, and **Random**, with a separate direction setting (**Down**, **Up**, or **Random**) grayed out in Manual mode and a blinking setting (**Off**, **On**, or **Random**). Defaults are Manual, Down, and On. Random choices are resolved at match start or restart and rerolled after each successful target hit, not on misses or every frame; fixed choices remain unchanged. The demo is unaffected. With blinking Off, targets are continuously hittable. Slow and Fast are approximate speeds, not historically verified.

If loading fails, check that the frontend accepts XRGB8888 and that the core and frontend architectures match. The `.ctg` files are selectors, not game ROMs; game logic is supplied by the core.

## Documents

- [Roadmap](docs/ROADMAP.md) — implementation status, frontend validation, and milestones.
- [Racing design](docs/RACING_DESIGN.md) — first-playable scope, compatibility and future plans.
- [0.1.3 release notes](docs/RELEASE_NOTES_0.1.3.md) — changes and validation limits.
- [Content format](docs/CONTENT_FORMAT.md) — five-byte format and provisional slots.
- [Core information](docs/CORE_INFO.md) — optional frontend metadata, installation, and validation limits.
- [Research](docs/RESEARCH.md) — preliminary sources and unresolved evidence and licensing questions.
- [Future development](docs/FUTURE_DEVELOPMENT.md) — additional proposed work.
