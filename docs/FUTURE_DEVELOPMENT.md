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

## 5. Packaging and distribution

After the core is functional and licensing is settled, create platform builds and matching Libretro core metadata, document controls, supported models and known limitations, and follow the [official distribution guidance](https://docs.libretro.com/development/cores/developing-cores/). Upstream infrastructure inclusion is a separate review process, not a guaranteed outcome.
