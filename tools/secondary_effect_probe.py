#!/usr/bin/env python3
"""Run original poison-status/miss MEFF players and integer compositor.

The original 0x4224a8 application invokes the real loader and player for 49/45.
Only host raster/window/audio/wait/numeric boundaries are intercepted. Raster
capture supplies a deterministic indexed background, so pixel hashes verify
compositing separately from SDL. Includes all 65,536 palette pairs for every
weight/amount pair used by these resources. No interactive Windows claim.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from tactic_cast_probe import prepared,CONTROL,LIVE
from tactic_effect_probe import configure as effect_configure
from tactic_application_probe import configure as apply_configure
from executable_probe import EDITION
from ls12 import decode

def generate(source):
    _,v,events,snapshot=prepared(source)
    v.cast_elapsed=10;v.call(0x4218d6,this=CONTROL,budget=5000000)
    effect_configure(v,events);v.call(0x4214bf,this=CONTROL,budget=12000000)
    apply_configure(v,events,True)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000)
    cases=[]
    for resource in (49,45):
      for xy in ([9,17],[0,0],[19,0],[0,19],[19,19]):
        v.write(0x400000,original);v.write(0x800000,scratch);events.clear();v.hits.clear()
        v.put32(CONTROL+0x84,0);v.write(CONTROL+0x255,bytes([112 if resource==49 else 0]))
        v.put32(LIVE+20*36+16,90);v.write(LIVE+20*36+6,bytes(xy))
        before=snapshot();v.call(0x4224a8,this=CONTROL,budget=5000000);after=snapshot()
        assert before['rng']==after['rng']
        allowed={0x403340,0x4036f9,0x418f49,0x4192c5,0x41e5db,0x41e5f6,0x41fb35,0x41fc89,
          0x42c4e0,0x42c56b,0x43030e,0x43eb1f,0x450984,0x45137c,0x454cef,0x474652,0x47467a,
          0x453b13,0x479a01,0x47d1f1,0x47e09b,0x47e251,0x4807f0,0x480b30,0x820e00}
        assert not(v.hits.intersection(v.hooks)-allowed),[hex(a) for a in v.hits.intersection(v.hooks)-allowed]
        assert sum(e['kind']=='effect' for e in events)==(13 if resource==49 else 9)
        target=next(u for u in after['units'] if u['battle']['slot']==20)
        assert target['battle']['status']==(16 if resource==49 else 0)
        cases.append(dict(resource=resource,position=xy,before=before,after=after,target=target,events=list(events),hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
    resources={b.index:b.data for b in decode((source/'Meff.e5').read_bytes(),indices={45,49})}
    parameters=sorted({tuple(raw[20+n*20+2:20+n*20+4]) for raw in resources.values() for n in range(raw[0])})
    blends=[]
    v.write(0xd00000,bytes(i//256 for i in range(65536)));v.write(0xd10000,bytes(i%256 for i in range(65536)))
    for weight,amount in parameters:
        v.call(0x42fd5a,[256,256,0xd00000,0xd10000,0xd20000,weight,0,0,amount],budget=18000000)
        blends.append(dict(weight=weight,amount=amount,pixels_sha256=hashlib.sha256(v.read(0xd20000,65536)).hexdigest()))
    return dict(executable_sha256=EDITION,resources={str(k):hashlib.sha256(raw).hexdigest() for k,raw in resources.items()},cases=cases,blends=blends,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_secondary_effect.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['cases']),'original secondary effects;',len(r['blends']),'exhaustive palette-pair parameter sets')
