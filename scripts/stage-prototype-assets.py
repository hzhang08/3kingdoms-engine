#!/usr/bin/env python3
"""Stage only prototype resources, preserving the original catalog and pixels."""
import hashlib
import json
import re
import shutil
from pathlib import Path
import struct

root = Path(__file__).resolve().parent.parent
source, target = root / 'assets/generated', root / 'assets/prototype'
manifest = json.loads((source / 'manifest.json').read_text())
names = {'combat_text.json', 'data.json', 'terrain.json', 'font.json', 'ui.json', 'images/font.bmp', 'images/battlefield.bmp',
         'scripts/S_00.json', 'images/pe/Ekd5.exe-302-2052.bmp', 'images/Mark/cursor.bmp', 'images/Mark/range.bmp', 'images/Mark/fire.bmp'}
names.update('images/Mark/'+name+'.bmp' for name in ['hp', 'mp', 'hp-pattern', 'mp-pattern'])
names.update(f'images/pe/Cmdicon.dll-{n}-1041.bmp' for n in range(100,104))
# Prototype's fixed classes/colors, including red cavalry in legacy snapshots.
# Core playback tests check that every emitted archive/frame is staged.
for sprite in range(151):
    for archive in ['Unit_mov', 'Unit_atk', 'Unit_spc']:
        names.add(f'raw/{archive}.e5/{sprite:03}.bin')
for sound in [7, 30, 31, 32, 33, 34, 35, 36, 37]:
    names.add(f'audio/Se{sound:02}.wav')
data = json.loads((source / 'data.json').read_text())
for hero in range(len(data['heroes'])):
    names.add(f"images/Face/{data['heroes'][hero]['face']:03}.bmp")
entries = [e for e in manifest['assets'] if e['path'] in names or e['path'].startswith('images/Unit_mov/')]
assert names <= {e['path'] for e in entries}, 'missing required prototype asset'
expansion = root / 'assets/expansion'
terrain_benefits=root/'assets/terrain-benefits'
items=root/'assets/items'
unit_details=root/'assets/unit-details'
glyphs = json.loads(((unit_details if (unit_details/'font.json').exists() else items if (items/'font.json').exists() else terrain_benefits if (terrain_benefits/'font.json').exists() else expansion) / 'font.json').read_text())['glyphs']
ui = ''.join((root / p).read_text() for p in ['src/prototype.cpp', 'src/prototype_frontend.cpp', 'src/prototype_dialogue.cpp', 'src/prototype_tactics.cpp', 'src/prototype_terrain.cpp', 'src/prototype_items.cpp', 'src/unit_details.cpp', 'src/prototype_unit_window.inc', 'src/prototype_special_choice.inc', 'src/prototype_duel.cpp'])
strings = ''.join(re.findall(r'"((?:[^"\\]|\\.)*)"', ui))
strings += (source / 'scripts/S_00.json').read_text()
ui_data = json.loads((source / 'ui.json').read_text())
strings += ''.join(e['text'] for group in ['classes', 'terrain'] for e in ui_data[group])
strings += ''.join(e['text'] for e in ui_data['labels'].values())
strings += ''.join(e['text'] for e in ui_data['command_menu']['controls'])
strings += ''.join(data['heroes'][hero]['name'] for hero in [0, 32, 3, 33, 210, 211, 258, 259, 146, 147, 474, 475, 476, 477, 478, 479, 480, 481, 482, 483, 484, 485])
missing = sorted({c for c in strings if ord(c) > 127 and str(ord(c)) not in glyphs})
assert not missing, f'missing prototype UI glyphs: {missing}'
for entry in entries:
    relative = Path(entry['path'])
    assert not relative.is_absolute() and '..' not in relative.parts
    raw = (source / relative).read_bytes()
    assert hashlib.sha256(raw).hexdigest() == entry['sha256'], f'asset changed: {relative}'
    dest = target / relative
    dest.parent.mkdir(parents=True, exist_ok=True)
    if not dest.exists() or dest.read_bytes() != raw:
        shutil.copy2(source / relative, dest)
# Original idle confusion marker: Cerberus 0x43eb1f reads Mark+0x1a2c0,
# draws 16x16 at tile X+4 or X+28. Keep the original catalog/save hash stable.
entry = next(e for e in manifest['assets'] if e['path'] == 'raw/Mark.e5/000.bin')
raw = (source / entry['path']).read_bytes()
assert hashlib.sha256(raw).hexdigest() == entry['sha256']
pixels = raw[0x1a2c0:0x1a3c0]
relative = 'images/Mark/confusion.bmp'
# Standard uncompressed RGB BMP; keep staging dependency-free for Emscripten's Python.
rgb = bytes(channel for y in reversed(range(16)) for x in range(16)
            for channel in reversed(ui_data['palette'][pixels[y*16+x]]))
bmp = struct.pack('<2sIHHI', b'BM', 54+len(rgb), 0, 0, 54)
bmp += struct.pack('<IiiHHIIiiII', 40, 16, 16, 1, 24, 0, len(rgb), 3780, 3780, 0, 0)
(target / relative).write_bytes(bmp+rgb)
entries.append({'path': relative, 'sha256': hashlib.sha256((target/relative).read_bytes()).hexdigest(),
                'source': 'Mark.e5@0x1a2c0', 'width': 16, 'height': 16,
                'index_pixels_sha256': hashlib.sha256(pixels).hexdigest(),
                'source_asset_sha256': entry['sha256']})
for name in ['manifest.json']:
    shutil.copy2(source / name, target / name)
extra = json.loads((expansion/'manifest.json').read_text())
identity = extra.pop('hash')
assert hashlib.sha256(json.dumps(extra, ensure_ascii=False, sort_keys=True, separators=(',', ':')).encode()).hexdigest() == identity
for e in extra['assets'] + [{'path': 'manifest.json', 'sha256': hashlib.sha256((expansion/'manifest.json').read_bytes()).hexdigest()}]:
    path = Path(e['path'])
    assert not path.is_absolute() and '..' not in path.parts
    raw = (expansion/path).read_bytes()
    assert hashlib.sha256(raw).hexdigest() == e['sha256']
    dest = target/'expansion'/path
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_bytes(raw)
    entries.append(dict(e, path='expansion/'+e['path']))
package = {'schema' : 1, 'mode': 'opening-prototype', 'original_manifest': manifest['hash'], 'assets': entries}
(target / 'prototype-package.json').write_text(json.dumps(package, ensure_ascii=False, sort_keys=True, indent=2) + '\n')
print(f'Verified {len(entries)} prototype assets and all UI glyphs; original catalog preserved.')
# A separate identity keeps historical physical-only snapshots valid.
magic=root/'assets/tactics'
if (magic/'manifest.json').exists():
 m=json.loads((magic/'manifest.json').read_text());identity=m.pop('hash')
 assert hashlib.sha256(json.dumps(m,ensure_ascii=False,sort_keys=True,separators=(',',':')).encode()).hexdigest()==identity
 for e in m['assets']+[dict(path='manifest.json',sha256=hashlib.sha256((magic/'manifest.json').read_bytes()).hexdigest())]:
  path=Path(e['path']);assert not path.is_absolute() and '..' not in path.parts
  raw=(magic/path).read_bytes();assert hashlib.sha256(raw).hexdigest()==e['sha256']
  dest=target/'tactics'/path;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(raw)
  entries.append(dict(e,path='tactics/'+str(path)))
 catalog=json.loads((magic/'catalog.json').read_text())
 assert all(str(ord(c)) in glyphs for t in catalog['tactics'] for c in t['name']), 'missing spell glyph'
 package['assets']=entries;(target/'prototype-package.json').write_text(json.dumps(package,ensure_ascii=False,sort_keys=True,indent=2)+'\n')
 print('Verified tactic assets; total',len(entries))

if (terrain_benefits/'manifest.json').exists():
 m=json.loads((terrain_benefits/'manifest.json').read_text());identity=m.pop('hash')
 assert hashlib.sha256(json.dumps(m,ensure_ascii=False,sort_keys=True,separators=(',',':')).encode()).hexdigest()==identity
 for e in m['assets']+[dict(path='manifest.json',sha256=hashlib.sha256((terrain_benefits/'manifest.json').read_bytes()).hexdigest())]:
  path=Path(e['path']);assert not path.is_absolute() and '..' not in path.parts
  raw=(terrain_benefits/path).read_bytes();assert hashlib.sha256(raw).hexdigest()==e['sha256']
  dest=target/'terrain-benefits'/path;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(raw)
  entries.append(dict(e,path='terrain-benefits/'+str(path)))
 package['assets']=entries;(target/'prototype-package.json').write_text(json.dumps(package,ensure_ascii=False,sort_keys=True,indent=2)+'\n')
 print('Verified terrain-benefit assets; total',len(entries))

if (items/'manifest.json').exists():
 m=json.loads((items/'manifest.json').read_text());identity=m.pop('hash')
 assert hashlib.sha256(json.dumps(m,ensure_ascii=False,sort_keys=True,separators=(',',':')).encode()).hexdigest()==identity
 for e in m['assets']+[dict(path='manifest.json',sha256=hashlib.sha256((items/'manifest.json').read_bytes()).hexdigest())]:
  path=Path(e['path']);assert not path.is_absolute() and '..' not in path.parts
  raw=(items/path).read_bytes();assert hashlib.sha256(raw).hexdigest()==e['sha256']
  dest=target/'items'/path;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(raw)
  entries.append(dict(e,path='items/'+str(path)))
 package['assets']=entries;(target/'prototype-package.json').write_text(json.dumps(package,ensure_ascii=False,sort_keys=True,indent=2)+'\n')
 print('Verified item assets; total',len(entries))

if (unit_details/'manifest.json').exists():
 m=json.loads((unit_details/'manifest.json').read_text());identity=m.pop('hash')
 assert hashlib.sha256(json.dumps(m,ensure_ascii=False,sort_keys=True,separators=(',',':')).encode()).hexdigest()==identity
 for e in m['assets']+[dict(path='manifest.json',sha256=hashlib.sha256((unit_details/'manifest.json').read_bytes()).hexdigest())]:
  path=Path(e['path']);assert not path.is_absolute() and '..' not in path.parts
  raw=(unit_details/path).read_bytes();assert hashlib.sha256(raw).hexdigest()==e['sha256']
  dest=target/'unit-details'/path;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(raw)
  entries.append(dict(e,path='unit-details/'+str(path)))
 package['assets']=entries;(target/'prototype-package.json').write_text(json.dumps(package,ensure_ascii=False,sort_keys=True,indent=2)+'\n')
 print('Verified unit-detail assets; total',len(entries))

magic=root/'assets/tactics-extended'
if (magic/'manifest.json').exists():
 m=json.loads((magic/'manifest.json').read_text());identity=m.pop('hash')
 assert hashlib.sha256(json.dumps(m,ensure_ascii=False,sort_keys=True,separators=(',',':')).encode()).hexdigest()==identity
 for e in m['assets']+[dict(path='manifest.json',sha256=hashlib.sha256((magic/'manifest.json').read_bytes()).hexdigest())]:
  path=Path(e['path']);assert not path.is_absolute() and '..' not in path.parts
  raw=(magic/path).read_bytes();assert hashlib.sha256(raw).hexdigest()==e['sha256']
  dest=target/'tactics-extended'/path;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(raw)
  entries.append(dict(e,path='tactics-extended/'+str(path)))
 package['assets']=entries;(target/'prototype-package.json').write_text(json.dumps(package,ensure_ascii=False,sort_keys=True,indent=2)+'\n')
 print('Verified extended tactic assets; total',len(entries))

# Browser typography is presentation-only: never change a snapshot identity.
presentation=root/'assets/presentation-font'
meta=json.loads((presentation/'font-sans.json').read_text())
assert meta['canonical_metrics_sha256']==hashlib.sha256((unit_details/'font.json').read_bytes()).hexdigest(), 'regenerate the presentation font for updated glyph metrics'
for e in json.loads((presentation/'manifest.json').read_text())['assets']:
 path=Path(e['path']);assert not path.is_absolute() and '..' not in path.parts
 raw=(presentation/path).read_bytes();assert hashlib.sha256(raw).hexdigest()==e['sha256']
 dest=target/'presentation-font'/path;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(raw)
 entries.append(dict(e,path='presentation-font/'+str(path)))
package['assets']=entries;(target/'prototype-package.json').write_text(json.dumps(package,ensure_ascii=False,sort_keys=True,indent=2)+'\n')
print('Verified presentation-only sans-serif font; canonical save identities unchanged.')

# Presentation translations do not participate in canonical snapshot identities.
(target/"localization").mkdir(exist_ok=True)
shutil.copyfile(root/"assets/localization/en.json",target/"localization/en.json")
