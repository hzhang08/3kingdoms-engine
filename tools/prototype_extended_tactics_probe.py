#!/usr/bin/env python3
"""Hash-gated original x86 oracle for the prototype's bounded 15-spell subset.
No game files are changed. Calls hit/damage/heal, grade/status setters and XP
arithmetic directly. It does not claim full selector or animation equivalence.
Refresh calls are the only hooked outputs after fixture preparation.
"""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from tactic_scoring_probe import configure
from initialization_probe import state,UNIT
from movement_probe import live_json
from executable_probe import EDITION
TACTICS=[0,2,5,7,10,15,17,20,23,25,33,35,39,61,63]
CONTROL=0x497af8

def generate(source):
 _,v,_,_=prepare(source);configure(v)
 v.ranges += [(0x4066c0,0x4066e0),(0x42097a,0x4214bf),(0x43c548,0x43c6f6),(0x43f98e,0x43fbb8),(0x43ff54,0x440044),(0x4072bd,0x4072fc),(0x425ad0,0x425ae1)]
 v.hooks[0x479a01]=lambda vm:vm.return_from_hook()
 original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000)
 cases=[]
 for tactic in TACTICS:
  for job in [0,24,36,39]:
   for grade in [0,2,3,4,6]:
    for seed in [1,20,23,42,0xffffffff]:
     v.write(0x400000,original);v.write(0x800000,scratch);v.hits.clear()
     actor,target=35,20;aid,bid=146,234
     v.write(UNIT+aid*72+0x2b,bytes([job]));v.write(LIVE+actor*36+14,b'\x07')
     v.write(LIVE+target*36+24,bytes([grade]*6));v.write(LIVE+target*36+30,b'\0')
     v.put32(LIVE+target*36+16,80 if seed in (20,42) else 20);v.put32(LIVE+actor*36+16,10)
     v.write(LIVE+target*36+5,bytes([0 if seed in (1,20) else 1]));v.write(UNIT+bid*72+0x2b,bytes([24 if seed in (23,42) else 0]))
     v.write(UNIT+aid*72+0x2c,bytes([3 if seed==20 else 8 if seed==42 else 5]))
     v.write(0x818640+live_json(v,target)['y']*20+live_json(v,target)['x'],bytes([7 if tactic in (5,7) else 3 if tactic in (15,17) else 0]))
     a,b=live_json(v,actor),live_json(v,target);au,bu=state(v,aid),state(v,bid)
     v.put32(0x48ecd4,seed)
     usable=bool(v.call(0x420422,[tactic,actor,target,0]));chance=v.call(0x43b73f,[0,LIVE+target*36,tactic],LIVE+actor*36)
     estimate=v.call(0x43c548,[tactic,actor,1],LIVE+target*36) if tactic==39 else v.call(0x43bfaa,[LIVE+target*36,UNIT+bid*72,tactic,1],LIVE+actor*36)
     assert v.u32(0x48ecd4)==seed
     effect=False
     if tactic in (23,25,33,35):effect=bool(v.call(0x43b4b0,[LIVE+target*36,2,tactic],LIVE+actor*36))
     damage=v.call(0x43c548,[tactic,actor,0],LIVE+target*36) if tactic==39 else v.call(0x43bfaa,[LIVE+target*36,UNIT+bid*72,tactic,0],LIVE+actor*36)
     after_rng=v.u32(0x48ecd4)
     if tactic in (23,25):v.call(0x420cb2,[tactic,target])
     elif tactic==61:v.call(0x420cb2,[53,target])
     elif tactic==63:
      for stat_tactic in [49,51,53,55]:v.call(0x420cb2,[stat_tactic,target])
     elif tactic in (33,35):v.call(0x420e25,[tactic,target])
     changed=live_json(v,target)
     # XP preparation's original eligibility bit is set here to examine its
     # formulas; real caller enables it only for side0 or AI7 casters.
     v.write(CONTROL,bytes([tactic,actor]));v.put32(CONTROL+4,UNIT+aid*72);v.put32(CONTROL+8,LIVE+actor*36);v.write(CONTROL+0xc,bytes([target]));v.put32(CONTROL+0x4a8,1);v.put32(CONTROL+0x84,damage)
     xp=v.call(0x420efb,[0],CONTROL);weapon=v.call(0x42128b,[],CONTROL);armor=v.call(0x4213a3,[0],CONTROL)
     assert v.u32(0x48ecd4)==after_rng
     assert not (v.hits.intersection(v.hooks)-{0x479a01}),[hex(a) for a in v.hits.intersection(v.hooks)]
     cases.append(dict(tactic=tactic,actor=a,actor_unit=au,target=b,target_unit=bu,seed=seed,usable=usable,chance=chance,estimate=estimate,amount=damage,status_hit=effect,rng=after_rng,grades_after=changed['grades'],status_after=changed['status'],experience=xp,weapon_experience=weapon,armor_experience=armor))
 return dict(executable_sha256=EDITION,cases=cases,boundaries=__doc__)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_prototype_extended_tactics.json'));a=p.parse_args();result=generate(a.source);a.output.write_text(json.dumps(result,ensure_ascii=False,separators=(',',':'))+'\n');print(len(result['cases']),'bounded original tactic cases')
