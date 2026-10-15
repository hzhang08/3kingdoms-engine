#!/usr/bin/env python3
"""Crop authored Guan Yu art and encode the native west-attack pose timeline.

No generation, background removal, repainting or game-data mutation here.
Run with .venv/bin/python tools/export_guanyu_voxel_attack.py.
"""
from pathlib import Path
import hashlib
import json
from PIL import Image, ImageDraw, ImageSequence

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'output/character-art/guanyu-voxel-sprites'
atlas = Image.open(OUT / 'source/attack-atlas-v2.png').convert('RGBA')
assert atlas.size == (1536, 1024)
# Rectangles follow the actual artwork gutters; the lower row is not a perfect grid.
specs = [
    ('move', 8, 'Ready', (0, 0, 512, 512), (238, 485), 285),
    ('attack', 8, 'Windup', (512, 0, 1024, 512), (753, 485), 285),
    ('attack', 9, 'Drive', (1024, 0, 1536, 512), (1254, 485), 285),
    ('attack', 10, 'Strike', (0, 512, 560, 1024), (270, 967), 277),
    ('attack', 11, 'Recovery', (560, 512, 1056, 1024), (754, 967), 277),
    ('move', 4, 'Idle', (1056, 512, 1536, 1024), (1255, 967), 277),
]
(OUT / 'frames').mkdir(exist_ok=True)
(OUT / 'source-frames').mkdir(exist_ok=True)
frames, metadata = {}, []
for group, index, label, rect, anchor, horse_height in specs:
    crop = atlas.crop(rect)
    source_name = f'source-frames/{group}-{index:02}.png'
    crop.save(OUT / source_name)
    assert Image.open(OUT / source_name).tobytes() == crop.tobytes()
    scale = 242 / horse_height
    scaled = crop.resize((round(crop.width * scale), round(crop.height * scale)), Image.Resampling.LANCZOS)
    origin = (round(256 - (anchor[0] - rect[0]) * scale), round(456 - (anchor[1] - rect[1]) * scale))
    assert origin[0] >= 0 and origin[1] >= 0
    content = scaled.getchannel('A').point(lambda a: 255 if a > 64 else 0).getbbox()
    assert content and origin[0] + content[0] > 4 and origin[1] + content[1] > 4
    assert origin[0] + content[2] < 508 and origin[1] + content[3] < 508
    frame = Image.new('RGBA', (512, 512))
    frame.alpha_composite(scaled, origin)
    bbox = frame.getchannel('A').point(lambda a: 255 if a > 64 else 0).getbbox()
    assert bbox and bbox[0] > 4 and bbox[1] > 4 and bbox[2] < 508 and bbox[3] < 508
    file = f'frames/{group}-{index:02}.png'
    frame.save(OUT / file)
    frames[group, index] = frame
    metadata.append(dict(group=group, index=index, label=label, file=file,
                         source_file='source/attack-atlas-v2.png', source_rect=rect,
                         source_crop=source_name, source_ground_anchor=anchor,
                         scale=scale, origin=origin, visible_alpha_bounds=bbox,
                         sha256=hashlib.sha256((OUT / file).read_bytes()).hexdigest()))

native = json.loads((OUT / 'reference/native-events.json').read_text())
animation = next(a for a in native['animations'] if a['name'] == 'attack' and a['direction'] == 3)
assert native['sprite'] == 125 and native['character_id'] == 3
archive_groups = {'UNIT_MOV.E5': 'move', 'UNIT_ATK.E5': 'attack'}
sequence = []
def append(pose, duration):
    group, index = archive_groups[pose['archive']], pose['frame']
    assert (group, index) in frames
    key = dict(file=f'frames/{group}-{index:02}.png', group=group, index=index,
               mirror=pose.get('mirror', False))
    if sequence and all(sequence[-1][k] == v for k, v in key.items()):
        sequence[-1]['ms'] += duration
    else:
        sequence.append(dict(**key, ms=duration))
current = animation['idle']
for event in animation['events']:
    if event['kind'] == 'frame' and event['slot'] == animation['subject_slot']:
        current = event
    elif event['kind'] == 'overlay' and event['slot'] == animation['subject_slot']:
        current = animation['idle']
    elif event['kind'] == 'wait':
        append(current, event['ticks'] * native['tick_ms'])
append(animation['idle'], 400)  # Same loop-end viewing pause as existing character GIFs.
durations = [e['ms'] for e in sequence]
assert durations == [80, 400, 40, 40, 440, 480]
assert len({m['sha256'] for m in metadata}) == 6

def checker(size):
    image = Image.new('RGBA', size, '#202b3a')
    draw = ImageDraw.Draw(image)
    for y in range(0, size[1], 16):
        for x in range(0, size[0], 16):
            if (x // 16 + y // 16) % 2 == 0:
                draw.rectangle((x, y, x + 15, y + 15), fill='#273444')
    return image

rgb, rgba = [], []
for entry in sequence:
    sprite = frames[entry['group'], entry['index']]
    assert not entry['mirror']
    canvas = checker((640, 640)); canvas.alpha_composite(sprite, (64, 64))
    rgb.append(canvas.convert('RGB'))
    transparent = Image.new('RGBA', (640, 640)); transparent.alpha_composite(sprite, (64, 64)); rgba.append(transparent)
strip = Image.new('RGB', (640, 640 * len(rgb)))
for i, frame in enumerate(rgb):
    strip.paste(frame, (0, i * 640))
palette = strip.quantize(colors=256, method=Image.Quantize.MEDIANCUT)
indexed = [frame.quantize(palette=palette, dither=Image.Dither.FLOYDSTEINBERG) for frame in rgb]
indexed[0].save(OUT / 'guanyu-attack-west.gif', save_all=True, append_images=indexed[1:],
                duration=durations, loop=0, disposal=2, optimize=False)
rgba[0].save(OUT / 'guanyu-attack-west.webp', save_all=True, append_images=rgba[1:],
             duration=durations, loop=0, lossless=True, method=4)
rgb[1].save(OUT / 'guanyu-attack-west-preview.png')
sheet = Image.new('RGBA', (1536, 1024))
contact = checker((1536, 1024))
for i, spec in enumerate(specs):
    frame = frames[spec[0], spec[1]]
    at = ((i % 3) * 512, (i // 3) * 512)
    sheet.paste(frame, at); contact.alpha_composite(frame, at)
sheet.save(OUT / 'attack-sheet.png')
contact.convert('RGB').save(OUT / 'attack-sheet-preview.png')
timeline = dict(action='attack', direction='west', frames=sequence, duration_ms=sum(durations),
                gif='guanyu-attack-west.gif', webp='guanyu-attack-west.webp')
manifest = dict(schema=1, character=dict(id=3, name='关羽', name_en='Guan Yu', sprite=125,
                class_id=native['class'], side=native['side']), format='2D voxel-style raster artwork',
                model='guanyu-model.png', model_dimensions=list(Image.open(OUT / 'guanyu-model.png').size),
                frames=metadata, timelines=[timeline], authoring='built-in imagegen',
                native_timing_source='reference/native-events.json', viewport=[640, 640],
                frame_canvas=[512, 512], foot_anchor=[256, 456], horse_height=242)
(OUT / 'manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2))
with Image.open(OUT / 'guanyu-attack-west.gif') as gif:
    encoded = [frame.info['duration'] for frame in ImageSequence.Iterator(gif)]
    assert gif.size == (640, 640) and gif.n_frames == 6 and gif.info['loop'] == 0 and encoded == durations
checks = dict(frames=6, distinct_frames=6, lossless_source_crops=6, border_clipping=False,
              native_character_id=3, native_sprite=125, native_pose_order=True,
              durations_ms=durations, loop_duration_ms=sum(durations), gif_dimensions=[640, 640],
              animation='in-place west attack', model_is_mesh=False, integrated_in_game=False)
(OUT / 'verification.json').write_text(json.dumps(checks, indent=2))
print(json.dumps(checks, indent=2))
