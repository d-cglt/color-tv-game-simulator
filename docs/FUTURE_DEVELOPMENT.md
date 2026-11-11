# Future development

This is a proposed roadmap, not a description of implemented features. The starting project contained no code or build system. See [RESEARCH.md](RESEARCH.md) for sources, limits of the evidence and licensing caveats.

## 1. Establish provenance and scope

- Obtain the intended GitHub reference URL and a specific revision. Review its license, file-level notices, third-party components and asset rights; record compatibility and attribution obligations before reuse. Do not copy code with unknown permissions.
- Decide whether the goal is hardware emulation or an explicitly labeled behavioral recreation. Choose one first target, tentatively TV-Game 6, rather than implying all family members share hardware.
- Collect independently verifiable manuals, board/chip evidence and recorded gameplay. Document uncertainty and provenance for each rule, timing, input and audiovisual detail. Do not distribute ROMs, artwork or other assets without verified rights.

**Exit criterion:** an approved target, accuracy goal, evidence ledger and licensing decision. Unknown hardware behavior remains explicitly marked as such.

## 2. Prototype a Libretro core

- Design a portable simulation separate from a thin Libretro adapter. Pin a reviewed `libretro.h` revision and preserve its applicable license notices if incorporated.
- Implement the required exported lifecycle, system information and callbacks according to the canonical header. Investigate no-content launch for the dedicated hardware; verify frontend behavior and provide a clear model/variation selection mechanism. Do not require a placeholder ROM without a justified design.
- Advance simulation deterministically by frames, poll inputs, submit video, and generate synchronized audio; negotiate pixel format and handle unsupported frontend features. Use measured timing when available and label provisional values.
- Map analog and digital input to virtual controls, with two-player support, descriptors, sensible defaults and adjustable sensitivity where necessary. Document the difference between an approximation and verified control behavior.

**Exit criterion:** a core that builds and loads in a real frontend, runs and resets the selected target, responds to both players, and unloads/reloads without leaks. No accuracy claim without evidence.

## 3. Validate behavior and lifecycle

- Add headless tests for rules and variation selection against documented evidence, including reset, scoring and boundary cases. Use reproducible input traces and frame/audio checks where stable reference material is available; record which assertions are inferred.
- Exercise API lifecycle and error paths with a test frontend, including unsupported environment requests, repeated load/unload and no-content behavior. Smoke-test in RetroArch and at least one other available frontend.
- Implement save states only when the complete simulation state can be restored deterministically; otherwise report serialization unsupported. Test round trips and replay equivalence, including audio and input state.
- Run portable builds and sanitizers where available. Measure frame/audio timing and compare with trustworthy captures; avoid presenting a gameplay approximation as cycle-accurate emulation.

**Exit criterion:** automated tests pass, frontend smoke-test results are recorded, and known accuracy gaps are documented.

## 4. Expand carefully

- Investigate TV-Game 15 after the first target: validate shared and distinct game modes rather than assuming internal equivalence. Then assess Racing 112 and Block Kuzushi separately, with dedicated controls and evidence for each.
- Evaluate Computer TV-Game as a separate feasibility project: hardware architecture, ROM availability, licensing and legally supplied assets are unresolved. Do not bundle dumps without permission.
- Add model-specific options, documentation, accessibility and regression fixtures only when supported by evidence. Test each model independently.

### Long-term cross-console scope

This is exploratory work beyond the current CTG-6/CTG-15 scope, not a commitment to a release or an assertion that these systems share hardware. Keep the current CTG content IDs and their meanings unchanged. No content-format changes are proposed now; selection and compatibility for any future console require a separate design decision.

1. **Evidence and feasibility:** Prioritize original manuals, hardware documentation and measurements, and recordings from verified systems; corroborate secondary accounts where possible. Record provenance, uncertainty and gaps in [RESEARCH.md](RESEARCH.md), and use the strongest available evidence to choose which console to investigate first rather than treating reported variation counts as verified implementations.
2. **Separate console investigations:** Assess Racing 112's steering and other controls and racing experience, Block Kuzushi's block-breaking variations and controls, and Computer TV-Game's Othello experience and substantially different reported architecture independently. Establish practical evidence, input, audiovisual and validation targets for each before committing to implementation; Computer TV-Game may require a distinct feasibility decision.
3. **Architecture and prototypes:** Keep console-specific state, rules and timing isolated behind appropriate interfaces rather than forcing a shared hardware model. Reuse existing input, renderer, sound, demo, AI and pause facilities where appropriate, adapting or separating them when a console's needs differ. Prototype one evidence-backed target at a time and retain deterministic behavior and independent regression coverage for existing CTG games.
4. **Validation and possible integration:** Compare each independent implementation against its own evidence, test controls and lifecycle in a frontend, and document approximations and limitations. Only after a prototype is justified should integration, model selection, compatibility and distribution be designed without disrupting existing CTG content.

Implement future systems independently and cleanly from documented behavior; do not copy unlicensed code or assets, require ROMs, or distribute ROMs. ROM availability or rights questions for Computer TV-Game do not imply a ROM-based approach is planned. Detailed mechanics and current implementation priorities belong in [ROADMAP.md](ROADMAP.md) and [RESEARCH.md](RESEARCH.md).

## 5. Packaging and distribution

After the core is functional and licensing is settled, create platform builds and matching Libretro core metadata, document controls, supported models and known limitations, and follow the [official distribution guidance](https://docs.libretro.com/development/cores/developing-cores/). Upstream infrastructure inclusion is a separate review process, not a guaranteed outcome.
