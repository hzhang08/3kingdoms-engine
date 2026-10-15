#!/usr/bin/env python3
"""Original attack camera destination and panel avoidance, no raster hooks.

Camera scroll destination is captured at 0x454cef; actual scrolling is covered
by the separate presentation oracle. Panel placement runs unhooked.
"""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from executable_probe import EDITION

def generate(source):
    _,v,_,_=prepare(source)
    v.ranges += [(0x44fc4b,0x450036),(0x455123,0x455298),(0x451a93,0x451ab5),(0x455e44,0x455e92),(0x47fad0,0x47fae7)]
    v.put32(0x4b4264,624);v.put32(0x4b4268,384);v.put32(0x4b4254,48);v.put32(0x4b4258,576)
    panels=[];cameras=[];requests=[]
    def scroll(vm):requests.append([vm.arg(0)&255,vm.arg(1)&255]);vm.return_from_hook(pop=8)
    v.hooks[0x454cef]=scroll
    for y in range(8):
     for x in range(13):
      for oy in range(8):
       for ox in range(13):
        v.write(LIVE+20*36+6,bytes([ox+1,oy+12]));v.hits.clear()
        v.call(0x44fc4b,[0x820f00,(x+1)|((y+12)<<8),4,2,20])
        assert not v.hits.intersection(v.hooks)
        panels.append(dict(target=[x+1,y+12],other=[ox+1,oy+12],camera=[1,12],result=list(v.read(0x820f00,2))))
    for y in range(20):
     for x in range(20):
      for tx,ty in ((0,0),(19,0),(0,19),(19,19),(9,15),(x,y),(max(0,x-8),max(0,y-5)),(min(19,x+8),min(19,y+5))):
        v.write(LIVE+38*36+6,bytes([tx,ty]));requests.clear();v.hits.clear();v.call(0x455123,[x,y,38],0x4b4250)
        assert v.hits.intersection(v.hooks)=={0x454cef}
        cameras.append(dict(actor=[x,y],target=[tx,ty],result=requests[0]))
    return dict(executable_sha256=EDITION,panels=panels,cameras=cameras,boundaries=__doc__)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_attack_camera.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['panels']),'placements;',len(r['cameras']),'camera destinations')
