# Color TV-Game Simulator for Linux

This package contains the Color TV-Game Simulator libretro core, `colortvgame_libretro.so`, the project `LICENSE`, and the vendored libretro API header's MIT notice in `LICENSE-libretro.txt`. The core reports its version to the frontend.

**Disclaimer:** Color TV-Game Simulator is an independently developed project. It is not affiliated with, endorsed by, or sponsored by Nintendo.

## Installation and use

Extract the archive and load `colortvgame_libretro.so` using RetroArch's **Load Core** (or another compatible libretro frontend). The core and frontend must have matching CPU architectures and compatible Linux runtime libraries. Alternatively, copy the core into the frontend's configured cores directory, then load it there. Sandboxed frontends may need permission to access the extracted file. No frontend, launcher, core metadata, or content files are included.

Start the core without content to open the game selector. Navigate and select a game, adjust its settings, then choose START. Alternatively, use **Generate game files** in Core Options to create `.ctg` selectors in the frontend's system directory; that directory must be writable. These files are selectors, not ROMs. Input bindings and hotkeys depend on frontend configuration; gameplay uses RetroPad controls. RetroPad Start opens the in-game pause menu.

## Limitations

All 15 provisional variants are playable approximations; historical identities, behavior and presentation are not verified. The frontend must support XRGB8888 video. Save states and rewind are unsupported. Automated tests use a simulated frontend. The 0.1.2 package has not yet been tested in a frontend; user-confirmed feature checks do not establish validation of the packaged binary. Vibration remains unconfirmed in a real frontend. Broader real-frontend and platform validation remains pending. If loading fails, check architecture, runtime dependencies and sandbox access.
