# Native physical attack animation — prototype 0.2.3

Updated 2026-10-05 (America/Los_Angeles). Player and enemy physical attacks now
play the decoded original wind-up, weapon swing, hit flash and recoil, followed
by a damage number and the committed result. This is a presentation update to
the simplified battle. It does not change combat rules, AI, RNG, layout or saves.

![Actual SDL attack frames](evidence/prototype-attack-frames.png)

[Animated renderer capture](evidence/prototype-attack-animation.gif).
The GIF crops the actual SDL renderer output and enlarges it with nearest-neighbor
sampling; no replacement artwork or interpolated animation is used.

## 0.2.3 follow-up: visible enemy actions and session diagnostics

The user still reports missing enemy animations after 0.2.2, while player attacks
animate. That specific observation is **not yet reproduced or explained** by the
automated cases. The slow-frame correction below remains valid, but should not
be taken as proof that the user's issue is resolved.

A separate HUD bug was reproduced: leaving the mouse on **结束回合** replaced the
enemy action/status text with the toolbar hint. The hint no longer hides an
enemy action. During an attack the phase label reads **敌军攻击**, the status names
both participants, the attacker tile has a gold outline and the target tile has
a red outline. The bottom portrait follows the attacker. Original sprite pixels,
combat results and animation timing are unchanged.

The opening layout starts on turn2 with all14 enemies confused. Their first
enemy phase clears confusion and skips attacks; no hit is expected there. The
status now leaves the **混乱，停止行动** explanation visible even with the mouse
on End Turn. This does not explain a missing animation when HP actually falls.

The new `--mixed-enemy-animation-smoke OUTPUT_DIR` exercises the frontend's tile
clicks for a player move/attack, waits for playback, then uses the End Turn button
through the confused and ordinary enemy phases. It parks the mouse over End Turn
and checks the status text, every damage transition, all four enemy swing frames,
hit flashes, damage numbers and the final state against an independent headless
simulation of those same commands. The packaged `Start Game.command` was run
from `/tmp` with its adjacent assets and the default **Metal** renderer. The
player attack,14 confused skips and8 subsequent enemy attacks passed. Each enemy
attack showed four swing frames, flash and number over1.146–1.202 seconds. Linux
passes the same mixed sequence, with identical final hash
`68a10959f9048b0d3fc10b4caeb3c93a65326ac74c77a1ca2f8b8a261bf271c2`.

Evidence: [packaged macOS result](evidence/prototype-0.2.3-exact-launcher.json),
[actual session diagnostics](evidence/prototype-0.2.3-exact-session.jsonl),
[confusion message](evidence/prototype-0.2.3-enemy-confused.png),
[enemy impact](evidence/prototype-0.2.3-enemy-001-impact.png),
[final Linux rendering tests](evidence/prototype-0.2.3-final-linux.log).
macOS/Linux/Wasm retain the365-checkpoint trace and cross-platform save behavior;
the updated Wasm frontend compiles. No browser UI acceptance rerun is claimed.

Reviewing the screenshots also found a **test-capture bug**: reading pixels after
`SDL_RenderPresent` could return the preceding Metal backbuffer. Diagnostic
captures now read the completed frame before presenting it. The evidence above
uses this corrected capture path. This bug mislabelled test images; it was not
a cause of missing animation in ordinary play, which does not read back frames.

Normal native launches write local JSONL playback diagnostics to
`~/.local/share/caocao-prototype/diagnostics/session-*.jsonl` (under
`XDG_DATA_HOME/caocao-prototype` or `CAOCAO_PROTOTYPE_SAVE_DIR` if set). They record
the build/renderer, commands, restored prototype snapshots, confused/wait actions,
attack begin/end and the original poses submitted on successful SDL presents.
Each log is capped at4MiB. No screenshots or network transmission are involved.
`CAOCAO_PROTOTYPE_LOG_DIR` overrides the log directory and enables it in diagnostic
test modes. These records can distinguish no attack, absent playback and failed
present calls in an actual user session; a successful SDL present is not proof
of what the physical display showed. The title identifies **0.2.3**.

## 0.2.2 correction: skipped frames under slow rendering

The earlier fast-render checks were insufficient. The user's exact launcher is
`dist/prototype-macos-executable/Start Game.command`, which runs the standalone
**macOS** binary. The standalone binary and app bundle had identical Mach-O
`__TEXT` sections in 0.2.1; whole-file hashes differ because of their signatures.
The standalone's packaged enemy-hit smoke passed, but that alone did not test
frame scheduling under load.

A new test inserts 120ms between rendered frames. Before the fix, the frontend
advanced up to 100ms at once through 40ms animation ticks. It skipped swing frame10
and **both hit-flash ticks13/14** in the first enemy attack. The test failed;
this is a reproduced defect, although it does not prove the frame rate during
the user's particular run. [Before-fix log](evidence/prototype-standalone-slow-before.log).

The frontend now advances at most one original wait tick before rendering.
Every swing and flash therefore reaches the renderer even when rendering stalls.
Playback stretches under load instead of jumping over the impact. Damage, RNG,
saves and enemy decisions are unchanged. All seven enemy attacks pass the same
slow-render sequence after the fix, with the same final simulation hash.
[After-fix log](evidence/prototype-standalone-slow-after.log).

The exact packaged launcher was then run from `/tmp`, using its adjacent bundled
assets, with both the default macOS video renderer and the artificially slow
renderer. Both runs show all seven attacks with all four swing frames, hit flash
and damage number; default-renderer presentation lasts 1.160–1.205 seconds in
this capture run. Both end at the same state hash. Evidence:
[exact-launcher default run](evidence/prototype-exact-launcher-default.json),
[exact-launcher slow run](evidence/prototype-exact-launcher-slow.json),
[default-renderer log](evidence/prototype-exact-launcher-metal.log),
[actual captured frames](evidence/prototype-standalone-fixed-enemy.gif).
Linux passes the same slow-frame regression. macOS/Linux/Wasm keep the original
365-checkpoint deterministic trace and compatible saves; no browser UI rerun.

The 0.2.2 window title identified that build. The launcher forwards optional diagnostic
arguments and the executable can discover adjacent assets while running tests,
so the exact launcher can be checked from outside the development directory:

```sh
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDL_AUDIODRIVER=dummy \
'/Users/henryzhang/Documents/workspace/caocao-engine/dist/prototype-macos-executable/Start Game.command' \
--enemy-animation-slow-smoke /tmp/caocao-standalone-slow-check
```

This test does not write saves. Normal launch is still the same command without
arguments. The fast playback helper remains available for non-rendered simulation;
only the visible frontend caps its progress. Historical negative reproductions
below are retained to show the coverage gap; they are superseded by this finding.

## Where the animation came from

- Original `Ekd5.exe` function `0x4044e9` drives the 27-tick physical-strike
  sequence. This turn exported a [Cerberus/Ghidra dossier](../output/reverse-engineering/cerberus/exports/caocao/Ekd5.exe/dossiers/0x4044e9/summary.md)
  with the existing isolated, network-disabled static-analysis wrapper.
- Existing original-code fixtures contain **189 cases and 2,830 indexed frame
  hashes**. The replay checks frame ordering, sprite selection, direction,
  mirroring, palette transforms, waits, sounds and number requests. All pass
  again in [the original-fixture test log](evidence/prototype-attack-original-fixtures.log).
  These are bounded executable observations at declared host boundaries,
  not a new recording of Windows gameplay.
- The native prototype now uses that shared `physical_attack_animation` helper.
  `Unit_atk.e5` supplies 64×64 attack frames, `Unit_spc.e5` supplies 48×48 hit
  reactions, and `Unit_mov.e5` supplies recovery/idle frames. Original indexed
  palette lookup tables reproduce the bright impact flash.
- Nine sprites cover every current and legacy prototype unit. Staging includes
  27 raw sprite records and nine original WAV files, each checked against the
  existing source manifest. The package has **210 assets**; its original manifest
  identity stays unchanged so existing saves continue to load.

## Playback and state

`Prototype::take_attack()` returns a consume-once packet: actor/target IDs,
pre-damage display state, damage and the original ordered presentation events.
`PrototypePlayback` advances those events independently of SDL and the simulation.
The renderer places the original frames using the current battlefield origin.

The simulation commits an attack once, atomically. While its presentation runs,
the frontend displays the pre-impact board and delays unit removal, HP/result
feedback and victory/defeat screens until the sequence finishes. Player commands,
saving, toolbar loading and the next enemy step wait for completion. The I panel
pauses visual progression. A successful explicit snapshot restore discards stale
frames/sounds; a corrupt restore leaves live state and playback intact.

Presentation events are excluded from the save format. Stable snapshots retain
all simulation fields, including RNG, and do not replay completed damage after
restoration. Schema1/prototype-0.1 and schema2/prototype-0.2 retain their existing
meanings. The application version is 0.2.3; no save migration is required.

The original swing and hit WAVs are submitted to SDL audio when an output device
is available. Lack of an audio device does not prevent playing the battle.

## Verification

- macOS, Linux x86-64 and WebAssembly/Node pass the focused playback checks:
  consume-once packets/sounds, pre-damage HP, 27 ordered waits, hit flash, damage
  number, lethal hit/victory boundary, cleanup/restore and **144 current/legacy
  roster/facing asset cases**.
- The unchanged full simulation trace reaches victory on turn8 and passive
  defeat on turn13, with **365 save/reload checkpoints** and SHA-256
  `6916cde52483c54ce12fb7e2a8d9cd3d3e4d714e93f93fcf68882ddb419fdcfe`.
  Old-save restoration and the turn20 defeat test also pass.
- Linux↔Wasm fresh-process restoration reproduces the exact next attack:
  `908ed536888bb4c0fb0385717c8d560b173b428b553200c56c55266c2ff49f5b`.
- The native SDL smoke renders every attack tick, checks rejected overlapping
  commands and save/load, paused information panels, rejected corrupt restore,
  successful restore cleanup, the real enemy scheduler and delayed defeat.
  Saves use an isolated temporary directory. Both native targets pass.
- Logs: [macOS simulation](evidence/prototype-attack-macos-tests.log),
  [macOS SDL](evidence/prototype-attack-macos-render.log),
  [Linux/Wasm build and checks](evidence/prototype-attack-build.log).

Reproduce from the project root:

```sh
cmake --build build-mac --target prototype_tests caocao_prototype_app animation_tests -j8
build-mac/prototype_tests assets/prototype
build-mac/animation_tests assets/generated
CAOCAO_PROTOTYPE_SAVE_DIR=/tmp/caocao-attack-check \
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDL_AUDIODRIVER=dummy \
build-mac/caocao_prototype_app assets/prototype --attack-smoke /tmp/caocao-attack-frames
scripts/build-prototype.sh all
python3 scripts/package-prototype-mac.py
```

## Remaining differences

- **40ms per original wait tick is a prototype pacing choice.** Exact original
  wall-clock timing, refresh pacing and Windows background composition remain
  unmeasured. The displayed number uses the existing font atlas/placement,
  not the original Windows numeric drawing routine.
- The prototype still uses a single, always-hit, noncritical strike. Its current
  rules do not introduce counters, double attacks, misses, criticals, tactics,
  items or XP simply because the recovered animation helper supports more cases.
  Separate original death blinking and movement animation are not added here.
- Original swing/hit files and SDL queueing are checked; speaker listening quality
  and original mixing equivalence are unverified. Campaign music is still deferred.
- No new full browser battle or Safari acceptance claim. Linux and Wasm share the
  implementation and deterministic trace; the historical full-fidelity checklist
  remains incomplete.

JEV call061 reviewed the atomic presentation/save boundary and evidence scope.
Its recommendations were advisory; original fixtures and local tests determined
correctness. The delegated issue and the packet's corrected fixture count are
recorded in [JEV-DECISIONS.md](JEV-DECISIONS.md).

## Follow-up: reported missing enemy animations (2026-10-05)

The report was checked against the installed 0.2.1 Mac app and an added native
real-time regression mode. No production behavior was changed in this follow-up.
The first enemy phase (turn 2) clears 14 initial confusion statuses without attacks.
The next enemy phase (turn 3) performs **seven attacks** in this passive-play trace.
Every attack displayed all four swing frames, impact flash, recoil and a damage
number, lasting 1.079–1.098 seconds in the recorded software-renderer run. The
scheduler did not skip presentation on nonlethal damage. The installed app was
also observed animating an ordinary attack against a friendly infantry unit at
the bottom of the battlefield. No user save was written during these checks.

This does not disprove a different sequence or machine-specific issue; the
reported disappearance has not been reproduced. The earlier attack smoke tested
only a forced lethal enemy strike; the new regression covers consecutive normal
strikes and the confusion-only phase through the actual timed frontend loop.

Evidence: [timed trace](evidence/prototype-enemy-animation-check.json),
[log](evidence/prototype-enemy-animation-check.log),
[installed-app capture](evidence/prototype-enemy-live-check.jpg).

```sh
cmake --build build-mac --target caocao_prototype_app -j8
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDL_AUDIODRIVER=dummy \
build-mac/caocao_prototype_app assets/prototype --enemy-animation-smoke /tmp/caocao-enemy-check
```

This test takes roughly 22 seconds, captures real rendered attack states and does
not modify a save slot. Its expected final simulation hash is
`9d6567e3a328c56cf1e0df4ff28d49f158ec9a5b0e4be3018115c13da7c1a32b`.

### Linux-specific confirmation (2026-10-05)

The existing packaged Linux x86-64 executable passed its enemy-hit smoke using
its own bundled assets inside the local Debian container/Xvfb. Rebuilding with
the new real-time regression then verified **seven consecutive normal enemy
attacks**, each showing all four swing frames, impact flash and damage number.
Measured presentation durations were 1.083–1.100 seconds; all 14 initially
confused enemies skipped their first action. The final hash exactly matches
the Mac run above. This verifies native Linux SDL playback, not browser streaming.

[Linux log](evidence/prototype-enemy-linux-check.log),
[Linux timed trace](evidence/prototype-enemy-linux-frames/result.json),
[Linux captured animation](evidence/prototype-linux-enemy-attack.gif).
The Linux package was refreshed with the tested executable and diagnostic mode;
production gameplay and save formats did not change. Launch the current package
with `dist/prototype-linux/play.sh` on Linux x86-64. No macOS app, user save or
original game file was changed by this check.
