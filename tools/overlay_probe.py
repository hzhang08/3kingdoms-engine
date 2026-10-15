#!/usr/bin/env python3
"""Original selection mask and tile redraw layering at raster boundaries."""
import argparse,json,hashlib
from pathlib import Path
from battle_events_probe import prepare
from range_probe import configure
from executable_probe import EDITION

def generate(source,extended=False):
    _,v,_,_=prepare(source);configure(v)
    v.ranges += [(0x452898,0x453b11),(0x451dbd,0x451e9e),(0x451ecd,0x452888),(0x453b6d,0x453c17)]
    for descriptor,buffer in ((0x4af4a8,0x82a000),(0x4aeb50,0x900000),(0x4ae4a8,0x930000)):
        if buffer>=0x900000:v.uc.mem_map(buffer,0x30000)
        v.put32(descriptor+4,0x30000);v.put32(descriptor+12,buffer);v.write(descriptor+16,b'\x01')
    v.write(0x900000,(source/'Mark.e5').read_bytes())
    camera=0x4b4250;v.put32(camera+4,0);v.put32(camera+8,0);v.put32(camera+0x3c,0);v.put32(camera+0x40,56)
    ranges=[];events=[]
    for kind in list(range(16))+[255]:
     for center in ((0,0),(19,19),(11,16),(9,11)):
      for include in (0,1):
        v.call(0x4539dc,[center[0]+center[1]*256,kind,include],camera)
        ranges.append(dict(center=center,kind=kind,include_center=include,mask=list(v.read(0x82a000,400))))
    def noop(vm):vm.return_from_hook()
    def icon(vm):
        pointer=vm.arg(4)
        if 0x900000<=pointer<0x930000:events.append(dict(kind='mark',offset=pointer-0x900000))
        else:events.append(dict(kind='map'))
        vm.return_from_hook()
    def checker(vm):
        pixels=vm.read(vm.arg(4),2304);color=max(pixels)
        assert pixels==bytes(color if x%2==y%2 else 0 for y in range(48) for x in range(48))
        events.append(dict(kind='checker',color=color));vm.return_from_hook()
    def unit(vm):events.append(dict(kind='unit',slot=vm.arg(2)&255));vm.return_from_hook()
    v.hooks.update({0x41e5db:noop,0x41e5f6:noop,0x47eac4:noop,0x47d1f1:noop,0x451913:icon,0x47e09b:checker,0x43eb1f:unit})
    cases=[]
    for style in range(4):
     for tile in ((6,4),(5,4),(5,3),(4,4),(3,4),(0,0)):
      for movement in (0,1):
       for reachable in (0,1):
        for marked in (0,1):
         for covered in (0,1):
          for blue in ((0,1) if extended else (0,)):
           for terrain_mark in ((255,3) if extended else (255,)):
            x,y=tile;index=y*20+x;events.clear();v.put32(camera+0x68,movement);v.put32(camera+0x64,blue)
            v.write(camera+0x54,bytes([6,4]));v.write(camera+0x61,bytes([style]));v.write(camera+0x6c,b'\xff\xff\xff');v.write(camera+0x1b85,b'\x00')
            v.write(camera+0x44,bytes([x,y,1,1]));v.put32(camera+0x50,covered)
            v.write(0x8192c0+index,bytes([0 if reachable else 255]));v.write(0x819f40+index,bytes([terrain_mark]));v.write(0x82a000+index,bytes([marked]));v.write(0x818c80+index,b'\x14')
            v.call(0x45218f,[x,y,1,1],camera)
            sample=dict(style=style,tile=tile,movement=movement,reachable=reachable,marked=marked,covered=covered,events=list(events))
            if extended:sample.update(blue=blue,terrain_mark=terrain_mark)
            cases.append(sample)
    return dict(executable_sha256=EDITION,ranges=ranges,cases=cases,boundaries='Original 0x4539dc/0x452898 UI mask painter, 0x45218f tile order, 0x451ecd cursor shapes, 0x453b6d popup exclusion, 0x451dbd checker pixels. Map/Mark blits and unit sprites captured at raster boundaries. No game-unit or RNG rules replaced.')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_overlay.json'));p.add_argument('--extended',action='store_true');a=p.parse_args();r=generate(a.source,a.extended);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['ranges']),'UI masks;',len(r['cases']),'tile layering cases')
