# Basic offensive tactics — prototype 0.5.0

2026-10-06, America/Los_Angeles. The native prototype now executes five original
single-target offensive tactics: **灼热 (0), 浊流 (5), 旋风 (10), 落石 (15),
毒烟 (33)**. These are learned by class and level; a unit is not given all five.
They consume MP, obey range/target terrain restrictions, and use recovered hit,
damage and RNG calculations. Poison has independent damage/status rolls.

The supplied roster calls the 汜水关 caster **李肃, 道士**. **李儒, 策士** is in
虎牢关 and 董卓追击. Both can now use their supported learned spells. They may
still attack physically when it is useful or when MP/targets prevent a spell.

## Controls and presentation

After walking, choose **策略** in the action menu, then a spell and a marked
opposing unit. **T** opens the spell picker for a selected eligible unit without
requiring a move. Number keys select the listed spell. Esc/right-click returns
to the action menu after a move; before moving, it dismisses the spell picker.
Invalid targets consume neither MP nor RNG. The previous action-menu timing fix
is retained: selecting/reselecting a unit does not open that menu before walking.

Both sides use the same caster pose, spell caption, MP cost, target reaction,
damage number, original MEFF images/frame order and WAV files. A queued walk
finishes before casting starts; commands/saves wait until playback completes.
A green **毒** marker identifies poison. At the afflicted side's next phase,
recovered status-recovery logic runs; ongoing poison loses 10% maximum HP,
rounded down with a minimum of one, and cannot kill the unit.

**Presentation limits:** original frame pixels, descriptors and sounds are
reused. The full-map caption/coordinates are adapted. SDL alpha compositing is
used instead of the original palette-index compositor, and prototype playback
uses its existing 40 ms tick; exact original colors/timing are not claimed.
This is not every strategy: area spells, buffs/debuffs other than poison,
healing, weather changes, tactic XP, items and the full campaign remain outside
this version. The current weather model remains simplified.

## Rules and evidence

- `tools/prototype_tactics_probe.py`: 750 unhooked original hit/damage/estimate/RNG
  cases for the five spells, ten target classes, five seeds and three statuses.
- Original source executable SHA-256:
  `199b1b67cf966ead46c49b430b9aab84bdc7d4f0e8dfa8e0ebeec5da280b01e0`.
- Cerberus dossiers: `0x43bfaa` (damage), `0x43b73f` (hit), `0x4214bf` (effect
  dispatch). The effect-index table is at VA `0x4866c0`: 0/5/10/15/33 map to
  MEFF resources 0/2/4/6/23. Original sources are read-only.
- Existing original availability, range, terrain, poison plan/application and
  recovery fixtures support the reused helpers. Availability uses original
  class/level/raw MP requirements; verified equipment discounts apply on charge.
- `tools/import_prototype_tactics.py` creates a separate hash-verified asset
  manifest with original resource/image identities and source hashes. It stages
  74 assets plus the manifest without changing the original asset catalog.

Local AI scores legal physical and tactic choices; its tactical preference is a
prototype heuristic, not the original AI's full scoring or random-consumption
order. The original-scoring helpers still reject unverified elemental AI scoring.

## JEV protocol and logging

`caocao-tactics-v2` exposes only legal spell/move/physical choices. Spell choices
include ID/name, MP cost, destination, target, hit probability, estimated damage
on hit and poison probability. Unit context includes current/max MP, spirit and
available supported spells. Explicit capabilities identify unsupported systems.
The request identity includes the protocol, tactic asset identity, exact state
and legal choices, keeping old physical-only cached decisions separate.

JEV returns only a listed choice ID. C++ validates the current request key and
choice, then executes the same rules as local AI. No model-authored damage or
arbitrary action executes. Planning estimates do not consume gameplay RNG.
Fallback, cancellation, cache and asynchronous transport retain their existing
behavior. Runtime logs remain in the native save folder's `jev/` directory.

This change made **three JEV calls**: advisory call 066 about the legal-action and
save contract, and two live runtime calls in isolated fixtures. The live model
chose 毒烟 for 李肃 and 灼热 for 李儒. Both executed and replayed identically from
a saved pending decision. Requests/responses and dispositions are linked from
`docs/JEV-DECISIONS.md`; no credentials are bundled.

## Saves and verification

Schema 6 (`prototype-0.6`, `basic-tactics-v1`) stores current MP, poison and pending
JEV choices with the tactic manifest identity. Corrupt MP/status or incompatible
assets are rejected transactionally. A native load of a balanced schema-5 save
first validates the old snapshot, then enables tactics without changing units,
HP, MP, positions, turn, dialogue flags or RNG. A pending old physical decision
is cleared and replanned. The source slot is not overwritten until the user saves.
The shared engine can still restore schema 5 exactly for comparison. Earlier
unbalanced saves retain their historical rules; restart that battle for tactics.

Evidence is in `docs/evidence/prototype-tactics/`:

- macOS/Linux/Wasm original comparisons and deterministic action-trace parity;
- four battle caster fixtures, MP charge, poison/save continuation, invalid and
  unavailable actions, corrupt-MP rejection and schema-5 preservation/upgrade;
- actual native caster/effect/player-picker captures and packaged launchers;
- action-menu timing regression and existing physical/save/animation checks;
- two accepted live JEV decisions and deterministic pending-choice replay.

Caster rendering fixtures move the caster to a legal contact tile and start at
its enemy cursor. They prove execution/presentation for that actor, not the AI's
ability to navigate the entire battle. An initial passive-player run did not
bring 李儒 into range before the battle ended; that was not a casting failure.
The browser frontend is compiled and its simulation trace compared; browser UI
interaction was not rerun for this native-focused update.

```sh
.venv/bin/python tools/import_prototype_tactics.py /path/to/original/game
python3 scripts/stage-prototype-assets.py
cmake --build build-mac --target caocao_prototype_app prototype_tactics_tests -j8
build-mac/prototype_tactics_tests assets/prototype /tmp/tactics-results.json
CAOCAO_JEV_MODE=off CAOCAO_PROTOTYPE_SAVE_DIR=/tmp/tactics-smoke \
  build-mac/caocao_prototype_app assets/prototype --tactics-smoke /tmp/tactics-frames
python3 scripts/package-prototype-mac.py
```

`jev_tactics_tests ASSETS OUTPUT` is a separate, explicit **two-live-request**
verification tool, not part of ordinary offline tests.
