#!/usr/bin/env python3
"""Original opening removal and facing handlers, with rendering/audio boundaries."""
import argparse,json,struct
from pathlib import Path
from battle_initialization_probe import probe
from deployment_probe import script_call
from executable_probe import EDITION
from eex import parse

LIVE=0x4b2c50

def generate(source):
    initial,v=probe(source,animation_state=True)
    v.uc.mem_map(0,0x1000)
    v.ranges += [(0x41557b,0x415771),(0x45a7b7,0x45a80f),(0x441744,0x44197b),
        (0x4419b6,0x441b06),(0x441ff9,0x44201b),(0x442abb,0x442b03),
        (0x44fa1f,0x44faf1),(0x451a93,0x451ab5),(0x472f30,0x472f41),
        (0x4032b0,0x4032e3),(0x403320,0x403337),(0x418f49,0x418f67),(0x41a190,0x41a1b0),
        (0x41c310,0x41c321),(0x425ad0,0x425ae1),
        (0x415f65,0x416129),(0x457428,0x45777f),(0x4351da,0x435884),
        (0x43e320,0x43e331),(0x403360,0x403375),(0x41c2b0,0x41c2c1),
        (0x454f10,0x45503f)]
    events=[]
    def noop(vm):vm.return_from_hook()
    def draw(vm):
        p=vm.this();b=vm.read(p,36)
        # Occupancy is the original map's visible-unit source, not lifecycle.
        occupancy=v.read(0x818c80+b[7]*20+b[6],1)[0]
        events.append(dict(kind='tile',unit=vm.u32(p),occupied=occupancy,live=list(b)))
        vm.return_from_hook()
    def sound(vm):events.append(dict(kind='sound',id=vm.arg(0),flag=vm.arg(1)));vm.return_from_hook(pop=8)
    def wait(vm):events.append(dict(kind='wait',ticks=vm.arg(0)));vm.return_from_hook(pop=4)
    def camera(vm):events.append(dict(kind='camera_request',x=vm.arg(0)&255,y=vm.arg(1)&255));vm.return_from_hook(pop=8)
    def scroll(vm):events.append(dict(kind='camera_scroll',x=vm.arg(0)&255,y=vm.arg(1)&255));vm.return_from_hook(pop=8)
    v.hooks.update({0x43fcc9:draw,0x47467a:sound,0x403340:wait,0x45503f:camera,
        0x454cef:scroll,0x42c4e0:noop,0x44a88a:noop,0x4192c5:noop,0x418f49:noop})
    v.put32(0x4b4254,48);v.put32(0x4b4258,576);v.put32(0x4b4264,624);v.put32(0x4b4268,384)
    original_live=v.read(LIVE,115*36);original_units=v.read(0x4a1b68,512*72);original_map=v.read(0x818c80,1600)
    def reset():
        v.write(LIVE,original_live);v.write(0x4a1b68,original_units);v.write(0x818c80,original_map);events.clear()
    def rows():return [list(v.read(LIVE+r['slot']*36,36)) for r in initial['battle']]
    def walk(body):
        for i in body:
            if i['opcode']==1:yield from walk(i['body'])
            else:yield i
    raw=(source/'S_00.eex').read_bytes();script=[]
    for i in walk(parse(source/'S_00.eex')['sections'][0]['events'][0]['body']):
        if i['opcode'] not in (79,83):continue
        reset();before=rows();script_call(v,{79:0x415f65,83:0x41557b}[i['opcode']],raw[i['offset']:i['end']])
        assert v.read(0x4a1b68,512*72)==original_units
        script.append(dict(offset=i['offset'],opcode=i['opcode'],args=[a['value'] for a in i['args']],before=before,after=rows(),events=list(events)))
    reset();chain=[]
    for i in walk(parse(source/'S_00.eex')['sections'][0]['events'][0]['body']):
        if i['opcode'] not in (79,83):continue
        before=rows();events.clear();script_call(v,{79:0x415f65,83:0x41557b}[i['opcode']],raw[i['offset']:i['end']])
        chain.append(dict(offset=i['offset'],opcode=i['opcode'],args=[a['value'] for a in i['args']],before=before,after=rows(),events=list(events)))
    directions=[];reset()
    # Every pair of relative positions, both tie orientations and all old directions.
    a=LIVE+18*36;b=LIVE+20*36
    v.write(a+6,bytes([10,10]))
    for y in range(20):
        for x in range(20):
            if (x,y)==(10,10):continue
            v.write(b+6,bytes([x,y]))
            for direction in range(4):
                v.write(a+15,bytes([direction]));value=v.call(0x4351da,[18,20,0,0,0])
                directions.append(dict(from_xy=[10,10],to_xy=[x,y],old=direction,direction=value))
    facing=[]
    for target in (234,146,65535,32,511):
        for direction in range(4):
            for reverse in (0,1):
                for pre,post in ((0,0),(0,1),(1,0),(1,1)):
                    reset();before=rows();v.call(0x457428,[210,target,direction,reverse,pre,post],0x4b5df0)
                    facing.append(dict(args=[210,target,direction,reverse,pre,post],before=before,after=rows(),events=list(events)))
    removal=[]
    for uid in (235,334,32,511):
        for ai in range(5):
            reset();v.write(LIVE+18*36+8,bytes([23]));v.write(LIVE+18*36+14,bytes([ai]))
            before=rows();v.call(0x45a7b7,[uid,0,0],0x4b5df0)
            removal.append(dict(args=[uid,0,0],before=before,after=rows(),events=list(events)))
    # Lifecycle/status/mirror and self-target cleanup are independent of the opening defaults.
    for status in (0,7,255):
        for flags in (0,8,6,128,255):
            reset();ptr=LIVE+23*36;v.write(ptr+8,bytes([23]));v.write(ptr+13,bytes([flags,0]));v.write(ptr+30,bytes([status]))
            before=rows();v.call(0x45a7b7,[235,0,0],0x4b5df0)
            removal.append(dict(args=[235,0,0],before=before,after=rows(),events=list(events)))
    return dict(executable_sha256=EDITION,script=script,chain=chain,directions=directions,facing=facing,removal=removal,
        boundaries={'state':'Original script decoder, lifecycle, occupancy, direction/mirror flags and AI target release execute. Original persistent unit bytes checked unchanged for the opening script forms.',
        'presentation':'Camera requests, sound IDs, waits and original tile redraw states captured; Win32 unit window/minimap, pixel mirroring/copy, raster drawing and message pumping omitted. Only removal mode 0 on non-party units is covered.'})
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_battle_actions.json'));a=p.parse_args()
    result=generate(a.source);a.output.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n');print({k:len(result[k]) for k in ('script','directions','facing','removal')})
