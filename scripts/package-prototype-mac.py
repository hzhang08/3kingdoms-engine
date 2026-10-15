#!/usr/bin/env python3
"""Package the Apple Silicon SDL prototype as an app and a raw executable."""
import argparse
import hashlib
import json
import os
import plistlib
import shutil
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--executable-only', action='store_true', help='leave the existing app bundle unchanged')
args = parser.parse_args()
subprocess.run(['python3', str(root / 'scripts/stage-prototype-assets.py')], check=True)

def copy_file(source, destination):
    """Replace an inode atomically; do not truncate a currently running game."""
    source, destination = Path(source), Path(destination)
    if destination.is_file() and source.read_bytes() == destination.read_bytes():
        return str(destination)
    pending = destination.with_name(destination.name + '.packaging-pending')
    shutil.copy2(source, pending)
    os.replace(pending, destination)
    return str(destination)

# A directly runnable Mach-O executable with adjacent assets. SDL_GetBasePath
# locates them even when launched outside this folder or after moving it.
portable = root / 'dist/prototype-macos-executable'
portable.mkdir(parents=True, exist_ok=True)
binary = portable / 'caocao-prototype'
copy_file(root / 'build-mac/caocao_prototype_app', binary)
binary.chmod(0o755)
shutil.copytree(root / 'assets/prototype', portable / 'assets', dirs_exist_ok=True, copy_function=copy_file)
for source, name in [
    ('assets/generated/fonts/OFL.txt', 'FONT-LICENSE.txt'),
    ('.cache/SDL3-3.4.16/LICENSE.txt', 'SDL-LICENSE.txt'),
    ('third_party/json.LICENSE.MIT', 'JSON-LICENSE.txt'),
    ('docs/PROTOTYPE.md', 'README.md'),
    ('docs/PROTOTYPE-VISUAL-UPDATE.md', 'PROTOTYPE-VISUAL-UPDATE.md'),
    ('docs/PROTOTYPE-LAYOUT-MATCH.md', 'PROTOTYPE-LAYOUT-MATCH.md'),
    ('docs/PROTOTYPE-ATTACK-ANIMATION.md', 'PROTOTYPE-ATTACK-ANIMATION.md'),
    ('docs/PROTOTYPE-MOVEMENT-ANIMATION.md', 'PROTOTYPE-MOVEMENT-ANIMATION.md'),
    ('docs/PROTOTYPE-DIALOGUE.md', 'PROTOTYPE-DIALOGUE.md'),
    ('docs/PROTOTYPE-ADDITIONAL-BATTLES.md', 'PROTOTYPE-ADDITIONAL-BATTLES.md'),
    ('docs/PROTOTYPE-BALANCE-JEV.md', 'PROTOTYPE-BALANCE-JEV.md'),
    ('docs/PROTOTYPE-HOVER-PANEL.md', 'PROTOTYPE-HOVER-PANEL.md'),
    ('docs/PROTOTYPE-ACTION-MENU.md', 'PROTOTYPE-ACTION-MENU.md'),
    ('docs/PROTOTYPE-TACTICS.md', 'PROTOTYPE-TACTICS.md'),
    ('docs/PROTOTYPE-TERRAIN.md', 'PROTOTYPE-TERRAIN.md'),
    ('docs/PROTOTYPE-ITEMS.md', 'PROTOTYPE-ITEMS.md'),
    ('docs/PROTOTYPE-UNIT-DETAILS.md', 'PROTOTYPE-UNIT-DETAILS.md'),
    ('docs/PROTOTYPE-EQUIPMENT.md', 'PROTOTYPE-EQUIPMENT.md'),
    ('docs/REA-GAMEPLAY-FIXES.md', 'REA-GAMEPLAY-FIXES.md'),
    ('docs/PROTOTYPE-DIAGNOSTICS.md', 'PROTOTYPE-DIAGNOSTICS.md'),
]:
    shutil.copy2(root / source, portable / name)
shutil.copytree(root / 'third_party/licenses', portable / 'licenses', dirs_exist_ok=True)
(portable / 'NOTICE.txt').write_text('Personal-use prototype using user-supplied original game art and data. Original game rights remain with their respective owners.\nSource Han Serif: SIL OFL. SDL3: zlib. nlohmann/json: MIT. See accompanying license files.\n')
launcher = portable / 'Start Game.command'
launcher.write_text('#!/bin/sh\nset -eu\ncd "$(dirname "$0")"\nexec ./caocao-prototype "$@"\n')
launcher.chmod(0o755)
(portable / 'START-HERE.txt').write_text(
    '颍川之战 0.10.0 — native Apple Silicon C++ executable\n\n'
    'Finder: double-click Start Game.command.\n'
    'Terminal: run ./caocao-prototype from this folder, or invoke its absolute path.\n'
    'Keep caocao-prototype and the assets folder together when moving the package.\n'
    'No Wine, Electron, browser, Python, or development tools are needed to play.\n'
    'Saves: ~/.local/share/caocao-prototype/manual-slot-1.json through manual-slot-10.json. F6 selects a slot, imports/exports backups, or reads an existing legacy battle save. Original slots are preserved.\n'
    'Local diagnostics: ~/.local/share/caocao-prototype/diagnostics/sessions-v2/run-*/. Each session has bounded events-*.jsonl, session.json and separate recovery-latest.json / recovery-previous.json backups. F6 Import Backup can restore a recovery file; manual slots are not overwritten automatically. Older session-*.jsonl logs are preserved.\n'
    'JEV: loads your external credential once at startup and keeps it in memory until exit; restart after changing the key. J toggles local AI; requests/cache/logs are in the save folder under jev/. Credentials are never bundled.\n'
    'Start a new battle (B or R) for revised combat, allied turns, tactics and events; loading an old save preserves its old rules. After victory, next battle carries earned progress. B starts an independent preset.\n'
    'Unit details: Control-click a unit, or select one and press V / click the bottom 武将 button. View 能力/装备/策略/武将列传/部队特性; Esc closes. G opens 宝物. After battle rewards, open 装备 then 装备更换 to manage owned gear; swaps preserve levels and experience. Normal right-click still cancels pending actions.\n'
    'Hover a unit briefly for its original-style class/level, HP/MP, experience/side and terrain panel.\n'
    'After moving, choose Attack, Wait or Cancel from the action menu. The menu opens only after that unit finishes moving. To act without moving, use the toolbar or click an adjacent enemy. Tactics: choose the strategy row or press T; fifteen offensive, healing and support spells are implemented for new games. Terrain heals at phase start. Scripted location items are collected once on committed actions; press P or click the Items button to view your saved inventory. Select a player-controlled unit, open Items, choose Use and click self or an adjacent friendly unit. Beans restore 30 HP; rice 80 HP; antidote clears poison. Item use ends the actor action.\n'
    'Press B to choose one of four independent battles. F5/F9 save/load the selected manual slot. This is a simplified prototype, not the complete campaign.\n'
)
subprocess.run(['codesign', '--force', '--sign', '-', str(binary)], check=True)
subprocess.run(['codesign', '--verify', '--strict', str(binary)], check=True)
files = {str(p.relative_to(portable)): hashlib.sha256(p.read_bytes()).hexdigest()
         for p in sorted(portable.rglob('*')) if p.is_file() and p.name != 'SHA256.json'}
(portable / 'SHA256.json').write_text(json.dumps(files, indent=2, sort_keys=True) + '\n')
print(f'Packaged directly runnable executable: {binary}')
if args.executable_only:
    raise SystemExit(0)

app = root / 'dist/prototype-macos/颍川之战.app'
contents = app / 'Contents'
resources = contents / 'Resources'
(contents / 'MacOS').mkdir(parents=True, exist_ok=True)
resources.mkdir(parents=True, exist_ok=True)
copy_file(root / 'build-mac/caocao_prototype_app', contents / 'MacOS/caocao_prototype_app')
shutil.copytree(root / 'assets/prototype', resources / 'assets', dirs_exist_ok=True, copy_function=copy_file)
(contents / 'Info.plist').write_bytes(plistlib.dumps({
    'CFBundleExecutable': 'caocao_prototype_app',
    'CFBundleIdentifier': 'com.local.caocao.prototype',
    'CFBundleName': '颍川之战',
    'CFBundleDisplayName': '颍川之战 · 战斗体验版',
    'CFBundlePackageType': 'APPL',
    'CFBundleShortVersionString': '0.10.0',
    'CFBundleVersion': '22',
    'NSHighResolutionCapable': True,
    'SDL_FILESYSTEM_BASE_DIR_TYPE': 'resource',
}))
shutil.copy2(root / 'assets/generated/fonts/OFL.txt', resources / 'FONT-LICENSE.txt')
shutil.copy2(root / '.cache/SDL3-3.4.16/LICENSE.txt', resources / 'SDL-LICENSE.txt')
shutil.copy2(root / 'third_party/json.LICENSE.MIT', resources / 'JSON-LICENSE.txt')
shutil.copy2(root / 'docs/PROTOTYPE.md', resources / 'README.md')
shutil.copy2(root / 'docs/PROTOTYPE-VISUAL-UPDATE.md', resources / 'PROTOTYPE-VISUAL-UPDATE.md')
shutil.copy2(root / 'docs/PROTOTYPE-LAYOUT-MATCH.md', resources / 'PROTOTYPE-LAYOUT-MATCH.md')
shutil.copy2(root / 'docs/PROTOTYPE-ATTACK-ANIMATION.md', resources / 'PROTOTYPE-ATTACK-ANIMATION.md')
shutil.copy2(root / 'docs/PROTOTYPE-MOVEMENT-ANIMATION.md', resources / 'PROTOTYPE-MOVEMENT-ANIMATION.md')
shutil.copy2(root / 'docs/PROTOTYPE-DIALOGUE.md', resources / 'PROTOTYPE-DIALOGUE.md')
shutil.copy2(root / 'docs/PROTOTYPE-ADDITIONAL-BATTLES.md', resources / 'PROTOTYPE-ADDITIONAL-BATTLES.md')
shutil.copy2(root / 'docs/PROTOTYPE-BALANCE-JEV.md', resources / 'PROTOTYPE-BALANCE-JEV.md')
shutil.copy2(root / 'docs/PROTOTYPE-HOVER-PANEL.md', resources / 'PROTOTYPE-HOVER-PANEL.md')
shutil.copy2(root / 'docs/PROTOTYPE-ACTION-MENU.md', resources / 'PROTOTYPE-ACTION-MENU.md')
shutil.copy2(root / 'docs/PROTOTYPE-TACTICS.md', resources / 'PROTOTYPE-TACTICS.md')
shutil.copy2(root / 'docs/PROTOTYPE-TERRAIN.md', resources / 'PROTOTYPE-TERRAIN.md')
shutil.copy2(root / 'docs/PROTOTYPE-ITEMS.md', resources / 'PROTOTYPE-ITEMS.md')
shutil.copy2(root / 'docs/PROTOTYPE-UNIT-DETAILS.md', resources / 'PROTOTYPE-UNIT-DETAILS.md')
shutil.copy2(root / 'docs/PROTOTYPE-EQUIPMENT.md', resources / 'PROTOTYPE-EQUIPMENT.md')
shutil.copy2(root / 'docs/REA-GAMEPLAY-FIXES.md', resources / 'REA-GAMEPLAY-FIXES.md')
shutil.copy2(root / 'docs/PROTOTYPE-DIAGNOSTICS.md', resources / 'PROTOTYPE-DIAGNOSTICS.md')
shutil.copytree(root / 'third_party/licenses', resources / 'licenses', dirs_exist_ok=True)
(resources / 'NOTICE.txt').write_text('Personal-use prototype using user-supplied original game art and data. Original game rights remain with their respective owners.\nSource Han Serif: SIL OFL. SDL3: zlib. nlohmann/json: MIT. See accompanying license files.\n')
# Local ad-hoc signing only. No developer certificate or notarization implied.
subprocess.run(['codesign', '--force', '--sign', '-', str(app)], check=True)
subprocess.run(['codesign', '--verify', '--deep', '--strict', str(app)], check=True)
files = {str(p.relative_to(app)): hashlib.sha256(p.read_bytes()).hexdigest()
         for p in sorted(app.rglob('*')) if p.is_file()}
(app.parent / 'SHA256.json').write_text(json.dumps(files, indent=2, sort_keys=True) + '\n')
print(f'Packaged local native app: {app}')
