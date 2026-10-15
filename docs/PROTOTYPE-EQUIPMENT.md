# 宝物、战后装备更换和武将情报 — 0.10.0

2026-10-08. Native macOS and Linux share this implementation with the WebAssembly build.

## Playing

- Click **宝物 G** above the battlefield, or press **G**. 宝物 shows owned collectible treasures; **全部装备** also includes ordinary weapons, armor and accessories. Each row shows its original icon, name, level/experience when applicable, and whether it is in the 行囊 or equipped by a named officer. Select a row to read the original item description.
- **查看武将装备** opens the owner's information. For a bag item it opens Cao Cao; use 上一武将 / 下一武将 to choose another officer.
- As requested, **equipment changes are available only between battles**: finish the victory dialogue, choices and rewards, click **整备装备** on the victory screen (or open a player's 装备 tab), then choose **装备更换 G**. Pick 武器 / 护具 / 辅助, choose an eligible bag item or 卸下当前装备, and confirm. Cancel leaves the snapshot unchanged. Battle-time inspection remains read-only.
- To transfer gear between officers, remove it from its current owner first, then equip it on the recipient. Weapon/armor class permissions and accessory restrictions use the recovered edition rules. Enemy and allied equipment can be inspected but never taken.
- Save with F5 after closing the inspector, or F6 to select a slot/export a backup. Continue with 下一战 after completing your preparations. B starts an independent preset and intentionally does not carry campaign possessions.
- Right-click or Control-click a unit, or press V, for **能力 / 装备 / 策略 / 武将列传 / 部队特性**. Tab, Left/Right or 1–5 change pages; Up/Down and the bottom buttons change officers. Right-click retains cancellation priority during a pending movement/action.

## Ownership and saves

Schema10 / engine `prototype-0.10` adds an explicit bag of original `[item ID, level, experience]` triples. Swaps preserve the full triple, return the replaced item to the bag, clamp current HP/MP to the new caps, and do not heal, advance turns, or consume RNG. Combat and JEV requests read the resulting actual unit records. Pending AI choices/caches are invalidated after a confirmed change.

A conservation check requires player-equipped items plus bag items to equal starting possessions, carried possessions and verified rewards. Script grant flags and the cumulative treasure ledger remain immutable acquisition history. Duplicated/missing items, consumables in the equipment bag, incompatible slots/classes and invalid gear experience are rejected atomically. Campaign entries carry the bag as well as each party record. Possessions from a nondeployed member are retained in the bag.

The engine still roundtrips schemas1–9 under their original rules. The frontend upgrades a loaded schema9 save in memory to enable equipment, making previously earned unassigned gear available once; it changes neither battle state nor reward flags. Earlier rulesets retain their older behavior. Existing slot files are not overwritten merely by loading. New schema10 saves require this version; export backups for older installations before overwriting a slot. Original Windows `.e5s` files are untouched.

## Original evidence and fidelity

Read-only PE dialog166 and its child resources178/179/200/205/284 provide the layout and five pages. Cerberus dossiers establish the tab order (`0x478f7b`), header (`0x478688`), equipment (`0x476eab`), biography (`0x4783c1`) and class information (`0x477b62`). Original screenshots of the ability, equipment and biography pages are in `docs/evidence/equipment-ui/`.

The importer reads GBK Imsg records0–103 for items,350–402 for classes,450–623 for named biographies. The latter two offsets are established at `0x40bfc6` and `0x40bfe8`. Base attributes are doubled as in `0x40709a`. Class grade/type labels are read from original PE pointer tables; Hitarea.e5 supplies the original range diagrams. The main save-bound asset manifest is unchanged: these are separately hashed presentation assets.

The patterned background was recovered through `0x4710c8 →0x40a83b →0x40aa39`; `0x40af77` loads Logo.e5[8+theme] into Mark+0x1abc0. Theme3 / Logo.e5#11 matches the original Electron screenshot. The48×48 pixels and original palette-based tint are imported read-only.

The native window reproduces the original left portrait/class/HP/MP/status column, two-row five-tab arrangement, ability/equipment sections, original descriptions, class grades and range art. Native font rasterization and window decoration differ from Wine/macOS. The strategy list uses explicit local paging and the post-battle equipment button is a prototype extension; exact pixel identity of all Windows controls is not claimed. Unsupported original spells remain unsupported in combat. This update does not establish full-campaign equivalence.

## Validation

- `prototype_equipment_tests`:40,560 slot/class/item combinations agree with the existing executable-verified equipment routine; between-battle gating, NPC protection, reward migration/idempotence, remove/re-equip, XP-preserving officer transfer, two campaign transitions, seven corrupt snapshot cases, and identical resumed NPC actions/RNG.
- The same equipment trace has SHA256 `66cf23920bda076c89184f4d9d742f8dbe462f0b46e54aa5a04bae7b93bd395f` on macOS, Linux x86-64 and WebAssembly. A concrete macOS snapshot is also restored by the other builds.
- Native unit-information smoke renders130 units ×5 tabs and checks scaled input, modal isolation, move cancellation, paused enemy execution, and save roundtrip.
- Equipment UI smoke checks treasure ownership, reward display, rejected battle-time swapping, cancel/confirm, manual-slot save/load, and five-tab clicks at720×840 and1200×960. Linux uses SDL's software/dummy renderer.
- Existing revised-battle, campaign and unit-detail data regressions pass. Fixture-generated victories test menus and transitions; they are not new full-playthrough evidence. Browser UI was not re-run for this native-first request; WebAssembly compilation and deterministic state checks are covered.

All tests use isolated saves and disable runtime JEV network traffic. One JEV advisory call was made for the inventory/conservation contract and regression selection; it is logged as072 in `JEV-DECISIONS.md`. Model advice was not treated as original-game evidence.

## Rebuild

```sh
.venv/bin/python tools/prototype_unit_assets.py --game-dir /Users/henryzhang/Downloads/sgzccz
python3 scripts/stage-prototype-assets.py
cmake --build build-mac --target caocao_prototype_app prototype_equipment_tests -j8
build-mac/prototype_equipment_tests assets/prototype
python3 scripts/package-prototype-mac.py
```

Linux uses the existing `caocao-engine-build:0.1` x86-64 container and `build-linux`. WebAssembly uses the pinned `.cache/emsdk/emsdk_env.sh` and `build-web`. Build both before `python3 scripts/package-prototype.py`. Source, dependency setup and the original game remain separate.
