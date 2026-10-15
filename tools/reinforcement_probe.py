#!/usr/bin/env python3
"""Original opcode 76 with actual placement and lifecycle/mirror changes."""
import argparse,json,struct
from pathlib import Path
from battle_events_probe import prepare,LIVE,READER
from deployment_probe import script_call,slots
from executable_probe import EDITION

def generate(source):
    initial,v,raw,_=prepare(source)
    v.ranges += [(0x414f42,0x4150e7),(0x4570ee,0x457428),(0x43f040,0x43f38b)]
    events=[]
    def draw(vm):
        b=vm.read(vm.this(),36);events.append(dict(kind='tile',live=list(b),visible=vm.read(0x818c80+b[7]*20+b[6],1)[0]!=255));vm.return_from_hook()
    def camera(vm):events.append(dict(kind='camera_request',x=vm.arg(0)&255,y=vm.arg(1)&255));vm.return_from_hook(pop=8)
    def sound(vm):events.append(dict(kind='sound',id=vm.arg(0),flag=vm.arg(1)));vm.return_from_hook(pop=8)
    def wait(vm):events.append(dict(kind='wait',ticks=vm.arg(0)));vm.return_from_hook(pop=4)
    v.hooks.update({0x43fcc9:draw,0x45503f:camera,0x47467a:sound,0x403340:wait,0x44a88a:lambda vm:vm.return_from_hook(),0x42c4e0:lambda vm:vm.return_from_hook()})
    image=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    for mode,uid,slot in ((0,32,0),(0,3,0),(0,33,0),(0,0,0),(1,-1,0),(0,511,0)):
        for lifecycle in (1,2,3):
            for blocked in (False,True):
                v.write(0x400000,image);v.write(0x800000,scratch);events.clear()
                found=next((r for r in initial['battle'] if r['unit']['id']==(uid if mode==0 else 0)),None)
                if found:
                    v.write(LIVE+found['slot']*36+12,bytes([lifecycle]));d=next(r for r in slots(v) if r['slot']==found['slot'])
                    if blocked:v.write(0x818c80+d['y']*20+d['x'],b'\x23')
                before=[list(v.read(LIVE+r['slot']*36,36)) for r in initial['battle']]
                occupancy=list(v.read(0x818c80,400));persistent=v.read(0x4a1b68,512*72)
                wire=struct.pack('<HHHHHHi',76,64,mode,2,uid&65535,4,slot)
                script_call(v,0x414f42,wire)
                assert v.read(0x4a1b68,512*72)==persistent
                cases.append(dict(args=[mode,uid,slot],before=before,occupancy=occupancy,after=[list(v.read(LIVE+r['slot']*36,36)) for r in initial['battle']],events=list(events)))
    return dict(executable_sha256=EDITION,descriptors=slots(v),cases=cases,
        boundaries='Original typed opcode-76 handler, unit lookup, descriptor lookup, placement search, lifecycle, mirror flags and occupancy update execute. Tile redraw state, camera requests, sound and wait captured; Win32 minimap, pixel transforms and message pumping omitted. Special apparition units/classes are not exercised.')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_reinforcements.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,indent=2)+'\n');print(len(r['cases']),'reinforcement cases')
