# 0.1.8 release notes

## Changes since 0.1.7

- **Shooting:** Revised target and shot blinking, added target size and Bounce direction choices, and separated shot speed from rally speed. Target size and shot speed include Random choices; target movement defaults to Normal.
- **Demos:** Previews now follow applicable rally speed and paddle sizes, Shooting target and shot settings, and Racing road mode. Preview randomness is separate from gameplay; Racing carries its preview layout into the race. Demo AI can struggle to keep up at fast ball speeds.
- **Racing:** Added a narrow road and two-player mode alongside the spacious one-player road. Two-player races have separate lanes, opponents and scores, with shared acceleration and collision slowdown. The user confirmed two-player scoring and behavior in RetroArch. Four gray IN DEV options are disabled and do not change gameplay.
- **Racing controls and speed:** Each player steers with their own D-pad or left stick; steering is available during the countdown. The Low/Medium/High SPEED switch sets the shared two-player speed and the one-player HIGH lever speed. In one-player mode L2/LT selects LOW and R2/RT selects HIGH. Opponent placement and road timing were revised.

## Compatibility and limitations

Version 0.1.8 reads older game-menu preferences, but writes a newer `colortvgame-preferences` format that older cores cannot read. Back up that file in the frontend save directory before upgrading if you may downgrade. Selector IDs and existing `.ctg` files retain their meanings; the separate selector archive contains 16 selectors, not ROMs.

Gameplay and presentation are approximations, not verified original-hardware behavior. Racing remains one console entry, not all 112 combinations. The frontend must support XRGB8888; save states and rewind are unsupported.

Linux ARM64, Windows x86-64 and macOS Intel builds are experimental and need broader frontend testing. Linux ARM64 does not establish Raspberry Pi, Recalbox or RetroPie compatibility; Apple Silicon is not built. Automated tests use a simulated frontend. Previous user checks, including two-player Racing, do not validate a packaged 0.1.8 binary or every release change. The packaged 0.1.8 cores have not been validated in a real frontend.
