> **Selectable special-duel heroes, 2026-10-13:** Mac2.3 and web0.13.0 add Cao Cao / Xiahou Yuan / Xiahou Dun / Li Dian selection, random Lu Bu or Dong Zhuo, four new28-frame voxel libraries and schema13 saves. All eight native/Wasm traces match; packaged Mac rendering, Chrome save/reload and emulated phone selection pass. Older fixed-duel saves retain progress. This custom mode does not change campaign fidelity. See [release report](SPECIAL-HULAO.txt).

> **Special duel gate access, 2026-10-13:** Mac 2.2 and web 0.12.0 open the two east entrance tiles in this custom duel, preserve wall restrictions and migrate old duel saves to schema12. Entry/exit, enemy pursuit, native/Wasm parity, packaged rendering and Chrome courtyard save/reload pass. This is a custom terrain adaptation; original campaign terrain is unchanged. See [release report](SPECIAL-HULAO.txt).

> **Special duel controls, 2026-10-13:** Mac 2.1 and web 0.11.1 enlarge/highlight movement cells and allow an explicit current-tile click/tap or Space to act without moving. Native/packaged stationary attacks, Chrome mouse/touch emulation, unchanged save identity and native/Wasm traces pass. See [release report](SPECIAL-HULAO.txt).

> **Custom voxel duel, 2026-10-13:** The separate special Hulao edition now deploys only Cao Cao and Dong Zhuo, with both 28-frame libraries integrated. Dedicated schema11 saves, native/Wasm deterministic traces, packaged Mac rendering and Chrome save/reload pass. This custom mode does not change the original-game fidelity claim. See [release report](SPECIAL-HULAO.txt).

> **JEV startup credential, 2026-10-08:** Version 0.9.3 loads the external JEV key once at startup and reuses it from memory. Missing/invalid credentials use local AI; key changes require an app restart. Documents permission may still be requested at startup. Gameplay and save formats are unchanged. See [JEV behavior](PROTOTYPE-BALANCE-JEV.md).

> **Diagnostics, 2026-10-08:** Prototype 0.9.2 adds bounded per-session logs and stable recovery snapshots without changing schema 9 or battle rules. Startup/error capture and recovery guards are tested separately from gameplay equivalence. See [diagnostics report](PROTOTYPE-DIAGNOSTICS.md).

> **Reward crash fix, 2026-10-08:** Prototype 0.9.1 corrects the treasure-notification image path and persists fatal exception details. The reported 太平清领道 exit was reproduced before repair; revised native smoke tests now render both scripted treasure notices plus additional equipment/consumable icons. Mac/Linux checks pass and Wasm builds. Rules and save schema are unchanged. See [evidence](REA-GAMEPLAY-FIXES.md).

> **Gameplay repairs, 2026-10-08:** Prototype 0.9.0 integrates full physical-attack sequences, XP/growth, fifteen tactics, allied turns, scenario ambushes/duels/outcomes, bounded campaign carryover and ten manual slots with JSON backups. Revised rule traces match on macOS, Linux and WebAssembly; native smoke checks pass. Old saves retain their historical rules. Exact AI, full story, reward cash/equipment levels and other original-game differences remain. See [changes, evidence and remaining gaps](REA-GAMEPLAY-FIXES.md).

> **Unit details, 2026-10-07:** Prototype 0.8.0 adds 能力 / 装备 / 策略, original item icons, equipment levels/experience, and learned-spell visibility. Control-click a unit or select it and use 武将 V. Inspection preserves pending moves and pauses enemy execution; save schemas and gameplay rules are unchanged. See [controls, evidence and limits](PROTOTYPE-UNIT-DETAILS.md).

> **Consumables, 2026-10-07:** Prototype 0.7.0 adds visible inventory/use buttons and original-rule beans (+30 HP), rice (+80 HP), and antidote. Schema-7 collected inventory is preserved; schema 8 tracks consumption and prevents refilling claimed sites. Native UI/persistence checks and original-code fixtures pass. Exact item animation timing and additional item types remain deferred. See [implementation and evidence](PROTOTYPE-ITEMS.md).

> **Terrain benefits, 2026-10-06:** Prototype 0.6.0 adds original terrain HP rates, phase-start recovery, eight one-time scripted pickups, inventory UI and schema-7 persistence. JEV receives recovery estimates and location facts. Item use and exact original pickup/healing animation timing remain deferred. See [implementation and evidence](PROTOTYPE-TERRAIN.md).

> **Basic tactics, 2026-10-06:** Prototype 0.5.0 implements five learned offensive
> spells for both sides, MP/poison persistence, original effect frames/sounds and
> updated JEV legal actions. Balanced saves upgrade without resetting progress.
> Other tactics and exact original visual timing remain incomplete.
> See [rules, JEV, saves, evidence and limits](PROTOTYPE-TACTICS.md).

> **Action-menu timing fix, 2026-10-06:** Prototype 0.4.3 fixes the reproduced
> consecutive-unit stale-walk flag. The menu opens only after that selected unit
> finishes moving (or after restoring its tentative move). Repeated clicks/Space
> before movement no longer open it. Existing saves and battle rules are unchanged.
> See [reproduction and regression coverage](PROTOTYPE-ACTION-MENU.md#043-timing-correction).

> **Unit action menu, 2026-10-06:** Prototype 0.4.2 adds the original five command
> labels and paired Cmdicon artwork, with working attack targeting, wait and
> movement cancellation. Tactics/items remain unavailable in the simplified
> mode; the reference's decorative background pattern is not reproduced.
> See [behavior, visual limits and tests](PROTOTYPE-ACTION-MENU.md).

> **Unit hover panel, 2026-10-06:** Prototype 0.4.1 reuses the recovered original
> unit panel for class/level, HP/MP bars, experience/side and terrain effect.
> Its placement and backing are adapted to the full-map view; gameplay rules
> and save identities are unchanged. See [report](PROTOTYPE-HOVER-PANEL.md).

> **Balance/JEV update, 2026-10-06:** Prototype0.4.0 restores original-script
> equipment and scenario-scaled NPC levels in all four battles, removes the
> artificial damage clamp for new games and corrects the defense display.
> Native JEV selects validated enemy actions; old saves retain their rules.
> Original-code records and macOS/Linux/Wasm trace parity are verified.
> Difficulty and full fidelity are not claimed. See [report](PROTOTYPE-BALANCE-JEV.md).

> **Additional battles, 2026-10-06:** Prototype0.3.0 adds standalone 汜水关之战,
> 虎牢关之战 and 董卓追击战, a battle selector, wider-map support and independent
> saves. Original maps/deployment/text are reused with declared simplified rules,
> visible-at-start ambush units, and no campaign progression or duel scripts.
> See [scope, evidence and verification](PROTOTYPE-ADDITIONAL-BATTLES.md).

> **Dialogue update, 2026-10-05:** Prototype 0.2.5 adds original Cao Cao/Zhang Bao
> and Cao Cao/Zhang Liang encounters, named retreat speeches, victory branches
> and defeat narration. Save/resume retains exact pages and single-use flags.
> Victory timing follows simplified rules. See [evidence and limits](PROTOTYPE-DIALOGUE.md).

> **Walking update, 2026-10-05:** Prototype 0.2.4 adds original walking frames
> along legal routes for both sides, with movement queued before enemy attacks.
> Original walking fixtures and499 integration paths pass; simulation/save hashes
> remain unchanged. See [walking behavior, verification and limits](PROTOTYPE-MOVEMENT-ANIMATION.md).

> **Enemy follow-up, 2026-10-05:** Prototype 0.2.3 fixes a toolbar-hover bug that
> hid enemy action/confusion messages, labels and highlights attack participants,
> and records local playback diagnostics. A new player-then-enemy regression
> expands coverage. The user's continued missing-animation report is not yet
> explained; passing automated cases do not establish resolution. See
> [investigation and evidence](PROTOTYPE-ATTACK-ANIMATION.md).

> **Frame scheduling fix, 2026-10-05:** Prototype 0.2.2 prevents slow rendering
> from skipping enemy swing and hit-flash frames. A 120ms render-delay regression
> fails before the fix and passes afterward. This corrects the earlier fast-test
> conclusion that no missing enemy animation could be reproduced. See the
> [standalone launcher investigation](PROTOTYPE-ATTACK-ANIMATION.md).

> **Attack update, 2026-10-05:** Prototype 0.2.1 plays original physical-strike
> frames, hit flash/recoil, damage numbers and swing/hit sounds. Native playback
> and deterministic macOS/Linux/Wasm traces pass. Timing and numeric layout remain
> provisional; simplified combat rules are unchanged. See
> [animation evidence and limits](PROTOTYPE-ATTACK-ANIMATION.md).

> **Layout update, 2026-10-05:** Prototype 0.2.0 matches the supplied Electron
> screenshot's 22 unit placements/facings and 27 camp fires, with original status
> artwork. New saves are versioned; old prototype saves retain their rules.
> See [layout verification](PROTOTYPE-LAYOUT-MATCH.md). Combat remains simplified.

> **Active scope update, 2026-10-04:** The user replaced the full-fidelity goal
> with a separate simplified playable battle. See [PROTOTYPE.md](PROTOTYPE.md)
> for its rules, builds, tests and limitations. The original-fidelity investigation
> and incomplete checklist below are historical work, preserved unchanged.

> **Visual update, 2026-10-05:** The simplified prototype now has a full-width
> original-art battlefield, classic toolbar, portraits, hover cursor and native
> Retina rendering, plus a standalone Apple Silicon `.app`. Gameplay/save
> identity is unchanged. See [the visual update](PROTOTYPE-VISUAL-UPDATE.md) for
> evidence and remaining differences from the reference screenshot. This does
> not advance the historical original-fidelity acceptance checklist.

# Compatibility record — updated 2026-10-04

## Latest source progress — player menu and input-loop contracts

The shared C++ player-action helpers now match **1106 original menu/wait/cancel
cases** on **macOS, Linux and WebAssembly**, including availability, distance/
threat/mask grids, target order, movement rollback, direction/mirror flags,
occupancy and ordered tail requests. Existing502 player movement cases and all
older movement/search/route/animation cases also pass in the focused suite.
The helpers do not yet enable player commands in the interactive engine.

A new **140-sequence original input-loop probe** runs the actual disabled-ASave
continuation and actual movement/menu callbacks. It captures retained hover,
right-click stats/end confirmation, command5 exit, timer branches, action-event
gates, death/event scans and end-turn confirmation. The new native hover and
remaining-unit helpers match **174 hover transitions and102 candidate scans**
on all three platforms. The rest of that outer loop is original-code evidence
awaiting an Engine implementation, not a native full-loop comparison.

Findings important to integration:

- Cancellation restores position/direction and clears moved4. East/west set the
  mirror explicitly; north/south preserve the current mirror. Wait sets flags6.
- Menu tactic availability checks learned levels, with equipment51 neighbor
  borrowing; this layer does not filter MP, silence, side or neighbor HP.
- A cancelled action still enters45b5fe, which runs an event batch in these
  captured cases. Completion adds a death scan and another event batch.
- Post-action remaining-unit counting excludes completion2/confusion8, but
  not script-control128. The entry gate has a different filter.

The probes explicitly activate CaoCao at(10,6) on a post-introduction board.
World tiles, mouse polls, timer/scroll state, focus, covered-panel state and
modal replies are supplied host inputs. They do **not** prove physical mouse
handling, original Windows geometry, a continuous turn-two path or full battle.
All required script-body continuations beyond the fixture scope stop explicitly.

Dictionary **1.0.2** now has114 fields across23 structures, including consumable
stock and the scoped panel-retention gate. Historical1.0.0/1.0.1 dictionaries are
archived; all13 dictionary tests pass. Refactoring shared probe setup preserves
all502 movement and1106 action fixtures exactly. Cerberus now has15 dossiers;
player attack43dada/tactic43dc21 remain static-only investigation inputs.
JEV057–059 are recorded: **59 total requests,58 successes,1 historical HTTP400**.
Abstentions and independent executable findings are distinguished in the log.

**Interactive checkpoint remains0.29**, schema7 and the same asset manifest;
`dist/` remains0.22. No new browser UI, audio, native rendering, snapshot or full
state-trace acceptance is claimed. No original game/save data was modified.

Next integration work:

1. Connect the verified movement/menu/wait/cancel helpers to the Engine's player
   stage, preserving the observed action-event ordering even after cancellation.
   Add stable player state, schema validation, deterministic restoration and
   native tile/menu input and presentation. Compare a continuous fresh-profile
   turn-two trace, not only explicitly supplied post-introduction inputs.
2. Execute player attack/tactic setup and targeting against the new Cerberus
   dossiers. Recover party HP/XP result handling before claiming complete combat;
   the existing NPC arithmetic/presentation tests are insufficient for that path.
3. Finish battle outcomes/rewards and the remaining original acceptance gates.

Evidence: [validation record](evidence/player-action-validation.json),
[action probe](../tools/player_action_probe.py),
[input probe](../tools/player_input_probe.py),
[C++ helpers](../src/player_action.cpp),
[macOS](evidence/tests-player-input-mac-final.log),
[Linux](evidence/tests-player-input-linux.log),
[Wasm](evidence/tests-player-input-web.log),
[JEV log](JEV-DECISIONS.md), [Cerberus](CERBERUS-RE.md).

## Prior source progress — player movement controller, integration pending

Cerberus43d538/43dde3 exposed the player movement and selected-unit action
controllers. A new shared C++ begin/pick interface matches **502 original
movement-controller cases** on **macOS, Linux and WebAssembly**: eligibility,
selection/cancellation, invalid-destination retry, original Chinese messages,
distance grids, path traversal, camera, occupancy, direction and moved flag.
The same suite also passes all existing movement/search/route/walking/AI/script
cases. The13 dictionary tests pass.

This helper **is not yet connected to the interactive game**. Tile-picker
responses are supplied to the original probe; the all-destination CaoCao cases
explicitly activate him at(10,6) on a post-introduction board with camera(4,2).
Those inputs do not establish a continuous turn-two playthrough. The outer
input loop, command-menu handoff/cancellation, snapshot integration, combat,
outcomes/rewards and full acceptance remain unfinished. The game still stops
at the0.29 player-input guard below; engine/schema/asset versions are unchanged.
No new browser UI, audio or package test was needed or performed.

Two further JEV calls055–056 are logged: **56 total requests,55 HTTP200 and
1 historical HTTP400**. JEV abstained on retained hover selection; that outer-loop
behavior remains unverified. Original execution independently verifies the
accepted movement advice. The binary map and tools catalog now cover the new
probe and the two additional Cerberus dossiers.

The missing0.29 checkpoint record is completed. Older dictionary observations
retain their original identity and bytes; an archived1.0.0 dictionary matches
and decodes all seven historical captures. Current dictionary remains1.0.1.

Evidence: [player movement record](evidence/player-movement-validation.json),
[original probe](../tools/player_movement_probe.py),
[C++ controller](../src/player_movement.cpp),
[macOS](evidence/tests-player-movement-mac.log),
[Linux](evidence/tests-player-movement-linux.log),
[Wasm](evidence/tests-player-movement-web.log),
[JEV decisions](JEV-DECISIONS.md), [Cerberus](CERBERUS-RE.md).

## Current checkpoint — 0.29, acceptance still incomplete

The native engine now completes fresh-profile turn-two poison maintenance after
the tutorial and battle-start banner. Original recovery consumes RNG
`340946258 → 601077795`; the poisoned allied infantryman then displays
**44 → 33 HP** through five interpolation steps and a terminal pause, and HP
changes only after the panel returns. Poison and the following zero-gain
regeneration pass consume no further RNG. The next guard is **player control**
(`0x44e1d3`, continuation `S_00.eex:0x2fa5`). The interactive gate's entry UI,
camera setup and input loop are still to be ported; reaching this guard does not
mean player commands are implemented.

Cerberus exported `0x450984` and its existing `0x44df61` dossier guided the work.
The damage panel now accepts absent-opponent sentinel255 instead of requiring
a fabricated unit. **272 original NPC panel sequences** pass, including240 new
no-opponent cases; all32 earlier cases are unchanged. Refactoring the probe's
shared panel setup also preserves all265 normal-panel cases. The sequential
original run executes recovery, poison/panel/HP setter, then regeneration in
caller order. C++ tests compare every panel operation at all six pauses, old HP
during presentation, final units/records/terrain/camera/RNG, event completion and
identical restoration from earlier stable saves. Transient saves are rejected.

**All18 Linux suites and all18 Wasm/Node suites pass**, plus four focused macOS
suites and13 dictionary tests. Both SDL frontends and the Wasm frontend compile.
Three-platform traces match at **2341/2009 checkpoints**. A Linux-generated
terrain-lesson snapshot restores to identical state on all three platforms.
The [native poison render](evidence/linux-phase-poison-0.29.png) was inspected;
Chinese labels and33/110HP are visible. No new browser UI, audible-audio or
packaged relocation run; existing rendering algorithms are reused by this path.

A sequential comparison exposed a56-pixel coordinate mismatch: the headless
prefix inherited client origin0,0 and never created a Windows window. The
fixture now explicitly supplies0,56, as the isolated panel fixtures do, records
both origins and preserves world camera coordinates. This is a declared host
input, **not proof of original Windows layout**. Dictionary1.0.1 records the
client/world-coordinate distinction and poison/RNG order. Host raster/cursor/
refresh boundaries and unresolved full composition/font metrics remain explicit.

Version **0.29.0-investigation**, snapshot schema **7**, unchanged importer
**0.19.0** and asset manifest
`18f29c1ee4e4fad94708408700fc17ebffb87f3bc3f3f277dbff11e8ef51a539`.
Original game files/saves remain unchanged. `dist/` remains0.22. Party HP/XP
panels, recovery effects, nonzero regeneration, player actions, full battle
outcomes/rewards, collection-dependent tutorial branches and complete browser
acceptance remain unfinished. Unsupported paths still stop explicitly.

Evidence: [checkpoint](evidence/checkpoint-0.29.json),
[sequential original probe](../tools/phase_maintenance_probe.py),
[original fixture](../tests/fixtures/original_phase_maintenance.json),
[Linux](evidence/tests-linux-0.29.log), [Wasm](evidence/tests-web-0.29.log),
[macOS](evidence/tests-mac-focused-0.29.log),
[parity](evidence/parity-0.29.json),
[snapshot interoperability](evidence/tutorial-snapshot-0.29.json),
[Cerberus workflow](CERBERUS-RE.md).
Two new [JEV reviews](JEV-DECISIONS.md) cover ordering/sentinel/RNG and the host
origin mismatch: **54 cumulative requests,53 successes,1 historical HTTP400**.
Accepted advice has independent original-code verification; no measured speedup.

## Historical checkpoint — 0.28

Cerberus/Ghidra now runs in a pinned, offline static-analysis container. Nine
function dossiers supported the next port: after the fresh-profile tutorial,
Xu Zijiang disappears using the original cropped sprite frames and terrain
mask; Cao Cao turns; the **“开始作战”** checker/banner sequence runs; script
variable13 records event completion. Both opening routes now stop explicitly at
**`S_00.eex:0x2fa5`, phase poison presentation `0x450984`, HP loss11**, before
actual player control. No unsupported maintenance or player command is skipped.

Version **0.28.0-investigation**, snapshot schema **7**, importer **0.19.0**.
The new banner label is decoded from original GBK bytes; source files are not
rewritten. Manifest (2394 assets,367 source files):
`18f29c1ee4e4fad94708408700fc17ebffb87f3bc3f3f277dbff11e8ef51a539`.
Older prototype snapshots are rejected by version/manifest without overwriting
slots. Original saves and executable remain unchanged. `dist/` remains the
older0.22 package; no new packaged release or browser UI acceptance is claimed.

Verified this checkpoint:

- **208 original disappearance cases**, including lifecycle, flags, AI-target
  cleanup and static terrain masks; **2678 frame comparisons**, each with
  source and post-mask pixel hashes. Persistent records and RNG remain unchanged
  in these cases. Animated terrain13 and other removal forms remain guarded.
- **Eight original start-banner layouts** and a sequential original run through
  all four post-tutorial instructions. Native integrated tests compare units,
  persistent records, camera, tick, RNG, terrain, occupancy and completion flags;
  restored farewell snapshots reach the same boundary without duplicate exit.
- **All18 Linux C++ suites and all18 Wasm/Node suites pass.** Focused macOS
  tutorial/exit tests pass; Linux, macOS and Wasm SDL frontends compile. The
  existing tutorial coverage remains18 branches,994 original interactions,
  994 deterministic snapshots and17,066 rejected malformed restores.
- **Three-platform traces match at2328/1996 checkpoints**, including both
  opening choices and tutorial completion/decline. Separate exit-test traces
  exercise both story choices with the full tutorial. A Linux-generated stable
  terrain-lesson snapshot restores with identical complete state on Linux,
  macOS and Wasm/Node. This is not browser IndexedDB/restart testing.
- **24 importer tests pass**, including source/asset hashes and Chinese glyph
  coverage. Native Xvfb guide-exit and start-banner renders were inspected;
  the new Chinese banner is readable. Windows font shapes, full original
  composition, real timing and audible playback remain unverified.

Evidence: [checkpoint](evidence/checkpoint-0.28.json),
[Linux suites](evidence/tests-linux-0.28.log),
[Wasm suites](evidence/tests-web-0.28.log),
[macOS exit tests](evidence/tests-mac-exit-parity-0.28.log),
[parity](evidence/parity-0.28.json),
[snapshot interoperability](evidence/tutorial-snapshot-0.28.json),
[importer](evidence/tests-importer-0.28-final.log),
[guide render](evidence/cerberus/render-guide-exit-0.28.png),
[banner render](evidence/cerberus/render-start-banner-0.28.png).

The [Cerberus record](CERBERUS-RE.md) preserves setup, pins, evidence, repaired
probe/build/report failures and analysis limits. Ghidra's player-gate view ends
at the patched ASave indirect jump; its apparent return is **not** a recovered
input loop. Original gate fixtures establish the continuation, but actual
player actions still require investigation. Two new advisory JEV calls are
documented: **52 cumulative requests,51 successes,1 historical HTTP400**;
one abstained on all questions. No measured time-saving claim.

The full acceptance checklist remains incomplete: player commands, complete
battle/outcome/reward paths, collection-dependent tutorial rewards, full
presentation fidelity and Chrome/Safari playthroughs remain. Older sections
below describe historical checkpoints and their then-current boundaries.

## Historical tooling update — 2026-10-03

The [game-state dictionary](GAME-STATE-DICTIONARY.md) version1.0.0 now covers112
field definitions across21 structures/buffers. Shared probe adapters use its
types and offsets; read-only observations support address lookup and named
memory differences, preserving unknown bytes and flags. All13 focused tests
pass. Regenerated initialization and strike fixtures match the retained results
exactly; isolated original tutorial setup, cancellation, strike planning and RNG
also verify the observation tool. See [regression evidence](evidence/game-state/regression.json).
These captures are diagnostic memory observations, not resumable checkpoints.
The gameplay acceptance status below is unchanged. [JEV log](JEV-DECISIONS.md):
50 cumulative requests,49 successful evaluations and1 initial HTTP400.

## Historical working update — 0.27

The fresh-profile fourth terrain lesson now completes through its summary and
farewell. The integrated path then stops at `S_00.eex:0x2f53`, opcode49;
collection-dependent tutorial rewards and actual player control remain unfinished.
All17 Linux C++ suites pass ([test log](evidence/tests-linux-0.27.log)).
The native tutorial suite compares994 original interactions and994 deterministic
snapshot continuations across18 branches, rejecting17,066 malformed restores
([macOS tutorial log](evidence/tests-mac-tutorial-0.27.log)). Terrain-panel tests
match138 original layouts,30 terrain predicates and256 grayscale indices;
12 original camera cases establish cursor clearing even without displacement.
The new asset import has2394 assets,367 sources and manifest
`c671ba07f1a605131e28ee49480e38157b42a2533e38318124f9b44ba45c8a52`.

Linux/macOS frontends and the Wasm frontend/tests compile. The latest complete
cross-platform trace comparison remains0.26;0.27 Wasm execution/parity and
terrain-panel visual inspection are pending. A macOS offscreen render attempt
failed at window creation (`Parameter 'window' is invalid`), so it is not a
passing render test. Existing dialogue/panel overlap and original UI/timing/audio
comparison remain unresolved. No new browser UI run or packaged release.
[Jev log](JEV-DECISIONS.md):19 requests,18 successful evaluations,1 HTTP400;
no measured development-time saving.

## Historical checkpoint — 0.26

**The opening-battle milestone is incomplete.** Native gameplay now plays the
tutorial entry and its basic, ally/enemy and movement/action lessons. The third
lesson demonstrates the blue movement range, Cao Cao's move and cancellation,
then the original command-menu demonstration and subsequent dialogue. Continuing
stops explicitly at **`S_00.eex:0x2f51`, opcode16, `Ekd5.exe:0x43419f`** before
the terrain lesson. Declining at any implemented choice completes the appropriate
summary/farewell and stops at **`S_00.eex:0x2f53`, opcode49**. Actual player
battle actions, complete outcomes, turn limits and rewards remain unfinished.

At this checkpoint, native Linux/macOS executables and Wasm build:
**0.26.0-investigation**, snapshot schema **6**, importer **0.17.0**. Manifest:
`f11e115ae388e57848239e65f713297cc9ab7a30a918eefd2a7d379fa53400b3`.
Original game files/saves remain unchanged. Older prototype saves remain in their
slots and are rejected by version/manifest, without overwriting them. `dist/`
archives remain at0.22; this is not a packaged milestone release. See the
[0.26 checkpoint](evidence/checkpoint-0.26.json).

## Native-first movement/action tutorial in 0.26.0

The original third lesson (`0x433bc8`) now executes through its next continuation
choice. Cao Cao temporarily moves from(10,6) to(10,0), then cancellation restores
his coordinates/occupancy while retaining the changed facing/flags. The existing
original movement/search code supplies the distance scratch even while the
range overlay is hidden. Schema6 records unit origins, the cancellation return
position, command-menu result and focus. Restoring a mid-lesson save replays
range/position history, rather than deriving earlier ranges from final positions.

The demonstration's original dialog resource0x117 supplies all five labels and
control positions. Actual callback execution verifies that only **策略, 待命,
取消** are enabled; each dismisses the demonstration without performing a battle
action. Six original response/continuation paths run through the original caller.
Additional probes cover16 enablement configurations, six command IDs/cancellation
and180 window-placement cases with supplied host dimensions/client origins.
Native SDL mouse/keyboard input, disabled commands and focus restoration pass.
This is a tutorial interaction, not the still-unimplemented player action menu.

**18 integrated branches** cover both story choices, all implemented tutorial
choices, all three enabled menu responses and continuing/declining afterward.
**826 original interaction comparisons and826 deterministic save continuations**
pass on Linux, macOS and Wasm/Node; **14,042 malformed snapshots** are rejected
transactionally. The importer includes59 original tutorial strings plus the
menu resource. Glyph coverage, one-page dialogue limits and all source/imported
asset hashes pass. Four guide walking frame/pixel comparisons remain covered.

**1,536 original tile-painter cases** verify blue palette60, green90, blue
precedence, range marks, terrain cursor order and covered-tile handling. The
renderer previously shaded over terrain marks; its order is corrected. The
menu placement smoke exposed a wrong full-window size assumption: original
placement uses the624×384 battlefield limits. The corrected
[native menu image](evidence/linux-native-tutorial-menu.png) includes all controls;
[native blue-range image](evidence/linux-native-tutorial-blue.png) shows the
original checker and readable dialogue.

All17 Linux C++ suites passed. After final focus/placement adjustments, the
changed tutorial and animation suites pass on all three targets; all three SDL
frontends compile. Linux SDL event tests exercise enabled/disabled clicks,
Escape/right-click, Tab/Enter, Up/Space, default Enter and focus save/restore.
Linux/macOS/Wasm traces match at **2,163/1,950 checkpoints**. Linux snapshots both
before move cancellation and at the command menu restore unchanged on macOS and
Wasm. See [parity](evidence/parity-0.26.json),
[snapshots](evidence/tutorial-snapshot-0.26.json), and
[native input results](evidence/tests-native-menu-input-0.26.log).

**Visual/integration limits remain:** the SDL menu uses a provisional16px font
conversion from original dialog units, with provisional colors/borders and focus
painting. Original font metrics, button decorations, dragging, timing and full
window composition still need comparison. The earlier dialogue-over-panel issue
remains. New menu/input and layering changes need targeted Chrome/Safari checks;
browser UI work was deferred under the native-first priority. No full-battle,
visual-identity or acceptance-complete claim is made.

[Jev decisions](JEV-DECISIONS.md) now records17 API requests (16 successful,
1 initial HTTP400). Calls015–017 concern blue-overlay registers, menu enablement
and signed popup placement; each accepted recommendation has independent
original-code evidence. Call015's existing record was reconciled into the human
log this checkpoint;016/017 were new calls. No speedup measurement is claimed.

Reproduce with the focused `tutorial_tests`/`animation_tests` binaries,
`tools/tutorial_menu_probe.py`, `tools/tutorial_menu_event_probe.py`,
`tools/overlay_probe.py --extended --output tests/fixtures/original_blue_overlay.json`,
and native `--render-tutorial-blue-smoke`/`--render-tutorial-menu-smoke`.
The menu smoke also executes its SDL input checks without writing user slots.

## Historical native-first tutorial progress in 0.25.0

The original second lesson (`0x4337f4`) is ported. The guide walks east, turns,
shows Liu Bei's panel, then demonstrates the green movement range and red attack
mask. Original movement search gives 39 reachable cells and 4 attack-mask cells.
Dismissal turns off movement display and clears the mask, retaining the computed
distance data. The guide ends at(10,0), tutorial cursor at(9,6), and Liu Bei stays
at(9,5); these distinct coordinates are preserved. Schema5 adds the initial
cursor origin so restoring after guide movement does not shift the cursor.

Eight integrated paths cover both opening story choices and all four implemented
tutorial paths: initial decline, decline after basics, decline after allies, and
continue to the next explicit boundary. **208 original dialogue/choice states**
match units, camera, tick, RNG, terrain/effects and cursor. Range/mask grids and
panel lifetime match where used. **208 save continuations** and **2,496 malformed
restore cases** pass. Four original guide walking requests and indexed-pixel
hashes also match. The importer includes 42 original tutorial strings, with
verified one-page layout limits and complete non-missing Chinese glyph coverage.

All **17 Linux C++ suites pass**; the expanded tutorial suite also passes on
**macOS arm64 and WebAssembly/Node**. SDL frontends compile on all three targets.
The [native range screenshot](evidence/linux-native-tutorial-range.png) shows the
green/red overlays, guide and readable Chinese text. Original-window composition,
font metrics, input and timing remain unverified. The dialogue-over-panel issue
recorded in 0.24 remains unresolved. No browser UI tests were run this checkpoint.

Linux/macOS/Wasm traces match at **2,055/1,950 checkpoints** for the two choices;
the active-range Linux snapshot restores unchanged on macOS and Wasm. See
[parity](evidence/parity-0.25.json) and
[snapshot interoperability](evidence/tutorial-snapshot-0.25.json). These checks
cover the opening and implemented tutorial, not complete battle gameplay.

The next lesson is now captured through movement cancellation and command-menu
entry: 154 original events and 13 speeches. Its cancellation restores coordinates
and occupancy without undoing facing/flags;24 additional original-code cases
check these fields and the early-return argument. The original command dialog
resource 0x117 is decoded read-only: five controls (攻击/策略/道具/待命/取消),
55×77 dialog units and 9-point 宋体. Dynamic enablement, window placement, painting
and modal input are still unverified. Neither this lesson nor player cancellation
is yet ported. Investigation tools: `tutorial_actions_probe.py` and
`tutorial_menu_resource.py`; corresponding fixtures retain exact source identity.

[Jev decisions](JEV-DECISIONS.md) logs all 14 requests, including this checkpoint's
snapshot-origin and cancellation reviews. Original execution determines behavior.

Reproduce native tests with `ctest --test-dir build-linux --output-on-failure`;
use `--render-tutorial-range-smoke` for the new native image. The focused binary
is `tutorial_tests` (or `build-web/tutorial_tests.js` under Emscripten Node).

## Historical native-first tutorial progress in 0.24.0

`tools/tutorial_basic_probe.py` continues the original objective-event VM through
entry and the first lesson (`0x43353c`), capturing all three fresh-profile paths:
accept/continue, accept/decline, and initial decline. Six integrated tests repeat
these paths under both opening story choices. **94 original dialogue/choice
states** match the live and persistent unit records, camera, RNG, animation tick,
terrain, fire records and tutorial cursor. This is bounded original-code evidence;
host text/raster/audio/window operations remain explicit probe boundaries.

The first lesson moves the white cursor down six cells with four-tick waits.
Its map-plane writes are preserved: mark3 is painted, then cleared to255.
The engine guards unverified collisions with another effect. The lesson retains
Cao Cao's unit panel and guide poses through dialogue. **192 new original-code
pose cases** verify animations1/2, including indexed pixels and direction-dependent
mirroring. Original EXE text addresses are retained in the imported UI data;
26 GBK strings decode to Unicode and have complete Chinese atlas coverage.

**94 stable tutorial snapshots** restore and continue identically; **846 mutated
snapshots** are rejected transactionally. Testing reproduced and fixed acceptance
of an extra queued dialogue page despite a recomputed checksum. Collection bytes,
tutorial position/answers, cursor, panel and retained poses are now serialized
under schema4. Fresh profiles start with108 zero collection bytes; the opening
collectible equipment sets item23's byte26. No original `SVCMN.E5S` is read or
changed. Completion/collection reward routines remain guarded and unimplemented.

All **17 C++ suites pass on native Linux x86-64** (22.45 seconds in this run).
The focused tutorial suite also passes on **macOS arm64 and WebAssembly/Node**.
The SDL frontend compiles on all three targets. Native [cursor](evidence/linux-native-tutorial-cursor.png)
and [panel/pose](evidence/linux-native-tutorial-panel.png) screenshots were inspected;
the dialogue is readable and the original white cursor and pose render. The
prototype dialogue overlay obscures much of the unit panel in the latter image;
original dialogue-window placement/composition still needs recovery and comparison.
These screenshots do **not** establish complete original UI identity. No browser
UI testing was performed for this checkpoint.

Both opening traces match across Linux, macOS and Wasm at **1,991/1,950 state
checkpoints**. Choice1 accepts the basic lesson; choice2 declines it. The separate
tutorial suite covers all three tutorial branches under both story choices.
A Linux snapshot with the cursor and Cao Cao panel active restores to the same
state hash on macOS and Wasm. See [parity](evidence/parity-0.24.json) and
[cross-platform snapshot evidence](evidence/tutorial-snapshot-0.24.json).
This does not establish complete player-action, victory or reward parity.

The new `tutorial_ally_probe.py` captures the second original lesson's118 events,
16 speeches, guide movement and Liu Bei's reachability/attack masks. Its39
reachable cells,4 attack-mask cells and cleanup are recorded for the next port.
This is investigation evidence only; the native engine still stops before that
lesson. See `tests/fixtures/original_tutorial_ally.json` and its documented hooks.

See [Jev decisions](JEV-DECISIONS.md) for all twelve requests, including the pose
abstention, save-validator review and second-lesson range investigation. Jev is
advisory; original execution and independent tests determine accepted behavior.

Reproduce: `ctest --test-dir build-linux --output-on-failure`,
`build-mac/tutorial_tests assets/generated`, and the Emscripten Node runtime with
`build-web/tutorial_tests.js assets/generated`. Native render switches are
`--render-tutorial-cursor-smoke` and `--render-tutorial-panel-smoke`.

## Historical native-first progress in 0.23.0

The original objective routine `0x44bd27` is implemented, including row sweeps,
three text layers, GBK byte wrapping, the turn-limit label, text waits, a final
input-interruptible pause and cleanup. 48 original-code layouts and six input
cases pass. The importer now supplies original objective labels and 32/48/64px
Chinese font atlases. Input cannot advance earlier uninterruptible waits; saves
are rejected during the animation. Original Windows font pixels and real-time
message-pump timing remain unverified.

Opcode92 now shows each enemy leader's unit panel and three target flashes.
Fifteen original schedules cover missing/inactive units and first-active-slot
lookup. Twelve original mask cases cover slot255 clearing and out-of-map
coordinates. The shared helper's missing clear-target case is fixed. Integrated
tests verify 张宝 then 张梁, all twelve waits, panel lifetime, unchanged unit/RNG
state, and deterministic continuation from a preceding dialogue snapshot.

Four further original handlers execute sequentially from the prior arrival
state: objective26, highlight92 twice, then appearance76. The complete native
trace matches the resulting units, camera, terrain, flags and RNG before the
explicit tutorial stop. The original reference captures host presentation
requests; it is not a full Windows visual playthrough.

All **16 C++ suites pass on native Linux x86-64 and macOS arm64**. Linux SDL
renderings of the [objective](evidence/linux-native-objective.png) and
[target panel](evidence/linux-native-highlight.png) were inspected: Chinese text
is readable and the original battlefield/sprites render. These automated renders
do not establish manual player input or full original UI identity. Browser UI
checks are deferred under the native-first workflow; WebAssembly compilation and
state comparison are retained at gameplay milestones. The Wasm renderer and core
compile; objective, highlight and core suites pass under Node. Both story choices
match across macOS/Linux/Wasm at **1,929/1,930 checkpoints**, including **81/82
stable snapshot continuations**. See [parity evidence](evidence/parity-0.23.json).
This establishes no player-action or full-battle parity.

Tutorial investigation has captured **68 setups and 32 entry/skip branches**.
The original setup reads completion bytes0–2 and collectible-item bytes at
item+3 from `SVCMN.E5S`. The probe supplies explicit bytes without opening that
save. It stops at the first unverified teaching/reward routine; no tutorial
instruction is skipped in the engine. Both the lessons and collection-dependent
reward branches remain to implement. See [Jev decisions](JEV-DECISIONS.md) for
all nine requests, accepted advice and the rejected sweep-axis answer.

Reproduce the new evidence with `tools/objective_banner_probe.py`,
`tools/highlight_probe.py`, `tools/objective_event_probe.py` and
`tools/tutorial_probe.py`. The corresponding fixtures are
`original_objective_banner.json`, `original_highlight.json`,
`original_objective_event.json` and `original_tutorial_entry.json` under
`tests/fixtures`. The sequential event probe traverses the earlier original-code
chain, so it is slower than the isolated probes.

## New in 0.22.0

The actual area-confusion command selects 13 active enemies in row-major order
using displayed side, plays each original MEFF48 effect, then ORs status 8 into
that unit. A sequential original-code trace captures 949 requests, including
169 animation frames and 13 sounds. The engine checks the state before every
mutation. RNG remains 340946258 and the battle tick remains 4. 604 isolated
original-code cases cover side filters, lifecycle, existing status, rectangle
boundaries and scan order.

MEFF48's 536 original bytes are imported without pixel changes. 327,680 palette
pairs verify the original zero-amount compositor, and the complete sequence's
raster hashes verify all effect frames. The player has a distinct direct-index
path for weight 0/palette 255: 39 frames must preserve raw indices instead of
passing through the quantizing compositor. Both paths are implemented. These
checks use deterministic raster captures at host boundaries; they do not prove
full original-window clipping, timing or audible playback.

Opcode 78's non-targeted AI assignment is checked against 225 original cases,
including missing/inactive/dead units and party-owner protection. The six actual
opening calls leave their units unchanged because the requested AI mode and
flags are already set. A further 37 original handlers execute in sequence,
covering those calls, 15 speech calls (16 prototype pages), the music request,
three appearances and eight walks. Occupied-tile fallback is preserved: Cao Cao
ends at (10,6), cavalry 258 at (9,6), cavalry 259 at (10,7), Liu Bei at (9,5), and the
camera at (240,48). The old generic-dialogue treatment of opcode 26 was removed:
its original handler calls the dedicated banner at 0x44bd27, which is not yet
verified, so the engine stops explicitly before it.

All **14 C++ suites** pass on macOS arm64, Linux x86-64 and WebAssembly/Node;
all **25 importer/font tests** pass. Both choices match across the three
platforms at **1,864/1,865 complete-state checkpoints**. All **81/82 stable
snapshot continuations** match. The post-confusion snapshot restores to the
same complete state on all platforms, retaining all 13 statuses and RNG/event
flags. All 367 original source hashes and 2,383 imported assets are verified.

Chrome has imported that native snapshot into unused slot 7 and recovered the
identical state after page reload. Older slot 6 and a corrupt imported backup
are rejected without changing live state or existing slots. Backup text has
the exact native checksum. The export button reports success, but the download
event tool times out; browser security policy blocks opening Chrome's download
history. Actual downloaded-file restoration remains unverified.
Chrome continues through arrival dialogue and walks to the exact native final
hash at the objective-banner guard. No game runtime error was observed; logged
console warnings originate from an installed extension. One browser-test wait
mistook identical successive cavalry lines for unchanged state; inspecting the
state hash confirmed correct advancement.

Safari now loads the current build, imports the native snapshot, saves empty
slot 3, reloads and restores the exact same hash. A click advances to Zhang Fei's
next dialogue. Visible Chinese and original artwork were inspected. This is a
save/render/input smoke test, not a complete Safari arrival or battle test.
Full browser-process restart, audible playback and actual download restoration
remain unverified.

The Linux package passes **34 rendering checks**, trace comparison and snapshot
restore from `/opt/test/三国 游戏`, with network disabled and no development
mount. Both confusion raster paths and Cao Cao's arrival were inspected. This
establishes package independence for the tested Debian 12 container; it does
not establish original Windows raster identity or full battle equivalence.

Jev was used for three bounded advisory decisions: status investigation priority,
zero-weight compositor coverage, and the next acceptance dependency. Exact
requests, responses, provider metadata, cost and our dispositions are documented
in [JEV-DECISIONS.md](JEV-DECISIONS.md). An initial HTTP 400 due to a request-field
mistake is retained too. No game rule was accepted on model advice alone, and
no speed improvement has been measured.

Evidence: [status command](../tests/fixtures/original_status_event.json),
[area/compositor cases](../tests/fixtures/original_confusion.json),
[AI assignment](../tests/fixtures/original_script_ai.json),
[arrival sequence](../tests/fixtures/original_arrival_event.json),
[platform parity](evidence/parity-0.22.json),
[snapshot restoration](evidence/snapshot-restore-0.22.json),
[macOS tests](evidence/core-tests-macos-0.22.log),
[Linux tests](evidence/build-tests-linux-0.22.log),
[Wasm tests](evidence/core-tests-wasm-0.22.log),
[importer tests](evidence/importer-tests-0.22.log),
[browser checks](evidence/browser-qa-0.22.json),
[Linux package checks](evidence/relocation-0.22.json), and
[checkpoint summary](evidence/checkpoint-0.22.json).

## New in 0.21.0

Opcode 107 now implements the opening's MCALL05 preload (105) and playback
(109). The read-only importer retains the exact 3,780,620-byte resource, all
30 indexed 300×300 images, 12 mask planes, descriptor offsets and descriptor
bytes, including ignored nonzero padding. Five bounded original-code players
verify the loader, worker, selected image/mask bytes, rectangles, restoration
order, sound 112, 100 ms frame waits and cleanup. The script wrapper adds its
original four-tick final wait. The prototype needs no Windows thread or helper.

The masked palette compositor reproduces original `0x4300c8`. **15,597,568**
source/background palette cases cover every original mask value and amounts
0–16, plus mask 255; all **30 full-frame compositions** also match the original
masked/unmasked arithmetic routines on a deterministic background. MCALL's
background uses the saved battle animation tick while the effect plays.
Host clipping and a full Windows screenshot/timing comparison remain unverified;
these references establish pixel arithmetic and requests at raster boundaries.
Audible playback is not claimed.

Scripted animation modes 5 and 8 are now covered. The animation oracle contains
**768 sequences** across units, directions and pre/post waits, with unchanged
unit and RNG state. Sound 112's original filename/flags are checked. Opcode 21
also displays the opening's fallback speaker, **许子将 (unit 157)**, while his
battle unit is inactive, without moving the camera. Other opcode-21 forms remain
explicitly guarded.

A sequential original-code continuation executes **52 source-ordered handlers**
from `0x22c0` through `0x2809`. It includes four pre-summon dialogue pages,
MCALL playback, the remaining 24 fires, and six subsequent enemy speeches.
The engine compares complete unit/persistent records, terrain/effect planes,
camera, tick and RNG at the observed boundaries. Final RNG is **340946258**,
camera **(144,432)**, with 27 fire records. The next status command remains an
explicit stop, rather than an assumed state change.

Snapshots retain the loaded MCALL identity. All four pre-summon dialogue saves
resume through one complete effect; the identity clears after playback. Invalid
resource IDs, missing preloads, fractional/string/null values and disagreement
with script position are rejected transactionally. All **65/66** stable snapshot
continuations for the two opening choices match. The native summon snapshot
restores to the identical complete state in Linux and Wasm.

All **14 C++ suites** pass on macOS arm64, Linux x86-64 and WebAssembly/Node;
all **24 importer/font tests** pass. Both choices match across these platforms
at **1,488/1,489 state checkpoints**. The importer rechecks all 367 original
source files and 2,381 imported assets.

Chrome imports the native summon snapshot, saves empty slot 6, reloads the page
and restores its exact hash. It continues through the summon, all fires and
enemy dialogue to the same final native hash. Older slot 5 and a backup with a
recomputed checksum but missing MCALL preload are rejected without changing the
live state or existing saves. Browser backup text has the same checksum and
MCALL identity as the native snapshot. No game runtime/console error was seen.
Actual downloaded-file restoration and full browser-process restart remain
unverified for this checkpoint. Safari is still unavailable because the Mac is
locked; no 0.21 Safari success is claimed.

The isolated Linux package passes 31 rendering checks, state-trace comparison
and snapshot restore from `/opt/test/三国 游戏`, without network or a development
mount. Summon frames, readable Chinese and the 27-fire scene were inspected.
The summon-dialogue smoke initially matched an earlier story speech; its
predicate now requires turn two and the loaded MCALL resource, and that corrected
scene is rerun separately. These checks do not establish original Windows
raster equivalence or a complete opening-battle playthrough.

Evidence: [original summon player](../tests/fixtures/original_mcall.json),
[original palette arithmetic](../tests/fixtures/original_mcall_blend.json),
[sequential event](../tests/fixtures/original_summon_event.json),
[platform parity](evidence/parity-0.21.json), and
[importer checks](evidence/importer-tests-0.21.log),
[browser QA](evidence/browser-qa-0.21.json), and
[packaged Linux rendering](evidence/relocation-0.21.json).

## New in 0.20.0

Opcode 33 placement mode 0 now executes with the original terrain-effect table:
250 five-byte records retain position, new terrain, previous terrain and effect
kind. Fire changes a tile to terrain **26** and sets its separate effect mark
to **0**. The original first-free-or-matching-slot search, duplicate replacement,
full-table behavior, focus/silent flags, sound **19**, one-tick wait and redraw
requests are compared against **56 bounded original-code cases**, including all
27 fire placements present in the opening script. These isolated placement
cases do not establish the remaining event sequence.

The original new-battle loop initializes all 1,250 record bytes to 255. The
reference map allocation now includes the separate effect plane. Four original
48×48 fire frames at `Mark.e5:0x8bc0` are imported without pixel substitution;
**256 original tick selections** verify their offsets and indexed-pixel hashes.
The frontend draws fire beneath units and selection graphics. Original sound
filename/flag dispatch is also checked. Audible playback and the wall-clock idle
animation cadence remain unverified.

A sequential original reference continues from the verified turn-two dialogue
through **13 source-ordered original opcode handlers**: Zhang Fei's animation,
first fire, Guan Yu's movement and dialogue, his animation and two fires, and
intervening delays. It stops before opcode 107 at `0x22c0`. The engine matches
unit and persistent records, terrain/effect records and marks, camera, animation
tick and RNG. RNG remains **340946258**. This continuation calls original
handlers with exact script bytes; it does not claim to run the whole Windows
interpreter or render its text windows. The earlier next-turn report reused an
event list while capturing dialogue; that reporting-only alias is corrected.
Its prior controller, unit, RNG, banner and dialogue results are unchanged.

Snapshots include the complete terrain-effect table and mark plane. A stable
save at Guan Yu's “这里也点上！” retains Zhang Fei's existing fire. Restoring it
continues identically through the remaining two placements. Nine malformed
record/plane cases are rejected without changing live state, including modified
terrain, missing records, duplicate coordinates and non-integer values.

All **14 C++ suites** pass on macOS arm64, Linux x86-64 and WebAssembly/Node.
All **23 importer/font tests** pass, including fire bitmap pixel equality and
all 367 original-source / 2,379 imported-asset hashes. Both opening choices match
at **1,308/1,309 state checkpoints**; **55/56** stable snapshot continuations
match. The fire-dialogue snapshot restores identically on all three platforms.
Chrome imports the native snapshot, saves empty slot 5 and restores the same
state hash after page reload. It continues to the native final hash, rejects an
older slot and a recomputed-checksum backup with inconsistent fire marks without
changing live state, and exposes backup text with the native checksum. The export
button reported success but the test tooling received no download event in ten
seconds; actual downloaded-file restoration is not claimed. No game runtime or
console errors were observed. Separate browser-extension warnings were present.

The packaged Linux build passes **27 rendering checks**, trace parity and
snapshot restore from `/opt/test/三国 游戏`, without network or a development
mount. The new first-fire dialogue and three-fire scene were visually inspected.
Safari's current rerun is unavailable because the Mac is locked; no 0.20 Safari
success is claimed. Evidence: [checkpoint](evidence/checkpoint-0.20.json),
[original placements](../tests/fixtures/original_fire.json),
[sequential original handlers](../tests/fixtures/original_fire_event.json),
[platform parity](evidence/parity-0.20.json),
[browser QA](evidence/browser-qa-0.20.json), and
[offline Linux package](evidence/relocation-0.20.json).

Fire removal, opcode 107's effect, later fire placements in their event context,
Cao Cao's arrival and player controls remain explicit unverified work. These
CPU references do not establish an interactive Windows playthrough or full
opening-battle equivalence.

## New in 0.19.0

The opening opcode-69 handler and the original controller initialization block
confirm region **1**, environment selector **1**, and turn limit **20**. The
previous isolated next-turn research case used zero-filled controller fields;
it was explicitly provisional and is superseded by the initialized sequential
reference. Replaying the corrected reference leaves all first-turn enemy unit,
RNG, camera and animation-tick results unchanged.

The original loop now verifies the transition into turn two: increment the
zero-based turn byte, check outcome and limit, consume the environment random
value, display the player-turn banner and reset action flags, then select event
`0x20a1`. RNG advances **3885567453 → 340946258**. Region/selector remain **1/1**
and the visible environment remains unchanged. The environment module compares
**1,680 original-code cases** across all five classification rows, six selectors,
eight random outcomes and both random/forced updates. Changed-environment
presentation and turn-limit defeat remain explicit atomic stops.

The original dispatcher executes the first four speech calls of the fire-attack
event, stopping before opcode 80. This exposed missing speaker camera focus in
the prototype. Battlefield dialogue now centers on each active speaker, including
after save/reload; retired speakers are skipped as the original wrapper
does. The turn-two speech camera positions are **(240,48), (192,48), (288,48),
(240,48)** for Liu Bei, Guan Yu, Zhang Fei and Liu Bei. The reinforcement dialogue
reference now also runs the original opcode-20 handler rather than assuming it
has no camera effects. Temporary action-flag presentation for a speaker who has
both acted and moved remains guarded until verified.

Snapshots now include environment region/selector and pending speaker-focus
requests. The engine version prevents older snapshots from being misread, and
invalid environment/focus fields are rejected transactionally. Four new stable
turn-two dialogue save points are exercised for both opening choices. The
prototype continues through Zhang Fei's already-covered animation 6 and stops
at `S_00.eex:0x21c7`; fire tiles, the subsequent summon and Cao Cao's entrance
sequence still need implementation and original-code comparison.

All **14 C++ suites** pass on macOS arm64, Linux x86-64 and WebAssembly/Node.
Both story choices match at **1,278/1,279 state checkpoints**. All **54/55** stable
snapshot continuations match; the native turn-two snapshot restores identically
on Linux and Wasm. All 367 original-source and 2,378 imported-asset hashes match.
The importer is unchanged; its 22 tests last ran at 0.18.

The isolated packaged Linux app passes **25 rendering checks**, including the
new turn banner and dialogue, from `/opt/test/三国 游戏` without network or a
development mount. The two new images were visually inspected for Chinese
text, original art and layout. Chrome imports the native turn-two snapshot,
saves empty slot 4, reloads and restores the exact hash, then continues through
the four dialogue pages to the same final native hash. The older slot 3 is
rejected without changing state. Browser backup text has the native snapshot
checksum and preserves environment/RNG fields. Safari's rerun remains
pending because the Mac is locked; no 0.19 Safari success is claimed.

Detailed results are recorded in `evidence/checkpoint-0.19.json`.
These CPU references do not establish interactive Windows timing, audible music
or a complete opening-battle playthrough.

## New in 0.18.0

The original MEFF#49 poison-status and MEFF#45 miss animations are decoded and
rendered with their original pixels. The moving rectangles restore the prior
frame's background, rather than the next frame's position. Every frame retains
its blend parameters, sound request and 50 ms interval. Ten original-code cases
cover both resources at the target and four map corners; **851,968** exhaustive
source/background palette pairs cover all 13 parameter combinations. The
original sound dispatcher separately confirms sound IDs 129 and 131 and their
filenames. Audible playback has not been assessed.

Zhang Liang's second cast now applies poison bit 16 **before** its status
animation. Infantry 210 takes no HP damage (110 remains 110); Zhang Liang spends
48 → 40 MP and finishes his action. Complete units, persistent records, RNG,
camera and animation tick match the sequential original-code reference.

The remaining original enemy actions are captured through their unit callers
and source-ordered post-action event/death calls. Units 474, 476, 477, 478 and
479 attack; units 480–485 finish movement without an available attack. The outer
original caller marks moved units as acted even when no attack occurs; their
post-action events therefore still run. This is now reproduced. Final RNG is
**3885567453**, camera is **(288,384)** and animation tick is **4**. No phase-end
or next-turn behavior is claimed from this reference.

All **14 C++ suites** pass on macOS arm64, Linux x86-64 and WebAssembly/Node,
plus **22 importer/font tests**. The integrated test initially assumed every AI
movement has a queued frame; a stationary enemy has none. The test now compares
that empty sequence with the original, and targeted macOS/Linux core reruns
pass. The complete Wasm suite passes after this correction. Both choices match
across platforms at **1,223/1,224 complete-state checkpoints**. All **50/51**
stable snapshot continuations remain identical through the extended sequence.

Chrome imports the native death-dialogue snapshot, saves slot 3, reloads the
page and restores the identical state. It continues to the same final hash as
native/Linux/Wasm, with no application error and saving disabled while unstable.
Safari's 0.18 rerun is pending because computer-use reports the Mac locked; the
user was asked to unlock. Successful 0.17 Safari results do not verify this
version. Full browser process restart and audible playback remain unverified.

The packaged Linux build passes all **23 rendering checks**, trace comparison
and snapshot restoration with no network or development-directory mount, from
`/opt/test/三国 游戏`. The poison-status sprite and final diagnostic were
visually inspected. All **367 source and 2,378 imported asset hashes** were
rechecked. A native death-dialogue snapshot restores to identical complete state
on Linux and Wasm.

Evidence: [secondary effects](../tests/fixtures/original_secondary_effect.json),
[second enemy caller](../tests/fixtures/original_second_enemy.json),
[remaining enemy actions](../tests/fixtures/original_enemy_continuation.json),
[sound dispatch](../tests/fixtures/original_presentation.json),
[importer checks](evidence/importer-0.18.log),
[platform parity](evidence/parity-0.18.json),
[macOS suites](evidence/tests-0.18-mac.log),
[macOS core rerun](evidence/tests-0.18-mac-core.log),
[Linux suites](evidence/tests-0.18-linux.log),
[Linux core rerun](evidence/tests-0.18-linux-core.log),
[Wasm suites](evidence/tests-0.18-web.log),
[browser checks](evidence/browser-qa-0.18.json),
[isolated package checks](evidence/relocation-0.18.json),
[poison-status rendering](evidence/linux-relocated-poison-status-0.18.png),
[checkpoint record](evidence/checkpoint-0.18.json).

Follow-on [turn-transition research](evidence/next-turn-research-0.18.json)
executes `0x449f91(255)` as an isolated case. With the reference VM's default
zero environment/region fields, it changes selector 0 → 1 and RNG
3885567453 → 340946258 without a visible classification change. These controller
fields' original opening initialization is **not yet verified**; the result is
not integrated or claimed as next-turn gameplay. Recovering that initialization
is required before continuing the full turn loop.

Next: enemy-phase completion and next-turn events/maintenance, player movement
and combat commands, battle outcomes, rewards and post-battle dialogue. All
unimplemented required branches retain explicit diagnostics. The full opening
battle and full campaign are not equivalent to the original yet.

## Historical: 0.17.0

**The opening-battle milestone is incomplete. This is an investigation build,
not a playable replacement for 颍川之战.** The original opening story, first-turn
allied actions, enemy banner, reinforcements and first enemy poison cast execute.
The archer now speaks his final line and retires. Zhang Liang then moves and casts
poison at infantry 210. Execution stops before the unimplemented secondary
poison-status animation, **MEFF#49** (`S_00.eex:0x3b5a`, original `0x401e5c`).
Player controls, complete turns, victory/defeat, turn limits and rewards remain.

Current source/build: **0.17.0-investigation**, snapshot schema **3**, importer
**0.9.0**. The asset manifest is unchanged:
`5b9dd297b08d4a66e34f60585293734e76dcea2e1641ca44580ee74125ece95a`.
Earlier prototype snapshots are rejected without overwriting their slots.
Historical results below apply to their versions.

## New in 0.17.0

The original event selector, interpreter dispatcher, dialogue handler and flag
setter execute through the archer's death event at `0x3af7`. He says
“唔，援军…援军还不来吗……？” before variable 55 is set. Two 18-scan event batches,
the seven blink redraws, four waits, death sound, retirement, target cleanup and
caster redraw match the reference. His lifecycle changes from 2 to 3; RNG stays
**1775811865**. The reference harness now also executes the reinforcement event's
trailing flag-11 write, avoiding a spurious repeated reinforcement event in
later scans. Original source files are not changed.

The death dialogue is a supported stable save point. Enemy AI order, cursor,
completed tactic, pending event return and original event flags survive restore.
Finished tactic plans are discarded before saving. Tests reject wrong-side AI
orders, invalid tactics/targets, unfinished casts and invalid return stages
without changing the live state. All **50/51** stable dialogue continuations
match for the two opening choices, including the next random attack result.

The next original enemy action is also captured through its complete caller.
Zhang Liang selects infantry 210: damage misses (**0**), poison succeeds (**112**),
and RNG reaches **2402686401**. The replacement matches the decision, movement,
target preview, plan, cast, caption and 13-frame primary effect. It then stops
atomically before the secondary MEFF#49 animation: infantry HP remains 110,
status remains 0 and Zhang Liang retains 48 MP. The original reference continues
to status 16 and 40 MP; **those final writes are not yet integrated**.

All 14 C++ suites pass across macOS arm64, Linux x86-64 and WebAssembly/Node.
The first test run exposed an extra reference-only character-window notification
in the new retirement comparison; the targeted rerun documents its explicit
host boundary. Both opening traces match across all three platforms:
**591/592 checkpoints**. The native death-dialogue snapshot restores to the same
complete state on Linux and Wasm:
`3682bb9306bb8180572182847facdde2b9736e75c311022e8a8c272be6f4cccc`.
Importer behavior is unchanged; its 21 tests last passed in 0.15.

Chrome and Safari render the death dialogue, save it in slot 2, reload the page,
restore its identical state and continue to the same secondary-effect guard.
Chrome also rejects older/corrupt snapshots without changing live state; its
export is byte-identical to the native snapshot. Full browser process restart,
audible playback and a continuous player-controlled battle remain unverified.
The isolated Linux package passes all **21 render modes**, the 591-checkpoint
trace and snapshot restoration with no network or development-directory mount,
from a path containing Chinese and spaces. All 367 source and 2,374 imported
asset hashes were rechecked.

Evidence: [post-poison caller stages](../tests/fixtures/original_poison_post.json),
[second original enemy action](../tests/fixtures/original_second_enemy.json),
[platform parity](evidence/parity-0.17.json),
[macOS suites](evidence/tests-0.17-mac.log),
[macOS retirement rerun](evidence/tests-0.17-mac-retirement.log),
[Linux suites](evidence/tests-0.17-linux.log),
[Linux retirement rerun](evidence/tests-0.17-linux-retirement.log),
[Wasm suites](evidence/tests-0.17-web.log),
[browser checks](evidence/browser-qa-0.17.json),
[isolated package checks](evidence/relocation-0.17.json),
[checkpoint record](evidence/checkpoint-0.17.json).

Next: secondary status/miss effects, remaining enemy actions, phase transitions,
player commands, victory/defeat/turn limit, rewards and post-battle dialogue.
No continuous playable battle or full-campaign equivalence is claimed.

## Historical: 0.16.0

The original first enemy caller (`0x43dc99`) now runs through casting, MEFF
playback, target application, success gating, MP/XP finalization and action flags
in the CPU reference. The replacement matches its complete unit records, RNG,
camera and animation tick. The allied archer loses **10 → 0 HP**, the caster
spends **48 → 40 MP**, its flags change **4 → 6**, and RNG remains **1775811865**.
The defeated archer is still present at this checkpoint: retirement and event
dispatch belong to the following caller stage.

**1,108 original-code cases** cover lethal/nonlethal damage, status success and
failure, existing status bits, tick boundaries, camera focus, all 104 equipment
IDs, MP limits/discounts, armor and weapon experience, canceled finalization and
automatic-item suspension. Poison applies status bit 16 only when the planned
signed damage is less than current HP. Lethal poison therefore does not add the
status. A surviving poisoned target requests MEFF#49; a complete miss requests
MEFF#45. Their state rules are compared, but these secondary animations are
not yet rendered by the replacement. The integrated engine stops before
committing any such unimplemented result. Automatic healing, level windows and
party XP remain explicit guards. No required branch is silently skipped.

All **14 C++ suites pass on macOS arm64, Linux x86-64 and WebAssembly/Node**.
Both opening choices produce identical complete traces across all three:
**537/538 checkpoints**. All 49/50 stable dialogue snapshot continuations match,
including RNG and event flags. Linux renders readable HP and MP result panels.
The importer and asset data are unchanged from 0.15 (21 importer/font tests
passed there); no new importer behavior is claimed.

Browser interaction checks for 0.16 are pending because computer-use reports
that the Mac is locked. This may explain the suspended Safari frames observed
in 0.15, but the cause has not been confirmed by an unlocked rerun. Chrome's
0.15 results do not substitute for testing this version. The packaged Linux build passes all 18 rendering checks, the 537-checkpoint
trace and snapshot restoration from a path containing Chinese and spaces,
with networking disabled and no development-directory mount. A native
reinforcement snapshot also restores to identical complete state in Linux and
WebAssembly. Audible playback, full browser process restart and
a complete battle remain unverified.

Evidence: [application/finalization fixtures](../tests/fixtures/original_tactic_application.json),
[complete first poison caller](../tests/fixtures/original_poison_controller.json),
[three-platform parity](evidence/parity-0.16.json),
[HP panel](evidence/linux-tactic-damage-0.16.png),
[MP panel](evidence/linux-tactic-cost-0.16.png),
[macOS suites](evidence/tests-0.16-mac.log),
[final macOS core/application checks](evidence/tests-0.16-mac-final.log),
[Linux suites](evidence/tests-0.16-linux.log),
[Wasm suites](evidence/tests-0.16-web.log),
[isolated package checks](evidence/relocation-0.16.json),
[browser verification status](evidence/browser-qa-0.16.json),
[checkpoint record](evidence/checkpoint-0.16.json).

Follow-on original-code research reaches event `0x3af7`: the defeated archer
speaks at `0x3b24` before retirement, then the script sets variable 55. The probe
suspends at this body; this dialogue/return sequence is not yet integrated.
Enemy-phase tactic snapshots also need their own restore validation before
this stable dialogue can support saving. See the
[research handoff](evidence/poison-post-research-0.16.json).

Next: post-tactic events and retirement, secondary status/miss effects, remaining
AI phases, player commands, victory/defeat/turn limit and rewards. This checkpoint
does not satisfy the milestone or establish identical gameplay.

## Historical: 0.15.0

The importer decodes `Meff.e5#23`: 13 original 75×90 indexed frames, descriptor
offsets and callback bytes, 100 ms intervals and sound 119 (`SE_M_19.WAV`). It
rejects unsupported descriptors, truncation and trailing bytes. Original
resource bytes remain available beside the versioned metadata.

The engine preserves the original camera, capture/restore requests, frame order,
one-shot sound, target reaction callback and final redraw. The callback draws
the reaction and planned damage on frame 1. It **does not apply HP or poison**.
Nine original-code player cases cover the actual action and corner positions
with all four combinations of damage/status success. Another 244 callback cases
cover all 53 classes, four directions and action-flag brightness changes.
Every frame's indexed pixels are checked against the decoded originals.

The poison effect uses the original quantized palette compositor. All **65,536
source/background color pairs** match original `0x42fd5a`; all 117 effect frames
also match against a deterministic synthetic background. SDL now paints to a
640×440 target before viewport scaling, so the compositor receives original
logical pixels on Linux and WebGL. Linux rendering shows the original effect
and readable damage digits. This is not a pixel-for-pixel Windows screenshot
comparison: host raster calls are reference boundaries, and fonts differ.

All **13 C++ suites pass on macOS arm64, Linux x86-64 and WebAssembly/Node**;
all **21 importer/font tests pass**. Both choices produce identical complete
presentation/state traces across these builds: **523/524 checkpoints**.
All 49/50 stable dialogue snapshots preserve continuation, event flags, RNG,
casting and poison frames. A native reinforcement snapshot restores to the
same complete state in Linux and WebAssembly. At the new boundary the archer
still has 10 HP, the planned damage is 10, poison effect code is 0x70, RNG is
1775811865 and battle tick is 6. No application or completed enemy action is
claimed.

Chrome imports the native snapshot, saves, reloads, overwrites and restores its
slot, rejects corrupt and older backups, and continues through the complete
poison animation to the same boundary hash with no runtime/console errors.
Safari imports, saves and restores the same state, but its tab reports hidden
visibility and zero animation frames (including an independent browser frame
probe). Safari rendering and continuation are unresolved for this version.
The packaged Linux build passes all 16 render modes, the 523-checkpoint trace
and snapshot restoration in a Chinese-and-spaces path, with networking disabled
and no development-directory mount. Audible playback, full browser process
restart and a complete battle remain unverified.

Evidence: [original poison player and reaction fixtures](../tests/fixtures/original_tactic_effect.json),
[three-platform parity](evidence/parity-0.15.json),
[Linux poison rendering](evidence/linux-poison-0.15.png),
[macOS tests](evidence/tests-0.15-mac.log),
[Linux tests](evidence/tests-0.15-linux.log),
[Wasm tests](evidence/tests-0.15-web.log),
[importer tests](evidence/tests-0.15-importer.log),
[browser checks](evidence/browser-qa-0.15.json),
[isolated package checks](evidence/relocation-0.15.json),
[checkpoint record](evidence/checkpoint-0.15.json).

Next: target application (`0x4224a8`), caster MP/XP/finalization (`0x422f0d`),
action flags and post-action events; then remaining phases, player commands,
victory/defeat/turn limit and rewards. This checkpoint does not satisfy the
milestone or establish identical gameplay.

## Historical: 0.14.0

The caster now faces the target, uses the original 64×64 directional attack
sprite, focuses the target and issues sounds 8 and 39. The Chinese tactic name
appears at the original glyph positions and palette color, on the decoded
`Mark.e5` caption background with the original border geometry. Closing the
caption and restoring the caster's background retain the original request
order. **235 original-code cases pass**, covering all 53 classes, four facing
directions, three message speeds, elapsed-clock boundaries, existing flags,
changed/unchanged camera focus and the opening's three tactic names. Three
additional window fixtures verify indexed background pixels and border lines.

The actual first cast reaches camera (144,576), with RNG **1775811865** and
battle tick **6**. HP, poison status, MP and experience are still unapplied.
The engine uses default message speed 0 and an explicit deterministic caption
clock input. Real rendering latency never affects state or randomness; exact
Windows wall-clock timing and Windows font shapes remain unverified. The
reference executes original sprite mirrors, caption formatting and GBK cursor
logic; raster, font, window, host clock, audio and wait operations are documented
boundaries. This is not a completed tactic or a recorded Windows playthrough.

All **13 C++ suites pass on macOS arm64, Linux x86-64 and WebAssembly/Node**;
all **20 importer/font tests pass**. Both opening choices produce identical full
presentation/state traces across the three builds: **506/507 checkpoints**.
Restoring each of 49/50 stable dialogue snapshots preserves continuation,
completed event flags, poison rolls and the new caster sequence. A native
reinforcement snapshot restores identically in Linux and WebAssembly.

Linux rendering verifies the original caption background, readable Chinese
and caster pose. Chrome and Safari restore a native snapshot, save in IndexedDB
and resume after page reload through casting without a runtime error. Chrome
also verifies overwrite/reload and rejects corrupt and older-version backups.
Browser testing caught an unmapped sound 39; the original sound routine confirms
`SE39.WAV`, and the final core/animation suites pass on all three platforms with
a regression checking that each emitted sound has a bundled resource.
The packaged Linux build passes the 506-checkpoint trace, snapshot restoration
and all 15 render modes in a Chinese-and-spaces path with networking disabled
and no development-directory mount. Audible playback, a full browser process
restart and a complete battle remain unverified.

Evidence: [caster fixtures](../tests/fixtures/original_tactic_cast.json),
[caption window fixtures](../tests/fixtures/original_caption_window.json),
[three-platform parity](evidence/parity-0.14.json),
[Linux caster rendering](evidence/linux-tactic-cast-0.14.png),
[macOS tests](evidence/tests-0.14-mac.log),
[Linux tests](evidence/tests-0.14-linux.log),
[Wasm tests](evidence/tests-0.14-web.log),
[browser verification](evidence/browser-qa-0.14.json),
[isolated package checks](evidence/relocation-0.14.json),
[checkpoint record](evidence/checkpoint-0.14.json).

Next: decode and execute the original `MEFF.E5#23` effect playback and callbacks,
target application (`0x4224a8`), caster MP/XP/finalization (`0x422f0d`), action
flags and post-action events; then remaining phases, player actions,
victory/defeat/turn limit and rewards. This checkpoint does not satisfy the
milestone or establish identical gameplay.

## Historical: 0.13.0

The first enemy now displays the original two-pass tactic selector, range,
target highlight and target panel. It preserves actor facing, advances the
original animation tick from 4 to 6 and focuses the camera at (144,480).
The HP-loss preview and Chinese labels are decoded from this edition.
**246 original-code preview cases pass**, including original glyph positions,
bar draw requests, hit chances and deterministic damage estimates for 钝兵,
虚脱 and 毒烟. The new text is included in the Chinese glyph atlas.

The actual 毒烟 controller makes **separate status and damage hit rolls**.
The damage roll can also consume a bounded random variation. The engine now
matches **181 original-code plans**, including misses, poisoned/immobilized
targets, different HP and seeds. The first action prepares 10 damage and status
code 0x70, with final RNG **1775811865**. HP, status, MP and experience have not
been applied at this boundary. NPC experience is zero under the verified gate;
party experience branches remain explicit guards.

These references run the original selector and pre-animation controller after
the previously verified reinforcement/maintenance/decision/walk sequence.
Raster, window, audio and waits remain explicit reference boundaries. The
controller probe stops at `0x4214bf`; no casting animation, completed enemy
action or full Windows gameplay recording is claimed.

All **13 C++ suites pass on macOS arm64, Linux x86-64 and WebAssembly/Node**;
all **19 importer/font tests pass**. Both opening choices have identical full
presentation/state traces across the three builds: **498/499 checkpoints**.
Restoring each of 49/50 stable dialogue snapshots preserves continuation,
completed event flags, the preview and both poison rolls. A native reinforcement
snapshot also restores identically in Linux and WebAssembly. All eight compiled
enemy walking frames match the original indexed pixel hashes.

Linux rendering verifies the targeting panel, readable Chinese damage/hit
labels and explicit casting boundary. Chrome and Safari import the native
reinforcement snapshot, save it in IndexedDB, restore after page reload and
reach the same final state hash. Chrome also overwrites the isolated test slot,
restores that newer state after reload and rejects a corrupt backup without
changing progress; its application console has no errors. The packaged Linux
build passes the 498-checkpoint trace, snapshot restoration and all 14 render
modes in a path containing Chinese and spaces, with networking disabled and
no development-directory mount. Exact original wall-clock timing,
Windows font shapes, audible playback, browser process restart and a complete
battle remain unverified.

Evidence: [tactic panels](../tests/fixtures/original_tactic_panel.json),
[poison plans](../tests/fixtures/original_tactic_plan.json),
[three-platform parity](evidence/parity-0.13.json),
[Linux tactic preview](evidence/linux-tactic-preview-0.13.png),
[macOS tests](evidence/tests-0.13-mac.log),
[Linux tests](evidence/tests-0.13-linux.log),
[Wasm tests](evidence/tests-0.13-web.log),
[browser verification](evidence/browser-qa-0.13.json),
[isolated package checks](evidence/relocation-0.13.json),
[checkpoint record](evidence/checkpoint-0.13.json).

Next: original casting animation (`0x4214bf` → `0x4218d6`, effect animation
loader/player), target effect application (`0x4224a8`), caster MP/XP/finalization
(`0x422f0d`), action flags and post-action events; then remaining enemy/allied
phases, player actions, victory/defeat/turn limit and rewards. This checkpoint
does not satisfy the milestone or establish identical gameplay.

## Historical: 0.12.0

Enemy tactic scoring now covers this opening's admitted 钝兵, 虚脱 and 毒烟
branches. The first two receive zero in the original AI scoring switch; this
is verified behavior, not a placeholder. 毒烟 scoring executes the original
hit/damage rules and consumes random numbers while evaluating candidate tiles.
The engine commits those draws with the decision, including draws for candidates
it does not select. This is necessary to keep subsequent combat and restored
runs consistent.

**558 original-code pair evaluations, 280 candidate scores and 24 complete AI
decisions pass**, including final RNG. Cases cover target status/grades,
player-AI exceptions, all 53 target classes, equipment bonuses/resistance,
HP thresholds, following targets and different seeds. Unsupported tactics
and area branches still stop explicitly.

The source-ordered original reinforcement/maintenance VM continues through
张宝 (ID 146, slot 35) choosing 毒烟 against 弓兵1 (slot 20) and walking from
(9,11) to (9,13). The integrated engine matches every represented live and
persistent unit, RNG **835903122**, tick and camera at decision and movement
completion. All eight compiled enemy walking frames also match the original
indexed pixel hashes on all three platforms. The reference stops at `0x43b3a2` before tactic selection/application.
Raster/window/wait/audio requests remain explicit reference boundaries; this
is not a full Windows gameplay recording. No complete enemy action is claimed.

All **13 C++ suites pass on macOS arm64, Linux x86-64 and WebAssembly/Node**;
all **18 importer/font tests pass**. Both opening choices have identical full
presentation/state traces across the three builds: **492/493 checkpoints**.
Save/restore continuation covers 49/50 stable dialogue points without repeating
completed events or changing the enemy decision/RNG. The native reinforcement
snapshot restores identically in Linux and WebAssembly.

Chrome imports the native snapshot, saves/reloads it in IndexedDB, overwrites
the isolated test slot and restores that newer state after page reload.
A corrupt snapshot is rejected without changing live progress. Dialogue hashes
and the final explicit tactic-resolution stop match native execution, with no
application console errors. Safari also imports, saves and reloads the native
checkpoint and plays the same dialogue/entrance/enemy continuation to the exact
same final state hash. Linux rendering checks show the enemy's original
walking sprite and the explicit diagnostic. The packaged Linux build also completes the 492-checkpoint trace, snapshot
restoration and 13 rendering checks in a Chinese/space path with no network
or development-directory mount. Browser process restart, audible
playback, exact original timing and a complete battle are not verified here.

Evidence: [tactic fixtures](../tests/fixtures/original_tactic_scoring.json),
[first enemy continuation](../tests/fixtures/original_enemy_phase.json),
[three-platform parity](evidence/parity-0.12.json),
[Linux enemy movement](evidence/linux-enemy-walk-0.12.png),
[macOS tests](evidence/tests-0.12-mac.log),
[Linux tests](evidence/tests-0.12-linux.log),
[Wasm tests](evidence/tests-0.12-web.log),
[browser verification](evidence/browser-qa-0.12.json),
[isolated package checks](evidence/relocation-0.12.json),
[checkpoint record](evidence/checkpoint-0.12.json).

Next: original tactic selection mode 2 and `0x43dc99`, including casting,
damage/status/MP/XP changes, animation and post-action events; then remaining
enemy/allied phases, player actions, victory/defeat/turn limit and rewards.
This checkpoint does not satisfy the milestone or establish identical gameplay.

## Historical: 0.11.0

Liu Bei, Guan Yu and Zhang Fei now walk from their entrance positions into the
original formation: (11,5), (10,5), and (12,5). Opcode 50 uses the recovered
movement wrapper, route search, camera requests, indexed sprite frames and
wait sequence. It restores AI mode, action flags, target coordinates and final
facing at the original points. Completed event flag 11 is set once and the
enemy phase proceeds to maintenance and priority selection.

**38 original-code movement cases and 484 indexed frame hashes pass.** These
include all three sequential source commands and 35 synthetic variations for
flags, facing, occupied destinations, invalid destinations and immobilization.
Every deployed unit, persistent record, RNG, battle tick and camera matches the
reference after each entrance walk and enemy maintenance. Raster/OS/wait and
camera requests are explicit reference boundaries; this is not a full original
Windows playthrough. Hidden/retired movement, invalid saved target coordinates
and unverified selector modes remain explicit diagnostics.

All **13 C++ suites pass on macOS arm64, Linux x86-64 and WebAssembly/Node**;
all **18 importer/font tests pass**. Both opening choices produce identical
full presentation/state traces across the three builds: **476/477 checkpoints**.
The snapshot continuation tests cover 49/50 stable dialogue points and ensure
restoration replays the complete entrance without duplicating completed events.
Native-to-Linux-to-Wasm restoration of the reinforcement snapshot is identical.
These are checks through the new explicit stopping point, not a victory test.

Chrome restores the native reinforcement snapshot, saves/reloads it through
IndexedDB, overwrites the test slot and restores its newer state. Corrupt and
0.10 snapshots are rejected without changing the live state or stored slot.
The four dialogue states and final entrance state match native hashes; there
are no application console errors. The two new Linux render checks show the
walk and final formation. A relocated package also passes the 476-checkpoint
trace, snapshot restoration and 11 render modes with networking disabled, no
development directory mounted, and a path containing Chinese and spaces.

Safari 0.11 verification is pending because native UI access was blocked by
the locked Mac; the 0.10 Safari results remain historical evidence.
Audible playback, a complete browser process restart and a full battle were
not re-verified by these checks.

Evidence: [movement fixture](../tests/fixtures/original_script_movement.json),
[three-platform parity](evidence/parity-0.11.json),
[Linux formation](evidence/linux-after-reinforcement-0.11.png),
[macOS tests](evidence/tests-0.11-mac.log),
[Linux tests](evidence/tests-0.11-linux.log),
[Wasm tests](evidence/tests-0.11-web.log),
[Chrome checks](evidence/browser-qa-0.11.json),
[isolated package checks](evidence/relocation-0.11.json),
[checkpoint record](evidence/checkpoint-0.11.json).

The next required work is the original enemy tactic scorer and resolution,
followed by remaining AI phases, player input, continuous victory/defeat and
post-battle rewards/dialogue. This checkpoint does not satisfy the milestone.

## Historical: 0.10.0

The enemy-phase transition executes the original right-to-left checker sweep,
three title paint requests, wait, action-flag reset, minimap request and clearing
sweep. The reset clears bits 2/4 on every represented unit, including hidden
and retired units, at the original point between the title wait and clearing.
It does not change RNG, battle tick, HP, equipment or persistent records.

**96 original-executable banner cases** compare both sides, preserve/reset
flags, six turn numbers (including the byte-wrap case), four viewport/client
rectangle inputs, checker pixel hashes and ordered text/column/wait requests.
The original caller, checker generation and shadow-rectangle helper run;
Windows font and column rasterization are explicit reference boundaries.
The 640×440 client rectangle is a prototype input, not a verified original
window measurement. Source Han title atlases at 48/64px avoid magnifying the
dialogue font; Windows font pixel identity and original timing remain unknown.

The continuous allied reference now also runs the original enemy banner and
event selector. It selects event `0x1ea4` before enemy maintenance or AI.
The original opcode-76 and opcode-79 handlers then execute in that source
event's order, with dialogue treated as a documented presentation boundary.
Every deployed unit, persistent record, RNG, tick and camera matches the port
at entry, the new dialogue points and the first movement instruction.
This command-level continuation does not claim a complete original interpreter
or original dialogue-window capture. All three reinforcements appear and four
dialogue pages play, then execution stops explicitly at opcode 50.

All **13** test suites pass on macOS and Linux x86-64; all **13** WebAssembly
executables pass under Node. **18** importer/text tests pass, including original
source preservation and coverage of all title glyphs. The prior 0.9 continuous
allied fixture was regenerated byte-for-byte. Both choice traces match across
macOS, Linux and WebAssembly at **437 / 438 checkpoints**. Stable reinforcement
dialogue snapshots restore with identical subsequent states and no repeated
allied attack or reset. The Linux renderer displays the banner and the new
dialogue with original sprites and readable, unclipped Chinese.

Chrome and Safari both import the Linux-compatible reinforcement snapshot,
save to a previously empty slot on the isolated `127.0.0.1:8767` origin, reload
and restore the exact state hash. All four dialogue hashes and the final explicit
movement-boundary hash match native execution. Chrome also overwrites its new
test slot at Guan Yu's dialogue, restores that state after reload, rejects a
corrupt snapshot without changing the current Zhang Fei state, and restores an
earlier infantry-event backup. Continuing that earlier backup executes all three
allied attacks, the banner and reinforcements to the same first Liu Bei dialogue.
Browser export is byte-identical to the native snapshot and restores on Linux.
No application console errors were captured in Chrome; Safari's runtime log
contains normal initialization. These are page reload tests, not full browser
process restarts. Existing save origins and original game saves were preserved.

The packaged Linux bundle also completes the 437-checkpoint trace and snapshot
restore in `/opt/test/三国 游戏`, with no network or development-directory mount.
It renders nine smoke-test views, including the new banner and reinforcements.
Audible sound/music, exact original wall-clock timing, Windows font shapes,
original window geometry and full-gameplay equivalence remain unverified.

The next required implementation is opcode 50 (`0x4132d4` → `0x459487`), then
the rest of this event, enemy maintenance/AI, subsequent turns and player input.
Victory, defeat, turn-limit, rewards and the complete opening battle are still
unverified. No milestone or full-campaign equivalence gate has been met.

See [trace parity](evidence/parity-0.10.json),
[browser QA](evidence/browser-qa-0.10.json),
[relocation](evidence/relocation-0.10.json),
[turn-banner reference](../tests/fixtures/original_turn_banner.json),
[enemy-entry reference](../tests/fixtures/original_enemy_entry.json),
[Linux banner](evidence/linux-banner-0.10.png) and
[reinforcement dialogue](evidence/linux-reinforcement-0.10.png).

## Historical checkpoint 0.9.0

The post-action caller now preserves the initial AI ordering and its cursor.
It records the action notification, runs the first event batch, handles defeated
NPCs, redraws the actor, and runs the second event batch before selecting the
next unit. Event dialogue retains a return point so restoring it cannot restart
phase initialization or repeat the completed attack.

A new bounded original-executable probe executes the allied caller, all three
AI decisions, walking, selections, physical attacks, seven event batches and
first NPC defeat continuously. No AI, combat, event condition or RNG result is
supplied by the replacement engine. Its input is the separately captured
post-introduction state with the first infantry event completed. Raster,
window, audio and wait operations remain explicit reference boundaries; this
is not a full interactive original-game capture.

| Actor | Destination | Target HP afterward | RNG afterward |
| --- | --- | --- | --- |
| Archer 234 / slot 20 | (9,17) | 45 | 2516284547 |
| Infantry 210 / slot 18 | (10,16) | 14 | 267834847 |
| Infantry 211 / slot 19 | (8,16) | 0 | 2937909499 |

The integrated engine compares every deployed live unit and persistent record,
RNG, animation tick and camera after each movement and completed attack.
The final original caller reaches the enemy-turn banner with target 475 in
lifecycle 3, status cleared, and the same final roster as the port.

Another **302 original-executable defeat cases** check non-party blinking,
waits, sound request, occupancy visibility, lifecycle/status/mirror cleanup,
and AI target release. They cover six units, five flag combinations, two status
values, five follower AI modes, and already-hidden/retired units. Party
retirement dialogue and record counters still stop explicitly. The original
minimap refresh is retained as a presentation request; its separate Windows
minimap window is not reproduced by this frontend.

All **13** test suites pass on macOS and Linux x86-64 and all **13** WebAssembly
executables pass under Node. The updated snapshot-validation tests also pass
on all three targets. A synthetic stable event interruption after the first
attack exercises the saved AI order/return point and compares every future
state hash after restoration. Eleven malformed continuation variants are
rejected transactionally. This fixture is explicitly a test input, not a claim
that the unmodified opening naturally shows another dialogue at that point.

Both complete current traces match across the three targets at **387 / 388
checkpoints**. Choice 1 ends at
`277dd02d87ad2e072b2f255c201ff2f86bb3130f0a22d7cd1fad174f3c83a322`;
choice 2 ends at
`67570c9d0d6b3f332aa498090fdeaaaf483a0dce8ad15cfdbced0b24719f5715`.
These are bounded investigation traces, not victory playthroughs.

Native Linux rendering covers the first defeat blink and battlefield after
removal. The packaged Linux build completes the same 387-checkpoint trace and
restores a snapshot from `/opt/test/三国 游戏` with networking disabled and no
development mount. Seven presentation screenshots are produced there.

Chrome and Safari import the native event snapshot, preserve a new save across
page reload, and reach both the exact critical-dialogue hash and final allied
boundary hash. Chrome uses slot 10; Safari uses slot 7. Chrome test slot 9 was
mistakenly selected once during QA; its prior 0.8 snapshot was restored from
the saved native backup and verified loading in 0.8 after reload. Its timestamp
changed; its snapshot state was recovered. The independent packaged web build
on port 8766 reaches the same final state without application console errors.
Critical dialogue correctly disables saving/export and no longer shows a save
hint. A snapshot exported by the packaged browser is byte-identical to the native
snapshot and restores on Linux with the expected state hash. These are page
reloads, not full browser-process restart tests.

All 367 source files in the asset manifest still match their original hashes.
Audio audibility, original wall-clock timing, the separate minimap window and
exact screen composition are not established by these tests.

Next work remains enemy-phase presentation/maintenance and decisions, tactics,
party retirement, player movement/combat/cancellation, subsequent event paths,
and continuous victory/defeat/turn-limit tests. None of the opening-battle or
full-campaign equivalence acceptance gates is being declared complete.

See [0.9 test evidence](evidence/checkpoint-0.9.json),
[browser QA](evidence/browser-qa-0.9.json),
[relocation](evidence/relocation-0.9.json),
[trace parity](evidence/parity-0.9.json),
[continuous allied reference](../tests/fixtures/original_allied_phase.json),
[defeated-unit reference](../tests/fixtures/original_defeated_unit.json) and
[Linux battlefield after removal](evidence/linux-after-defeat-0.9.png).

## Historical checkpoint 0.8.0


The physical attack controller is integrated into native and browser gameplay.
It preserves strike planning, critical-speech RNG, animation requests, HP/MP and
status application, double-strike checks, XP requests and recursive counters.
It operates on copies and explicitly rejects unsupported area attacks, automatic
items and level-up windows. The first NPC attack exercises the supported path;
it does not establish every hero, item, level-up or lethal-attack path.

- 73 complete original-executable controller cases cover 121 strikes, including
  40 counters and 8 second strikes, with 1,536 indexed frame hashes. The original
  critical-message wrapper executes, including its camera recentering and input
  flush request. Text windows and raster I/O are explicit oracle boundaries.
- 264 attack-preview panels match the original damage/hit display. Another
  10,816 panel placements and 3,200 attack-camera destinations match. The first
  preview reports 28 damage and 90% hit chance; its subsequent critical roll
  raises the actual damage to 45.
- 32 NPC damage-panel cases match header text, icons, HP/MP bar geometry and all
  five interpolation steps. Player-party XP panels remain unsupported.
- 189 isolated attack animations match 2,830 indexed frame hashes, ordered
  frames, sounds, waits and battle-tick increments. All 4,096 quantized palette
  lookup entries match. Original pixels are retained in the imported assets.
- 2,264 character-XP and 2,187 equipment-XP cases compare persistent records and
  ordered level requests. Character tests use the original no-animation option;
  they do not verify live HP or level-up windows. 1,957 strike-application cases
  include 11 explicit suspensions before automatic healing items.
- 5,120 critical-speech cases compare message selection and final RNG; 1,024
  selected messages also compare original GBK bytes. The importer now includes
  all 51 combat message records 700–750 and covers all 1,050 atlas glyphs.

All **13** native CTest suites pass on macOS and Linux x86-64, all **13**
WebAssembly test executables pass under Node, and **17** importer/text tests
pass. Both choice traces agree across macOS, Linux and WebAssembly at every
one of their 268/269 checkpoints. The first attack ends with RNG 2516284547,
actor flags 6, battle tick 6, target HP 45 and camera (144,576).

The relocated Linux bundle renders the event, preview, critical dialogue, hit
effect and HP panel from `/opt/test/三国 游戏`, with networking disabled and no
development mount. Its native snapshot restores with the expected hash. The
separately served `dist/web` bundle also imports that snapshot and reaches the
same completed-attack boundary without application console errors. Browser
offline caching is not established.

Chrome and Safari both import the native event snapshot, preserve new saves
across page reloads, display the critical speech and reach the same post-attack
state hash. Chrome slot 9 and Safari slot 6 were used; prior slots remain intact.
Saving/exporting is disabled during a pending attack, including its dialogue.
Core tests restore earlier stable snapshots and compare subsequent events and
random outcomes. These are page reload tests, not full browser-process restarts.

Safari initially stalled during resource loading. Reloading succeeded twice;
the root cause was not established. Startup logs now retain messages and
unhandled errors so a recurring stall can be diagnosed. Browser keyboard input
is bound to the canvas so using page controls cannot also advance dialogue.

Linux preview, critical speech, hit effect and interpolated HP-panel images
were inspected for original art and readable Chinese. Exact original screen
composition, wall-clock timing, audible music/effect transitions and a full
original interactive playthrough are **not yet verified**. No complete
victory/defeat/turn-limit or full-campaign equivalence is claimed.

See [0.8 test evidence](evidence/checkpoint-0.8.json),
[browser checks](evidence/browser-qa-0.8.json),
[trace parity](evidence/parity.json) and
[relocation checks](evidence/relocation-0.8.json).

The next original-code probe now records the nonlethal post-attack sequence:
first action-event batch, death scan, unit redraw, second event batch, then
selection of infantry unit 210/slot 18. Each batch scans all 18 events and the
AI priority is not recomputed in this case. Its AI action is an explicit supplied
input boundary, and no death handler executes. This is investigation evidence;
the 0.8 runtime still stops before that sequence.

## Historical checkpoint 0.7.0

The combined AI decision and walking modules are now connected to the browser
and native controller. Both opening choices reach the same archer movement,
with target slot 38, tactic 255, score 26 and unchanged RNG. No attack is skipped
to expose later units or turns.

- 129 original-executable decisions match destination, attack target, score,
  complete distance/threat/score grids, and candidate evaluation order. Cases
  cover modes 0–4, low HP, scripted-movement flags, target changes and status
  effects. The first archer chooses (9,17) from (12,17), targeting slot 38 with
  score 26. Tactic and area scoring still stop explicitly when required.
- 68 tactic records match the original loader, including its rearranged header
  fields and Chinese names; 2,226 class/level/MP availability cases match.
  Data.e5#5 holds 73 disk records, but this edition loads only 68. The importer
  retains the complete raw archive and emits the 68 loaded records separately.
- 1,749 original walking sequences cover all 53 classes, three speeds, four
  directions and viewport boundaries. Frame pixels/orientation are checked
  against original indexed sprites while the original mirror code executes.
  Frame order, waits, scroll requests and occupied-tile overlay order match.
- 45 complete AI movement cases match final coordinates, facing/mirror,
  occupancy, moved flags, viewport and presentation requests. The movement
  oracle's selection boundary is covered separately by 280 original selection
  sequences, including the original input-interruption branches.
- 265 normal unit panels match original text, Chinese/ASCII cursor placement,
  HP/MP bar scaling, icons and terrain-affinity values. The formatter and GBK
  cursor execute in the original probe; font glyph rasterization is a boundary.
- 136 display-range masks, 384 tile-layer combinations and all 256 palette
  darkening entries match their original routines. The importer retains the
  original Mark.e5 pixels for cursors, range marks, icons and bar patterns.

The runtime uses the uninterrupted selection sequence. Input interruption,
exact wall-clock tick duration, full Windows palette/display conversion, action
preview panels and full original UI layout remain unverified. Intermediate
walking unit-state mutations have not yet been compared frame by frame.
The first movement is not user-controlled movement/cancellation.

The next combat modules now match 1,297 original strike preparations, 392
critical/double chance cases and 958 counterattack gates. Strike preparation
compares HP/MP damage, critical flags, XP requests, equipment interactions and
final RNG, including critical rolls after misses and secondary targets. A
synthetic original divide-by-zero case is rejected atomically. Counter probes
stop at facing and recursive attack requests. These modules are **not yet
connected to gameplay**: target-list assembly, HP/XP application, status effects,
death processing and attack presentation are still required. Numeric XP requests
do not establish level-up or equipment-experience application.

All ten CTest suites pass on macOS and Linux x86-64; all ten Wasm executables
pass under Node. Fourteen importer tests pass, including original source and
asset hashes and coverage of all 997 atlas glyphs. Linux/Wasm traces match all 217/218
checkpoints for the two choices. Chrome slot 8 and Safari slot 5 restore the
Linux snapshot after a page reload and continue to the identical movement
boundary hash. Earlier slots are preserved. A downloaded Chrome backup is
byte-identical to the Linux snapshot and restores natively. These tests do not
cover a full browser-process restart or continuous victory/defeat gameplay.

The packaged Linux executable renders both the event dialogue and selection
panel and restores its snapshot from `/opt/test/三国 游戏` in a fresh container
with networking disabled and no development mount. The relocated images were
inspected. See [0.7 relocation evidence](evidence/relocation-0.7.json).
The separately served browser bundle also imports the downloaded backup and
reaches the identical first-movement boundary, with no application console
errors observed. This is a local HTTP bundle test, not offline browser caching.

See [0.7 evidence](evidence/checkpoint-0.7.json),
[browser checks](evidence/browser-qa-0.7.json) and
[trace parity](evidence/parity.json). Historical unconnected-module evidence is
retained in `evidence/ai-movement-next.json`.

## Historical checkpoint 0.6.0

The first infantry event now continues through player-side status/grade recovery,
poison/regeneration preflight, the automatic empty-player-phase exit, its camera
focus (10,17), allied event dispatch and AI ordering. The original controller
probe executes those calls together through entry to the first AI action. It
confirms actor 234 in slot 20, phase 1, unchanged unit records and RNG, and
18 selector scans per empty event batch. The engine no longer shortens that
bounded event scan. Completed event 0x1e00 is still recorded exactly once.

Recovery with visible effects remains an explicit stop: the engine computes on
copies and commits only a fully supported phase. An unsupported animation,
poison display or regeneration/XP request cannot partially change units or RNG.
The normal first player phase needs none of these effects and advances using
the generic verified rules. This is not a bypass of the AI or a completed turn.

The poison/regeneration module matches **1,913** original cases, including
terrain healing, equipment effects, capped HP/MP, effect-source precedence and
the original zero-HP poison edge. Numeric XP requests are covered; XP application,
level-up animation and numeric display are not claimed. **2,417** recovery cases
continue to match effect requests and RNG.

AI ordering now matches **3,182** original cases across all 53 jobs, 30 terrain
values, HP boundaries, lifecycles and control flags. **271** original player-phase
cases cover eligible units and focus selection, including the supplied AutoDLL
patch with automatic saving disabled. A test caught and corrected a fixture
collector's shared camera-list reference before acceptance; the generated
fixture now preserves each case's actual focus. These probes run in a bounded
CPU emulator, not a Windows process or DLL loader.

A new movement module matches **344 original distance grids and return values**
and **2,968 movement-point cases**. It preserves row/neighbor traversal order,
terrain costs, friendly occupancy, opposing slot-range allegiance, zones of
control, Red Hare/Di Lu equipment effects, paralysis, temporary grades, and
an early target match. Another **120** original cases cover mode 8's first visible
target lookup, including stale references and competing target cells. Mode 4
action scoring remains unimplemented.

Route reconstruction and fallback position scores match **550** original cases,
including limited budgets, zones of control, job 15/16 exceptions, and the
original routine's dependence on caller scratch after an interrupted descent.
The route routine does not append a terminator; callers must use their endpoint.
Movement animation/cancellation and player commands remain unimplemented.
Passing these grid and route tests does not make movement available in the UI.

All 16 range shapes, boundary clipping, ally/enemy selection, preferred-target
ordering and separate output masks match **1,376** cases; class/equipment range
selection matches **106** cases. AI threat maps and low-HP healing priorities
match **1,596** cases. The original paints opponents using the acting unit's
attack range. This prepares candidate positions but does not choose an action.

Physical hit chance and damage now match **2,106** original cases with actual
unit, equipment, terrain and grade getters executing without rule stubs. Both
the returned damage and final RNG state match, including missed attacks,
guaranteed chances, confusion, ranged immunity and counterattack scaling.
Another **2,184** cases compare all five battle stat getters. These supersede
the earlier arithmetic-only coverage for this numeric subset. Critical/double
attack selection, HP/XP application, death events and presentation remain
unimplemented; this does not establish a complete attack action.

Single-target AI attack scores and target choices now match **1,720** original
cases. The comparison covers kill priority, low-damage choices, confusion,
named-unit priority, class restrictions, counterattack-range penalties and
first-target tie-breaking. Area-attack scoring stops explicitly and has a
guard test. Tactic scoring and the combined move/action decision remain open;
the engine still stops before executing its first AI action.

Native macOS and Linux CTest pass **10/10**; all ten WebAssembly test executables
pass under Node. Both story choices and every stable snapshot still resume to
the same diagnostic boundary, including an atomic-failure recovery test.
Linux and Wasm match **200/201** checkpoints for the two choices, with **45/46**
stable snapshot continuations. See `evidence/parity.json`.

Chrome slot 7 and Safari slot 4 save/reload checks restore the identical first
infantry state hash and continue to the identical AI boundary hash. Safari
imports the actual downloaded Chrome backup; that file is byte-identical to
the Linux snapshot and also restores natively. Earlier browser slots remain
preserved. Readable Chinese and original graphics were inspected. These are
page-reload checks, not full browser-process restarts. See
`evidence/browser-qa-0.6.json` for the observed scope.

The 0.6 Linux bundle renders and restores the same dialogue from
`/opt/test/三国 游戏` in a fresh container with networking disabled and no
development mounts. The rendered image was inspected. All 367 original source
files were rehashed and remain unchanged. See `evidence/relocation-0.6.json`.

## Historical checkpoint 0.5.0

The section-1 event selector now follows the original executable: evaluate every
condition, including outcome-latch side effects, and choose the **last** eligible
event. Required opening conditions cover variables, turn comparisons, phase,
occupied positions, adjacency, HP and outcome latches. A hash-gated CPU probe
executes the actual selector, dispatcher and condition handlers with no rule
hooks. C++ matches **159 cases**, including simultaneous death events, proximity,
turn/phase changes, prior completion flags and both outcome latches set.

The engine continuously reaches the first-turn infantry dialogue at `0x1e4f`,
executes its facing/attack gesture and sets flag 14 once. Stable snapshots within
that event resume identically; invalid scheduler fields are rejected before
changing state. This is event dispatch, not a completed turn or playable combat.

Original placement search now replaces the earlier occupied/impassable-tile
stop. **1,485 comparisons** cover all 27 movement groups, map edges, occupied
positions, local search ordering, Manhattan-distance fallback and no available
tile. Original opcode 76 appearance is also implemented with lifecycle/mirror
updates, placement, redraw/camera ordering, sound 15 and its four-tick pause.
**36 original-handler comparisons** cover ID/party-slot selection, occupied entry
positions, already-visible/removed units and a missing unit. These reinforcement
handlers are tested in isolation; the ordinary playthrough cannot reach the
first enemy-phase reinforcement until AI turns are implemented. Special apparition
animations remain explicit unsupported cases.

A separate probe executes the original first player-side status/grade recovery,
poison and terrain/equipment recovery calls in caller order. For the actual
post-introduction input, no live or persistent unit bytes change. This observation
is recorded in `original_turn_start.json`; generic turn-start execution and its
presentation are not yet implemented. AI has not been bypassed to expose later
script scenes.

The recovery rules module now matches **2,417 original-executable cases** for
status recovery, all six temporary grades, equipment-assisted recovery and
neighboring healer effects. It compares state mutations, effect request order
and final RNG state, including mixed grades and multiple healers. Recovery reads
base luck; confusion at exactly 100% skips a random draw, while other status
checks still consume one. Healer allegiance is determined by slot range rather
than the displayed side field. These rules are not yet connected to turn
presentation; poison, regeneration and AI remain unfinished.

Native macOS and Linux CTest pass **6/6**. All six WebAssembly test executables
pass under Node. Linux and Wasm match **199/200** presentation/state checkpoints
for the two opening choices, with **45/46** stable snapshot continuations. The
native first-turn dialogue is recorded in `evidence/linux-event-0.5.png`.

Chrome and Safari both import the Linux victory-instruction snapshot, continue
to the first infantry dialogue, save to an empty slot, reload the page and
restore the same state hash:
`f3aa41bc6727cc33e1adc1536f0b289c665aea02a982a79ec35589bb0cc551f8`.
Chrome uses slot 6 and Safari slot 3; earlier slots are preserved. Safari's
visible canvas and readable Chinese dialogue were inspected. This supersedes
the earlier Safari lock/rendering limitation for this subset. A complete browser
process restart and downloaded backup-file restoration are still unverified.

The 0.5 Linux bundle renders and restores the first infantry snapshot from
`/opt/test/三国 游戏` in a fresh container with networking disabled and no
development directory mounted. The relocated image was inspected. All 367
original source hashes were rechecked and remain unchanged. The first original
AI-order probe also runs without rule hooks: visible allied order is slots
20, 18, 19. This is research evidence; AI actions are still unimplemented.

## What exists

A separate C++20 core, SDL3 presentation frontend, Emscripten browser build,
Linux x86-64 executable, read-only importer, versioned manifests, script listings,
ten prototype save slots and JSON snapshot backups. OrbStack's daemon was
started and Linux binaries were built and tested in a local amd64 container.
The packaged prototype has no Wine runtime, Windows executable or music helper.
Wine was used only for an isolated reference attempt in the development cache.

The original game, Electron project and original save slots remain separate.
All 367 source files still match the recorded hashes. Browser slots are in
`CaoCaoEnginePrototype`; Linux slots are in the new `caocao-engine` data directory.
Prototype saves are not original `.e5s` saves.

## New in checkpoint 0.4.0

Opcode 83's two opening removals now execute their original blink sequence,
sound 16, lifecycle change to 3, status/mirror cleanup and AI target release.
Removed units stay removed after saving and restoring. Opcode 79 now turns
units toward a target or an explicit direction, including the original no-op
when already facing the requested direction and the reverse-facing behavior.
Sound 6 accompanies a turn. Animation 7 repeats the four attack frames twice
with two-tick pauses and restores the normal sprite; animation 9 alternates
wounded frames 9/10 three times with eight-tick pauses.

The new-battle entry investigation corrected an earlier integration mistake:
`0x44c38b` flag **0** selects party, enemies, then allies for a new game; flag 1
belongs to the loading path. The opening new-game focus is therefore (9,11),
with camera pixels (144,336), rather than the prior (10,17). The surrounding
cleanup loop also sets Cao Cao's control mode to 7 and clears flags/status
bits. A bounded probe runs that original loop across 108 cases. This supersedes
the old initial-focus claim below. Full original window/fade composition is
still unverified.

Current comparison coverage includes 576 original animation sequences and
1,920 indexed frame hashes; 1,596 direction selections; 160 facing sequences;
35 removal sequences (including self-target, mirror and status cleanup); nine
script commands both independently and sequentially; 1,200 centered and 1,200
visibility-preserving camera paths; and 14 sound filename/flag cases. The core
also checks the final intro roster against the sequential original commands.
The probes execute original rule code but omit documented Win32/raster/audio
boundaries. They do not establish whole-game equivalence.

Native macOS/Linux CTest passes 4/4; WebAssembly core, action and animation tests
pass under Node. Linux and Wasm match every state/presentation checkpoint for
both choices (186/187), with 44/45 stable snapshot continuations. These traces
cover the introduction, not playable turns. Native Linux/Xvfb renders readable
victory-condition text with the original map and unit graphics.

Chrome imports a Linux wounded-archer snapshot and plays continuously through
both removals, the second attack, wounded/turning gestures and the remaining
intro dialogue. Its victory-instruction state hash is
`fa0652fe3768fb818041a0b085c1589c7fe9fc5e288bb78faf7dc63403df46b1`;
the displayed backup checksum matches Linux exactly. Slot 5 save, page reload
and restore preserve that state. Loading older slot 4 or a deliberately corrupt
backup fails without changing it. Original browser slots 1–4 are preserved.
Safari 0.4 testing could not begin because macOS was locked; the user was asked
to unlock it while independent checks continued. The 0.3 Safari result below
is historical, not validation of the current binary. Full browser process
restart, downloaded-file export, original audio listening and full battle
interaction remain unverified. A fresh network-disabled Debian container also
renders this introduction and restores the same hash from `/opt/test/三国 游戏`
with only a staged bundle mounted, without the development directory.
See [relocation evidence](evidence/relocation-0.4.json).

## Historical checkpoint 0.3.0

Opcode 80 now drives the SDL frontend through verified animation kinds 0
(regional redraw), 3 (hit pose), 4 (wounded pose), and 6 (attack). The shared core
queues frame, sound and wait events; it advances identically without a renderer.
Unverified animation kinds still stop with the original file and byte offset.

A bounded CPU probe executes original routine `0x45778f`, including original
sprite selection and horizontal mirroring. C++ matches 256 sequences and 384
indexed-pixel hashes. Another probe matches 1,200 camera paths and ten original
sound filename/API-flag cases. The original 624×384 viewport begins at y=56;
initial focus is tile (10,17), yielding camera pixels (192,576). Camera changes
redraw the view, while stationary hit/wounded poses persist until the scripted
regional redraw or a camera redraw. The attack and impact cues use `Se07.wav`
and `Se08.wav`; the following scripted vocal cue uses `Se36.wav`.

Native macOS/Linux tests and WebAssembly tests under Node pass. Both opening
choices match all 92/93 Linux/Wasm state and presentation checkpoints. All
30/31 stable snapshots per branch resume identically, including animation
continuation from the preceding dialogue and the new retained-pose dialogue.
Malformed pose references, hidden-unit poses, pending animations in a stable
snapshot, corrupt checksums and incompatible schema/engine versions are
rejected transactionally. Save operations during animations are disallowed.

Chrome plays from the Linux first-battle snapshot through the attack to the
wounded dialogue with no engine error. Saving slot 4, reloading the page and
restoring it preserves the exact state hash and original wounded sprite.
Loading older slot 3 fails by version without changing current state or its
stored timestamp. Native SDL under Xvfb renders the wounded dialogue with
readable Chinese and the retained pose. Safari now also renders visibly, plays
the attack and reaches the same state hash; slot 2 saves and remains present
after reload. The final Safari reload-and-restore click was interrupted when
the Mac locked, so that last check is not recorded as passing. The prior hidden
window/zero-frame limitation was absent during this run. See
[evidence/browser-qa-0.3.json](evidence/browser-qa-0.3.json).

These are scripted presentation checks, not player combat. Original full-screen
composition, smoothness under real machine load, idle cadence and audio listening
quality remain unverified. Camera redraws are presented on browser/native frame
boundaries; the recovered original camera loop has no explicit sleep. Script
and dialogue pacing outside the new animation queue still use provisional
frontend delays. The animation oracle omits Win32 composition, cursor restoration
and audio hardware. The initial battle presentation skips the original window
setup/fade timing. Public distribution and campaign equivalence remain out of scope.

## Evidence and limits

| Area | Verified | Remaining / limitation |
|---|---|---|
| Archives | Bounded LS11/Ls12 decompression; original block IDs, offsets and hashes; index overlap, truncation, output limits, repeated back-references tested | Other campaign archives not investigated |
| Import publication | Simulated rename failure restores the published directory; a new run recovers a prior interruption between publication renames; source edition and font notice checked before import | Concurrent import processes and power-loss durability are not validated |
| Graphics | Original Hm00 terrain image, forest scene, 228 portraits, battle sprite frames, eight opening story sprite sheets, item/UI bitmaps decoded; PE resources read without loading DLLs | Story projection, camera framing, animation timing and dialogue layout remain provisional; not pixel-identical to original UI |
| Character data | 512 original loader/portrait records; 3,104 growth, 5,300 default-equipment, seven party-average, 80 join and 351 NPC-growth fixtures pass | Full item effects and statistics used by attacks/tactics still need integration and reference checks |
| Scripts | Complete typed parse of R_00 and S_00: all sections, event conditions and original offsets; 487 instructions including structural markers | Section-0 setup, live construction, HP commands, six animation kinds, removal and facing execute; first-turn event selection executes; later actions and phase/AI dispatch remain incomplete |
| Opening story | Both choices execute their text/control-flow branches, flag 300 and ambition 60/40; unsupported execution stops explicitly | R_00 join/equipment now calculate persistent state, including 倚天剑 and its collection flag; visual/audio timing still differs |
| Battle initialization | All 26 live units match original construction, including persistent statistics/equipment, hidden reinforcements, counters, side, position, HP/MP, status grades and AI mode; three scripted HP reductions match | 1,485 placement and 36 ordinary appearance cases pass; special apparition forms remain unsupported; no player turn yet |
| Terrain and sprites | 3,180 original movement-cost lookups; sprite/frame, walking, movement range and pathfinding comparisons described above | 2,400 centered/visibility camera paths match; wall-clock idle cadence unverified |
| RNG | Original LCG, physical hit/damage, strike/critical/double roll ordering match bounded CPU fixtures | Initial seed selection and whole-battle call order not verified |
| Physical damage | 2,106 hit/damage and 2,184 stat cases; 1,297 strike plans, 392 critical/double chances and 958 counter gates; 73 complete single-target controller cases and integrated first attack | Automatic healing, level-up windows, area attacks, death events and later battle sequences remain incomplete |
| Movement and AI preparation | Original distance grids, routes, range shapes, threat maps, target lookups and combined decisions match; first archer movement integrated | Tactic/area scoring, remaining AI phases, player commands and movement cancellation remain unimplemented |
| Linux/WebAssembly parity | Both opening choices match across Linux/macOS/Wasm at2,163/1,950 checkpoints through the third tutorial lesson/decline;826 stable tutorial continuations pass | No player battle action traces or full battle parity exist |
| Snapshots | Schema, engine and asset identity; SHA-256 envelope; transactional restoration; corrupt/incompatible rejection; older-state restoration; pending dialogue and RNG included | Battle-intro snapshots now validate live units, unique slots/positions and descriptor consistency; later battle state completeness still unproven |
| Browser storage (0.1.0 historical checks) | Chrome: separate slots, overwrite, older-slot restore, page reload, transaction abort, Linux snapshot import, corrupt import rejection without live-state changes | Chrome process restart not exercised; downloadable file destination not inspected |
| Cross-platform backup (0.1.0 historical checks) | A Linux snapshot restores in WebAssembly; Chrome's displayed backup payload has the same canonical checksum as that Linux snapshot | Actual battle snapshots and battle RNG continuation not tested |
| Chinese | Imported atlas glyph coverage checked, including new tactic/panel labels; dialogue readable in Chrome/Safari and native Linux; normal panel PNG inspected | All dialogue pages, descriptions, preview panels and original save UI still need visual comparison |
| Audio | SDL native/browser decode path for original MP3/WAV; opening bird-effect filename corrected; no helper required by the replacement | Loop semantics, transitions, every sound cue and listening quality unverified |
| Linux rendering | Native x86-64 ELF runs with SDL under Xvfb; story and first battlefield dialogue images inspected with readable Chinese, original map and original unit sprites | Interactive player battle actions remain unavailable |
| Linux bundle relocation | 0.8 bundle renders first infantry dialogue and the attack preview, critical speech, hit effect and HP panel, and restores its snapshot from `/opt/test/三国 游戏` with networking disabled and no development mount; relocated PNGs inspected | Pixel identity is not asserted. Desktop interaction, other Linux distributions and complete battle behavior remain untested |
| Safari 26.5 (0.1.0 historical check) | WebAssembly initializes; Linux snapshot import, local save, page reload/restore produce the same state hash; IndexedDB rollback passes | The automated window reports `visibilityState=hidden`; both SDL and independent requestAnimationFrame counters stay at zero. Canvas rendering and interactive play are unverified |

Historical 0.2.0 Chrome checks passed for the first battlefield snapshot: native Linux
import, slot 3 save, page reload/restore, identical exported payload checksum,
corrupt import rejection with an unchanged live-state hash, and restore after
the explicit opcode-80 stop. The larger snapshot exposed a Wasm stack overflow
in `ccall`'s string conversion; the browser bridge now transfers bounded UTF-8
through heap memory and frees it on success or failure. Older prototype slots
remain present and are rejected by version/manifest without being overwritten.
This is a page-reload test, not a full Chrome process-restart test.

The 0.2.0 Safari retest could not obtain an accessible native application
window. The current 0.3.0 visible-rendering and first-attack checks supersede
that historical limitation for this tested subset; full Safari battle acceptance
is still incomplete.

## Acceptance checklist

- [ ] Continuous browser new-game-to-victory, rewards and post-battle dialogue.
- [ ] Defeat and turn-limit paths.
- [ ] Complete battle movement, cancellation, attacks, tactics/items and AI.
- [ ] Required script instruction coverage, including all alternative branches.
- [ ] Original-edition comparison fixtures for movement, terrain, XP, AI and event order.
- [ ] Battle trace identity between Linux and WebAssembly.
- [ ] Battle save/reload with identical next attack, event flags and rewards.
- [ ] Complete Chrome and Safari playthroughs.
- [ ] Native Linux battle rendering/input checks.

## Original reference run

The reference attempt used a disposable cloned prefix and game copy in
`.cache/reference`, not the original game or the user's writable Electron game.
An initial launch remained on the intro screen. Retrying with the existing
lossless Press.avi compatibility copy reached the title/load menus; the session
later became unresponsive and logged invalid-window/heap errors. Its Wine server
was stopped using only the disposable prefix. See
[evidence/original-reference-attempt.log](evidence/original-reference-attempt.log).
This attempt does not establish any new battle reference behavior. Earlier
Electron observations are useful leads, but do not prove this replacement's
compatibility.

## Next engineering work

1. Establish a repeatable original battle trace for visual/timing and gameplay
   comparisons; isolated CPU fixtures do not replace a full reference playthrough.
2. Continue from the player gate after turn-two maintenance: recover its
   interactive-entry camera/UI/tick setup and player loop after the ASave patch
   at `0x44e1d3`. The fresh-profile NPC poison display/camera/HP order now passes.
   Preserve the raw
   partial Ghidra dossier; keep any flow annotation separately labeled and
   validate it against original input traces. All four fresh-profile tutorial
   lessons and exit now pass; collection-dependent reward branches remain.
3. Finish remaining AI phases, player commands and movement cancellation,
   automatic items, player-party result/XP panels, retirement and level-up
   presentation. Compare full raster composition and secondary-target effects
   before accepting area attacks. Recovery/poison/regeneration presentation
   remains required for later turns and altered states; its current guard must
   not be removed silently.
4. Complete and compare victory, defeat, turn limit, rewards and post-battle paths.
5. Run the full persistence, browser and Linux acceptance suite against those paths.

Google accounts, cloud saves, rankings, deployment and later scenarios remain
outside this checkpoint. No campaign-equivalence claim is made.

## Reproducible evidence

- [0.29 checkpoint](evidence/checkpoint-0.29.json), [maintenance fixture](../tests/fixtures/original_phase_maintenance.json), [parity](evidence/parity-0.29.json), and [snapshot interoperability](evidence/tutorial-snapshot-0.29.json).
- [0.28 checkpoint](evidence/checkpoint-0.28.json), [Cerberus workflow](CERBERUS-RE.md), [parity](evidence/parity-0.28.json), and [snapshot interoperability](evidence/tutorial-snapshot-0.28.json).
- [0.7 checkpoint](evidence/checkpoint-0.7.json): fixture counts/hashes and platform test logs.
- [0.7 offline Linux relocation](evidence/relocation-0.7.json): dialogue and selection rendering, native snapshot restore and binary hashes.
- [0.7 browser checks](evidence/browser-qa-0.7.json): native import, Chrome/Safari page reload and native restoration of an actual browser export.
- [0.7 selection rendering](evidence/linux-selection-0.7.png) and [walking rendering](evidence/linux-walking-0.7.png).
- [Strike preparation](../tests/fixtures/original_strikes.json) and [counter gates](../tests/fixtures/original_counters.json): module evidence, not full attack execution.
- [0.6 offline relocation](evidence/relocation-0.6.json) and [inspected Linux rendering](evidence/linux-relocated-event-0.6.png).
- [0.6 browser checks](evidence/browser-qa-0.6.json): Chrome/Safari persistence, actual backup download/import and identical AI boundary.
- [0.6 route fixtures](../tests/fixtures/original_routes.json): 550 scratch-buffer and fallback-score comparisons.
- [0.6 range and AI preparation](../tests/fixtures/original_ranges.json): shapes, targeting, threat maps and first-target searches.
- [0.6 physical resolution](../tests/fixtures/original_physical_resolution.json): real stat/terrain/equipment/hit getters and final RNG.
- [0.6 AI attack scoring](../tests/fixtures/original_ai_attack.json): 1,720 original single-target scores and choices; no action execution.
- [0.5 relocation](evidence/relocation-0.5.json): offline packaged Linux rendering and snapshot restoration.
- [0.5 browser checks](evidence/browser-qa-0.5.json): Chrome/Safari first event and save/reload identity.
- [0.5 Linux first event](evidence/linux-event-0.5.png): readable original infantry dialogue.
- [Recovery fixtures](../tests/fixtures/original_recovery.json): 2,417 bounded original-executable cases.

- [0.4 browser checks](evidence/browser-qa-0.4.json): scripted intro playback, native snapshot identity and Chrome persistence.
- [0.4 Linux introduction](evidence/linux-intro-0.4.png): inspected victory-instruction rendering.
- [Browser checks](evidence/browser-qa.json): observed UI results and untested cases.
- [0.3.0 browser checks](evidence/browser-qa-0.3.json): first attack, pose snapshots and current Safari behavior.
- [Historical 0.2.0 battlefield browser checks](evidence/browser-qa-0.2.json): Chrome rendering, native snapshot import and persistence; Safari limitation.
- [0.3.0 wounded dialogue rendering](evidence/linux-wounded-dialogue-0.3.png): inspected native SDL output.
- [0.11 parity results](evidence/parity-0.11.json): every checkpoint for both story branches through the reinforcement walks.
- [Linux first dialogue](evidence/linux-first-dialogue.png): inspected SDL output.
- [Linux first battlefield dialogue](evidence/linux-first-battle-dialogue.png): current initialized roster on the original map.
- [0.3.0 offline bundle relocation](evidence/relocation-0.3.json): first-attack rendering and pose restoration from a Chinese/space path without the development directory.
- [Historical 0.2.0 offline bundle relocation](evidence/relocation-0.2.json) and [rendered image](evidence/linux-battle-relocated.png): isolated run with Chinese and space characters in its path.
- [Source preservation](evidence/source-preservation.json): original source hashes.
- `tests/fixtures/original_units.json`, `original_rng.json`, and
  `original_physical_arithmetic.json`, `original_initialization.json`,
  `original_deployment.json`, `original_battle_initialization.json`,
  `original_animations.json`, `original_presentation.json`,
  `original_battle_actions.json`, `original_battle_entry.json`: hash-gated
  original executable probes. Reproduction commands are in README.md.

The automated checks establish the stated subset. Passing them does not satisfy
the unchecked battle acceptance criteria above.
