> **网页版本，2026-10-09:** The four-battle prototype is live at [3kingdoms.cc.cd](https://3kingdoms.cc.cd), with Google login, cloud saves, leaderboards, original music and server-side JEV. [Deployment and verification](WEB-GAME-DEPLOYMENT.md). This does not complete the original full-fidelity milestone.

> **宝物与装备，2026-10-08:** Version0.10.0 adds owned treasures, equipment changes **only between battles**, schema10 inventory/campaign persistence, and the original five-page 武将情报 layout. Press G for 宝物, V for 武将情报. [Controls, compatibility and tests](PROTOTYPE-EQUIPMENT.md).

> **JEV startup credential, 2026-10-08:** Version 0.9.3 loads the external JEV key once at startup and reuses it from memory. Missing/invalid credentials use local AI; key changes require an app restart. Documents permission may still be requested at startup. Gameplay and save formats are unchanged. See [JEV behavior](PROTOTYPE-BALANCE-JEV.md).

> **Diagnostics, 2026-10-08:** Version 0.9.2 adds rotating local logs, startup/runtime error context, action/save/load/AI events and two separate recovery checkpoints at stable interaction points. Manual saves and gameplay rules are unchanged. See [log locations, recovery and tests](PROTOTYPE-DIAGNOSTICS.md).

> **Reward crash fix, 2026-10-08:** Version 0.9.1 fixes a missing-image error when displaying treasures such as 太平清领道 and 布手套. Future fatal errors include their exact message and a best-effort state snapshot in the diagnostic log. Existing save formats and rules are unchanged. See [reproduction and verification](REA-GAMEPLAY-FIXES.md).

> **Gameplay repairs, 2026-10-08:** Version 0.9.0 adds multi-strike combat/XP, 15 tactics, allied turns, original scenario conditions/events, rewards and prototype campaign carryover. F6 opens ten save slots and backup controls. See [current behavior, evidence and remaining limits](REA-GAMEPLAY-FIXES.md); older entries below describe historical releases.

> **Unit details, 2026-10-07:** Prototype 0.8.0 adds 能力 / 装备 / 策略, original item icons, equipment levels/experience, and learned-spell visibility. Control-click a unit or select it and use 武将 V. Inspection preserves pending moves and pauses enemy execution; save schemas and gameplay rules are unchanged. See [controls, evidence and limits](PROTOTYPE-UNIT-DETAILS.md).

# 三国志曹操传 — four-battle prototype 0.7.0

Updated 2026-10-07 (America/Los_Angeles). This is the separate, simplified
opening-battle mode requested by the user. It does **not** complete the earlier
original-fidelity milestone. The faithful `Engine`, evidence and older releases
remain available unchanged alongside it.

Press **选择战场 / B** to choose 颍川之战, 汜水关之战, 虎牢关之战, or
董卓追击战. Added battles are independent simplified scenarios; each has its
own save slot. See [additional battles, balance and evidence](PROTOTYPE-ADDITIONAL-BATTLES.md).
The opening-specific mechanics and historical validation below remain useful;
they do not describe all added-battle objectives or rosters.

New games now restore scripted/default equipment and scale enemies from the
party average plus the original scenario/unit adjustments. JEV enemy decisions
are available in the native app, on by default when your external credential
exists; press **J** to toggle. See [0.4.0 balance, JEV, saves and verification](PROTOTYPE-BALANCE-JEV.md).
Loading balanced saves preserves progress and enables basic tactics and terrain benefits;
earlier unbalanced saves retain their old rules. **B/R** starts corrected battles.

Version 0.4.1 adds an original-style unit information popup. Rest the pointer on
any living unit for 220 ms to see class/level, HP/MP, experience or army side, and
terrain effect. See [hover panel evidence and limits](PROTOTYPE-HOVER-PANEL.md).

Version 0.4.3 fixes the five-row unit action window from the supplied screenshot.
It opens only after the selected unit finishes moving, including consecutive units.
Clicking a unit again or pressing Space before moving no longer opens the menu.
Attack, Wait and Cancel work. Version 0.5.0 enables five learned offensive tactics; Items remain disabled.
See [action menu behavior and evidence](PROTOTYPE-ACTION-MENU.md).

Version 0.5.0 adds 灼热、浊流、旋风、落石、毒烟 for eligible classes/levels,
including enemy/JEV decisions, MP costs, poison and visible spell effects.
Select **策略** after moving, or **T** for a selected unit. Balanced saves upgrade
on load without resetting progress. See [tactics and compatibility](PROTOTYPE-TACTICS.md).

Version 0.7.0 adds top **道具** and bottom **查看道具 / P** buttons, quantities, descriptions and use controls. Select 曹操, choose 恢复用豆, then click 曹操 to restore up to 30 HP. Item use ends the actor’s action. Existing collected items survive loading. See [consumable rules, save compatibility and tests](PROTOTYPE-ITEMS.md).

Version 0.6.0 adds terrain HP recovery at each army’s phase start and eight original-script location pickups across the four battles. Press **道具 / P** to view collected items. Inventory and one-time claims survive saves; moving and cancelling cannot collect loot. Version 0.7.0 enables use of the three collected consumables. See [terrain rules, pickup locations and verification](PROTOTYPE-TERRAIN.md).

## Play

On this Apple Silicon Mac, open `dist/prototype-macos/颍川之战.app` for the
native C++/SDL game. It includes its assets and uses only macOS system libraries;
Wine and Electron are not involved. This is a locally signed personal-use build,
not a notarized public release. The app fits its initial window to the display
and renders at Retina resolution. Native saves use the path documented below.

A plain Apple Silicon C++ executable is also available at
`dist/prototype-macos-executable/caocao-prototype`. Double-click
`dist/prototype-macos-executable/Start Game.command`, or invoke the binary's
absolute path from Terminal. Keep its adjacent `assets` folder when moving it.
It discovers assets relative to the executable and shares the native app's save
location. It needs no Wine, Electron, browser or development tools to run.

On this Mac, use Chrome and open <http://127.0.0.1:8177/prototype.html> while the
local server is running. To start it again:

```sh
cd /Users/henryzhang/Documents/workspace/caocao-engine
./dist/prototype-web/play.sh
```

The server serves static files only. Gameplay runs locally in WebAssembly;
there is no streaming server, Wine, Windows helper, login or hosting bill.
The browser data bundle is approximately45 MB, plus the JavaScript/Wasm engine. Internet access is not
needed to play once the package and local server are available.

On Linux x86-64, run `dist/prototype-linux/play.sh` in a desktop session.
Inside either extracted package, run `./play.sh` from that package directory.
It resolves its own assets directory, including when its path contains spaces.
The executable uses SDL3 linked statically and the standard Linux C/C++ runtime;
the tested environment is Debian 12 in the existing local container.

### Controls

- Click a friendly unit to select it. Blue tiles show its movement range.
- Click an empty reachable tile to move. Right-click or **取消 / Esc** restores
  that uncommitted movement and clears the selection.
- After movement finishes, choose an action from the unit's popup. Selecting a
  unit, repeated clicks or **Space** before moving do not open it. **攻击** enters target
  selection and is disabled when no enemy is currently in range. **待命** commits
  the action; **取消** rolls back the move. **策略** opens learned supported spells; **道具** opens the consumable picker (beans, rice and antidote).
- **Up/Down/Tab** choose an enabled menu entry; **Return/Space** activates it.
  During attack targeting, **Esc/right-click** returns to the menu; cancelling
  there restores movement. Direct map clicks and the existing toolbar also work.
- Click an enemy unit in attack range to attack. A red marker identifies a target.
- **待命 / W** commits the selected unit's movement without attacking.
- **J** switches native JEV/local AI; **I** shows its status.
- **结束回合 / E** commits remaining movement and starts the enemy phase.
- Original Cao Cao/Zhang Bao and Cao Cao/Zhang Liang encounters appear after
  committing an action while adjacent (including diagonally). **Space / Return**
  or clicking the dialogue box advances the page. Combat pauses during dialogue.
- **保存 / F5**, **读取 / F9** use the current battle’s independent manual save slot. The browser also
  has ordinary HTML save/load buttons below the canvas.
- **重新开始 / R** starts a new battle without deleting the saved slot.
- **情报 / I** shows objectives, selected-unit details and controls. Close with
  **Esc**, right-click, or its close button. Gameplay input is blocked while open.
- Hover a unit for its floating class/level, HP/MP and terrain panel. The bottom
  strip also shows portrait, attack and defense. Moving off the unit dismisses
  the popup; it does not intercept clicks or pause turns. Hover terrain to see
  its name. **H** toggles all HP bars;
  selected/hovered units show them automatically. The original white tile cursor
  follows the mouse. Completed units are dimmed.

The presentation uses the October 5 Electron/original-game screenshot as
a visual reference: a full-width 20×20 battlefield, native 48×48 sprite frames,
gray beveled toolbar with original EXE icons, blue title/turn ribbons and a
compact portrait/status strip. All seven toolbar buttons work; unavailable
actions are disabled. See [layout matching and evidence](PROTOTYPE-LAYOUT-MATCH.md).

Selecting an enemy shows its statistics when no friendly unit is selected.
An attack or waiting ends that unit's action. End the turn explicitly after
acting with your units. Units can move once before attacking or waiting.

During an attack the gold outline marks the attacker and the red outline marks
the target; the phase ribbon and bottom strip name the action. In the initial
turn2 enemy phase, all enemies are confused and skip their attacks. The bottom
strip shows **混乱，停止行动**; ordinary enemy attacks begin after that phase.
Native 0.2.3 also records local playback diagnostics under
`~/.local/share/caocao-prototype/diagnostics` to investigate missing-animation
reports. These logs do not replace or modify your save slot.

## Deliberately simplified rules

- Start on the original 20×20 opening battlefield, after the introductory story/tutorial.
- Eight controllable friendly units: Cao Cao, Liu Bei, Guan Yu, Zhang Fei, two
  cavalry and two infantry. Fourteen enemies: Zhang Bao, Zhang Liang and twelve
  Yellow Turban units.
  All friendlies are player controlled; no separate allied AI phase is needed.
- Opening party preset level3; ordinary NPCs level1, named opening enemies level5, Liu Bei/Guan Yu/Zhang Fei level7, following the scripts. Statistics do not gain XP during the battle.
  Original identity, class, terrain costs, movement/range calculations, sprites
  and map pixels are reused. Positions, facing, allegiance colors and roster
  reproduce the supplied slot-1 screenshot (turn 2). The fixed statistics and
  full-health start are prototype balance choices, not imported original HP.
- Single physical attacks use recovered damage, hit checks and RNG. Misses remain zero; new games have no artificial12–45 clamp. Historical saves keep their old clamp. Equipment statistics and supported movement/range effects apply, but counters, critical/double strikes, treasure extra attacks, XP, advanced/support tactics and additional items remain incomplete. The five basic offensive spells are available.
- The 27 camp fires use all four original animation frames and terrain 26;
  movement/AI cannot enter them. Fires remain for the whole prototype battle.
  All fourteen enemies begin confused with the original question-mark artwork.
  Simplified recovery skips one enemy action then clears confusion; its status
  is saved. This is not the original probabilistic recovery/terrain lifecycle.
- Enemy turns use deterministic terrain-distance planning, legal movement
  ranges and stable tie-breaking. Favor a reachable low-HP target, otherwise
  approach the nearest opponent. This is a simplified AI, not the original AI.
- Victory: all enemies defeated. Defeat: Cao Cao defeated or enemy turn 20
  completes with enemies remaining. No post-battle campaign continuation. The outcome screen offers restart/load.
- Original proximity exchanges, named retreat lines and victory/defeat speeches
  appear at stable action boundaries. [Dialogue scope](PROTOTYPE-DIALOGUE.md)
  documents the original offsets and adapted victory timing.
- Tile-by-tile walking uses original directional infantry/cavalry frames. Enemy
  walking completes before attack playback; input and saves wait until playback
  finishes. See [walking animation evidence](PROTOTYPE-MOVEMENT-ANIMATION.md).
  Physical attacks play the original wind-up, swing, hit
  flash and recoil frames with a damage number. Swing/hit WAVs use SDL audio.
  The 960×1120 logical viewport scales proportionally with letterboxing. Native
  initial size fits the display; minimum size is 720×840 or that initial size
  when the display is smaller. macOS uses a Retina backing surface to preserve
  thin Chinese glyph strokes. No claim of exact original timing/layout.
  Attack playback uses 40ms per recovered tick, a prototype pacing choice.
  Slow rendering stretches playback instead of skipping swing/flash frames.
  Exact original timing, numeric glyph layout and campaign music are deferred.

## Saves

The portable snapshot contains every live simulation field, including units,
fixed records, HP/MP, positions/directions, action flags, selected/inspected unit,
tentative movement rollback, phase, turn, enemy cursor, confusion/poison status,
pending enemy decision, turn-limit outcome, message and RNG state.
Triggered/pending conversation IDs and the current page are also saved.
The versioned layout determines the immutable fire grid.
Commands and enemy steps are atomic; snapshots are taken between them, including
an uncommitted player move or between two enemy actions. Saving and gameplay
commands are blocked while an attack is being presented; no partial animation
is written into a snapshot. The I information panel pauses playback.

- Native: `$XDG_DATA_HOME/caocao-prototype/slot-1.json`, or
  `~/.local/share/caocao-prototype/slot-1.json` when XDG_DATA_HOME is absent.
  `CAOCAO_PROTOTYPE_SAVE_DIR` overrides this directory for isolated tests.
- Browser: IndexedDB `caocao-opening-prototype`, store `snapshots`, key `slot-1`.
  Storage belongs to the exact browser profile and origin. Use the same host
  and port when returning; clearing site data removes local saves.
- New games use schema `6`, engine `prototype-0.6`, ruleset `basic-tactics-v1`,
  with current MP, poison, recorded pending enemy choices and the separate
  tactics asset identity. Balanced schema-5 saves upgrade on native load without
  resetting progress; the stored slot remains unchanged until the next Save.
  An old pending physical-only decision is cleared and replanned.
  The historical compatibility path remains:
  schema `3`, engine `prototype-0.3`, ruleset `prototype-0.2`,
  layout `electron-slot1-v1`. Existing schema2 saves still load unchanged in
  gameplay; a subsequent Save writes schema3, unreadable by older app versions.
  Schema `1` / `prototype-0.1` saves still load using their old fourteen-unit
  roster, terrain and rules, preserving their next attack and RNG. Restart uses
  the updated layout. Mode `opening-prototype`, original asset
  manifest hash and a SHA-256 payload checksum identify the snapshot.
  Restore validates compatibility, fixed records, numeric ranges, occupancy,
  outcome, pending selection and dialogue conditions/page before changing live state.
  Older saves have no conversation history: completed deaths/outcomes are
  suppressed and future proximity encounters remain eligible.
- Native saves write a temporary file then rename; browser writes use one
  IndexedDB transaction. Reading never overwrites the stored slot.
- The exact supplied `Sv00s.e5s` was read to establish layout evidence; no
  general legacy importer exists. Original `.e5s` files, faithful-engine
  snapshots and Electron saves are never overwritten. Legacy save import and backup UI are deferred.

## Verification and evidence

Current 0.2.5 dialogue behavior and verification are recorded in
[PROTOTYPE-DIALOGUE.md](PROTOTYPE-DIALOGUE.md). The production-mode full trace
includes379 action/page checkpoints. Earlier 0.2.4 walking verification is in
[PROTOTYPE-MOVEMENT-ANIMATION.md](PROTOTYPE-MOVEMENT-ANIMATION.md).
Earlier attack verification is recorded in
[PROTOTYPE-ATTACK-ANIMATION.md](PROTOTYPE-ATTACK-ANIMATION.md); layout evidence
is in [PROTOTYPE-LAYOUT-MATCH.md](PROTOTYPE-LAYOUT-MATCH.md).
Native automated commands reach victory on turn 8 and passive defeat on turn 13.
All 365 command/step checkpoints restore identically. A separate turn-20 fixture
checks timeout defeat; restored confusion never causes a repeated skipped action.
The exact original slot fixture checks all 22 unit IDs/coordinates/facings/sides,
27 fire tiles, and impassable movement ranges for every friendly unit.

Schema-1 compatibility uses an immutable historical snapshot and expected next
attack; a failed load across rulesets leaves live state and terrain unchanged.
The render smoke checks scaled input, movement/cancellation, toolbar actions,
modal blocking, saves and restoration in a fresh process. Current resources:
211 assets, including the original opening script, 27 raw attack/reaction/idle sprite records, nine WAVs and
a 16×16 confusion marker extracted losslessly from hash-checked `Mark.e5` bytes.
Original asset provenance is preserved.
All displayed Chinese strings are checked against the bundled glyph atlas.

Historical browser full-battle/storage checks in `prototype-chrome.json` and
`prototype-packaged-chrome.json` apply to version 0.1. Current browser verification
is explicitly scoped in the layout report; historical browser passes are not
presented as a new complete acceptance run.

## Rebuild

From this project's root, the existing decoded assets can be reused directly.
For a new setup, install/start the local container environment and run:

```sh
./scripts/bootstrap-mac.sh
.venv/bin/python tools/import_assets.py /Users/henryzhang/Downloads/sgzccz \
  --font /Users/henryzhang/Documents/workspace/caocao-test/vendor/fonts/CaoCaoSong.ttf
./scripts/build-prototype.sh all
```

The importer reads the original folder only. The font's
`SourceHanSerif-LICENSE.txt` must remain beside its input TTF. Reimporting is not
necessary on this prepared workspace; keep existing generated assets to retain
the same manifest and save compatibility.

Pinned dependencies: C++20; Emscripten 6.0.10 at emsdk commit
`e566f7bdcc7735f44037911c24b87a58a3c93145`; SDL3 3.4.16 with its SHA-256 in
CMakeLists.txt; nlohmann/json 3.12.0 vendored. The Linux Dockerfile pins the
Debian base digest, CMake 3.25.1 and Ninja 1.11.1 package versions. The bootstrap
installs missing host CMake/Ninja and the Python dependencies. Initial dependency
setup can require internet access. A running Docker-compatible daemon is required.

For individual builds, use `./scripts/build-prototype.sh linux` or `web`.
The `all` command performs both, exchanges snapshots and packages the outputs.
The native Mac build on this prepared workspace is:

```sh
cmake -S . -B build-mac -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-mac --target caocao_prototype_app prototype_tests -j 8
./build-mac/prototype_tests assets/prototype
python3 scripts/package-prototype-mac.py
```

The Mac packager stages assets and creates the `.app` with a local ad-hoc
signature. It expects the existing pinned SDL source at `.cache/SDL3-3.4.16`.
It also creates the plain-executable package. Use
`python3 scripts/package-prototype-mac.py --executable-only` to refresh that
package without modifying an existing app bundle.
For native Linux without Docker, install the development dependencies listed in
`scripts/Dockerfile`, run `python3 scripts/stage-prototype-assets.py`, then:

```sh
cmake -S . -B build-native -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-native --target prototype_tests caocao_prototype_app -j 8
./build-native/prototype_tests assets/prototype
./build-native/caocao_prototype_app assets/prototype
```

`tests/prototype_browser_check.js` is a development-only Chrome harness, not
shipped in the playable package. It uses the exported command interface and
real IndexedDB functions. Its `prototype-test/` fixtures now use the production
conversation trace: copy
`docs/evidence/prototype-linux.snapshot.json.dialogue.commands.json` to
`commands.json`, `prototype-linux.snapshot.json.dialogue.json` to `snapshot.json`,
and its `.next.json` to `next.json`. Historical schema2 hashes cannot validate
dialogue-inclusive hashes. Actual mouse tests remain a separate check; this
harness was updated for dialogue but browser UI was not rerun in0.2.5.

## Scope and remaining limitations

This contains four independent simplified battles, not full campaign equivalence.
The pre-battle story, tutorial, general script alternatives, advanced/support
advanced/support tactics, additional items, XP, original rewards, later campaign scenarios, accounts,
cloud saves, leaderboards, comprehensive Safari validation and campaign music
remain deferred. It has one manual slot per battle and no autosave. Small viewports may
require scrolling. The Linux rendering check used Xvfb in the existing container,
not every Linux desktop/GPU configuration. Determinism checks cover the recorded
traces; they are not an exhaustive proof over all possible play sequences.

The reference is an existing turn-two save, not a fresh turn-one introduction.
Matching its visible battlefield does not establish original combat, HP, event,
reward, audio or campaign equivalence. The original window chrome, fonts, toolbar
commands and hover position still differ from the native prototype UI.

Cerberus static dossier `0x43eb1f` supplied the original confusion-marker resource
and positioning formula. JEV call060 reviewed save-compatibility/evidence scope;
its exact request, response and independent disposition are logged in
[JEV-DECISIONS.md](JEV-DECISIONS.md).
