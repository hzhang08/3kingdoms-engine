#!/usr/bin/env python3
"""Original AI decision and movement state through the action boundary."""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from initialization_probe import UNIT,state
from ai_decision_probe import MovementBoundary
from movement_animation_probe import configure
from movement_probe import live_json
from executable_probe import EDITION

def generate(source):
    _,v,_,_=prepare(source);events=configure(v)
    v.ranges += [(0x43a956,0x43aafb),(0x442abb,0x442b03)]
    def stop(vm):raise MovementBoundary()
    def camera(vm):
        # Focus is a separately verified presentation subsystem. Keep this
        # oracle's viewport fixed; step-level camera requests still execute.
        events.append(dict(kind='camera_request',x=vm.arg(0)&255,y=vm.arg(1)&255));vm.return_from_hook(pop=8)
    def selection(vm):
        events.append(dict(kind='selection',slot=(vm.this()-LIVE)//36,args=[vm.arg(n)&255 for n in range(3)]));vm.return_from_hook(pop=12)
    def redraw(vm):events.append(dict(kind='redraw'));vm.return_from_hook()
    v.hooks.update({0x45503f:camera,0x441488:selection,0x44a88a:redraw})
    v.put32(0x4b4254,48);v.put32(0x4b4258,576);v.put32(0x4b4264,624);v.put32(0x4b4268,384);v.write(0x4b0770+8,b'\x01')
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    for slot in (18,19,20,37,38,39,40,41,42,43,44,45,46,47,48):
     for ai in (1,2,3):
        v.write(0x400000,original);v.write(0x800000,scratch);v.write(0x900000,v.walking_pixels);events.clear();v.write(LIVE+slot*36+14,bytes([ai]));v.hooks[0x43a956]=stop
        try:v.call(0x43ad1c,this=LIVE+slot*36,budget=12000000)
        except MovementBoundary:pass
        else:raise RuntimeError('expected movement boundary')
        del v.hooks[0x43a956]
        b=live_json(v,slot);roster=[live_json(v,n) for n in range(115)];distance=list(v.read(0x8192c0,400));route=list(v.read(0x819900,256));destination=list(v.read(0x4b2c20,2));terrain=list(v.read(0x818640,400));persistent=v.read(UNIT,512*72);rng=v.u32(0x48ecd4);occupancy=list(v.read(0x818c80,400))
        v.call(0x43a956,this=LIVE+slot*36,budget=3000000)
        assert persistent==v.read(UNIT,512*72) and rng==v.u32(0x48ecd4)
        cases.append(dict(battle=b,unit=state(v,b['id']),roster=roster,width=20,height=20,camera=[1,12],speed=1,
          terrain=terrain,distance=distance,route=route,destination=destination,occupancy=occupancy,
          after=live_json(v,slot),after_occupancy=list(v.read(0x818c80,400)),after_camera=[v.u32(0x4b4254)//48,v.u32(0x4b4258)//48],events=list(events)))
    return dict(executable_sha256=EDITION,cases=cases,boundaries='Unhooked AI decision through 0x43a956 movement, route descent, per-tile walking, coordinates, direction/mirror, occupancy and moved flag. Selection UI 0x441488 is captured as an interface request; its range/highlight/window/timing presentation is NOT established here. Camera focus captured against a fixed initial viewport. Raster boundaries as movement_animation_probe. No attack, XP, action completion or phase transition.')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_ai_movement.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'original AI movement cases')
