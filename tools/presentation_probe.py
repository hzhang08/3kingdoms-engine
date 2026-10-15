#!/usr/bin/env python3
"""Original camera progression and sound filename/API-flag oracle; no host GUI/audio."""
import argparse,json
from pathlib import Path
from battle_initialization_probe import probe
from executable_probe import EDITION


def generate(source):
    _,v=probe(source,animation_state=True)
    v.ranges += [(0x45503f,0x455123),(0x454f10,0x45503f),(0x454b7e,0x454f10),(0x47fad0,0x47fae7),
                 (0x45461c,0x454709),(0x453b13,0x453b48),
                 (0x474652,0x474a69),(0x44c38b,0x44c60e),(0x403360,0x403371)]
    camera=0x4b4250;events=[]
    v.put32(camera+0x14,624);v.put32(camera+0x18,384);v.write(camera+0x54,b'\xff\xff');v.write(camera+0x61,b'\x00')
    def noop(vm):vm.return_from_hook()
    def redraw(vm):
        events.append(dict(camera=[vm.u32(camera+4),vm.u32(camera+8)],rectangle=[vm.arg(n)&255 for n in range(4)]));vm.return_from_hook(pop=16)
    def no_window(vm):vm.return_from_hook(0,pop=4)
    v.put32(0x4862c8,0x83e000);v.hooks[0x83e000]=no_window
    for addr in (0x41e5db,0x41e5f6,0x476882):v.hooks[addr]=noop
    v.hooks[0x45218f]=redraw
    # No cursor selection; 0x453b48 should never execute in this input state.
    v.call(0x44c38b,[0x810000,1],0x4b5df0)
    focus=list(v.read(0x810000,2))
    cases=[]
    for start in ((0,0),(336,576),(0,560)):
        for y in range(20):
            for x in range(20):
                events.clear();v.put32(camera+4,start[0]);v.put32(camera+8,start[1])
                v.call(0x45503f,[x,y],camera)
                cases.append(dict(start=list(start),focus=[x,y],redraws=list(events),end=[v.u32(camera+4),v.u32(camera+8)]))
    visible=[]
    for start in ((0,0),(336,576),(48,576)):
        for y in range(20):
            for x in range(20):
                events.clear();v.put32(camera+4,start[0]);v.put32(camera+8,start[1]);v.write(0x4b2c50+6,bytes([x,y]))
                v.call(0x454f10,[0,0],camera)
                visible.append(dict(start=list(start),focus=[x,y],redraws=list(events),end=[v.u32(camera+4),v.u32(camera+8)]))
    sounds=[];calls=[]
    v.hooks[0x42a910]=lambda vm:vm.return_from_hook(0x1000)
    # Path resolution returns the still-live input string; a host filesystem is never touched.
    v.hooks[0x418ed4]=lambda vm:vm.return_from_hook(vm.arg(0))
    def play(vm):
        p=vm.arg(0);name=vm.read(p,30).split(b'\0')[0].decode('ascii') if p else None
        calls.append(dict(file=name,flags=vm.arg(1)));vm.return_from_hook(1,pop=8)
    v.put32(0x4863c8,0x83e100);v.hooks[0x83e100]=play
    for id in (6,7,8,15,16,17,19,30,31,32,33,34,35,36,37,39,112,119,129,131,201):
        for flag in (0,1):
            v.put32(0x4b69b0,0xffffffff);v.put32(0x4b69b4,0);calls.clear()
            v.call(0x47467a,[id,flag],0x4b69b0)
            sounds.append(dict(id=id,flag=flag,calls=list(calls)))
    script_sounds=[]
    for id,mode in ((201,0),(201,255),(36,1)):
        v.put32(0x4b69b0,0xffffffff);v.put32(0x4b69b4,0);calls.clear()
        v.call(0x474a07,[id,mode],0x4b69b0)
        script_sounds.append(dict(id=id,mode=mode,calls=list(calls)))
    return dict(executable_sha256=EDITION,viewport=[0,56,624,384],initial_focus=focus,camera=cases,visible_camera=visible,sounds=sounds,script_sounds=script_sounds,
        boundaries={'camera':'Original centering, step progression, pixel-camera setter and full redraw dispatch run; raster drawing, begin/end painting, cursor restoration and Win32 active-window check are hooked. No camera sleep is inserted by these routines.',
        'sound':'Original filename and sndPlaySound flags captured. Sound-enabled option supplied; host path lookup and sndPlaySound replaced. Playback quality and original message-pump timing are not tested.'})
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_presentation.json'));a=p.parse_args()
    result=generate(a.source);a.output.write_text(json.dumps(result,indent=2)+'\n');print(len(result['camera']),'camera cases,',len(result['sounds']),'sound cases; initial focus',result['initial_focus'])
