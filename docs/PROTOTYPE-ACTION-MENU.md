> Version 0.5.0 enables the 策略 row for supported learned spells. **T** also
> opens the spell picker. See [current tactics behavior](PROTOTYPE-TACTICS.md).
> Historical 0.4.3 availability below describes the pre-tactics release.

# Unit action window — prototype 0.4.3

2026-10-06. Implements the unit command window requested in
`Screenshot 2026-10-06 at 5.28.26 PM.png`, in the simplified native prototype.
This is a presentation/input change; combat, RNG and saved-state rules remain
unchanged. Existing saves, original assets and the Electron project are preserved.

## Controls and behavior

- Select a friendly unit, move it, and the action popup opens after its walking
  animation ends. Selecting a unit, repeated clicks and Space before moving do
  not open it. To act in place, use the toolbar or a direct attack on an enemy
  already in range. Space can reopen the menu after a tentative move.
- Rows are **攻击、策略、道具、待命、取消**, using the original dialog labels.
- Attack is enabled only when an enemy is currently in range. Choose it and click
  a marked enemy. Invalid targets do nothing. Esc/right-click returns to the menu
  without losing the move; choosing Cancel then restores position and facing.
- Wait commits the unit's turn; Cancel reverses tentative movement and selection.
- Tactics and Items remain disabled: neither system exists in this simplified
  mode. Their status hints explain this, and disabled clicks cannot change state.
- Up/Down/Tab skip disabled rows; Return/Space confirms. Existing toolbar, quick
  direct attacks and map clicks remain usable. Menu borders/gaps consume clicks.
- F5/F9 work while the menu is open. Restoring a tentative move reopens the menu.
  The menu focus itself is transient UI state, not part of the save schema.
- The hover-stat popup is suppressed while the command window is open. The menu
  is cleared on actions, scenario changes and successful restoration. It never
  opens over enemy animation, dialogue, an info screen or the scenario chooser.

## Evidence and visual scope

`ui.json.command_menu` retains Ekd5.exe RT_DIALOG 279 (0x117), language 2052,
the five control IDs/labels and their original dialog-unit rectangles.
`tools/tutorial_menu_resource.py` extracted this resource read-only; it did not
run a DLL. Its resource hash is
`2d6df367731ce3508a16a4840bebd3831f7db4ec29c39bd7dc5107b0fd9909c3`.

Paired sword, feather, item and boot icons are the unmodified decoded pixels of
Cmdicon.dll RT_BITMAP 100–103, language 1041. These four pre-existing imports are
now staged with their original source hashes. Disabled icons are faded at render
time. Menu labels, row gaps and icon ordering follow the reference.

The menu is sized to 1.5× its approximate Windows pixel dimensions for the
whole-map viewport, just as the hover panel is enlarged for readability. It is
clamped inside the actual map, including the shorter 28×20 pursuit view, and
does not cover its own unit. The bundled Chinese font remains in use.

This is **not a pixel-identical reproduction**: the screenshot's faint decorative
background pattern has not been recovered; buttons use a plain light backing.
Windows font metrics, exact disabled embossing and original modal-loop timing
are not claimed. Original tactic/item enablement is deliberately not presented
as functional when those systems are absent from the prototype.

## Focused verification

`--menu-smoke OUTPUT` uses the production balanced/conversation-enabled mode and
SDL mouse/key events with isolated saves. It covers menus after movement in all four battles,
unavailable clicks, keyboard skipping/confirmation, map edges, two window shapes,
movement completion before opening, Wait, Cancel/position/facing rollback,
F5/F9 at the menu, targeting cancellation and an ordinary physical attack.
The attack result and RNG match the direct engine command exactly.

The hidden smoke window avoids interference from real pointer events while
preserving native renderer captures. Mac Metal and Linux SDL software checks,
including packaged launchers, were recorded in `docs/evidence/action-menu/`
for 0.4.2; the 0.4.3 timing regression is recorded in
`docs/evidence/action-menu-timing/`.
The prior hover and toolbar/movement/save checks were also run for regression.
The WebAssembly frontend compiles; browser UI interaction was not rerun for this
native-focused change. No JEV API calls were needed for this existing-resource UI.

```sh
python3 scripts/stage-prototype-assets.py
cmake --build build-mac --target caocao_prototype_app -j8
CAOCAO_JEV_MODE=off CAOCAO_PROTOTYPE_SAVE_DIR=/tmp/caocao-menu-save \
  build-mac/caocao_prototype_app assets/prototype \
  --menu-smoke docs/evidence/action-menu/mac
python3 scripts/package-prototype-mac.py
```

Save/close an already running game, then reopen the packaged
`dist/prototype-macos-executable/Start Game.command` to load version 0.4.3.

## 0.4.3 timing correction

The reported consecutive-unit failure reproduced before the fix: move Cao Cao,
Wait, then select another friendly unit. The menu appeared before that unit had
moved (`action-menu-timing/before.log`). `PrototypePlayback::moving()` inspected
the retained last packet even when playback was inactive, and every tile click
used it to schedule a menu. The former in-place click/Space shortcuts also opened
it before moving, contrary to the requested interaction.

A completed packet now reports no active movement. Automatic menu opening is
scheduled only at the end of the actual walk, for that packet's actor, and only
if that same unit is still selected with a tentative move. All menu entry points
require that move; restoring a saved tentative move remains supported. The
regression drives the real frame-completion scheduler with 120 ms frame gaps
(the visible progression is still capped at 40 ms per frame), consecutive units,
Cancel/reselection, rapid clicks and Space, plus unmoved and moved snapshots.
The simulation, RNG and save schema are unchanged. No JEV calls were needed for
this local presentation-state defect.
