# Libretro integration proposal — draft

This is a draft for maintainer review. It has not been submitted.

## Proposed message

Hello Libretro team,

I maintain [Color TV-Game Simulator](https://github.com/d-cglt/color-tv-game-simulator), an independent MIT-licensed Libretro core providing playable approximations of the 15 Color TV-Game variants. It uses independently implemented gameplay and does not require original ROMs or firmware. It is not affiliated with, endorsed by, or sponsored by Nintendo.

The core supports starting without content and optionally uses small `.ctg` selector files. It provides core options and input descriptors; save states and rewind are not supported. Gameplay and historical console mappings remain approximate or unverified.

The project currently builds and passes automated tests on Linux x86-64, Windows x86-64 and macOS Intel via GitHub Actions. Linux has been tested in RetroArch; Windows and macOS frontend testing remains pending. The draft `colortvgame_libretro.info` has been tested with RetroArch Core Information. The current build uses CMake and produces `colortvgame_libretro`.

Would this core be suitable for consideration for the Core Downloader? If so, could you advise on the preferred registration, buildbot/CI configuration, metadata submission, and initial platform targets? We understand that compatibility, licensing and maintenance are subject to your review.

- Repository: https://github.com/d-cglt/color-tv-game-simulator
- Releases: https://github.com/d-cglt/color-tv-game-simulator/releases
