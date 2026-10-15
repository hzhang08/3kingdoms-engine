> Follow-up: implemented in prototype 0.6.0. [Current terrain/pickup report](PROTOTYPE-TERRAIN.md) supersedes the native-gap section and resolves the item placements below. This document retains the prior investigation boundaries.

# Original terrain benefits: 兵营 and 鹿砦

Checked 2026-10-06 (America/Los_Angeles). Investigation only; no gameplay code or
save files changed. Applies to the supplied edition of `Ekd5.exe`, SHA-256
`199b1b67cf966ead46c49b430b9aab84bdc7d4f0e8dfa8e0ebeec5da280b01e0`.

## Confirmed original rules

| Terrain | Original ID | HP recovery per turn-start maintenance | Terrain MP recovery |
| --- | --- | --- | --- |
| 兵营 | 22 | 20% of maximum HP | None |
| 鹿砦 | 20 | 20% of maximum HP | None |
| 城池 | 18 | 25% of maximum HP | None |
| 关隘 | 19 | 20% of maximum HP | None |
| 宝物库 | 24 | None | None |

Recovery uses integer arithmetic and is capped at missing HP. Example: an
unequipped unit with maximum HP 100 goes from 40 to 60 HP on 兵营 or 鹿砦;
99 becomes 100. MP 20 remains 20. Terrain healing is turn-start maintenance,
not an immediate healing event each time a unit enters or crosses a tile.
The original controller shares player/allied maintenance; enemy maintenance
also invokes the regeneration routine. Equipment can supply separate HP/MP
regeneration and must not be confused with a terrain benefit.

The original terrain tutorial explicitly says:

> 有不能移动的地形、还有鹿砦、村庄等等……可以让部队恢复体力的地形。
>
> 其中也有兵营和宝物库等，能够得到道具的地形。

Thus item-bearing locations exist. The terrain label alone does not establish
which item a particular tile contains. This investigation did not recover each
battle's item placements, pickup conditions, repeat-prevention state or exact
loot contents; it does not promise a rare item at every camp or fort.

## Evidence and verification boundaries

- Reopened the installed Electron/Wine game and loaded supplied slot 2 into
  颍川之战, turn 4. No save was written. Coordinate automation for battlefield
  clicks returned `windowNotFoundAtPosition`; no fresh before/after healing or
  item-pickup UI observation is claimed.
- Reran `tools/maintenance_probe.py`: all 1,913 original x86 cases matched the
  retained fixture exactly. This runs the actual original calculations and HP/MP
  setters; display/camera calls are declared substitutions.
- `build-mac/recovery_tests assets/generated` passed 1,913 maintenance and
  2,417 status/grade recovery cases.
- Original regeneration entry: VA `0x43cddd`; terrain predicate/helper region
  `0x435182..0x4351da`. Existing controller evidence establishes phase timing.
- Tutorial text: sequential original-code fixture
  `tests/fixtures/original_tutorial_terrain.json`, captured from the original
  text-window boundary. Terrain names: `assets/generated/ui.json`.
- Fresh results: [summary](evidence/terrain-benefits/summary.json),
  [original cases](evidence/terrain-benefits/original-maintenance.json),
  [test log](evidence/terrain-benefits/probe.log).

## Native prototype gap

Prototype 0.5.0 does not yet apply terrain healing or location-item pickup.
The reusable `battle_regeneration` helper already implements the recovered
numeric rules, but the simplified prototype's turn loop does not call it.
The separate fidelity engine guards nonzero regeneration presentation rather
than treating it as implemented. Neither should be described as complete.

No new JEV calls were needed for this investigation; the direct original-code
evidence resolves the healing question. The earlier JEV call log is unchanged.
