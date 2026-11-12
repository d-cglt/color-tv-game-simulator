# 0.1.4 release notes

This release fixes game-file generation and provides a separate archive of ready-to-use selectors.

## Changes since 0.1.3

- Fixes **Generate game files** option detection when a frontend does not support variable-update notifications or returns a changed value without signaling an update. The core checks the option value directly and generates on a Ready-to-Generate transition, rather than repeatedly generating while it remains selected.
- Provides a separate, platform-independent ZIP containing 16 generated, descriptively named five-byte `.ctg` selectors for IDs 0–15 and usage instructions. These are selectors for games built into the core, not ROMs; they are not included in the core archives. Extract the archive, load the core in a compatible frontend, then load a selector as content. Starting without content opens the built-in selector instead.
- IDs 0–14 retain their meanings. ID 15 selects the one-player Racing 112 prototype, not 112 configurations; it requires a core with Racing support (0.1.3 or newer). Older cores reject ID 15. See [content format](CONTENT_FORMAT.md) for IDs and compatibility.

The generator remains an alternative to the separate archive: in Core Options change **Generate game files** from **Ready** to **Generate**, then resume. It writes descriptively named selectors under the frontend system directory, which must be available and writable. Existing files are not overwritten. A failed write can leave a partial file that must be removed before retrying. If the frontend cannot reset the option to Ready, reset it manually before generating again; a saved Generate value at startup does not trigger generation.

## Intended platform targets

- Linux x86-64: `colortvgame_libretro.so`; the Linux release package includes licenses and [instructions](LINUX_RELEASE.md).
- Windows x86-64: `colortvgame_libretro.dll`.
- macOS Intel (x86-64): `colortvgame_libretro.dylib`; Apple Silicon is not built.

The [build workflow](../.github/workflows/build.yml) builds and tests these three targets and checks the macOS architecture. Its artifacts contain the core, license notices and [usage instructions](CI_ARTIFACT.md), not selectors. Release availability must be checked separately for each platform; report any missing target rather than assuming it was published. The separate selector archive can be created and checked as described in the [README](../README.md#separate-selector-archive).

## Validation and limitations

The generator fix was confirmed in the user's RetroDECK session. Automated tests use a simulated frontend; they do not establish compatibility with every real frontend or validate packaged binaries. Earlier user checks of Racing steering and controller vibration do not validate a 0.1.4 package. Windows and macOS builds remain experimental and their frontend compatibility, runtime requirements and distribution readiness are unverified; macOS may require permission to load an unsigned library. Match the frontend and core CPU architecture and check runtime libraries and sandbox access.

The frontend must support XRGB8888 video. Save states and rewind are unsupported. The 15 provisional variants and Racing prototype are independently implemented approximations, not verified original hardware behavior; see the [Racing design](RACING_DESIGN.md).
