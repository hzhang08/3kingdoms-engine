#!/usr/bin/env python3
"""Original 0x473202 type-0 caption background and border raster requests.

Original tile selection, border coordinates and color sequence execute.
Raster writes are captured; backing-store copies and Win32 are not executed.
"""
import argparse,hashlib,json
from pathlib import Path
from probe_vm import ProbeVM
from executable_probe import EDITION

def generate(source):
    v=ProbeVM(source,[(0x473202,0x47349c),(0x47fa40,0x47fa9e)])
    v.write(0x800000,(v.source/'Mark.e5').read_bytes());v.put32(0x4aeb54,0x1c7e0);v.put32(0x4aeb5c,0x800000);v.write(0x4aeb60,b'\1')
    events=[];color=0
    for address in (0x41e5db,0x41e5f6,0x47cae3,0x47d1ca):v.hooks[address]=lambda vm:vm.return_from_hook()
    def style(vm):
        nonlocal color
        assert vm.arg(0)==4 and vm.arg(2)==0;color=vm.arg(1);vm.return_from_hook()
    def tile(vm):
        x,y,w,h,p=[vm.arg(n) for n in range(5)]
        events.append(dict(kind='caption_tile',x=x,y=y,width=w,height=h,offset=p-0x800000,pixels_sha256=hashlib.sha256(vm.read(p,w*h)).hexdigest()));vm.return_from_hook()
    def line(vm):
        events.append(dict(kind='caption_line',args=[vm.arg(n) for n in range(5)],color=color));vm.return_from_hook()
    v.hooks.update({0x47d1f1:style,0x47e09b:tile,0x47d4be:line})
    cases=[]
    for width in (96,144,192):
        events.clear();v.call(0x473202,[264,144,width,48,0,0],0x4b6930)
        cases.append(dict(rectangle=[264,144,width,48],events=list(events)))
    return dict(executable_sha256=EDITION,cases=cases,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_caption_window.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'original caption windows')
