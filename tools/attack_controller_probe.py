#!/usr/bin/env python3
"""Original physical attack controller, including presentation RNG and pixels.

Executes target assembly, preparation, critical speech selection, attack sprite
preparation/animation, HP/status/XP application, double gate and counter gate.
File reads, camera scrolling, redraw, text/numeric display, raster writes and
host waits are explicit boundaries. Automatic items and level windows suspend
instead of pretending completion. This is not a full original-game playthrough.
"""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from initialization_probe import UNIT,state
from movement_probe import live_json
from attack_animation_probe import configure as animation_configure
from strike_application_probe import configure as application_configure,ItemBoundary
from strike_probe import CONTROL
from executable_probe import EDITION

def configure(v):
    events=animation_configure(v);application_configure(v,events)
    v.ranges += [(0x405744,0x405889),(0x405c6c,0x405ce6),(0x4061fc,0x406577),
      (0x4065a0,0x4065c6),(0x406670,0x40668c),(0x435d06,0x436510),(0x43652e,0x436586),
      (0x44f960,0x44f976),(0x40c02f,0x40c14f),(0x40bf56,0x40bf82),
      (0x40dfd0,0x40dfe1),(0x45503f,0x455298),(0x47fad0,0x47fae7),
      (0x45965d,0x459777)]
    raw=(v.source/'Imsg.e5').read_bytes()
    def read(vm):
        offset,destination,count=[vm.arg(n) for n in range(3)]
        assert vm.this()==0x4ae4bc and count==200 and offset%200==0 and offset+count<=len(raw)
        events.append(dict(kind='message',index=offset//200));vm.write(destination,raw[offset:offset+count]);vm.return_from_hook(count,pop=12)
    def speech(vm):
        text=vm.read(vm.arg(0),201).split(b'\0')[0]
        assert vm.arg(2)==0
        events.append(dict(kind='speech',unit=vm.arg(1),text=text.decode('gbk')));vm.return_from_hook()
    def scroll(vm):
        x,y=[vm.arg(n)&255 for n in range(2)]
        events.append(dict(kind='camera',tile=[x,y]));vm.put32(0x4b4254,x*48);vm.put32(0x4b4258,y*48);vm.return_from_hook(pop=8)
    def redraw(vm):events.append(dict(kind='unit_redraw',slot=(vm.this()-LIVE)//36));vm.return_from_hook()
    def observe(kind):
        def capture(vm):
            slots=[]
            for s in vm.read(CONTROL+16,116):
                if s==255:break
                slots.append(s)
            events.append(dict(kind=kind,actor=vm.read(CONTROL,1)[0],targets=slots,
              round=vm.read(CONTROL+0x608,1)[0],depth=vm.u32(0x492e04),rng=vm.u32(0x48ecd4),
              hp_damage=[vm.u32(CONTROL+0x84+i*4) for i in range(len(slots))],
              mp_damage=[vm.u32(CONTROL+0x254+i*4) for i in range(len(slots))],
              armor_xp=[vm.u32(CONTROL+0x430+i*4) for i in range(len(slots))],
              xp=vm.u32(CONTROL+0x428),weapon_xp=vm.u32(CONTROL+0x42c),critical=bool(vm.u32(CONTROL+0x604)),
              units=[dict(battle=live_json(vm,s),unit=state(vm,vm.u32(LIVE+s*36))) for s in [vm.read(CONTROL,1)[0]]+slots]))
        return capture
    def unsupported(vm):raise RuntimeError('unverified level-up display')
    v.hooks.update({0x419184:read,0x42d5f1:speech,0x42c557:lambda vm:vm.return_from_hook(),0x454cef:scroll,0x43fcc9:redraw,
      0x406508:observe('assembled'),0x406541:observe('planned'),0x406549:observe('presented'),
      0x406551:observe('applied'),0x406559:observe('double_checked'),0x406569:observe('experience_applied'),
      0x4083ef:unsupported,0x4073b8:unsupported})
    return events

def generate(source):
    _,v,_,_=prepare(source);events=configure(v)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    specs=[dict(actor=20,target=38,position=[9,17],seed=1)]
    specs += [dict(actor=actor,target=target,position=position,seed=seed)
      for actor,target,position in ((20,38,[9,17]),(18,38,[9,16])) for seed in range(32)]
    specs += [dict(actor=18,target=38,position=[9,16],seed=seed,agility=100) for seed in range(8)]
    for spec in specs:
        v.write(0x400000,original);v.write(0x800000,scratch);v.write(0x900000,v.walking_pixels);events.clear()
        actor,target=spec['actor'],spec['target'];v.write(LIVE+actor*36+6,bytes(spec['position']))
        if 'agility' in spec:v.write(UNIT+v.u32(LIVE+actor*36)*72+0x18,bytes([spec['agility']]))
        v.write(0x818c80,b'\xff'*1600)
        for s in range(115):
            b=live_json(v,s)
            if b['lifecycle']==2:v.write(0x818c80+b['y']*20+b['x'],bytes([s]))
        v.put32(0x4b4254,288);v.put32(0x4b4258,576);v.put32(0x4b4264,624);v.put32(0x4b4268,384)
        v.put32(0x48ecd4,spec['seed']);v.put32(0x492e04,0);v.write(0x4b2c28,b'\xff')
        v.call(0x435829,[actor,target]);events.clear();v.hits.clear()
        before=[dict(battle=live_json(v,s),unit=state(v,v.u32(LIVE+s*36))) for s in range(115) if v.read(LIVE+s*36+12,1)[0]==2]
        v.call(0x4064f1,[actor,target],CONTROL,budget=20000000)
        allowed={0x403340,0x4036f9,0x406508,0x406541,0x406549,0x406551,0x406559,0x406569,
          0x418f49,0x419184,0x4192c5,0x41e5db,0x41e5f6,0x41fb35,0x41fc89,0x42c4e0,
          0x4303c0,0x43053e,0x43eb1f,0x43fcc9,0x44faf1,0x450984,0x45137c,0x451913,
          0x4519e6,0x454cef,0x42d5f1,0x42c557,0x47467a,0x47fcfd,0x406730}
        assert not (v.hits.intersection(v.hooks)-allowed),[hex(a) for a in v.hits.intersection(v.hooks)-allowed]
        cases.append(dict(spec=spec,before=before,events=list(events),after=[dict(battle=live_json(v,s),unit=state(v,v.u32(LIVE+s*36))) for s in range(115) if v.read(LIVE+s*36+12,1)[0]==2],rng=v.u32(0x48ecd4),depth=v.u32(0x492e04),tick=v.read(0x4b3d08+5,1)[0],camera=[v.u32(0x4b4254),v.u32(0x4b4258)],hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
    return dict(executable_sha256=EDITION,cases=cases,boundaries=__doc__)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_attack_controller.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['cases']),'complete controller cases')
