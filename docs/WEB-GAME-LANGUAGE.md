# Site and battle language — web.7

Live: https://3kingdoms.cc.cd. Release `0.10.0-web.7`, October 10, 2026.
Worker: `f33fc1cc-a632-4832-a961-896bc90f2dd7`.

The top of the mobile homepage now has a **中文 / EN** switch, available before
sign-in and during first-time username setup. It uses the same remembered language
as the battle switch. Desktop and open dialogs also provide this switch.

English now covers homepage/navigation, Google sign-in prompts, account setup and
rename forms, save cards, overwrite/new-game confirmations, both leaderboard tabs,
ranking details and dates, loading/recovery/status/error messages, and About/privacy.
The battle keeps its existing English catalog. Original title artwork and its
Chinese lettering are preserved; usernames are shown exactly as entered. Google's
own sign-in pages and browser-owned dialogs use their own language settings.

The 167-entry site catalog is `web-game/client/site-language.ts`. Explicit static
markers and translated dynamic templates avoid translating user content. Language
changes preserve first-login/rename drafts, the current leaderboard tab, the open
save dialog, battle state, and saved progress. The existing localStorage key
`caocao-battle-language` is reused, so existing English selections carry over.
Document language, accessibility labels, placeholders and page metadata update too.

## Current verification

- Client/server type checks and all **23 automated web tests passed**, including
  coverage for every server Chinese error, marked static strings, dynamic template
  placeholders, client dialogs/status messages and the stored preference.
- **24 browser checks passed** in in-app Chromium: 390 × 844 and 320 × 740 touch
  emulation, plus 844 × 390 landscape. Homepage and dialogs fit; small-phone dialog
  headings use their own row. First-login setup, rename validation, save cards,
  leaderboard tabs, confirmations, sign-out and refresh persistence were checked.
- A restored battle receives the homepage's English selection. Changing language
  from its cloud-save dialog leaves the complete snapshot SHA-256 unchanged.
- Production homepage switch and refresh verified. Live HTML, JS, CSS and engine
  manifest hashes equal local build output; anonymous `/api/saves` returns 401.
- One initial input assertion encountered the browser tool's redaction; direct
  inspection of the synthetic local draft confirmed preservation. The evidence
  retains that invalid assertion. A local first-login fixture initially reused a
  unique email; distinct synthetic addresses fixed the fixture before validation.

No gameplay/C++/Wasm changes or rebuilds were needed. Engine bundle remains
`bb0399bd255806ad`. No database migration, secret change or production save mutation
was performed. Local tests use disposable identities and JEV remains disabled in
that fixture. This release did not repeat Google OAuth end-to-end, physical-phone,
Safari, or full-campaign tests.

Evidence: `docs/evidence/web-game/site-language-*`. Reproduce with `npm run typecheck`,
`npm test`, `npm run build` in `web-game`, then use the existing local UI fixture.
`/__test/login?first=1` creates a fresh local-only first-login identity.

Previous Worker `202e8150-7770-4ab7-998b-f5c2961b2c9c` (web.6) is retained for rollback.

---

# Battle language switch — web.6 (previous release)

Release: `0.10.0-web.6`, https://3kingdoms.cc.cd.
Worker: `202e8150-7770-4ab7-998b-f5c2961b2c9c`; engine bundle: `bb0399bd255806ad`.
Live HTML, JS, CSS, Wasm and all three asset chunks match the local release byte for byte.

Mobile replaces ＋ / − with **中文 / EN**. Pinch zoom and one-finger panning remain.
Percentage and reset stay hidden on primary touch devices. Desktop retains all
zoom controls and also has the language switch. The active language is underlined.
Chinese is the default; the choice is remembered in this browser's localStorage.

The switch translates the battle canvas: toolbar, action menu, objectives,
turn/status messages, units and classes, tactics, inventory, treasures, equipment,
five information tabs, battle dialogue/choices, combat cries, victory and defeat.
The surrounding battle toolbar, inspection hint and save-status indicator also
switch language. Account setup, homepage, leaderboard and cloud-save dialogs
remain Chinese; this release localizes the battle experience, not the entire site.

## Source and implementation

- `assets/localization/ui-en.tsv`: interface strings, dynamic message fragments,
  combat cries and punctuation.
- `script-lines-zh.json` / `script-lines-en.tsv`: stable correspondence for all 572
  unique source lines from S_00–S_03, including script annotations. These are not
  572 dialogue pages. Original speaker IDs, offsets and page boundaries remain.
- `data-en.json`: names, 53 classes/descriptions, 30 terrain labels, 68 tactics,
  104 item names/descriptions, equipment effects/types and 27 reachable named-unit
  biographies. Descriptions translate the original edition's claims; they do not
  establish that every described original mechanic exists in this prototype.
- `tools/build_battle_translations.py`: rebuilds `en.json`, records source hashes,
  checks current script/source/metadata coverage and font glyph availability.
- `include/caocao/battle_language.hpp`: exact phrases first, longest-match fragments
  for composed numeric messages, with bounded presentation caching.
- `prototype_web_language` changes presentation only. No engine commands, RNG,
  event flags, dialogue advancement or snapshots are modified. `prototype_web_ui_state`
  exposes the current language; `prototype_web_translate` handles presentation text.
- English text wraps on words and fits existing bounds. Dialogue can use six
  rendered lines while retaining the original saved page. English hover headers
  are reflowed around the original HP/MP bars. Chinese keeps its existing layout.

The catalog is staged separately from the canonical game manifest, so existing
save identities remain valid. No database migration, backend, JEV or credential
change accompanies this release. Existing packaged native apps remain unchanged;
the shared frontend compiles for native and WebAssembly.

## Validation

The generated catalog contains **1,566 entries**. All **1,439 required source
strings** in the checked battle surface translate without residual Han characters;
all translated glyphs exist in the bundled atlas. This catches coverage gaps,
not every possible editorial issue or unvisited dynamic combination.

- Native and WebAssembly catalog tests cover every exact entry, composed unit
  labels, tactics/items, combat cries, numeric values and repeated language changes.
- Native and WebAssembly frontend builds pass; client/server TypeScript passes.
- Existing 18 web tests pass, including gestures, camera bounds and backend tests.
- Chrome 390 × 844 touch emulation: mobile zoom buttons are absent; language
  switching preserves the stable battle hash, current dialogue page, information
  tab/unit, tentative movement and action menu. Closing info restores the menu.
- Inspected English stats, equipment, tactics, biography and class tabs, an action
  menu, battle headers, and dialogue in later battles. Loaded fixtures from all
  four scenarios. English remains selected after refresh and save resume.
- Pinch changes camera scale without changing the battle hash or dialogue page.
- At 390 px with desktop input, plus/minus/percentage/reset remain visible;
  plus and reset work without changing the battle state.
- A historical Yingchuan fixture had all player units acted, so existing automatic
  end-turn advanced it from turn 2 to 3. The initial fixed-turn assertion was an
  invalid test assumption; scenario/language checks passed. Evidence retains it.

Testing used an isolated local account/database with external JEV disabled.
Production users' saves were not used or modified. Chrome extension warnings were
unrelated to the game. Physical phone and Safari verification were not performed
for this release. This is not a new full-campaign acceptance run.

Evidence: `docs/evidence/web-game/language-*.txt`,
`language-browser-checks.json`, `language-*-mobile.png`,
`language-live-verification.json`; coverage: `assets/localization/coverage.json`.

## Reproduce

```sh
python3 tools/build_battle_translations.py
python3 scripts/stage-prototype-assets.py
cmake --build build-mac --target caocao_prototype_app battle_language_tests -j8
ctest --test-dir build-mac -R '^battle_language$' --output-on-failure
export PATH="$PWD/.venv/bin:/opt/homebrew/bin:$PATH"
source .cache/emsdk/emsdk_env.sh
cmake --build build-web --target caocao_prototype_app battle_language_tests -j8
node build-web/battle_language_tests.js assets/localization/en.json
npm --prefix web-game run typecheck
npm --prefix web-game test
npm --prefix web-game run build
```

Deploy from `web-game` with `npm run deploy` after verification. The previous
Worker `86a43872-7c14-449a-9873-9c4916285c8e` (web.5) is retained for rollback.
Save before refreshing a running game to load the new engine and catalog.
