#!/usr/bin/env python3
"""First original enemy decision/walk after source-ordered reinforcement/maintenance.

Uses script_movement_probe's sequential original-code VM. Decision, movement,
and RNG execute in the supplied executable; rendering,
window, wait and sound operations retain explicit presentation boundaries.
Stops at 0x43b3a2 before tactic selection/presentation/application. This is not
a completed enemy action, interactive Windows playthrough or full battle.
"""
import argparse,json
from pathlib import Path
from unicorn import UC_HOOK_CODE
from script_movement_probe import generate as prepare
from tactic_scoring_probe import configure
from battle_events_probe import LIVE
from executable_probe import EDITION
class TacticBoundary(Exception):pass

def generate(source,return_vm=False):
    entry,v,events,snapshot=prepare(source,True);configure(v)
    captures=[];actions=[]
    def observe(uc,address,size,user):
        if address in (0x43a956,0x43b367):
            captures.append(dict(routine=hex(address),**snapshot(),
              best_tile=list(v.read(0x4b2c20,2)),best_target=v.read(0x4b2c28,1)[0],
              best_tactic=v.read(0x4b2c44,1)[0],best_score=v.read(0x4b2c4c,1)[0]))
    observer=v.uc.hook_add(UC_HOOK_CODE,observe)
    def stop(vm):raise TacticBoundary()
    v.hooks[0x43b3a2]=stop
    for slot in (35,):
        before=snapshot();events.clear();captures.clear();v.hits.clear()
        try:v.call(0x43ad1c,this=LIVE+slot*36,budget=12000000)
        except TacticBoundary:pass
        else:raise AssertionError('expected tactic resolution boundary')
        allowed={0x43b3a2,0x403340,0x4036f9,0x41e5db,0x41e5f6,0x42c4e0,0x43eb1f,
          0x43f38b,0x43fcc9,0x44a88a,0x45137c,0x451913,0x454b7e,0x454cef,0x47e251,0x47fcfd,
          0x42c646,0x42c56b,0x406730,0x4350f0,0x453e32,0x453b13,0x4544d3,0x4539dc,0x45461c,0x4404bd}
        assert not(v.hits.intersection(v.hooks)-allowed),[hex(a) for a in v.hits.intersection(v.hooks)-allowed]
        actions.append(dict(slot=slot,before=before,after=snapshot(),captures=list(captures),events=list(events),
          hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
    v.uc.hook_del(observer)
    result=dict(executable_sha256=EDITION,initial=entry['maintenance']['after'],terrain=entry['terrain'],actions=actions,boundaries=__doc__)
    if return_vm:return result,v,events,snapshot
    return result

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'))
    p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_enemy_phase.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n')
    print([(c['slot'],[(m['routine'],m['best_tile'],m['best_target'],m['best_tactic']) for m in c['captures']],c['after']['rng']) for c in r['actions']])
