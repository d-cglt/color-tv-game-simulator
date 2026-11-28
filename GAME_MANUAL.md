# Color TV-Game Simulator — game manual

This independently developed libretro core supplies its own games; no ROM or BIOS is needed. Gameplay and presentation are approximations, not verified original hardware behavior. Original and Variant are project labels, not historical claims.

## Getting started and controls

Load the core in a compatible frontend such as RetroArch and start it without content to choose a game. Alternatively load a `.ctg` selector as content to open that game's menu; selectors are not ROMs. In menus, use RetroPad Up/Down to choose a row, Left/Right to change it, A to confirm and B to go back. Choose START to play. If an idle preview hides the menu, the first input restores it. Start during play opens the pause menu (resume, restart or exit); frontend Reset returns to the game menu. A direct-launch lock core option can prevent returning to the selector.

Controls below are RetroPad names: actual controller and keyboard bindings depend on the frontend. Where the frontend delivers direct keyboard input, arrows navigate menus, Enter confirms, Escape goes back and P pauses. In rally and Shooting, Up/Down or the left stick moves vertically; analog deflection controls speed and an active stick takes precedence over the D-pad. Each player uses their own controller. Game-menu settings persist if the frontend provides a writable save directory.

## Original features mode

The frontend core option **Original features mode** defaults to Off. On restricts rally games to two human players and a score of 15, hides MODE, SCORE and ICE HOCKEY, and disables ice effects. SPEED and independent P1/P2 sizes remain available. In Shooting it fixes 15 shots and DOWN direction; TARGET MOVE offers MANUAL or NORMAL, TARGET SIZE LARGE or SMALL, and SHOTS SPEED LOW or HIGH. TARGET SIZE sets both the shooter and target size independently of shot speed; P1 controls the shooter and P2 controls the target in MANUAL mode. Firing is silent. Stereo and vibration are disabled for all games; those frontend options and Shooting fire sound remain visible, and changes to them take effect when the mode is turned off. Anti-stalemate remains available and unchanged. Racing's game modes and switches remain available.

The option is saved by the frontend. Existing game-menu choices are retained when overridden and restored when the mode is turned off, including across restarts if the save directory is writable. Unsupported saved Shooting selections use NORMAL movement, LARGE target and LOW shot speed while the mode is on, without changing the stored selections. Changing an available setting in this mode updates that saved setting. Turning the mode on or off during a rally or Shooting match restarts that match; Racing continues. This is a configurable simulator mode, not a claim of historical fidelity.

## Rally games

Tennis, Hockey, Volleyball and Ping-Pong each have Singles and Doubles. Each side is controlled by one player: in Doubles that player's two paddles move together. Return the ball past the opponent to score; the first side to the selected SCORE wins. Serves are automatic after a short wait, first from the left, then from the player who scored.

- **Tennis Original:** a straightforward paddle rally. **Tennis Variant:** adds central blocks that affect the ball.
- **Hockey Original:** defend the goal opening; end walls return balls outside it. **Hockey Variant:** adds a central barrier: an end-wall hit closes its opening and a center hit opens it. The optional **ICE HOCKEY** launch setting defaults to OFF. When ON, both human and computer paddles accelerate smoothly and glide noticeably after release; reverse direction to brake, and the court has an icy color with dark blue markings. Scoring and other settings are unchanged. The choice is saved for Hockey games; it is a simulator feature, not a claim about original hardware.
- **Volleyball Original:** the center net can change the ball's vertical direction. **Volleyball Variant:** central blocks can reflect the ball horizontally.
- **Ping-Pong:** a center net reflects low balls; higher balls can pass over it.

In each rally launch menu, **MODE** selects 2 PLAYERS or a one-player opponent at EASY, NORMAL or HARD. Normal falls back to Easy after 15 seconds of active rally; Hard starts at Hard for 30 seconds, then alternates Normal and Easy every 15 seconds until the point ends. **SPEED** selects LOW or HIGH; **P1 SIZE** and **P2 SIZE** select LARGE or SMALL paddles; **SCORE** selects 5, 10, 15, 20, 25 or 30 (default 15). The frontend's **Rebound behavior** and **Anti-stalemate** core options also affect rallies.

## Shooting Game

Move P1's shooter with Up/Down or the left stick and fire with A or Y. Direct keyboard W/S aims and X or Space fires if delivered by the frontend. Hit the moving target as often as possible within the selected number of shots; the display counts shots and hits. Only one shot is in flight at a time. The blinking target and shot can still hit while invisible.

- **TARGET MOVE:** MANUAL lets P2 move the target with Up/Down or the left stick; SLOW, NORMAL (default) and FAST move it automatically; RANDOM chooses automatic behavior.
- **DIRECTION:** DOWN, UP, RANDOM or BOUNCE for the automatic target; not used in manual mode.
- **TARGET SIZE:** LARGE, SMALL or RANDOM. In Original features mode, the selected size applies to both shooter and target.
- **SHOTS SPEED:** LOW, HIGH or RANDOM; with Original features mode off, LOW uses a small shooter and HIGH a large one.
- **SHOTS:** 5, 10, 15, 20, 25 or 30 (default 15).

Random settings resolve at match start or restart and after successful hits, not after misses. The frontend's Shooting fire sound and Vibration core options are optional.

## Racing 112

Steer with Left/Right or the left stick's horizontal axis; direct keyboard Left/Right or A/D also works if delivered by the frontend. Avoid traffic and complete road sections without a collision to earn points during the 64-second active race. A collision slows the car and forfeits that section's point. In two-player mode scores are independent, but a crash in either lane slows both cars.

- **MODE:** 1P SPACIOUS, 1P NARROW or 2 PLAYERS. In two-player mode each player steers their own car on a side-by-side track.
- **SPEED:** LOW, MEDIUM or HIGH. In one-player mode P1 L2 selects the LOW gear lever (always low speed), and R2 selects HIGH (the SPEED setting). The selected gear stays after release; pressing both cancels shifting. In two-player mode SPEED is shared and fixed; there are no gear controls.
- **MORE CARS:** adds a second traffic car.
- **WALL CRASH:** enables boundary collisions; the car moves clear during recovery.
- **CAR PATH:** STRAIGHT or ZIGZAG traffic.
- **BAD ROAD:** adds dirt stretches that cause steering wiggle.

Racing is one selector entry with configurable modes and switches, not 112 separate files. Historical game numbering is not displayed.

## License

Color TV-Game Simulator is independently developed and is not affiliated with, endorsed by, or sponsored by Nintendo. The project, including this manual, is distributed under the MIT license; see `LICENSE` in the package.
