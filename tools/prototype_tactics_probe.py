"""Bounded original elemental/poison damage oracle; no game files are changed."""
import json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from tactic_scoring_probe import configure
from initialization_probe import state,UNIT
from movement_probe import live_json
from executable_probe import EDITION
_,v,_,_=prepare(Path('/Users/henryzhang/Downloads/sgzccz'));configure(v)
v.ranges += [(0x42097a,0x420bbd)]
original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
for tactic,terrain in [(0,0),(5,7),(10,0),(15,3),(33,0)]:
 for job in [0,3,6,24,30,39,42,43,49,50]:
  for seed in [1,20,23,42,0xffffffff]:
   for status in [0,8,16]:
    v.write(0x400000,original);v.write(0x800000,scratch);v.hits.clear()
    v.write(UNIT+234*72+0x2b,bytes([job]));v.write(LIVE+20*36+30,bytes([status]));b=live_json(v,20)
    v.write(0x818640+b['y']*20+b['x'],bytes([terrain]));v.put32(0x48ecd4,seed)
    chance=v.call(0x43b73f,[0,LIVE+20*36,tactic],LIVE+35*36)
    damage=v.call(0x43bfaa,[LIVE+20*36,UNIT+234*72,tactic,0],LIVE+35*36);after=v.u32(0x48ecd4)
    v.put32(0x48ecd4,seed);estimate=v.call(0x43bfaa,[LIVE+20*36,UNIT+234*72,tactic,1],LIVE+35*36)
    assert v.u32(0x48ecd4)==seed
    assert not v.hits.intersection(v.hooks),[hex(a) for a in v.hits.intersection(v.hooks)]
    cases.append(dict(actor=live_json(v,35),actor_unit=state(v,146),target=b,target_unit=state(v,234),terrain=terrain,tactic=tactic,seed=seed,chance=chance,damage=damage,rng=after,estimate=estimate))
p=Path('tests/fixtures/original_prototype_tactics.json');p.write_text(json.dumps(dict(executable_sha256=EDITION,cases=cases,boundary=__doc__),separators=(',',':'))+'\n');print(len(cases),'original hit/damage/estimate/RNG cases')
