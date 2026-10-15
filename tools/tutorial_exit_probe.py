#!/usr/bin/env python3
"""Sequential original tutorial return, guide exit and Cao Cao's facing.

Runs exact opcode49/79 script bytes after the real tutorial continuation.
State, occupancy, RNG, event order and source sprite pixels are captured. Host
raster/audio/wait boundaries match disappearance_probe; no Windows process runs.
"""
import argparse,json
from pathlib import Path
from tutorial_terrain_probe import generate as prepare
from disappearance_probe import configure
from start_banner_probe import configure as start_configure
from turn_banner_probe import HOOKS as BANNER_HOOKS
from deployment_probe import script_call
from executable_probe import EDITION
from eex import parse

def generate(source,return_vm=False):
    entry,v,events,capture=prepare(source,True)
    assert entry==json.loads(Path('tests/fixtures/original_tutorial_terrain.json').read_text())
    original_hooks=dict(v.hooks);configure(v,events)
    # Retain the previously verified camera/pixel setter and its observer.
    if 0x454cef in original_hooks:v.hooks[0x454cef]=original_hooks[0x454cef]
    else:v.hooks.pop(0x454cef,None)
    before=capture();raw=(source/'S_00.eex').read_bytes();commands=[]
    def walk(body):
        for i in body:
            if i['opcode']==1:yield from walk(i['body'])
            else:yield i
    for i in walk(parse(source/'S_00.eex')['sections'][1]['events'][2]['body']):
        if i['offset']<0x2f53 or i['opcode'] in (0,2):continue
        if i['opcode'] not in (49,79,90,11):raise AssertionError(f"unexpected tutorial-exit instruction: {i}")
        events.clear();v.hits.clear();prior=capture()
        if i['opcode']==90:start_configure(v,events)
        script_call(v,{49:0x41311c,79:0x415f65,90:0x416c97,11:0x4142b2}[i['opcode']],raw[i['offset']:i['end']])
        allowed={0x43fcc9,0x47467a,0x403340,0x42c4e0,0x44a88a,0x4192c5,0x418f49,0x479a01,0x47fcfd,0x41fb35,0x41fc89,0x41e5db,0x41e5f6,0x4036f9,0x45137c,0x451913,0x454b7e,0x453b13,0x476882}|BANNER_HOOKS
        assert not(v.hits.intersection(v.hooks)-allowed),[hex(a) for a in sorted(v.hits.intersection(v.hooks)-allowed)]
        commands.append(dict(offset=i['offset'],opcode=i['opcode'],before=prior,after=capture(),events=list(events),hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
    assert [(c['offset'],c['opcode']) for c in commands]==[(0x2f53,49),(0x2f79,79),(0x2f95,90),(0x2f97,11)]
    assert v.u32(0x492f90+0x38+13*4)==1
    r=dict(executable_sha256=EDITION,before=before,commands=commands,after=capture(),event_complete=True,script_variable13=v.u32(0x492f90+0x38+13*4),boundaries=__doc__)
    return (r,v,events,capture) if return_vm else r
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_tutorial_exit.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['commands']),'original exit commands;',r['event_complete'])
