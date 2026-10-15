#!/usr/bin/env python3
"""Read-only native Cao Cao asset export. Run the companion event exporter first."""
import hashlib, html, json, math, shutil, struct, zipfile
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageOps

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'output/character-art/caocao-native-sprites'
ASSETS=ROOT/'assets/prototype'
OUT.mkdir(parents=True,exist_ok=True)
ui=json.loads((ASSETS/'ui.json').read_text());palette=ui['palette']
catalog={e['path']:e for e in json.loads((ROOT/'assets/generated/manifest.json').read_text())['assets']}
sha=lambda data:hashlib.sha256(data).hexdigest()
font=lambda n:ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf',n)
archives={'move':('Unit_mov',48,11),'attack':('Unit_atk',64,12),'special':('Unit_spc',48,5)}
labels={'move':['South A','South B','North A','North B','West A','West B','South / acted','North / acted','West / acted','Weak A','Weak B'],
        'attack':[f'{d} / {n}' for d in ['South','North','West'] for n in range(4)],
        'special':['South / evade','North / evade','West / evade','Hit reaction','Script pose 1']}
directions=['north','east','south','west'];sources=[];frames={};raw={};gallery=[]
def rgba(pixels,size):
    return Image.frombytes('RGBA',(size,size),bytes(v for p in pixels for v in (*palette[p],255 if p else 0)))
def checker(size):
    im=Image.new('RGBA',size,(37,47,62,255));d=ImageDraw.Draw(im)
    for y in range(0,size[1],12):
        for x in range(0,size[0],12):
            if (x//12+y//12)%2:d.rectangle((x,y,x+11,y+11),fill=(44,56,73,255))
    return im
def put(im,path):
    path.parent.mkdir(parents=True,exist_ok=True);im.save(path)
for sprite in range(108,111):
    folder=OUT/f'pixel/{sprite}';folder.mkdir(parents=True,exist_ok=True)
    for group,(archive,size,count) in archives.items():
        rel=f'raw/{archive}.e5/{sprite:03}.bin';data=(ASSETS/rel).read_bytes()
        assert len(data)==size*size*count and sha(data)==catalog[rel]['sha256']
        sources.append({**catalog[rel],'frame_size':[size,size],'frames':count})
        for n in range(count):
            pixels=data[n*size*size:(n+1)*size*size];raw[archive,sprite,n]=pixels
            im=rgba(pixels,size);frames[archive,sprite,n]=im;put(im,folder/f'frames/{group}-{n:02}.png')
        cols=4 if group=='attack' else 3 if group=='move' else 5
        sheet=Image.new('RGBA',(cols*size,math.ceil(count/cols)*size))
        for n in range(count):sheet.paste(frames[archive,sprite,n],((n%cols)*size,(n//cols)*size))
        put(sheet,folder/f'{group}-sheet.png')
        put(sheet.resize((sheet.width*4,sheet.height*4),Image.Resampling.NEAREST),folder/f'{group}-sheet-4x.png')
    sheet=Image.new('RGB',(1308,1500),(17,25,37));draw=ImageDraw.Draw(sheet)
    draw.text((28,20),f'CAO CAO  |  native pixel sprites  |  resource {sprite}',font=font(29),fill='#f5dfad')
    draw.text((28,62),'28 original frames. Transparent PNG files included. East-facing poses use mirrored west frames.',font=font(18),fill='#b8c8d8')
    y=106
    for group,(archive,size,count) in archives.items():
        draw.text((28,y),f'{group.upper()}  /  {count} frames  /  {size} x {size} px',font=font(24),fill='#f5dfad');y+=43
        for n in range(count):
            x=28+(n%6)*212;row=y+(n//6)*226
            tile=checker((192,192));im=frames[archive,sprite,n].resize((size*3,size*3),Image.Resampling.NEAREST)
            tile.alpha_composite(im,((192-im.width)//2,(192-im.height)//2));sheet.paste(tile.convert('RGB'),(x,row))
            draw.text((x,row+196),f'{n:02}  {labels[group][n]}',font=font(16),fill='#d7e3f2')
        y+=math.ceil(count/6)*226+12
    sheet=sheet.crop((0,0,1308,y+15));put(sheet,folder/'all-frames-labeled.png')

# Use event streams from the actual compiled C++ helpers; interpret only this
# character's sprite and waits, omitting the target, map, sound and spell FX.
events=json.loads((OUT/'native-events.json').read_text())
quantized=ui['combat_palette']['quantized'];lookup=ui['combat_palette']['lookup']
def effect_image(e,sprite):
    archive={'UNIT_MOV.E5':'Unit_mov','UNIT_ATK.E5':'Unit_atk','UNIT_SPC.E5':'Unit_spc'}[e['archive']]
    pixels=bytearray(raw[archive,sprite,e['frame']]);size=e['width']
    for effect in e.get('effects',[]):
        for n,p in enumerate(pixels):
            if p==0:continue
            rgb=[]
            for c in range(3):
                value=quantized[p*3+c]
                if effect[0]==0:value=min(15,value+(effect[1]&255))
                elif effect[0]==2:value=max(0,value-(effect[1]&255))
                else:
                    amount=effect[3 if c==0 else c]&255
                    value=min(15,value+((amount*2-14)&255)) if amount>7 else max(0,value-(15 if amount==0 else (7-amount)*2))
                rgb.append(value)
            pixels[n]=lookup[(rgb[0]<<8)+(rgb[1]<<4)+rgb[2]]
    im=rgba(pixels,size)
    return ImageOps.mirror(im) if e.get('mirror',False) else im
def scene(e,sprite):
    im=checker((160,160));p=effect_image(e,sprite)
    im.alpha_composite(p,(56+e['x']-192,56+e['y']-248))
    return im.convert('RGB').resize((480,480),Image.Resampling.NEAREST)
def gif(path,images,durations):
    path.parent.mkdir(parents=True,exist_ok=True)
    images[0].save(path,save_all=True,append_images=images[1:],duration=durations,loop=0,optimize=False,disposal=2)
for a in events['animations']:
    sprite=a['sprite'];pose=a['idle'];images=[];durations=[]
    for e in a['events']:
        kind=e['kind']
        if kind=='frame' and e['slot']==a['subject_slot']:pose=e
        elif kind=='overlay' and e['slot']==a['subject_slot']:pose=a['idle']
        elif kind=='wait':images.append(scene(pose,sprite));durations.append(e['ticks']*events['tick_ms'])
    if not images:images=[scene(pose,sprite)];durations=[400]
    # Pause between loops so the isolated action is easier to inspect.
    images.append(scene(a['idle'],sprite));durations.append(400)
    rel=f'pixel/{sprite}/animations/{a["name"]}-{directions[a["direction"]]}.gif'
    gif(OUT/rel,images,durations);gallery.append({'file':rel,'sprite':sprite,'action':a['name'],'direction':directions[a['direction']]})
for sprite in range(108,111):
    for direction in range(4):
        first=[2,4,0,4][direction];images=[]
        for n in (first,first+1):images.append(scene({'archive':'UNIT_MOV.E5','frame':n,'width':48,'x':192,'y':248,'mirror':direction==1},sprite))
        rel=f'pixel/{sprite}/animations/idle-{directions[direction]}.gif';gif(OUT/rel,images,[180,180]);gallery.append({'file':rel,'sprite':sprite,'action':'idle','direction':directions[direction]})

voxel=OUT/'voxel-special';voxel.mkdir(exist_ok=True)
shutil.copy2(ROOT/'assets/special-hulao/source/unit-0.png',voxel/'caocao-source-transparent.png')
data=(ASSETS/'special-hulao/unit-0.rgba').read_bytes();w,h=struct.unpack('<II',data[:8]);im=Image.frombytes('RGBA',(w,h),data[8:]);put(im,voxel/'caocao-runtime.png')
Image.open(ASSETS/'images/Face/000.bmp').save(OUT/'caocao-portrait.png')
manifest={'character':{'id':0,'name':'曹操'},'source_frame_count':84,'forms':[{'sprite':108,'class':'群雄','current_four_battle_form':True},{'sprite':109,'class':'英雄'},{'sprite':110,'class':'霸王'}], 'sources':sources,'palette_sha256':sha(json.dumps(palette,separators=(',',':')).encode()),'preview_tick_ms':40,'idle_frame_ms':180,'previews':gallery,'voxel':{'source_file':'voxel-special/caocao-source-transparent.png','source_sha256':sha((voxel/'caocao-source-transparent.png').read_bytes()),'runtime_size':[w,h],'authored_frames':1,'animation':'Runtime transforms: translation/bob, tilt/lunge and color modulation; no authored per-action voxel frame sheets.'}}
(OUT/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
(OUT/'README.txt').write_text('''CAO CAO — NATIVE SPRITE EXPORT

pixel/108: 群雄, Cao Cao's form in the current four-battle prototype.
pixel/109: 英雄, promotion form included in the native asset bundle.
pixel/110: 霸王, promotion form included in the native asset bundle.
Each form: 11 movement/idle frames (48x48), 12 attack frames (64x64),
5 special/reaction frames (48x48). Total 84 source frames, all extracted.

*-sheet.png: transparent atlases at original resolution, row-major order.
Movement: 3 columns; attack: 4 columns; special: 5 columns.
*-sheet-4x.png: nearest-neighbor enlargement, no invented detail.
all-frames-labeled.png: readable contact sheet; frame indexes are zero-based.
frames/: one transparent PNG per source frame, original dimensions/pixels.

animations/: isolated GIF previews from existing native C++ animation helpers.
Includes 4 directions for walking, attacks, critical attacks, received hits,
dodges, all scripted animation IDs 1-9, and idle (180 previews across 3 forms).
East-facing frames are horizontal mirrors, as used by the game.
GIFs use native 40ms waits and a 400ms end pause; idle switches every 180ms.
They omit the battlefield, other characters, audio, numbers and shared spell
particles. Script-ID previews include assets supported by helpers; this does
not mean every sequence occurs in the current four battles. GIF palette
quantization may affect preview colors; the lossless PNG frames are definitive.
Script poses are labeled by ID where a story/emotion name is not verified.

voxel-special/: the actual transparent HD source and 318x320 runtime cutout.
This version has ONE authored character image. Its movement and attack effects
are procedural transforms in src/prototype_voxel.inc, not separate frame art.
No additional voxel poses or artwork were generated for this export.

manifest.json retains original archive/block identifiers, source offsets,
hashes, frame dimensions, class mapping and preview inventory.
native-events.json is the actual C++ helper event export.
The original game, native builds and saves are not changed.

Reproduce from project root:
c++ -std=c++20 -Iinclude -Ithird_party tools/export_caocao_animation_events.cpp build-mac/libcaocao_core.a -o output/character-art/caocao-native-sprites/export-events
output/character-art/caocao-native-sprites/export-events assets/prototype output/character-art/caocao-native-sprites/native-events.json
.venv/bin/python tools/export_caocao_sprites.py
''')
parts=['<!doctype html><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>曹操 · Native sprite library</title><style>body{margin:0;padding:30px;background:#111925;color:#dae6f2;font:17px system-ui}h1,h2{color:#f5dfad}a{color:#93d1ff}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:16px}.card{background:#25313f;border-radius:12px;padding:12px}.card img{width:100%;image-rendering:pixelated}figure{margin:0}figcaption{padding:8px 0} .sheet{max-width:100%}nav{display:flex;gap:20px;flex-wrap:wrap}</style><h1>曹操 · Native sprite library</h1><p>84 original PNG frames · 3 class forms · 180 animation previews</p><nav><a href="#s108">群雄 · Current form</a><a href="#s109">英雄</a><a href="#s110">霸王</a><a href="#voxel">Voxel special</a><a href="README.txt">Export notes</a><a href="manifest.json">Manifest</a></nav>']
for sprite,name in [(108,'群雄 · Current form'),(109,'英雄 · Promotion'),(110,'霸王 · Promotion')]:
    parts.append(f'<h2 id="s{sprite}">{name} — resource {sprite}</h2><p><a href="pixel/{sprite}/all-frames-labeled.png">Full labeled sheet</a> · <a href="pixel/{sprite}/move-sheet.png">Movement PNG</a> · <a href="pixel/{sprite}/attack-sheet.png">Attack PNG</a> · <a href="pixel/{sprite}/special-sheet.png">Special PNG</a></p><img class="sheet" loading="lazy" src="pixel/{sprite}/all-frames-labeled.png"><details><summary>Show all 60 animation previews</summary><div class="grid">')
    for a in gallery:
        if a['sprite']==sprite:parts.append(f'<figure class="card"><img loading="lazy" src="{a["file"]}"><figcaption>{a["action"]} / {a["direction"]}</figcaption></figure>')
    parts.append('</div></details>')
parts.append('<h2 id="voxel">Voxel special battle</h2><p>One authored cutout; the game animates it through transforms. There is no separate voxel attack/walk frame sheet.</p><a href="voxel-special/caocao-source-transparent.png">HD transparent source</a> · <a href="voxel-special/caocao-runtime.png">Actual runtime PNG</a><p><img style="max-width:480px;width:100%" src="voxel-special/caocao-source-transparent.png"></p>')
(OUT/'index.html').write_text('\n'.join(parts))
checks={'original_frame_count':84,'forms':3,'preview_files':len(gallery),'source_hashes_verified':True,'png_pixel_roundtrips':0}
for (archive,sprite,n),im in frames.items():
    group=next(k for k,v in archives.items() if v[0]==archive);exported=Image.open(OUT/f'pixel/{sprite}/frames/{group}-{n:02}.png').convert('RGBA');assert im.tobytes()==exported.tobytes();checks['png_pixel_roundtrips']+=1
for a in gallery:
    with Image.open(OUT/a['file']) as im:assert im.size==(480,480) and im.n_frames>=1
(OUT/'verification.json').write_text(json.dumps(checks,indent=2)+'\n')
with zipfile.ZipFile(OUT.with_suffix('.zip'),'w',zipfile.ZIP_DEFLATED) as z:
    for p in sorted(OUT.rglob('*')):
        if p.is_file() and p.name!='export-events':z.write(p,p.relative_to(OUT.parent))
print(json.dumps(checks));print(OUT.with_suffix('.zip'))
