#!/usr/bin/env python3
"""Original first enemy's poison plan, stopping before animation/application.

Continues the original-code reinforcement, maintenance, decision and walk VM.
The real selector, controller initialization, target filtering, two independent
hit checks, damage and experience calculation execute. Stops at 0x4214bf;
no casting animation or state application is claimed by this fixture.
"""
import argparse,json
from pathlib import Path
from enemy_phase_probe import generate as prepare
from battle_events_probe import LIVE
from executable_probe import EDITION

CONTROL=0x497af8
class PresentationBoundary(Exception):pass
class NoTargetBoundary(Exception):pass

def configure(v):
    v.ranges += [(0x43dc99,0x43dcdc),(0x4251e1,0x4259c3),(0x423549,0x42378d),
      (0x45533d,0x4553a2),(0x455712,0x45571a),(0x42097a,0x420bbd),
      (0x420efb,0x4214bf),(0x43b4b0,0x43b50d)]
    def stop(vm):raise PresentationBoundary()
    v.hooks[0x4214bf]=stop
    def no_target(vm):raise NoTargetBoundary()
    v.hooks[0x4259ac]=no_target

def plan(v):
    targets=[]
    for slot in v.read(CONTROL+0xd,116):
        if slot==255:break
        targets.append(slot)
    return dict(targets=targets,
      damage=[v.u32(CONTROL+0x84+n*4) for n in range(len(targets))],
      effects=list(v.read(CONTROL+0x255,len(targets))),
      experience=v.u32(CONTROL+0x2d0),weapon_experience=v.u32(CONTROL+0x2d4),
      armor_experience=[v.u32(CONTROL+0x2d8+n*4) for n in range(len(targets))],
      rng=v.u32(0x48ecd4))

def generate(source):
    entry,v,events,snapshot=prepare(source,True);configure(v)
    events.clear();v.hits.clear();before=snapshot()
    v.call(0x441488,[2,20,33],LIVE+35*36)
    selection=dict(before=before,after=snapshot(),events=list(events))
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    # 20 misses both rolls; 23 hits status but misses damage. Include high-bit
    # seeds as well as consecutive small seeds to catch signedness mistakes.
    seeds=(*range(16),20,23,0xffffffff,0x80000000)
    specs=[{}]+[dict(seed=seed,hp=hp,status=status) for seed in seeds for hp in (1,10,90) for status in (0,8,16)]
    for spec in specs:
        v.write(0x400000,original);v.write(0x800000,scratch);events.clear();v.hits.clear()
        if spec:
            v.put32(0x48ecd4,spec['seed']);v.put32(LIVE+20*36+16,spec['hp']);v.write(LIVE+20*36+30,bytes([spec['status']]))
        before=snapshot();stopped=False
        try:v.call(0x43dc99,[33],LIVE+35*36)
        except PresentationBoundary:stopped=True
        except NoTargetBoundary:pass
        assert all(snapshot()[key]==before[key] for key in ('units','camera','tick'))
        assert not(v.hits.intersection(v.hooks)-{0x4214bf,0x4259ac})
        cases.append(dict(spec=spec,before=before,plan=plan(v) if stopped else None,after=snapshot(),stopped=stopped,events=list(events)))
    combinations={(bool(c['plan']['effects'][0]),bool(c['plan']['damage'][0])) for c in cases if c['plan']}
    assert len(combinations)==4,'fixture must cover independent status/damage misses'
    return dict(executable_sha256=EDITION,selection=selection,cases=cases,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_tactic_plan.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['cases']),'original tactic plans')
