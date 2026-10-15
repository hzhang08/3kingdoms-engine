# 3kingdoms-engine

Play the production web game (Google sign-in required):

- [Main game — 四战征程](https://3kingdoms.cc.cd/)
- [Special battle — 虎牢关之战 · Voxel Special](https://3kingdoms.cc.cd/special/hulao/)

Source-only snapshot of the native/WebAssembly game and web backend. Images,
game media, credentials and player data are excluded. Read
[SOURCE_ONLY.md](SOURCE_ONLY.md) for repository scope and local asset requirements.

# 颍川之战 — simplified playable prototype

The separate **simplified opening-battle mode** is now the active deliverable.
See [play instructions, build instructions and the verification report](docs/PROTOTYPE.md).
Browser and Linux packages are in `dist/prototype-web` and `dist/prototype-linux`.
Gameplay uses the same C++ engine locally on Linux and in WebAssembly.

The older faithful-reconstruction checkpoint below remains preserved. Its
unfinished original-fidelity checklist is separate from the simplified goal.

---

# 三国志曹操传 replacement engine — investigation checkpoint

**The requested opening-battle milestone is NOT complete.** This project currently
runs the opening story and both choices in C++20/SDL3, using decoded original
art and Chinese text, initializes all 26 opening units, and plays the full
section-0 battle introduction and the first-turn infantry event. The first allied archer
selects, walks from (12,17) to (9,17), and completes its first critical attack
with Chinese speech, original sprite effects and an HP panel (90 → 45). The two
allied infantry follow, reducing the enemy to 14 and then 0 HP. Post-action
events, defeat blinking, removal and target cleanup execute. The enemy-phase
banner resets action flags, then Liu Bei, Guan Yu and Zhang Fei appear and
speak, and walk to (11,5), (10,5), and (12,5). Enemy maintenance completes;
the first enemy selects 毒烟 against the allied archer and moves to (9,13).
It now shows the tactic target, predicted damage and hit chance, then computes
the original poison plan, plays the caster pose and displays the original tactic
caption, all 13 poison-effect frames, the target reaction and damage digits.
The first cast applies damage (10 → 0 HP), spends 8 MP (48 → 40), and marks
the caster as acted. The archer speaks his final line and retires; Zhang Liang
then moves and casts poison at an allied infantryman. Its 13-frame status
effect applies poison without HP damage and spends 8 MP. Five further enemy
attacks and six movement-only actions complete with original state/RNG results.
Turn two now begins with the original environment update, banner and flag reset.
Liu Bei, Guan Yu and Zhang Fei discuss the fire attack, with the camera focused
on each speaker. Zhang Fei places the first fire; Guan Yu moves, speaks and
places two more fires. Original terrain changes and animated fire pixels persist
in snapshots. The 30-frame MCALL05 summon now plays, with its original image/mask
planes and palette compositor. Xu Zijiang speaks while off the battlefield; all
27 fires are placed, followed by the enemy panic animations and dialogue.
The original area-confusion effect applies to 13 enemies, then the six AI
instructions and Cao Cao/cavalry arrival, dialogue and walks complete.
The objective screen, 张宝/张梁 target highlights and Xu Zijiang appearance now
complete. Tutorial entry, basic, ally/enemy and movement/action lessons now run,
including cursor/panels, guide poses, blue/green ranges, Cao Cao movement and
cancellation, and the original command-menu demonstration. The terrain lesson, summary and
farewell also complete. Xu Zijiang disappears with the original cropped frames
and terrain mask, Cao Cao turns, and the “开始作战” banner plays. The fresh-profile
path now completes turn-two recovery and displays the poisoned infantryman’s
HP changing from44 to33 before applying that loss. It stops at the unimplemented
player-input loop (`Ekd5.exe:0x44e1d3`, continuation `S_00.eex:0x2fa5`).
Player actions, complete combat/AI turns, victory, defeat, rewards,
and post-battle execution are not implemented. Do not treat this as a playable
replacement for the original game yet.

The new project is separate from `caocao-test`, the original game, and original
saves. Its shipped prototype contains no Wine, Windows executable, or helper.
Read [the compatibility report](docs/COMPATIBILITY.md) for the acceptance status.

The [Cerberus static-analysis workbench](docs/CERBERUS-RE.md) now provides pinned
Ghidra decompilation and function dossiers alongside original-code probes.

Use the [development tools catalog](docs/TOOLS-CATALOG.md) to find the right
decoder, original-code probe, state inspector, build command, or test utility.

## Current development workflow

Gameplay is developed and tested natively first. Run focused original-code and
snapshot tests after changing rules; compile Wasm and compare traces at gameplay
milestones. Browser UI checks are reserved for relevant integration changes and
final acceptance. The `dist/` archives are still the 0.22 checkpoint; current
source/native Linux, macOS and Wasm builds are 0.29. The shared player helpers additionally match502 movement cases,1106 menu/wait/cancel cases,174 hover transitions and102 post-action candidate scans on all three platforms. The original input loop has140 captured sequences; interactive Engine/frontend/snapshot integration is still pending. See the compatibility report for exact
validation scope. No full-battle completion is claimed.

New focused native checks (after building):

```sh
ctest --test-dir build-linux -R '^(tutorial_exit|tutorial|objective|highlight|core)$' --output-on-failure
SDL_AUDIODRIVER=dummy XDG_RUNTIME_DIR=/tmp xvfb-run -a build-linux/caocao assets/generated --render-objective-smoke
SDL_AUDIODRIVER=dummy XDG_RUNTIME_DIR=/tmp xvfb-run -a build-linux/caocao assets/generated --render-highlight-smoke
SDL_AUDIODRIVER=dummy XDG_RUNTIME_DIR=/tmp xvfb-run -a build-linux/caocao assets/generated --render-tutorial-panel-smoke
SDL_AUDIODRIVER=dummy XDG_RUNTIME_DIR=/tmp xvfb-run -a build-linux/caocao assets/generated --render-tutorial-range-smoke
SDL_AUDIODRIVER=dummy XDG_RUNTIME_DIR=/tmp xvfb-run -a build-linux/caocao assets/generated --render-tutorial-blue-smoke
SDL_AUDIODRIVER=dummy XDG_RUNTIME_DIR=/tmp xvfb-run -a build-linux/caocao assets/generated --render-tutorial-menu-smoke
```

## Run the prepared browser build

```sh
cd /Users/henryzhang/Documents/workspace/caocao-engine
./scripts/serve.sh
```

Open http://127.0.0.1:8765/caocao.html . Click the canvas to start, use Space
or click to advance, and 1/2 to choose. The ten browser slots use IndexedDB.
Save only becomes available at a stable dialogue/choice/tutorial-menu boundary without a
pending action; critical-hit dialogue cannot be saved mid-attack. Import/export uses
prototype JSON snapshots, never original `.e5s` files. HTTP is required; opening
the HTML with `file://` will not load its Wasm data package.

## Reproduce the import and web build

```sh
./scripts/bootstrap-mac.sh
.venv/bin/python tools/import_assets.py /Users/henryzhang/Downloads/sgzccz \
  --font /Users/henryzhang/Documents/workspace/caocao-test/vendor/fonts/CaoCaoSong.ttf
./scripts/build-web.sh
./scripts/serve.sh
```

The font's `SourceHanSerif-LICENSE.txt` must remain alongside the input font.
Bootstrap installs missing CMake/Ninja through Homebrew, sets up Python packages,
and installs pinned Emscripten 6.0.10 in `.cache/emsdk`; it does not modify shell
startup files. The web build script uses that Python environment, avoiding
Xcode’s older system Python. CMake obtains SDL 3.4.16 from its official archive and checks its
SHA-256. Header dependencies and licenses are included in `third_party`.

The importer only opens the game for reading. It verifies every source hash
again before publishing a staging directory. A publication rename failure
restores the last output; an interruption between renames is recovered on the
next import. Do not run concurrent imports. The previous output is retained as
`assets/generated.previous`. A versioned manifest identifies each source,
archive block, PE bitmap identifier, palette/tile transform, and decoded file.
Original resources are not included in the source archive; import your local copy.

## Linux x86-64

Start OrbStack (already started during this implementation), then:

```sh
./scripts/build-linux.sh
```

This uses a pinned Debian image, GCC 12, CMake 3.25.1, and SDL built from the
same pinned source. The test container uses Xvfb for a real SDL render test.
For an interactive Linux desktop, run:

```sh
./build-linux/caocao ./assets/generated
```

Linux needs a display/audio environment and the usual glibc/libstdc++ runtime.
SDL loads the host's display/audio libraries. See `docs/evidence/linux-packages.txt`
for the test environment. The prepared bundle in `dist/linux` has `run.sh`.
F5 saves; F9 loads; `[` and `]` select one of ten slots. Local files live in
`$XDG_DATA_HOME/caocao-engine` or `~/.local/share/caocao-engine`. Export a Linux
snapshot by copying its `slot-N.json`; import a validated browser export by
copying it into a prototype slot (keep your prior file as a backup).

Validate a snapshot without changing any slot:

```sh
./build-linux/caocao_trace ./assets/generated --restore backup.json
```

## Tests and evidence

```sh
.venv/bin/python -m unittest discover -s tests -p 'test_*.py' -v
cmake -S . -B build-core -G Ninja -DCAOCAO_RENDERER=OFF
cmake --build build-core
ctest --test-dir build-core --output-on-failure
.venv/bin/python tools/executable_probe.py /Users/henryzhang/Downloads/sgzccz/Ekd5.exe
.venv/bin/python tools/unit_probe.py /Users/henryzhang/Downloads/sgzccz
.venv/bin/python tools/combat_probe.py /Users/henryzhang/Downloads/sgzccz/Ekd5.exe
.venv/bin/python tools/initialization_probe.py
.venv/bin/python tools/deployment_probe.py
.venv/bin/python tools/battle_initialization_probe.py
.venv/bin/python tools/animation_probe.py
.venv/bin/python tools/presentation_probe.py
.venv/bin/python tools/battle_actions_probe.py
.venv/bin/python tools/battle_entry_probe.py
.venv/bin/python tools/battle_events_probe.py
.venv/bin/python tools/recovery_probe.py
.venv/bin/python tools/maintenance_probe.py
.venv/bin/python tools/ai_priority_probe.py
.venv/bin/python tools/player_phase_probe.py
.venv/bin/python tools/player_movement_probe.py
.venv/bin/python tools/player_action_probe.py
.venv/bin/python tools/player_input_probe.py
.venv/bin/python tools/phase_transition_probe.py
.venv/bin/python tools/movement_probe.py
.venv/bin/python tools/route_probe.py
.venv/bin/python tools/range_probe.py
.venv/bin/python tools/physical_resolution_probe.py
.venv/bin/python tools/ai_attack_probe.py
.venv/bin/python tools/ai_decision_probe.py
.venv/bin/python tools/tactics_probe.py
.venv/bin/python tools/movement_animation_probe.py
.venv/bin/python tools/ai_movement_probe.py
.venv/bin/python tools/selection_probe.py
.venv/bin/python tools/panel_probe.py
.venv/bin/python tools/overlay_probe.py
.venv/bin/python tools/palette_probe.py
.venv/bin/python tools/strike_probe.py
.venv/bin/python tools/counter_probe.py
.venv/bin/python tools/experience_probe.py
.venv/bin/python tools/strike_application_probe.py
.venv/bin/python tools/critical_speech_probe.py
.venv/bin/python tools/combat_palette_probe.py
.venv/bin/python tools/attack_animation_probe.py
.venv/bin/python tools/attack_panel_probe.py
.venv/bin/python tools/attack_camera_probe.py
.venv/bin/python tools/damage_panel_probe.py
.venv/bin/python tools/attack_controller_probe.py
.venv/bin/python tools/post_action_probe.py
.venv/bin/python -m unittest discover -s tests -p 'test_*.py' -v
.venv/bin/python tools/script_listing.py
.venv/bin/python tools/parity.py
```

The original-code probes use bounded CPU emulation of the hash-checked supplied
executable. Their permitted routines and explicit I/O/presentation hooks are
documented in [FORMATS.md](docs/FORMATS.md). They do not launch Windows or Wine.
The unit test target compares the replacement against those generated fixtures.
The `battle_actions_tests` target checks original removal/facing state and event
sequences, new-battle entry cleanup and direction selection.
The `animation_tests` target additionally compares 768 original animation
sequences and 1,920 indexed frame hashes. It is included in native CTest; run its
WebAssembly build with the pinned Node runtime:

```sh
source .cache/emsdk/emsdk_env.sh
cmake --build build-web
bash scripts/test-web-core.sh
```

The current verified sequence ends after turn two's area confusion and Cao
Cao/cavalry arrival, dialogue and walks. The dedicated objective banner at
`S_00.eex:0x2f51` remains an explicit stop. Read [COMPATIBILITY.md](docs/COMPATIBILITY.md)
for the exact original-code evidence and remaining acceptance gaps.

`parity.py` uses the image prepared by `build-linux.sh`, an isolated container
with networking disabled, and completed `build-linux` and `build-web` outputs.
It compares every presentation event and
state hash for both opening choices (1864 and 1865 checkpoints), including both
scripted removals, all pre-turn dialogue, the first infantry event, the
empty player phase, all three allied attacks, the first enemy retirement,
the enemy-turn banner, reinforcement dialogue, all three scripted walks, and the first enemy decision
movement, target preview, poison planning, caster pose, tactic caption, poison
effect, damage application, caster MP/action flags, archer death dialogue and
retirement, the next enemy decision/walk/poison/status effect, and all remaining
first-phase enemy actions, the turn-two banner/dialogue and the first three fire
placements, MCALL05 summon, all 27 fires, area confusion, six AI instructions,
and Cao Cao/cavalry arrival dialogue and walks. This is **opening-sequence parity
between replacement builds**, not a playable battle or complete equivalence
with the original executable.

Recreate the local source, Linux and web archives after both builds complete:

```sh
.venv/bin/python scripts/package-checkpoint.py
.venv/bin/python tools/check_relocation.py
```

`dist/SHA256SUMS.json` records each archive's checksum. The browser bundle can
be moved to another directory and served with its included `serve.sh`.
The bundles include original assets for personal local use and dependency
notices, but no development SDK, Wine runtime or Windows helper.

## Structure

- `tools/ls12.py`, `eex.py`, `import_assets.py`: strict, read-only import pipeline.
- `include/caocao/engine.hpp`, `src/engine.cpp`: portable state and script interface.
- `src/frontend.cpp`: SDL rendering/audio/input; no simulation rules in JavaScript.
- `web/shell.html`: IndexedDB transactions and snapshot backup controls.
- `tools/executable_probe.py`: bounded x86 arithmetic probes, without Windows/Wine.
- `docs/FORMATS.md`: observed fields and unresolved semantics.
- `docs/R_00-listing.txt`, `S_00-listing.txt`: complete offset-preserving listings.

Version 0.23.0-investigation (snapshot schema 3) rejects older prototype saves without overwriting
their slots. Battle-intro snapshots include deployment, complete initialized
records, live HP/MP, hidden units, equipment, counters, script state, retained
poses, camera position and looping-effect identity. Fire-event saves also include
all terrain-effect records, terrain changes and effect marks. Four pre-summon
dialogue saves retain the loaded MCALL resource; restoring them replays the
effect exactly once.

Movement grids/routes, attack ranges, AI preparation and physical hit/damage
getters now have original-executable comparisons on native and Wasm builds.
Single-target AI attack scoring is also compared; area scoring stops explicitly.
Strike preparation, critical/double chances, HP/MP/status application and counters
are connected through the physical controller. Automatic items, level-up windows
and area attacks still stop explicitly. Opening-enemy tactic scoring now preserves original decision-time RNG.
The tactic target preview and independent poison status/damage rolls are now verified.
Next required work is the objective banner, target highlights and tutorial,
Cao Cao's arrival, party retirement, tactic/area rules, player commands,
movement cancellation and continuous victory/defeat playthroughs.
The implementation must not turn decoded-but-unverified
instructions into no-ops to make a playthrough appear to pass.

The current enemy-entry reference and banner fixtures can be reproduced with:

```sh
.venv/bin/python tools/turn_banner_probe.py
.venv/bin/python tools/script_movement_probe.py
.venv/bin/python tools/allied_phase_probe.py --enemy-entry \
  --output tests/fixtures/original_enemy_entry.json
./build-core/caocao_trace assets/generated 1 --reinforcement-snapshot \
  --snapshot-out reinforcement.json
```

The last command writes a new prototype snapshot at Liu Bei's first arrival
dialogue; choose an unused output filename to keep earlier backups.

Reproduce the opening-enemy tactic comparisons and first movement:

```sh
.venv/bin/python tools/tactic_scoring_probe.py
.venv/bin/python tools/enemy_phase_probe.py
```

Reproduce the tactic preview and pre-animation controller comparisons:

```sh
.venv/bin/python tools/tactic_panel_probe.py
.venv/bin/python tools/tactic_plan_probe.py
.venv/bin/python tools/tactic_cast_probe.py
.venv/bin/python tools/caption_window_probe.py
```

The turn transition references can be regenerated with `.venv/bin/python tools/environment_probe.py`
and `.venv/bin/python tools/next_turn_probe.py` from the project root. These execute bounded original
code and record all host boundaries; they do not constitute an interactive Windows playthrough.

Reproduce fire-placement and sequential event references:

```sh
.venv/bin/python tools/fire_probe.py
.venv/bin/python tools/fire_event_probe.py
./build-core/caocao_trace assets/generated --fire-snapshot \
  --snapshot-out fire-dialogue.json
```

The snapshot is at Guan Yu's “这里也点上！” dialogue after Zhang Fei's first
fire. The continuation now reaches the guarded tutorial opcode16 at 0x2f51. Choose an unused filename when
keeping backups. `--render-fire-smoke` and `--render-three-fires-smoke` on the
Linux frontend render the corresponding verified scenes.

Reproduce the summon and following fire sequence:

```sh
.venv/bin/python tools/mcall_probe.py
.venv/bin/python tools/mcall_blend_probe.py
.venv/bin/python tools/summon_event_probe.py
./build-core/caocao_trace assets/generated --summon-snapshot \
  --snapshot-out summon-dialogue.json
```

The snapshot pauses at Zhang Fei’s “火势太小了” dialogue with MCALL05 loaded.
The Linux frontend also supports `--render-summon-dialogue-smoke`,
`--render-summon-smoke`, `--render-summon-end-smoke` and
`--render-all-fires-smoke`. These check rendering at selected points;
they are not a full battle or Windows screenshot comparison.


## Jev-assisted investigation

[Jev decisions](docs/JEV-DECISIONS.md) records each delegated issue, exact model
response, our disposition and subsequent original-code verification. Jev is used
for prioritization and test-risk review; original executable evidence determines
game semantics. The narrow client `tools/jev_decide.py` reads its credential from
an external file at runtime and writes append-only, non-secret request records.
The game and packaged binaries do not use Jev, credentials or any online service.

New read-only reference probes:

```sh
.venv/bin/python tools/confusion_probe.py
.venv/bin/python tools/script_ai_probe.py
.venv/bin/python tools/status_event_probe.py
.venv/bin/python tools/arrival_event_probe.py
```

The sequential probes include the earlier turn and may take several minutes.
Save at the first stable post-confusion dialogue for focused continuation checks:

```sh
build-core/caocao_trace assets/generated 1 --confusion-snapshot --snapshot-out .cache/confusion.json
```

Use a fresh output filename to preserve previous snapshots.
