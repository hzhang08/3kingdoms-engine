#!/usr/bin/env python3
"""Package the separate prototype, without touching older checkpoint releases."""
import hashlib
import json
import shutil
import subprocess
import tarfile
from pathlib import Path

root = Path(__file__).resolve().parent.parent
subprocess.run(['python3', str(root / 'scripts/stage-prototype-assets.py')], check=True)
web, linux = root / 'dist/prototype-web', root / 'dist/prototype-linux'
web.mkdir(parents=True, exist_ok=True)
linux.mkdir(parents=True, exist_ok=True)
for ext in ['html', 'js', 'wasm', 'data']:
    shutil.copy2(root / f'build-web/prototype.{ext}', web)
shutil.copy2(root / 'build-linux/caocao_prototype_app', linux)
shutil.copytree(root / 'assets/prototype', linux / 'assets', dirs_exist_ok=True)
for dest in [web, linux]:
    shutil.copy2(root / 'assets/generated/fonts/OFL.txt', dest / 'FONT-LICENSE.txt')
    sdl_license = next(p for p in [root / 'build-linux/_deps/sdl3-src/LICENSE.txt', root / '.cache/SDL3-3.4.16/LICENSE.txt'] if p.exists())
    shutil.copy2(sdl_license, dest / 'SDL-LICENSE.txt')
    if (root / 'docs/PROTOTYPE.md').exists():
        shutil.copy2(root / 'docs/PROTOTYPE.md', dest / 'README.md')
    shutil.copy2(root / 'docs/PROTOTYPE-VISUAL-UPDATE.md', dest / 'PROTOTYPE-VISUAL-UPDATE.md')
    shutil.copy2(root / 'docs/PROTOTYPE-LAYOUT-MATCH.md', dest / 'PROTOTYPE-LAYOUT-MATCH.md')
    shutil.copy2(root / 'docs/PROTOTYPE-ATTACK-ANIMATION.md', dest / 'PROTOTYPE-ATTACK-ANIMATION.md')
    shutil.copy2(root / 'docs/PROTOTYPE-MOVEMENT-ANIMATION.md', dest / 'PROTOTYPE-MOVEMENT-ANIMATION.md')
    shutil.copy2(root / 'docs/PROTOTYPE-DIALOGUE.md', dest / 'PROTOTYPE-DIALOGUE.md')
    shutil.copy2(root / 'docs/PROTOTYPE-ADDITIONAL-BATTLES.md', dest / 'PROTOTYPE-ADDITIONAL-BATTLES.md')
    shutil.copy2(root / 'docs/PROTOTYPE-BALANCE-JEV.md', dest / 'PROTOTYPE-BALANCE-JEV.md')
    shutil.copy2(root / 'docs/PROTOTYPE-HOVER-PANEL.md', dest / 'PROTOTYPE-HOVER-PANEL.md')
    shutil.copy2(root / 'docs/PROTOTYPE-ACTION-MENU.md', dest / 'PROTOTYPE-ACTION-MENU.md')
    shutil.copy2(root / 'docs/PROTOTYPE-TACTICS.md', dest / 'PROTOTYPE-TACTICS.md')
    shutil.copy2(root / 'docs/PROTOTYPE-TERRAIN.md', dest / 'PROTOTYPE-TERRAIN.md')
    shutil.copy2(root / 'docs/PROTOTYPE-ITEMS.md', dest / 'PROTOTYPE-ITEMS.md')
    shutil.copy2(root / 'docs/PROTOTYPE-UNIT-DETAILS.md', dest / 'PROTOTYPE-UNIT-DETAILS.md')
    shutil.copy2(root / 'docs/PROTOTYPE-EQUIPMENT.md', dest / 'PROTOTYPE-EQUIPMENT.md')
    shutil.copy2(root / 'docs/REA-GAMEPLAY-FIXES.md', dest / 'REA-GAMEPLAY-FIXES.md')
    shutil.copy2(root / 'docs/PROTOTYPE-DIAGNOSTICS.md', dest / 'PROTOTYPE-DIAGNOSTICS.md')
    (dest / 'NOTICE.txt').write_text('Personal-use prototype using user-supplied original game art and data. Original game rights remain with their respective owners.\nSource Han Serif font rasterization: SIL Open Font License (FONT-LICENSE.txt). SDL3: zlib license (SDL-LICENSE.txt). nlohmann/json: MIT (JSON-LICENSE.txt).\n')
    shutil.copy2(root / 'third_party/json.LICENSE.MIT', dest / 'JSON-LICENSE.txt')
    shutil.copytree(root / 'third_party/licenses', dest / 'licenses', dirs_exist_ok=True)
(web / 'play.sh').write_text('#!/bin/sh\nset -eu\ncd "$(dirname "$0")"\nprintf "Open http://127.0.0.1:8177/prototype.html\\n"\nexec python3 -m http.server 8177 --bind 127.0.0.1\n')
(linux / 'play.sh').write_text('#!/bin/sh\nset -eu\ncd "$(dirname "$0")"\nexec ./caocao_prototype_app "$PWD/assets" "$@"\n')
for dest in [web, linux]:
    (dest / 'play.sh').chmod(0o755)
    files = {str(p.relative_to(dest)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(dest.rglob('*')) if p.is_file() and p.name != 'SHA256.json'}
    (dest / 'SHA256.json').write_text(json.dumps(files, indent=2, sort_keys=True) + '\n')
    with tarfile.open(dest.with_suffix('.tar.gz'), 'w:gz') as archive:
        archive.add(dest, arcname=dest.name)
    print(f'Packaged {dest.name}: {len(files)} files')
