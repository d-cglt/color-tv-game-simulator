# Racing 112 — first implementation and future design

The current scope includes **one player on a spacious or narrow road and newly implemented two-player Racing**, with approximate steering and traffic. A separate Racing engine is integrated into the core's selector, direct launch and lifecycle; the user confirmed Racing play and steering in a frontend. Scoring, collisions, timing and presentation are approximations, not verified hardware behavior; see [research notes](RESEARCH.md). The intended selector has **one Racing 112 entry**, not 112 games. The current menu selects spacious, narrow or two-player mode and a speed switch; other configuration choices below are future proposals, not implemented combinations. Earlier one-player play and steering were confirmed in a frontend; two-player Racing awaits frontend confirmation.

## Proposed launch menu

The existing scrolling menu can display these rows; `RETURN` and `START` remain available as in other games. The game number is a derived, visibly provisional label, not a claim of verified historical numbering.

| Setting | Choices | Notes |
| --- | --- | --- |
| PLAYERS | 1 PLAYER, 2 PLAYERS | Selects the configuration group. |
| ROAD | WIDE, NARROW | In two-player mode show WIDE fixed; preserve the one-player selection for later. |
| SPEED | LOW, MEDIUM, HIGH | In one-player mode the LOW lever is always low and HIGH uses this switch; the user reports fixed switch-selected speed in two-player mode. |
| BAD ROAD | OFF, ON | Reported hazard; effect requires verification. |
| WALL CRASHES | OFF, ON | Reported barrier collision switch. |
| OPPONENT CARS | 1, 2 | Reported traffic choice; exact spawning requires verification. |
| CAR PATH | STRAIGHT, ZIGZAG | Reported opposing-vehicle behavior. |

Suggested default: one player, wide road, low/medium gears, no bad road, wall crashes off, one opponent and straight traffic. The translated chart labels this provisional game 1. Left/right changes options; up/down navigates; Start begins the chosen configuration. Disable the fixed two-player road setting visibly. Show the selected game's number as a derived label only if it fits the interface; do not imply that unimplemented combinations are playable.

The seven reported configuration groups and four binary switches give 112 combinations. Derive numbering from an **explicit lookup table**, not binary mask order: the translated chart uses a different ordering. See [RESEARCH.md](RESEARCH.md) for the transcription and its limitations. An example from that secondary transcription: two players, medium speed and zigzag only gives provisional game 85. Validate labels, constraints and numbering against original material before calling them historically accurate.

## Content and preferences

- Integrate Racing as an additional selector entry; preserve all current entries, relative order and first-selection behavior.
- ID 15 (`0F`) is assigned to Racing as one five-byte CTG1 console descriptor, `content/Racing 112 (Color TV Game Racing 112).ctg`. The updated generator includes one corresponding named file, for 16 descriptors in total. IDs 0–14 and existing files retain their meanings. Released older cores reject ID 15; the new descriptor requires an updated core with Racing integration. A descriptor identifies Racing, not a particular configuration.
- Preserve existing game preferences and saved selections. Racing mode and speed-switch preferences are saved with the game-menu preferences for no-content and direct launches. Consider separately versioned storage for additional future choices if needed.
- Separate Racing game state and rules from the Libretro interface; explicitly dispatch gameplay, preview, rendering, input, pause, reset and result paths. Existing rally-specific arrays and logic must never be indexed with the new game ID. Reuse shared facilities only where appropriate.
- Provide analog steering with a digital alternative, gear and two-player controls, but treat steering response and mappings as provisional until researched. Keep preview state separate from actual games.

## Road geometry

The provisional Racing grid has 53 columns and 48 rows of 4 × 5-pixel cells. The centered 212-pixel road spans x=54–265 (end 266 exclusive), with borders at X00 and X52 and 51 interior columns. The borders have 240-pixel dashes and 6-cell (30-pixel) gaps, giving a 270-pixel period. The opponent's top is 10 vertical cells (50 pixels) after the end of the gap, modulo that period. Reset and preview start with the same alignment; preview wrapping retains pixel overshoot, and starting from a preview retains its road and traffic layout. X26 is the conceptual center; a divider is drawn in narrow and two-player modes, but not on the spacious one-player road. The single score's digits occupy X04–X07 and X12–X15. Both cars have a 6 × 6-cell (24 × 30-pixel) silhouette: a two-column body and two pairs of two-row wheels, connected to the body by two-pixel-high bridges. The player starts at y=194, retaining its bottom edge at y=224; opponents are clipped to y=9–223. Their collision boxes use the full sprite bounds, including the gaps; steering and traffic placement keep those bounds inside the borders. The grid and geometry are Racing-only.

## First implementation stage

A collision-free 64-second run accelerates from 30 by one speed unit per active frame. With HIGH selected on frame 1 when applicable, the calibrated caps and distances are:

| Gear | Cap (speed units) | Distance (pixels) | Score across all 270 starting phases | Steady speed (pixels/second) |
| --- | ---: | ---: | ---: | ---: |
| Low | 136 | 15,308 | 56–57 | 241.8 |
| Medium | 166 | 18,615 | 68–69 | 295.1 |
| High | 233 | 25,902 | 95–96 | 414.2 |

Distance uses the 16/9 movement scale and includes initial acceleration. The reset phase scores 56, 69 and 96 respectively; preview can supply any other phase. Collisions, recovery and later shifts reduce distance. The longer road period changes traffic and scoring cadence, and the higher caps change movement and acceleration times; these are deliberate consequences of the recalibration. Engine pitch uses the named speed caps as interpolation anchors, retaining 193 Hz at low, 243 Hz at medium, 393 Hz at high and 128 Hz at the starting speed. These values are calibration choices, not hardware measurements.

The standalone engine implements one-player spacious and narrow road modes with digital steering or rate-based analog steering, traffic, collision and road-section scoring, and a fixed 64-second active timer following a separate startup sequence. Narrow mode uses the left half of the road, a center boundary at X26, a provisional purple palette and opposing cars that may overlap either boundary by one cell. The player remains within the left lane. The newly implemented two-player mode has side-by-side tracks with a green road and green player cars, one opponent and independent scoring per lane. Its colors approximate reference material, not verified hardware colors. Both players steer with their own D-pad or left-stick horizontal axis; P2 has no gear controls. Both lanes accelerate together at the start to the speed selected by the shared Low/Medium/High SPEED switch; a collision in either lane slows both, as reported by the user, and both accelerate together after recovery. One-player behavior is unchanged. The diagnostic fixed-score display uses both left and right values in two-player mode, and the left value in one-player mode. Frontend confirmation of two-player Racing is pending. Analog deflection controls steering speed with a dead zone; full deflection matches the D-pad at six pixels per frame, and releasing the stick holds the car's position. L2/LT selects the LOW lever position and R2/RT selects HIGH. The SPEED switch selects the HIGH position's low, medium or high speed; LOW is always low. Holding does not repeat, simultaneous held triggers cancel, and the choice persists after release. A new match starts in low gear; steering is available during the countdown, while gear presses during countdown or pause are not queued. The car automatically speeds up after startup; on impact it returns to a provisional nonzero starting speed, holds it through the blink and sound sequence, and then speeds up again. Road/traffic movement and engine pitch follow the current speed. Shifting down progressively reduces speed. The trigger mapping, speed values and acceleration and deceleration curves are provisional; published accounts describe a two-position gear lever and no accelerator pedal. Automated tests cover existing IDs and files, selector and direct-content paths, lifecycle and input; the user confirmed Racing play and steering in a frontend; other frontend paths and the packaged binary have not been established by that confirmation. Do not expose unavailable combinations as playable. Expand toward the proposed menu only as the relevant rules are independently implemented and tested. Validate against original documentation or recordings where available; document invented scoring, collision, timing, audiovisual details and other approximations.
