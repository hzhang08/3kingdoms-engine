#!/usr/bin/env python3
"""Pack generated camp artwork; keep gameplay/save asset identities unchanged."""
import hashlib, json, struct
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / 'assets/special-hulao/source/camps-v1'
TARGET = ROOT / 'assets/prototype/special-hulao/camps-v1'
TARGET.mkdir(parents=True, exist_ok=True)
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
background = Image.open(SOURCE / 'terrain-without-tents.png').convert('RGB')
assert background.size == (1536, 1024)
background.save(TARGET / 'terrain.bmp')
tent = Image.open(SOURCE / 'tent.png').convert('RGBA')
assert tent.getchannel('A').getextrema() == (0, 255)
# Standard runtime texture packing, after built-in image_gen authored the edit.
# Ignore near-transparent fringe when locating the sprite's content rectangle.
bounds = tent.getchannel('A').point(lambda a: 255 if a > 16 else 0).getbbox()
pivot = (tent.width * .53 - bounds[0], tent.height * .78 - bounds[1])
tent = tent.crop(bounds)
scale = min(1, 768 / max(tent.size))
tent = tent.resize((round(tent.width * scale), round(tent.height * scale)), Image.Resampling.LANCZOS)
(TARGET / 'tent.rgba').write_bytes(struct.pack('<II', *tent.size) + tent.tobytes())
manifest = {
    'version': 'hulao-camp-presentation-v1',
    'background': 'terrain.bmp',
    'tent': {'file': 'tent.rgba', 'w': tent.width, 'h': tent.height,
             'pivot': [round(pivot[0] * scale, 3), round(pivot[1] * scale, 3)],
             'render_width': 68, 'source_rect': list(bounds)},
    'sources': {p.name: sha(p) for p in sorted(SOURCE.glob('*.png'))},
    'assets': {name: sha(TARGET / name) for name in ('terrain.bmp', 'tent.rgba')},
    'note': 'Presentation only. Derive camps from healing terrain; preserve game map and save manifest.'
}
(TARGET / 'manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n')
print(json.dumps(manifest, ensure_ascii=False, indent=2))
