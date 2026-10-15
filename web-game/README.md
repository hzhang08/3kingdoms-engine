# 三国志曹操传 · 四战征程

Live app: https://3kingdoms.cc.cd

This hosts the existing C++20/SDL3 prototype as WebAssembly. Battles run on the
player's device. Cloudflare Workers handles Google authentication, per-user D1
save slots, leaderboards and authenticated JEV decisions. No Wine, virtual
machine or video streaming server is involved.

## Playing

Sign in with Google and choose a unique public username. Start a campaign or
resume a cloud checkpoint. The four battles run in order: 颍川之战 → 汜水关之战 →
虎牢关之战 → 董卓追击战. Equipment changes remain available only between battles.

- Click a friendly unit, move, then choose its action.
- Right-click or Control-click a unit for 武将情报. E ends your turn.
- On phones, hold a unit for half a second to view 武将情报. Pinch to zoom
  the game from 1× to 4×, and drag with one finger to pan. A single-finger
  drag cancels the touch instead of moving a unit.
- ＋ / − / 复位 control the camera. Information opens fully in view;
  关闭情报 restores the previous battlefield camera. Loading or advancing
  battles resets zoom. These view settings are not part of cloud saves.
- 保存 / 读取 or F6 opens ten manual slots plus the automatic slot.
- Automatic saves run about every 30 seconds at stable points, on returning
  home and around battle transitions. Confirm the cloud status before closing.
- Use the music/JEV toggles in the top bar. Unavailable JEV uses local AI.
- 修改名字 in the main menu changes the public username without changing saves.

Cloud slots are separate from the original Windows saves and native prototype
files. A failed upload leaves a per-account recovery draft in IndexedDB. A
second tab cannot silently overwrite a newer slot revision.

## Rebuilding

The parent project pins SDL3, Emscripten and engine dependencies. Use its
bootstrap instructions first when setting up a new machine.

```sh
cd /Users/henryzhang/Documents/workspace/caocao-engine
scripts/build-prototype.sh web
.venv/bin/python web-game/scripts/import-presentation.py /Users/henryzhang/Downloads/sgzccz
cd web-game
npm ci
npm run typecheck
npm test
npm run build
```

The importer checks this edition's executable hash and reads the original
files without modifying them. `public/presentation-manifest.json` records the
title resource IDs and original MP3 hashes. `scripts/build.mjs` splits the
53 MiB engine asset package into three pieces below Cloudflare's 25 MiB per-file
limit. The browser verifies every piece before starting SDL. Engine filenames
are versioned by content hash; public files never contain the credential files.

## Local development and isolated UI tests

`npm run dev` serves the production entrypoint locally; Google production
callbacks deliberately do not point at localhost. To test UI with disposable
local identities, use the separate loopback-only fixture Worker:

```sh
npx wrangler d1 migrations apply caocao-ui-test --config wrangler.ui-test.json --local --persist-to .wrangler/ui-state
npx wrangler dev --config wrangler.ui-test.json --ip 127.0.0.1 --port 8788 --persist-to .wrangler/ui-state
```

Open http://127.0.0.1:8788/__test/login and set a test username. This wrapper is
not imported by the production Worker and cannot issue test logins on a remote
hostname. Its fake credentials are not production credentials. Never deploy
`wrangler.ui-test.json`.

Optional campaign UI fixtures (synthetic victories, not real playthroughs):

```sh
cd ..
cmake --build build-mac --target prototype_web_campaign_tests
build-mac/prototype_web_campaign_tests assets/prototype docs/evidence/web-game/campaign-fixtures
cd web-game
../.venv/bin/python scripts/seed-ui-fixtures.py
```

The seeder refuses occupied slots 6–9 and touches only `.wrangler/ui-state`.
Tests in `tests/accounts.test.ts` create and remove their own temporary D1.
`tests/game-gestures.test.ts` checks touch arbitration and transformed targeting
without a browser. See [phone release verification](../docs/WEB-GAME-PHONE-CONTROLS.md)
for the separate browser checks and real-device testing limits.

## Production resources and deployment

- Worker: `caocao-game`; custom domain `3kingdoms.cc.cd`.
- D1: `caocao-game`, ID `7cbce041-f6c8-4037-93bb-511aba60b253`.
- Cloudflare account: `ad9fe8d09042662dde159d59992cf1bd`.
- Dedicated Google client: **Three Kingdoms Web**, in existing `legendgo`.
- Callback: `https://3kingdoms.cc.cd/api/auth/callback/google`.
- DNSHE delegates to `kenneth.ns.cloudflare.com` and `nelly.ns.cloudflare.com`.

```sh
npm run db:remote
npm run deploy
# Initial credential setup, or an intentional credential update:
node scripts/install-secrets.mjs /Users/henryzhang/Documents/credentials/3kingdoms-google.json
curl https://3kingdoms.cc.cd/api/health
```

The secret installer verifies the target, reads private files and uploads via
stdin. It preserves an existing session secret. Do not paste secrets into the
source, browser, CLI arguments, evidence or configuration variables. The four
Worker secrets are `GOOGLE_CLIENT_ID`, `GOOGLE_CLIENT_SECRET`,
`BETTER_AUTH_SECRET` and `JEV_API_KEY`. Routine releases preserve secrets and
D1 data. Apply additive migrations before deploying code that requires them.

Google login requests only `openid`, `email` and `profile`. The shared project's
audience remains unchanged. Google documents an exception for identity-only
login that does not require users to be on its testing allowlist:
https://support.google.com/cloud/answer/15549945

## Ranking, JEV and limits

Each user appears once per applicable board. Completed users are ordered by
their first server-recorded completion timestamp. In-progress users are ordered
by battles cleared, then enemy casualties as a fraction of that battle's enemy
roster, then the earliest matching progress record. Restoring an older save
does not reduce the recorded maximum. Completed users leave the progress board.

This is a casual prototype leaderboard, **not an authoritative anti-cheat
system**. The server checks ownership, versions, hashes, progression and write
revisions; it does not replay every command. A deliberately modified client
can forge a plausible state and recompute its checksum. Public competitive
ranking would require server verification of command traces.

JEV receives bounded game state and legal candidate actions, not Google identity
data. The browser gets only a chosen action. Responses are validated by the
server and C++ engine. Rate limits are 20 requests/minute/user, 720/day/user and
5000/day globally, with deterministic local fallback. Identical requests are
cached. `jev_decisions` records attempts, result category, HTTP status, choice,
request hash and elapsed time, never the key or raw provider error.

```sh
npx wrangler d1 execute caocao-game --remote --command 'SELECT source,provider_attempted,provider_status,outcome,COUNT(*) AS decisions FROM jev_decisions GROUP BY source,provider_attempted,provider_status,outcome'
```

No new paid plan or VM was enabled. Hosting is subject to the account's Workers
and D1 quotas; JEV provider usage is separate. First load downloads about 65 MiB
including optional music. See the dated [verification report](../docs/WEB-GAME-DEPLOYMENT.md)
for tested behavior and limits. This release preserves the native prototype's
simplified rules; it does not establish full original-game equivalence.

Mobile release web.3 adds one-finger panning, bolder phone text, and automatic
end-turn after all own actions complete (never after a tentative move alone).
See `../docs/WEB-GAME-PHONE-CONTROLS.md` for validation and rollback.

Battle text in web.4 uses Noto Sans CJK SC Bold with antialiased coverage, keeping
original glyph spacing and save identities. The font source, pinned hashes,
license and generator are in `../assets/presentation-font/` and
`../tools/generate_battle_font.py`; no system font installation is needed.

On touch devices, tap the same unit again to dismiss its small popup without
losing selection; long-press still opens full information. Percentage/reset are
hidden for primary touch devices in web.5, while desktop retains both controls.

Battle language (web.6): **中文 / EN** switches battle text immediately and remembers
this browser's choice. Mobile uses pinch instead of ＋ / −; desktop retains zoom
buttons. See [translation sources and validation](../docs/WEB-GAME-LANGUAGE.md).

### Site language (web.7)

The homepage, dialogs and battle share a remembered 中文 / EN preference. The
homepage switch works before Google sign-in and during username setup. Account,
save, leaderboard, loading, status and privacy text now have English translations;
original title artwork and player usernames stay intact. `client/site-language.ts`
contains the site catalog; `tests/site-language.test.ts` checks coverage and
placeholders. The existing WebAssembly bundle and save identities are unchanged.
See [language verification](../docs/WEB-GAME-LANGUAGE.md).
