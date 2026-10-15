# Unit hover panel — prototype 0.4.1

2026-10-06. Hover any living unit for 220 ms. A floating panel appears beside it,
flips to the left near the right edge, and stays inside the battlefield. Moving
to another tile or leaving the window dismisses it. It is presentation only:
clicking still selects, moves or attacks as before, and it does not pause the AI.
The popup is hidden during movement/attacks, dialogue, information/battle-choice
modals and victory/defeat screens.

## Original evidence reused

`battle_unit_panel` in `src/panel.cpp` implements Ekd5.exe's `0x4404bd` mode 0.
`tools/panel_probe.py` and the 265 cases in `tests/fixtures/original_panel.json`
record the original text, glyph positions, bar widths, icons and palette values.
The prototype now consumes those drawing commands directly, including:

- Unit name, class and `Lv`.
- Current/maximum HP and MP, with original Mark.e5 icons and fill patterns.
- `Exp` for the player's army, or the original friendly/enemy label for NPCs.
- Terrain name and the unit class's terrain percentage.

No new executable interpretation or JEV decision was needed. No JEV API calls
were made for this feature. Native AI integration remains available as before.

## Adaptations and limits

The original 192×96 panel is scaled to 288×144 logical pixels for readability.
Coordinates are translated from the original 13×8 viewport to the prototype's
whole-map display, including the 28×20 pursuit map. A translucent dark backing
replaces the original palette-darkened battlefield backing. The bundled Chinese
font is retained. These are intentional layout adaptations, not a pixel-identical
Windows panel claim. The 220 ms hover delay is a prototype UI choice.

Displayed values come from the current prototype records. Displaying MP/Exp
does not add tactics or experience progression to this simplified battle mode.
The original game, Electron wrapper, gameplay code, save schema, asset identity
used by saves and existing user save files are unchanged. Four original panel
images are added to the staged package with their existing source hashes.

## Verification

`--hover-smoke OUTPUT` renders the production balanced/conversation-enabled
mode through SDL, exercises pointer input at two window shapes, all 130 units
across the four battles, delay and dismissal, edge placement, modal suppression
and click-through selection. It checks that inspection leaves serialized state
unchanged. Its hidden test window prevents the user's real pointer from racing
the injected test events; ordinary launches still use a visible window.

Run it with isolated saves and local AI:

```sh
cmake --build build-mac --target caocao_prototype_app -j8
CAOCAO_JEV_MODE=off CAOCAO_PROTOTYPE_SAVE_DIR=/tmp/caocao-hover-save \
  build-mac/caocao_prototype_app assets/prototype \
  --hover-smoke docs/evidence/hover-panel/mac
python3 scripts/package-prototype-mac.py
```

Validation evidence is stored in `docs/evidence/hover-panel/`. See its platform
`verification.json` files and captures. Reopen `Start Game.command` to use the
new executable; an already-running game keeps its older UI until restarted.

Completed checks: macOS Metal rendering, Linux x86-64 SDL software rendering,
and the packaged Mac `Start Game.command` all passed the 130-unit hover run.
All four scenario hashes agree between those runs. Captures were inspected for
Chinese text, level, HP/MP bars, NPC/player footers and scaled windows. The
WebAssembly frontend compiled successfully with the four added images; browser
UI interaction was not rerun for this native-focused change.

During test development, real macOS pointer events raced the injected pointer
events, and one resized-window test initially remained in introductory dialogue.
The smoke run now uses a hidden window and explicitly finishes dialogue before
testing ordinary hover. These were test setup issues; the production modal and
dialogue suppression remains enabled.
