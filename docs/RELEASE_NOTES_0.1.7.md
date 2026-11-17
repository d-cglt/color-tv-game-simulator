# 0.1.7 release notes


## Changes since 0.1.6

- **Racing:** Tuned gameplay and added three selectable gears. L2 shifts down and R2 shifts up, one gear per press.
- **Analog controls:** Use the left stick for proportional paddle movement in rally games and aiming or manual target movement in Shooting.
- **Diagnostics:** Test-menu settings persist between sessions when the frontend provides a writable save directory. A new option can disable Racing car collisions.
- **Volleyball:** Improved collision handling.
- **Ping Pong:** Improved computer opponent behavior and background presentation.

## Platforms and validation

Linux ARM64, Windows and macOS Intel builds are experimental and need broader frontend testing. Linux ARM64 CI does not establish compatibility with Raspberry Pi, Recalbox or RetroPie. Apple Silicon is not built. If you try an experimental build, please [share your results](https://github.com/d-cglt/color-tv-game-simulator/issues).

Automated tests use a simulated frontend; they do not replace testing a packaged core in a real frontend. Earlier user checks confirmed Racing play and steering, and controller vibration in RetroArch, but do not validate a packaged 0.1.7 binary or every change in this release.

The separate selector archive contains 16 `.ctg` files that select built-in games; they are not ROMs. Gameplay and presentation remain approximations, and Racing is a one-player prototype rather than all 112 configurations. XRGB8888 frontend support is required; save states and rewind are unsupported.
