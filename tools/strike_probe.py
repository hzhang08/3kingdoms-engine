#!/usr/bin/env python3
"""Original strike planning, critical/double rolls and experience requests.

Runs original instructions/getters/RNG without hooks after input preparation.
The caller supplies the ordered target list; this is not a complete attack,
target selection, HP/XP application, counterattack or animation oracle.
"""
import argparse,json,random
from unicorn import UcError
from unicorn.x86_const import UC_X86_REG_EIP
from pathlib import Path
from battle_events_probe import prepare,LIVE,READER
from initialization_probe import state,UNIT
from movement_probe import live_json
from physical_resolution_probe import configure as physical_configure
from executable_probe import EDITION

CONTROL=0x4927f0

def configure(v):
    physical_configure(v)
    v.ranges += [(0x405ce6,0x4061fc),(0x40379b,0x403b8f),
      (0x4065d0,0x40660f),(0x406630,0x406649),(0x40681a,0x406906),
      (0x4072bd,0x4072fc),(0x43ff54,0x43ff6e),(0x472840,0x472851)]

def generate(source):
    _,v,_,_=prepare(source);configure(v)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000)
    cases=[];chances=[];faults=[];rng=random.Random(0x405e03)
    def reset():v.write(0x400000,original);v.write(0x800000,scratch)
    def setup(actor,targets,round_number=0,counter=0):
        v.write(CONTROL,b'\0'*0x614)
        uid=v.u32(LIVE+actor*36)
        v.write(CONTROL,bytes([actor,targets[0]]));v.put32(CONTROL+4,uid)
        v.put32(CONTROL+8,UNIT+uid*72);v.put32(CONTROL+12,LIVE+actor*36)
        v.write(CONTROL+16,bytes(targets+[255]));v.write(CONTROL+0x608,bytes([round_number]))
        v.put32(CONTROL+0x600,int(v.read(LIVE+actor*36+5,1)[0]==0))
        v.put32(0x492e04,counter)
    def invariant():return v.read(LIVE,115*36),v.read(UNIT,512*72)
    def capture(actor,targets,round_number,counter,seed,previous_xp=0,previous_weapon_xp=0):
        setup(actor,targets,round_number,counter)
        v.put32(CONTROL+0x428,previous_xp);v.put32(CONTROL+0x42c,previous_weapon_xp)
        v.put32(0x48ecd4,seed);before=invariant();v.hits.clear()
        a=live_json(v,actor)
        c=dict(attacker=a,attacker_unit=state(v,a['id']),targets=[dict(battle=live_json(v,t),unit=state(v,v.u32(LIVE+t*36))) for t in targets],
          attack_terrain=v.read(0x818640+a['y']*20+a['x'],1)[0],
          target_terrain=[v.read(0x818640+v.read(LIVE+t*36+7,1)[0]*20+v.read(LIVE+t*36+6,1)[0],1)[0] for t in targets],
          round=round_number,counter_depth=counter,seed=seed,priority_id=int.from_bytes(v.read(READER+0x3d06,2),'little'),
          previous_xp=previous_xp,previous_weapon_xp=previous_weapon_xp)
        try:v.call(0x405e03,this=CONTROL)
        except UcError:
            pc=v.uc.reg_read(UC_X86_REG_EIP)
            if pc!=0x43b66a:raise
            c['original_fault']='division by zero at 0x43b66a';faults.append(c);return
        c.update(hp_damage=[v.u32(CONTROL+0x84+i*4) for i in range(len(targets))],
          mp_damage=[v.u32(CONTROL+0x254+i*4) for i in range(len(targets))],
          armor_xp=[v.u32(CONTROL+0x430+i*4) for i in range(len(targets))],
          xp=v.u32(CONTROL+0x428),weapon_xp=v.u32(CONTROL+0x42c),critical=bool(v.u32(CONTROL+0x604)),strike_rng=v.u32(0x48ecd4))
        c['another_strike']=bool(v.call(0x406181,this=CONTROL));c['final_rng']=v.u32(0x48ecd4)
        assert v.u32(CONTROL+0x604)==0 and invariant()==before
        assert not v.hits.intersection(v.hooks),[hex(a) for a in v.hits.intersection(v.hooks)]
        cases.append(c)
    # Actual first allied attack destination and target, then each visible pair.
    reset();v.write(LIVE+20*36+6,bytes([9,17]));capture(20,[38],0,0,1)
    for actor,target in ((18,35),(20,38),(35,18),(48,20)):
     for seed in (1,42,0xffffffff):
      for round_number in (0,1):
       for counter in (0,1,2):reset();capture(actor,[target],round_number,counter,seed)
    # All item effects in each slot, plus zero-HP/MP and primary/secondary hits.
    for item in range(104):
     for slot in range(3):
      for on_attacker in (False,True):
        reset()
        uid=234 if on_attacker else 146
        v.write(UNIT+uid*72+0x2e,b'\xff'*9)
        if on_attacker:v.write(UNIT+uid*72+0x2e,bytes([6,1,0]))
        v.write(UNIT+uid*72+0x2e+slot*3,bytes([item,9,0]))
        v.write(LIVE+20*36+5,b'\0');v.write(LIVE+35*36+5,b'\0')
        v.write(UNIT+234*72+0x2c,bytes([rng.choice((1,5,30,50))]));v.write(UNIT+146*72+0x2c,bytes([rng.choice((1,5,30,50))]))
        v.put32(LIVE+35*36+16,rng.choice((0,1,20,100,500)));v.put32(LIVE+35*36+20,rng.choice((0,1,20,250)))
        v.write(UNIT+234*72+0x19,bytes([rng.choice((1,30,100,255))]));v.write(UNIT+146*72+0x19,bytes([rng.choice((1,30,100,255))]))
        v.write(READER+0x3d06,(146 if item%2 else 0).to_bytes(2,'little'))
        capture(20,[35,38,18] if item%3==0 else [35],item%2,item%3,rng.getrandbits(32),item%8,item%5)
    # Randomized original unit/terrain/stat combinations exercise roll order.
    for n in range(600):
        reset()
        for slot,uid in ((20,234),(35,146),(38,475)):
            v.write(UNIT+uid*72+0x2b,bytes([rng.randrange(53)]))
            v.write(UNIT+uid*72+0x15,bytes(rng.choice((0,1,10,29,30,31,60,90,100,255)) for _ in range(5)))
            # Keep these valid-domain sequences clear of the original zero
            # divisor. Probe that crash separately, last, on this VM.
            v.write(UNIT+uid*72+0x18,bytes([rng.choice((10,29,30,31,60,90,100,255))]))
            v.write(UNIT+uid*72+0x2c,bytes([rng.choice((1,2,10,30,50))]))
            v.write(LIVE+slot*36+5,bytes([rng.randrange(3)]))
            v.write(LIVE+slot*36+24,bytes([rng.randrange(7) for _ in range(6)]+[rng.choice((0,8))]))
            v.put32(LIVE+slot*36+16,rng.choice((0,1,2,20,500)));v.put32(LIVE+slot*36+20,rng.choice((0,1,2,20,250)))
            b=live_json(v,slot);v.write(0x818640+b['y']*20+b['x'],bytes([rng.randrange(30)]))
        capture(20,[35,38],n%2,n%3,rng.getrandbits(32))
    # Exhaustive ratio boundaries and guaranteed-critical equipment getter.
    for av in (0,1,2,3,9,10,29,30,31,59,60,90,100,255):
     for bv in (0,1,2,3,9,10,29,30,31,59,60,90,100,255):
      for item in (255,82):
        reset();setup(20,[35])
        for slot,uid,value in ((20,234,av),(35,146,bv)):
            v.write(UNIT+uid*72+0x18,bytes([value,value]));v.write(UNIT+uid*72+0x2e,b'\xff'*9)
            v.write(LIVE+slot*36+24,b'\x03'*6)
        v.write(UNIT+234*72+0x34,bytes([item,255,255]));before=invariant();seed=v.u32(0x48ecd4);v.hits.clear()
        double=v.call(0x405ce6,this=CONTROL)&255;critical=v.call(0x405d69,this=CONTROL)&255
        assert before==invariant() and seed==v.u32(0x48ecd4) and not v.hits.intersection(v.hooks)
        chances.append(dict(attacker=live_json(v,20),attacker_unit=state(v,234),target=live_json(v,35),target_unit=state(v,146),double=double,critical=critical))
    reset()
    for uid,value in ((234,0),(146,1)):
        v.write(UNIT+uid*72+0x18,bytes([value]));v.write(UNIT+uid*72+0x2e,b'\xff'*9)
    v.write(UNIT+234*72+0x2e,bytes([6,1,0]))
    capture(20,[35],0,0,1)
    assert len(faults)==1
    return dict(executable_sha256=EDITION,cases=cases,chances=chances,faults=faults,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_strikes.json'));a=p.parse_args()
    result=generate(a.source);a.output.write_text(json.dumps(result,separators=(',',':'))+'\n');print({k:len(v) for k,v in result.items() if isinstance(v,list)})
