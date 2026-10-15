#!/usr/bin/env python3
"""Original player-side maintenance after the complete fresh-profile tutorial.

Runs status/grade recovery, poison and regeneration in the original caller order.
The no-opponent NPC numeric panel, formatter and HP setter execute. Window,
glyph raster, cursor requests, audio/clock and refresh are explicit boundaries.
Host viewport origin (0,56) is supplied as in the isolated panel fixtures; this
headless prefix never creates a Windows client window. World camera is retained.
Unverified recovery effects and party XP panels stop; no player input executes.
"""
import argparse,json
from pathlib import Path
from tutorial_exit_probe import generate as prepare
from damage_panel_probe import configure as panel_configure,HOOKS as PANEL_HOOKS
from battle_events_probe import LIVE,CTRL
from executable_probe import EDITION

RANGES=[(0x43c738,0x43d538),(0x41def0,0x41df0b),(0x41e030,0x41e041),
 (0x4066e0,0x406700),(0x435182,0x4351da),(0x435991,0x4359f5),
 (0x43ff54,0x43ff6e),(0x44df61,0x44e0d9),(0x407a09,0x407b41),
 (0x4082d6,0x4083ed),(0x409650,0x40969f),(0x408950,0x4089cb),
 (0x40709a,0x40721b),(0x406650,0x406670),
 (0x47fad0,0x47fb4c),(0x45b850,0x45b911),(0x418da0,0x418dc5),
 (0x409540,0x409551),(0x40c76e,0x40c7ac),(0x4032b0,0x4032e3),
 (0x403320,0x403337),(0x41c2b0,0x41c2c1),(0x44fa1f,0x44faf1),
 (0x451a93,0x451ab5)]

def generate(source,return_vm=False):
    entry,v,events,capture=prepare(source,True)
    assert entry==json.loads(Path('tests/fixtures/original_tutorial_exit.json').read_text())
    before=capture();v.ranges+=RANGES;panel_configure(v,events,True)
    inherited_origin=[v.u32(0x4b4250+0x3c),v.u32(0x4b4250+0x40)]
    v.put32(0x4b4250+0x3c,0);v.put32(0x4b4250+0x40,56)
    for address in (0x43c7d6,0x43c975,0x44df61,0x43cddd,0x45503f):v.hooks.pop(address,None)
    def stop(vm):raise RuntimeError('unverified maintenance presentation reached')
    v.hooks[0x43e69f]=stop;v.hooks[0x450036]=stop
    def numbers(vm):
        events.append(dict(kind='numbers_begin',args=[vm.arg(n) for n in range(8)],before=capture()))
        assert vm.arg(1)==255
        # Observation only: the original 450984 body executes.
    def refresh(vm):events.append(dict(kind='refresh',unit=vm.arg(0),after=capture()));vm.return_from_hook()
    v.hooks[0x450984]=numbers;v.hooks[0x479a01]=refresh
    allowed=PANEL_HOOKS|{0x450984,0x479a01,0x454b7e,0x45218f,0x476882,0x453b13,0x418f49}
    phases=[]
    def run(name,action):
        events.clear();v.hits.clear();prior=capture();action()
        unexpected=v.hits.intersection(v.hooks)-allowed
        assert not unexpected,[hex(a) for a in sorted(unexpected)]
        phases.append(dict(name=name,before=prior,after=capture(),events=list(events),hooks=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
        print(name,len(events),'events',flush=True)
    def recover():
        for slot in range(35):
            v.call(0x43c7d6,this=LIVE+slot*36);v.call(0x43c975,this=LIVE+slot*36)
    run('recovery',recover)
    run('poison',lambda:v.call(0x44df61,[0],CTRL))
    run('regeneration',lambda:[v.call(0x43cddd,this=LIVE+slot*36) for slot in range(35)])
    r=dict(executable_sha256=EDITION,host_viewport_origin=[0,56],inherited_uninitialized_host_origin=inherited_origin,before=before,phases=phases,after=capture(),boundaries=__doc__)
    return (r,v,events,capture) if return_vm else r

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_phase_maintenance.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print('maintenance complete;',r['after']['rng'])
