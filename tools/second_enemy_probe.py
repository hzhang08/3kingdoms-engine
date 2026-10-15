#!/usr/bin/env python3
"""Original second enemy decision, poison cast and application after archer death.

Continues poison_post_probe's sequential VM through original 0x43ad1c for
slot 36, including selector, planner, both MEFF players and HP/status/MP writes.
Original indexed compositing uses the documented synthetic captured background;
host raster/window/audio/wait boundaries are inherited. No next post-action
event batch, interactive Windows playthrough or player control is claimed.
"""
import argparse,json
from pathlib import Path
from unicorn import UC_HOOK_CODE
from poison_post_probe import generate as prepare
from tactic_plan_probe import plan
from battle_events_probe import LIVE
from executable_probe import EDITION

def generate(source,return_vm=False):
    entry,v,events,snapshot=prepare(source,True);captures=[]
    del v.hooks[0x43b3a2]
    def observe(uc,address,size,user):
        if address in (0x43a956,0x43b367,0x43dc99,0x4214bf,0x4224a8):
            row=dict(routine=hex(address),**snapshot(),event_index=len(events))
            if address in (0x43a956,0x43b367):
                row.update(best_tile=list(v.read(0x4b2c20,2)),best_target=v.read(0x4b2c28,1)[0],best_tactic=v.read(0x4b2c44,1)[0])
            if address in (0x4214bf,0x4224a8):row['plan']=plan(v)
            captures.append(row)
    observer=v.uc.hook_add(UC_HOOK_CODE,observe)
    before=snapshot();events.clear();v.hits.clear()
    v.call(0x43ad1c,this=LIVE+36*36,budget=24000000)
    v.uc.hook_del(observer)
    allowed={0x403340,0x4036f9,0x406730,0x40f548,0x40fa1a,0x418f49,0x4192c5,
      0x41e5db,0x41e5f6,0x41fb35,0x41fc89,0x426066,0x42b81b,0x42c4e0,
      0x42c557,0x42c56b,0x42c600,0x42c646,0x43030e,0x4350f0,0x43eb1f,
      0x43f38b,0x43fcc9,0x4404bd,0x44a88a,0x450984,0x45137c,0x451913,
      0x4519e6,0x4539dc,0x453a40,0x453b13,0x453e32,0x4544d3,0x45461c,
      0x454cef,0x4735bb,0x473623,0x474652,0x47467a,0x479a01,0x47d1f1,
      0x47d239,0x47e09b,0x47e251,0x47eb97,0x4807f0,0x480b30,0x820e00}
    assert not(v.hits.intersection(v.hooks)-allowed)
    assert [c['routine'] for c in captures]==['0x43a956','0x43b367','0x43dc99','0x4214bf','0x4224a8']
    assert captures[-1]['plan']['targets']==[18] and captures[-1]['plan']['damage']==[0] and captures[-1]['plan']['effects']==[112]
    after=snapshot();by_id={u['battle']['id']:u['battle'] for u in after['units']}
    assert by_id[210]['status']==16 and by_id[210]['hp']==110 and by_id[147]['mp']==40 and by_id[147]['flags']&6==6
    assert sum(e['kind']=='effect' for e in events[captures[-1]['event_index']:])==13
    assert before==entry['stages'][-1]['after'] and after['rng']==2402686401
    report=dict(executable_sha256=EDITION,before=before,after=snapshot(),captures=captures,events=list(events),
      hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))],boundaries=__doc__)
    if return_vm:return report,v,events,snapshot
    return report

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_second_enemy.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print([(c['routine'],c.get('best_target'),c.get('plan')) for c in r['captures']])
