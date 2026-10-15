#!/usr/bin/env python3
"""Read-only original command-menu callbacks at explicit Win32 boundaries.

Window rectangles and client origins are supplied host inputs, not claims about
Windows font metrics. Executes original menu initialization, placement, enabled
controls, command-result writes and cancellation. No game action is substituted.
"""
import argparse,itertools,json,struct
from pathlib import Path
from probe_vm import ProbeVM
from executable_probe import EDITION

RANGES=[(0x45b930,0x45bd05),(0x45f650,0x45f680),(0x451a4f,0x451ab5),
        (0x455e44,0x455e7c),(0x46e81f,0x46e982),(0x435110,0x435144)]
DIALOG=0x4b5e98;CAMERA=0x4b4250

def host(v,size,client):
    v.ranges+=RANGES
    events=[]
    def get_rect(vm):vm.write(vm.arg(1),struct.pack('<4i',0,0,*size));vm.return_from_hook(1,8)
    def parent(vm):vm.return_from_hook(0x101,4)
    def to_screen(vm):
        ptr=vm.arg(1);x,y=struct.unpack('<2i',vm.read(ptr,8));vm.write(ptr,struct.pack('<2i',x+client[0],y+client[1]));vm.return_from_hook(1,8)
    def position(vm):events.append(dict(kind='position',args=[vm.arg(n) for n in range(7)]));vm.return_from_hook(1,28)
    def item(vm):vm.return_from_hook(0x10000+vm.arg(1),8)
    def enable(vm):events.append(dict(kind='enable',control=vm.arg(0)-0x10000,enabled=vm.arg(1)));vm.return_from_hook(1,8)
    def end(vm):events.append(dict(kind='end',hwnd=vm.arg(0),result=vm.arg(1)));vm.return_from_hook(1,8)
    for n,(iat,fn) in enumerate([(0x486398,get_rect),(0x48639c,parent),(0x486320,to_screen),(0x486280,position),(0x4862dc,item),(0x4862e8,enable),(0x4862d4,end)]):
        stub=0x83e000+n*16;v.put32(iat,stub);v.hooks[stub]=fn
    v.put32(DIALOG,0x4867c8);v.put32(DIALOG+4,0x100)
    return events

def generate(source):
    v=ProbeVM(source,[]);events=host(v,(110,154),(0,0))
    v.put32(CAMERA+0x3c,0);v.put32(CAMERA+0x40,56)
    v.put32(CAMERA+0x14,624);v.put32(CAMERA+0x18,384)
    v.put32(CAMERA+4,144);v.put32(CAMERA+8,0)
    cases=[]
    for flags in itertools.product((0,1),repeat=4):
        v.call(0x45bba0,[10,6],DIALOG)
        for n,f in enumerate(flags):v.put32(DIALOG+0x34+n*4,f)
        events.clear();v.call(0x45bcd0,this=DIALOG)
        cases.append(dict(flags=flags,events=list(events),stored_flags=[v.u32(DIALOG+0x34+n*4) for n in range(4)],result=v.read(DIALOG+0x28,1)[0]))
    commands=[]
    for control in (0x507,0x508,0x509,0x50a,0x50b,0x999):
        v.call(0x45bba0,[10,6],DIALOG);events.clear();v.call(0x45bc23,[0x111,control,0],DIALOG)
        commands.append(dict(control=control,result=v.read(DIALOG+0x28,1)[0],events=list(events)))
    events.clear();v.call(0x45bcea,this=DIALOG)
    cancel=dict(result=v.read(DIALOG+0x28,1)[0],events=list(events))
    # Original tutorial's actual setters, followed by the original init callback.
    v.call(0x45bba0,[10,6],DIALOG);v.call(0x435110,[1],DIALOG);v.call(0x435130,[1],DIALOG)
    events.clear();v.call(0x45bcd0,this=DIALOG)
    tutorial=dict(flags=[v.u32(DIALOG+0x34+n*4) for n in range(4)],events=list(events))
    layouts=[]
    for size in ((82,116),(110,154),(140,210),(81,115),(55,31),(140,511)):
      for client in ((0,0),(100,150)):
        events=host(v,size,client)
        for camera in ((0,0),(3,0),(7,12)):
          v.put32(CAMERA+4,camera[0]*48);v.put32(CAMERA+8,camera[1]*48)
          for tile in ((0,0),(6,4),(10,6),(19,19),(255,255)):
            v.call(0x45bba0,tile,DIALOG);events.clear();v.call(0x45bcd0,this=DIALOG)
            layouts.append(dict(size=size,viewport=[624,384],client=client,camera=camera,tile=tile,anchor=[v.u32(DIALOG+0x2c),v.u32(DIALOG+0x30)],events=list(events)))
    return dict(executable_sha256=EDITION,cases=cases,commands=commands,cancel=cancel,tutorial=tutorial,layouts=layouts,boundaries=__doc__)

if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_tutorial_menu.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['cases']),'enablement cases;',len(r['commands']),'command IDs;',len(r['layouts']),'host-supplied layouts;',r['tutorial'])
