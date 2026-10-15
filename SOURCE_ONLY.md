# 3kingdoms-engine — source-only repository

This repository contains the C++20/SDL3 native and WebAssembly engine, browser
client, Cloudflare Worker backend, database migrations, tests, asset-import and
sprite-export tools, build scripts, translations, and engineering documentation.

Image assets are intentionally excluded. Original game resources, generated
artwork, sprite sheets, animations, fonts, audio/video, packaged executables,
build outputs, player saves, production database contents, diagnostic logs, and
credentials are not uploaded. Resource paths in code refer to local assets.
The live Cloudflare deployment and its secrets are unchanged by this upload.

## Building

- Native/browser setup and dependency versions: [README](README.md),
  [prototype guide](docs/PROTOTYPE.md), and `dependencies.lock.json`.
- Asset import: `tools/import_assets.py` accepts an original game directory;
  `scripts/stage-prototype-assets.py` and the other stage scripts prepare the
  local prototype asset tree. Obtain the original game and font separately.
  Special voxel artwork is also local and is not included in this repository.
- C++ source can be compiled without the SDL renderer using
  `cmake -S . -B build-source -DCAOCAO_RENDERER=OFF`, followed by
  `cmake --build build-source`. Asset-dependent tests and gameplay require the
  separately prepared asset tree.
- Web backend/client: `cd web-game && npm ci && npm run typecheck`.
  [Web deployment instructions](web-game/README.md) describe local assets,
  Google OAuth, Worker secrets, database setup, and the full web build.
  `npm run build` requires locally generated game and presentation files.
- Tests remain under `tests/` and `web-game/tests/`. Comparison fixtures retain
  numerical results, event metadata and image hashes. Two fixtures with embedded
  raster pixels (`original_terrain_panel.json`, `original_tutorial_terrain.json`)
  are intentionally excluded; regenerate them locally with their matching probes
  when running tests that need them.

Do not commit `.dev.vars`, credential files, original game data, images or player
records. `.gitignore` uses an explicit source-directory allowlist plus media and
private-state exclusions. Vendored source dependency notices are retained in
`THIRD_PARTY.md` and `third_party/`.
