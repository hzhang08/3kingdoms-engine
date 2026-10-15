#!/usr/bin/env python3
"""Original opcode92 scheduler, first-active-slot lookup, and target mask.

Typed argument reader and original lookup/waits execute; camera/panel/tile
requests are explicit presentation boundaries. The target mask routine executes
separately with original buffer accessors. No unit or RNG rule is replaced.
"""
import argparse,json,struct
from pathlib import Path
from battle_events_probe import prepare,LIVE,CTRL
from deployment_probe import script_call
from movement_probe import live_json
from executable_probe import EDITION

CAMERA=0x4b4250
HOOKS={0x45503f,0x4404bd,0x453a40,0x43fcc9,0x403340,0x42c4e0,0x453e32}

def configure(v,events):
    v.ranges += [(0x416e17,0x416fd8),(0x44049e,0x4404bd)]
    def record(kind,argc=0,pop=0,actor=False):
        def hook(vm):
            row=dict(kind=kind,args=[vm.arg(n)&255 for n in range(argc)])
            if actor:row['slot']=(vm.this()-LIVE)//36
            events.append(row);vm.return_from_hook(pop=pop)
        return hook
    def camera(vm):events.append(dict(kind='camera_request',x=vm.arg(0)&255,y=vm.arg(1)&255));vm.return_from_hook(pop=8)
    def tile(vm):events.append(dict(kind='unit_redraw',unit=vm.u32(vm.this())));vm.return_from_hook()
    def wait(vm):events.append(dict(kind='wait',ticks=vm.arg(0)));vm.return_from_hook(pop=4)
    v.hooks.update({0x45503f:camera,0x4404bd:record('selection_panel',3,12,True),
        0x453a40:record('selection_target',1,4),0x453e32:record('selection_clear_panel'),
        0x43fcc9:tile,0x403340:wait,0x42c4e0:lambda vm:vm.return_from_hook()})

def generate(source):
    _,v,_,_=prepare(source);events=[];configure(v,events)
    initial=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    specs=[dict(id=uid) for uid in (146,147,157,0,511,512,65535)]
    specs += [dict(id=146,lifecycle=n) for n in (0,1,2,3,4)]
    specs += [dict(id=146,duplicate_lifecycle=n) for n in (1,2,3)]
    for spec in specs:
        v.write(0x400000,initial);v.write(0x800000,scratch);events.clear()
        slot=next(n for n in range(115) if v.u32(LIVE+36*n)==146)
        if 'lifecycle' in spec:v.write(LIVE+36*slot+12,bytes([spec['lifecycle']]))
        if 'duplicate_lifecycle' in spec:
            v.write(LIVE,v.read(LIVE+36*slot,36));v.write(LIVE+4,b'\0');v.write(LIVE+12,bytes([spec['duplicate_lifecycle']]))
        units=[live_json(v,n) for n in range(115) if v.u32(LIVE+36*n)<512]
        before=(v.read(LIVE,115*36),v.read(0x4a1b68,512*72),v.u32(0x48ecd4),v.read(CTRL,24))
        v.hits.clear();result=script_call(v,0x416e17,struct.pack('<HHH',92,2,spec['id']))
        assert before==(v.read(LIVE,115*36),v.read(0x4a1b68,512*72),v.u32(0x48ecd4),v.read(CTRL,24))
        assert not(v.hits.intersection(v.hooks)-HOOKS),[hex(a) for a in v.hits.intersection(v.hooks)-HOOKS]
        cases.append(dict(input=spec,units=units,result=result,events=list(events),state_unchanged=True))
    # Verify the sentinel and out-of-map guards in the actual mask routine.
    del v.hooks[0x453a40]
    v.ranges += [(0x453a40,0x453b13),(0x4065a0,0x4065c0),(0x47fd13,0x47fd40)]
    v.put32(0x4af4a8+4,1600);v.put32(0x4af4a8+12,0x82a000);v.write(0x4af4a8+16,b'\x01')
    masks=[]
    for slot in (0,255):
      for x,y in ((0,0),(19,19),(20,0),(0,20),(255,255),(9,11)):
        v.write(LIVE+6,bytes([x,y]));v.write(0x82a000,b'\x07'*1600)
        before=v.read(LIVE,115*36),v.u32(0x48ecd4)
        v.hits.clear();v.call(0x453a40,[slot],CAMERA)
        assert not(v.hits.intersection(v.hooks)),[hex(a) for a in v.hits.intersection(v.hooks)]
        assert before==(v.read(LIVE,115*36),v.u32(0x48ecd4))
        masks.append(dict(slot=slot,position=[x,y],width=20,height=20,mask=list(v.read(0x82a000,400)),style=v.read(CAMERA+0x1b85,1)[0]))
        assert v.read(0x82a000+400,1200)==bytes(1200)
    return dict(executable_sha256=EDITION,cases=cases,masks=masks,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_highlight.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['cases']),'highlight sequences;',len(r['masks']),'target masks')
