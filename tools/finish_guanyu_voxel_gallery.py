#!/usr/bin/env python3
"""Add Guan Yu action labels, an open gallery and inspection controls after packaging."""
from pathlib import Path
import hashlib
import html
import json
import zipfile

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'output/character-art/guanyu-voxel-sprites'
manifest = json.loads((OUT / 'manifest.json').read_text())
assert manifest['frame_count'] == 28 and len(manifest['animations']) == 60
labels = {
    'walk': '移动 / Walk', 'attack': '普通攻击 / Attack',
    'critical-attack': '暴击 / Critical attack', 'receive-hit': '受击 / Receive hit',
    'dodge': '闪避 / Dodge', 'idle': '待机 / Idle',
    'script-1': '抬手示意 / Script 1', 'script-2': '防御姿势 / Script 2',
    'script-3': '受击姿势 / Script 3', 'script-4': '虚弱静止 / Script 4',
    'script-5': '攻击蓄势 / Script 5', 'script-6': '快速挥刀 / Script 6',
    'script-7': '连续挥刀 / Script 7', 'script-8': '转身动作 / Script 8',
    'script-9': '虚弱摇摆 / Script 9',
}
directions = {'south': '南 / South', 'west': '西 / West', 'north': '北 / North', 'east': '东 / East (mirrored)'}
actions = list(dict.fromkeys(t['action'] for t in manifest['animations']))
page = (OUT / 'index.html').read_text()
for action, label in labels.items():
    page = page.replace(f'<option>{action}</option>', f'<option value="{action}">{html.escape(label)}</option>')
page = page.replace('<h2>Animation viewer</h2>', '<h2>动作查看器 / Animation viewer</h2>')
page = page.replace('<label>Action<select', '<label>动作 / Action<select').replace('<label>Direction<select', '<label>方向 / Direction<select')
page = page.replace('<button id="play">Pause / 暂停</button>', '''<button id="play">Pause / 暂停</button>
<button id="tour">逐个播放全部60个动画 / Play all 60</button>
<div class="steps"><button id="previous-frame">上一帧 / Previous</button><button id="next-frame">下一帧 / Next</button></div>
<nav><a id="download-gif" download>下载当前 GIF</a><a id="download-webp" download>透明 WebP</a></nav>''')
page = page.replace('28 transparent PNG frames · 60 animated previews · same slot mapping as the native sprite library.',
                    '28 transparent PNG poses · 15 actions × 4 directions = 60 previews. Click any preview below to inspect it above.')
page = page.replace('<a href="README.txt">Readme</a>', '<a href="README.txt">Readme</a><a href="guanyu-model.png">Model PNG</a><a href="guanyu-attack-west.gif">640px attack GIF</a>')
start = page.index('<details><summary>All 60 animation previews</summary>')
end = page.index('</details>', start) + len('</details>')
groups = []
for action in actions:
    cards = []
    for direction in ['south', 'west', 'north', 'east']:
        t = next(t for t in manifest['animations'] if t['action'] == action and t['direction'] == direction)
        cards.append(f'<a class="card checker" href="{t["webp"]}" data-action="{action}" data-direction="{direction}" '
                     f'aria-label="{html.escape(labels[action]+" "+directions[direction])}">'
                     f'<img src="{t["webp"]}" alt="{action} {direction}" width="384" height="384">'
                     f'<span>{html.escape(directions[direction])} · {t["duration_ms"]} ms</span></a>')
    groups.append(f'<section class="action-group"><h3>{html.escape(labels[action])}</h3><div class="grid">'+''.join(cards)+'</div></section>')
gallery = '<section id="gallery"><h2>全部动作 / All 60 animation previews</h2><p>点击任意动画，在上方放大查看。Each action appears in all four directions.</p>'+''.join(groups)+'</section>'
page = page[:start] + gallery + page[end:]
page = page.replace('</style>', '.steps{display:flex;gap:10px}.steps button{flex:1}.action-group{margin-top:28px}h3{color:#f5dfad}.grid{grid-template-columns:repeat(4,minmax(0,1fr))}.card{color:#dce6f2}.card span{background:#202e40}@media(max-width:760px){.grid{grid-template-columns:repeat(2,minmax(0,1fr))}}\n</style>')
page = page.replace('let loaded=false,playing=true,elapsed=0,last=0;', 'let loaded=false,playing=true,elapsed=0,last=0,touring=false,tourIndex=0;')
page = page.replace('function select(){elapsed=0;scrub.max=current().frames.length-1;draw(0)}', '''function select(){elapsed=0;scrub.max=current().frames.length-1;draw(0);document.querySelector('#download-gif').href=current().gif;document.querySelector('#download-webp').href=current().webp;}
function stopTour(){touring=false;document.querySelector('#tour').textContent='逐个播放全部60个动画 / Play all 60';}
function tourNext(){tourIndex=(tourIndex+1)%timelines.length;const t=timelines[tourIndex];action.value=t.action;direction.value=t.direction;select();document.querySelector('#tour').textContent=`停止轮播 / Stop (${tourIndex+1}/60)`;}''')
page = page.replace('let t=current(),position=elapsed%t.duration_ms,i=0;', 'let t=current();if(touring&&elapsed>=t.duration_ms){tourNext();t=current();}let position=elapsed%t.duration_ms,i=0;')
page = page.replace("el.addEventListener('change',select)", "el.addEventListener('change',()=>{stopTour();select();})")
page = page.replace("play.addEventListener('click',()=>{playing=!playing;", "play.addEventListener('click',()=>{stopTour();playing=!playing;")
page = page.replace("scrub.addEventListener('input',()=>{playing=false;", "scrub.addEventListener('input',()=>{stopTour();playing=false;")
marker = 'Promise.all([...new Set(timelines.flatMap'
extra = '''document.querySelector('#tour').onclick=()=>{if(!loaded)return;if(touring){stopTour();return;}touring=true;tourIndex=-1;playing=true;play.textContent='Pause / 暂停';tourNext();};
for(const [id,delta] of [['previous-frame',-1],['next-frame',1]])document.getElementById(id).onclick=()=>{if(!loaded)return;scrub.value=(Number(scrub.value)+delta+current().frames.length)%current().frames.length;scrub.dispatchEvent(new Event('input'));};
for(const card of document.querySelectorAll('[data-action]'))card.onclick=e=>{e.preventDefault();stopTour();action.value=card.dataset.action;direction.value=card.dataset.direction;select();playing=true;play.textContent='Pause / 暂停';document.querySelector('.viewer').scrollIntoView({behavior:'smooth',block:'start'});};
'''
assert marker in page
page = page.replace(marker, extra + marker)
(OUT / 'index.html').write_text(page)
baseline = json.loads((OUT / 'reference/attack-only-frame-hashes.json').read_text())
correction_path = OUT / 'windup-correction.json'
correction = json.loads(correction_path.read_text()) if correction_path.exists() else None
changed = set(correction['changed_frames']) if correction else set()
assert all(hashlib.sha256((OUT / p).read_bytes()).hexdigest() == h for p, h in baseline.items() if p not in changed)
if correction:
    from PIL import Image, ImageDraw
    before = json.loads((OUT / correction['before_hashes_file']).read_text())
    actual = {p for p, h in before.items() if hashlib.sha256((OUT / p).read_bytes()).hexdigest() != h}
    assert actual == changed, actual
    timeline = next(t for t in manifest['animations'] if t['action'] == 'attack' and t['direction'] == 'west')
    rgb, rgba = [], []
    durations = [e['ms'] for e in timeline['frames']]
    for e in timeline['frames']:
        sprite = Image.open(OUT / e['file']).convert('RGBA')
        assert not e['mirror']
        canvas = Image.new('RGBA', (640, 640), '#202b3a')
        draw = ImageDraw.Draw(canvas)
        for y in range(0, 640, 16):
            for x in range(0, 640, 16):
                if (x // 16 + y // 16) % 2 == 0:
                    draw.rectangle((x, y, x + 15, y + 15), fill='#273444')
        canvas.alpha_composite(sprite, (64, 64)); rgb.append(canvas.convert('RGB'))
        transparent = Image.new('RGBA', (640, 640)); transparent.alpha_composite(sprite, (64, 64)); rgba.append(transparent)
    rgb[0].save(OUT / 'guanyu-attack-west.gif', save_all=True, append_images=rgb[1:], duration=durations, loop=0, optimize=False, disposal=2)
    rgba[0].save(OUT / 'guanyu-attack-west.webp', save_all=True, append_images=rgba[1:], duration=durations, loop=0, lossless=True, method=4)
    rgb[1].save(OUT / 'guanyu-attack-west-preview.png')
    for ext in ['gif', 'webp']:
        with Image.open(OUT / f'guanyu-attack-west.{ext}') as im:
            assert im.size == (640, 640)
            total = 0
            for i in range(im.n_frames):
                im.seek(i); im.load(); total += im.info.get('duration', 0)
            assert total == sum(durations)
checks = json.loads((OUT / 'verification.json').read_text())
checks.update(initial_six_frames_unchanged=not bool(changed & set(baseline)), gallery_cards=60, labelled_actions=15,
              native_slot_coverage={'move':11,'attack':12,'special':5}, browser_gallery_open='Not run; local file URL access previously blocked by browser policy.')
if correction:
    checks.update(corrected_frames=sorted(changed), other_26_frames_byte_identical=True, standalone_attack_gif_rebuilt=True)
(OUT / 'verification.json').write_text(json.dumps(checks, indent=2))
readme = (OUT / 'README.txt').read_text()
readme += '''
VIEWER
All 60 previews are displayed in an open gallery, grouped into 15 labelled actions.
Click any directional preview to enlarge it, change speed, pause, scrub or step frames.
Play all 60 runs a continuous tour through the original helper sequences.
Frame slots 6/7/8 are standing/finished-turn poses. Weak A/B are slots9/10.
Scripted IDs are animation capabilities; use in a particular story depends on its scripts.
Original poses are checked against their saved hashes, except explicitly recorded corrections.
The attack-only version is retained in ../guanyu-voxel-sprites-attack-only.zip.

Full reproducible export:
 .venv/bin/python tools/package_voxel_character.py output/character-art/guanyu-voxel-sprites
 .venv/bin/python tools/finish_guanyu_voxel_gallery.py
Do not run the older export_guanyu_voxel_attack.py for the full library: it exports
only the previously requested six-pose attack subset.
'''
(OUT / 'README.txt').write_text(readme)
if correction:
    with (OUT / 'README.txt').open('a') as f:
        f.write('\nWINDUP CORRECTION\nAttack 00 (south) and attack 08 (west) now place the guandao in front of the head.\nAll other 26 poses remain byte-identical. Previews, atlases and the 640px GIF were rebuilt.\nOriginals are retained under reference/before-windup-fix/.\nPrompts: windup-correction-prompts.json; built-in imagegen, 2 edit calls.\n')
(OUT / 'generation-prompts-complete.json').write_bytes((OUT / 'generation-prompts.json').read_bytes())
archive = OUT.with_suffix('.zip')
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
    for path in sorted(OUT.rglob('*')):
        if path.is_file() and path.name not in ['package.log', 'export-events']:
            z.write(path, path.relative_to(OUT.parent))
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
print('Guan Yu: 28 unique poses, 60 labelled previews, original/corrected frame hashes checked, ZIP verified.')
