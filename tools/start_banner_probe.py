#!/usr/bin/env python3
"""Original opcode90 battle-start banner, including outside-in then row sweeps.

Executes the real handler and caption routine; Win32 client/font/raster and
wait boundaries are captured. Unit records, controller and RNG are checked
unchanged. This fixture does not claim Windows glyph or wall-clock equivalence.
"""
import argparse,hashlib,json
from pathlib import Path
from battle_events_probe import prepare,LIVE,CTRL
from turn_banner_probe import configure as banner_configure,HOOKS,CAMERA
from executable_probe import EDITION

def configure(v,events,client=(0,0,640,440)):
    banner_configure(v,events,client)
    v.hooks.pop(0x453b13,None) # Execute the final full-viewport redraw wrapper.
    v.ranges += [(0x416c97,0x416cbb),(0x44bac2,0x44bd27),(0x453b13,0x453b48)]
    def region(vm):
        events.append(dict(kind='banner_cells',args=[vm.arg(n)&255 for n in range(4)],color=vm.read(CAMERA+0x6d,1)[0]));vm.return_from_hook(pop=16)
    v.hooks[0x45218f]=region

def generate(source):
    _,v,_,_=prepare(source);events=[];configure(v,events)
    initial=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    for viewport in ((624,384),(576,384),(720,480),(960,720)):
      for client in ((0,0,640,440),(7,13,647,493)):
        v.write(0x400000,initial);v.write(0x800000,scratch);events.clear();configure(v,events,client)
        v.put32(CAMERA+0x14,viewport[0]);v.put32(CAMERA+0x18,viewport[1]);v.write(CAMERA+0x54,b'\xff\xff')
        persistent=v.read(0x4a1b68,512*72);live=v.read(LIVE,115*36);control=v.read(CTRL,0x530);rng=v.u32(0x48ecd4)
        v.hits.clear();v.call(0x416c97)
        assert live==v.read(LIVE,115*36) and persistent==v.read(0x4a1b68,512*72) and control==v.read(CTRL,0x530) and rng==v.u32(0x48ecd4)
        assert not(v.hits.intersection(v.hooks)-HOOKS)
        cases.append(dict(viewport=viewport,client=client,events=list(events),checker_sha256=hashlib.sha256(v.read(CAMERA+0x96f,2304)).hexdigest(),final_color=v.read(CAMERA+0x6d,1)[0]))
    return dict(executable_sha256=EDITION,cases=cases,boundaries=__doc__)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_start_banner.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['cases']),'original battle-start banners')
