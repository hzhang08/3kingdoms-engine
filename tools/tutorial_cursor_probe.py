#!/usr/bin/env python3
"""Original camera-scroll cursor lifetime, including unchanged destinations.

454cef and45461c execute unmodified. Camera placement and tile redraw are
captured host boundaries. No units, rules, assets or random state are altered.
"""
import argparse,json
from pathlib import Path
from probe_vm import ProbeVM
from executable_probe import EDITION

def generate(source):
 v=ProbeVM(source,[(0x454cef,0x454f10),(0x45461c,0x454709),(0x453b48,0x453b6d)])
 camera=0x4b4250;events=[]
 def place(vm):
  x,y=[vm.arg(n)&255 for n in range(2)];v.put32(camera+4,x*48);v.put32(camera+8,y*48)
  events.append(dict(kind='camera',tile=[x,y]));vm.return_from_hook(pop=12)
 def redraw(vm):events.append(dict(kind='redraw',args=[vm.arg(n)&255 for n in range(4)]));vm.return_from_hook(pop=16)
 v.hooks.update({0x454b7e:place,0x45218f:redraw,0x476882:lambda vm:vm.return_from_hook()})
 v.put32(camera+0x14,624);v.put32(camera+0x18,384);v.write(camera+0x61,b'\0');v.write(camera+0x54,b'\xff\xff')
 cases=[]
 for destination in ([3,0],[4,0],[3,2],[7,12]):
  for cursor in ([255,255],[8,4],[0,0]):
   v.put32(camera+4,144);v.put32(camera+8,0);v.write(camera+0x54,bytes(cursor));events.clear()
   units=v.read(0x4b2c50,115*36);persistent=v.read(0x4a1b68,512*72);rng=v.u32(0x48ecd4)
   v.call(0x454cef,destination,camera)
   after=list(v.read(camera+0x54,2));assert after==[255,255]
   assert units==v.read(0x4b2c50,115*36) and persistent==v.read(0x4a1b68,512*72) and rng==v.u32(0x48ecd4)
   cases.append(dict(start=[3,0],destination=destination,before_cursor=cursor,after_cursor=after,events=list(events)))
 return dict(executable_sha256=EDITION,cases=cases,boundaries=__doc__)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_tutorial_cursor.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,indent=2)+'\n');print(len(r['cases']),'original camera cursor-clear cases')
