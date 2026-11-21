# Color TV-Game Simulator for Linux

This package contains the Color TV-Game Simulator libretro core, `colortvgame_libretro.so`, the project `LICENSE`, and the vendored libretro API header's MIT notice in `LICENSE-libretro.txt`. The core reports its version to the frontend.

**Disclaimer:** Color TV-Game Simulator is an independently developed project. It is not affiliated with, endorsed by, or sponsored by Nintendo.

## Installation and use

Extract the archive and load `colortvgame_libretro.so` using RetroArch's **Load Core** (or another compatible libretro frontend). The core and frontend must have matching CPU architectures and compatible Linux runtime libraries. Alternatively, copy the core into the frontend's configured cores directory, then load it there. Sandboxed frontends may need permission to access the extracted file. No frontend, launcher, core metadata, or content files are included.

Start the core without content to open the game selector. Navigate and select a game, adjust its settings, then choose START. Alternatively, use **Generate game files** in Core Options to create `.ctg` selectors in the frontend's system directory; that directory must be writable. A separate versioned archive containing 16 selectors accompanies each release, even when its contents are unchanged; it is not included in this core package. Extract it and load a selector as content after loading the core. These files are selectors, not ROMs; ID 15 requires a core with Racing support (0.1.3 or newer). Input bindings and hotkeys depend on frontend configuration; gameplay uses RetroPad controls. RetroPad Start opens the in-game pause menu.

## Limitations

The 15 provisional variants and the Racing prototype are playable approximations; historical identities, behavior and presentation are not verified. Racing includes one-player spacious and narrow roads and a two-player mode, but is one console entry, not all 112 combinations. MORE CARS adds a second opponent, WALL CRASH enables boundary collisions, and CAR PATH enables zigzag traffic; BAD ROAD remains disabled. The user confirmed two-player scoring and behavior, MORE CARS, WALL CRASH, the latest zigzag changes and settings migration in RetroArch. Those checks do not establish original-hardware fidelity or frontend validation of a packaged 0.1.9 binary. Demo AI has been improved at fast speeds but may still struggle. Linux ARM64 is experimental; Raspberry Pi, Recalbox and RetroPie compatibility is unverified. The frontend must support XRGB8888 video. Save states and rewind are unsupported. Automated tests use a simulated frontend. Broader real-frontend and platform validation remains pending. If loading fails, check architecture, runtime dependencies and sandbox access.

Version 0.1.9 reads game-menu preference formats 1–9 and writes version 9, which 0.1.8 and earlier cannot read. Version 0.1.8 ignores it, retains current or default settings, and may overwrite it when a menu setting changes. Before upgrading, back up `colortvgame-preferences` in the frontend save directory if you may downgrade; restore the backup when returning to an older core. Changes made after the backup will not be retained. A packaged 0.1.9 core has not yet been validated in a real frontend.
