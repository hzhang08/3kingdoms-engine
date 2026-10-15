#!/usr/bin/env python3
"""Original poison and regeneration calculation fixtures.
The numeric display boundary is captured; original HP/MP setters execute.
XP-gain fixtures remain below level-up thresholds; no XP-level presentation
is claimed. Persistent records after the original call are retained as evidence.
"""
import argparse,json,struct
from pathlib import Path
from battle_events_probe import prepare,LIVE,CTRL
from initialization_probe import state,UNIT
from executable_probe import EDITION

def generate(source):
    _,v,_,_=prepare(source)
    v.ranges += [(0x43cddd,0x43d108),(0x44df61,0x44e0d9),(0x4066e0,0x406700),
      (0x435182,0x4351da),(0x435991,0x4359f5),(0x43ff54,0x43ff6e),
      (0x407a09,0x407c0e),(0x4082d6,0x4083ed),(0x409650,0x40969f),
      (0x408950,0x4089cb),(0x406650,0x406670),(0x409540,0x409551),
      (0x40c74c,0x40c7a9),(0x4072bd,0x4072fc)]
    events=[]
    def numeric(vm):
        args=[vm.arg(n) for n in range(8)]
        args[:2]=[x&255 for x in args[:2]]
        args[2:7]=[struct.unpack('<i',struct.pack('<I',x))[0] for x in args[2:7]]
        events.append(args);vm.return_from_hook()
    def camera(vm):vm.return_from_hook(pop=8)
    v.hooks.update({0x450984:numeric,0x45503f:camera})
    v.write(LIVE,b'\0'*(115*36));v.write(0x818640,b'\0'*400)
    base=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000)
    specs=[]
    # Original armor/accessory effects, both competing HP-recovery sources.
    for armor,accessory in ((255,255),(52,255),(255,75),(52,75),(255,77),(255,73),(255,79),(255,80)):
      for terrain in range(30):
       for hp,mp in ((0,0),(1,1),(40,20),(99,49),(100,50)):
        specs.append(dict(kind='regeneration',terrain=terrain,hp=hp,mp=mp,armor=armor,accessory=accessory,side=0,slot=0,lifecycle=2,max_hp=100))
    for life in (0,1,2,3,4):
     for slot in (0,34,35,114):
      for hp in (0,1,2,9,10,11,100):
       for maximum in (1,9,10,101,500):
        specs.append(dict(kind='poison',terrain=0,hp=min(hp,maximum),mp=0,armor=255,accessory=255,side=slot%4,slot=slot,lifecycle=life,max_hp=maximum))
    for side in (1,2,3):
     for accessory in (73,79,80):specs.append(dict(kind='regeneration',terrain=18,hp=40,mp=20,armor=255,accessory=accessory,side=side,slot=0,lifecycle=2,max_hp=100))
    for life in (0,1,3,4):specs.append(dict(kind='regeneration',terrain=18,hp=40,mp=20,armor=52,accessory=75,side=0,slot=0,lifecycle=life,max_hp=100))
    cases=[]
    for spec in specs:
        v.write(0x400000,base);v.write(0x800000,scratch);events.clear();v.load_unit(0)
        v.put32(UNIT+0x1c,spec['max_hp']);v.write(UNIT+0x20,b'\x32');v.write(UNIT+0x2c,b'\x01\0')
        v.write(UNIT+0x2e,bytes([0,1,0,spec['armor'],1,0,spec['accessory'],255,255]))
        slot=spec['slot'];p=LIVE+slot*36;v.put32(p,0);v.write(p+4,bytes([slot,spec['side'],10,10]));v.write(p+12,bytes([spec['lifecycle']]))
        v.put32(p+16,spec['hp']);v.put32(p+20,spec['mp']);v.write(0x818640+210,bytes([spec['terrain']]))
        if spec['kind']=='poison':v.write(p+30,b'\x10')
        before=state(v,0)
        if spec['kind']=='poison':v.call(0x44df61,[int(slot>=35)],CTRL)
        else:v.call(0x43cddd,this=p)
        cases.append(dict(input=spec,unit=before,after=state(v,0),hp=v.u32(p+16),mp=v.u32(p+20),numbers=list(events)))
    return dict(executable_sha256=EDITION,cases=cases,boundaries='Original poison/regeneration routines, equipment tests/strength selection, maximum HP/MP getters and record setters. Camera/display hooks only. XP samples below level-up boundaries; this fixture verifies numeric requests and HP/MP results, not level-up presentation.')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_maintenance.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'maintenance cases')
