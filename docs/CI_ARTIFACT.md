# Color TV-Game Simulator — CI build

This artifact contains a libretro core, the project `LICENSE`, and the vendored libretro header's MIT notice in `LICENSE-libretro.txt`. It does not include RetroArch, content, launchers, or core metadata. These are automated build artifacts, not release packages; the artifacts themselves have not been validated in a real frontend. Windows x86-64 and macOS Intel are experimental targets; Apple Silicon is not built by this workflow.

Extract the artifact and use RetroArch's **Load Core** to select `colortvgame_libretro.so` (Linux), `colortvgame_libretro.dll` (Windows), or `colortvgame_libretro.dylib` (macOS). The frontend must match the artifact's CPU architecture and have compatible operating system and runtime libraries. macOS may require allowing a downloaded, unsigned library in system security settings. Sandboxed frontends may need access to the extracted file.

Start the core without content to use its game selector. Alternatively, generate `.ctg` selector files through the **Generate game files** core option; the frontend's system directory must be writable. The files are selectors, not ROMs. Configure controls in the frontend; RetroPad Start opens the pause menu.

The frontend must support XRGB8888. Save states and rewind are unsupported. All 15 variants are approximations, not verified original hardware behavior. Automated tests use a simulated frontend and do not establish compatibility with a real frontend. If loading fails, check architecture, runtime dependencies, security restrictions and sandbox permissions.

Color TV-Game Simulator is independently developed and is not affiliated with, endorsed by, or sponsored by Nintendo.
