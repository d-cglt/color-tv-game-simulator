# 0.1.3 release notes

## Changes since 0.1.2

- Adds a first playable Racing 112 prototype as one selector and direct-content entry (ID 15), with one player on a wide road, traffic, scoring and a timer. It does not implement all 112 combinations.
- Steer with the D-pad or left analog stick. L2/LT selects speed 1 and R2/RT selects speed 2; the selection persists after release. The car accelerates automatically after startup and impacts. Controls, physics, scoring and presentation are approximations, not verified original hardware behavior.
- The game-file generator includes a Racing descriptor; existing IDs 0–14 and their files retain their meanings. Older cores without Racing support reject ID 15.

## Validation and limitations

The user confirmed Racing play and steering in a frontend. This does not establish validation of a packaged 0.1.3 binary or of every launch and lifecycle path. Automated tests use a simulated frontend. Windows x86-64 and macOS Intel remain experimental and frontend compatibility is unverified; Apple Silicon is not built. Save states and rewind are unsupported, and the frontend must support XRGB8888 video. See the [Racing design](RACING_DESIGN.md) for scope and future plans.
