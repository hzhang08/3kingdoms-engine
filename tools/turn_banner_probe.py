#!/usr/bin/env python3
"""Original turn banner, sweep, text requests and action-flag reset.

The caller, checker generation, text-shadow rectangles and flag setter execute
original instructions. Window client rectangle is an explicit test input;
Windows font rasterization, column rasterization, waits and minimap are captured
boundaries. This does not establish original window dimensions or glyph pixels.
"""
import argparse, hashlib, json, struct
from pathlib import Path
from battle_events_probe import prepare, LIVE, CTRL
from executable_probe import EDITION

CAMERA=0x4b4250
HOOKS={0x47eac4,0x41e5db,0x41e5f6,0x42c4e0,0x403340,0x44a88a,
       0x45218f,0x41e9b4,0x47d239,0x820e20,0x820e40}

def configure(v, events, client=(0,0,640,440)):
    v.ranges += [(0x44ac28,0x44b047),(0x451cea,0x451dc0),
                 (0x453b48,0x453b7e),(0x41ecf8,0x41ed5c),
                 (0x455e44,0x455e93),(0x406690,0x4066b0)]
    v.put32(0x4862ec,0x820e20);v.put32(0x4862c0,0x820e40)
    color=[0]
    def string(vm,p):return vm.read(p,128).split(b'\0')[0]
    def noop(vm):vm.return_from_hook()
    def rect(vm):vm.write(vm.arg(1),struct.pack('<4i',*client));vm.return_from_hook(1,8)
    def format_text(vm):
        raw=string(vm,vm.arg(1))
        if b'%2u' in raw:raw=raw % vm.arg(2)
        assert b'%' not in raw
        vm.write(vm.arg(0),raw+b'\0');vm.return_from_hook(len(raw))
    def set_color(vm):
        assert vm.arg(0)==4 and vm.arg(2)==0
        color[0]=vm.arg(1);vm.return_from_hook()
    def text(vm):
        events.append(dict(kind='banner_text',rect=list(struct.unpack('<4i',vm.read(vm.arg(0),16))),
          width=vm.arg(1),height=vm.arg(2),text=string(vm,vm.arg(3)).decode('gbk'),
          color=color[0],center=vm.arg(4),opaque=vm.arg(5),length=vm.arg(6)))
        vm.return_from_hook()
    def column(vm):
        events.append(dict(kind='banner_column',args=[vm.arg(n)&255 for n in range(4)],color=vm.read(CAMERA+0x6d,1)[0]))
        vm.return_from_hook(pop=16)
    def wait(vm):events.append(dict(kind='wait',ticks=vm.arg(0)));vm.return_from_hook(pop=4)
    def minimap(vm):events.append(dict(kind='minimap'));vm.return_from_hook()
    v.hooks.update({0x47eac4:noop,0x41e5db:noop,0x41e5f6:noop,0x42c4e0:noop,
      0x403340:wait,0x44a88a:minimap,0x45218f:column,0x41e9b4:text,0x47d239:set_color,
      0x820e20:rect,0x820e40:format_text})

def generate(source):
    _,v,_,_=prepare(source);events=[];configure(v,events)
    initial=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    for viewport,client in [((624,384),(0,0,640,440)),((720,480),(0,0,800,600)),((960,720),(0,0,1024,768)),((624,384),(7,13,647,493))]:
      for enemy in (0,1):
       for preserve in (0,1):
        for turn in (1,19,20,21,255,256):
          v.write(0x400000,initial);v.write(0x800000,scratch);events.clear();configure(v,events,client)
          v.put32(CAMERA+0x14,viewport[0]);v.put32(CAMERA+0x18,viewport[1]);v.write(CTRL+3,b'\x14')
          for n in range(115):v.write(LIVE+n*36+13,bytes([(n*17+6)&255]))
          before=v.read(LIVE,115*36);persistent=v.read(0x4a1b68,512*72);rng=v.u32(0x48ecd4);tick=v.read(CTRL+5,1)[0]
          v.hits.clear();v.call(0x44ac28,[turn,enemy,preserve],CTRL)
          after=v.read(LIVE,115*36)
          assert all(after[n]==(before[n] if preserve or n%36!=13 else before[n]&~6) for n in range(len(before)))
          assert persistent==v.read(0x4a1b68,512*72) and rng==v.u32(0x48ecd4) and tick==v.read(CTRL+5,1)[0]
          assert not(v.hits.intersection(v.hooks)-HOOKS)
          cases.append(dict(input=dict(viewport=viewport,client=client,turn=turn,enemy=bool(enemy),preserve=bool(preserve),limit=20),
            events=list(events),before=list(before[13::36]),after=list(after[13::36]),
            checker_sha256=hashlib.sha256(v.read(CAMERA+0x96f,48*48)).hexdigest(),final_color=v.read(CAMERA+0x6d,1)[0]))
    return dict(executable_sha256=EDITION,cases=cases,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_turn_banner.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['cases']),'original turn banners')
