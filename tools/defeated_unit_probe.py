#!/usr/bin/env python3
"""Original non-party defeated-unit handler 0x441744, arguments (0,0).

The blink, occupancy, lifecycle/status/mirror cleanup and AI target release
execute originally. Tile raster, camera scroll, minimap, audio, sprite refresh
and host waits are presentation boundaries. Party retirement is not covered.
"""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE,CTRL
from initialization_probe import UNIT,state
from movement_probe import live_json
from executable_probe import EDITION

def configure(v,events=None):
    v.ranges += [(0x441744,0x441b06),(0x441ff9,0x44201b),(0x442abb,0x442b03),(0x44fa1f,0x44faf1),
      (0x451a93,0x451ab5),(0x472f30,0x472f41),(0x43ff54,0x4400a2),
      (0x4032b0,0x4032e3),(0x403320,0x403337),(0x418f49,0x418f67),(0x41a190,0x41a1b0),
      (0x41c310,0x41c321),(0x425ad0,0x425ae1),(0x43e320,0x43e331),
      (0x403360,0x403375),(0x41c2b0,0x41c2c1),(0x454f10,0x455123)]
    if events is None:events=[]
    def noop(vm):vm.return_from_hook()
    def draw(vm):
        b=live_json(vm,(vm.this()-LIVE)//36)
        events.append(dict(kind='tile',battle=b,visible=vm.read(0x818c80+b['y']*20+b['x'],1)[0]!=255));vm.return_from_hook()
    def sound(vm):events.append(dict(kind='sound',id=vm.arg(0),flag=vm.arg(1)));vm.return_from_hook(pop=8)
    def wait(vm):events.append(dict(kind='wait',ticks=vm.arg(0)));vm.return_from_hook(pop=4)
    def scroll(vm):
        x,y=[vm.arg(n)&255 for n in range(2)];events.append(dict(kind='camera',tile=[x,y]))
        vm.put32(0x4b4254,x*48);vm.put32(0x4b4258,y*48);vm.return_from_hook(pop=8)
    def minimap(vm):events.append(dict(kind='minimap'));vm.return_from_hook()
    v.hooks.update({0x43fcc9:draw,0x47467a:sound,0x403340:wait,0x454cef:scroll,
      0x42c4e0:noop,0x44a88a:minimap,0x4192c5:noop,0x418f49:noop})
    return events

def generate(source):
    _,v,_,_=prepare(source);events=configure(v)
    v.put32(0x4b4254,96);v.put32(0x4b4258,576);v.put32(0x4b4264,624);v.put32(0x4b4268,384)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    specs=[dict(slot=s,flags=f,status=st,ai=a) for s in (18,19,20,35,36,38) for f in (0,6,8,128,255) for st in (0,31) for a in range(5)]
    specs += [dict(slot=38,flags=0,status=0,ai=0,lifecycle=life) for life in (1,3)]
    for spec in specs:
        v.write(0x400000,original);v.write(0x800000,scratch);events.clear()
        slot=spec['slot'];p=LIVE+slot*36;v.put32(p+16,0);v.write(p+13,bytes([spec['flags']]))
        v.write(p+30,bytes([spec['status']]));v.write(p+12,bytes([spec.get('lifecycle',2)]))
        v.write(LIVE+37*36+8,bytes([slot]));v.write(LIVE+37*36+14,bytes([spec['ai']]))
        roster=[live_json(v,s) for s in range(115) if v.read(LIVE+s*36+12,1)[0] in (1,2,3)]
        uid=v.u32(p);assert state(v,uid)['owner']!=0
        persistent=v.read(UNIT,512*72);rng=v.u32(0x48ecd4);control=v.read(CTRL,0x530);v.hits.clear()
        v.call(0x441744,[0,0],p)
        assert persistent==v.read(UNIT,512*72) and rng==v.u32(0x48ecd4) and control==v.read(CTRL,0x530)
        assert not(v.hits.intersection(v.hooks)-{0x43fcc9,0x47467a,0x403340,0x454cef,0x42c4e0,0x44a88a,0x4192c5,0x418f49,0x41ee53,0x479a01,0x47fcfd}),[hex(a) for a in sorted(v.hits.intersection(v.hooks))]
        cases.append(dict(spec=spec,before=roster,after=[live_json(v,b['slot']) for b in roster],events=list(events),camera=[v.u32(0x4b4254),v.u32(0x4b4258)],rng=rng))
    return dict(executable_sha256=EDITION,cases=cases,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_defeated_unit.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'non-party defeat sequences')
