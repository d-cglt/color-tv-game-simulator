# Renderer regression tests

`tests/renderer_tests.cpp` checks rendering without a running frontend. Its large `expected[]` table contains **hashes**, not images, runtime assets or gameplay code.

## How golden frames work

1. `fixtures::cases()` builds an ordered list of scenarios. Each `Case` contains a game ID, a screen or state (`Kind`), a menu selection, and an `Effect` such as grayscale, the grid or an RF filter.
2. `draw()` creates the corresponding game and renderer state, renders a 320 × 240 frame, and applies the selected effect.
3. `fixtures::hash()` calculates a 64-bit FNV-1a hash over every pixel's four bytes, from low to high byte. This makes the byte ordering independent of host endianness; the unused high byte is included.
4. The test compares the result to `expected[]` at the **same index**. Each table comment records the case fields as numeric values; their meanings are defined by the enums near the top of the test.

For example, `id=0 kind=0 choice=0 effect=0` is game ID 0, a `serve` frame, menu choice 0, with the `plain` effect. IDs are defined in [CONTENT_FORMAT.md](CONTENT_FORMAT.md). The table also contains representative effects and a Racing crash frame.

The initial baseline came from an earlier core renderer using a temporary runner; individual hashes have since been updated for intentional changes. They are regression expectations for this project, **not proof of original-hardware accuracy**. The earlier runner and generated output were temporary build files and are not required to run the current tests.

## Running and maintaining the tests

```sh
cmake --build build-release --target renderer_tests --parallel
ctest --test-dir build-release -R '^renderer_tests$' --output-on-failure
```

A mismatch prints the case index, its fields, the expected hash and the actual hash. A hash indicates that the frame differs, but does not identify the changed pixels or explain whether the change is correct. Use the targeted pixel assertions in the same test, other rendering tests, and visual inspection where appropriate to investigate.

For an intentional rendering change:

1. Verify the actual visual or pixel-level change against the intended behavior. Do not accept a new hash merely to make a test pass.
2. Check the reported case fields and update only the affected hashes. If a scenario is inserted, removed or reordered, update the table in exactly the same order as `fixtures::cases()`; the count check alone cannot detect a permutation.
3. Run the renderer test and relevant focused tests, then the broader test suite. Keep meaningful pixel assertions alongside the full-frame hashes.

The hashes are compact and sensitive to any changed pixel, but can theoretically collide and provide little diagnostic detail. The table and scenario list are currently coupled by index; pairing each scenario directly with its hash would be a possible future cleanup.
