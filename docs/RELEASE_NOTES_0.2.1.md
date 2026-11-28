# 0.2.1 release notes

## Changes since 0.2.0

- **Direct launch:** An optional lock keeps a directly loaded `.ctg` game in its own menus until content is closed or the option is disabled. No-content sessions are unaffected.
- **Controls:** Stable frontend input descriptors cover the controls for all games rather than changing with the selected game.
- **Game manual:** A guide to controls, games and settings is included in Linux and selector packages and configured for all GitHub Actions platform artifacts.
- **Stereo:** Optional positional stereo balances sounds across channels; mono remains available.
- **Ice hockey:** An optional Hockey setting adds paddle acceleration and glide with an icy court. It is a simulator feature, not a historical claim.
- **Original features mode:** An optional restricted configuration hides unsupported menu choices and temporarily overrides selected settings while preserving saved preferences. Menu layout and alignment have also been adjusted. See the manual for behavior and restart implications.

## Compatibility and validation

The user confirmed original features mode and its menus, ice visuals and stereo; direct-launch locking was also confirmed in RetroArch, although the subsequently hidden Return layout was not separately confirmed. These checks do not establish original-hardware fidelity or independently validate the final CI-built release binaries. Gameplay, colors and presentation remain approximations.

Version 0.2.1 reads preference formats 1–9 and writes version 9. Older version 9 readers preserve unknown settings when saving; 0.1.8 and earlier cannot read version 9 and may overwrite preferences. Back up `colortvgame-preferences` before upgrading if downgrading to those versions, and restore it on downgrade; later changes will be lost. Existing selectors remain compatible.

A fresh local Linux x86-64 Release build passed all 23 CTest tests; the Linux and selector archives were inspected, including the manual, selector IDs and packaged core version. The user confirmed the locally packaged Linux x86-64 0.2.1 core in RetroArch. Automated tests use a simulated frontend; the final CI-built release artifacts have not been independently confirmed in a frontend. Linux ARM64, Windows x86-64 and macOS Intel remain experimental; their frontend behavior is unverified. Apple Silicon is not built by CI; Raspberry Pi, Recalbox and RetroPie compatibility is unverified. XRGB8888 is required; save states and rewind are unsupported. The release remains unpublished until the draft is reviewed and published.
