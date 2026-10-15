#!/usr/bin/env python3
"""Read-only, edition-gated import of exact location-item events in S_00..03."""
import hashlib,io,json,sys
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
from eex import parse
from ls12 import decode
from executable_probe import EDITION
ROOT=Path(__file__).resolve().parents[1]
def canonical(v):return json.dumps(v,ensure_ascii=False,sort_keys=True,separators=(',',':')).encode()
def sha(v):return hashlib.sha256(v).hexdigest()
def locations(source):
 result=[]
 for n in range(4):
  p=source/f'S_{n:02}.eex';script=parse(p)
  for e in script['sections'][1]['events']:
   position=next((i for i in e['conditions'] if i['opcode']==37),None)
   if position is None:continue
   assert [i['opcode'] for i in e['conditions']]==[2,37,5]
   # S_03 carries parser-visible block-end/separator tokens between its two
   # actions. Accept precisely those structural tokens, not arbitrary opcodes.
   assert [i['opcode'] for i in e['body']] in ([61,11],[61,1,0,11])
   body=[i for i in e['body'] if i['opcode'] not in (0,1)]
   vals=lambda i:[a['value'] for a in i['args']]
   side,x,y=vals(position);yes,no=vals(e['conditions'][2]);item,level,animate,recipient=vals(body[0]);flag,value=vals(body[1])
   assert side==recipient==1025 and not yes and no==[flag] and value==1 and level==-1 and animate==1
   assert item in (87,88,92)
   result.append(dict(scenario=n,event=e['offset'],flag=flag,x=x,y=y,item=item,quantity=1,
     script=p.name,source_sha256=sha(p.read_bytes()),condition_offset=position['offset'],grant_offset=e['body'][0]['offset']))
 assert len(result)==8
 return result
def run(source):
 assert sha((source/'Ekd5.exe').read_bytes())==EDITION
 out=ROOT/'assets/terrain-benefits';out.mkdir(parents=True,exist_ok=True);entries=[]
 def write(path,data,origin):
  p=out/path;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data);entries.append(dict(path=path,sha256=sha(data),source=origin))
 sites=locations(source);data=json.loads((ROOT/'assets/generated/data.json').read_text())
 catalog=dict(locations=sites,rates=[25,20,20,15,20],items=[data['items'][i] for i in (87,88,92)])
 write('catalog.json',canonical(catalog),'S_00..03.eex;Data.e5;original recovery oracle')
 pal=json.loads((ROOT/'assets/generated/ui.json').read_text())['palette']
 raw=(source/'Item.e5').read_bytes()
 for b in decode(raw,indices={87,88,92}):
  assert len(b.data)==32*32
  im=Image.frombytes('P',(32,32),b.data);im.putpalette([c for rgb in pal for c in rgb]);buff=io.BytesIO();im.convert('RGB').save(buff,format='BMP')
  write(f'images/{b.index}.bmp',buff.getvalue(),f'Item.e5#{b.index};pixels={sha(b.data)};archive={sha(raw)}')
 write('audio/Se14.wav',(source/'Se14.wav').read_bytes(),'Se14.wav;original item-acquisition sound')
 chars={chr(int(c)) for c in json.loads((ROOT/'assets/expansion/font.json').read_text())['glyphs']}
 for p in (ROOT/'src').glob('prototype*.cpp'):chars.update(p.read_text())
 chars.update('地形恢复兵力获得已收集尚未收集本战道具仅收集使用暂未开放背包物资回合开始恢复最大兵力道具已领取')
 for i in catalog['items']:chars.update(i['name'])
 chars=sorted(chars-{'\r','\n','\t'});face=ImageFont.truetype(str(ROOT/'assets/generated/fonts/CaoCaoSong.ttf'),16)
 cols=64;cell=20;im=Image.new('RGB',(cols*cell,((len(chars)+cols-1)//cols)*cell),(247,0,255));draw=ImageDraw.Draw(im);draw.fontmode='1';glyphs={}
 for n,c in enumerate(chars):
  x=n%cols*cell;y=n//cols*cell;draw.text((x,y+16),c,font=face,fill='white',anchor='ls');glyphs[str(ord(c))]=dict(x=x,y=y,advance=round(face.getlength(c)))
 buff=io.BytesIO();im.save(buff,format='BMP');write('font.bmp',buff.getvalue(),'existing bundled Source Han Serif')
 write('font.json',canonical(dict(cell=20,size=16,glyphs=glyphs)),'existing bundled Source Han Serif')
 m=dict(schema=1,executable_sha256=EDITION,assets=entries);m['hash']=sha(canonical(m));(out/'manifest.json').write_bytes(canonical(m))
 print('Imported',len(sites),'original locations;',len(entries),'assets;',m['hash'])
if __name__=='__main__':run(Path(sys.argv[1]) if len(sys.argv)>1 else Path('/Users/henryzhang/Downloads/sgzccz'))
