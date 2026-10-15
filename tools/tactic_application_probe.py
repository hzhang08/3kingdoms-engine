#!/usr/bin/env python3
"""Original poison target application and caster finalization, at display boundaries.

Starts after the original first cast and MEFF player. HP/MP/status/equipment
getters and setters execute x86. Additional MEFF effects, numeric panels and
window refreshes are recorded presentation requests, not rendered here.
Automatic items and character level animations suspend explicitly. No claim
of complete tactic playback, later AI events or a Windows playthrough.
"""
import argparse,json
from pathlib import Path
from tactic_cast_probe import prepared,CONTROL,LIVE,UNIT
from tactic_effect_probe import configure as effect_configure
from strike_application_probe import configure as application_configure,ItemBoundary
from initialization_probe import state
from movement_probe import live_json
from executable_probe import EDITION

class LevelBoundary(Exception):pass

def configure(v,events,render_effects=False):
    application_configure(v,events)
    v.ranges += [(0x4224a8,0x422dc8),(0x422f0d,0x42313e),
      (0x420e25,0x420eba),(0x425ad0,0x425ae1),(0x41c2d0,0x41c2e1)]
    def effect_load(vm):
        assert vm.read(vm.arg(0),8)==b'MEFF.E5\0'
        events.append(dict(kind='effect_load',resource=vm.arg(1)&255));vm.return_from_hook(1,pop=28)
    def effect_target(vm):events.append(dict(kind='effect_target',slot=vm.arg(0)&255));vm.return_from_hook(1,pop=4)
    def effect_play(vm):
        assert vm.arg(0)==0 and vm.arg(1)==1
        events.append(dict(kind='effect_play'));vm.return_from_hook(pop=8)
    def effect_clear(vm):events.append(dict(kind='effect_clear'));vm.return_from_hook()
    def level(vm):events.append(dict(kind='character_level',unit=(vm.this()-UNIT)//72));raise LevelBoundary()
    if not render_effects:v.hooks.update({0x4014f8:effect_load,0x401e3c:effect_target,0x401e5c:effect_play,0x4016ab:effect_clear})
    v.hooks[0x4073b8]=level

def generate(source):
    _,v,events,snapshot=prepared(source)
    v.cast_elapsed=10;v.call(0x4218d6,this=CONTROL,budget=5000000)
    effect_configure(v,events);v.call(0x4214bf,this=CONTROL,budget=12000000)
    configure(v,events)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000)
    def reset():v.write(0x400000,original);v.write(0x800000,scratch);events.clear();v.hits.clear()
    def record(slot):return dict(battle=live_json(v,slot),unit=state(v,v.u32(LIVE+slot*36)))
    def capture(spec,finalize=False):
        reset()
        # 0x42377d sets success after target application returns, before the
        # outer controller invokes 0x422f0d. It is zero at the prepared boundary.
        if finalize:v.put32(CONTROL+0x4ac,1)
        for name,offset in (('damage',0x84),('armor_xp',0x2d8),('experience',0x2d0),('weapon_xp',0x2d4),('cast',0x4ac)):
            if name in spec:v.put32(CONTROL+offset,spec[name])
        for name,offset in (('effect',0x255),('area',0x2ca)):
            if name in spec:v.write(CONTROL+offset,bytes([spec[name]]))
        for name,offset in (('hp',16),('mp',20)):
            if name in spec:v.put32(LIVE+20*36+offset,spec[name])
        for name,offset in (('status',30),('flags',13),('side',5)):
            if name in spec:v.write(LIVE+20*36+offset,bytes([spec[name]]))
        if 'tick' in spec:v.write(0x4b3d0d,bytes([spec['tick']]))
        if 'beans' in spec:v.write(0x4b09db,bytes([spec['beans']]))
        if 'equipment' in spec:v.write(UNIT+234*72+0x2e,bytes(spec['equipment']))
        if 'actor_equipment' in spec:v.write(UNIT+146*72+0x2e,bytes(spec['actor_equipment']))
        if 'actor_mp' in spec:v.put32(LIVE+35*36+20,spec['actor_mp'])
        if 'camera' in spec:
            v.put32(0x4b4254,spec['camera'][0]);v.put32(0x4b4258,spec['camera'][1])
        before=snapshot();actor=record(35);target=record(20);pending=None
        values=dict(damage=v.u32(CONTROL+0x84),effect=v.read(CONTROL+0x255,1)[0],armor_xp=v.u32(CONTROL+0x2d8),area=v.read(CONTROL+0x2ca,1)[0],beans=v.read(0x4b09db,1)[0],cast=bool(v.u32(CONTROL+0x4ac)),experience=v.u32(CONTROL+0x2d0),weapon_xp=v.u32(CONTROL+0x2d4))
        try:v.call(0x422f0d if finalize else 0x4224a8,this=CONTROL,budget=3000000)
        except ItemBoundary:pending='automatic_item'
        except LevelBoundary:pending='character_level'
        after=snapshot()
        assert before['rng']==after['rng']
        allowed={0x4014f8,0x4016ab,0x401e3c,0x401e5c,0x403340,0x4036f9,
          0x406730,0x4083ef,0x41dc43,0x41e5db,0x41e5f6,0x42c4e0,
          0x43eb1f,0x450984,0x453b13,0x454cef,0x479a01,0x4073b8}
        assert not(v.hits.intersection(v.hooks)-allowed),[hex(a) for a in v.hits.intersection(v.hooks)-allowed]
        other=lambda s:[u for u in s['units'] if u['battle']['slot'] not in (20,35)]
        assert other(before)==other(after),'unrelated unit changed'
        cases.append(dict(spec=spec,finalize=finalize,actor=actor,target=target,values=values,
          before_camera=before['camera'],before_tick=before['tick'],after_actor=record(35),after_target=record(20),
          after_camera=after['camera'],after_tick=after['tick'],rng=after['rng'],events=list(events),pending=pending,
          hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
    cases=[];capture({})
    for hp in (0,1,10,90,500):
     for damage in (0,1,10,499,500,0xffffffff):
      for effect in (0,112):
       for status in (0,8,16,255):capture(dict(hp=hp,damage=damage,effect=effect,status=status))
    for tick in (0,1,2,3,4,5,6,7,254,255):
     for area in (0,1):capture(dict(tick=tick,area=area,hp=90,damage=10,effect=112))
    for item in range(104):
     for side in (0,1,2):capture(dict(hp=90,damage=10,side=side,beans=item%2,armor_xp=1,equipment=[255,255,255,40,1,99,item,9,0]))
    for flags in (0,6,32,255):
     for camera in ([0,0],[144,576]):capture(dict(hp=90,damage=10,flags=flags,camera=camera))
    capture({},True)
    for item in range(104):
     for mp in (0,1,5,10,250):capture(dict(actor_equipment=[255,255,255,255,255,255,item,9,0],actor_mp=mp),True)
    for cast in (0,1):
     for xp in (0,99,100):capture(dict(cast=cast,experience=xp,weapon_xp=xp,actor_equipment=[0,1,99,255,255,255,255,255,255]),True)
    return dict(executable_sha256=EDITION,cases=cases,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_tactic_application.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['cases']),'original poison applications/finalizations')
