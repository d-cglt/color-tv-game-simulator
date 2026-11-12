# Color TV-Game Simulator selectors

These 16 `.ctg` files select games in the Color TV-Game Simulator libretro core. They are five-byte project-defined descriptors, not ROMs or original console assets; the independently developed core supplies the game logic. This archive does not include the core or RetroArch.

Extract the archive, install a compatible core in RetroArch, choose **Load Core**, then **Load Content** and select a `.ctg` file. The files can be placed anywhere the frontend can access. Frontend and core architectures must match, and the frontend must support XRGB8888 video. Alternatively, start the core without content and choose a game in its selector.

IDs 0–14 retain their existing meanings. ID 15 is one Racing 112 console selector, not 112 separate games; older released cores reject it. Use a core with Racing integration, such as version 0.1.3 or later. Racing is a prototype with approximate gameplay, not a complete or hardware-verified implementation. The other games and their presentation are also approximations. Names and historical console assignments are provisional; Original and Variant are project labels, not claims of historical primacy. The `6 + 15` designation is unverified.

Older generated A/B filenames can be ambiguous across generator versions; the encoded ID, not the filename, determines the game. These selectors use the current generator's names and stable IDs. Existing selectors need not be replaced.

This independent project is not affiliated with, endorsed by, or sponsored by Nintendo.

## Building the archive from source

With CMake 3.16+, a C++17 compiler and Python 3 installed, run from the repository root:

```sh
cmake -S . -B build
cmake --build build --config Release --target package_selectors
```

The result is `build/colortvgame-selectors.zip`. The target builds the same generator used by the core, then writes a ZIP with fixed entry order and metadata. It replaces the output only after successful packaging. Run `ctest --test-dir build -C Release --output-on-failure` after building all targets to run the test suite.
