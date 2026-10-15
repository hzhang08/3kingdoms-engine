#!/usr/bin/env python3
"""Package the explicitly incomplete investigation build, without dev runtimes."""
from pathlib import Path
import hashlib,json,shutil,tarfile
ROOT=Path(__file__).resolve().parents[1]
VERSION='0.22.0-investigation'
sdl_notices=[ROOT/'.cache/SDL3-3.4.16/LICENSE.txt',ROOT/'build-web/_deps/sdl3-src/LICENSE.txt',ROOT/'build-linux/_deps/sdl3-src/LICENSE.txt']
sdl_notice=next((p for p in sdl_notices if p.is_file()),None)
if sdl_notice is None:raise SystemExit('SDL license missing; complete an SDL frontend build first')
dist=ROOT/'dist';dist.mkdir(exist_ok=True)
for platform in ('linux','web'):
    output=dist/platform
    output.mkdir(exist_ok=True)
    if platform=='linux':
        for name in ('caocao','caocao_trace'):shutil.copy2(ROOT/'build-linux'/name,output/name)
        shutil.copytree(ROOT/'assets/generated',output/'assets',dirs_exist_ok=True)
        (output/'run.sh').write_text('#!/bin/sh\nset -eu\nbundle_dir="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"\nexec "$bundle_dir/caocao" "$bundle_dir/assets" "$@"\n')
        (output/'run.sh').chmod(0o755)
    else:
        for name in ('caocao.html','caocao.js','caocao.wasm','caocao.data','CaoCaoSong.ttf'):shutil.copy2(ROOT/'build-web'/name,output/name)
        (output/'serve.sh').write_text('#!/bin/sh\nset -eu\ncd "$(dirname "$0")"\nexec python3 -m http.server "${PORT:-8765}" --bind 127.0.0.1\n')
        (output/'serve.sh').chmod(0o755)
    shutil.copy2(ROOT/'docs/COMPATIBILITY.md',output/'COMPATIBILITY.md')
    shutil.copy2(ROOT/'THIRD_PARTY.md',output/'THIRD_PARTY.md')
    shutil.copytree(ROOT/'third_party/licenses',output/'licenses',dirs_exist_ok=True)
    shutil.copy2(sdl_notice,output/'licenses/SDL3.txt')
    shutil.copy2(ROOT/'assets/generated/fonts/OFL.txt',output/'licenses/SourceHanSerif.txt')
    (output/'README.txt').write_text('INCOMPLETE INVESTIGATION BUILD — NOT THE OPENING-BATTLE MILESTONE.\nVersion '+VERSION+'. Runs the opening story, initializes 26 battle units and plays the complete introduction and first-turn infantry dialogue.\nThree allied attacks defeat the first target. The enemy-phase banner resets action flags; Liu Bei, Guan Yu and Zhang Fei appear, speak and complete their scripted entrance walks. The first enemy chooses poison, moves to its original attack tile, displays its target preview and prepares separate poison/damage rolls and plays the original caster pose and Chinese tactic caption, all 13 poison-effect frames and target reaction/damage number. Applies the first cast damage (10 to 0 HP), MP cost (48 to 40) and action flags. The archer speaks his last line and retires; his dialogue supports save/resume. Zhang Liang then moves and casts poison at infantry 210. The original secondary poison-status effect now completes; the target gains poison and the caster spends 8 MP. Five further enemy attacks and six movement-only actions complete. Continues into turn two with the original environment update, banner and action-flag reset. Liu Bei, Guan Yu and Zhang Fei discuss the fire attack with speaker camera focus; dialogue supports save/resume. Zhang Fei places the first fire; Guan Yu moves, speaks and places two more fires. Fire terrain, original animated Mark pixels and terrain-effect records persist across save/reload. The original MCALL05 summon plays with all 30 image frames and 12 masks; its loaded identity survives four dialogue save points. Xu Zijiang speaks while absent from the battlefield. All 27 fires are placed, then the enemy panic animations and dialogue continue. Area confusion now plays for all 13 selected enemies, including the original direct-pixel frames. The following six AI commands and Cao Cao/cavalry arrival, dialogue and movement complete. Stops at S_00.eex:0x2f13 before the dedicated objective banner. Player actions, complete combat and victory remain unimplemented.\nOlder prototype saves are rejected without overwriting their slots.\nSee COMPATIBILITY.md. Personal local use; original assets remain copyrighted.\n')
    archive=dist/f'caocao-engine-{VERSION}-{platform}.tar.gz'
    with tarfile.open(archive,'w:gz') as tar:tar.add(output,arcname=f'caocao-engine-{platform}')
source=dist/f'caocao-engine-{VERSION}-source.tar.gz'
with tarfile.open(source,'w:gz') as tar:
    for name in ['CMakeLists.txt','README.md','THIRD_PARTY.md','dependencies.lock.json','requirements.txt','.gitignore','.dockerignore','include','src','tools','tests','scripts','web','third_party','docs']:
        tar.add(ROOT/name,arcname='caocao-engine/'+name,filter=lambda t:None if '__pycache__' in t.name or t.name.endswith('.pyc') else t)
checksums={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(dist.glob('*.tar.gz'))}
(dist/'SHA256SUMS.json').write_text(json.dumps(checksums,indent=2)+'\n');print(json.dumps(checksums,indent=2))
