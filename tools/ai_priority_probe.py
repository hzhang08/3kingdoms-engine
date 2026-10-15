#!/usr/bin/env python3
"""Capture original AI ordering at the post-introduction boundary."""
import json,argparse
from pathlib import Path
from battle_events_probe import prepare,LIVE,CTRL
from executable_probe import EDITION
from initialization_probe import state,UNIT

def generate(source):
    initial,v,_,_=prepare(source)
    v.ranges += [(0x44da7a,0x44df61),(0x44f980,0x44f998),(0x425ab0,0x425ac1),
                 (0x435182,0x4351da),(0x435991,0x4359f5),(0x43a1f4,0x43a28a),
                 (0x43ff6e,0x4400a2),(0x406906,0x406947)]
    live=v.read(LIVE,115*36);persistent=v.read(0x4a1b68,512*72);rng=v.u32(0x48ecd4)
    cases=[]
    def capture(allied):
        before=v.read(LIVE,115*36);records=v.read(UNIT,512*72);rng=v.u32(0x48ecd4)
        roster=[dict(live=list(before[n*36:(n+1)*36]),unit=state(v,v.u32(LIVE+n*36))) for n in range(115) if before[n*36+12]]
        v.call(0x44da7a,[int(allied)],CTRL);after=v.read(LIVE,115*36)
        assert v.read(UNIT,512*72)==records and v.u32(0x48ecd4)==rng
        for n in range(115):assert before[n*36:n*36+11]==after[n*36:n*36+11] and before[n*36+12:(n+1)*36]==after[n*36+12:(n+1)*36]
        begin,end=(0,35) if allied else (35,115)
        order=sorted([dict(slot=n,id=v.u32(LIVE+n*36),priority=after[n*36+11]) for n in range(begin,end) if after[n*36+12]==2],key=lambda r:r['priority'])
        return dict(allied=allied,roster=roster,terrain=list(v.read(0x818640,400)),order=order)
    for allied in (True,False):
        v.write(LIVE,live);cases.append(capture(allied))
    for job in range(53):
      for terrain in range(30):
        v.write(LIVE,b'\0'*(115*36));v.write(0x818640,bytes([terrain])*400)
        for n,(slot,uid) in enumerate(zip((0,1,2,18,35,36,37,114),(0,173,174,234,1,175,2,474))):
            v.load_unit(uid);v.write(UNIT+72*uid+0x2b,bytes([(job+n*3)%53]));v.write(UNIT+72*uid+0x2e,b'\xff'*9)
            v.put32(UNIT+72*uid+0x1c,100+n);v.put32(LIVE+slot*36,uid)
            v.write(LIVE+slot*36+4,bytes([slot,n%4,n,0]))
            v.write(LIVE+slot*36+12,bytes([2 if n!=3 else (terrain%3)+1,terrain*7%256,7 if n%2 else 0]))
            v.put32(LIVE+slot*36+16,(0,1,19,20,21,39,40,41,100)[(terrain+n)%9])
        for allied in (True,False):cases.append(capture(allied))
    return dict(executable_sha256=EDITION,cases=cases,boundaries='Unhooked original AI-priority routine and predicates. Initial post-introduction input plus explicit synthetic class/terrain/HP/lifecycle/control/side cases. Only priority bytes may change. No AI action executes.')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_ai_priority.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'AI-priority cases')
