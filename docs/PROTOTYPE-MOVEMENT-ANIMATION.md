# Native walking animation — prototype 0.2.4

Updated 2026-10-05 (America/Los_Angeles).

Player and enemy units now travel tile by tile using original `Unit_mov.e5`
walking frames. Infantry and cavalry retain their recovered stepping/facing
patterns. Enemy movement finishes before weapon swings, hit flashes and damage
numbers. The status ribbon distinguishes **我军移动**, **敌军移动** and attacks;
the gold outline follows the moving unit. No new artwork was generated.

## Implementation and limits

`Prototype::movement_packet` uses the existing legal movement grid and recovered
route routine to emit a consume-once before-state, path and walking events.
`battle_walking_step` is the recovered original function `0x44201b`; its original
viewport coordinates are translated to the prototype's full-map view without
scrolling. Stationary friendly-unit overlays preserve pass-through ordering.
The renderer redraws terrain every frame, accounting for original final-tile
redraw requests; unexpected event types stop explicitly.

Movement and attack results still commit atomically in the simulation. A movement
packet queues before the existing attack packet, so neither damage nor the next
enemy action appears prematurely. Playback is transient: no RNG, rules, path
destination, combat result, snapshot schema or asset identity changed. Ordinary
commands and save/load controls wait for the entire presentation to finish. The
information panel pauses playback. A valid explicit restore discards movement
and queued attacks; invalid restores leave both state and presentation intact.
After movement finishes, cancellation restores the prior tile immediately, as
in the previous prototype.

The original helper's speed2 stepping is used, with40ms per wait and one final
destination-frame hold. As with attack playback, these milliseconds are a
prototype pacing choice, not a measured match to the original Windows window.
The full map does not reproduce original camera scrolling. Existing simplified
combat, AI and campaign limits remain.

## Verification

- All1,749 existing original walking sequences,550 original route cases and
  the rest of the movement comparison suite pass on macOS:
  [original-code comparison log](evidence/prototype-0.2.4-original-walking.log).
- New integration coverage exercises499 legal paths across current and legacy
  rosters,1,364 friendly pass-through overlay requests, full-map coordinates,
  final destination frames, consume-once packets, cancellation and restoration.
  Three ordinary enemy move/attack queues preserve pre-impact HP and ordering.
- The full365-checkpoint simulation trace remains
  `6916cde52483c54ce12fb7e2a8d9cd3d3e4d714e93f93fcf68882ddb419fdcfe`;
  autoplay still wins on turn8 and passive play loses on turn13.
  [Core log](evidence/prototype-0.2.4-core.log).
- `--movement-smoke` uses a120ms render delay and captures all20 walking frames
  on the path `(10,6) → (9,6) → (9,7) → (8,7) → (8,8) → (8,9)`.
  It checks command/save blocking, modal pause, corrupt/valid restore and
  post-movement cancellation/load. It requires an isolated save directory.
- The timed player-then-enemy test includes movement and eight enemy attacks,
  still showing every swing, flash and damage number. Its final state hash is
  unchanged. In this version its attack duration includes any preceding walk.
  [Mixed render log](evidence/prototype-0.2.4-mixed-render.log).

The exact packaged `Start Game.command` was tested from `/tmp`, with bundled
assets and the default **Metal** renderer. Its slow walking check presented all20
frames. The mixed check recorded one player walk, eight enemy walks and nine
attacks (one player/eight enemy), with no overlapping presentations and the
unchanged final hash `68a10959f9048b0d3fc10b4caeb3c93a65326ac74c77a1ca2f8b8a261bf271c2`.
Evidence: [walking result](evidence/prototype-0.2.4-packaged-walk.json),
[mixed result](evidence/prototype-0.2.4-packaged-mixed.json),
[ordered session events](evidence/prototype-0.2.4-packaged-session.jsonl).

![Walking renderer capture](evidence/prototype-0.2.4-walking.gif)

This cropped GIF contains actual Metal renderer frames from the deliberately
slow120ms test, enlarged with nearest-neighbor sampling; it is not normal-speed
gameplay footage. The [full frame](evidence/prototype-0.2.4-walking.png) shows the
moving-unit label and battlefield layout.

Linux passes the walking, attack, mixed enemy and slow enemy rendering tests.
Linux and WebAssembly both pass all499 paths and the same365-checkpoint trace;
cross-platform saves preserve the next attack. Both packages were rebuilt.
[Linux/Wasm build and verification log](evidence/prototype-0.2.4-linux-wasm.log).
No complete original-game equivalence or browser UI acceptance run is implied.

## Original evidence and JEV

The installed [Cerberus RE skill](/Users/henryzhang/.codex/skills/cerberus-re/SKILL.md)
exported a static [0x44201b dossier](../output/reverse-engineering/cerberus/exports/caocao/Ekd5.exe/dossiers/0x44201b/summary.md)
using the existing read-only, network-disabled analysis container. The existing
x86 execution fixtures determine behavior; decompiled C supplies context.

JEV reviewed queued movement/attack presentation, the final frame hold and
evidence scope. Request062 returned HTTP400; request063 changed the boolean
question to a choice and succeeded. Its advice agreed with the proposed queue
and bounded claims. Both attempts and the independent verification disposition
are retained in [JEV-DECISIONS.md](JEV-DECISIONS.md). No credential or user save was
sent in the review packet.

## Running and diagnosing

Use the existing `dist/prototype-macos-executable/Start Game.command`; the window
title must show **0.2.4**. An already-running0.2.3 process keeps its loaded code:
save during the player phase, close it and reopen the launcher to use the update.
Packaging replaces the executable atomically without stopping that process.

Local diagnostics now include `movement_begin`, `movement_presented` and
`movement_end` before any queued `attack_begin`. They remain under
`~/.local/share/caocao-prototype/diagnostics`, with the existing environment
overrides. These logs record renderer submissions, not physical-display proof.

```sh
CAOCAO_PROTOTYPE_SAVE_DIR=/tmp/caocao-walking-check \
'dist/prototype-macos-executable/Start Game.command' \
--movement-smoke /tmp/caocao-walking-frames
```
