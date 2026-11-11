# Roadmap

Checkboxes distinguish implemented work from pending work; they do not establish historical accuracy. Names, variant groupings, rules and behavior remain provisional until supported by reliable evidence. The [content IDs](CONTENT_FORMAT.md) are stable; historical identities remain provisional. The Linux build and headless tests passed. A bounded RetroArch 1.22.2 smoke test loaded ID 2 and ran 120 frames. A separate interactive launcher session reached the Tennis B Singles win screen; broader frontend acceptance remains pending.

## Release priority

The user approved 0.1.0 as the first release and the current source for preparation as 0.1.2, confirmed RetroArch displays version 0.1.0, chose pseudonymous publication with an independence disclaimer, and considers current gameplay and filters playable and acceptable for release. Prioritize a bounded release-readiness pass: build and automated tests, a concise real-frontend smoke test per release platform, licensing and redistribution review, and verified packages and instructions. Use existing successful tests and user confirmations; do not repeat full playthroughs without a specific regression or release blocker. Historical fidelity, additional comparisons, filter refinements and exploratory testing are deferred until new evidence or a concrete problem arises. Existing unchecked research items below are not release blockers by themselves.

## Public label decision and history

The former B group (IDs 2, 3, 6, 7, 10 and 11) is now **Original**; the former A group (IDs 0, 1, 4, 5, 8 and 9) is **Variant**. The selector still lists the Original group first. IDs, gameplay, saved selections and provisional availability suffixes are unchanged. These labels do not establish historical primacy or CTG-6 correspondence.

- [x] User confirmed the RetroArch checks for Original/Variant menu and demo labels, generated filenames, and launching one file from each group.

- [x] Update menus, generated filenames and launcher names while retaining A/B launcher aliases and existing files.
- [ ] Confirm the new labels and generated files in RetroArch.
- [ ] Investigate the user-supplied [Nintendo Encyclopedia wiki](https://w.atwiki.jp/nitendo/pages/9742.html) and compare original diagrams or hardware; see [research notes](RESEARCH.md).

The milestone history below retains the A/B names used at the time of implementation and testing. A temporary inversion preceded restoration of those labels; neither that inversion nor the current renaming changed gameplay or constitutes a new playtest.

Latest development check: a clean Linux Debug build with `-Wall -Wextra -Wpedantic` completed without compiler warnings and all three tests passed. Editor diagnostics were resolved by configuring clangd to use CMake's compilation database.

## Fidelity requirements

- Match the Thomas Visvader simulation's per-game colors, geometry, object placement, score presentation, timing and behavior as closely as practical, using independent implementation rather than copying unlicensed source or assets.
- Use the [Nintendo CTG-15S instruction manual](https://archive.org/details/color-tv-game-15-manual-1977-nintendo-jp) as a continuing primary reference when checking each simulation, alongside original-console photographs and recordings where available. Verify translations and revision differences; the Visvader simulation is a reference, not proof of hardware accuracy. Record discrepancies and approximations in [RESEARCH.md](RESEARCH.md).
- [ ] Audit implemented Tennis B Singles/Doubles and Hockey B Singles against the reference, including score visibility, serves, collision geometry, rendering and sound; replace arbitrary deviations with reference-consistent behavior where feasible.
- [x] Inspect the CTG-15S manual's rebound passage and diagrams: they show four outgoing directions including horizontal for each paddle movement direction, with prior movement affecting stopped paddles.
- [ ] Determine how the hardware chooses the particular rebound direction, movement-history lifetime and exact slopes before replacing the hit-position-only approximation. Investigate long stationary-paddle rallies and possible anti-stalemate behavior on hardware; the optional intervention is a modern approximation, not proven original behavior.
- For every variant, document measurable Visvader reference behavior and check applicable original-manual descriptions before implementing; independently implement and compare, add visual and behavioral regression tests, request an interactive RetroArch playthrough, and record results and remaining discrepancies here before proceeding.

## Milestone 0: Research and verification — in progress

- [x] Record preliminary sources, uncertainties and licensing questions in [RESEARCH.md](RESEARCH.md).
- [ ] Verify model and variation identities, including which six variants belong to CTG-6, against reliable sources.
- [ ] Gather evidence for controls, rules, scoring, timing, display and sound; distinguish observations from assumptions.
- [ ] Resolve reference implementation provenance, licensing and asset rights before reuse.

**Acceptance:** an evidence-backed scope and source ledger, with uncertainties and rights issues recorded. Preliminary research alone does not establish historical verification.

## Milestone 1: Minimal core — implementation present; frontend acceptance pending

- [x] Implement the basic libretro lifecycle, provisional menu, video, input and silent audio callbacks, and five-byte `.ctg` parsing.
- [x] Add bundled selectors and headless tests; build and tests pass on Linux.
- [x] Show provisional titles in the launch menu; direct content loading remains supported alongside the later no-content selector.
- [x] Run a bounded RetroArch smoke test: load ID 2, initialize video and audio, run 120 frames and unload without an error.
- [ ] Verify visible menu rendering, interactive navigation and unload/reload in a real frontend.

**Acceptance:** the content-based core builds and passes automated tests; a recorded real-frontend smoke test confirms loading and the menu and lifecycle behavior. Interactive frontend acceptance remains pending.

## Milestone 2: Tennis B Singles — implementation present; frontend match confirmed

- [x] Record research and provisional rules, controls, scoring and behavior in RESEARCH.md; historical details remain unverified.
- [x] Implement a two-player Tennis B Singles approximation with ball and paddle behavior, scoring, reset and procedural sound.
- [x] Add deterministic tests for gameplay, scoring, boundaries and reset; document approximations.
- [x] Play a two-player game through to 15 and the win screen in RetroArch via a launcher session, using an Xbox controller for player 1 and keyboard arrows for player 2.
- [x] Align Tennis B Singles and Doubles serve wait and score visibility approximately with the simulation and add automated tests; serves and trajectories remain approximations.
- [x] User confirmed that Tennis B displays the score between points after the serve changes.
- [ ] Verify Tennis B restart and a setup with two separate controllers, confirm both variants individually after the serve changes, and compare behavior with stronger historical evidence.

**Acceptance:** ID 2 launches a playable Tennis B Singles implementation, automated gameplay tests pass, and a frontend smoke test is recorded. Historical fidelity is claimed only where supported by evidence.

## Milestone 3: Six CTG-6 variants — starting with Tennis B Doubles

- [x] Implement Tennis B Doubles using the shared Tennis B engine; automated collision, paired-paddle and frontend integration tests pass.
- [x] User reported playing Tennis B Doubles for 15 rounds in RetroArch.
- [ ] Confirm Doubles restart and validate collision behavior against the reference and, where possible, original hardware.
- [x] Implement Hockey B Singles with side-wall rebounds and central goals; automated engine and frontend tests pass.
- [x] User confirmed Hockey B Singles launches in RetroArch and reported an appearance mismatch.
- [x] Compare the court with the Thomas Visvader simulation and museum Hockey B screenshots; update wall and goal-strip rendering.
- [x] Implement Hockey B score visibility during serve delays and an approximate 130-frame delay with deterministic pseudo-random side serves; automated tests pass.
- [x] Document measurable Hockey B reference characteristics and the manual's conflicting rebound description in RESEARCH.md; adjust serve positions and ball color outside the court with regression tests.
- [x] Add varied deterministic Hockey B serve slopes and restricted spawn range; test serve variation and ball clipping at both court edges. Horizontal serves remain excluded as a documented approximation.
- [x] Align Hockey B paddle contact tests with rendered paddle and ball geometry, and clip paddles at court edges; automated collision, clipping and input tests pass. Travel limits remain approximate.
- [x] User tested Hockey B Singles in RetroArch: paddles are partly hidden at travel limits, scores appear between points, and the win screen appears after a match.
- [ ] Compare Hockey B serve distribution, exact paddle travel limits, collision edge cases and win presentation with the reference; paddle-contact alignment was not visually confirmed in the user test.
- [x] Implement Hockey B Doubles with alternating paddle ownership; automated engine and frontend tests pass.
- [x] User confirmed an interactive Hockey B Doubles test in RetroArch, including alternating paddle controls and a match reaching 15 points.
- [ ] Compare Hockey B Doubles collision and visual details with the reference and original hardware evidence.
- [x] Investigate a proposed Hockey goal-mouth edge rebound against the simulation; reject it as unsupported and document the finding.
- [x] Compare Hockey B scoring-side wall-check suppression with the simulation and add regression tests for both goals, opening boundaries, persistence and reset.
- [x] Investigate and independently implement the reference simulation's extreme top/bottom scoring condition in Hockey B; automated boundary and scoring tests pass.
- [ ] Compare exact goal geometry and movement ordering with the simulation; interactive comparison remains pending.
- [x] Research and implement Volleyball B Singles with center-line direction changes and automated tests; comparison details and limitations are in RESEARCH.md.
- [x] User reported Volleyball B Singles seems OK in an interactive RetroArch test.
- [ ] Confirm a complete Volleyball B Singles match and compare its play and appearance in detail with the reference; the user's report did not specify which behaviors were tested.
- [x] Implement Volleyball B Doubles with same-side paired paddles and shared net behavior; automated engine and frontend tests pass.
- [x] User reported Volleyball B Doubles seems OK in an interactive RetroArch test.
- [ ] Confirm a complete Volleyball B Doubles match and compare its behavior with the reference; the user's report did not specify which behaviors were tested.
- [x] Update Volleyball B center-line behavior to check each qualifying update, including repeated checks; deterministic Singles and Doubles regression tests pass.
- [x] User confirmed Volleyball B center-line behavior: vertical direction can change while horizontal travel continues.
- [ ] Recheck each Volleyball B variant individually after the center-line change and confirm detailed reference behavior; the user's report did not specify a variant.
- [x] Research CTG-6 availability: collector documentation identifies Tennis, Hockey and Volleyball in Singles and Doubles, but consulted sources do not establish which CTG-15 A/B variants correspond.
- [ ] Verify which six provisional IDs correspond to CTG-6 using its original game diagrams or recordings and compare with CTG-15; keep the ID-based `6 + 15` filename labels provisional (the affected IDs were formerly labeled B and are now Original).
- [ ] Implement the six identified variants, their selection, rules, controls and scoring, reusing shared behavior where justified.
- [ ] Test each variant and its differences, including reset and transitions; record approximations.
- [ ] Smoke-test all six in a real frontend.

**Acceptance:** six identified variants are playable and individually tested, with frontend results and the evidence or remaining uncertainty for their grouping documented.

## Milestone 4: All 15 provisional IDs, including shooting — pending

- [x] Implement Tennis A Singles using the shared engine and reference-inspired center blocks; automated gameplay and rendering tests pass.
- [x] User reported Tennis A Singles seems OK in an interactive RetroArch test.
- [ ] Confirm a complete Tennis A Singles match and compare gameplay and appearance in detail with the reference; the user's report did not specify which behaviors were tested.
- [x] Implement Tennis A Doubles with shared paired-paddle and center-block mechanics; automated tests pass.
- [x] User reported Tennis A Doubles seems OK in an interactive RetroArch test.
- [x] User confirmed Tennis A Singles and Doubles center blocks can change vertical direction without reversing horizontal travel.
- [ ] Confirm a complete Tennis A Doubles match and compare it in detail with the reference; the user's report did not specify which behaviors were tested.
- [x] Implement Hockey A Singles with a stateful center barrier and automated tests.
- [x] User reported Hockey A Singles and scoring seem OK; confirmed the center opening changes when the ball hits a wall rather than blinking continuously.
- [ ] Confirm a complete Hockey A Singles match and compare barrier transitions in detail with the reference and manual.
- [x] Implement Hockey A Doubles with alternating paddles and the shared center barrier; automated tests pass.
- [x] User reported Hockey A Doubles seems OK in an interactive RetroArch test.
- [x] User confirmed Hockey A Singles and Doubles barrier transitions, scoring and paddle controls.
- [ ] Confirm a complete Hockey A Doubles match and compare it in detail with the reference; the user's report did not specify which behaviors were tested.
- [x] Implement Volleyball A Singles with center blocks and automated tests.
- [x] User reported Volleyball A Singles seems OK compared with the museum's Color TV-Game 15 screenshots, including direction changes at the center obstacles; vertical-angle behavior was not independently confirmed.
- [x] Add approximate Volleyball A center-block vertical angle changes for Singles and Doubles, with deterministic collision tests.
- [x] User confirmed Volleyball A Singles and Doubles center blocks reverse horizontal travel and can change vertical direction.
- [ ] Compare Volleyball A center-block behavior with the reference and confirm a complete Singles match.
- [x] Implement Volleyball A Doubles with paired paddles and center blocks; automated tests pass.
- [x] User reported Volleyball A Doubles seems good in an interactive RetroArch test.
- [ ] Confirm a complete Volleyball A Doubles match and compare behavior with the reference; the user's report did not specify which behaviors were tested.
- [x] Implement Ping-Pong Singles with a center net and automated tests.
- [x] User reported Ping-Pong Singles seems good in an interactive RetroArch test.
- [x] User confirmed a complete Ping-Pong Singles match in RetroArch.
- [ ] Compare Ping-Pong Singles behavior with the reference.
- [x] Implement Ping-Pong Doubles with paired paddles and the shared net; automated tests pass.
- [x] User reported Ping-Pong Doubles is good in an interactive RetroArch test.
- [x] User confirmed a complete Ping-Pong Doubles match in RetroArch.
- [ ] Compare Ping-Pong Doubles behavior with the reference.
- [x] Implement Shooting Game with automatic target, firing, 15-shot scoring and automated tests, based on the reference simulation.
- [x] User reported Shooting Game seems good in an interactive RetroArch test.
- [x] Add manual P2 target control as the default and a separate Visvader-inspired automatic mode, selectable from the Shooting launch menu; automated tests pass.
- [x] User tested Shooting Game with player 2 moving the target in manual mode.
- [x] User confirmed Shooting automatic mode moves the target continuously from top to bottom.
- [x] User confirmed a complete Shooting Game session in RetroArch.
- [ ] Verify Shooting Game manual-control behavior against original hardware.
- [ ] Confirm all 15 variants and game-specific controls and rules against the reference and available original documentation.
- [x] Add a 15-variant headless Libretro harness for content loading, menu transitions, gameplay frames and reset/unload/reload; generated files are checked separately.
- [ ] Verify each `.ctg` and menu entry interactively where needed; investigate historical identity and control differences independently.
- [ ] Add per-ID regression tests and exercise each game in a frontend; document unsupported input devices or approximations.

**Acceptance:** all 15 IDs select playable implementations, including shooting, with passing per-ID tests and recorded frontend checks. This does not by itself verify the historical mapping or accuracy.

## Milestone 5: Menu and controllers — pending

- [x] Support launching the core without a `.ctg` file and display a game-selection menu for all 15 variants; retain direct `.ctg` loading and its game-specific launch menu. Headless tests cover navigation and loading.
- [x] Render horizontally centered menus over the selected or opened game's background, updating the background as the selection changes.
- [x] Provide a no-content launch script with native and RetroDECK frontend argument tests.
- [ ] Verify no-content launch, centered menus, navigation and return behavior interactively in RetroArch.
- [x] Add a six-row game selector with arrow indicators, game menu with Return/Start and inline options, translucent black panel, pause menu with Resume/Restart/Exit, and matching game-over menu with Restart/Exit; action buttons no longer exit gameplay, B does not dismiss pause, and Start always resumes without activating the selected row.
- [x] Move Shooting target selection directly into the game menu as a row changed with left/right and draw matching directional arrow icons; other settings remain pending.
- [x] Expand Shooting settings: TARGET Manual/Slow/Normal/Fast/Random; direction Down/Up/Random, grayed out in Manual; and blinking Off/On/Random. Default to Manual/Down/On. Resolve random choices at match start or restart and after successful target hits, not on misses or continuously; fixed choices and the demo remain unaffected. With blinking Off, targets are continuously hittable. Slow and Fast are approximations, not historically verified. Add automated tests.
- [x] User confirmed in RetroArch that Shooting's Random behavior works after successful hits and makes the game more interesting.
- [ ] Verify the remaining new Shooting settings individually in RetroArch.
- [x] Add held up/down menu navigation repeat for D-pad, analog and keyboard with automated tests.
- [x] Center the game-selection list and adjacent scroll arrows within the translucent panel. Show all game-menu items without arrows when there are seven or fewer; use five-row scrolling for longer menus. Add layout regression tests.
- [x] User confirmed pause Resume/Restart/Exit and game-over menu in RetroArch.
- [ ] Verify selector scrolling, held navigation and game-over restart/exit choices individually in RetroArch.
- [x] Add attract/demo previews behind the game selector and launch menus: animate without scores, alternating both competitive players between Hard and Easy AI every three seconds, hide the menu and show a demo prompt after 10 seconds of inactivity, consume wake input, and keep preview state separate from real matches and settings. Add headless regression coverage.
- [x] User confirmed attract/demo timing, input, appearance and normal game start in RetroArch, including the selector and launch menus.
- [x] Add game-menu Low/High speed and independent P1/P2 Large/Small paddle-size settings across all variants, with automated motion, collision and menu tests; add the missing Z font glyph and clarify confirm/pause input descriptors.
- [x] User reported Low/High speed and paddle-size settings work in a Tennis Doubles test.
- [ ] Verify speed and paddle-size settings across other variants and compare measurements and behavior with original hardware and the reference.
- [x] Add SCORE for competitive games and a separate SHOTS setting for Shooting: 5, 10, 15, 20, 25 or 30, defaulting to 15. Automated tests cover scoring, result screens and restart.
- [x] User tested a 30-point winning score and reported it seems good.
- [ ] Verify other winning-score settings and the Shooting shot limit interactively across representative games and AI modes.
- [x] Persist game-menu preferences in the frontend save directory across reset, exit and subsequent direct `.ctg` and no-content launches: speed, paddle sizes, score, shot limit, per-game opponent mode, and Shooting target, direction and blinking choices. Save Random preferences rather than transient outcomes. This is independent of core options, last-game selection and unsupported save states. Failed saves are reported and retried while retaining session values.
- [x] Add automated persistence coverage for launch paths, reset, reload, preference types, per-game modes and invalid or unavailable storage.
- [x] User confirmed game-menu settings persistence in RetroArch across reset, full exits and direct `.ctg` / no-content launches.
- [x] Investigate paddle rebounds without changing gameplay: the CTG-15S manual describes dependence on current or preceding paddle movement, whereas the reference and current implementation use contact position. Exact slope selection and motion-history lifetime remain unverified.
- [ ] Evaluate a separately selectable movement-history rebound prototype against current hit-position behavior; test all paddle ownership paths, AI and horizontal-rally edge cases before deciding whether to change defaults. Do not claim hardware accuracy without evidence.
- [ ] Design optional rally behavior choices: a baseline based on the Thomas Visvader simulation and an optional modern long-rally intervention (final name to be decided). Keep intervention off by default, clearly distinguish it from original behavior, and document how it works.
- [ ] Test speed and rally options, including deterministic behavior.
- [ ] Design and document player and game-specific controller mappings, including suitable alternatives for paddles or shooting input where available.
- [ ] Test navigation, controller configuration, player assignments and input edge cases in supported frontends.

**Acceptance:** launching without content offers all 15 games; opening a valid `.ctg` still selects its game directly. Menus are horizontally centered over the selected or opened game's background, remain readable, and work with a gamepad. Documented controls work for supported games and players, and real-frontend results and limitations are recorded.

## Milestone 6: AI — implementation present; balancing and frontend validation pending

- [x] Add an aligned MODE row beneath Start with 2 PLAYERS and 1P Easy, Normal and Hard for all 14 two-player variants; Shooting remains separate. Expert was removed after user feedback.
- [x] Switch Normal to Easy after 15 seconds of active rally time; Hard starts with 30 seconds Hard, then repeats 15 seconds Normal and 15 seconds Easy. Reset on each point; pause and serve waits do not count.
- [x] Control P2 through the same paddle input interface, with deterministic prediction and error; add engine and frontend tests for controls, resets and mode isolation.
- [x] User initially reported Easy, Normal and Hard seem balanced; after the revised Hard rally timing, user reports Hard is challenging but possible.
- [x] Add a Hockey-specific Easy AI adjustment: increase target error without changing other games or Normal/Hard behavior. Fallback to Easy uses the same Hockey adjustment. Deterministic opponent tests cover the adjustment; user reported the selected Easy setting feels better.
- [x] User reports Hockey B Easy is balanced in Singles and Doubles after the Hockey AI adjustment; Hockey A Easy was also reported better.
- [ ] Continue monitoring Hockey AI balance across variants and review other games separately if needed.
- [ ] Validate difficulty across paddle arrangements and obstacles in a frontend. Prediction currently approximates obstacles and replans after collisions.
- [x] Leave save-state and rewind support disabled by project decision; deterministic opponent behavior remains tested independently.

**Acceptance:** applicable games offer a usable computer opponent, AI tests pass, and behavior, settings and historical uncertainty are documented.

## Milestone 7: Libretro options and integration — in progress

- [x] Add a **Vibration** core option, Off by default: briefly vibrate the relevant player's controller on paddle hits and shots, but not automatic serves. Use Libretro rumble where available, exclude demos and AI-controlled players, and handle unsupported frontends or controllers gracefully. Automated tests cover events and lifecycle; this is a modern convenience rather than an original controller feature.
- [ ] Confirm vibration and the Off default with a supported controller in RetroArch.

- [x] Use the selected core display name **Color TV-Game Simulator** instead of the provisional name; retain the binary filename, option keys, generated content names and save paths. Add API regression coverage.
- [x] User confirmed the new core display name in RetroArch after rebuilding the stale launcher core.
- [x] Deliberately do not support save states: serialization size is zero and save/load calls fail.
- [x] Disable rewind and automatic state save/load in project launcher sessions without changing global RetroArch settings.
- [x] User confirmed RetroArch starts at the game selector despite auto-save/load settings and that the rewind button has no effect.
- [x] User confirmed a manual save-state attempt reports that save states are not supported by the core.
- [ ] Verify manual load failure messaging and launcher rewind settings in a real frontend; manual core loading relies on the frontend honoring unsupported serialization.
- [x] Add a core option to generate 15 named `.ctg` files on demand under `Nintendo Color TV-Game` in the frontend system directory, with provisional console-availability labels, preservation of existing files, and generator and core-option tests, including automatic reset where supported and manual-reset fallback. Shooting Game has no unnecessary variant label.
- [x] Verify generation of all 15 valid descriptors in RetroDECK on Linux and confirm that the generated Tennis B Singles file opens its launch menu in RetroArch. Confirm the core option reports all 15 existing files and resets to Ready after generation.
- [x] Launch all 15 generated files individually in RetroDECK on Linux; each reached 320×240 video output without RetroArch error logs. This smoke test did not start or verify gameplay.
- [ ] Verify generated games and frontend messages in RetroArch on Windows; add other appropriate core options and metadata.
- [x] Add **Selected game at start**: Last used (default), First, or Random. Persist highlighted selector choices as the user navigates in the frontend save directory, fall back to First if unavailable or invalid, and leave direct `.ctg` launches unchanged. Add startup and persistence tests.
- [x] User confirmed startup selection works in RetroArch, including remembering the highlighted game without entering its options.
- [x] User confirmed Last used persists after a full RetroArch restart on Linux.
- [ ] Verify startup selection and persistence after a full RetroArch restart on Windows.
- [x] Implement **Demo sounds**: On/Off, default On, with live updates and tests covering preview sound and isolation from gameplay audio.
- [x] User confirmed the Demo sounds core option works in RetroArch.
- [x] Add **Demo game rotation**: Off/Every 15 seconds, default Off. Rotate the highlighted game while the selector is idle; input restores the current game, and automatic rotation does not change the saved last selection. Launch menus and gameplay do not rotate. Add headless regression tests.
- [x] Extend **Demo game rotation** with **Random every 15 seconds** as the default, retaining Off and sequential Every 15 seconds. Preserve wake behavior and avoid changing the saved last selection during automatic rotation; add regression tests.
- [x] Restrict demo rotation to the hidden selector menu, starting its 15-second timer when the menu hides; show the game title and variant at the bottom of hidden demos. Add headless regression coverage.
- [x] Group the selector by provisional console availability: CTG-6 and CTG-15 shared games first (IDs 2, 3, 6, 7, 10, 11), followed by CTG-15-exclusive games (IDs 0, 1, 4, 5, 8, 9, 12, 13, 14), in sport and singles/doubles order within each group. First starts at the first shared game; sequential demos follow display order. Keep content IDs and persisted last-selected IDs unchanged, with headless order, persistence and rotation coverage.
- [x] User confirmed demo game rotation, the hidden-screen title and wake behavior in RetroArch.
- [x] User confirmed automatic demo rotation does not change the saved last selection after restarting the core in RetroArch.
- [x] User tried Paddle movement in RetroArch and could feel the difference; both rebound options remain available because the preferred default is undecided.
- [x] User confirmed potentially infinite rallies with Paddle movement when paddles repeatedly produce horizontal rebounds.
- [x] Add **Anti-stalemate**, On by default and switchable Off, to competitive games and demos. On the third consecutive horizontal paddle rebound it adds a small vertical angle at the hit, without a timer; this is a modern approximation, not proven original hardware behavior. Apply changes live and retain the setting when engines are reconstructed.
- [x] User confirmed the paddle-hit-based anti-stalemate behavior in RetroArch.
- [ ] Compare anti-stalemate behavior with any original hardware evidence.
- [ ] Continue validating **Rebound behavior** and compare against hardware evidence. Hit position remains the unchanged default; Paddle movement is an approximate alternative applied live to gameplay and demos. Original behavior is uncertain.
- [x] Add **Color**: Off/On, default On. Off presents the complete core image in grayscale, including menus and demos, without affecting game state; add live-switch regression tests.
- [x] User confirmed Color Off with the RF filter at 100%, 50% and 0% in RetroArch, including grayscale output and menu readability.
- [x] Add a provisional **RF filter**: Off/On, default Off. The scanline-local YIQ approximation limits luma/chroma bandwidth and adds phase-dependent cross-color; Off preserves existing output and Color Off converts the filtered image to grayscale. This is not a verified RF model.
- [x] User confirmed the RF prototype effect works in RetroArch.
- [x] User reported no noticeable performance issues with the filters during testing.
- [ ] Compare the RF prototype visually and measure performance in RetroArch on Linux and Windows; verify original hardware characteristics before claiming historical accuracy. The user's observation is not a formal benchmark.
- [x] Add **RF signal quality**: 0–100% in 10% steps, default 100%, active when the RF filter is On. Lower values progressively add color bleed and moving noise; 100% retains the clean RF prototype. Add regression tests for output, grayscale and gameplay isolation.
- [x] User confirmed RF signal quality works in RetroArch.
- [x] User confirmed RF quality appearance and menu readability at 100%, 50% and 0% with Color Off in RetroArch.
- [ ] Prototype and compare a portable in-core analog-signal filter for the core-option controls: RGB-to-luma/chroma encoding, bandwidth-limited composite mixing and decoding, with an optional quality-dependent RF degradation/noise stage. Compare with RetroArch's existing NTSC/RF shader presets; benchmark and test Linux/Windows, color/monochrome interaction, determinism and legibility. Avoid applying a second RF effect when a frontend shader already supplies one.
  - Research: [Blargg's NTSC libraries](https://www.slack.net/~ant/libs/ntsc.html) use optimized NTSC signal filtering; the author lists uses in Nestopia, Mednafen and bsnes. The RGB-input `snes_ntsc` is a potentially relevant comparison, but is system-specific and LGPL; review licensing and suitability before using any code. [RetroArch's NTSC shaders](https://github.com/libretro/slang-shaders/tree/master/ntsc) include a multi-pass RF preset. Shaders are a useful visual benchmark but are frontend-controlled rather than a portable core option. The [Stella 7.0 manual](https://stella-emu.github.io/docs/index.html) identifies Blargg TV filtering and lists Composite and Bad Adjust presets, plus adjustable sharpness, resolution, artifacts, fringing and bleed; these are useful comparison points for the quality control, not proof of physical RF tuning simulation. Neither source establishes the original console's exact RF output.

**Acceptance:** options and metadata work as documented, unsupported state operations fail safely, and frontend behavior is recorded.

## Milestone 8: Testing and releases — in progress

- [x] Publish the source under `d-cglt` and create the [0.1.0 GitHub release](https://github.com/d-cglt/color-tv-game-simulator/releases/tag/v0.1.0) with the Linux x86-64 archive. RetroArch Core Downloader inclusion is a separate, unsubmitted process.
- [x] Publish [0.1.1](https://github.com/d-cglt/color-tv-game-simulator/releases/tag/v0.1.1) with CI-tested Linux, Windows x86-64 and macOS Intel artifacts; Windows and macOS are labeled experimental pending frontend tests.

Linux release-readiness review: on 2026-11-10 a CMake Release build succeeded and all 8 CTest tests passed, including assembling, extracting, checking and dynamically loading the Linux package. A standalone Linux x86-64 archive was also assembled and its contents listed. The package includes the project license, the vendored libretro header's MIT notice and installation/usage instructions. Prior user-confirmed RetroArch testing covers the playable core; the earlier packaged-binary frontend smoke test and its isolation caveat are recorded below; it predates the 0.1.0 version update. Version 0.1.0 is approved for the first release; the rebuilt versioned package has not been frontend tested. The user confirms being the sole contributor and chose MIT licensing and pseudonymous publication; a pseudonym has not yet been specified. Before distribution, review attribution, target runtime compatibility, and suitable frontend metadata. Manual core loading has already worked without a `.info` file; metadata requirements depend on the distribution target. At that review, Windows was unverified and no release revision was verified.

- [x] On 2026-11-10, extracted the core from `build-release/colortvgame-linux.tar.gz` into a temporary directory and ran bundled `content/slot-02.ctg` with RetroDECK Flatpak's RetroArch 1.22.2, `--max-frames 120` and a 12-second timeout. RetroArch exited with status 0, logged content loading, XRGB8888, 320×240 video, a Wayland/OpenGL context, synchronous PulseAudio startup and core unload. This was unattended: controls, audible sound and visual correctness were not verified. Temporary files were removed.
  - Isolation caveat: despite a temporary HOME, XDG directories and explicit temporary config with `config_save_on_exit = false`, RetroArch logged writing its persistent RetroDECK `config/retroarch/playlists/builtin/content_history.lpl` and `config/retroarch/config/Color TV-Game Simulator/Color TV-Game Simulator.opt`. Their prior contents were not captured, so they were not restored. It also logged persistent save/state paths, but skipped SRAM loading; no save/state write was logged. Avoid claiming this run left permanent frontend files untouched.
- [x] GitHub Actions [run 42813010356](https://github.com/d-cglt/color-tv-game-simulator/actions/runs/42813010356) built and passed automated tests on hosted Linux x86-64, Windows x86-64 and macOS Intel runners and uploaded artifacts. The macOS binary architecture was checked. Windows and macOS RetroArch frontend validation remains pending; Apple Silicon is not built.

For 0.1.2, Linux is the established target; Windows x86-64 and macOS Intel remain experimental, not frontend-validated platforms. The recorded successful CI run predates the 0.1.1 and 0.1.2 source changes. User-confirmed settings persistence and Shooting Random behavior are recorded above; vibration remains unconfirmed in a real frontend. See the [draft release notes](RELEASE_NOTES_0.1.2.md). CI artifacts are separate from the Linux release package; prepare and verify release assets before distribution.
- [ ] Run the existing automated suite and build release artifacts for each intended release platform; record results. Portability and sanitizer checks are optional unless needed to investigate a failure.
- [ ] Use existing frontend confirmations where applicable and perform only a concise smoke test of content loading, controls, video, audio and lifecycle on each intended release platform; investigate failures rather than repeating complete playthroughs.
- [ ] Review licenses and asset rights; prepare versioned artifacts, core metadata, installation instructions, controls and known limitations.
- [ ] Record test environments and results, and verify packages before distribution.

**Acceptance:** passing tests and reproducible builds for the platforms actually shipped, recorded essential frontend checks, and release packages with documented licensing and limitations. Windows and macOS Intel remain experimental targets; do not claim frontend support without validation. Upstream acceptance is a separate process and is not assumed.

## Future milestones — research first, after current release priorities

These are future projects, not release blockers or claims of verified hardware behavior. Names, counts, controls and mechanics below are research targets or tentative interpretations. Consult manuals, advertisements and commercials, photographs, recordings and original hardware where available; record sources, translations, discrepancies, rights and unresolved questions in [RESEARCH.md](RESEARCH.md). Implement independently without copying protected code or assets. Split work into small, independently testable increments and revise plans when evidence warrants.

## Milestone 9: Shared multi-system architecture — pending

- [ ] Design a system registry and separate model-specific state, rules, rendering, audio and controls; share only genuinely common frontend, timing and utility code. Do not assume the consoles share hardware or game engines.
- [ ] Plan model and game selection, direct content loading, menus, demos, input descriptors, configuration, reset and lifecycle isolation. Preserve existing 0–14 IDs, five-byte `.ctg` descriptors, generated files, saved selections and direct-launch behavior; design and document a backward-compatible way to identify new systems without reinterpreting existing content. Do not assign new stable IDs or change the format until the design is reviewed.
- [ ] Define mappings and accessible alternatives for steering, paddles, buttons and board selection, including analog and digital input, calibration, player assignment and unsupported devices. Consider the existing Vibration option separately for each system and gracefully handle unavailable rumble.
- [ ] Add compatibility and isolation tests for old content, selection persistence, invalid descriptors, reset, unload/reload and switching systems; verify menus and representative controls in a real frontend.

**Acceptance:** a documented, backward-compatible system design and tested integration support new independent models without regressions to existing IDs, content or gameplay; frontend limitations and approximations are recorded.

## Milestone 10: Racing 112 — pending

- [ ] Research the reported seven base games and 16 variations each (112 combinations): identify and name each base game, settings and valid combinations from primary evidence rather than treating 112 as 112 independently established games. Record the evidence and uncertainties for each.
- [ ] Research analog steering response, the two-position gear lever, two paddle controllers, start/reset and configuration switches; verify controller attachment and player modes. Map controls to analog and digital alternatives, including gamepad sensitivity, calibration and player assignment.
- [ ] Recreate the seven reported configurations and all 16 combinations of four binary difficulty switches, exposing all 112 advertised combinations with intuitive settings and original numbering where verified. Research wide/narrow roads, speed combinations, independent two-player tracks and fixed speed, straight/zigzag traffic, optional roadside collisions, bad-road steering hazards, one/two opponents, road scrolling and markers, crashes, scoring, timers, game-over, restart, graphics and sound. Investigate optional AI assistance only as a clearly labeled modern addition; do not assume any details verified.
- [ ] Test every base game and its 16 combinations or document evidence-backed exceptions; cover steering limits, digital and analog input, multiplayer, collision boundaries, scoring, timing, configuration, restart and transitions with deterministic tests. Compare visuals, sound and play with recordings and hardware where available, then record representative real-frontend playthroughs and limitations.

**Acceptance:** the researched mode/variation matrix is selectable and playable, with automated coverage of all supported combinations, documented control mappings and frontend checks; any unverified mechanics and unsupported combinations are explicit.

## Milestone 11: Block Kuzushi — pending

- [ ] Research the six secondary-source names and distinguishing rules: Standard Block Kuzushi (traditional), Easy (missing row), Safe (protective blocks), Block Through (timed, ball passes through), Block Flash (flashing targets), and Block Kill (a reported Through/Flash combination with a different layout). Verify names, translations, arrangements and rules against manual illustrations and gameplay before implementation.
- [ ] Research rotary paddle and serve button, ball-count choices reported as 3/5/7, block layouts and colors, rebound physics, score/high score, time limits, flashing timing, game-over, restart and sound. Verify the reported ceiling-triggered half-width paddle and restoration after a lost ball.
- [ ] Analyze the period commercial frame by frame for layouts, paddle and ball movement, block destruction, flashing, scores, colors and proportions. Treat edits, staging and speed changes as possible; cross-check against manual illustrations and gameplay recordings rather than inferring exact timing or physics from the commercial.
- [ ] Independently implement a basic playable block-breaking mode, then add each of the other five researched modes and their distinct layouts and rules in manageable increments. Preserve documented uncertainties and distinguish optional modern behavior from historical behavior.
- [ ] Add deterministic tests for all six modes, block removal and special blocks, collision and rebound edges, paddle resizing, score, ball count and exhaustion, mode selection, reset and input; compare with the commercial and other evidence where applicable and record real-frontend checks for each mode.

**Acceptance:** all six evidence-identified modes are selectable and playable, with their names, differences, ball-count behavior, automated tests and frontend results documented; unresolved details remain explicitly tentative.

## Milestone 12: Computer TV-Game — pending

- [ ] Research Computer TV-Game and Computer Othello using manuals, hardware, recordings and reliable board documentation. Investigate its reported relationship to the arcade board, but treat the home system and arcade game as distinct targets until similarities and differences are evidenced.
- [ ] Design an independent, original Othello implementation requiring no ROM, dump or copyrighted assets; do not bundle or depend on arcade ROMs. Research board presentation, controls, legal moves, flipping, passes, scoring, end conditions, computer opponent, difficulty, timing, visuals and sound before choosing approximations. Document provenance and any fidelity limitations.
- [ ] Implement board interaction and rules first, then an independently designed computer opponent and model-specific presentation and controls. Do not claim arcade-perfect behavior or original AI equivalence without evidence.
- [ ] Test legal and illegal moves, directional flips, passes, full board and no-move endings, scoring, AI legality, reset, input and deterministic behavior where applicable; compare with reliable home-system evidence and record a complete frontend game and remaining differences.

**Acceptance:** an independently implemented, ROM-free playable Computer TV-Game approximation passes rules and opponent tests and a recorded frontend game; its relationship to the arcade version and all unverified behavior are clearly documented.
