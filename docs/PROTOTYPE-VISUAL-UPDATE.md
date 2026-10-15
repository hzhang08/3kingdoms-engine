# Opening-battle visual update — 2026-10-05

> Historical 0.1.1 UI pass. The subsequent 0.2.0 update implements the roster,
> positions, fires and confusion markers; see [layout matching](PROTOTYPE-LAYOUT-MATCH.md).

The simplified prototype's presentation was revised using the original game's
Electron/Wine screenshot as a reference. This update changes the shared SDL
frontend, staged presentation assets and packaging. It does not change the
simulation, initial roster, combat balance or snapshot identity.

Reference: [original 颍川之战, supplied slot 1, turn 2/20](/Users/henryzhang/Documents/workspace/caocao-test/docs/screenshots/yingchuan-opening-electron-2026-10-05.jpg).
This is a loaded opening save, not a newly recorded turn-one playthrough.

## Changes

- Replaced the permanent right sidebar with a full-width square battlefield and
  a compact bottom strip. Logical map size is the original 960×960 pixels;
  unit frames are now 48×48 instead of being reduced to 32×32.
- Added a gray beveled toolbar and blue battle/turn/phase ribbons. Buttons use
  pixels from the already imported `Ekd5.exe:RT_BITMAP:302:2052` resource.
  Wait, cancel, end turn, save, load, restart and information all work.
- Added the original `Mark.e5` white cursor and target marker. Reduced visual
  clutter: permanent colored unit boxes are gone; HP bars appear on selection
  or hover, with H available to show them all. Completed units remain dimmed.
- Added original portraits, Chinese class/terrain names, live unit counts and
  unit statistics. The information dialog shows actual prototype objectives
  and controls. Closing it does not commit/cancel a move. Gameplay commands are
  blocked while it is open; enemy animation pacing pauses until it closes.
- Enabled Retina rendering on native macOS. Inspection found that rendering to
  low-resolution window points first removed thin Chinese strokes; rendering
  into the high-density backing surface fixes this.
- Added an Apple Silicon native `.app` with bundled assets, license notices,
  local ad-hoc signature and a SHA-256 inventory. It opens without a terminal
  or a development working directory. The executable links only to macOS
  system libraries/frameworks; no Wine, Electron or separately installed SDL.
- Updated the browser canvas proportions to the same 960×1120 layout.

All 170 packaged assets were verified against the existing importer manifest.
Toolbar/portrait/cursor pixels were reused directly. No new art was generated.
The original game, Electron project, full importer output and user saves were
not edited. No JEV calls or new Cerberus investigations were needed: this work
used existing decoded assets and a visual reference, not new binary semantics.

## Validation

| Check | Result and evidence |
| --- | --- |
| Native macOS simulation | Victory turn 6, passive defeat turn 12; all 236 checkpoints unchanged. [log](evidence/prototype-visual-tests-mac.log) |
| Native Linux simulation/rendering | Same 236-checkpoint trace; scaled selection in portrait and wide/letterboxed windows, map edge rejection, modal input blocking, toolbar actions, move/cancel/save/load and fresh-process restoration pass. [log](evidence/prototype-visual-linux.log) |
| Native macOS rendering/input | Same SDL event smoke and fresh-process restoration pass with Retina enabled and an isolated save directory. [log](evidence/prototype-visual-mac-smoke.log) |
| WebAssembly compatibility | Rebuild succeeds and produces the identical simulation trace and next attack after restoration. [log](evidence/prototype-visual-web.log) |
| Browser integration | Packaged page in Chrome on isolated localhost port 8181: real canvas clicks select Cao Cao at (10,6), move him to (10,8), toolbar-save, cancel back to (10,6), and load back to (10,8). Information/close also render correctly. A tall page may require scrolling. |
| Native app presentation | Launched the packaged Mac app; opening board and I/Esc information dialog visually inspected. Thin-stroke issue corrected and rechecked. [opening](evidence/prototype-native-opening-2026-10-05.jpg), [information](evidence/prototype-native-information-2026-10-05.jpg) |
| Glyph coverage | Staging checks all UI strings, class/terrain names and prototype roster names against the bundled Chinese atlas; no missing glyphs. |

The Linux/Wasm trace remains
`e596739d21c1df7563e5b512250be1fcfe15045e875e71b0bd2f09ee4cce43f4`.
The renderer's native save/load smoke hash remains
`f2cd292c2aba23a06e61b4a2774e2a617ca2bac6601b54a848dc27f36263af4a`.
Snapshots still use schema 1 and engine `prototype-0.1`; UI release 0.1.1 does
not invalidate existing saves. Test writes used isolated directories/origins.

This turn's browser check is a focused layout/input/persistence check, not a
new complete browser campaign playthrough or Safari acceptance run. Linux
rendering used Xvfb in the local x86-64 container. Native input smoke uses SDL
mouse events and the renderer's coordinate conversion; it is not an exhaustive
physical-input test across all desktop configurations.

## Differences intentionally retained

The screenshot contains the original turn-2 opening roster, camp fires and
confused enemies. The simplified mode still starts at turn 1 with six
controllable friendlies and eight enemies, its existing starting positions,
and no turn limit. It does not display a false `/20` limit or paint fire and
confusion effects unsupported by its simulation. Story, original battle
scripts, status rules, audio and later campaign stages remain outside this
visual update. This is closer visual presentation, not original-game parity.

## Deliverables

- `dist/prototype-macos/颍川之战.app`: local Apple Silicon native app.
- `dist/prototype-linux.tar.gz`: updated native Linux package.
- `dist/prototype-web.tar.gz`: updated browser/WebAssembly package.
- `src/prototype_frontend.cpp`: shared frontend source.
- `scripts/package-prototype-mac.py`: reproducible Mac packaging.

See [PROTOTYPE.md](PROTOTYPE.md) for controls, save locations and build commands.
