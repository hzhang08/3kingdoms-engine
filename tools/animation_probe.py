#!/usr/bin/env python3
"""Bounded original animation oracle. Captures indexed pixels before Win32 drawing.

Camera scrolling, audio output, drawing and waiting are explicit host boundaries;
this verifies animation sequencing, not those subsystems or a full game session.
"""
import argparse, hashlib, json
from pathlib import Path
from battle_initialization_probe import probe
from ls12 import decode
from executable_probe import EDITION


def generate(source,animations=(0,3,4,5,6,7,8,9)):
    initial,v=probe(source,animation_state=True)
    v.uc.mem_map(0,0x1000) # original SEH chain; no exception handlers are invoked
    v.uc.mem_map(0x900000,0x20000)
    v.put32(0x4ae4c8+4,0x20000);v.put32(0x4ae4c8+12,0x900000);v.write(0x4ae4c8+16,b'\x01')
    v.ranges += [(0x45778f,0x45912d),(0x4032b0,0x4032e3),(0x403320,0x403337),
                 (0x403360,0x403375),(0x451a93,0x451ab5),(0x41c2b0,0x41c2c1),
                 (0x43e320,0x43e331),(0x47fa9e,0x47fab9),
                 (0x41edf7,0x41eec0)]
    # Execute the original mirror loop, replacing the earlier initialization hook.
    del v.hooks[0x41ee53]
    archives={name.upper():{b.index:b.data for b in decode((source/name).read_bytes())}
              for name in ('Unit_atk.e5','Unit_spc.e5','Unit_mov.e5')}
    events=[];opened={};current={}
    def noop(vm):vm.return_from_hook()
    def camera(vm):
        events.append(dict(kind='camera_request',x=vm.arg(0)&255,y=vm.arg(1)&255))
        vm.return_from_hook(pop=8)
    def open_archive(vm):
        name=vm.read(vm.arg(0),32).split(b'\0')[0].decode('ascii')
        assert name in archives;opened[vm.this()]=name;vm.return_from_hook(1,pop=4)
    def load(vm):
        name=opened[vm.this()];index=vm.arg(0);data=archives[name][index]
        current.update(archive=name,sprite=index);vm.write(vm.arg(1),data);vm.return_from_hook(1,pop=8)
    def copy(vm):vm.write(vm.arg(1),vm.read(vm.arg(0),vm.arg(2)));vm.return_from_hook()
    def sound(vm):events.append(dict(kind='sound',id=vm.arg(0),flag=vm.arg(1)));vm.return_from_hook(pop=8)
    def wait(vm):events.append(dict(kind='wait',ticks=vm.arg(0)));vm.return_from_hook(pop=4)
    def draw(vm):
        x,y,w,h,slot,ptr=[vm.arg(n) for n in range(6)];pixels=vm.read(ptr,w*h)
        atlas=archives[current['archive']][current['sprite']];matches=[]
        for n in range(len(atlas)//(w*h)):
            frame=atlas[n*w*h:(n+1)*w*h]
            for mirror in (False,True):
                candidate=b''.join(frame[r*w:(r+1)*w][::-1] for r in range(h)) if mirror else frame
                if candidate==pixels:matches.append(dict(frame=n,mirror=mirror))
        assert matches,'unidentified original frame'
        events.append(dict(kind='frame',x=x,y=y,width=w,height=h,slot=slot&255,
            **current,matches=matches,pixels_sha256=hashlib.sha256(pixels).hexdigest()))
        vm.return_from_hook()
    def redraw(vm):events.append(dict(kind='redraw',rectangle=[vm.arg(n) for n in range(4)]));vm.return_from_hook(pop=16)
    def tile(vm):
        events.append(dict(kind='unit_redraw',unit=vm.u32(vm.this())));vm.return_from_hook()
    v.hooks[0x43fcc9]=tile
    v.hooks.update({0x45503f:camera,0x41fb35:open_archive,0x41fc89:load,
        0x4192c5:noop,0x418f49:noop,0x47fcfd:copy,0x47467a:sound,
        0x403340:wait,0x42c4e0:noop,0x45137c:draw,0x45218f:redraw})
    for address in (0x41e5db,0x41e5f6,0x4036f9,0x4519e6,0x451913):v.hooks[address]=noop
    cases=[]
    for uid in (474,235,334,477,234,147):
        row=next(r for r in initial['battle'] if r['unit']['id']==uid);ptr=0x4b2c50+row['slot']*36
        for direction in range(4):
            v.write(ptr+15,bytes([direction]))
            for animation in animations:
                for pre,post in ((0,0),(0,1),(1,0),(1,1)):
                    events.clear();current.clear();v.put32(0x4b4254,0);v.put32(0x4b4258,0)
                    rng=v.u32(0x48ecd4)
                    before=v.read(0x4b2c50,115*36)+v.read(0x4a1b68,512*72)
                    v.call(0x45778f,[uid,animation,pre,post],0x4b5df0)
                    assert before==v.read(0x4b2c50,115*36)+v.read(0x4a1b68,512*72),'animation mutated unit state'
                    assert v.u32(0x48ecd4)==rng,'animation changed RNG'
                    cases.append(dict(unit=uid,direction=direction,animation=animation,pre=pre,post=post,events=list(events)))
    return dict(executable_sha256=EDITION,routine='0x45778f',tick_ms=50,
        boundaries={'camera':'request captured; scrolling and viewport configuration not emulated',
        'archive':'read-only LS12 decoder supplies original indexed sprite bytes',
        'drawing':'indexed pixels captured before Win32 composition; background painting omitted',
        'sound':'original sound IDs and flags captured; audio output omitted',
        'wait':'original tick counts captured; message pumping omitted',
        'copy':'byte copy hook; horizontal mirroring executes original machine code'},cases=cases)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_animations.json'));p.add_argument('--animations',type=int,nargs='+',default=[0,3,4,5,6,7,8,9]);a=p.parse_args()
    result=generate(a.source,a.animations);a.output.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n');print(len(result['cases']),'original animation cases verified')
