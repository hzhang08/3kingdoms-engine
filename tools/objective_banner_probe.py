#!/usr/bin/env python3
"""Execute the original objective banner and its interruptible terminal pause.

Original row sweeps, text/shadow rectangles, byte wrapping and loop control
execute. Explicit Win32 client rectangle, text/raster, clock and input boundaries
are captured. No unit rules or RNG calls are replaced. This is not a Windows
font-pixel or wall-clock equivalence claim.
"""
import argparse,json,struct
from pathlib import Path
from battle_events_probe import prepare,LIVE,CTRL
from turn_banner_probe import configure as base_configure,HOOKS,CAMERA
from executable_probe import EDITION

def configure(v,events,client=(0,0,640,440),interrupt_after=0):
    base_configure(v,events,client)
    # A sequential caller may have earlier host-boundary replacements here.
    # This probe must execute the real pause loop and final full-view redraw.
    v.hooks.pop(0x42c646,None);v.hooks.pop(0x453b13,None)
    v.ranges += [(0x44bd27,0x44c186),(0x453b13,0x453b48),
                 (0x451dab,0x451dbd),(0x42c646,0x42c733)]
    def row(vm):
        events.append(dict(kind='banner_region',args=[vm.arg(n)&255 for n in range(4)],color=vm.read(CAMERA+0x6d,1)[0]))
        vm.return_from_hook(pop=16)
    def format_text(vm):
        raw=vm.read(vm.arg(1),128).split(b'\0')[0]
        raw=raw % vm.arg(2)
        vm.write(vm.arg(0),raw+b'\0');vm.return_from_hook(len(raw))
    def noop(vm):vm.return_from_hook()
    def outside(vm):vm.return_from_hook(0xffffffff)
    ticks=[0]
    def tick(vm):
        ticks[0]+=1;events.append(dict(kind='input_wait_tick',ticks=vm.arg(0)))
        vm.return_from_hook()
    def input_state(vm):vm.return_from_hook(int(bool(interrupt_after and ticks[0]>=interrupt_after)))
    v.hooks.update({0x45218f:row,0x820e40:format_text,0x42c557:noop,
        0x42c89c:noop,0x42c4cb:tick,0x42c56b:input_state,
        0x425c26:outside,0x425c37:outside})

def generate(source):
    _,v,_,_=prepare(source);events=[];configure(v,events)
    image=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    texts=['击毙张宝和张梁！','','第一行\n第二行','汉'*14+'字','A'*28+'B','\n']
    for viewport,client in [((624,384),(0,0,640,440)),((720,480),(0,0,800,600)),((960,720),(0,0,1024,768)),((672,432),(7,13,711,509))]:
      for text in texts:
       for limit in (20,255):
        v.write(0x400000,image);v.write(0x800000,scratch);events.clear();configure(v,events,client)
        v.put32(CAMERA+0x14,viewport[0]);v.put32(CAMERA+0x18,viewport[1]);v.write(CTRL+3,bytes([limit]))
        v.write(0x822000,text.encode('gbk')+b'\0')
        before=(v.read(LIVE,115*36),v.read(0x4a1b68,512*72),v.u32(0x48ecd4),v.read(CTRL,24))
        v.hits.clear();v.call(0x44bd27,[0x822000],CTRL)
        assert before==(v.read(LIVE,115*36),v.read(0x4a1b68,512*72),v.u32(0x48ecd4),v.read(CTRL,24))
        allowed=HOOKS|{0x42c557,0x42c89c,0x42c4cb,0x42c56b,0x425c26,0x425c37}
        assert not(v.hits.intersection(v.hooks)-allowed),[hex(a) for a in v.hits.intersection(v.hooks)-allowed]
        cases.append(dict(input=dict(viewport=viewport,client=client,text=text,limit=limit),events=list(events),state_unchanged=True))
    pauses=[]
    for interrupt in (0,1,2,19,20,21):
        events.clear();configure(v,events,interrupt_after=interrupt);v.call(0x42c646,[20])
        pauses.append(dict(interrupt_after=interrupt,ticks=len(events)))
    return dict(executable_sha256=EDITION,cases=cases,pauses=pauses,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_objective_banner.json'));a=p.parse_args()
    result=generate(a.source);a.output.write_text(json.dumps(result,ensure_ascii=False,separators=(',',':'))+'\n');print(len(result['cases']),'banners;',result['pauses'])
