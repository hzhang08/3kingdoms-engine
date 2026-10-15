#!/usr/bin/env python3
"""Original counterattack gate, stopping at facing/recursive attack requests."""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from initialization_probe import state,UNIT
from movement_probe import live_json
from strike_probe import configure,CONTROL
from executable_probe import EDITION

def generate(source):
    _,v,_,_=prepare(source);configure(v)
    v.ranges += [(0x40639a,0x4064f1),(0x4065a0,0x4065c6)]
    events=[]
    def facing(vm):
        events.append(dict(kind='facing',attacker=vm.arg(0)&255,target=vm.arg(1)&255));vm.return_from_hook()
    def attack(vm):
        events.append(dict(kind='attack',attacker=vm.arg(0)&255,target=vm.arg(1)&255));vm.return_from_hook(pop=8)
    v.hooks.update({0x435829:facing,0x4064f1:attack})
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000)
    roster=[live_json(v,n) for n in range(115) if v.read(LIVE+n*36+12,1)[0]==2]
    specs=[dict(job=j,position=pos,depth=depth) for j in range(53) for pos in ((11,17),(13,17),(12,18),(14,17),(16,17)) for depth in range(3)]
    specs += [dict(job=3,position=[11,17],depth=d,attacker_item=ai,target_item=ti,hp=hp,status=status)
      for d in range(3) for ai in (6,25) for ti in (0,21,31) for hp in (0,1,90) for status in (0,8,16)]
    specs.insert(0,dict(job=40,position=[9,15],actor_position=[9,17],depth=0))
    cases=[]
    for spec in specs:
        v.write(0x400000,original);v.write(0x800000,scratch);events.clear()
        v.write(LIVE+20*36+6,bytes(spec.get('actor_position',[12,17])))
        v.write(LIVE+38*36+6,bytes(spec['position']));v.put32(LIVE+38*36+16,spec.get('hp',90))
        v.write(LIVE+38*36+30,bytes([spec.get('status',0)]));v.write(UNIT+475*72+0x2b,bytes([spec['job']]))
        v.write(UNIT+234*72+0x2e,bytes([spec.get('attacker_item',6),1,0]))
        v.write(UNIT+475*72+0x2e,bytes([spec.get('target_item',0),1,0]))
        v.write(0x818c80,b'\xff'*1600)
        for n in range(115):
            b=live_json(v,n)
            if b['lifecycle']==2:v.write(0x818c80+b['y']*20+b['x'],bytes([n]))
        v.write(CONTROL,b'\0'*0x614);v.write(CONTROL,bytes([20,38]));v.write(CONTROL+16,bytes([38,255]))
        v.put32(CONTROL+4,234);v.put32(CONTROL+8,UNIT+234*72);v.put32(CONTROL+12,LIVE+20*36)
        v.put32(0x492e04,spec['depth']);v.write(0x4b2c28,b'\xff')
        before=v.read(LIVE,115*36),v.read(UNIT,512*72),v.u32(0x48ecd4);v.hits.clear()
        v.call(0x40639a,this=CONTROL)
        assert before==(v.read(LIVE,115*36),v.read(UNIT,512*72),v.u32(0x48ecd4))
        assert not (v.hits.intersection(v.hooks)-{0x435829,0x4064f1})
        cases.append(dict(attacker=live_json(v,20),attacker_unit=state(v,234),target=live_json(v,38),target_unit=state(v,475),depth=spec['depth'],final_depth=v.u32(0x492e04),events=list(events)))
    return dict(executable_sha256=EDITION,roster=roster,cases=cases,boundaries='Original 0x40639a counter gate, real class/equipment/status/HP getters and original range/ordered-target lookup. Facing and recursive 0x4064f1 captured as requests without execution. All unit/persistent bytes and RNG invariant. Does not prove recursive attack results, animation or XP application.')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_counters.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'original counter gates')
