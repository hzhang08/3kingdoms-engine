#!/usr/bin/env python3
"""Original 0x441488 selection sequence at explicit presentation boundaries."""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE,CTRL
from ai_decision_probe import configure as ai_configure
from movement_probe import live_json
from initialization_probe import state,UNIT
from executable_probe import EDITION

def generate(source):
    _,v,_,_=prepare(source);ai_configure(v)
    v.ranges += [(0x441488,0x441744),(0x44049e,0x4404bd),(0x406730,0x40675a)]
    events=[];poll=[0,-1]
    def record(kind,argc,pop,actor=False):
        def hook(vm):
            row=dict(kind=kind,args=[vm.arg(n)&255 for n in range(argc)])
            if actor:row['slot']=(vm.this()-LIVE)//36
            events.append(row);vm.return_from_hook(pop=pop)
        return hook
    def cursor(vm):events.append(dict(kind='cursor',args=list(vm.read(vm.arg(0),2))));vm.return_from_hook(pop=4)
    def redraw(vm):events.append(dict(kind='redraw',tick=vm.read(CTRL+5,1)[0]));vm.return_from_hook()
    def input_poll(vm):
        skip=poll[0]==poll[1];poll[0]+=1;vm.return_from_hook(int(skip))
    v.hooks.update({0x45503f:record('camera_request',2,8),0x4544d3:cursor,
      0x4404bd:record('panel',3,12,True),0x4350f0:record('movement_overlay',1,4),
      0x4539dc:record('range',3,12),0x453e32:record('clear_panel',0,0),
      0x454f10:record('focus_target',2,8),0x453a40:record('target',1,4),
      0x45461c:record('clear_cursor',0,0),0x453b13:redraw,0x42c56b:input_poll,
      0x42c646:record('wait',1,0)})
    # Range origin is a packed two-byte coordinate, unlike other byte args.
    def range_hook(vm):
        packed=vm.arg(0);events.append(dict(kind='range',args=[packed&255,(packed>>8)&255,vm.arg(1)&255,vm.arg(2)]));vm.return_from_hook(pop=12)
    v.hooks[0x4539dc]=range_hook
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    for mode,tactic in [(0,255),(1,255)]+[(2,n) for n in range(68)]:
     for moved,skip in ((False,-1),(True,-1),(False,0),(False,1)):
        v.write(0x400000,original);v.write(0x800000,scratch);events.clear();poll[:]=[0,skip]
        v.write(LIVE+20*36+13,bytes([4 if moved else 0]));v.write(CTRL+5,b'\x07')
        before=v.read(LIVE,115*36),v.read(UNIT,512*72),v.u32(0x48ecd4)
        target=255 if mode==0 else 35
        v.call(0x441488,[mode,target,tactic],LIVE+20*36)
        assert before==(v.read(LIVE,115*36),v.read(UNIT,512*72),v.u32(0x48ecd4))
        cases.append(dict(battle=live_json(v,20),unit=state(v,234),mode=mode,target=target,tactic=tactic,interrupt_at=skip,initial_tick=7,final_tick=v.read(CTRL+5,1)[0],events=list(events)))
    return dict(executable_sha256=EDITION,cases=cases,boundaries='Original selection scheduler and range/tactic getters, including tick increments and early input interruption. Presentation calls captured as typed requests; popup text/raster/highlight geometry and wait/input host implementations remain separate. No unit/RNG mutations. Range and movement overlays are not game movement commands.')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_selection.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'original selection sequences')
