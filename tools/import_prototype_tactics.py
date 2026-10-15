#!/usr/bin/env python3
"""Read-only import of this edition's five basic offensive tactic effects."""
import hashlib,json,struct,sys
from pathlib import Path
import pefile
from ls12 import decode
from executable_probe import EDITION
root=Path(__file__).resolve().parent.parent
source=Path(sys.argv[1]) if len(sys.argv)>1 else Path('/Users/henryzhang/Downloads/sgzccz')
out=root/'assets/tactics';out.mkdir(parents=True,exist_ok=True)
sha=lambda b:hashlib.sha256(b).hexdigest()
exe=(source/'Ekd5.exe').read_bytes();assert sha(exe)==EDITION
mapping=pefile.PE(data=exe).get_data(0x866c0,68);ids=[0,5,10,15,33]
raw=(source/'Meff.e5').read_bytes();resources={b.index:b.data for b in decode(raw,indices={mapping[t] for t in ids})}
records=json.loads((root/'assets/generated/tactics.json').read_text());palette=json.loads((root/'assets/generated/ui.json').read_text())['palette'];entries=[];meta={}
def write(path,data,origin):
 p=out/path;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data);entries.append(dict(path=path,sha256=sha(data),source=origin))
for t in ids:
 rid=mapping[t];b=resources[rid];n,images,masks,palettes,ms,w,h,sound=struct.unpack_from('<4B4I',b)
 assert masks==palettes==0 and len(b)==20+n*20+images*w*h and ms in [50,100]
 frames=[]
 for k in range(n):
  image,pal,weight,amount,masked,dx,dy,cb=struct.unpack_from('<4BIiiB',b,20+k*20)
  assert pal==255 and masked==0 and image<images and cb in (0,1)
  frames.append(dict(image=image,weight=weight,amount=amount,dx=dx,dy=dy,callback=cb))
 for i in range(images):
  pixels=b[20+n*20+i*w*h:20+n*20+(i+1)*w*h];rows=[]
  for y in reversed(range(h)):
   row=bytes(c for index in pixels[y*w:(y+1)*w] for c in reversed(palette[index]));rows.append(row+b'\0'*((-len(row))%4))
  rgb=b''.join(rows);bmp=struct.pack('<2sIHHI',b'BM',54+len(rgb),0,0,54)+struct.pack('<IiiHHIIiiII',40,w,h,1,24,0,len(rgb),3780,3780,0,0)+rgb
  write(f'images/{rid}-{i}.bmp',bmp,f'Meff.e5#{rid}:image{i};source_sha256={sha(raw)};block_sha256={sha(b)}')
 meta[str(t)]=dict(resource=rid,width=w,height=h,ticks=ms//50,sound=sound+100,frames=frames)
for sound in [8,39,100,102,104,106,119]:
 name=f'Se{sound:02}.wav' if sound<100 else f'Se_m_{sound-100:02}.wav';write('audio/'+name,(source/name).read_bytes(),name)
write('catalog.json',json.dumps(dict(tactics=[r for r in records if r['id'] in ids],effects=meta),ensure_ascii=False,sort_keys=True,separators=(',',':')).encode(),'Data.e5#5;Ekd5.exe@0x4866c0;Meff.e5')
m=dict(schema=1,executable_sha256=EDITION,assets=entries);m['hash']=sha(json.dumps(m,sort_keys=True,ensure_ascii=False,separators=(',',':')).encode());(out/'manifest.json').write_text(json.dumps(m,ensure_ascii=False,indent=2)+'\n');print('Imported',len(entries),'tactic assets',m['hash'])
