#!/usr/bin/env python3
"""Original turn-start grade/status rules, including RNG consumption.

Rules, equipment tests, neighbor scan, status setter and grade setters execute
original machine code. Only presentation/archive boundaries are replaced.
"""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from initialization_probe import state,UNIT
from executable_probe import EDITION

def generate(source):
    _,v,_,_=prepare(source)
    v.ranges += [(0x43c738,0x43d538),(0x41def0,0x41df0b),(0x41e030,0x41e041),
      (0x4066e0,0x406700),(0x407a09,0x407af2),(0x40709a,0x407112),
      (0x47fad0,0x47fb4c),(0x45b850,0x45b911),(0x418da0,0x418dc5),
      (0x409540,0x409551),(0x40c76e,0x40c7ac),(0x4032b0,0x4032e3),(0x403320,0x403337),
      (0x41c2b0,0x41c2c1),(0x44fa1f,0x44faf1),(0x451a93,0x451ab5)]
    changes=[];auras=[]
    def noop(vm):vm.return_from_hook()
    def change(vm):
        changes.append([vm.arg(n)&255 for n in range(3)])
        vm.return_from_hook()
    def camera(vm):
        x,y=vm.arg(0)&255,vm.arg(1)&255
        auras.append(v.read(0x818c80+y*20+x,1)[0]);vm.return_from_hook(pop=8)
    def archive(vm):vm.return_from_hook(1,pop=4)
    def load(vm):vm.return_from_hook(1,pop=8)
    def wait(vm):vm.return_from_hook(pop=4)
    # 0x43e69f displays the pre-mutation status/grade effect. The original
    # 0x45aa5c caller still performs the subsequent state mutation itself.
    v.hooks.update({0x43e69f:change,0x45503f:camera,0x41fb35:archive,0x41fc89:load,
      0x4192c5:noop,0x418f49:noop,0x45137c:noop,0x4036f9:noop,
      0x41e5db:noop,0x41e5f6:noop,0x42c4e0:noop,0x403340:wait})
    # Synthetic scenarios isolate one target in slot 0 and up to eight
    # neighbors; source asset data and item records remain the original bytes.
    for n in range(115):v.write(LIVE+n*36+12,b'\0')
    v.write(0x818c80,b'\xff'*1600)
    v.write(LIVE,bytes([0]*36));v.write(LIVE+6,bytes([10,10]));v.write(LIVE+24,bytes([3]*6))
    v.write(0x818c80+10*20+10,b'\0')
    initial=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000)
    items=json.loads(Path('assets/generated/data.json').read_text())['items']
    equipment=[(slot,item['id']) for slot in range(3) for item in items if
        (slot==2 and item['type']==20) or (slot<2 and item['type']<18 and item['type']%2==1 and item['special_type']==20)]
    equipment=[(-1,255)]+equipment
    cases=[]
    specs=[]
    for luck in (0,1,10,15,25,49,50,51,99,100,255):
      for seed in (0,1,42,0xffffffff):
       for equip in equipment:
        for grade in range(7):specs.append(dict(kind='grades',luck=luck,seed=seed,equip=equip,grades=[grade]*6,status=0,lifecycle=2))
        for status in (0,1,2,4,8,16,30,255):specs.append(dict(kind='status',luck=luck,seed=seed,equip=equip,grades=[3]*6,status=status,lifecycle=2))
    for life in (0,1,3,4):
     for kind in ('status','grades'):specs.append(dict(kind=kind,luck=50,seed=42,equip=(-1,255),grades=[0,1,2,4,5,6],status=30,lifecycle=life))
    # Position, side, lifecycle and job-group gates around an aura provider.
    for direction,(dx,dy) in enumerate(((0,-1),(1,0),(0,1),(-1,0),(1,-1),(1,1),(-1,1),(-1,-1),(0,-2))):
     for side in range(4):
      for life in (1,2,3):
       for job in (35,36,37,38,39):
        specs.append(dict(kind='status',luck=0,seed=42,equip=(-1,255),grades=[3]*6,status=30,lifecycle=2,neighbor=dict(x=10+dx,y=10+dy,side=side,lifecycle=life,job=job)))
    neighbor_cases=[s for s in specs if 'neighbor' in s]
    for spec in neighbor_cases:
        specs.append(dict(spec,neighbor=dict(spec['neighbor'],slot=35)))
    for seed in (0,1,42,0xffffffff):
        specs.append(dict(kind='grades',luck=50,seed=seed,equip=(-1,255),grades=[0,2,4,6,2,4],status=0,lifecycle=2))
    for status in (0,1,30,128,255):
        specs.append(dict(kind='status',luck=0,seed=42,equip=(-1,255),grades=[3]*6,status=status,lifecycle=2,
            neighbors=[dict(slot=n+1,x=10+dx,y=10+dy,side=n%4,lifecycle=2,job=36+n%3)
                       for n,(dx,dy) in enumerate(((0,-1),(1,0),(0,1),(-1,0),(1,-1),(1,1),(-1,1),(-1,-1)))]))
    for spec in specs:
        v.write(0x400000,initial);v.write(0x800000,scratch);changes.clear();auras.clear()
        v.write(LIVE+12,bytes([spec['lifecycle']]));v.write(LIVE+24,bytes(spec['grades']));v.write(LIVE+30,bytes([spec['status']]))
        v.write(UNIT+0x25,bytes([spec['luck']]));v.write(UNIT+0x2e,b'\xff'*9)
        slot,item=spec['equip']
        if slot>=0:v.write(UNIT+0x2e+slot*3,bytes([item,1,0]))
        neighbors=spec.get('neighbors',[spec['neighbor']] if 'neighbor' in spec else [])
        for n in neighbors:
            other_slot=n.get('slot',1);p=LIVE+other_slot*36;v.put32(p,other_slot);v.write(p+4,bytes([other_slot,n['side'],n['x'],n['y']]));v.write(p+12,bytes([n['lifecycle']]))
            v.write(UNIT+other_slot*72+0x2b,bytes([n['job']]))
            # Occupancy is explicit: all lifecycles tested are present in the
            # synthetic lookup to exercise the later visible-unit predicate.
            v.write(0x818c80+n['y']*20+n['x'],bytes([other_slot]))
        v.put32(0x48ecd4,spec['seed']);before=list(v.read(LIVE,36));unit=state(v,0)
        other_before=v.read(LIVE+36,114*36);records_before=v.read(UNIT,512*72)
        v.call(0x43c975 if spec['kind']=='grades' else 0x43c7d6,this=LIVE)
        assert v.read(LIVE+36,114*36)==other_before and v.read(UNIT,512*72)==records_before
        cases.append(dict(input=spec,unit=unit,before=before,after=list(v.read(LIVE,36)),rng=v.u32(0x48ecd4),changes=list(changes),auras=list(auras)))
    return dict(executable_sha256=EDITION,cases=cases,boundaries='Original 0x43c975/0x43c7d6, recovery chance, equipment tests, aura-neighbor scan, RNG and setters. Presentation-only status effects, aura sprite I/O, draw, camera and wait calls omitted or captured. No battle AI or full phase execution is claimed.')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_recovery.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'recovery cases')
