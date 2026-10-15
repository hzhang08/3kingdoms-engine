#!/usr/bin/env python3
"""Original single-target AI attack scoring (area attacks excluded explicitly)."""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE,READER
from initialization_probe import state,UNIT
from movement_probe import live_json
from physical_resolution_probe import configure as physical_configure
from executable_probe import EDITION

def configure(v):
    physical_configure(v)
    v.ranges += [(0x438965,0x438d96),(0x407818,0x407872),(0x406630,0x406649)]

def generate(source):
    _,v,_,_=prepare(source);configure(v)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    def reset():v.write(0x400000,original);v.write(0x800000,scratch)
    def capture(slot,center,priority):
        v.write(0x82e000,bytes(center));v.write(READER+0x3d06,priority.to_bytes(2,'little'))
        v.write(0x4b2c28,b'\xff');before=v.read(LIVE,115*36),v.read(UNIT,512*72),v.u32(0x48ecd4)
        result=v.call(0x438965,[0x82e000],LIVE+slot*36)&255
        assert before==(v.read(LIVE,115*36),v.read(UNIT,512*72),v.u32(0x48ecd4))
        roster=[live_json(v,n) for n in range(115) if v.read(LIVE+n*36+12,1)[0]==2]
        cases.append(dict(battle=live_json(v,slot),unit=state(v,v.u32(LIVE+slot*36)),roster=roster,records=[state(v,b['id']) for b in roster],center=center,priority_id=priority,width=20,height=20,terrain=list(v.read(0x818640,400)),score=result,target=v.read(0x4b2c30,1)[0]))
    for slot in (18,20,35,36,48):
     for y in range(7,20):
      for x in range(7,15):reset();capture(slot,(x,y),0)
    for job in range(53):
     if job in (16,17,43):continue # Those classes invoke unported area scoring.
     for hp in (1,5,20,100):
      for status in (0,8):
       for priority in (0,146,147):
        reset();v.write(UNIT+234*72+0x2b,bytes([job]));v.write(UNIT+234*72+0x2e,b'\xff'*9)
        for slot in range(35,115):
            if v.read(LIVE+slot*36+12,1)[0]!=2:continue
            v.put32(LIVE+slot*36+16,hp);v.write(LIVE+slot*36+30,bytes([status]))
        capture(20,(10,11),priority)
    return dict(executable_sha256=EDITION,cases=cases,boundaries='Original 0x438965/0x438a20 with actual range, physical estimate, terrain, class/equipment and counter-range code. No rule hooks or RNG/unit changes. Output is the selected target and score, not an executed action. Classes16/17/43 and effect43 area attacks are outside this fixture and must stop explicitly in the port.')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_ai_attack.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'original single-target AI attack scores')
