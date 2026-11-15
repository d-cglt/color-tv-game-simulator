# 0.1.5 release notes

These notes describe the prepared 0.1.5 release and changes since 0.1.4; they do not assert that a release has been published.

## Changes since 0.1.4

- Revises rally-field presentation around a provisional 41 × 50 grid, including walls, center markings, paddles and shared 4 × 8 score digits. Shooting and Racing also use the revised score glyphs. Some game geometry and collision behavior have been adjusted, including Hockey doubles inner-paddle contacts and opponent targeting.
- Adds a diagnostic **TEST MENU** for visual comparison. Hold RetroPad **L2 + R2** and press **Start** from the selector, game menu or gameplay. Use Up/Down to select, Left/Right to change settings, and B or **EXIT** to return. The menu pauses the underlying activity and silences audio while open.
- **GRID** overlays the provisional rally grid and coordinates, including on menus; in Shooting and Racing it still uses rally-field coordinates. **FIXED SCORE** displays selected left and right values from 0 to 30 and prevents games in progress from ending normally. Racing displays the left value only. It does not restart a game already finished. These diagnostic settings persist for the current core session and reset on core reinitialization; turn fixed score off for normal match endings.
- A separate `colortvgame-0.1.5-selectors.zip` archive contains 16 `.ctg` selectors. These select built-in games and are not ROMs. A versioned selector archive accompanies each release, even when its contents are unchanged. Core packages and CI artifacts do not include selectors. Alternatively, generate them through the core option; see the [README](https://github.com/d-cglt/color-tv-game-simulator/blob/62230a7/README.md#generate-individual-game-files).

## Platform packages

- Linux x86-64: `colortvgame_libretro.so`; the Linux package includes licenses and [instructions](https://github.com/d-cglt/color-tv-game-simulator/blob/62230a7/docs/LINUX_RELEASE.md).
- Windows x86-64: `colortvgame_libretro.dll`.
- macOS Intel (x86-64): `colortvgame_libretro.dylib`; Apple Silicon is not built by the workflow.

The [build workflow](https://github.com/d-cglt/color-tv-game-simulator/blob/62230a7/.github/workflows/build.yml) built and tested all three targets and checked the macOS architecture in [CI run 43690015029](https://github.com/d-cglt/color-tv-game-simulator/actions/runs/43690015029), at commit `62230a7`. The prepared packages use those CI binaries and include license notices and usage instructions.

## Validation and limitations

The local Linux Release suite passed 22 tests; the three CI builds and their automated tests also passed. Automated tests use a simulated frontend and do not establish real-frontend compatibility. The refactored Linux core was tested in RetroArch for selectors and menus, a paddle game, Shooting, Racing, sound, controller input and settings persistence. Grid alignment, paddle limits and Hockey doubles back-side rebound were also checked. These checks did not validate the packaged CI binary.

An earlier Windows build was interactively checked in Windows RetroArch under Wine, not native Windows; the latest Windows binary has not been tested in a real frontend. An earlier macOS Intel core returned metadata under Darling, but gameplay could not be verified there. Native macOS frontend testing remains unverified. Windows and macOS remain experimental, and macOS may require permission to load an unsigned library.

The grid and score patterns are informed by footage and user observations, not verified hardware specifications. Rendering, geometry, physics, AI and Racing remain independently implemented approximations; the 15 provisional variants and one-player Racing prototype do not establish original hardware behavior or all 112 Racing configurations. See [research](https://github.com/d-cglt/color-tv-game-simulator/blob/62230a7/docs/RESEARCH.md) and [Racing design](https://github.com/d-cglt/color-tv-game-simulator/blob/62230a7/docs/RACING_DESIGN.md). The frontend must support XRGB8888; save states and rewind are unsupported.
