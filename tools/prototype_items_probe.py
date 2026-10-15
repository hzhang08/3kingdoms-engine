#!/usr/bin/env python3
"""Original manual consumable effects, stock and action commit.

Executes43dda8 ->41dc43 and HP/status setters. Target-picker input is supplied
at45533d; legal range is separately exercised through original436586. Animation
41c762 is replaced by its statically verified success-field write, without
replacing the healing/stock/status rules. Camera, numeric display and UI refresh
are captured/no-op host boundaries. No original pixels/timing claim.
"""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from initialization_probe import UNIT,state
from movement_probe import live_json
from range_probe import configure as range_configure
from executable_probe import EDITION
def generate(source):
 _,v,_,_=prepare(source);range_configure(v)
 v.ranges += [(0x43dda8,0x43dde3),(0x41dbbc,0x41dd19),(0x41c4e9,0x41c72a),
   (0x41ccdf,0x41d06c),(0x43c42e,0x43c548),(0x40d955,0x40d9ee),
   (0x442718,0x442744),(0x43f6cd,0x43f74f),(0x418da0,0x418dc5),
   (0x409540,0x409551),(0x4066e0,0x406700)]
 events=[];control=dict(target=0)
 def pick(vm):
  args=[vm.arg(n)&255 for n in range(4)];assert args[1:3]==[1,4]
  events.append(dict(kind='pick',args=args));vm.return_from_hook(control['target'],16)
 def animation(vm):
  events.append(dict(kind='animation',item=vm.read(vm.this(),1)[0],effect=vm.arg(0)&255));vm.put32(vm.this()+0x1c,1);vm.return_from_hook(pop=4)
 def numbers(vm):events.append(dict(kind='numbers',args=[vm.arg(n) for n in range(8)]));vm.return_from_hook()
 v.hooks.update({0x45533d:pick,0x41c762:animation,0x450984:numbers,
   0x455123:lambda vm:vm.return_from_hook(pop=12),0x479a01:lambda vm:vm.return_from_hook()})
 memory=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000)
 cases=[];ranges=[]
 for item in [87,88,92]:
  for maximum in [31,100,112,499]:
   for hp in [1,maximum-1,maximum]:
    for status in [0,16,24]:
     for target in [0,18,255]:
      v.write(0x400000,memory);v.write(0x800000,scratch);events.clear()
      v.write(LIVE,b'\0'*(115*36));v.write(0x818c80,b'\xff'*1600)
      for slot in [0,18]:
       uid=0 if slot==0 else 3;v.load_unit(uid);v.put32(UNIT+uid*72+0x1c,maximum);v.write(UNIT+uid*72+0x2e,b'\xff'*9)
       p=LIVE+slot*36;v.put32(p,uid);v.write(p+4,bytes([slot,0 if slot==0 else 1,10+(slot!=0),10]));v.write(p+12,b'\x02\x00\x07\x02');v.put32(p+16,hp);v.put32(p+20,10);v.write(p+30,bytes([status]));v.write(0x818c80+210+(slot!=0),bytes([slot]))
      v.write(0x4b09db,b'\0'*17);v.write(0x4b09db+item-87,b'\x02');control['target']=target
      rng=v.u32(0x48ecd4);before=[live_json(v,s) for s in [0,18]]
      v.call(0x43dda8,[item],LIVE)
      assert rng==v.u32(0x48ecd4)
      cases.append(dict(item=item,maximum=maximum,target=target,before=before,after=[live_json(v,s) for s in [0,18]],stock=v.read(0x4b09db+item-87,1)[0],events=list(events),rng=rng))
 # Dense board around actor: original friend+self range1, including dead/inactive exclusions.
 for slot in [18,35]:
  for life in [2,3]:
   for hp in [0,1]:
    v.write(0x400000,memory);v.write(0x800000,scratch);v.write(LIVE,b'\0'*(115*36));v.write(0x818c80,b'\xff'*1600)
    for n,(dx,dy) in enumerate([(0,0),(0,-1),(1,0),(0,1),(-1,0),(1,-1),(1,1),(-1,1),(-1,-1),(0,2)]):
     sid=0 if n==0 else (slot if n==1 else 20+n);p=LIVE+sid*36;v.put32(p,0);v.write(p+4,bytes([sid,2 if sid>=35 else 0,10+dx,10+dy]));v.write(p+12,bytes([life if n==1 else 2]));v.put32(p+16,hp if n==1 else 10)
     # Occupancy is rebuilt from lifecycle2 units in the game; do not invent a stale cell for a removed unit.
     if n!=1 or life==2:v.write(0x818c80+(10+dy)*20+10+dx,bytes([sid]))
    v.write(0x82e000,bytes([10,10]));v.write(0x4b2c28,b'\xff');v.call(0x436586,[0x82e000,1,1,0],LIVE)
    raw=v.read(0x4ac748,100);ranges.append(dict(actor=live_json(v,0),roster=[live_json(v,n) for n in range(115) if v.read(LIVE+n*36+12,1)[0]],targets=list(raw[:raw.index(255)])))
 return dict(executable_sha256=EDITION,boundaries=__doc__,cases=cases,ranges=ranges)
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_prototype_items.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['cases']),'manual item cases;',len(r['ranges']),'range cases')
