#!/usr/bin/env python3
"""Complete original first enemy poison caller, from selection to action flags.

Executes original 0x43dc99, including planning, casting, MEFF pixels, target
application, success flag, MP/XP finalization and action flags. Raster, window,
audio, clock, numeric display and host waits remain explicit boundaries.
Automatic items and level animations suspend rather than being skipped.
"""
import argparse,json
from pathlib import Path
from tactic_cast_probe import prepared,CONTROL,LIVE
from tactic_effect_probe import configure as effect_configure
from tactic_application_probe import configure as apply_configure
from executable_probe import EDITION

def generate(source,return_vm=False):
    _,v,events,snapshot=prepared(source)
    # prepared has run the planning routine only. Rewind its RNG to the
    # observed pre-plan seed, then execute the complete original caller.
    planned=json.loads(Path('tests/fixtures/original_tactic_plan.json').read_text())['cases'][0]
    assert snapshot()['units']==planned['before']['units']
    v.put32(0x48ecd4,planned['before']['rng']);v.cast_elapsed=10
    assert snapshot()==planned['before']
    effect_configure(v,events);del v.hooks[0x4218d6]
    del v.hooks[0x4259ac]
    apply_configure(v,events,True)
    v.ranges += [(0x455d54,0x455d6a)]
    events.clear();v.hits.clear();before=snapshot()
    result=v.call(0x43dc99,[33],LIVE+35*36,budget=16000000)
    allowed={0x403340,0x4036f9,0x40f548,0x40fa1a,0x418f49,0x4192c5,
      0x41e5db,0x41e5f6,0x41fb35,0x41fc89,0x426066,0x42b81b,
      0x42c4e0,0x42c557,0x42c56b,0x42c600,0x42c646,0x43030e,
      0x43fcc9,0x44faf1,0x450984,0x45137c,0x451913,0x4519e6,
      0x453b13,0x45461c,0x454cef,0x4735bb,0x473623,0x474652,
      0x47467a,0x479a01,0x47d1f1,0x47d239,0x47e09b,0x47e251,
      0x47eb97,0x4807f0,0x480b30,0x820e00}
    assert not(v.hits.intersection(v.hooks)-allowed)
    report=dict(executable_sha256=EDITION,before=before,after=snapshot(),events=list(events),
      success=v.u32(CONTROL+0x4ac),return_value=result,
      hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))],boundaries=__doc__)
    if return_vm:return report,v,events,snapshot
    return report

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_poison_controller.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print('original poison caller completed; success',r['success'])
