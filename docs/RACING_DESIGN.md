# Racing 112 — first implementation and future design

The first-playable scope is **one player on a wide road**, with approximate steering and traffic. A separate Racing engine is integrated into the core's selector, direct launch and lifecycle; the user confirmed Racing play and steering in a frontend. Scoring, collisions, timing and presentation are approximations, not verified hardware behavior; see [research notes](RESEARCH.md). The intended selector has **one Racing 112 entry**, not 112 games. The configuration menu below is a future proposal, not implemented combinations.

## Proposed launch menu

The existing scrolling menu can display these rows; `RETURN` and `START` remain available as in other games. The game number is a derived, visibly provisional label, not a claim of verified historical numbering.

| Setting | Choices | Notes |
| --- | --- | --- |
| PLAYERS | 1 PLAYER, 2 PLAYERS | Selects the configuration group. |
| ROAD | WIDE, NARROW | In two-player mode show WIDE fixed; preserve the one-player selection for later. |
| GEAR SPEEDS / SPEED | 1P: LOW / MED, LOW / HIGH; 2P: LOW, MED, HIGH | One-player choice selects the two gear speeds; two-player speed is fixed. Remember each mode's selection independently. |
| BAD ROAD | OFF, ON | Reported hazard; effect requires verification. |
| WALL CRASHES | OFF, ON | Reported barrier collision switch. |
| OPPONENT CARS | 1, 2 | Reported traffic choice; exact spawning requires verification. |
| CAR PATH | STRAIGHT, ZIGZAG | Reported opposing-vehicle behavior. |

Suggested default: one player, wide road, low/medium gears, no bad road, wall crashes off, one opponent and straight traffic. The translated chart labels this provisional game 1. Left/right changes options; up/down navigates; Start begins the chosen configuration. Disable the fixed two-player road setting visibly. Show the selected game's number as a derived label only if it fits the interface; do not imply that unimplemented combinations are playable.

The seven reported configuration groups and four binary switches give 112 combinations. Derive numbering from an **explicit lookup table**, not binary mask order: the translated chart uses a different ordering. See [RESEARCH.md](RESEARCH.md) for the transcription and its limitations. An example from that secondary transcription: two players, medium speed and zigzag only gives provisional game 85. Validate labels, constraints and numbering against original material before calling them historically accurate.

## Content and preferences

- Integrate Racing as an additional selector entry; preserve all current entries, relative order and first-selection behavior.
- ID 15 (`0F`) is assigned to Racing as one five-byte CTG1 console descriptor, `content/slot-15.ctg`. The updated generator includes one corresponding named file, for 16 descriptors in total. IDs 0–14 and existing files retain their meanings. Released older cores reject ID 15; the new descriptor requires an updated core with Racing integration. A descriptor identifies Racing, not a particular configuration.
- Preserve existing game preferences and saved selections. For future Racing choices, consider a separate versioned preference record in the frontend save directory with validated reads and safe writes, avoiding changes to the existing strict-length record. Both no-content and direct launches should use the same Racing choices; this is a proposal, not a claim that Racing preferences are implemented.
- Separate Racing game state and rules from the Libretro interface; explicitly dispatch gameplay, preview, rendering, input, pause, reset and result paths. Existing rally-specific arrays and logic must never be indexed with the new game ID. Reuse shared facilities only where appropriate.
- Plan analog steering with a digital alternative, gear and two-player controls, but treat steering response and mappings as provisional until researched. Keep preview state separate from actual games.

## First implementation stage

The standalone engine implements a one-player wide road with digital steering or rate-based analog steering, traffic, collision and road-section scoring, and a fixed timer. Analog deflection controls steering speed with a dead zone; full deflection matches the D-pad at six pixels per frame, and releasing the stick holds the car's position. L2/LT selects speed 1 and R2/RT selects speed 2; the choice persists after release. The car automatically speeds up after startup; on impact it returns to a provisional nonzero starting speed, holds it through the blink and sound sequence, and then speeds up again. Road/traffic movement and engine pitch follow the current speed. Shifting down progressively reduces speed. The trigger mapping, speed values and acceleration and deceleration curves are provisional; published accounts describe a two-position gear lever and no accelerator pedal. Automated tests cover existing IDs and files, selector and direct-content paths, lifecycle and input; the user confirmed Racing play and steering in a frontend; other frontend paths and the packaged binary have not been established by that confirmation. Do not expose unavailable combinations as playable. Expand toward the proposed menu only as the relevant rules are independently implemented and tested. Validate against original documentation or recordings where available; document invented scoring, collision, timing, audiovisual details and other approximations.
