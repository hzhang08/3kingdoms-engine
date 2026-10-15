# Web game deployment and verification — 2026-10-09

Live: **https://3kingdoms.cc.cd**. Source, controls and reproducible deployment:
[web-game/README.md](../web-game/README.md).

## Delivered

The current C++/SDL3 native prototype runs in the browser through WebAssembly.
Cloudflare Workers handles Google identity, D1 saves, leaderboards and JEV. There
is no Wine runtime, helper process, virtual machine or backend video stream.

Google login leads first-time users to a unique username prompt. Returning users
can start a four-battle campaign, resume the automatic checkpoint, choose from
ten manual slots, or open the two leaderboards. The title artwork is decoded
from `Logo.e5#0` with its original palette; four original MP3s are copied unchanged.

Battle order: 颍川之战 → 汜水关之战 → 虎牢关之战 → 董卓追击战. The existing engine
carries party growth, equipment, inventory and rewards across battles. Equipment
changes remain available only between battles. The fourth victory returns home.
Native app releases, original game files and Windows saves remain separate.

## Deployed resources

- Dedicated Worker and D1 database: `caocao-game`.
- Database ID: `7cbce041-f6c8-4037-93bb-511aba60b253`.
- Current Worker version: `f33fc1cc-a632-4832-a961-896bc90f2dd7`
  (site-wide Chinese/English switch, `0.10.0-web.7`). Previous version retained:
  `202e8150-7770-4ab7-998b-f5c2961b2c9c`.
- HTTPS custom domain active; DNSHE delegates to Cloudflare. No unrelated DNS
  records, databases or OAuth clients were replaced.
- Dedicated **Three Kingdoms Web** Google client in approved project `legendgo`;
  callback `https://3kingdoms.cc.cd/api/auth/callback/google`.
- Google and JEV secrets are installed in Cloudflare. Private Google JSON is
  mode 0600 in the user's credentials directory. Secrets never enter browser
  assets or responses. An exact-value scan passed for all 21 public artifacts.
- The shared Google project's audience was unchanged. Identity-only login is
  covered by Google's [testing-mode exception](https://support.google.com/cloud/answer/15549945).
  A separate non-test account and Workspace admin restrictions were not tested.

Initial deployment encountered a Cloudflare API 504 before uploading. Retrying
succeeded without recreating any resource. No new paid plan or VM was enabled;
Cloudflare account quotas apply, and JEV provider billing is separate.

## Verified behavior

The phone-control update adds long-press inspection, pinch zoom and one-finger
panning, bolder mobile text, and automatic end-turn after all own actions finish. Its separate [verification report](WEB-GAME-PHONE-CONTROLS.md) records
Chrome touch emulation, desktop Safari, native regressions and deployment byte
checks. Physical iPhone/Android testing remains outstanding. The initial-release
checks below describe the earlier deployment and are retained as historical evidence.

| Area | Evidence and scope |
|---|---|
| Real Google login | Chrome account chooser → callback → username onboarding → main menu. This was a completed login, not merely a redirect. The live account uses Henry; 修改名字 can change it. |
| Real cloud saves | Production slot 1 preserves initial turn 2, and slot 2 preserves turn 4. Loading slot 1 retains slot 2; the restored checkpoint survives page reload. |
| Live battle | Original assets load from the custom domain. Two actual end-turn commands advance allies/enemies to turn 4. No synthetic victories were put in production. |
| Movement/UI | Chrome local test: Cao Cao selection, legal range, animated movement, action menu after movement, manual save. Retina framebuffer verified at 1202×1402 for a ~601×701 CSS box. |
| Safari | Loaded the same engine and resumed the Chrome-created local checkpoint with the moved unit position. Chinese UI rendered and Safari showed active audio. Production OAuth was tested in Chrome, not Safari. |
| Campaign UI/music | Local synthetic victories exercise all three next-battle controls. All four tracks show advancing audio playback. Final victory returns home. |
| Leaderboards | Local UI verifies both boards and transfer from progress to completed. Automated tests verify first completion timestamp and no progress regression after old-save restoration. |
| Account isolation | Five integration tests cover real signed test sessions, anonymous/expired/unverified/forged denial, origin checks, username collisions, foreign saves/runs, corrupt checksums and skipped battle rejection. |
| Atomic persistence | Concurrent first saves yield one success and one 409. Injected run-update failure rolls back the slot too; a later retry succeeds. |
| Native/Wasm parity | Campaign trace hash matches: `273261977866bb2bbf865a03cff2de833c8a6affee5554c692f6b89f23bbfaef`. All four new schema10 victory/reward fixture hashes also match across native/Wasm. |
| Native regression | Shared macOS frontend builds successfully. Packaged native releases were not replaced. |
| Live access controls | Anonymous saves, leaderboard and JEV requests return 401. HTTPS, CSP, nosniff and frame denial checked. |
| Production dependencies | `npm audit --omit=dev` reports zero known vulnerabilities at verification time; retained in `dependency-audit.json`. |

These are bounded smoke, integration and fixture checks, **not a full human
four-battle playthrough**, load test or original Windows equivalence claim.
Prior native evidence remains in [REA-GAMEPLAY-FIXES.md](REA-GAMEPLAY-FIXES.md)
and [PROTOTYPE-EQUIPMENT.md](PROTOTYPE-EQUIPMENT.md).

## JEV verification

The live smoke test recorded **5 gateway attempts: 3 valid HTTP 200 choices and
2 early request failures**. Failures occurred before receiving an HTTP response
with `redirect: error`; changing to `manual` without following redirects produced
valid choices. The slowest successful observed decision took 850 ms. Failed
requests used local AI and did not stop play.

Only the fixed Vercel JEV endpoint receives the key. Requests contain game state
and enumerated legal actions, not Google emails or account credentials. The
browser receives only request key, action ID and source; C++ checks the action.
Identical requests reuse a cached choice. Abandoned requests seal a fallback.

`jev_decisions` records request hash, ownership, timestamp, provider attempt,
HTTP status, outcome, choice and duration. Raw provider errors and secrets are
omitted. Caps: 20/minute/user, 720/day/user, 5000/day globally. Counts above refer
only to this deployment test, not historical native or investigation calls.

## Limits

- Rankings validate client snapshots and progression but do not replay every
  command. A modified client can forge a plausible state and recompute its hash.
  This is a casual prototype leaderboard, not an anti-cheat system.
- First load fetches ~53 MiB of game data plus Wasm/JS. Music loads as needed;
  content-addressed engine files are cached. Load/concurrency limits were not
  benchmarked, and free hosting does not imply unlimited JEV calls.
- Autosaves occur about every 30 seconds at stable points and at transitions.
  A sudden close may lose newer actions. Failed writes retain an IndexedDB
  recovery draft; manual saves provide separate recovery points.
- Native prototype simplifications and the four-battle boundary remain. The
  original full-campaign acceptance checklist is not completed by this release.

## Evidence

Under `docs/evidence/web-game/`: `backend-tests.txt`, `build.json`,
`live-security.json`, `live-jev-audit.json`, `native-campaign.json`,
`wasm-campaign.json`; `campaign-fixtures/` and `campaign-fixtures-wasm/` contain
explicitly synthetic fixtures and identical hashes. These are not deployed.

Screenshots: `live-login.png`, `live-main-menu.png`, `live-turn4.png`, `live-save-slots.png`,
`safari-battle.png`, `sishui-transition.png`, `hulao-transition.png`,
`dongzhuo-transition.png`, `completed-board-fixture.png`, `dns-delegated.png`.
