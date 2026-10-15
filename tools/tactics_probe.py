#!/usr/bin/env python3
"""Original tactic loader and mode-4 availability; no tactic effect executes."""
import argparse,json
from pathlib import Path
from ai_decision_probe import configure
from battle_events_probe import prepare,LIVE
from initialization_probe import UNIT
from executable_probe import EDITION
from tactics import decode_tactics

def generate(source):
    _,v,_,_=prepare(source);configure(v)
    records=decode_tactics(v.data[5])
    for r in records:assert bytes(r['fields'])==v.read(0x4af4c0+r['id']*70,70)
    p=LIVE+20*36;v.write(p+13,b'\x80');v.write(0x82e000,v.read(p+6,2));cases=[]
    for job in range(53):
     for level in (1,5,10,15,20,30,50):
      for mp in (0,1,5,10,30,250):
        v.write(UNIT+234*72+0x2b,bytes([job,level]));v.put32(p+20,mp)
        before=v.read(LIVE,115*36),v.read(UNIT,512*72),v.u32(0x48ecd4)
        v.call(0x438308,[0x82e000,0,4,255,255,1],p)
        assert before==(v.read(LIVE,115*36),v.read(UNIT,512*72),v.u32(0x48ecd4))
        ids=v.read(0x4ac7ac,69);cases.append(dict(job=job,level=level,mp=mp,available=list(ids[:ids.index(255)])))
    return dict(executable_sha256=EDITION,records=records,cases=cases,boundaries='Original loader consumes 68 70-byte records. All 53 classes, seven levels, six MP values through original mode-4 availability. Flag128/budget0 suppress candidate scoring without a rule hook. No tactic hit/effect/AI score is established.')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_tactics.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['records']),'decoded tactics;',len(r['cases']),'availability cases')
