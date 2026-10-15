#!/usr/bin/env python3
"""Original route reconstruction and fallback AI-position scores, without hooks.

Keep caller-owned scratch bytes: 0x437e2e does not append a terminator, and its
failed descent includes one pre-existing byte in the reverse/invert pass.
"""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from initialization_probe import state,UNIT
from movement_probe import configure,live_json
from executable_probe import EDITION

def generate(source):
    _,v,_,_=prepare(source);configure(v)
    v.ranges += [(0x437e2e,0x438308),(0x43f84d,0x43f8be),(0x443090,0x4430ac)]
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    def reset():v.write(0x400000,original);v.write(0x800000,scratch)
    def capture(slot,origin,destination,budget,score=True,seed=255):
        width,height=v.read(0x4b422c,2);count=width*height;b=live_json(v,slot)
        v.write(0x82e000,bytes(origin));v.write(0x82e004,bytes(destination))
        # Mode 2's original grid supplies the input; route itself also runs on
        # unreachable destinations to exercise the failure/fallback path.
        v.call(0x438308,[0x82e000,255,2,*destination,1],LIVE+slot*36,budget=3000000)
        costs=list(v.read(0x8192c0,count))
        route=[seed]*256;route[0]=255
        scores=[0 if n%11 else 7 for n in range(count)]
        v.write(0x819900,bytes(route));v.write(0x824640,bytes(scores))
        before=v.read(LIVE,115*36);persistent=v.read(UNIT,512*72);rng=v.u32(0x48ecd4)
        result=v.call(0x437e2e,[0x82e000,0x82e004,budget,int(score)],LIVE+slot*36,budget=3000000)
        assert result==0x819900
        assert before==v.read(LIVE,115*36) and persistent==v.read(UNIT,512*72) and rng==v.u32(0x48ecd4)
        cases.append(dict(unit=state(v,b['id']),battle=b,
          roster=[live_json(v,n) for n in range(115) if v.read(LIVE+n*36+12,1)[0]==2],
          width=width,height=height,terrain=list(v.read(0x818640,count)),costs=costs,
          origin=origin,destination=destination,budget=budget,score_on_failure=score,
          route_before=route,route_after=list(v.read(0x819900,256)),
          scores_before=scores,scores_after=list(v.read(0x824640,count))))
    for slot in (18,20,35,48):
      for destination in ((0,0),(10,17),(11,15),(9,11),(19,19)):
       for budget in (0,1,4,255):
        reset();b=live_json(v,slot);capture(slot,(b['x'],b['y']),destination,budget)
    # The AI also descends from the actor toward a grid rooted at another unit.
    for target in ((10,17),(9,11),(11,10),(0,0)):
     for budget in (0,1,4,255):
      for seed in (0,1,2,3,255):
        reset();capture(20,target,(11,17),budget,True,seed)
    for job in range(53):
     for flags in (0,128):
      for budget in (0,3,255):
        reset();v.load_unit(0);v.write(UNIT+0x2b,bytes([job]));v.write(UNIT+0x2e,b'\xff'*9)
        v.write(LIVE,b'\0'*(115*36));v.write(0x818c80,b'\xff'*1600);v.write(0x4b422c,bytes([9,9]))
        v.write(0x818640,bytes((x*3+y*7+job)%30 for y in range(9) for x in range(9)))
        for slot,x,y in ((0,4,4),(18,4,3),(35,6,4),(36,3,6)):
            v.put32(LIVE+slot*36,0);v.write(LIVE+slot*36+4,bytes([slot,slot%4,x,y]));v.write(LIVE+slot*36+12,b'\x02')
            v.write(0x818c80+y*9+x,bytes([slot]))
        v.write(LIVE+13,bytes([flags]));capture(0,(0,0),(4,4),budget)
    for accessory in (255,62,63):
     for destination in ((4,4),(4,0),(8,8)):
      for budget in (0,2,8,255):
       for score in (False,True):
        reset();v.load_unit(0);v.write(UNIT+0x2e,bytes([255]*6+[accessory,255,255]))
        v.write(LIVE,b'\0'*(115*36));v.write(0x818c80,b'\xff'*1600);v.write(0x4b422c,bytes([9,9]));v.write(0x818640,b'\0'*81)
        for slot,x,y in ((0,4,4),(35,5,4),(36,4,2)):
            v.write(LIVE+slot*36+4,bytes([slot,0,x,y]));v.write(LIVE+slot*36+12,b'\x02');v.write(0x818c80+y*9+x,bytes([slot]))
        capture(0,(4,4),destination,budget,score)
    return dict(executable_sha256=EDITION,cases=cases,
      boundaries='Original 0x438308 mode 2 supplies distance grids; original 0x437e2e and 0x438178 reconstruct routes and score incomplete descents without rule hooks. Complete route scratch and score grid compared, including stale bytes and nonzero existing scores. Units, persistent records and RNG must remain unchanged. Does not execute AI action selection or movement presentation.')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_routes.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'original route cases')
