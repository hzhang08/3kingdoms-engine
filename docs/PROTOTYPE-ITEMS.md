# Consumables and inventory controls — prototype 0.7.0

Updated 2026-10-07. The reported inability to use 恢复用豆 was caused by the
0.6.0 prototype deliberately disabling item actions. Collection and persistence
worked, but no item-use command existed. This was not a Cao Cao class restriction.

## Playing

Click **道具** in the top toolbar, **查看道具 / P** at the bottom, or press **P**.
The panel shows original icons, quantities and effects. Select a living friendly
unit that has not acted before using an item. Click **使用** (or 1/2/3), then click
that unit or a friendly unit in one of its eight neighboring cells. Green outlines
mark valid targets. The post-movement action menu's **道具** row opens the same
panel. Viewing inventory does not require a selected unit.

- 恢复用豆: restores up to **30 HP**, capped at maximum HP.
- 恢复用米: restores up to **80 HP**, capped at maximum HP.
- 解毒药: removes poison (bit 16), preserving other status bits.
- One successful use consumes one item and the acting unit's action, without
  spending MP or randomness. The recipient's action is not consumed.
- Escape/right-click while choosing a target cancels without spending anything.
- The original manual-use path allows full-HP healing and antidote use without
  poison. The panel explicitly warns that these zero-effect uses still consume
  the item and action. Dead/enemy/distant targets are excluded.

**An already-running 0.6.0 game remains running during installation. Save with
F5 before closing it. Reopen `Start Game.command`, then F9 to load that battle in
0.7.0.** The update does not overwrite user saves or reset the current session.
Independent scenarios continue to have separate inventories; campaign carryover
and additional item types remain outside this change.

## Original evidence

Read-only source edition: Ekd5.exe SHA-256
`199b1b67cf966ead46c49b430b9aab84bdc7d4f0e8dfa8e0ebeec5da280b01e0`.
Cerberus/Ghidra static dossiers cover `0x43dcdc`, `0x41ccdf` and `0x41c762`.
The bounded original-code probe executes:

| Original routine | Evidence used |
| --- | --- |
| `0x43dda8` | Manual successful use commits the actor's action flags |
| `0x41dbbc`, `0x41dc43` | Range 1, item dispatch and success-gated stock reduction |
| `0x45533d`, `0x436586` | Friendly/self target mode and ordered adjacent range |
| `0x41c4e9`, `0x41ccdf`, `0x43c42e` | Healing validity, strength and HP cap |
| `0x41cf61`, `0x418da0` | Antidote clears poison |
| `0x40d955` | Stock decreases once on successful use |
| `0x41c762` | Original Se41 item-use sound and presentation-success marker |

`tools/prototype_items_probe.py` records **324 manual-use/cancel cases and eight
range cases** in `tests/fixtures/original_prototype_items.json`. Manual input,
camera/UI redraw and presentation are host boundaries. The presentation hook
writes only the success field verified in the static dossier; effect, stock,
status and action-commit code execute from the original EXE. Range occupancy is
constructed consistently from lifecycle-2 units; removed units do not occupy a
cell. These fixtures do not validate the Windows mouse dialog or visual timing.

The native effect uses the original item icon and Se41 sound, a Chinese caption,
and green HP numbers. The original sprite/item fade and antidote MEFF44 animation
are **not** reproduced exactly. No full-game or campaign-equivalence claim.

## Saves and JEV

Schema 8 (`prototype-0.8`, `consumables-v1`) adds the item supplement identity and
nonnegative consumed counts. For every supported item, remaining + consumed must
equal the grants from unique claimed scenario events. Consumption never removes
a claim. Revisiting cannot replenish spent items. Corrupt snapshots are rejected
transactionally; zero-effect uses count as consumption.

Schema-7 saves restore exactly in the engine. The frontend enables consumables
with zero consumed counts, retaining HP, turn, positions, RNG, claims and stock.
Old pending JEV choices are invalidated and replanned against current capability
facts; loading does not otherwise advance simulation. The source slot changes
only when the user saves. Earlier unbalanced saves retain their old rules.
A separate `assets/items` font/sound manifest preserves the previous terrain
manifest and save compatibility.

The `caocao-items-v4` JEV context includes remaining inventory, the supported
player effects, range, action cost, and the explicit prohibition on enemies
accessing player stock. Legal enemy choices remain physical/tactic/movement
commands validated by C++. One advisory call (068) reviewed the consumed ledger
and cancellation/restore tests; one live runtime call verified this new context
and saved-choice replay. See [JEV ledger](JEV-DECISIONS.md).

## Verification and reproduction

Evidence is under [`evidence/prototype-items/`](evidence/prototype-items/).
Core tests compare original effects and ranges, self/ally use, stock/action costs,
MP/RNG preservation, illegal targets, full-HP use, canceled movement, spent-item
restore, corrupt snapshots, schema-7 migration, revisits and pending JEV replay.
SDL input/render tests cover both view buttons, item selection, self-heal feedback,
keyboard/right-click cancellation, the post-move menu, and F5/F9 persistence.
The tests use isolated saves and synthetic legal battlefield placements.

```sh
.venv/bin/python tools/prototype_items_probe.py --source /path/to/original/game
.venv/bin/python tools/prototype_item_assets.py /path/to/original/game
python3 scripts/stage-prototype-assets.py
cmake --build build-mac --target caocao_prototype_app prototype_items_tests -j8
build-mac/prototype_items_tests assets/prototype /tmp/items-result.json
CAOCAO_JEV_MODE=off CAOCAO_PROTOTYPE_SAVE_DIR=/tmp/fresh-item-saves \
  build-mac/caocao_prototype_app assets/prototype --items-smoke /tmp/item-frames
python3 scripts/package-prototype-mac.py
```

Use a fresh isolated save directory for each smoke run. Linux uses the pinned
`caocao-engine-build:0.2` container and `xvfb-run`; Wasm uses the existing pinned
Emscripten SDK. `prototype_items_tests ASSETS --resume SNAPSHOT` verifies restore
and the next action across platforms. `jev_items_tests ASSETS ISOLATED_OUTPUT`
is an explicit one-live-call test; ordinary tests are offline. Browser UI testing
is deferred in this native-focused change; WebAssembly compilation/state parity
are checked separately.

Verified release results: macOS, Linux x86-64 and Wasm/Node agree on core trace
`9d7de2abc222e046e9ad5d8858e130fe118b90ac74a3e7924457d22514954ea3`.
Both native platforms and packaged launchers produce the same saved UI state
`792f8f99a299766a7de39fc6250c068e6471486002eea1de0c9a190fc77e60b5`;
cross-platform reload and next action agree. The actual previous 0.6.0 packaged
checkpoint restores, and the new app upgrades it without losing collected items.
Terrain and tactics regression trace hashes remain unchanged. Chinese inventory
and self-heal frames were visually inspected. See [machine-readable verification](evidence/prototype-items/verification.json).
