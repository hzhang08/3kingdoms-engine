# Cerberus RE workbench — 2026-10-04

The installed [Cerberus skill](/Users/henryzhang/.codex/skills/cerberus-re/SKILL.md)
now has a working, static-only Ghidra environment. It imported this edition of
`Ekd5.exe` and produced fifteen function dossiers. Decompiled C is an investigation
aid, not recovered original source or proof of runtime behavior. The original
x86 probes and native/Wasm comparisons remain the behavior checks.

## Setup and use

Run from `/Users/henryzhang/Documents/workspace/caocao-engine` with the local
Docker-compatible daemon running:

```sh
scripts/cerberus/setup.sh
scripts/cerberus/run.sh doctor
scripts/cerberus/run.sh import analyze /input/Ekd5.exe caocao
scripts/cerberus/run.sh export function-dossier caocao Ekd5.exe --address 0x4419b6 --linear-instruction-limit 1200
```

Setup downloads dependencies and builds `caocao-cerberus:12.0.4`. The analysis
wrapper then runs without networking, as a non-root user, with capabilities
dropped, no-new-privileges, a 256-process limit and a 6 GB memory limit. Its only
host mounts are the exact executable, read-only, and
`output/reverse-engineering/cerberus`, writable. It mounts no home directory,
credentials, saves or Docker socket. No Windows process executes in this workflow.
Frida injection, LLDB attachment, live bridge, shared-note synchronization,
publishing and companion installation are outside this local profile.

Both setup and execution explicitly set the skill's seven environment switches:
shared notes and auto-sync off, no notes repository, unsafe MCP execution and
bridge writes off, and no runtime names/PIDs. The wrapper permits only `doctor`,
`import analyze`, and `export function-dossier`. The executable is SHA-gated:

`199b1b67cf966ead46c49b430b9aab84bdc7d4f0e8dfa8e0ebeec5da280b01e0`

Pinned inputs:

| Component | Pin |
| --- | --- |
| Cerberus source | `3116c94de17386d18367ed14e55dd2c7d1c7fe14` (CLI 0.2.0) |
| Ghidra | 12.0.4, `ghidra_12.0.4_PUBLIC_20260303.zip` |
| Ghidra ZIP SHA-256 | `c3b458661d69e26e203d739c0c82d143cc8a4a29d9e571f099c2cf4bda62a120` |
| Java base | `eclipse-temurin:21-jdk-noble@sha256:b468c3fc688b14450571494f588bd939378e7fd542ed5a73f8efc13f17872a87` |
| Python packages | [requirements.lock.txt](../scripts/cerberus/requirements.lock.txt) |

The tested container uses Java 21.0.12.1 and Python 3.12. Base/image inputs and
Python versions are recorded; Ubuntu apt package availability is not frozen to
a repository snapshot, so a future rebuild is not promised to be bit-identical.
The original skill installation provenance remains in its own directory.

## Recovered evidence

Dossiers live under
`output/reverse-engineering/cerberus/exports/caocao/Ekd5.exe/dossiers/`.
Each contains `decompile.c`, `linear_instructions.txt`, `context.json` and
`summary.md`. The project database persists under the same workspace's `projects/`.

| Address / dossier | Use and verified scope |
| --- | --- |
| `0x41311c` | Opcode49 selector/removal wrapper; exact tutorial single-unit arguments execute in the sequential probe. Other selector forms remain unsupported. |
| `0x45a7b7` | Script wrapper around retirement mode1; sets/clears flag128 around the original call. |
| `0x441744` | Retirement and cleanup. Mode1 preserves persistent records/RNG in the captured cases; it is distinct from mode0 defeat behavior. |
| `0x4419b6` | Mode1 animation: moving frame6, sound6, four-tick pause; special frame0 cropped to heights48,44,…,4, sound17 and one-tick pauses. Mode2 remains unresolved. |
| `0x45137c` | Terrain masking before the lower raster call. The six captured static terrains cover offsets0x27c0,0x30c0,0x39c0,0x42c0 and no mask; animated terrain13 remains guarded. |
| `0x416c97` | Opcode90 battle-start banner entry. |
| `0x44bac2` | Outside-in checker columns, three text/shadow requests,20-tick pause, row cleanup, full redraw. Eight supplied viewport/client layouts execute in the original probe. |
| `0x44e0d9` | **Incomplete:** player-phase gate decompilation stops at the ASave patch transfer; it does not describe the interactive loop. |
| `0x44df61` | Poison phase maintenance, numeric presentation before HP mutation. Fresh turn-two NPC path now executes and matches the portable engine. |
| `0x450984` | NPC HP/MP panel:272 original cases, including absent-opponent255; sequential44→33HP display verified. Party result/XP branch remains unsupported. |
| 0x43d538 | Player movement controller:502 original cases and portable C++ transitions match on macOS/Linux/Wasm. Tile-picker responses supplied; full interactive caller not verified. |
| 0x43dde3 |1106 original menu availability/wait/cancel cases match native/Linux/Wasm. Attack/tactic/item callbacks remain guarded. |
| 0x43e273 | Original manual-unit attack-menu predicate; full range grids and ordered targets compared. Alternate options not covered. |
| 0x43dada | **Static only:** player physical attack callback; target setup, possible second attack and party results need execution checks. |
| 0x43dc21 | **Static only:** player tactic selection/execution callback; full player cast path needs verification. |
| 0x44201b | Original per-tile walking; static dossier refreshed for prototype0.2.4. Existing1,749 x86 frame/wait/scroll/overlay cases still pass. Prototype adapts coordinates to a full-map view; exact window timing is not claimed. |

`tools/disappearance_probe.py` independently executes208 original cases and
compares2678 frames at both source-pixel and post-terrain-mask boundaries.
`tools/start_banner_probe.py` records eight original banner sequences.
`tools/tutorial_exit_probe.py` replays the full fresh-profile tutorial, then the
four exact instructions at0x2f53/0x2f79/0x2f95/0x2f97. It checks expected opcodes,
explicit hook allowlists, original state, event order and variable13 completion.
The new `tutorial_exit_tests` compares these fixtures with the portable engine,
including deterministic save continuation and transient-save rejection.

Host window/text rasterization, actual audio output and wall-clock waits remain
explicit boundaries. Pixel comparisons do not establish full Windows window or
font equivalence. The native render captures show the implemented composition:
[guide exit](evidence/cerberus/render-guide-exit-0.28.png) and
[start banner](evidence/cerberus/render-start-banner-0.28.png).

## Decompiler limitation and next work

At `0x44e1cd`, bytes `ff 25 14 d0 4c 00` jump through the added ASave import slot.
Ghidra reports an unrecovered indirect transfer and emits `ASave(); return;`.
The existing271 original gate cases execute the supplied AutoDLL branch with
autosave disabled and continue at `0x44e1d3`. The apparent return is not proof
that the phase ends there. Preserve the raw dossier; any future Ghidra flow
override must be a separately labeled analysis view, checked against original
execution. No override or modification of the supplied executable was made.

The0.29 engine completes the fresh turn-two maintenance sequence and stops at
actual player input (`0x44e1d3`). Original recovery consumes RNG340946258→601077795;
poison displays ally210’s44→33HP before the HP setter and consumes no further RNG.
The no-opponent255 argument is represented by an optional opponent, not a
fabricated unit. Six panel pauses match original requests in both opening-story
routes, and earlier saves replay the same result. Original glyph/window/audio
host boundaries still limit visual-identity claims.

`tools/phase_maintenance_probe.py` adds sequential evidence after the tutorial.
The inherited headless client origin was0,0; a56-pixel comparison discrepancy
exposed this missing host initialization. The fixture explicitly supplies0,56
like isolated panel tests, records both origins and keeps the world camera
unchanged. This is a declared input, not a recovered original Windows window
size. Dictionary1.0.1 distinguishes those coordinates and records HP/RNG order.

JEV calls051–054 and their dispositions appear in
[JEV-DECISIONS.md](JEV-DECISIONS.md).051 abstained;052 identified a partial player
loop;053 reviewed poison ordering/sentinel/RNG;054 reviewed the host-origin
mismatch. Original execution independently establishes accepted semantics.

Next: recover actual player commands after the ASave patch, then complete
battle outcomes/rewards and acceptance. Collection-dependent tutorial rewards,
party HP/XP panels and other recovery/regeneration presentations remain guarded.
See [compatibility](COMPATIBILITY.md).

## Verification issues retained

- Initial Ghidra import warned about hostname lookup in its network-disabled
  container; analysis completed. The wrapper now supplies a local hostname map.
- Early probes stopped on inherited observer hooks, an unmapped host repaint
  boundary and overlapping Mark scratch memory. Explicit hook inventories and
  collision-free scratch allocation resolved these; final runs passed.
- A direct Wasm rebuild selected Xcode Python3.9 and failed before compilation.
  The supported `scripts/build-web.sh` selects the prepared Python environment.
- The map renderer originally required every later label to have a historical
  JEV review. It now reports entries without one explicitly; byte/range/resource
  checks remain mandatory. Opcode90 has original probe evidence, not an
  entry-specific historical model review.

Build/import/dossier logs are retained under the Cerberus workspace; final
doctor and render evidence are under `docs/evidence/cerberus/`.

The0.29 sequential probe also stopped at an omitted neighbor-scan range and an
unlisted temporary-buffer release hook. The existing recovery probe supplied
the verified ranges/boundary; final allowlist checks pass. The subsequent
host-origin mismatch is recorded above rather than hidden by changing assertions.

## Historical player movement investigation after0.29

The original continuation assembly at44e1d3 shows hover selection, unit-action
dispatch and post-action event/death checks. Its semantics have not yet been
verified by executing the outer loop. New Cerberus43dde3 and43d538 dossiers
identify the player movement dependency; no database flow override or original
EXE modification was necessary. Decompiler types and local names remain inferred.

The new player_movement_probe.py executes502 original cases: eligibility with
AI/flag/confusion variations, cancellation and own-tile choices, occupied and
unreachable destinations, and every map destination for CaoCao explicitly
activated at(10,6) on the post-introduction board. That activation and supplied
camera(4,2) are recorded inputs, not a continuous turn-two trace.
The picker45418f supplies tile/cancel responses; the original controller, search,
route, walking, occupancy and moved-flag instructions run. Walking pixels are
checked against original frames; persistent records and RNG remain unchanged.

The new native begin/pick interface matches all502 final states, intermediate
picker distance grids, and ordered presentation requests on macOS/Linux/Wasm.
It preserves retry after invalid selection and distinguishes movement(flag4)
from action completion(flag2). It is not wired into the interactive engine yet:
the outer player loop and command-menu handoff/rollback are next.
No browser, audio, snapshot or complete-battle acceptance is claimed for this helper.

Investigation repairs: an inherited43fcc9 redraw hook was explicitly captured
and added to the allowlist. The draft observer confused Ghidra local_8 with
machine EBP-8; assembly identifies the mode atEBP-4. Captured narrow range
arguments now use their declared16/8-bit widths instead of irrelevant register
high bits. Early logs remain under docs/evidence/player-movement-investigation*.
The raw helper assembly export includes linear data/table decodes and truncated
regions; it is context, not a recovered control-flow graph.

JEV055 abstained on retained-hover semantics; its action-result interpretation
remains static only. JEV056 movement recommendations have independent original
execution and portable comparison evidence. See the JEV decision log.


## Player menu and outer loop — current evidence

player_action_probe.py executes1106 selected-unit controller cases:160 move/menu
orientation combinations,848 class/level/MP/silence combinations,34 independent
stock mutations and64 equipment/neighbor combinations. Both final flags/state
and attack range scratch/target order match C++ on macOS/Linux/Wasm. Refactoring
its reusable configure_action/answer helpers preserved the entire fixture.

player_input_probe.py executes140 sequences through the unchanged44e1d3
continuation, real43dde3 and45b5fe/death/event scans. Supplied host inputs and
captured windows are declared; physical input handlers remain unverified.
C++ hover and remaining-unit helpers match174 transitions/102 scans. Complete
native outer-loop integration is still pending. Cancellation's action-event
batch and the differing initial/post-completion filters must be preserved.
Raw Ghidra44e0d9 still ends at the ASave transfer: no flow override was applied.

New dictionaries and map annotations link these observations. Attack43dada and
tactic43dc21 dossiers remain static-only; JEV059's suggestions are plans pending
original execution. Actual game input and snapshot integration are next, then
player combat/party-result composition. See the latest compatibility section.

Investigation repairs are retained in logs: action probe's inventory range
ended too early (40d9ee actually continues through40da44); an east destination
was unreachable and was replaced by an explicitly valid tile rather than
changing movement rules. The first outer-loop run stopped at42c4cb, whose
original wait wrapper is now allowed. Host command input usesCTRL+1 as proved
by44f9a0/40be20. An early native run read the older fixture while the new one was
still generating and failed on missing hover_ready; the final run occurred
after the original process completed and passed. No original rule was bypassed.

## 2026-10-05: simplified prototype status artwork

Exported static dossier `0x43eb1f` using the existing network-disabled wrapper.
The isolated status8 branch reads `Mark.e5+0x1a2c0`, draws 16×16 pixels at
`tileX + 4 + 24*(tick&1)` and tileY. Raw instruction offsets and the decompile
agree; the decoded original yellow question mark visually matches the screenshot.
This is the sixteenth retained dossier. Only this marker path is reused by the
simplified prototype. Combined statuses, poison recoloring and full original
raster composition are not claimed as newly verified. No live process attach,
injection, shared notes, binary patching or new external dependencies were used.

## 2026-10-05: simplified prototype physical attack playback

Exported the seventeenth static dossier, `0x4044e9`, using the same isolated
wrapper: [log](evidence/cerberus/prototype-attack-dossier.log). The 27-iteration
switch, frame resource calls and wait/sound ordering support reuse of the existing
physical-animation helper. All189 retained original-code cases and2,830 indexed
frame hashes pass again. This turn connects that helper to the simplified SDL
prototype; it does not change the original-probe fixture or patch game bytes.
No live attach, instrumentation, network access or new dependencies were needed.
Real-time pacing remains provisional. See [attack report](PROTOTYPE-ATTACK-ANIMATION.md).

## 2026-10-05: opening-battle encounter selection

Exported static dossier `0x4100a1` with the existing isolated, network-disabled
wrapper: [log](evidence/prototype-dialogue-cerberus.log),
[dossier](../output/reverse-engineering/cerberus/exports/caocao/Ekd5.exe/dossiers/0x4100a1/decompile.c).
Its loop scans every event and replaces the selected position each time all
conditions pass. This supports last-eligible-event priority, including Zhang
Liang before Zhang Bao if both adjacency conditions hold. Re-executing159 bounded
original-code condition/selection cases exactly reproduced the retained fixture;
the C++ comparison passes. No live attach, injection, shared notes or binary
modification was used. Speech extraction comes from hash-checked S_00.eex,
not a guess based on decompiler names. See [prototype dialogue](PROTOTYPE-DIALOGUE.md).
