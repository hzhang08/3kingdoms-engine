#!/usr/bin/env python3
"""Read-only verified original item/tactic assets and a presentation-only font atlas.
Separate from save-bound manifests: refreshing UI never invalidates user progress.
"""
import argparse,io,json,re
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
from prototype_terrain_assets import ROOT,canonical,sha
out=ROOT/'assets/unit-details';out.mkdir(parents=True,exist_ok=True)
source=ROOT/'assets/generated';manifest=json.loads((source/'manifest.json').read_text());entries=[]
def write(path,raw,origin):
 p=out/path;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(raw);entries.append(dict(path=path,sha256=sha(raw),source=origin))
for e in manifest['assets']:
 if e['path']=='tactics.json' or e['path'].startswith('images/Item/'):
  raw=(source/e['path']).read_bytes();assert sha(raw)==e['sha256'];write(e['path'],raw,e)
assert (out/'tactics.json').exists()
assert sum(e['path'].startswith('images/Item/') for e in entries)==104
# Original property-page text and attack-range artwork. No DLL execution.
from combat_assets import decode_message
from import_assets import palette,EDITION
from ls12 import decode
import struct,pefile
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--game-dir',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));original=parser.parse_args().game_dir
exe=(original/'Ekd5.exe').read_bytes();assert sha(exe)==EDITION
messages=(original/'Imsg.e5').read_bytes();mapped=pefile.PE(data=exe).get_memory_mapped_image()
def table(address,count):
 result=[]
 for n in range(count):
  pointer=struct.unpack_from('<I',mapped,address-0x400000+n*4)[0]-0x400000
  result.append(mapped[pointer:pointer+256].split(b'\0')[0].decode('gbk'))
 return result
metadata=dict(items=[decode_message(messages,n) for n in range(104)],
 classes=[decode_message(messages,350+n) for n in range(53)],
 biographies=[decode_message(messages,450+n) for n in range(174)],
 grades=table(0x48be24,5),equipment_types=table(0x48bc20,9),equipment_effects=table(0x48bc50,45),
 sources={'Imsg.e5':sha(messages),'Ekd5.exe':sha(exe)},
 routines={'biography':'0x40bfe8','class':'0x40bfc6','base_attributes':'0x40709a'})
write('descriptions.json',canonical(metadata),'Imsg.e5 + verified PE pointer tables')
spal=palette(decode((original/'Spalet.e5').read_bytes(),indices={0})[0].data)
for block in decode((original/'Hitarea.e5').read_bytes()):
 assert len(block.data)==64*64
 im=Image.frombytes('P',(64,64),block.data);im.putpalette(spal)
 buf=io.BytesIO();im.convert('RGB').save(buf,format='BMP')
 write(f'images/Hitarea/{block.index:03}.bmp',buf.getvalue(),dict(file='Hitarea.e5',id=block.index,sha256=sha((original/'Hitarea.e5').read_bytes())))
# Dialog paint chain 0x4710c8 ->0x40a83b/0x40aa39. 0x40af77 replaces
# Mark+0x1abc0 with Logo.e5[8+theme]. The Electron reference uses theme3.
logo=(original/'Logo.e5').read_bytes();tile=decode(logo,indices={11})[0].data
assert len(tile)==48*48
colors=json.loads((source/'ui.json').read_text())['palette'];denominator=sum(colors[22])
pixels=bytes(min(255,240*sum(colors[pixel])//denominator) for pixel in tile)
buf=io.BytesIO();Image.frombytes('L',(48,48),pixels).convert('RGB').save(buf,format='BMP')
write('images/dialog-pattern.bmp',buf.getvalue(),dict(file='Logo.e5',id=11,sha256=sha(logo),palette='Spalet.e5#0',tint=240,routine='0x40aa39'))
chars={chr(int(c)) for c in json.loads((ROOT/'assets/items/font.json').read_text())['glyphs']}
for p in list((ROOT/'src').glob('prototype*'))+[ROOT/'src/unit_details.cpp']:
 if p.is_file():chars.update(p.read_text())
chars.update((source/'combat_text.json').read_text());chars.update((out/'descriptions.json').read_text());
for p in (ROOT/'assets/expansion').glob('S_*.json'):chars.update(p.read_text())
chars.update((source/'data.json').read_text());chars.update((out/'tactics.json').read_text())
chars=sorted(chars-{'\r','\n','\t'});face=ImageFont.truetype(str(source/'fonts/CaoCaoSong.ttf'),16)
cols=64;cell=20;im=Image.new('RGB',(cols*cell,((len(chars)+cols-1)//cols)*cell),(247,0,255));draw=ImageDraw.Draw(im);draw.fontmode='1';glyphs={}
for n,c in enumerate(chars):
 x=n%cols*cell;y=n//cols*cell;draw.text((x,y+16),c,font=face,fill='white',anchor='ls');glyphs[str(ord(c))]=dict(x=x,y=y,advance=round(face.getlength(c)))
buf=io.BytesIO();im.save(buf,format='BMP');write('font.bmp',buf.getvalue(),'bundled Source Han Serif');write('font.json',canonical(dict(cell=20,size=16,glyphs=glyphs)),'bundled Source Han Serif')
m=dict(schema=1,original_manifest=manifest['hash'],assets=entries);m['hash']=sha(canonical(m));(out/'manifest.json').write_bytes(canonical(m));print('Verified unit-detail assets:',len(entries),m['hash'])
