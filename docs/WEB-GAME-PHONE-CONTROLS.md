# Phone controls — 2026-10-09

Released at https://3kingdoms.cc.cd as `0.10.0-web.6`.

## Controls and behavior

- Hold a friendly, allied or enemy unit for 500 ms to open the existing original-art
  武将情报 window. A circular indicator shows the hold. Ordinary taps act on release.
- Moving more than 10 CSS pixels, adding a finger, cancellation, focus loss or
  opening another window cancels the pending tap. A blocked/empty long-press
  shows a short hint and never becomes a battle action.
- Pinch the game area from 1× fitted view to 4×; drag with **one finger** to pan.
  A drag beyond 10 CSS pixels cancels the tap/hold and pans; lifting one finger
  after a pinch continues panning without issuing a command. At fitted zoom,
  the camera remains bounded to the game area. Mobile shows **中文 / EN** instead
  of zoom buttons; desktop retains ＋ / − / percentage / reset.
- Opening information fits the complete window. It can be zoomed further;
  关闭情报 restores the previous camera. Existing tabs and unit switching remain.
- Rotation keeps relative zoom and bounds the camera to the new viewport.
  New games, resumed saves and battle transitions reset the view.

The camera transforms the fixed 960×1120 canvas. Pointer coordinates use the
inverse transform; the engine retains its own per-scenario tile scaling. Touch
events are intercepted before SDL's immediate mouse synthesis. Native mouse
and keyboard event handling remains in SDL.

New WebAssembly presentation APIs are `prototype_web_tap(x,y)`,
`prototype_web_inspect(x,y)`, `prototype_web_close_details()` and
`prototype_web_ui_state()` (logical dimensions, panel bounds/visibility, unit,
tab, pending action-menu unit and inspection message). The inspection API uses
existing stable-state guards and preserves tentative movement. Camera/gesture
state is not serialized. The core battle rules, schema10 snapshots, manifest identities,
backend storage, OAuth and JEV credentials were not changed.

## Battle language switch (web.6)

Mobile now uses pinch for zoom, with a **中文 / EN** switch in place of ＋ / −.
Battle text and the surrounding battle controls translate instantly; the choice
is remembered in this browser. [Catalog, coverage and tests](WEB-GAME-LANGUAGE.md).

## Dismissible touch popup and compact toolbar (web.5 history)

- A unit tap shows its small name/class/level popup; a second tap on the same
  unit dismisses it. Dismissal does not send a battle command, deselect the unit
  or cancel a tentative move. A later tap can show it again. Unit popup state
  is presentation-only and resets on scenario/load transitions and full inspection.
- Long-press retains the separate full 武将情报 window. Action menus, dialogue,
  spell/item targeting and animation keep their existing input guards.
- Mouse input retains the original desktop hover behavior.
- Percentage and 复位 are hidden only when `(hover:none) and (pointer:coarse)`
  matches. Desktop keeps both, even with a narrow window. ＋ / − and pinch zoom
  remain available on touch devices.

Native and Wasm frontend builds and TypeScript checking passed. Chrome phone
emulation verified 乐进 tap/tap/reopen, unchanged state hash on dismissal,
long-press, movement, cancellation, enemy popup dismissal and landscape controls.
Desktop and a 390px-wide desktop retained both controls and the usual hover.
One desktop test click used pre-resize bounds and landed on an empty tile;
repeating it with freshly observed bounds passed. The evidence preserves that
invalid test setup separately. Physical phone testing remains outstanding.

Evidence: `toggle-native-build.txt`, `toggle-wasm-build.txt`,
`toggle-browser-checks.json`, `toggle-popup-open.png`, `toggle-popup-closed.png`,
`toggle-desktop.png`, `toggle-deploy.txt`, `toggle-live-verification.json`.

## Battle font replacement (web.4)

The hosted battle now uses **Noto Sans CJK SC Bold 2.004**, replacing the previous
Source Han Serif atlas and its simulated mobile bolding. Chinese sans-serif
strokes have more even weight. A 3× grayscale coverage atlas and linear texture
sampling smooth text edges; original art keeps nearest-neighbor sampling.
The change applies to text drawn by the browser game on both phone and desktop.
Existing packaged native apps and their default typeface are unchanged.

All 1,883 required glyphs are present. Canonical glyph advances, logical cells,
wrapping and game/save manifests remain unchanged. Only unusually wide glyphs
are fitted into their existing advances. The source font is pinned by SHA-256;
its SIL OFL and copyright notice ship with the game and at
`/NOTO-SANS-FONT-LICENSE.txt`. Source:
https://github.com/notofonts/noto-cjk/tree/Sans2.004 .

Regenerate with `.venv/bin/python tools/generate_battle_font.py`, then rebuild
WebAssembly and the web package normally. `assets/presentation-font/manifest.json`
keeps presentation hashes separate from save identities. Staging rejects stale
metrics or changed font files; CMake relinks when the atlas manifest changes.

Validation: native frontend and Wasm builds; TypeScript checking; all-glyph
coverage/cell bounds; unchanged save-manifest identities; Chrome phone emulation
and desktop rendering of ability/equipment/strategy/biography/class tabs and a
campaign dialogue. An existing saved battle loaded successfully and inspection
left its state hash unchanged. No physical-phone font check was performed.
Evidence: `sans-font-generation.txt`, `sans-identity-check.json`,
`sans-native-build.txt`, `sans-wasm-build.txt`, `sans-browser-checks.json`,
`sans-info-phone.png`, `sans-equipment-phone.png`, `sans-dialogue-phone.png`,
`sans-dialogue-desktop.png`, `sans-deploy.txt` and `sans-live-verification.json`.

Deployed font release: `504072a9-835b-4553-b5f2-6b5d2307e0a3`; bundle `08e6757472976af2`.
The live manifest, engine JS/Wasm, all three resource chunks and font license
match the tested local files byte for byte.

## Mobile refinements (web.3)

- In web.3, mobile website text uses a semibold Chinese sans-serif stack, with heavier
  headings, brighter hints and larger small labels. The former serif SDL font atlas was drawn
  with a second offset stroke on coarse-pointer/small-screen devices, retaining
  the existing glyph advances and line wrapping. Rotation updates presentation
  without changing snapshots or the asset-manifest identity.
- The hosted game automatically invokes the existing `end_turn` command when
  all living, deployed, controllable own units have completed their actions.
  Allied NPCs do not count; confused own units cannot act. A tentative move is
  not a completed action. Animations, dialogue, choices and open game/website
  panels hold the transition. Manual early end-turn remains available.
- The native shared frontend compiles; the hosted convenience is enabled only
  by the website. Existing packaged native executables were not replaced.
- 18 web tests pass (13 gesture/geometry, 5 backend). Focused native and Wasm
  automatic-turn tests agree on transition hash
  `f61d9a4b563b2eb1135532764392b0bbee3da353258d35c8188f7a1bd389c2f3`.
  Checks cover tentative moves/cancel, multiple own units, casualties/confusion,
  non-player phases, rewards/dialogue, read-only eligibility and save restoration.
- The full existing Wasm prototype regression suite passes. An additional build
  target initially picked Xcode's Python 3.9; rerunning with the documented venv
  and emsdk environment passed. No runtime dependency was added.

Evidence: `mobile-refinements-tests.txt`, `mobile-native-build.txt`,
`mobile-native-auto-turn.txt`, `mobile-wasm-build.txt`,
`mobile-wasm-auto-build.txt`, `mobile-wasm-auto-turn.txt`,
`mobile-bold-info.png`, `mobile-bold-landscape.png` and
`mobile-browser-checks.json` under `docs/evidence/web-game/`.

The updated browser checks passed (17 recorded assertions): one-finger pan and
pinch-to-pan without battle mutation, transformed taps/inspection, pending
movement/action menus, automatic allied-turn entry after the last own action,
390×844 / 844×390 / 320×568 layouts, existing saves, a new manual checkpoint,
fresh-page restoration, desktop right-click and desktop Safari information.
`mobile-browser-checks.json` records the scope; these are not physical-device
results. `mobile-bold-small-phone.png` confirms compact toolbar labels.

Deployed Worker `cb4e2715-a43e-4275-ac0c-bc8b595cd31b`; bundle
`5203c274cc5e54c8`. Six public artifacts match local SHA-256 hashes with a
release query for cache revalidation. Initial plain curl requests returned the
previous cached release; fresh Chrome navigation displayed the updated controls.
Anonymous saves still return 401. See `mobile-deploy.txt` and
`mobile-live-verification.json`. Production saves were not used as test fixtures.

## Previous web.2 verification

| Check | Result and boundary |
| --- | --- |
| Automated web tests | 16 passing: 11 gesture/geometry tests plus 5 existing backend integration tests. |
| WebAssembly | Rebuilt successfully; existing battle, playback, save/restore and fresh-process continuation tests passed. |
| Native frontend | macOS build passed. Unit-information smoke passed for 130 units / 650 tabs, including pending movement, saves and enemy pause. Packaged native apps were not replaced. |
| Chrome phone emulation | 390×844 DPR3, 844×390 DPR3 and 320×568 DPR2. Long-press, pinch, pan, information tabs, movement, portrait/landscape, local cloud-style save/load and fresh page restoration checked. |
| State safety | Initial hash `b5105a92d5ec73320577c9d6955396a548f561db3bd6c9aa055332815690460d` and RNG `534775905` unchanged after inspection and pinch. Pending movement and action menus survive information viewing. Canceled touches were also checked with Cao Cao selected and a destination subsequently proven movable. |
| Four maps | Inspected Cao Cao in all four scenarios, including the edge of the 28-column fourth map. Existing synthetic victory saves were used for transitions; this is not another four-battle playthrough. |
| Desktop Chrome | Right-click cancellation and inspection still target correctly after zoom. |
| Desktop Safari | Loaded the Chrome-created local checkpoint, zoomed, opened information with V, closed it and returned home. |
| Live deployment | HTTPS HTML, app JS, CSS, engine manifest, engine JS and Wasm SHA-256 hashes exactly match the tested build; anonymous `/api/saves` returns 401. Existing production gameplay/saves were not used as test fixtures. |

Physical iPhone Safari and Android Chrome were **not tested**. Desktop Safari
does not establish iOS gesture compatibility. Automated Chrome Control-key
injection was unavailable through the browser tool; its existing native
modifier path is retained, but a fresh browser Control-click check is not claimed.

Evidence under `docs/evidence/web-game/`: `phone-tests.txt`,
`phone-wasm-build.txt`, `phone-native-build.txt`, `phone-native-details.txt`,
`phone-native-details.tar.gz` (losslessly archived screenshots),
`phone-browser-checks.json` (22 recorded browser assertions),
`phone-info-portrait.png`, `phone-zoom-portrait.png`,
`phone-safari-desktop.png`, `phone-deploy.txt`, `phone-live-verification.json`.

## Rebuild and rollback

```sh
cd /Users/henryzhang/Documents/workspace/caocao-engine
scripts/build-prototype.sh web
cmake --build build-mac --target caocao_prototype_app -j 8
cd web-game
npm run typecheck
npm test
npm run deploy
```

Current Worker: `202e8150-7770-4ab7-998b-f5c2961b2c9c`.
Engine bundle: `bb0399bd255806ad`. Previous Worker retained for rollback:
`86a43872-7c14-449a-9873-9c4916285c8e`.

If rollback is needed, run from `web-game`:

```sh
npx wrangler rollback 86a43872-7c14-449a-9873-9c4916285c8e
```

No database migration or credential rotation accompanied this release. Save
before refreshing an already-open game to load the new controls. The shell JS
and CSS now request revalidation; engine assets retain immutable content hashes.
