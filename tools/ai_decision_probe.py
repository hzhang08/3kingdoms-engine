#!/usr/bin/env python3
"""Original AI decision up to movement entry; no movement/action/Windows hooks."""
import argparse,json
from pathlib import Path
from unicorn import UC_HOOK_CODE
from battle_events_probe import prepare,LIVE,READER
from initialization_probe import state,UNIT
from movement_probe import live_json
from ai_attack_probe import configure as attack_configure
from executable_probe import EDITION

class MovementBoundary(Exception):pass

def configure(v):
    attack_configure(v)
    v.ranges += [(0x40ddaf,0x40deb5),(0x409580,0x409596),
      (0x437e2e,0x438308),(0x43a4bf,0x43a956),(0x43ad1c,0x43b40c),
      (0x45f650,0x45f665),(0x4391dc,0x439276),(0x439516,0x439522),
      (0x442669,0x442688),(0x43b490,0x43b4b0)]
    # The original loader 0x41b84f consumes the first 68 of 73 disk records
    # using 0x40ddaf. It rearranges header bytes; raw memcpy is incorrect.
    v.write(0x826000,v.data[5]);p=0x826000
    for n in range(68):p=v.call(0x40ddaf,[p],0x4af4c0+70*n)
    assert p==0x826000+68*70

def generate(source):
    _,v,_,_=prepare(source);configure(v)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000)
    cases=[];trace=[]
    def stop(vm):raise MovementBoundary()
    v.hooks[0x43a956]=stop
    def observe(uc,address,size,user):
        if address==0x438308:
            trace.append(dict(kind='search',origin=list(v.read(v.arg(0),2)),budget=v.arg(1)&255,mode=v.arg(2)&255,target=[v.arg(3)&255,v.arg(4)&255],zoc=bool(v.arg(5))))
        elif address==0x43a529:trace.append(dict(kind='candidate',tile=list(v.read(v.arg(0),2))))
    v.uc.hook_add(UC_HOOK_CODE,observe)
    def reset():v.write(0x400000,original);v.write(0x800000,scratch);trace.clear()
    def capture(slot):
        w,h=v.read(0x4b422c,2);n=w*h
        roster=[live_json(v,s) for s in range(115)]
        records=[state(v,uid) for uid in sorted(set(b['id'] for b in roster))]
        b=roster[slot];before=v.read(UNIT,512*72),v.u32(0x48ecd4)
        reached=False
        try:v.call(0x43ad1c,this=LIVE+slot*36,budget=12000000)
        except MovementBoundary:reached=True
        assert before==(v.read(UNIT,512*72),v.u32(0x48ecd4))
        cases.append(dict(battle=b,unit=state(v,b['id']),roster=roster,records=records,
          width=w,height=h,terrain=list(v.read(0x818640,n)),priority_id=int.from_bytes(v.read(READER+0x3d06,2),'little'),
          reached_movement=reached,after=live_json(v,slot),trace=list(trace),
          distance=list(v.read(0x8192c0,n)),threat=list(v.read(0x824000,n)),scores=list(v.read(0x824640,n)),
          best_score=v.read(0x4b2c4c,1)[0],best_tile=list(v.read(0x4b2c20,2)),best_target=v.read(0x4b2c28,1)[0],
          best_tactic=v.read(0x4b2c44,1)[0],seek_flag=v.u32(0x4b2c1c),scratch=list(v.read(0x819900,256))))
    reset();capture(20)
    for slot in (18,19,37,38,39,40,41,42,43,44,45,46,47,48):reset();capture(slot)
    for slot in (18,20,37,48):
     for ai in range(5):
      for hp in (1,100):
       for flags in (0,128):
        reset();v.write(LIVE+slot*36+14,bytes([ai]));v.put32(LIVE+slot*36+16,hp);v.write(LIVE+slot*36+13,bytes([flags]));v.write(LIVE+slot*36+8,bytes([255,9,11]));capture(slot)
    for slot in (18,20,37,48):
     for target in (0,18,35,48):
      reset();v.write(LIVE+slot*36+14,b'\0');v.write(LIVE+slot*36+8,bytes([target]));capture(slot)
    for slot in (18,20,37,48):
     for status in (2,4,8,16):
      reset();v.write(LIVE+slot*36+30,bytes([status]));capture(slot)
    for slot in (35,36):
        reset();v.write(LIVE+slot*36+30,b'\x04');capture(slot) # Explicit sealed-tactics cases.
    return dict(executable_sha256=EDITION,tactics=[list(v.read(0x4af4c0+n*70,70)) for n in range(68)],cases=cases,
      boundaries='Original 0x43ad1c, preparation, path searches, routes, candidate and physical scoring. Only hook stops at 0x43a956 movement entry. Tactics loaded by original 0x40ddaf; nonempty tactic scoring remains outside permitted instruction ranges. No action executes; persistent units and RNG must remain unchanged.')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_ai_decision.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'original AI decisions');print({k:r['cases'][0][k] for k in ('best_tile','best_score','best_target','best_tactic','seek_flag','trace')})
