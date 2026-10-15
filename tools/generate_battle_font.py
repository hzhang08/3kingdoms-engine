#!/usr/bin/env python3
"""Build a presentation-only, antialiased Chinese sans-serif coverage atlas.
Requires the pinned Pillow in requirements.txt. Does not change save identities.
"""
import hashlib,json
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'assets/presentation-font'
SOURCE=OUT/'NotoSansCJKsc-Bold.otf'
FONT_SHA='b5f0d1a190a7f9b43c310a8850630af12553df32c4c050543f9059732d9b4c0a'
LICENSE_SHA='6a73f9541c2de74158c0e7cf6b0a58ef774f5a780bf191f2d7ec9cc53efe2bf2'
sha=lambda b:hashlib.sha256(b).hexdigest()
assert sha(SOURCE.read_bytes())==FONT_SHA,'font differs from Noto Sans CJK SC Bold 2.004'
assert sha((OUT/'OFL.txt').read_bytes())==LICENSE_SHA,'font license changed'
raw=(ROOT/'assets/unit-details/font.json').read_bytes();metrics=json.loads(raw)
scale=3;cell=metrics['cell']*scale;glyphs=metrics['glyphs']
face=ImageFont.truetype(str(SOURCE),metrics['size']*scale)
width=max(g['x'] for g in glyphs.values())*scale+cell
height=max(g['y'] for g in glyphs.values())*scale+cell
assert max(width,height)<=4096,'atlas exceeds portable texture size'
atlas=Image.new('L',(width,height),0);notdef=bytes(face.getmask(chr(0)));fitted=[]
for key,g in glyphs.items():
 c=chr(int(key));assert bytes(face.getmask(c))!=notdef,f'missing glyph U+{int(key):04X}'
 left,top,right,bottom=face.getbbox(c,anchor='ls')
 baseline=min(metrics['size']*scale,cell-1-bottom)
 assert baseline+top>=0,f'glyph exceeds top of cell: {c}'
 # Render into a padded temporary image, then fit unusually wide Latin glyphs
 # to the established advances. All Chinese advances remain 16 logical pixels.
 glyph=Image.new('L',(max(cell,right-left+2),cell),0)
 ImageDraw.Draw(glyph).text((-min(0,left),baseline),c,font=face,anchor='ls',fill=255)
 box=glyph.getbbox()
 if box:
  ink=glyph.crop(box);available=max(1,g['advance']*scale-2)
  if box[2]>available:
   ink=ink.resize((available,ink.height),Image.Resampling.LANCZOS);x=0;fitted.append(key)
  else:x=box[0]
  assert ink.height+box[1]<cell and ink.width+x<=g['advance']*scale
  atlas.paste(ink,(g['x']*scale+x,g['y']*scale+box[1]))
atlas.save(OUT/'font-sans.bmp')
meta={'schema':1,'family':'Noto Sans CJK SC','weight':'Bold','version':'2.004',
 'source_url':'https://github.com/notofonts/noto-cjk/tree/Sans2.004',
 'source_sha256':FONT_SHA,'canonical_metrics_sha256':sha(raw),'raster_scale':scale,
 'glyph_count':len(glyphs),'size':[width,height],'coverage':'8-bit grayscale; white RGB with coverage alpha at runtime',
 'fitted_glyphs':fitted}
(OUT/'font-sans.json').write_text(json.dumps(meta,ensure_ascii=False,indent=2)+'\n')
entries=[{'path':p,'sha256':sha((OUT/p).read_bytes())} for p in ['font-sans.bmp','font-sans.json','OFL.txt','NOTICE.txt']]
(OUT/'manifest.json').write_text(json.dumps({'schema':1,'presentation_only':True,'assets':entries},indent=2)+'\n')
print(f'Generated {len(glyphs)} glyphs at {width}×{height}; all covered, advances unchanged, {len(fitted)} fitted glyphs.')
