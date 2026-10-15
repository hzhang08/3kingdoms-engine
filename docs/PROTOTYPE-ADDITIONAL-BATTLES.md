> Superseded balance, 2026-10-06: the level1 enemy override and omitted
> equipment described below are corrected for new games in0.4.0. This document
> preserves the0.3.0 release evidence; see [current balance/JEV report](PROTOTYPE-BALANCE-JEV.md).

# Four standalone native battles — prototype 0.3.0

Updated 2026-10-06. This extends the agreed simplified prototype, using the
[legacy-game-reconstruction skill](/Users/henryzhang/.codex/skills/legacy-game-reconstruction/SKILL.md).
It does not complete the older original-fidelity acceptance checklist.

## Playing

Use the existing `dist/prototype-macos-executable/Start Game.command` or
`dist/prototype-macos/颍川之战.app`. Open **选择战场 / B**, then click a battle
(or press1–4). Escape returns to the current battle. Selection starts a new
independent battle; **F9** loads that battle's saved progress. After victory
and dialogue, **下一战** starts the next independent battle.

| Battle | Map | Friendly / enemy units | Prototype objective |
| --- | --- | --- | --- |
| 颍川之战 |20×20|8 /14|Defeat every enemy; existing captured opening unchanged.|
| 汜水关之战 |20×20|17 /18|Defeat every enemy.|
| 虎牢关之战 |20×20|16 /23|Defeat the enemies outside the gate, columns7–19.|
| 董卓追击战 |28×20|7 /27|Defeat 李儒.|

Cao Cao's defeat or exceeding20 turns loses. New battles start at turn1;
颍川 keeps its captured turn2. Space/Return or clicking advances original Chinese
dialogue. Movement, cancellation, attacks, enemy turns, original walking/swing/hit
frames and attack sounds use the shared C++ prototype. The wider pursuit map uses
one proportional transform for map, units, animation and mouse input.

Each battle has one independent manual save. The opening keeps its existing
`~/.local/share/caocao-prototype/slot-1.json`; the additions use
`slot-battle-1.json` through `slot-battle-3.json` beside it. Choosing/restarting a
battle never writes a save. F5 deliberately overwrites only the current battle's
slot. The browser uses separate corresponding IndexedDB keys. It has no accounts
or cloud persistence. Older opening schema1–3 snapshots are retained; new scenes
use schema4 including scenario and expansion-manifest identities, RNG, unit and
pending-dialogue state. Failed restores are transactional.

## Original evidence and deliberate adaptations

`tools/prototype_scenarios.py --source GAME_DIR --verify-original` reads the
original edition without modifying it. It checks the executable SHA-256, parses
S_01–03 with original byte offsets, decodes Hexzmap records1–3, untile-decodes
Hm01–03 with the existing original palette, and emits a checksummed expansion.
The original generated manifest remains unchanged. Staging includes original
portrait and attack/walking/special sprite archives and verifies their hashes.
The Chinese atlas uses the existing Source Han Serif font, including the original
atlas's glyph coverage and all added script characters.

For deployment, bounded x86 probes execute original handlers68/69/70/71/75
(0x412230,0x4122f1,0x4125c5,0x4129a4,0x415e31). They compare every selected unit's
ID, slot, coordinates, direction and original hidden flag. Fixture inputs declare
the seven-character party `[0,1,5,6,10,11,12]`; this is a prototype preset,
**not** a recovered campaign save. The selected-ID array at0x48b350 is explicitly
initialized with that party and sentinel65535 in unused slots. Data/archive and
collection hooks are recorded in the fixtures; this is not a full Windows run.

The scripted 虎牢关 ending explicitly says the army cannot capture the gate.
S_02 offset0x368e has condition65 arguments `[2,0,2,1,7,0,19,19]`; offsets0x36be,
0x36e3 and0x3df1/0x3e16 contain the outside-enemy victory narrative. This prototype
uses the outside region as its objective; it does not open invented passages or
require killing inaccessible defenders. The region interpretation is based on
the parsed script and map; a new original-code condition65 oracle was not added.

The additions are standalone adaptations, not complete script execution:

- All allies are player-controlled; all originally hidden enemies appear at
  the beginning. Original ambush timing, retreat/removal effects, duels, allied
  AI and campaign recruitment are omitted. Dong Zhuo and his pursuit escort
  remain visible instead of executing their scripted departure.
- Fixed friendly levels are10 for the two passes and20 for pursuit; enemies
  are level1. These are explicit prototype balance settings, not recovered
  campaign progression. Physical strikes always hit, clamp damage12–45, and
  omit counterattacks, tactics, items, experience, rewards and growth.
- Original introduction, named retreat, outcome text and the concrete
  Cao Cao/Li Ru adjacency speech are extracted. Other branch choices and
  encounter/duel side effects are excluded. Trigger timing is adapted to the
  prototype's committed-action boundaries. Pending pages/flags are saved.
- Full R_01 campaign parsing is unresolved: strict GBK decoding stops at
  original byte0x04ac. This update neither skips that failure in a campaign
  interpreter nor depends on that scene; only the standalone S scripts load.
- No new music/campaign movie support, Google login, cloud saves or leaderboard.
  Original files, Electron project and original `.e5s` saves remain untouched.

## Verification

Final macOS, Linux and Node/WebAssembly checks passed. All950 new-battle
checkpoints match across the three platforms:400 in 汜水关 (victory turn8),222
in 虎牢关 (turn4),328 in pursuit (turn9). Passive defeats occur at turns19,12,16.
All three turn-limit cases, nine cross-platform fresh-process next-action checks,
native slot isolation and fresh native-process reload passed. Both native
renderers also presented enemy walking, weapon frames and hit flashes on every
new map, including the scaled pursuit map (`combat-mac.log`, `combat-linux.log`).

Build/platform verification is recorded in
`docs/evidence/scenarios/validation-{mac,linux,web}.log`, `parity.log`, and
`packaged-mac.log`. Renderer screenshots are under `render-mac/` and
`render-linux/`. `*-outcome-control.bmp` uses a clearly identified terminal
fixture to test the Next Battle button; it is not a screenshot of the automated
playthrough's final frame.

The shared-engine tests play each new battle to victory using normal commands,
restore and compare the next state after each command/page/enemy action, verify
all emitted animation files and sounds, and cover passive defeat, turn limit,
restart, corrupt identity/page rejection and restoration of the old opening.
The pursuit strategy keeps Cao Cao behind the advance and prioritizes Li Ru
when in range. This is one successful strategy, not a difficulty guarantee.

Rendering tests use SDL events at two window sizes, original intro pages,
map-edge input, walking playback, every roster's sprite resources, per-battle
save isolation, fresh-process reload and the victory navigation controls. The
older opening test's production379-checkpoint hash remains
`25de5680625b5f0d35a0526298adb15bf3adc0cbffdcbe8ceec8a4e4ca2295e4`;
the historical365-checkpoint trace remains
`6916cde52483c54ce12fb7e2a8d9cd3d3e4d714e93f93fcf68882ddb419fdcfe`.

Chrome also rendered the added maps and preserved separate 虎牢关 page2 and
pursuit page1 saves through a page reload. See [targeted browser record](evidence/scenarios/browser-check.md), including an unresolved startup performance advisory.
This update does not claim a clean browser console or browser-process restart.

No full campaign, original difficulty/AI equivalence, complete browser campaign
playthrough, or new Safari acceptance is claimed.

## Reproduce

From the engine project, using its pinned dependencies and existing CMake,
Emscripten and local Docker environment:

```sh
.venv/bin/python tools/prototype_scenarios.py --verify-original
scripts/check-prototype-scenarios.sh all
python3 scripts/package-prototype-mac.py
python3 scripts/package-prototype.py
```

The check script supports `mac`, `linux`, `web`, and `parity` individually.
`parity` compares each checkpoint trace byte-for-byte and resumes Linux-produced
snapshots in Wasm/macOS, and Wasm snapshots in Linux. Tests use isolated native
save directories. The Mac packager replaces the executable atomically; an
already-running older game stays older until closed and relaunched.

## Screenshots

Native battle selector:

![Native battle selector](evidence/scenarios/render-mac/selector.png)

Native pursuit enemy hit flash:

![Enemy attack on the wider pursuit map](evidence/scenarios/render-mac/3-enemy-flash.png)

Chrome restored original dialogue page after saving another battle and reloading:

![Hulao dialogue restored in Chrome](evidence/scenarios/chrome-hulao-restored.png)
