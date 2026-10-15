#!/usr/bin/env python3
"""Original 0x405889 strike application, up to an automatic-item request.

HP/MP setters, equipment XP, status setters and RNG execute original code.
Numeric popups and equipment level windows are captured display boundaries.
An automatic item is an explicit suspension, not a stubbed success; its item
controller/effects and the later flag clear have not executed in those cases.
"""
import argparse,json,random
from pathlib import Path
from battle_events_probe import prepare,LIVE
from initialization_probe import state,UNIT
from movement_probe import live_json
from experience_probe import configure as xp_configure
from strike_probe import CONTROL
from executable_probe import EDITION

class ItemBoundary(Exception):pass

def configure(v,events):
    xp_configure(v,events)
    v.ranges += [(0x405889,0x405c6c),(0x43f6cd,0x43f74f),
      (0x43ff54,0x43ff6e),(0x472840,0x472851),(0x406650,0x4066ea),
      (0x40d9c4,0x40d9ee),(0x40db3e,0x40db83)]
    def numbers(vm):
        events.append(dict(kind='numbers',target=vm.arg(0)&255,actor=vm.arg(1)&255,
          hp=vm.arg(2),mp=vm.arg(3),xp=vm.arg(4),weapon_xp=vm.arg(5),armor_xp=vm.arg(6),wait=bool(vm.arg(7)),
          target_before=live_json(vm,vm.arg(0)&255)))
        vm.return_from_hook()
    def item(vm):
        events.append(dict(kind='automatic_item',item=vm.arg(0)&255,target=vm.arg(1)&255,
          argument=vm.arg(2),free=bool(vm.arg(3))))
        raise ItemBoundary()
    v.hooks.update({0x450984:numbers,0x41dc43:item})

def generate(source):
    _,v,_,_=prepare(source);events=[];configure(v,events)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000)
    cases=[];rng=random.Random(0x405889)
    def reset():v.write(0x400000,original);v.write(0x800000,scratch);events.clear()
    def capture(actor,targets,damage,mp_damage,armor_xp,seed=1,area=0,beans=0):
        v.write(CONTROL,b'\0'*0x614);uid=v.u32(LIVE+actor*36)
        v.write(CONTROL,bytes([actor,targets[0]]));v.put32(CONTROL+4,uid);v.put32(CONTROL+8,UNIT+uid*72);v.put32(CONTROL+12,LIVE+actor*36)
        v.write(CONTROL+0x10,bytes(targets+[255]));v.write(CONTROL+0x424,bytes([area]))
        for offset,values in ((0x84,damage),(0x254,mp_damage),(0x430,armor_xp)):
            for i,x in enumerate(values):v.put32(CONTROL+offset+i*4,x)
        v.write(0x4b0770+0x26b,bytes([beans]));v.put32(0x48ecd4,seed)
        def records():return [dict(battle=live_json(v,t),unit=state(v,v.u32(LIVE+t*36))) for t in targets]
        c=dict(actor=live_json(v,actor),actor_unit=state(v,uid),targets=records(),hp_damage=damage,mp_damage=mp_damage,armor_xp=armor_xp,seed=seed,area=area,beans=beans)
        before_actor=state(v,uid),live_json(v,actor);inventory=v.read(0x4b0770,0x2e0);v.hits.clear();pending=False
        try:v.call(0x405889,this=CONTROL)
        except ItemBoundary:pending=True
        assert before_actor==(state(v,uid),live_json(v,actor)) and inventory==v.read(0x4b0770,0x2e0)
        assert not (v.hits.intersection(v.hooks)-{0x450984,0x4083ef,0x41dc43})
        c.update(after=records(),events=list(events),pending_item=pending,final_rng=v.u32(0x48ecd4));cases.append(c)
    reset();capture(20,[38],[45],[0],[0],662824084)
    # Every equipment effect in each attacking slot.
    for item in range(104):
     for slot in range(3):
        reset();v.write(UNIT+234*72+0x2e,b'\xff'*9);v.write(UNIT+234*72+0x2e+slot*3,bytes([item,9,0]))
        v.write(LIVE+35*36+13,b'\xff');v.write(LIVE+35*36+30,bytes([item%32]))
        capture(20,[35],[1 if item%3 else 0],[1 if item%3==0 else 0],[100],rng.getrandbits(32),item%4)
    # Class effects, miss/MP-only/lethal hits, and already-present statuses.
    for job in range(53):
     for status in (0,255):
      for seed in (1,42,0xffffffff):
       for hp,mp in ((0,0),(0,1),(1,0),(500,0)):
        reset();v.write(UNIT+234*72+0x2b,bytes([job]));v.write(LIVE+35*36+30,bytes([status]));v.write(LIVE+35*36+13,b'\xff')
        v.put32(LIVE+35*36+16,500);v.put32(LIVE+35*36+20,1)
        capture(20,[35],[hp],[mp],[0],seed)
    # Original unsigned setters (overkill wraps, then caps), HP/MP equipment
    # maxima, equipment level requests, and automatic healing boundary gates.
    for item in range(104):
     for side in (0,1,2):
        reset();v.write(UNIT+146*72+0x2e,bytes([255]*3+[40,1,99]+[item,9,0]));v.write(LIVE+35*36+5,bytes([side]))
        v.put32(LIVE+35*36+16,100);v.put32(LIVE+35*36+20,50)
        capture(20,[35],[10],[3],[1],42,0,item%2)
    data=json.loads(Path('assets/generated/data.json').read_text());automatic=[x['id'] for x in data['items'] if x['type']==62]
    for item in automatic:
     for side in (0,1,2):
      for beans in (0,1,255):
       for hp in (0,1,100):
        reset();v.write(UNIT+146*72+0x34,bytes([item,1,0]));v.write(LIVE+35*36+5,bytes([side]));v.write(LIVE+35*36+13,b'\xff');v.put32(LIVE+35*36+16,100)
        capture(20,[35],[hp],[0],[0],1,0,beans)
    for hp,mp in ((101,51),(0xffffffff,0xffffffff),(100,50)):
        reset();v.put32(LIVE+35*36+16,100);v.put32(LIVE+35*36+20,50);capture(20,[35],[hp],[mp],[0])
    for job in (42,49,50):
     for seed in range(10):
        reset();v.write(UNIT+234*72+0x2b,bytes([job]));capture(20,[35,38,18],[1,0,2],[0,1,0],[0,100,200],seed,1)
    return dict(executable_sha256=EDITION,cases=cases,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_strike_application.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'cases;',sum(c['pending_item'] for c in r['cases']),'automatic-item suspensions')
