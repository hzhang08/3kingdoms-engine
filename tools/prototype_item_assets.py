#!/usr/bin/env python3
"""Edition-gated consumable sound/font supplement; never rewrites older manifests."""
import io,json,sys
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
from prototype_terrain_assets import ROOT,canonical,sha,EDITION

def run(source):
 assert sha((source/'Ekd5.exe').read_bytes())==EDITION
 out=ROOT/'assets/items';out.mkdir(parents=True,exist_ok=True);entries=[]
 def write(path,data,origin):
  p=out/path;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data);entries.append(dict(path=path,sha256=sha(data),source=origin))
 write('audio/Se41.wav',(source/'Se41.wav').read_bytes(),'Se41.wav;0x41c762 original item-use sound')
 chars={chr(int(c)) for c in json.loads((ROOT/'assets/terrain-benefits/font.json').read_text())['glyphs']}
 for p in (ROOT/'src').glob('prototype*.cpp'):chars.update(p.read_text())
 chars=sorted(chars-{'\r','\n','\t'});face=ImageFont.truetype(str(ROOT/'assets/generated/fonts/CaoCaoSong.ttf'),16)
 cols=64;cell=20;im=Image.new('RGB',(cols*cell,((len(chars)+cols-1)//cols)*cell),(247,0,255));draw=ImageDraw.Draw(im);draw.fontmode='1';glyphs={}
 for n,c in enumerate(chars):
  x=n%cols*cell;y=n//cols*cell;draw.text((x,y+16),c,font=face,fill='white',anchor='ls');glyphs[str(ord(c))]=dict(x=x,y=y,advance=round(face.getlength(c)))
 buff=io.BytesIO();im.save(buff,format='BMP');write('font.bmp',buff.getvalue(),'bundled Source Han Serif; separate atlas preserves schema7 assets')
 write('font.json',canonical(dict(cell=20,size=16,glyphs=glyphs)),'bundled Source Han Serif')
 m=dict(schema=1,executable_sha256=EDITION,assets=entries);m['hash']=sha(canonical(m));(out/'manifest.json').write_bytes(canonical(m));print('Item supplement:',m['hash'])
if __name__=='__main__':run(Path(sys.argv[1]) if len(sys.argv)>1 else Path('/Users/henryzhang/Downloads/sgzccz'))
