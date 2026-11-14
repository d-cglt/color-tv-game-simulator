# 0.1.5 release notes

These notes describe the planned 0.1.5 release and the current working-tree changes since 0.1.4; they do not assert that a release or binaries have been published.

## Changes since 0.1.4

- Revises rally-field presentation around a provisional 41 × 50 grid, including walls, center markings, paddles and shared 4 × 8 score digits. Shooting and Racing also use the revised score glyphs. Some game geometry and collision behavior have been adjusted, including Hockey doubles inner-paddle contacts and opponent targeting.
- Adds a diagnostic **TEST MENU** for visual comparison. Hold RetroPad **L2 + R2** and press **Start** from the selector, game menu or gameplay. Use Up/Down to select, Left/Right to change settings, and B or **EXIT** to return. The menu pauses the underlying activity and silences audio while open.
- **GRID** overlays the provisional rally grid and coordinates, including on menus; in Shooting and Racing it still uses rally-field coordinates. **FIXED SCORE** displays selected left and right values from 0 to 30 and prevents games in progress from ending normally. Racing displays the left value only. It does not restart a game already finished. These diagnostic settings persist for the current core session and reset on core reinitialization; turn fixed score off for normal match endings.
- The release is planned to include a separate `colortvgame-0.1.5-selectors.zip` archive with 16 `.ctg` selectors. These select built-in games and are not ROMs. A versioned selector archive is to accompany every release, even when its contents are unchanged. Core packages and CI artifacts do not include selectors. Alternatively, generate them through the core option; see the [README](../README.md#generate-individual-game-files).

## Intended platform targets

- Linux x86-64: `colortvgame_libretro.so`; the Linux package includes licenses and [instructions](LINUX_RELEASE.md).
- Windows x86-64: `colortvgame_libretro.dll`.
- macOS Intel (x86-64): `colortvgame_libretro.dylib`; Apple Silicon is not built by the workflow.

The [build workflow](../.github/workflows/build.yml) is configured to build and test these targets and check the macOS architecture. Its artifacts include the core, license notices and [usage instructions](CI_ARTIFACT.md). Confirm actual builds and availability separately; no 0.1.5 artifacts or release availability are implied here.

## Validation and limitations

A local Linux Release build and CTest run of the current working tree passed all 15 tests, including diagnostic menu, rendering, selector and package tests. Tests use a simulated frontend; the package test loads the packaged core but is not a real-frontend test. The user confirmed the current grid alignment, paddle limits and Hockey doubles back-side rebound in RetroArch; these checks do not validate a packaged 0.1.5 binary. Earlier checks of steering, vibration and generator behavior also do not validate the package. Windows and macOS builds and real-frontend operation for these changes are unverified; those platforms remain experimental, and macOS may require permission to load an unsigned library.

The grid and score patterns are informed by footage and user observations, not verified hardware specifications. Rendering, geometry, physics, AI and Racing remain independently implemented approximations; the 15 provisional variants and one-player Racing prototype do not establish original hardware behavior or all 112 Racing configurations. See [research](RESEARCH.md) and [Racing design](RACING_DESIGN.md). The frontend must support XRGB8888; save states and rewind are unsupported.
