# 0.1.2 release notes — draft

Source preparation for the user-approved 0.1.2 version; this document is not a publication or a claim that release packages have been validated.

## Changes since 0.1.1

- Game-menu preferences persist across reset, exit and subsequent launches, including direct `.ctg` and no-content launches, when the frontend provides a writable save directory. Random preferences are saved as Random rather than as resolved values. The user confirmed persistence in RetroArch across reset and full exits.
- Shooting adds target speed, direction and blinking choices, including Random. Random choices resolve at match start or restart and reroll after successful hits, not misses. The user confirmed Random behavior after successful hits in RetroArch; other new settings still need individual frontend checks. Slow and Fast are approximations.
- The new **Vibration** option defaults to Off and provides optional controller rumble for human paddle hits and shots, excluding automatic serves and demos. It is a modern convenience, not original hardware behavior. Automated tests cover events and lifecycle; vibration and its default have not yet been confirmed in a real frontend with a supported controller.

## Limitations and validation

All games remain approximations, not verified original hardware behavior. Save states and rewind are unsupported. Preference persistence requires a writable frontend save directory. Existing user frontend confirmations do not verify a 0.1.2 release package. Windows x86-64 and macOS Intel remain experimental pending frontend validation; Apple Silicon is not built. The previously recorded successful CI run predates the 0.1.2 changes. See the [roadmap](ROADMAP.md) for detailed status and remaining checks.
