#!/usr/bin/env python3
"""Run original physical hit/damage including real getters and RNG, no stubs."""
import argparse,json,random
from pathlib import Path
from battle_events_probe import prepare,LIVE
from initialization_probe import state,UNIT
from movement_probe import live_json
from range_probe import configure as range_configure
from executable_probe import EDITION

def configure(v):
    range_configure(v)
    v.ranges += [(0x407126,0x40721b),(0x43f4fe,0x43f6cd),(0x43f84d,0x43f8be),
      (0x443090,0x4430ac),(0x43b4b0,0x43b73f),(0x43bc4b,0x43bf96),
      (0x40be7d,0x40be92),(0x43e4d6,0x43e515),(0x43e3f0,0x43e40e),
      (0x41c2b0,0x41c2c1),(0x47fae7,0x47fb4c)]

def generate(source):
    _,v,_,_=prepare(source);configure(v)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);stats=[];cases=[]
    def reset():v.write(0x400000,original);v.write(0x800000,scratch)
    getters=(0x43f4fe,0x43f595,0x43f5e3,0x43f631,0x43f67f)
    for item in range(104):
     for slot in range(3):
      for grade in range(7):
        reset();v.load_unit(0);v.write(UNIT+0x15,bytes([255,254,100,255,250]));v.write(UNIT+0x2e,b'\xff'*9);v.write(UNIT+0x2e+slot*3,bytes([item,9,0]));v.write(LIVE+24,bytes([grade])*6)
        before=v.read(UNIT,512*72),v.read(LIVE,115*36),v.u32(0x48ecd4)
        values=[v.call(a,this=LIVE) for a in getters]
        assert before==(v.read(UNIT,512*72),v.read(LIVE,115*36),v.u32(0x48ecd4))
        stats.append(dict(unit=state(v,0),battle=live_json(v,0),values=values))
    def capture(attacker,target,randomize,counter,seed):
        a=live_json(v,attacker);b=live_json(v,target)
        before=v.read(UNIT,512*72),v.read(LIVE,115*36);v.put32(0x48ecd4,seed)
        chance=v.call(0x43b50d,[0,LIVE+target*36,255],LIVE+attacker*36)
        assert seed==v.u32(0x48ecd4)
        damage=v.call(0x43bc4b,[UNIT+b['id']*72,int(randomize),int(counter)],LIVE+attacker*36)
        assert before==(v.read(UNIT,512*72),v.read(LIVE,115*36))
        cases.append(dict(attacker=a,attacker_unit=state(v,a['id']),target=b,target_unit=state(v,b['id']),attack_terrain=v.read(0x818640+a['y']*20+a['x'],1)[0],defense_terrain=v.read(0x818640+b['y']*20+b['x'],1)[0],randomize=randomize,counterattack=counter,seed=seed,chance=chance,damage=damage,final_rng=v.u32(0x48ecd4)))
    for attacker in (18,19,20,35,36,48):
     for target in (18,20,35,40,48):
      for seed in (1,42,0xffffffff):
       for randomize in (False,True):
        for counter in (False,True):reset();capture(attacker,target,randomize,counter,seed)
    rng=random.Random(0x43bc4b)
    for job in range(53):
     for terrain in range(30):
        reset()
        for slot,uid,j in ((20,234,job),(35,146,(job+terrain)%53)):
            v.write(UNIT+uid*72+0x2b,bytes([j]));v.write(UNIT+uid*72+0x15,bytes(rng.choice((3,10,30,31,50,100,255)) for _ in range(5)))
            v.write(UNIT+uid*72+0x2e,bytes([rng.choice((22,23,255)),9,0,rng.choice((44,255)),9,0,rng.choice((53,54,56,57,58,70,71,255)),255,255]))
            v.write(LIVE+slot*36+24,bytes([rng.randrange(7) for _ in range(6)]+[rng.choice((0,8))]))
        a=live_json(v,20);b=live_json(v,35);v.write(0x818640+a['y']*20+a['x'],bytes([terrain]));v.write(0x818640+b['y']*20+b['x'],bytes([(terrain*7)%30]));v.put32(LIVE+35*36+16,rng.choice((0,1,20,100,500)))
        capture(20,35,bool(rng.randrange(2)),bool(rng.randrange(2)),rng.getrandbits(32))
    # Exhaustive agility thresholds without equipment, including odd divisors.
    for attack in (0,1,2,3,9,10,29,30,31,59,60,100,255):
     for defense in (0,3,4,9,10,29,30,31,59,60,100,255):
        reset()
        for slot,uid,agility in ((20,234,attack),(35,146,defense)):
            v.write(UNIT+uid*72+0x18,bytes([agility]));v.write(UNIT+uid*72+0x2e,b'\xff'*9);v.write(LIVE+slot*36+24,b'\x03'*6)
        capture(20,35,True,False,rng.getrandbits(32))
    return dict(executable_sha256=EDITION,stats=stats,cases=cases,boundaries='Unhooked original battle/persistent stat getters, equipment effects, terrain factors, physical hit chance 0x43b50d and damage 0x43bc4b including original RNG draws. No unit/persistent mutations allowed; damage is a returned amount, not HP application. Initial opening pairs plus explicit class/terrain/grade/equipment/HP/seed cases. Does not establish action-level critical/double/counter selection, XP, death events or animation.')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_physical_resolution.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print({k:len(v) for k,v in r.items() if isinstance(v,list)})
