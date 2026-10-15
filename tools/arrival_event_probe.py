#!/usr/bin/env python3
"""Sequential original turn-two AI changes and dialogue before Cao Cao arrives.

Continues the full original status-command VM. Exact typed bytes drive original
handlers. Host raster/window/audio/wait boundaries are retained from the earlier
probes. Stops at the first unverified handler; no player interaction is claimed.
"""
import argparse,json
from pathlib import Path
from status_event_probe import generate as prepare
from script_ai_probe import configure
from deployment_probe import script_call
from executable_probe import EDITION
from eex import parse

def generate(source,return_vm=False):
    entry,v,events,snapshot=prepare(source,True)
    assert entry==json.loads(Path('tests/fixtures/original_status_event.json').read_text())
    configure(v)
    v.ranges += [(0x41581c,0x415890)]
    def music(vm):events.append(dict(kind='music',mci_track=vm.arg(0)));vm.return_from_hook(pop=4)
    v.hooks[0x4745fe]=music
    def walk(body):
        for i in body:
            if i['opcode']==1:yield from walk(i['body'])
            else:yield i
    raw=(source/'S_00.eex').read_bytes();commands=[];before=snapshot()
    allowed={0x403340,0x4036f9,0x418f49,0x4192c5,0x41e5db,0x41e5f6,0x41fb35,0x41fc89,0x42c4e0,0x42c557,0x42d5f1,0x43fcc9,0x454cef,0x47467a,0x479a01,0x47fcfd,0x45137c,0x4745fe,0x43eb1f,0x43f38b,0x44a88a,0x451913,0x454b7e,0x47e251,0x47eac4,0x4807f0}
    # Existing original fire-event animation raster/text/window boundaries.
    allowed|={0x4519e6,0x45218f,0x476882,0x83e000,0x453b13,0x45461c,0x406730}
    for i in walk(parse(source/'S_00.eex')['sections'][1]['events'][2]['body']):
        if i['offset']<0x2870 or i['opcode']==0:continue
        handlers={78:0x4146d4,79:0x415f65,80:0x416129,20:0x413661,11:0x4142b2,36:0x41581c,76:0x414f42,50:0x4132d4}
        if i['opcode'] not in handlers:break
        events.clear();v.hits.clear();prior=snapshot()
        script_call(v,handlers[i['opcode']],raw[i['offset']:i['end']],budget=2000000)
        assert not(v.hits.intersection(v.hooks)-allowed),[hex(a) for a in v.hits.intersection(v.hooks)-allowed]
        commands.append(dict(offset=i['offset'],opcode=i['opcode'],args=[a['value'] for a in i['args']],before=prior,after=snapshot(),events=list(events),hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
        print('arrival',hex(i['offset']),i['opcode'],len(events),'events',flush=True)
    result=dict(executable_sha256=EDITION,before=before,commands=commands,after=snapshot(),pending=dict(opcode=i['opcode'],offset=i['offset']),boundaries=__doc__)
    return (result,v,events,snapshot) if return_vm else result

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_arrival_event.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['commands']),'commands',r['pending'])
