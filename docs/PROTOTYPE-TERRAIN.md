> Historical 0.6.0 report. Item use is now enabled in [0.7.0](PROTOTYPE-ITEMS.md); the original terrain manifest is preserved.

# Terrain recovery and location items — prototype 0.6.0

Updated 2026-10-06 (America/Los_Angeles). Applies to the native four-battle
prototype; the same simulation and presentation code compile for WebAssembly.
The original game, Electron project, original saves and existing prototype slots
are not rewritten by installation or verification.

## Playing

Leave a wounded living unit on recovery terrain until its army's next turn.
兵营 and 鹿砦 restore **20% of maximum HP**, 城池 25%, 关隘 20%, and terrain ID21
(labelled 树木 in this edition) 15%. Recovery rounds down and stops at maximum HP.
It grants no MP. Player/allied units recover together at player-phase start;
enemies recover at enemy-phase start. Poison maintenance precedes recovery.
Entering or passing through the tile does not heal immediately.

End a friendly unit's action on a scripted item location to collect its item.
Wait, attack, tactic and End Turn commit movement; a tentative move or Cancel
does not collect. Enemy occupation does not take the player's item. Pickup
checks use living friendly slots below 35, matching the original conditions.
The original event selector's reverse priority is retained between simultaneously
eligible location events. Inventory is shared by the player's army in that battle.

Recovery has a green `+HP` number and Chinese caption. Pickup has the original
item image and Se14 sound. Press **P** or click **背包** to view collected items.
The panel pauses presentation/enemy advancement and cannot dispatch battle keys.
**Using consumables is not implemented**; the action-menu 道具 command remains
disabled. This update implements acquisition, display and persistence.

## Original item placements

Coordinates are zero-based map tiles, not screen pixels. Each grants one item
once per battle. Terrain names alone do not imply an item or a rare treasure.

| Battle | Tile | Item | Original flag | Grant instruction offset |
| --- | --- | --- | --- | --- |
| 颍川 | 13,12 | 恢复用豆 | 20 | S_00:0x2fce |
| 颍川 | 13,13 | 恢复用豆 | 21 | S_00:0x3017 |
| 颍川 | 10,11 | 恢复用豆 | 22 | S_00:0x3060 |
| 颍川 | 9,11 | 恢复用豆 | 23 | S_00:0x30a9 |
| 汜水关 | 2,11 | 解毒药 | 20 | S_01:0x2132 |
| 汜水关 | 2,8 | 恢复用豆 | 21 | S_01:0x217b |
| 虎牢关 | 8,9 | 恢复用豆 | 20 | S_02:0x1f28 |
| 董卓追击 | 8,9 | 恢复用米 | 20 | S_03:0x23f9 |

The importer validates position condition 37, unclaimed-flag condition 5, item
grant 61 and set-flag 11, including the explicit structural tokens in S_03. Unknown
event shapes fail import. Source identifiers, offsets and hashes are retained.
Unit drops, duel rewards and campaign rewards are outside this location feature.

## Evidence and limits

The supplied Ekd5.exe SHA-256 is
`199b1b67cf966ead46c49b430b9aab84bdc7d4f0e8dfa8e0ebeec5da280b01e0`.

- Original regeneration `0x43cddd` and terrain helpers `0x435182..0x4351da`:
  all 1,913 original maintenance cases and 2,417 status/grade cases still pass.
- New original-code oracle: **480** exact script-condition cases across all
  eight sites, varied friendly/enemy slots, HP, lifecycle, occupancy and flags.
  **18** original `0x40da45` inventory increments also pass without RNG changes.
- Cerberus dossiers for `0x4143c2` (grant interpreter), `0x45991b` (grant
  presentation/inventory), `0x45b5fe` (post-action event scan) and `0x442718`
  were inspected. Dossiers are static evidence, not executable test results.
- The inventory oracle executes condition dispatch and stock increments
  separately; it does not execute every grant-animation body. The original
  Electron/Wine UI was opened during the prior investigation, but automated
  battlefield clicks failed. No new original-UI before/after comparison is claimed.
- Artwork and sound are original. The caption, inventory window and recovery
  number timing are native prototype adaptations. Full original pickup/healing
  animation equivalence is not claimed. Equipment regeneration is separate and
  is not enabled by this terrain-only change.
- Location events run after committed prototype actions, ahead of queued
  prototype conversations. They are not a claim of full original global event
  priority against all outcome, reward, ambush and dialogue scripts.

## Saves

Schema 7 (`prototype-0.7`, `terrain-benefits-v1`) stores the terrain asset identity,
inventory and claimed original event IDs. Claims are unique and checked against
the current scenario; inventory must exactly match their grants. Corrupt,
inconsistent or incompatible payloads are rejected without altering the live
state or overwriting a slot. Presentation is transient; the UI blocks saving
until the committed action's presentation finishes.

Balanced schema 5/6 saves still restore exactly in the core. The native frontend
then enables terrain benefits with empty inventory and unclaimed locations.
Loading does not heal, grant an item, advance time or consume RNG. Future
committed actions can collect occupied unclaimed locations. Old pending enemy
choices are cleared and replanned against the new capabilities. No old-history
pickup guesses are made; the source slot changes only when the user saves.
Earlier unbalanced saves retain their old rules; restart or choose a new battle
to enable current mechanics. Independent battles have independent inventories;
campaign carryover is not implemented.

## JEV

`caocao-terrain-v3` sends terrain rates/timing, each legal destination's expected
next-phase HP recovery, exact item sites/claimed status and player inventory.
Up to eight healing moves are reserved before other choices fill the 48-choice
cap. Enemy inability to collect player loot and unavailable item use are explicit.
Request/cache identity includes these capabilities and facts. Planning does not
consume RNG. The local fallback retains its existing offensive heuristic;
neither it nor JEV claims original AI equivalence.

This change made **two JEV calls**: advisory 067 on migration and cancellation
tests, plus one live runtime decision using the new protocol. The latter chose
毒烟 from a legal attack position, not a healing move. C++ validated and executed
that choice, and the saved pending decision reproduced the same next state.
This proves the data path and replay contract, not that JEV always chooses the
best retreat. Full records and dispositions: [JEV ledger](JEV-DECISIONS.md).

## Verification and reproduction

Evidence: [`docs/evidence/terrain-benefits/`](evidence/terrain-benefits/).
macOS, Linux x86-64 and Wasm/Node pass the same original-condition and engine
tests. Actual SDL render/input tests exercise pickup, cancel, healing, inventory,
save/reload and repeat prevention on macOS and Linux. Existing tactics,
movement/attack, action-menu and historical save regressions also pass.
The browser frontend is compiled; browser UI checks are deferred for this
native-focused update. No full-campaign or full original-battle acceptance claim.

```sh
.venv/bin/python tools/prototype_terrain_assets.py /path/to/original/game
.venv/bin/python tools/terrain_pickup_probe.py
python3 scripts/stage-prototype-assets.py
cmake --build build-mac --target caocao_prototype_app prototype_terrain_tests -j8
build-mac/prototype_terrain_tests assets/prototype /tmp/terrain-results.json
CAOCAO_JEV_MODE=off CAOCAO_PROTOTYPE_SAVE_DIR=/tmp/terrain-test-saves \
  build-mac/caocao_prototype_app assets/prototype --terrain-smoke /tmp/terrain-frames
python3 scripts/package-prototype-mac.py
```

The probe accepts `--source ORIGINAL_DIRECTORY --output OUTPUT_JSON` and defaults
to this Mac's original game path. Linux uses the existing
`caocao-engine-build:0.2` container and `xvfb-run`; Wasm uses the pinned local
Emscripten SDK. `jev_terrain_tests ASSETS OUTPUT` is a separate, explicit
**one-live-call** test; ordinary regression tests are offline.
