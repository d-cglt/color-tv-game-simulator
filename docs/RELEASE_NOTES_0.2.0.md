# 0.2.0 release notes

## Changes since 0.1.9

- **Racing 112 configuration coverage:** All seven reported player, road and speed groups and all combinations of the four binary switches are selectable: 112 reported combinations. One-player low/low gearing is additionally selectable, outside those seven groups. Racing remains one selector entry; historical game numbering is not implemented.
- **BAD ROAD:** Enable dirt stretches that cause steering wiggle, with optional controller rumble. The user confirmed the spacing, dirt border color, preview, and vibration on exiting dirt in RetroArch, as well as rumble on dirt. Dirt generation and the effect remain approximations, not verified original-hardware behavior.

## Compatibility and limitations

Version 0.2.0 reads game-menu preference formats 1–9 and writes extensible version 9. Version 0.1.9 can read version 9, ignores the unknown BAD ROAD setting and preserves it when saving. Version 0.1.8 and earlier cannot read version 9; 0.1.8 retains current or default settings and may overwrite the file after a menu change. If you may downgrade to 0.1.8 or earlier, back up `colortvgame-preferences` in the frontend save directory before upgrading and restore it when downgrading. Changes after the backup will not be retained. Existing selector IDs and `.ctg` files retain their meanings; selectors are not ROMs.

The reported configuration chart is a secondary transcription; historical numbering, gameplay, timing, colors and presentation are not verified against original hardware. Earlier Racing play, steering, two-player behavior, switches and preference migration were also confirmed by the user in RetroArch. An earlier packaged 0.2.0 build was checked before the final menu-label fix; the updated label was confirmed in RetroArch using the `build/` core. These checks do not validate the final platform release packages or establish historical fidelity. Automated tests use a simulated frontend.

XRGB8888 frontend support is required; save states and rewind are unsupported. Linux ARM64, Windows x86-64 and macOS Intel remain experimental and need broader frontend testing. Linux ARM64 does not establish Raspberry Pi, Recalbox or RetroPie compatibility; Apple Silicon is not built by CI.
