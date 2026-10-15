#!/usr/bin/env python3
"""Package imagegen-authored voxel poses; no generation or game mutation here.

The seven retained source images and prompt record are in the output folder.
Cropping, uniform scaling, atlas layout and preview playback are reproducible.
"""
from pathlib import Path
import hashlib
import html
import json
import math
import shutil
import zipfile
from PIL import Image, ImageDraw, ImageFont, ImageOps

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'output/character-art/caocao-voxel-sprites'
ORIGINAL = ROOT / 'output/character-art/caocao-native-sprites'
SIZE = 512
PIVOT = (256, 456)
BODY_HEIGHT = 332
GROUPS = {'move': 11, 'attack': 12, 'special': 5}
ARCHIVES = {'UNIT_MOV.E5': 'move', 'UNIT_ATK.E5': 'attack', 'UNIT_SPC.E5': 'special'}
LABELS = {
    'move': ['South A', 'South B', 'North A', 'North B', 'West A', 'West B',
             'South / standing', 'North / standing', 'West / standing', 'Weak A', 'Weak B'],
    'attack': [f'{d} / {p}' for d in ['South', 'North', 'West']
               for p in ['Windup', 'Swing', 'Strike', 'Recover']],
    'special': ['South / guard', 'North / guard', 'West / guard', 'Hit reaction', 'Script pose 1'],
}
DIRS = ['north', 'east', 'south', 'west']


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def put(im, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    im.save(path)


def font(size):
    for path in ['/System/Library/Fonts/Supplemental/Arial.ttf',
                 '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf']:
        if Path(path).exists():
            return ImageFont.truetype(path, size)
    return ImageFont.load_default(size=size)


def checker(size, cell=16):
    im = Image.new('RGBA', size, '#202b3a')
    d = ImageDraw.Draw(im)
    for y in range(0, size[1], cell):
        for x in range(0, size[0], cell):
            if (x // cell + y // cell) % 2:
                d.rectangle((x, y, x + cell - 1, y + cell - 1), fill='#273444')
    return im


# Rectangles separate actual figure gutters, which need not be the exact
# mathematical half of a generated sheet. Anchors are hand-inspected ground
# positions in source coordinates, with a common crown-to-ground body scale.
specs = []


def add(group, index, source, rect, ground, body_height):
    specs.append(dict(group=group, index=index, source=f'source/{source}.png',
                      source_rect=rect, source_ground_anchor=ground,
                      source_body_height=body_height))


move_rects = [(0, 0, 420, 430), (420, 0, 830, 430), (830, 0, 1254, 430),
              (0, 430, 420, 827), (420, 430, 830, 827), (830, 430, 1254, 827),
              (0, 827, 420, 1254), (420, 827, 830, 1254), (830, 827, 1254, 1254)]
for index, cell, anchor, height in [
    (0, 0, (242, 414), 348), (1, 1, (624, 414), 345),
    (2, 3, (230, 809), 328), (4, 6, (232, 1194), 332),
    (5, 7, (638, 1194), 332), (6, 2, (1034, 415), 346),
    (7, 5, (1039, 809), 329), (8, 8, (1039, 1195), 333),
]:
    add('move', index, 'movement', move_rects[cell], anchor, height)
add('move', 3, 'north-walk-b', (0, 0, 1290, 1219), (680, 1165), 1023)

rects = [(0, 0, 650, 648), (650, 0, 1254, 648),
         (0, 648, 660, 1254), (660, 648, 1254, 1254)]
for index, anchor, height in [(9, (358, 609), 565), (10, (926, 609), 565)]:
    add('move', index, 'reactions', rects[index - 9], anchor, height)
for direction, anchors, heights in [
    ('south', [(347, 623), (911, 623), (391, 1198), (935, 1198)], [535, 533, 528, 541]),
    ('north', [(350, 562), (929, 562), (356, 1121), (936, 1121)], [384, 385, 389, 389]),
    ('west', [(370, 639), (963, 639), (387, 1193), (975, 1193)], [537, 532, 519, 528]),
]:
    offset = ['south', 'north', 'west'].index(direction) * 4
    for i in range(4):
        add('attack', offset + i, f'attack-{direction}', rects[i], anchors[i], heights[i])
for i, rect, anchor, height in [
    (0, (0, 0, 700, 743), (376, 709), 658),
    (1, (700, 0, 1370, 743), (1035, 707), 649),
    (2, (1370, 0, 2116, 743), (1745, 708), 642),
]:
    add('special', i, 'guard', rect, anchor, height)
add('special', 3, 'reactions', rects[2], (423, 1204), 565)
add('special', 4, 'reactions', rects[3], (950, 1213), 521)
specs.sort(key=lambda v: (list(GROUPS).index(v['group']), v['index']))

frames = {}
checks = {'frame_count': 0, 'rgba_frames': 0, 'transparent_borders': 0,
          'source_crops_pixel_identical': 0, 'native_pose_references_resolved': 0,
          'atlas_roundtrips': 0, 'distinct_frame_hashes': 0, 'preview_files': 0}
for spec in specs:
    source = Image.open(OUT / spec['source']).convert('RGBA')
    rect = spec['source_rect']
    assert 0 <= rect[0] < rect[2] <= source.width and 0 <= rect[1] < rect[3] <= source.height
    crop = source.crop(rect)
    name = f'{spec["group"]}-{spec["index"]:02}'
    raw_path = OUT / 'source-frames' / f'{name}.png'
    put(crop, raw_path)
    assert Image.open(raw_path).tobytes() == crop.tobytes()
    checks['source_crops_pixel_identical'] += 1
    scale = BODY_HEIGHT / spec['source_body_height']
    scaled = crop.resize((round(crop.width * scale), round(crop.height * scale)), Image.Resampling.LANCZOS)
    origin = (round(PIVOT[0] - (spec['source_ground_anchor'][0] - rect[0]) * scale),
              round(PIVOT[1] - (spec['source_ground_anchor'][1] - rect[1]) * scale))
    # Preserve generated alpha and colors. No background-removal or recoloring.
    im = Image.new('RGBA', (SIZE, SIZE))
    im.alpha_composite(scaled, origin)
    file = f'frames/{name}.png'
    put(im, OUT / file)
    put(im.resize((256, 256), Image.Resampling.LANCZOS), OUT / f'frames-256/{name}.png')
    frames[spec['group'], spec['index']] = im
    bbox = im.getchannel('A').point(lambda x: 255 if x > 64 else 0).getbbox()
    # A low-alpha speckle does not define a sprite edge; keep it in the PNG.
    assert bbox and bbox[0] > 4 and bbox[1] > 4 and bbox[2] < SIZE - 4 and bbox[3] < SIZE - 4, (name, bbox)
    assert im.mode == 'RGBA' and im.getchannel('A').getextrema() == (0, 255)
    checks['frame_count'] += 1
    checks['rgba_frames'] += 1
    checks['transparent_borders'] += 1
    spec.update(file=file, original_frame=spec['index'], label=LABELS[spec['group']][spec['index']],
                original_archive=next(k for k, v in ARCHIVES.items() if v == spec['group']),
                source_crop_file=str(raw_path.relative_to(OUT)), scale=scale,
                paste_origin=list(origin), canvas_size=[SIZE, SIZE], pivot=list(PIVOT),
                visible_alpha_bounds=list(bbox), sha256=sha(OUT / file))
checks['distinct_frame_hashes'] = len({s['sha256'] for s in specs})
assert checks['frame_count'] == checks['distinct_frame_hashes'] == 28

# Native-style group atlases, plus one combined atlas. All use row-major order.
atlases = []
for group, count in GROUPS.items():
    cols = {'move': 3, 'attack': 4, 'special': 5}[group]
    im = Image.new('RGBA', (cols * SIZE, math.ceil(count / cols) * SIZE))
    for n in range(count):
        im.paste(frames[group, n], ((n % cols) * SIZE, (n // cols) * SIZE))
    file = f'{group}-sheet.png'
    put(im, OUT / file)
    decoded = Image.open(OUT / file)
    for n in range(count):
        x, y = (n % cols) * SIZE, (n // cols) * SIZE
        assert decoded.crop((x, y, x + SIZE, y + SIZE)).tobytes() == frames[group, n].tobytes()
        checks['atlas_roundtrips'] += 1
    atlases.append({'file': file, 'group': group, 'columns': cols, 'cell_size': [SIZE, SIZE], 'count': count})
combined = Image.new('RGBA', (SIZE * 4, SIZE * 7))
for i, spec in enumerate(specs):
    combined.paste(frames[spec['group'], spec['index']], ((i % 4) * SIZE, (i // 4) * SIZE))
    spec['combined_atlas_rect'] = [(i % 4) * SIZE, (i // 4) * SIZE, SIZE, SIZE]
put(combined, OUT / 'all-frames-transparent.png')

# Labeled human-readable overview.
sheet = Image.new('RGB', (1584, 1818), '#111925')
draw = ImageDraw.Draw(sheet)
draw.text((28, 22), 'CAO CAO  |  VOXEL SPRITE LIBRARY', font=font(34), fill='#f5dfad')
draw.text((28, 68), '28 authored poses  /  512px transparent frames  /  mounted Cao Cao', font=font(20), fill='#c7d5e3')
draw.text((28, 96), 'New imagegen artwork using the approved voxel design. East uses mirrored west frames.', font=font(17), fill='#94a9bd')
y = 145
for group, count in GROUPS.items():
    draw.text((28, y), f'{group.upper()}  /  {count} frames', font=font(25), fill='#f5dfad')
    y += 43
    for n in range(count):
        x, row = 28 + (n % 6) * 256, y + (n // 6) * 278
        tile = checker((240, 240))
        tile.alpha_composite(frames[group, n].resize((240, 240), Image.Resampling.LANCZOS))
        sheet.paste(tile.convert('RGB'), (x, row))
        draw.text((x, row + 244), f'{n:02}  {LABELS[group][n]}', font=font(16), fill='#dce6f1')
    y += math.ceil(count / 6) * 278 + 19
put(sheet.crop((0, 0, 1584, y + 12)), OUT / 'all-frames-labeled.png')

# Existing native C++ event streams supply slot order, mirroring and wait times.
# These are in-place pose previews: world translation, original palette FX,
# sound, opponents, map, spell particles and damage numbers are intentionally
# not represented as extra authored voxel artwork.
native = json.loads((ORIGINAL / 'native-events.json').read_text())
native = {**native, 'animations': [a for a in native['animations'] if a['sprite'] == 108]}
(OUT / 'native-events.json').write_text(json.dumps(native, ensure_ascii=False, indent=2) + '\n')
timelines = []


def pose_key(e):
    group, index = ARCHIVES[e['archive']], e['frame']
    assert (group, index) in frames
    checks['native_pose_references_resolved'] += 1
    return dict(file=f'frames/{group}-{index:02}.png', group=group, index=index,
                mirror=e.get('mirror', False))


def append_pose(timeline, pose, duration):
    if timeline and {k: v for k, v in timeline[-1].items() if k != 'ms'} == pose:
        timeline[-1]['ms'] += duration
    else:
        timeline.append({**pose, 'ms': duration})


for a in native['animations']:
    sequence = []
    current = pose_key(a['idle'])
    for e in a['events']:
        if e['kind'] == 'frame' and e['slot'] == a['subject_slot']:
            current = pose_key(e)
        elif e['kind'] == 'overlay' and e['slot'] == a['subject_slot'] and a['name'] != 'walk':
            current = pose_key(a['idle'])
        elif e['kind'] == 'wait':
            append_pose(sequence, current, e['ticks'] * native['tick_ms'])
    append_pose(sequence, pose_key(a['idle']), 400)
    timelines.append({'action': a['name'], 'direction': DIRS[a['direction']], 'frames': sequence})
for direction, start in enumerate([2, 4, 0, 4]):
    timelines.append({'action': 'idle', 'direction': DIRS[direction], 'frames': [
        dict(file=f'frames/move-{i:02}.png', group='move', index=i, mirror=direction == 1, ms=180)
        for i in (start, start + 1)]})

for t in timelines:
    rgba, gif_frames, durations = [], [], []
    for e in t['frames']:
        im = frames[e['group'], e['index']]
        if e['mirror']:
            im = ImageOps.mirror(im)
        im = im.resize((384, 384), Image.Resampling.LANCZOS)
        rgba.append(im)
        background = checker((384, 384))
        background.alpha_composite(im)
        gif_frames.append(background.convert('RGB'))
        durations.append(e['ms'])
    name = f'{t["action"]}-{t["direction"]}'
    (OUT / 'animations').mkdir(exist_ok=True)
    rgba[0].save(OUT / f'animations/{name}.webp', save_all=True, append_images=rgba[1:],
                 duration=durations, loop=0, lossless=True, method=4, minimize_size=True)
    gif_frames[0].save(OUT / f'animations/{name}.gif', save_all=True, append_images=gif_frames[1:],
                       duration=durations, loop=0, optimize=False, disposal=2)
    t.update(webp=f'animations/{name}.webp', gif=f'animations/{name}.gif', duration_ms=sum(durations))
    for ext in ('webp', 'gif'):
        with Image.open(OUT / t[ext]) as decoded:
            assert decoded.size == (384, 384)
            total = 0
            for n in range(decoded.n_frames):
                decoded.seek(n)
                decoded.load()
                total += decoded.info.get('duration', 0)
            # WebP stores a single unchanged pose as a still with no duration.
            if not (ext == 'webp' and decoded.n_frames == 1):
                assert total == t['duration_ms'], (name, ext, total, t['duration_ms'])
            checks['preview_files'] += 1

manifest = {
    'schema_version': 1, 'character': {'id': 0, 'name': '曹操', 'name_en': 'Cao Cao'},
    'artwork': 'New imagegen-authored 2D renders in the approved mounted voxel style; not a rigged 3D model.',
    'tool': 'built-in image_gen', 'generation_calls': 10, 'retained_source_images': 7,
    'form': 'One approved mounted design, matching resource 108 slot layout. No invented promotion outfits.',
    'frame_count': len(specs), 'frame_size': [SIZE, SIZE], 'pivot': list(PIVOT),
    'east_facing': 'Horizontal mirror of west-facing pose; weapon handedness mirrors too, as in original sprites.',
    'runtime_integration': False, 'frames': specs, 'atlases': atlases,
    'combined_atlas': {'file': 'all-frames-transparent.png', 'columns': 4, 'rows': 7, 'cell_size': [SIZE, SIZE]},
    'sources': [{'file': str(p.relative_to(OUT)), 'sha256': sha(p), 'size': list(Image.open(p).size)}
                for p in sorted((OUT / 'source').glob('*.png'))],
    'native_event_source_sha256': sha(ORIGINAL / 'native-events.json'),
    'preview_notes': 'In-place pose previews reuse native frame order, mirrors and waits, plus 400ms loop pause. '
                     'No audio, original indexed-palette flashes/tints, world movement, opponent, damage numbers or spell particles. '
                     '9 scripted IDs are helper capabilities, not claims of use in every battle. '
                     'Generated 2D renders have some costume/volume variation between views; engine integration and temporal art polish remain separate.',
    'animations': timelines,
}
(OUT / 'reference').mkdir(exist_ok=True)
reference_path = OUT / 'reference/approved-caocao.png'
shutil.copy2(ROOT / 'assets/special-hulao/source/unit-0.png', reference_path)
manifest['approved_reference'] = {'file': str(reference_path.relative_to(OUT)), 'sha256': sha(reference_path)}
(OUT / 'manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n')
(OUT / 'README.txt').write_text('''CAO CAO — VOXEL SPRITE LIBRARY

28 new authored poses matching the original resource-108 slot arrangement:
11 movement / idle / weak, 12 attack (4 each south/north/west), 5 guard / reaction / script.
East-facing previews mirror west, exactly as the old sprite convention does.
This is ONE approved mounted voxel design, not 3 promotion outfits or a rigged 3D model.
The native game and public special battle are not modified by this asset export.

Open index.html for the interactive viewer, playback speed, direction and frame inspection.
frames/: 28 uniformly positioned 512x512 transparent RGBA PNG images.
frames-256/: compact 256px copies for asset previews.
source-frames/: full-resolution, unmodified rectangular crops from generated sheets.
source/: seven retained imagegen outputs, untouched including their alpha channels.
reference/: the approved Cao Cao design used as the identity/style reference.
move-sheet.png, attack-sheet.png, special-sheet.png: row-major transparent group atlases.
all-frames-transparent.png: 4x7 combined atlas; per-frame rectangles in manifest.json.
all-frames-labeled.png: readable overview; this is the only sheet with labels/background.
animations/: 60 in-place action/direction previews, both transparent lossless WebP and GIF.
GIF uses a checkerboard and limited palette; PNG and WebP retain the complete RGBA color.
native-events.json: resource-108 event sequences from existing compiled C++ helpers.
manifest.json: frame mapping, crop rectangles, anchors, scales, hashes and playback timelines.
generation-prompts.json: complete built-in imagegen prompt set, including corrections.
generation-sources.json: provenance of retained source files (portable copies are in source/).
verification.json: file, alpha, atlas, native-reference, duration and archive checks.

TIMING AND LIMITS
Previews use native pose ordering, directional mirrors and 40ms helper waits;
idle alternates every 180ms and actions have a 400ms loop-end viewing pause.
Native world translation, palette flashes/tints, opponents, audio, numbers and shared
spell effects are omitted. Critical and ordinary attacks therefore share pose art,
with the additional critical timing retained. No spell-specific art is invented.
Generated 2D poses can have small detail/proportion changes between views. These are
new voxel illustrations, not exact 3D renders from a single mesh. Temporal art polish
and game integration need a separate pass. Source PNGs retain generated soft alpha
edges. Uniform frames are resampled for common scale; source crops are lossless.

REPRODUCE THE PACKAGE
From the caocao-engine project root:
  .venv/bin/python tools/package_caocao_voxel_sprites.py
Requires Pillow with WebP support, retained source images, and the prior native
sprite event export. This command does not call an image service or change game data.
''')

# Embed data rather than fetch JSON so the viewer works directly under file://.
data = json.dumps(timelines, ensure_ascii=False).replace('</', '<\\/')
options = ''.join(f'<option>{html.escape(a)}</option>' for a in dict.fromkeys(t['action'] for t in timelines))
cards = ''.join(f'<a class="card" href="{t["webp"]}"><img loading="lazy" src="{t["webp"]}" '
                f'alt="{t["action"]} {t["direction"]}"><span>{t["action"]} / {t["direction"]}</span></a>' for t in timelines)
page = '''<!doctype html><html lang="zh-CN"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>曹操 · Voxel sprite library</title><style>
*{box-sizing:border-box}body{margin:0;background:#101925;color:#dce6f2;font:16px/1.55 system-ui,sans-serif;padding:30px}main{max-width:1400px;margin:auto}h1,h2{color:#f5dfad}h1{font-size:clamp(28px,5vw,44px);margin:0}p{max-width:960px}a{color:#a9d9ff}nav{display:flex;gap:20px;flex-wrap:wrap;margin:22px 0}button,select{background:#2a394c;border:1px solid #52677f;color:white;border-radius:9px;padding:10px;font:inherit}button{cursor:pointer}.viewer{display:grid;grid-template-columns:minmax(280px,640px) minmax(260px,1fr);gap:30px;padding:20px;background:#182536;border-radius:18px}.checker{background-color:#202b3a;background-image:linear-gradient(45deg,#273444 25%,transparent 25%),linear-gradient(-45deg,#273444 25%,transparent 25%),linear-gradient(45deg,transparent 75%,#273444 75%),linear-gradient(-45deg,transparent 75%,#273444 75%);background-size:32px 32px;background-position:0 0,0 16px,16px -16px,-16px 0}canvas{width:100%;height:auto;display:block;border-radius:12px}.controls{display:flex;flex-direction:column;gap:18px}label{display:grid;gap:6px}.note{color:#aebed0;font-size:14px}.sheet{width:100%;display:block;border-radius:12px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(230px,1fr));gap:16px}.card{background:#202e40;border-radius:12px;padding:12px;text-decoration:none}.card img{width:100%;display:block}.card span{display:block;padding:8px}summary{cursor:pointer;padding:20px;background:#223246;border-radius:12px;margin:18px 0}input[type=range]{width:100%}.badge{color:#a5ddb6;letter-spacing:.1em;font-size:13px}#frameName{min-height:50px}@media(max-width:760px){body{padding:16px}.viewer{grid-template-columns:1fr;padding:12px}.controls{gap:12px}}
</style><main><span class="badge">CHARACTER ART LIBRARY · 28 POSES</span><h1>曹操 · Voxel sprite library</h1><p>骑马曹操的体素精灵库：移动、待机、挥剑、格挡、受击、虚弱与剧情动作。<br>28 transparent PNG frames · 60 animated previews · same slot mapping as the native sprite library.</p>
<nav><a href="../caocao-voxel-sprites.zip">Download ZIP</a><a href="all-frames-labeled.png">Labeled sheet</a><a href="all-frames-transparent.png">Transparent atlas</a><a href="README.txt">Readme</a><a href="manifest.json">Manifest</a><a href="generation-prompts.json">Generation prompts</a></nav>
<section class="viewer"><canvas id="preview" width="640" height="640" class="checker" aria-label="Animated Cao Cao voxel preview"></canvas><div class="controls"><h2>Animation viewer</h2><label>Action<select id="action">OPTIONS</select></label><label>Direction<select id="direction"><option value="south">南 / South</option><option value="west" selected>西 / West</option><option value="north">北 / North</option><option value="east">东 / East (mirrored)</option></select></label><label>Playback speed<select id="speed"><option value="1">1× native timing</option><option value="0.5">½×</option><option value="0.25">¼× slow motion</option></select></label><button id="play">Pause / 暂停</button><label>Frame<input id="scrub" type="range" min="0" value="0"></label><div id="frameName"></div><p class="note">Pose previews use the native frame order and waits. Translation, palette flashes, sound and shared spell effects are omitted. These are newly authored 2D voxel-style images; they have not yet been installed in the game.</p></div></section>
<h2>All 28 poses</h2><nav><a href="move-sheet.png">Movement atlas</a><a href="attack-sheet.png">Attack atlas</a><a href="special-sheet.png">Reaction atlas</a></nav><a href="all-frames-labeled.png"><img class="sheet" src="all-frames-labeled.png" alt="All 28 Cao Cao voxel frames"></a><details><summary>All 60 animation previews</summary><div class="grid">CARDS</div></details></main><script>
const timelines=DATA;const canvas=document.querySelector('#preview'),ctx=canvas.getContext('2d');const action=document.querySelector('#action'),direction=document.querySelector('#direction'),speed=document.querySelector('#speed'),scrub=document.querySelector('#scrub'),play=document.querySelector('#play'),name=document.querySelector('#frameName');const images=new Map();let loaded=false,playing=true,elapsed=0,last=0;
function current(){return timelines.find(t=>t.action===action.value&&t.direction===direction.value)}
function draw(index){let t=current(),e=t.frames[index],im=images.get(e.file);ctx.clearRect(0,0,640,640);if(im&&im.complete){ctx.save();ctx.translate(320,320);ctx.scale(e.mirror?-1:1,1);ctx.drawImage(im,-256,-256,512,512);ctx.restore()}scrub.value=index;name.textContent=`${e.group}-${String(e.index).padStart(2,'0')} · ${index+1}/${t.frames.length} · ${e.ms} ms${e.mirror?' · mirrored':''}`;}
function select(){elapsed=0;scrub.max=current().frames.length-1;draw(0)}
function tick(now){if(playing&&loaded){elapsed+=(last?Math.min(now-last,100):0)*Number(speed.value);let t=current(),position=elapsed%t.duration_ms,i=0;for(;i<t.frames.length-1&&position>=t.frames[i].ms;i++)position-=t.frames[i].ms;draw(i)}last=now;requestAnimationFrame(tick)}
action.value='attack';for(const el of [action,direction])el.addEventListener('change',select);play.addEventListener('click',()=>{playing=!playing;play.textContent=playing?'Pause / 暂停':'Play / 播放'});scrub.addEventListener('input',()=>{playing=false;play.textContent='Play / 播放';elapsed=current().frames.slice(0,Number(scrub.value)).reduce((a,e)=>a+e.ms,0);draw(Number(scrub.value))});
Promise.all([...new Set(timelines.flatMap(t=>t.frames.map(e=>e.file)))].map(file=>new Promise((resolve,reject)=>{const im=new Image();im.onload=resolve;im.onerror=()=>reject(new Error(file));images.set(file,im);im.src=file}))).then(()=>{loaded=true;select();requestAnimationFrame(tick)}).catch(e=>{name.textContent='Image loading failed: '+e.message});
</script></html>'''.replace('OPTIONS', options).replace('CARDS', cards).replace('DATA', data)
(OUT / 'index.html').write_text(page)
checks.update(native_timelines=len(timelines), source_images=7, generation_calls=10,
              source_alpha_preserved=True, game_files_changed=False,
              browser_gallery_open='Not tested: browser URL policy blocked local file:// navigation.',
              visual_inspection='Source sheets and labeled PNG overview inspected directly.')
assert len(timelines) == 60 and checks['preview_files'] == 120
checks['zip_integrity'] = True
(OUT / 'verification.json').write_text(json.dumps(checks, indent=2) + '\n')
archive = OUT.with_suffix('.zip')
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
    for p in sorted(OUT.rglob('*')):
        if p.is_file():
            z.write(p, p.relative_to(OUT.parent))
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    print(f'ZIP: {len(z.infolist())} entries, {archive.stat().st_size / 1048576:.2f} MiB')
print(json.dumps(checks, indent=2))
print(archive)
