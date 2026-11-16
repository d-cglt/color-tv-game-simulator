# 0.1.6 release notes

These notes describe the planned 0.1.6 release; a 0.1.6 release and packages have not yet been published.

## Changes since 0.1.5

- Recreates rally wall and obstacle collisions at 240 Hz and paddle collisions at 480 Hz, with shaped pulses and an approximate fade. Point-loss and match-ending sounds alternate between the two pitches.
- Blinks the score after a rally match or Shooting game ends: 400 ms visible and approximately 133 ms hidden, until restart or exit.
- Updates rally serves: the winner serves from the current paddle center at grid X04 or X36, retaining the previous trajectory; the first serve starts on the left and travels downward. Serves remain automatic with an approximately 2.17-second delay.
- Changes Shooting target-hit, shot-exit and game-ending sounds. The new **Shooting fire sound** core option defaults to On and plays the wall-collision sound; Off silences firing, as observed on the original console. The setting also applies to previews and does not affect vibration.
- Renames bundled source-tree selectors to the descriptive names used by the generator; their contents and IDs are unchanged. Existing paths and playlists pointing at the old `slot-XX.ctg` names need updating.

## Platform targets and selectors

Planned targets are Linux x86-64, Windows x86-64 and macOS Intel. Apple Silicon is not built by the workflow. Each release also includes a separate versioned `colortvgame-0.1.6-selectors.zip` archive containing 16 `.ctg` game selectors; they select built-in games and are not ROMs. Core packages do not include them.

## Validation and limitations

The user confirmed the updated sounds, Shooting option and score blinking, and the revised serve behavior in RetroArch, including a serve from outside the field. These checks do not validate a packaged 0.1.6 binary. Automated tests use a simulated frontend; Windows and macOS remain experimental, and native macOS frontend testing remains unverified. Confirm CI and release artifact availability before publishing.

The sound waveform and fade are approximations informed by observations. Rendering, geometry, physics, AI and Racing remain independently implemented approximations, not verified original hardware behavior. Racing is a one-player prototype, not all 112 configurations. The frontend must support XRGB8888; save states and rewind are unsupported.
