#!/usr/bin/env python3
"""Original walking frame/timing/composition requests at raster boundaries."""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from ai_decision_probe import configure as ai_configure
from movement_probe import live_json
from executable_probe import EDITION
from ls12 import decode

def configure(v):
    ai_configure(v)
    v.ranges += [(0x44201b,0x442603),(0x451a93,0x451ab5),(0x455e44,0x455e92),
      (0x43e515,0x43e562),(0x43e320,0x43e331),(0x41c310,0x41c321),(0x40e090,0x40e0a1),(0x41edf7,0x41eec0)]
    v.put32(0x4aab80+4,115*0x6300);v.put32(0x4aab80+12,0x900000);v.write(0x4aab80+16,b'\x01')
    v.uc.mem_map(0x900000,0x300000);v.uc.mem_map(0xc00000,0x4000)
    v.put32(0x4af490+4,0x4000);v.put32(0x4af490+12,0xc00000);v.write(0x4af490+16,b'\x01')
    archives={b.index:b.data for b in decode((v.source/'Unit_mov.e5').read_bytes())};atlases={};pixels=bytearray(115*0x6300)
    def mirror(frame):return b''.join(frame[y*48:(y+1)*48][::-1] for y in range(48))
    for slot in range(115):
        atlas=archives[v.call(0x43fc48,this=LIVE+slot*36)];atlases[slot]=atlas;loaded=bytearray(atlas)
        if v.read(LIVE+slot*36+13,1)[0]&8:
            for frame in (4,5,8):loaded[frame*0x900:(frame+1)*0x900]=mirror(atlas[frame*0x900:(frame+1)*0x900])
        pixels[slot*0x6300:(slot+1)*0x6300]=loaded
    v.walking_pixels=bytes(pixels);v.write(0x900000,v.walking_pixels)
    del v.hooks[0x41ee53] # Run the original pixel mirror, not just its flag.
    events=[]
    def noop(vm):vm.return_from_hook()
    def copy(vm):vm.write(vm.arg(1),vm.read(vm.arg(0),vm.arg(2)));vm.return_from_hook()
    def wait(vm):events.append(dict(kind='wait',ticks=vm.arg(0)));vm.return_from_hook(pop=4)
    def camera(vm):
        x,y,animate=[vm.arg(n) for n in range(3)];events.append(dict(kind='camera_set',x=x,y=y,animate=animate))
        vm.put32(0x4b4254,x*48);vm.put32(0x4b4258,y*48);vm.return_from_hook(pop=12)
    def draw(vm):
        x,y,w,h,slot,ptr=[vm.arg(n) for n in range(6)];slot&=255
        frame=(ptr-0x900000-slot*0x6300)//0x900
        assert 0<=frame<11 and (ptr-0x900000)%0x900==0
        actual=vm.read(ptr,w*h);base=atlases[slot][frame*0x900:(frame+1)*0x900]
        flipped=frame in (4,5,8) and bool(v.read(LIVE+slot*36+13,1)[0]&8)
        assert actual==(mirror(base) if flipped else base),'original walking pixels do not match the selected frame'
        events.append(dict(kind='frame',x=x,y=y,width=w,height=h,slot=slot,frame=frame,mirror=flipped))
        vm.return_from_hook()
    def overlay(vm):events.append(dict(kind='overlay',x=vm.arg(0),y=vm.arg(1),slot=vm.arg(2)));vm.return_from_hook()
    def final(vm):events.append(dict(kind='final_tile',group=vm.arg(0)&255,x=vm.arg(1)&255,y=(vm.arg(1)>>8)&255));vm.return_from_hook()
    v.hooks.update({0x454b7e:camera,0x45137c:draw,0x403340:wait,0x43eb1f:overlay,0x43f38b:final,
      0x47fcfd:copy,0x47e251:noop,0x41e5db:noop,0x41e5f6:noop,0x4036f9:noop,0x451913:noop,0x42c4e0:noop})
    return events

def generate(source):
    _,v,_,_=prepare(source);events=configure(v);cases=[]
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000)
    for job in range(53):
     for speed in range(3):
      for direction in range(4):
       for position,camera_xy in (((10,15),(4,12)),((4,12),(4,12)),((16,19),(4,12))):
        nx,ny=position[0]+(0,1,0,-1)[direction],position[1]+(-1,0,1,0)[direction]
        if not(0<=nx<20 and 0<=ny<20):continue
        v.write(0x400000,original);v.write(0x800000,scratch);v.write(0x900000,v.walking_pixels);events.clear()
        p=LIVE+20*36;v.write(p+6,bytes(position));v.write(0x4a1b68+234*72+0x2b,bytes([job]));v.write(0x4b0770+8,bytes([speed]))
        v.put32(0x4b4254,camera_xy[0]*48);v.put32(0x4b4258,camera_xy[1]*48);v.put32(0x4b4264,624);v.put32(0x4b4268,384)
        before=live_json(v,20);persistent=v.read(0x4a1b68,512*72);rng=v.u32(0x48ecd4)
        v.call(0x44201b,[direction],p)
        assert persistent==v.read(0x4a1b68,512*72) and rng==v.u32(0x48ecd4)
        cases.append(dict(battle=before,job=job,direction=direction,speed=speed,camera=camera_xy,occupancy=list(v.read(0x818c80,400)),events=list(events),after=live_json(v,20)))
    return dict(executable_sha256=EDITION,cases=cases,boundaries='Original 0x44201b frame addresses, direction/mirror flags, speed/timing, scrolling requests and overlay order. Original horizontal pixel mirror executes; every emitted frame is compared byte-for-byte with the decoded Unit_mov frame/orientation. Plain byte-copy helper supplied. Raster/background copies and host message pump omitted; camera_set captured and coordinates applied. No walking coordinate/occupancy update: caller owns those after each step.')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_movement_animation.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'original walking animations')
