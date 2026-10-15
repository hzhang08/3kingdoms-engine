#!/usr/bin/env python3
"""Read-only import of three standalone prototype battles; original catalog stays intact."""
import argparse, hashlib, json, shutil, struct
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from eex import parse
from ls12 import decode
from import_assets import EDITION, canonical, untile

ROOT=Path(__file__).resolve().parents[1]
PARTY=[0,1,5,6,10,11,12]
NAMES={1:'汜水关之战',2:'虎牢关之战',3:'董卓追击战'}
def sha(b):return hashlib.sha256(b).hexdigest()
def walk(body):
 for i in body:
  yield i
  if 'body' in i:yield from walk(i['body'])
def original_deployment(source,n,script):
 from deployment_probe import prepare_script,script_call,slots
 v=prepare_script(source)
 for uid in PARTY:v.call(0x40c28c,[uid,0,2])
 # Explicit test input: a fixed seven-character prototype party, not a recovered save.
 v.write(0x48b350,struct.pack('<15I',*PARTY,*([65535]*8)))
 raw=(source/f'S_{n:02}.eex').read_bytes()
 handlers={68:0x412230,69:0x4122f1,70:0x4125c5,71:0x4129a4,75:0x415e31}
 executed=[]
 for i in walk(script['sections'][0]['events'][0]['body']):
  if i['opcode'] in handlers:
   script_call(v,handlers[i['opcode']],raw[i['offset']:i['end']]);executed.append({'opcode':i['opcode'],'offset':i['offset']})
  if i['opcode']==75 and i['args'][0]['value']==14:break
 return {'executable_sha256':EDITION,'scenario':n,'party':PARTY,'selected_ids_va':'0x48b350',
  'method':'bounded original x86 handlers; declared fixed party; section0 deployment only',
  'hooks':{'0x41b51e':'validated Data.e5 archive read','0x41a1b0':'collection write captured without host I/O'},
  'instructions':executed,'rows':slots(v)}
def run(source,verify):
 generated=ROOT/'assets/generated';out=ROOT/'assets/expansion'
 if sha((source/'Ekd5.exe').read_bytes())!=EDITION:raise ValueError('unsupported executable edition')
 out.mkdir(parents=True,exist_ok=True);assets=[];sources=[]
 base=json.loads((generated/'manifest.json').read_text())
 data=json.loads((generated/'data.json').read_text());ui=json.loads((generated/'ui.json').read_text())
 def record_source(name):
  b=(source/name).read_bytes();sources.append({'path':name,'sha256':sha(b)});return b
 def write(name,b,origin):
  p=out/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b);assets.append({'path':name,'sha256':sha(b),'source':origin})
 def bitmap(name,pixels,w,h):
  im=Image.frombytes('P',(w,h),pixels);im.putpalette([x for c in ui['palette'] for x in c]);im=im.convert('RGB')
  import io
  b=io.BytesIO();im.save(b,format='BMP');write(name,b.getvalue(),name.replace('battlefield.bmp','Hm.e5'))
 terrain={b.index:b for b in decode(record_source('Hexzmap.e5'),indices=set(NAMES))}
 catalog=[];chars=set('战场选择下一战选择战场每场独立开始不影响已有存档伏兵开场显示所有友军可操控重新开始此战返回当前战场击退李儒消灭全部敌军')
 for n,name in NAMES.items():
  script=parse(source/f'S_{n:02}.eex');record_source(f'S_{n:02}.eex')
  write(f'S_{n:02}.json',canonical(script),f'S_{n:02}.eex')
  items=list(walk(script['sections'][0]['events'][0]['body']));roster=[]
  for i in items:
   vals=[a['value'] for a in i['args']]
   if i['opcode'] in (70,71):
    enemy=i['opcode']==71;k=12 if enemy else 11
    for slot in range(len(vals)//k):
     row=vals[slot*k:(slot+1)*k]
     if row[0]<0:continue
     extra=int(enemy)
     roster.append({'id':row[0],'slot':slot+(35 if enemy else 15),'side':2 if enemy else 1,
       'x':row[2+extra],'y':row[3+extra],'direction':row[4+extra],'hidden_original':row[2 if enemy else 1]>0,
       'ai_original':row[7+extra],'source_offset':i['offset'],'row':slot})
   if i['opcode']==75 and vals[0]<len(PARTY):
    roster.append({'id':PARTY[vals[0]],'slot':vals[0],'side':0,'x':vals[1],'y':vals[2],'direction':vals[3],
     'hidden_original':False,'ai_original':7,'source_offset':i['offset'],'row':0})
  roster.sort(key=lambda x:x['slot']);assert len({r['id'] for r in roster})==len(roster)
  if verify:
   oracle=original_deployment(source,n,script)
   for r in roster:
    o=next(x for x in oracle['rows'] if x['slot']==r['slot'])
    assert [o['id'],o['x'],o['y']]==[r['id'],r['x'],r['y']]
    assert o['flags']&(32,8,64,16)[r['direction']]
    assert bool(o['flags']&4)==r['hidden_original']
   (ROOT/f'docs/evidence/scenarios/S_{n:02}-deployment.json').write_bytes(canonical(oracle))
  raw=terrain[n].data;w,h=raw[0]//3,raw[1]//3;assert len(raw)==w*h+2
  cells=list(raw[2:]);write(f'terrain-{n}.json',canonical({'width':w,'height':h,'cells':cells}),f'Hexzmap.e5#{n}')
  pixels=untile(record_source(f'Hm{n:02}.e5'),w*48,h*48,48);bitmap(f'battlefield-{n}.bmp',pixels,w*48,h*48)
  assets[-1].update(source=f'Hm{n:02}.e5;Spalet.e5#0',index_pixels_sha256=sha(pixels),width=w*48,height=h*48)
  intro=[i['offset'] for i in items if i['opcode'] in (20,22)]
  death=[];encounters=[];victory=[];defeat=[]
  for e in script['sections'][1]['events']:
   ops=[i['opcode'] for i in e['conditions']]
   direct=[i['offset'] for i in e['body'] if i['opcode'] in (20,22)]
   if not direct:continue
   if 54 in ops:death.append({'event_offset':e['offset'],'unit':next(i['args'][0]['value'] for i in e['conditions'] if i['opcode']==54),'offsets':direct})
   if 46 in ops:
    a=next([a['value'] for a in i['args']] for i in e['conditions'] if i['opcode']==46)
    # Only verified non-duel, concrete-id adjacency. Duels have extra effects.
    if a[2]==1 and all(0<=u<512 for u in a[:2]):encounters.append({'event_offset':e['offset'],'units':a[:2],'offsets':direct})
   if 66 in ops:victory=direct
   if 67 in ops:defeat=direct
  # Li Ru's original defeat event already carries the pursuit victory narration.
  if n==3:victory=[]
  config={'id':n,'name':name,'width':w,'height':h,'turn_limit':20,'start_turn':1,'friendly_level':20 if n==3 else 10,'enemy_level':1,'victory_target':132 if n==3 else -1,
    'objective':'击退李儒' if n==3 else '消灭关外敌军' if n==2 else '消灭全部敌军',
    'victory_region':[7,0,19,19] if n==2 else [],'victory_region_source_offset':13966 if n==2 else None,'roster':roster,'intro':intro,'deaths':death,'encounters':encounters,
    'victory':victory,'defeat':defeat,'script':f'S_{n:02}.json','terrain':f'terrain-{n}.json','image':f'battlefield-{n}.bmp',
    'adaptations':['fixed party preset; independent battle, no campaign progression','all friendlies player-controlled','hidden units visible from start; no scripted ambush/duel effects','fixed prototype levels, simple AI and single physical strikes']}
  catalog.append(config);chars.update(name);chars.update(canonical(script).decode())
  print(n,name,f'{w}x{h}',len(roster),'units',len(intro),'intro instructions',len(death),'retreat events')
 write('scenarios.json',canonical(catalog),'S_01..03.eex; original deployment handlers; declared prototype party')
 for p in (ROOT/'src').glob('prototype*.cpp'):chars.update(p.read_text())
 chars.update(''.join(x['name'] for x in data['heroes']));chars.update(chr(i) for i in range(32,127))
 chars.update(chr(int(cp)) for cp in json.loads((generated/'font.json').read_text())['glyphs'])
 chars=sorted(chars-{'\n','\r','\t'});fontpath=generated/'fonts/CaoCaoSong.ttf';face=ImageFont.truetype(str(fontpath),16)
 cols=64;cell=20;im=Image.new('RGB',(cols*cell,((len(chars)+cols-1)//cols)*cell),(247,0,255));draw=ImageDraw.Draw(im);draw.fontmode='1';glyphs={}
 for j,ch in enumerate(chars):
  x=j%cols*cell;y=j//cols*cell;draw.text((x,y+16),ch,font=face,fill='white',anchor='ls');glyphs[str(ord(ch))]={'x':x,'y':y,'advance':round(face.getlength(ch))}
 import io
 b=io.BytesIO();im.save(b,format='BMP');write('font.bmp',b.getvalue(),'Source Han Serif, existing bundled font')
 write('font.json',canonical({'cell':20,'size':16,'glyphs':glyphs}),'Source Han Serif, existing bundled font')
 manifest={'schema':1,'version':'prototype-battles-1','executable_sha256':EDITION,'original_manifest':base['hash'],'font_sha256':sha(fontpath.read_bytes()),'sources':sources,'assets':assets}
 manifest['hash']=sha(canonical(manifest));(out/'manifest.json').write_bytes(canonical(manifest))
 for r in sources:assert sha((source/r['path']).read_bytes())==r['sha256']
 print('expansion manifest',manifest['hash'])
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));ap.add_argument('--verify-original',action='store_true');a=ap.parse_args();run(a.source,a.verify_original)
