> **Updated in0.10.0:** The current inspector has five original pages and the left-column layout; equipment is changeable only after battle rewards. See [current controls and evidence](PROTOTYPE-EQUIPMENT.md). The following records the earlier0.8.0 implementation.

# Unit details — native prototype 0.8.0

2026-10-07. The detailed viewing interface now provides **能力 / 装备 / 策略**
for living player, allied and enemy units in all four standalone battles.

## Opening and controls

- Control-click a unit to inspect it without selecting or moving it.
- Select a unit, then press **V** or click the bottom **武将 V** button.
  V also inspects a hovered unit when no action picker is open.
- Ordinary right-click opens details on a unit when no move/action is pending.
  During a pending move or action, it retains its existing cancellation behavior.
- Click the tabs, use **1/2/3**, or **Tab / Left / Right**. Previous/next officer
  buttons and Up/Down cycle living units. Strategy pages use buttons or PgUp/PgDn.
- **Esc / Enter / right-click / 确定** closes the window. Battle commands and
  save/load hotkeys are blocked while it is visible. A tentative move and its
  action menu remain intact when the window closes.
- Opening is blocked during animation/dialogue or other modal pickers. Enemy
  execution and pending JEV-result application pause while details are visible.

## Content and original evidence

The common header displays the original portrait, name, army side, class, level,
experience (including MAX), live HP/MP with maxima, and current supported statuses.

**能力:** attack, spirit, defense, burst and morale values from the same verified
`battle_stat` helpers used in combat; movement points, terrain and terrain effect.
The order follows original dialog resource178.

**装备:** actual weapon, armor and accessory IDs from `UnitRecord`; original
Item.e5 pixels, Chinese item name, equipment level/experience when applicable,
and numeric statistic bonuses from `equipment_bonus`. Empty equipment says 无;
255 sentinel values are never displayed as levels/experience. Horses appear in
辅助 when equipped, alongside other accessories. This is viewing only; changing
loadouts and a complete special-effect description catalog are outside this UI.

**策略:** all original class/level-learned tactics, their MP cost and learning
level. The five implemented offensive tactics are distinguished from 暂未开放
entries. Learned spells remain listed at zero MP, with 策略值不足. This does not
implement additional spells or provide a cast action in the inspector. It lists
class/level knowledge, not equipment-granted copying of adjacent units' spells.

Evidence:

- Original `Ekd5.exe` SHA256:
  `199b1b67cf966ead46c49b430b9aab84bdc7d4f0e8dfa8e0ebeec5da280b01e0`.
- Existing feature map: tab initialization VA `0x478f7b`, original GBK labels at
  `0x48ec4c/54/5c`. Read-only RT_DIALOG extraction is recorded in
  `docs/evidence/unit-details/original-dialogs.json` (resources166/178/284).
- `tests/fixtures/original_tactics.json`: all68 tactic records were read from
  the original running x86 loader and are compared byte-for-byte by the new test.
- All104 Item.e5 images and tactic data are copied only after checking existing
  original-import manifest hashes. `tools/prototype_unit_assets.py` adds a separate
  presentation font/manifest; it does not modify save-bound asset manifests.
- JEV advisory call069 reviewed learned-versus-castable presentation and
  nonmutating modal tests. Request and response are in `docs/evidence/jev/`.
  No runtime JEV requests are required or added by opening the window.

This is an SDL adaptation to the full-map native viewport, not a pixel-identical
Windows dialog. Original biography and class-description tabs, special-effect
prose, and equipment changes are not ported. Displayed values reflect the current
prototype battle, not the separate Electron save. No combat rules, roster,
experience progression or snapshot schema change in this release.

## Verification and reproducible commands

New data tests inspect130 units, eight equipped accessories and56 learned spell
entries across the four default rosters, compare68 original tactic records,
verify zero-MP visibility and exact nonmutating snapshot restoration. macOS,
Linux and Node/Wasm reports agree on the detail-data hash:
`ca6c42959fe52603688b03cbef998fe5b6db2de5dbf2b8c4a6e880bafaac30d2`.

The SDL `--unit-details-smoke` mode renders390 tabs and checks scaled/letterboxed
pointer coordinates, Ctrl-click, right-click cancellation priority, V/button,
tabs, officer switching, modal input isolation, animation gating, pending move
preservation, enemy pause and save roundtrip. macOS Metal and Linux SDL checks
passed. Existing action-menu and item UI checks are retained as regressions.
Evidence and captures: `docs/evidence/unit-details/`.

```sh
.venv/bin/python tools/prototype_unit_assets.py
python3 scripts/stage-prototype-assets.py
cmake --build build-mac --target caocao_prototype_app unit_details_tests -j8
build-mac/unit_details_tests assets/prototype
CAOCAO_JEV_MODE=off CAOCAO_PROTOTYPE_SAVE_DIR=/tmp/caocao-unit-details-test \
  build-mac/caocao_prototype_app assets/prototype \
  --unit-details-smoke /tmp/caocao-unit-details-captures
python3 scripts/package-prototype-mac.py
```

Build Linux using the existing `caocao-engine-build:0.1` container and run the
same targets with `xvfb-run -a`. Build WebAssembly after sourcing
`.cache/emsdk/emsdk_env.sh`, then run `node build-web/unit_details_tests.js
assets/prototype`. Wasm compilation/data parity does not claim browser UI
acceptance; Chrome/Safari UI was not rerun for this native-focused release.

Existing saves and originals are preserved. Save and close an older running
native instance, then reopen `Start Game.command` to load0.8.0.
