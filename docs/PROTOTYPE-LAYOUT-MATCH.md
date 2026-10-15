# Electron screenshot layout matching — prototype 0.2.0

Updated October 5, 2026 (America/Los_Angeles).

The native C++/SDL prototype now starts with the **22 visible units, original
positions/facing/allegiance colors, 27 animated fires and fourteen confusion
markers** shown in the supplied original-game screenshot. The reference is a
loaded slot-1 position on turn **2/20**, not a new turn-one game. New games now
start at turn 2, with a functional 20-turn limit.

- [Original Electron/Wine reference](/Users/henryzhang/Documents/workspace/caocao-test/docs/screenshots/yingchuan-opening-electron-2026-10-05.jpg)
- [Rebuilt native app capture](evidence/prototype-matched-opening-2026-10-05.jpg)
- [Shared WebAssembly renderer](evidence/prototype-layout-chrome.png)

## Recovered layout

The old prototype had six friendlies/eight enemies and omitted camp fires.
The corrected board has eight friendlies/fourteen enemies. All eight friendlies
remain player-controlled in this simplified mode; side1 uses original orange
allied sprites and cannot be attacked by other friendly units.

Coordinates are zero-based tiles in the original 20×20 map:

| Unit | ID | Tile | Facing |
| --- | --- | --- | --- |
| Cao Cao | 0 | 10,6 | south |
| Liu Bei | 32 | 9,6 | south |
| Guan Yu | 3 | 8,5 | south |
| Zhang Fei | 33 | 12,5 | south |
| Allied infantry | 210 / 211 | 10,16 / 9,16 | north / north |
| Allied cavalry | 258 / 259 | 9,7 / 10,7 | south / south |
| Zhang Bao | 146 | 9,11 | south |
| Zhang Liang | 147 | 10,11 | north |
| Yellow Turbans | 474–485 | See exact fixture | Per saved unit |

The exact identity of visually identical troops was resolved from a **read-only,
hash-gated inspection** of the supplied `Sv00s.e5s`, SHA-256:
`e06a1f9782ecccfcae244568feb0af9f4cbac8570a4e14c00b5939f317aa08be`.
All 115 slot-index bytes align with packed 26-byte records beginning at `0x515`;
22 have lifecycle2 and agree with the screenshot. Fields used for layout are ID,
slot, side, X/Y, direction and status. Fire records occupy five-byte entries from
`0x33`, with 27 active records. Raw records and file offsets are retained in
[the fixture](../tests/fixtures/prototype-electron-layout.json).
This is a checked interpretation of this exact save, not a general `.e5s` decoder
or an execution-verified port of the original save reader. Unknown fields remain
uninterpreted; saved HP/MP are recorded as evidence but not imported into combat.

`tools/prototype_layout.py` verifies the exact source and generates/checks the
compiled `prototype_layout.hpp`. It never writes the source save. Original game,
Electron files and user save slots were preserved.

## Rendering and simplified behavior

- Fire uses all four original `Mark.e5` frames (`0x8bc0`, 48×48 each). The same
  27 cells become original terrain26 for movement, terrain labels, combat and AI.
  Tests check fire exclusions for every friendly unit. Fires persist throughout
  this prototype battle; original spread/extinguishing behavior is not ported.
- Cerberus static dossier `0x43eb1f` identifies the confusion marker at
  `Mark.e5+0x1a2c0`, 16×16, X offset `4 + 24*(tick&1)`. The unchanged indexed
  pixels and original palette produce the visible yellow question mark. The
  frame cadence is provisional. Combined statuses and the complete original
  unit rasterizer are not newly verified.
- All fourteen enemies initially have status8. Prototype recovery skips one
  enemy action and clears the marker. This is explicitly described in the
  information dialog; it is not the original random recovery algorithm.
- The new turn-limit rule ends in defeat after enemy turn20 if enemies remain.
  Cao Cao's defeat and eliminating all enemies still produce their usual outcomes.
- Original unit classes and sprites are reused. Fixed level3 friendlies, level1
  enemies, full initial HP, always-hit clamped damage, instant actions and
  simplified AI remain deliberate differences. Original dialogue, tutorial,
  tactics/items, XP/rewards, audio and campaign fidelity remain outside this build.
- Native toolbar, font, status strip, hover position and viewport differ from
  Windows. This is matching the captured battlefield arrangement, not a claim
  of pixel-identical windows or identical gameplay.

## Saves and validation

New snapshots: schema2 / `prototype-0.2` / layout `electron-slot1-v1`. Layout
identity determines the immutable fire grid; unit confusion and timeout outcome
are explicit saved fields. Restore validates a temporary candidate before
replacing any live state.

Old schema1 / `prototype-0.1` snapshots retain their old 14-unit roster, terrain,
rules and serialization identity. Restart selects the updated board; loading an
old save does not silently add troops or fire. The immutable historical fixture's
next attack still hashes to:
`89f89c67e3f3f86591b84d91a10321ac652e5099ed486dbe75dc4e8dd07524c1`.

Current verification:

- macOS arm64, Linux x86-64 and WebAssembly/Node: ordinary-command autoplay
  reaches victory on turn8; passive defeat occurs on turn13. Every one of 365
  checkpoints round-trips state; next player/enemy action stays deterministic.
- Trace hash on all three platforms:
  `6916cde52483c54ce12fb7e2a8d9cd3d3e4d714e93f93fcf68882ddb419fdcfe`.
- Fresh-process snapshot exchange Linux→Wasm and Wasm→Linux preserves the next
  attack: `908ed536888bb4c0fb0385717c8d560b173b428b553200c56c55266c2ff49f5b`.
- Exact fixture checks all22 unit placements/directions/sides/statuses, all27
  fire positions, blocked movement, confusion skip/clear across saves, turn20
  defeat, legacy continuation/restart and failed cross-ruleset loads.
- Native SDL smoke: map bounds, scaled/letterboxed input, movement/cancel,
  all toolbar actions, modal input blocking, save/load and fresh-process restore.
  Save hash `c704a300a38f556aba6ee2d1104fd45cb800fd68b0a2697f3f2d497c243fa43d`.
- Packaged native app was launched and visually inspected on this Mac. Its
  information dialog is readable and includes the actual simplified rules.
- Chrome smoke confirmed the updated packaged Wasm board, original sprites,
  fires, Chinese labels and 8/14 count. This was a rendering/startup check,
  not a repeated full browser acceptance run. Safari was not retested.
  Console retains the pre-existing Emscripten timer/main-loop performance
  advisory; browser-extension warnings were unrelated to the game.
- Packaged Mac executable passed render/input and fresh-process save tests
  after relocation to a Chinese path containing a space, with only system dylibs.
  Packaged Linux passed the same tests in a network-disabled container with only
  the read-only release and a results directory mounted. Inventories verified.
  Evidence: [Mac package](evidence/prototype-layout-package-macos.json),
  [Linux package](evidence/prototype-layout-package-linux.log).
- 174 staged resources:173 existing decoded resources, plus the losslessly
  extracted confusion marker with source/output hashes. Glyph coverage passes.

Build log: [prototype-layout-build.log](evidence/prototype-layout-build.log).
Native render evidence: `evidence/prototype-layout-macos/` and
`evidence/prototype-linux/`. New portable fixtures: `prototype-linux.snapshot.json`
and `prototype-wasm.snapshot.json`; old fixture remains under
`tests/fixtures/prototype-legacy/`.

One packaging attempt exposed a missing Pillow dependency in Emscripten's Python.
Staging now writes the small standard BMP with Python's standard library; the
final complete build passed without adding a runtime or build dependency.

JEV call060 reviewed backward compatibility and evidence scope. It did not
recover the layout or execute the game. Its request, response, usage and
independent disposition are in [JEV-DECISIONS.md](JEV-DECISIONS.md).

## Rebuild and play

See [PROTOTYPE.md](PROTOTYPE.md) for prerequisites, controls and save locations.
On this prepared workspace:

```sh
python3 tools/prototype_layout.py
scripts/build-prototype.sh all
cmake --build build-mac --target caocao_prototype_app prototype_tests -j8
build-mac/prototype_tests assets/prototype
python3 scripts/package-prototype-mac.py
```

Open `dist/prototype-macos/颍川之战.app`, or double-click
`dist/prototype-macos-executable/Start Game.command`.
