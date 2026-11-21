# 0.1.9 release notes

These notes describe the changes in 0.1.9; they do not establish frontend validation of release packages.

## Changes since 0.1.8

- **Racing traffic:** MORE CARS adds a second opponent. The user confirmed the implementation in RetroArch; side-by-side traffic on the spacious one-player road follows a user observation, while spacing on narrow and two-player roads is provisional. CAR PATH adds an approximate zigzag option alongside straight traffic.
- **Racing collisions:** WALL CRASH optionally enables collisions with road boundaries, including invisible gaps in dashed edges. Cars move clear and gain temporary protection during recovery. Car-to-car hitboxes are narrower, and both players have initial collision protection after the countdown.
- **Previews:** Racing previews reflect the additional traffic and path settings. Rally demo paddle movement has been improved at fast speeds, although demo AI can still struggle.

## Compatibility and limitations

Version 0.1.9 reads game-menu preference formats 1–9 but writes version 9. The user confirmed settings migration in RetroArch. Version 0.1.8 rejects version 9 and falls back to its current/default settings; a subsequent menu change can overwrite the file, losing newer settings. Before upgrading, back up `colortvgame-preferences` in the frontend save directory if you may downgrade; restore the backup for the older core. Changes made after the backup will not be retained. Selector IDs and existing `.ctg` files retain their meanings; the separate selector archive contains 16 selectors, not ROMs.

Racing remains one console entry, not all 112 combinations; BAD ROAD is disabled. Gameplay, traffic, collisions, colors and presentation are approximations, not verified original-hardware behavior. The frontend must support XRGB8888; save states and rewind are unsupported.

The user confirmed one-player Racing play and steering, two-player scoring and behavior, MORE CARS, WALL CRASH and the latest zigzag changes in RetroArch. These checks do not validate a packaged 0.1.9 binary or establish original-hardware fidelity. Automated tests use a simulated frontend. Linux ARM64, Windows x86-64 and macOS Intel are experimental and need broader frontend testing; Linux ARM64 does not establish Raspberry Pi, Recalbox or RetroPie compatibility, and Apple Silicon is not built by CI. No packaged 0.1.9 cores have been validated in a real frontend.
