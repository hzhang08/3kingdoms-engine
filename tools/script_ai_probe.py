#!/usr/bin/env python3
"""Original opcode78 single-unit non-targeted AI assignment.

Runs the exact typed handler and live lookup/owner check/flag and AI setters.
No game-rule or presentation hooks are allowed. Covers the opening mode1 and
adjacent non-targeted modes0/2, absent/inactive/dead units and party protection.
"""
import argparse,json,struct
from pathlib import Path
from battle_events_probe import prepare,LIVE
from deployment_probe import script_call
from movement_probe import live_json
from initialization_probe import UNIT,state
from executable_probe import EDITION

def configure(v):v.ranges += [(0x4146d4,0x4148fe),(0x41a190,0x41a1b0),(0x418d80,0x418d91)]

def instruction(uid,mode):
    values=[-1,uid,0,0,0,0,-1,mode,-1,0,0]
    types=json.loads(Path('tools/eex_schema.json').read_text())['78']['types']
    result=struct.pack('<H',78)
    for t,value in zip(types,values):result+=struct.pack('<H',t)+struct.pack('<I' if t==4 else '<H',value& (0xffffffff if t==4 else 0xffff))
    return result

def generate(source):
    _,v,_,_=prepare(source);configure(v)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    for uid in (0,146,511):
      for life in range(5):
       for owner in (0,1,255):
        for mode in (0,1,2,7,255):
         v.write(0x400000,original);v.write(0x800000,scratch);v.hits.clear()
         slot=next((n for n in range(115) if v.u32(LIVE+n*36)==uid),None)
         if slot is not None:
            v.write(LIVE+slot*36+12,bytes([life]));v.write(LIVE+slot*36+13,b'\xff')
         v.write(UNIT+uid*72+0x2a,bytes([owner]))
         before=[live_json(v,n) for n in range(115) if v.u32(LIVE+n*36)<512]
         persistent=v.read(UNIT,512*72);rng=v.u32(0x48ecd4)
         script_call(v,0x4146d4,instruction(uid,mode))
         assert not v.hits.intersection(v.hooks),[hex(a) for a in v.hits.intersection(v.hooks)]
         assert persistent==v.read(UNIT,512*72) and rng==v.u32(0x48ecd4)
         cases.append(dict(id=uid,mode=mode,record=state(v,uid),before=before,after=[live_json(v,n) for n in range(115) if v.u32(LIVE+n*36)<512]))
    return dict(executable_sha256=EDITION,modes=[v.u32(0x48e380+n*4) for n in range(7)],cases=cases,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_script_ai.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'AI assignment cases')
