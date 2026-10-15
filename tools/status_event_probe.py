#!/usr/bin/env python3
"""Continue the actual original fire event through its area confusion command.

Calls original opcode77 with original typed arguments. Occupancy scan, side
filter, MEFF48 loader/player/compositor, status mutation, camera and RNG execute
in the original code. Host raster captures use a deterministic indexed pattern;
Win32 windows/audio/waits are captured boundaries, not a Windows playthrough.
"""
import argparse,json,hashlib
from pathlib import Path
from summon_event_probe import generate as prepare
from tactic_effect_probe import configure as effects
from deployment_probe import script_call
from executable_probe import EDITION
from eex import parse

def configure(v,events):
    effects(v,events)
    v.ranges += [(0x45ade5,0x45afae),(0x43e69f,0x43eb03),(0x4066c0,0x4066e0)]
    v.hooks.pop(0x43e69f,None)

def generate(source,return_vm=False):
    entry,v,events,snapshot=prepare(source,True)
    assert entry==json.loads(Path('tests/fixtures/original_summon_event.json').read_text())
    configure(v,events)
    def walk(body):
        for i in body:
            if i['opcode']==1:yield from walk(i['body'])
            else:yield i
    raw=(source/'S_00.eex').read_bytes();commands=[];before=snapshot()
    for i in walk(parse(source/'S_00.eex')['sections'][1]['events'][2]['body']):
        if i['offset']<0x282c or i['opcode']==0:continue
        if i['opcode']!=77:break
        events.clear();v.hits.clear();prior=snapshot()
        script_call(v,0x4150e7,raw[i['offset']:i['end']],budget=20000000)
        allowed={0x403340,0x418f49,0x4192c5,0x41e5db,0x41e5f6,0x41fb35,0x41fc89,0x42c4e0,0x42c56b,0x43030e,0x43fcc9,0x454cef,0x474652,0x47467a,0x479a01,0x47d1f1,0x47e09b,0x47e251,0x4807f0,0x480b30,0x820e00}
        assert not(v.hits.intersection(v.hooks)-allowed)
        commands.append(dict(offset=i['offset'],opcode=i['opcode'],args=[a['value'] for a in i['args']],
            before=prior,after=snapshot(),events=list(events),hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
        print('status',hex(i['offset']),len(events),'events',flush=True)
    report=dict(executable_sha256=EDITION,before=before,commands=commands,after=snapshot(),
        pending=dict(opcode=i['opcode'],offset=i['offset']),boundaries=__doc__)
    if return_vm:return report,v,events,snapshot
    return report

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_status_event.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['commands']),'commands',r['pending'])
