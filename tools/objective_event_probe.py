#!/usr/bin/env python3
"""Sequential original objective, two target highlights and guide appearance.

Continues the verified arrival VM using exact script bytes and original handlers.
Font/raster/window/input/wait and popup requests remain explicit boundaries.
Camera setters and unit changes are captured by the existing arrival observer.
Stops before the tutorial; no player control or tutorial behavior is claimed.
"""
import argparse,json
from pathlib import Path
from arrival_event_probe import generate as prepare
from objective_banner_probe import configure as objective_configure
from highlight_probe import configure as highlight_configure,HOOKS as HIGHLIGHT_HOOKS
from turn_banner_probe import HOOKS as BANNER_HOOKS
from deployment_probe import script_call
from executable_probe import EDITION
from eex import parse

def generate(source,return_vm=False):
    entry,v,events,snapshot=prepare(source,True)
    assert entry==json.loads(Path('tests/fixtures/original_arrival_event.json').read_text())
    old_hooks=v.hooks.copy();v.ranges += [(0x413c0e,0x413c56)]
    raw=(source/'S_00.eex').read_bytes();commands=[];before=snapshot()
    allowed={0x403340,0x4036f9,0x418f49,0x4192c5,0x41e5db,0x41e5f6,0x41fb35,0x41fc89,0x42c4e0,0x42c557,0x42d5f1,0x43fcc9,0x454cef,0x47467a,0x479a01,0x47fcfd,0x45137c,0x4745fe,0x43eb1f,0x43f38b,0x44a88a,0x451913,0x454b7e,0x47e251,0x47eac4,0x4807f0,0x4519e6,0x45218f,0x476882,0x83e000,0x453b13,0x45461c,0x406730}
    allowed |= HIGHLIGHT_HOOKS|BANNER_HOOKS|{0x42c557,0x42c89c,0x42c4cb,0x42c56b,0x425c26,0x425c37}
    def walk(body):
        for i in body:
            if i['opcode']==1:yield from walk(i['body'])
            else:yield i
    for i in walk(parse(source/'S_00.eex')['sections'][1]['events'][2]['body']):
        if i['offset']<0x2f13 or i['opcode'] in (0,2):continue
        if i['opcode'] not in (26,92,76):break
        events.clear();v.hits.clear();prior=snapshot();v.hooks=old_hooks.copy()
        if i['opcode']==26:objective_configure(v,events)
        if i['opcode']==92:
            highlight_configure(v,events)
            # Keep the verified original camera path and observed pixel setter.
            if 0x45503f in old_hooks:v.hooks[0x45503f]=old_hooks[0x45503f]
            else:v.hooks.pop(0x45503f,None)
        script_call(v,{26:0x413c0e,92:0x416e17,76:0x414f42}[i['opcode']],raw[i['offset']:i['end']],budget=2000000)
        assert not(v.hits.intersection(v.hooks)-allowed),[hex(a) for a in v.hits.intersection(v.hooks)-allowed]
        commands.append(dict(offset=i['offset'],opcode=i['opcode'],args=[a['value'] for a in i['args']],before=prior,after=snapshot(),events=list(events),hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
        print('objective event',hex(i['offset']),len(events),'events',flush=True)
    v.hooks=old_hooks
    result=dict(executable_sha256=EDITION,before=before,commands=commands,after=snapshot(),pending=dict(opcode=i['opcode'],offset=i['offset']),boundaries=__doc__)
    return (result,v,events,snapshot) if return_vm else result

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_objective_event.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['commands']),'commands',r['pending'])
