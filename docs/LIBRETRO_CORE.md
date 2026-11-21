# Color TV-Game Simulator — Libretro core (draft)

Color TV-Game Simulator is an independently developed, MIT-licensed Libretro core. It simulates 15 provisional Color TV-Game 6/15 variants and includes a first-playable Racing 112 prototype. The core supplies the game logic; it is not an original-hardware emulator or an official Nintendo product. Gameplay, sound, presentation, historical identities and console assignments are approximate or unverified. **Original** and **Variant** are project labels, not claims about historical primacy; the `6 + 15` designation is unverified. See the project's [research notes](https://github.com/d-cglt/color-tv-game-simulator/blob/main/docs/RESEARCH.md).

## Getting started

1. Load a compatible `colortvgame_libretro` library in your Libretro frontend, such as RetroArch.
2. In RetroArch, select **Start Core** without content to open the game selector. Choose a game, adjust its launch-menu settings and select **START**.
3. Alternatively, load a `.ctg` selector as content to open that game's launch menu directly. No ROM, BIOS or original console assets are needed.

Frontend menus and paths vary. The optional [core information file](https://github.com/d-cglt/color-tv-game-simulator/blob/main/docs/CORE_INFO.md) is installed separately and is not included in the existing core artifacts. Its presence does not mean this core is available from the Libretro Core Downloader.

### Platforms and testing

The repository's CI is configured to build and run tests for Linux x86-64, experimental Linux ARM64, Windows x86-64 and macOS Intel (x86-64); Apple Silicon is not built by that workflow. Linux ARM64 does not establish Raspberry Pi, Recalbox or RetroPie compatibility. Windows and macOS artifacts are experimental and have not been validated in a real frontend. Linux has had a bounded RetroArch smoke test and limited user play testing; automated tests use a simulated frontend. Packaged binaries have not been frontend-tested. Match the frontend and core architectures, and check runtime libraries, sandbox access and OS security restrictions. A successful build does not guarantee frontend compatibility. See the project [README](https://github.com/d-cglt/color-tv-game-simulator#github-actions-builds) for the recorded validation and artifacts.

## Content and selectors

A `.ctg` file is a project-defined selector, **not a ROM**: exactly five bytes, ASCII `CTG1` followed by a single ID from 0 to 15. Other lengths, magic values and IDs are rejected. IDs 0–14 select the 15 existing variants; ID 15 selects Racing 112 as one console entry, not 112 separate games. Racing needs a core with Racing integration, such as 0.1.3 or newer. See the [content format](https://github.com/d-cglt/color-tv-game-simulator/blob/main/docs/CONTENT_FORMAT.md) for the complete ID table and historical naming caveats.

Download and extract the [16-selector archive](https://github.com/d-cglt/color-tv-game-simulator/releases/download/v0.1.4/colortvgame-0.1.4-selectors.zip), then load a `.ctg` file as content. Alternatively, to generate named selectors, open **Quick Menu → Core Options**, change **Generate game files** from **Ready** to **Generate**, then resume the core. The core writes to `Nintendo Color TV-Game` under the frontend's system directory and reports created, existing and failed counts. The directory must be available and writable; there is no fallback. Existing paths are skipped, not overwritten; a failed write may leave a partial file that must be removed before retrying. The core asks the frontend to reset the option to Ready; if it cannot, reset it manually. A saved Generate value does not trigger generation at startup: change it to Ready and then Generate again. Older A/B filenames can be ambiguous across versions; inspect the encoded ID rather than relying on the name. Existing files are not renamed or migrated.

## Controls

These are Libretro RetroPad controls; physical controller and frontend keyboard bindings depend on the frontend configuration. The project's Linux launchers provide their own temporary keyboard mapping, which does not apply to manual launches. The core also polls some keyboard keys directly when the frontend delivers keyboard input.

| Context | Control |
| --- | --- |
| Menus | D-pad navigates and changes settings; A confirms, B goes back. After the idle demo hides a menu, input first restores it without activating a selection. |
| Rally games | Each player's D-pad or left stick moves their paddle. Choose two players or one player with an AI opponent in the launch menu. |
| Shooting | P1 D-pad or left stick aims; A or Y fires. In manual target mode, P2 D-pad or left stick controls the target. |
| Racing | Each player steers with their own D-pad left/right or left-stick horizontal axis. In one-player mode P1 L2 selects the LOW lever position and R2 selects HIGH: LOW is always low speed, while HIGH uses the launch menu's Low/Medium/High SPEED switch. Holding does not repeat; both held together cancel shifting, and the selection stays after release. In two-player mode the shared SPEED switch fixes speed; P2 has steering controls only. Analog deflection controls steering rate; releasing the stick holds position. |
| During play | Start opens the pause menu (resume, restart, exit). Frontend Reset returns to the game's launch menu. |

In rally games and Shooting, stick deflection outside the inclusive 8192 deadzone sets a proportional movement rate, reaching the normal digital speed at full deflection; small movements accumulate over frames. Releasing the stick stops movement. Rally paddles and the shooter stop at their bounds; the manual Shooting target continues to wrap as before. Active analog deflection takes precedence over RetroPad directions, including frontend-generated analog-to-digital directions; center the stick to use the D-pad at full speed. Direct Shooting W/S overrides the stick. AI, demos, automatic targets and menu navigation retain their existing behavior.

Direct keyboard input includes arrow keys for menu navigation, Enter to confirm, Escape to go back, and P for pause. Racing accepts Left/Right or A/D for steering; Shooting accepts W/S to aim and X or Space to fire. RetroArch may intercept keys or map them to RetroPad inputs, so actual behavior depends on frontend configuration.

Rally menus provide speed, paddle sizes and winning score (5–30, default 15); rally games also offer one-player AI difficulty. Shooting has a separate shot limit and manual or automatic target and direction settings. Random Shooting choices resolve at match start or restart and after successful hits. The selector and game menus show previews reflecting applicable game options; after 10 seconds idle the menu hides, and input restores it. Preview randomness is independent of gameplay, although Racing carries the preview road and traffic layout into the race, with the player centered in the selected lane.

Shooting's **TARGET MOVE** defaults to Normal, and direction includes Bounce. **TARGET SIZE** offers Large, Small and Random; **SHOTS SPEED** below it offers Low, High and Random independently of rally speed. Low shot speed uses a small shooter and High a large shooter. Random target size is resolved at match start or restart and after successful hits, not on misses or every frame.

## Core options

Change these in the frontend's core options; saved frontend settings can override the defaults below.

| Option | Default | Behavior |
| --- | --- | --- |
| Generate game files | Ready | Switch to Generate to write named selectors as described above. |
| Vibration | Off | Optional rumble on paddle hits, Shooting fire and Racing collisions; requires frontend and controller support. |
| Shooting fire sound | On | Use the wall-collision sound when firing in Shooting gameplay and previews. Off matches the original console's silent firing. |
| Demo sounds | On | Sound in menu previews; independent of gameplay audio. |
| Selected game at start | Last used | No-content selector starts on the last successfully saved and readable manual selection, First, or Random. Direct content selects its own game. |
| Demo game rotation | Random every 15 seconds | Idle selector rotates randomly, sequentially every 15 seconds, or not at all. Launch menus do not rotate. |
| Rebound behavior | Hit position | Choose hit position or last paddle movement as an approximate rebound model. |
| Anti-stalemate | On | Adds a small angle on the third consecutive horizontal paddle rebound; applies to competitive games and previews. |
| Color | On | Off renders the core image in grayscale. |
| RF filter | Off | Enables an approximate signal effect, before grayscale conversion. |
| RF signal quality | 100% | 0–100% in 10% steps; lower quality adds bleed and noise. Has no effect with the filter off. |

## Geometry and timing

The core reports 320 × 240 base and maximum dimensions, a 4:3 aspect ratio, 60 frames per second and 48,000 Hz audio.

## Persistence

Game-menu preferences, including speed, paddle sizes, score and shot limits, opponent mode, Shooting settings and Racing mode and switches, are stored in `colortvgame-preferences` in the frontend's save directory. Random preferences are stored as Random. Persistence across resets, exits and direct-content or no-content launches requires successful writes and the same readable save directory on subsequent loads. The last manually highlighted selector entry is stored separately in `colortvgame-last-game`; direct launches and automatic demo rotation do not update it. If saving fails, a later load can read an older selection; if the file cannot be read, Last used falls back to the first game.

Version 0.1.9 reads preference formats 1–9 and writes version 9; 0.1.8 and earlier cannot read version 9; 0.1.8 ignores it and retains current or default settings, and a later menu change may overwrite it. Back up `colortvgame-preferences` before upgrading if you may downgrade, and restore it for the older core; changes made after the backup will not be retained.

Version 9 is `CTGP` followed by byte 9 and zero or more entries: a little-endian 16-bit ID, a little-endian 16-bit payload length, and that many payload bytes. IDs 5–33 correspond to the former fixed-record byte offsets and have one-byte values. Missing or invalid known values use factory defaults. Unknown IDs and their payloads are ignored but retained through menu saves, resets, unloads and reloads; their ordering relative to known entries may change. Duplicate IDs, truncated entries and records over 65,536 bytes are rejected as a whole, leaving current session settings intact. The writer emits all known fields and retained unknown entries; if they would exceed the limit the save fails rather than discarding them. Future versions must retain this version-9 framing and stable ID meanings for this preservation guarantee; unknown format versions are rejected. External edits while a game is loaded are not merged.

The frontend must provide an available, writable save directory. Save failures are reported; menu preferences remain in memory for the session. These files are distinct from frontend-managed core options and are not save states. The project reports a user-confirmed RetroArch persistence check across resets, exits and direct/no-content launches; other frontend behavior may differ.

## Racing 112 prototype

Racing is one selectable entry with one-player spacious and narrow road modes and a two-player mode. MORE CARS adds a second opponent; WALL CRASH enables provisional boundary collisions and CAR PATH selects straight or zigzag traffic. BAD ROAD remains disabled. The user confirmed MORE CARS, WALL CRASH, the latest zigzag changes and settings migration in RetroArch; spacing in narrow and two-player modes remains provisional. Two-player mode has side-by-side tracks, a green road and green player cars, one opponent by default and independent scoring per lane. Its colors approximate reference material; they are not verified hardware colors. In two-player mode, both lanes accelerate together at the start to the speed selected by the shared Low/Medium/High SPEED switch. In one-player mode the car automatically accelerates after startup and impacts. Steering, speed behavior, scoring, timing and presentation are provisional approximations. Earlier one-player Racing play and steering and two-player scoring and behavior were confirmed in RetroArch; this does not validate a packaged 0.1.9 binary. Not all frontend paths, packaged binaries or the 112 historical combinations are verified or implemented. The Racing diagnostic fixed-score display uses both left and right values in two-player mode and the left value in one-player mode. See the [Racing design](https://github.com/d-cglt/color-tv-game-simulator/blob/main/docs/RACING_DESIGN.md).

## Frontend features and scope

The frontend must support XRGB8888 video. Save states, rewind, cheats and Libretro memory saves are not implemented; the games are designed for short sessions. Menu preferences are persisted separately as described above.

This is a simulator, not verified original hardware behavior. AI, modern rumble, anti-stalemate and RF effects should not be taken as historical claims. Availability and interactive testing vary by platform; the project does not claim upstream Libretro distribution or acceptance.

## Playlists

Selectors can be loaded manually. Automatic scanning, thumbnails and Explore metadata should not be assumed to work for these project-defined selectors without appropriate database entries, artwork and testing. See RetroArch's [playlist guide](https://docs.libretro.com/guides/roms-playlists-thumbnails/) for manual playlists and scanning.
