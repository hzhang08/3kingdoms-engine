#!/usr/bin/env python3
"""Unhooked original opening-enemy tactic hit, damage and AI scoring.

Inputs come from the post-introduction oracle, with explicit synthetic stat,
status, equipment, target and RNG variations. No HP application or tactic
animation is claimed. All decision-time random draws execute as original x86.
"""
import argparse,json,random
from pathlib import Path
from battle_events_probe import prepare,LIVE
from initialization_probe import state,UNIT
from movement_probe import live_json
from ai_decision_probe import configure as decision_configure,MovementBoundary
from unicorn import UC_HOOK_CODE
from executable_probe import EDITION

def configure(v):
    decision_configure(v)
    v.ranges += [(0x4390ef,0x43a148),(0x43b73f,0x43bb6b),(0x43bfaa,0x43c35c),
      (0x41fec9,0x4203c2),(0x420422,0x4208f4),(0x420bbd,0x420c68),
      (0x41df90,0x41e042),(0x425ab0,0x425ac1),(0x425b30,0x425b61),
      (0x43e320,0x43e331),(0x449bac,0x449be0),(0x449c36,0x449c55),
      (0x435aad,0x436586)]

def generate(source):
    _,v,_,_=prepare(source);configure(v)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000)
    pairs=[];scores=[]
    def reset():v.write(0x400000,original);v.write(0x800000,scratch);v.hits.clear()
    def invariant():return v.read(LIVE,115*36),v.read(UNIT,512*72)
    def pair(actor,target,tactic,seed):
        a,b=live_json(v,actor),live_json(v,target);before=invariant()
        v.put32(0x48ecd4,seed)
        eligible=v.call(0x4390ef,[actor,target,tactic])
        chance=v.call(0x43b73f,[0,LIVE+target*36,tactic],LIVE+actor*36)
        assert v.u32(0x48ecd4)==seed
        damage=v.call(0x43bfaa,[LIVE+target*36,UNIT+b['id']*72,tactic,0],LIVE+actor*36)
        damage_rng=v.u32(0x48ecd4);v.put32(0x48ecd4,seed)
        estimate=v.call(0x43bfaa,[LIVE+target*36,UNIT+b['id']*72,tactic,1],LIVE+actor*36)
        assert v.u32(0x48ecd4)==seed
        value=v.call(0x4395ae,[target,tactic,a['x']|(a['y']<<8),v.read(0x818640+a['y']*20+a['x'],1)[0]],LIVE+actor*36)
        assert invariant()==before
        assert not v.hits.intersection(v.hooks),[hex(a) for a in v.hits.intersection(v.hooks)]
        pairs.append(dict(actor=a,actor_unit=state(v,a['id']),target=b,target_unit=state(v,b['id']),tactic=tactic,seed=seed,
          eligible=bool(eligible),chance=chance,damage=damage,damage_rng=damage_rng,estimate=estimate,score=value,score_rng=v.u32(0x48ecd4)))
    for tactic in (23,25,33):
     for actor,target in ((35,18),(35,20),(36,0),(35,36),(20,35)):
      for seed in (1,42,0xffffffff):
       for status in (0,8,16,24):
        reset();v.write(LIVE+target*36+30,bytes([status]));pair(actor,target,tactic,seed)
    rng=random.Random(0x4395ae)
    for job in range(53):
     for tactic in (23,25,33):
      for ai in (2,7):
        reset();v.write(LIVE+35*36+14,bytes([ai]));v.write(UNIT+234*72+0x2b,bytes([job]))
        for slot,uid in ((35,146),(20,234)):
            v.write(UNIT+uid*72+0x15,bytes(rng.choice((0,1,3,10,30,31,50,100,255)) for _ in range(5)))
            v.write(UNIT+uid*72+0x2e,bytes([rng.choice((24,255)),9,0,rng.choice((37,43,44,255)),9,0,rng.choice((49,53,55,56,57,58,59,255)),255,255]))
            v.write(LIVE+slot*36+24,bytes([rng.randrange(7) for _ in range(6)]+[rng.choice((0,8,16,24))]))
        v.put32(LIVE+20*36+16,rng.choice((0,1,10,20,40,100,300)))
        pair(35,20,tactic,rng.getrandbits(32))
    # This edition actually supplies hit-bonus equipment 36 and tactic-damage
    # resistance 47. Exercise them directly rather than relying on random picks.
    for weapon in (36,255):
     for armor in (47,255):
      for hp in (1,39,40,41,100):
       for seed in (1,42,0xffffffff):
        reset();v.write(UNIT+146*72+0x2e,bytes([weapon,9,0]+[255]*6))
        v.write(UNIT+234*72+0x2e,bytes([255]*3+[armor,9,0]+[255]*3))
        v.put32(UNIT+234*72+0x1c,100);v.put32(LIVE+20*36+16,hp);pair(35,20,33,seed)
    for actor in (35,36):
     for y in (0,5,8,11,14,17,19):
      for x in (0,8,10,12,19):
       for available in ((23,),(25,),(33,),(23,25,33)):
        reset();seed=rng.getrandbits(32);v.put32(0x48ecd4,seed);v.write(0x82e000,bytes([x,y]));v.write(0x4ac7ac,bytes([*available,255]))
        v.write(LIVE+actor*36+8,bytes([rng.choice((0,18,19,20,255))]))
        before=invariant();value=v.call(0x4391dc,[0x82e000],LIVE+actor*36)
        assert invariant()==before
        assert not v.hits.intersection(v.hooks),[hex(a) for a in v.hits.intersection(v.hooks)]
        scores.append(dict(actor=live_json(v,actor),center=[x,y],available=available,seed=seed,score=value&255,
          target=v.read(0x4b2c2c,1)[0],tactic=v.read(0x4b2c40,1)[0],final_rng=v.u32(0x48ecd4)))
    decisions=[];trace=[]
    def stop(vm):raise MovementBoundary()
    v.hooks[0x43a956]=stop
    def observe(uc,address,size,user):
        if address==0x438308:trace.append(dict(kind='search',origin=list(v.read(v.arg(0),2)),budget=v.arg(1)&255,mode=v.arg(2)&255,target=[v.arg(3)&255,v.arg(4)&255],zoc=bool(v.arg(5))))
        elif address==0x43a529:trace.append(dict(kind='candidate',tile=list(v.read(v.arg(0),2))))
    observer=v.uc.hook_add(UC_HOOK_CODE,observe)
    for actor in (35,36):
     for ai in (1,2):
      for seed in (1,42,0xffffffff):
       for followed in (0,255):
        reset();v.write(LIVE+actor*36+14,bytes([ai]));v.write(LIVE+actor*36+8,bytes([followed]));v.put32(0x48ecd4,seed);before=invariant();trace.clear()
        battle=live_json(v,actor)
        try:v.call(0x43ad1c,this=LIVE+actor*36,budget=12000000)
        except MovementBoundary:pass
        else:raise AssertionError('expected movement boundary')
        after=invariant();assert after[1]==before[1]
        assert after[0][:actor*36]==before[0][:actor*36] and after[0][(actor+1)*36:]==before[0][(actor+1)*36:]
        assert v.hits.intersection(v.hooks)=={0x43a956}
        decisions.append(dict(actor=battle,seed=seed,after=live_json(v,actor),trace=list(trace),
          best_score=v.read(0x4b2c4c,1)[0],best_tile=list(v.read(0x4b2c20,2)),best_target=v.read(0x4b2c28,1)[0],best_tactic=v.read(0x4b2c44,1)[0],
          final_rng=v.u32(0x48ecd4),distance=list(v.read(0x8192c0,400)),threat=list(v.read(0x824000,400)),scores=list(v.read(0x824640,400)),seek_flag=v.u32(0x4b2c1c)))
    v.uc.hook_del(observer)
    reset();roster=[live_json(v,s) for s in range(115)]
    return dict(executable_sha256=EDITION,pairs=pairs,scores=scores,decisions=decisions,roster=roster,
      records=[state(v,uid) for uid in sorted({b['id'] for b in roster})],
      tactics=[list(v.read(0x4af4c0+n*70,70)) for n in range(68)],terrain=list(v.read(0x818640,400)),boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'))
    p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_tactic_scoring.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n')
    print(len(r['pairs']),'original tactic pair cases;',len(r['scores']),'candidate scores')
