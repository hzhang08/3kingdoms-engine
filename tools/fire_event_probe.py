#!/usr/bin/env python3
"""Sequential turn-two continuation through the first three scripted fires.

Continues the initialized original enemy/next-turn reference. Executes original
opcode handlers 80, 9, 33, 50, 20 in source order, ending before opcode 107.
Animation frames and host raster/audio/waits remain captured boundaries. Camera
setter is the existing observed setter; terrain writes and RNG are never hooked.
The dispatcher prefix is separately verified; this continuation calls each
handler with its exact original typed bytes and validates argument consumption.
"""
import argparse,json
from pathlib import Path
from next_turn_probe import generate as prepare
from fire_probe import configure,snapshot as terrain_snapshot
from eex import parse
from deployment_probe import script_call
from executable_probe import EDITION

def generate(source,return_vm=False):
    entry,v,events,units=prepare(source,True)
    previous=json.loads(Path('tests/fixtures/original_next_turn.json').read_text())
    for key in ('before','after','trace','banner','dialogue_prefix'):assert entry[key]==previous[key],key
    # Separate lists prevent the earlier report's turn events being overwritten
    # when the reused capture list is cleared for the dialogue prefix.
    Path('tests/fixtures/original_next_turn.json').write_text(json.dumps(entry,ensure_ascii=False,separators=(',',':'))+'\n')
    del v.hooks[0x416129];observer=configure(v,events)
    v.ranges += [(0x416129,0x41626a),(0x45778f,0x45912d),(0x415b0f,0x415b45),
      (0x42c4cb,0x42c4e0),(0x42c522,0x42c540),(0x451a93,0x451ab5),
      (0x41c2b0,0x41c2c1),(0x47fa9e,0x47fab9),(0x41edf7,0x41eec0)]
    def snapshot():return {**units(),**terrain_snapshot(v)}
    def speech(vm):
        events.append(dict(kind='speech',unit=vm.arg(1),text=vm.read(vm.arg(0),1024).split(b'\0')[0].decode('gbk'),before=snapshot()));vm.return_from_hook()
    v.hooks[0x42d5f1]=speech
    def walk(body):
        for i in body:
            if i['opcode']==1:yield from walk(i['body'])
            else:yield i
    raw=(source/'S_00.eex').read_bytes();commands=[];before=snapshot()
    allowed={0x41e5db,0x41e5f6,0x476882,0x45218f,0x47467a,0x403340,0x42c4e0,0x83e000,
      0x4036f9,0x418f49,0x4192c5,0x41fb35,0x41fc89,0x45137c,0x451913,0x4519e6,
      0x47fcfd,0x454cef,0x43fcc9,0x42d5f1,0x42c557,0x47eac4,0x4807f0,
      0x43eb1f,0x43f38b,0x454b7e,0x47e251,0x479a01,0x453b13,0x45461c,0x406730,0x44a88a}
    for i in walk(parse(source/'S_00.eex')['sections'][1]['events'][2]['body']):
        if i['offset']<0x21ad or i['opcode']==0:continue
        if i['opcode']==107:break
        opcode=i['opcode'];handler={80:0x416129,9:0x415b0f,33:0x413efb,50:0x4132d4,20:0x413661}[opcode]
        events.clear();v.hits.clear();prior=snapshot()
        script_call(v,handler,raw[i['offset']:i['end']])
        assert not(v.hits.intersection(v.hooks)-allowed),[hex(a) for a in v.hits.intersection(v.hooks)-allowed]
        commands.append(dict(offset=i['offset'],opcode=opcode,args=[a['value'] for a in i['args']],before=prior,after=snapshot(),events=list(events),hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
    v.uc.hook_del(observer)
    report=dict(executable_sha256=EDITION,before=before,commands=commands,after=snapshot(),pending=dict(opcode=107,offset=i['offset']),boundaries=__doc__)
    if return_vm:return report,v,events,snapshot
    return report
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_fire_event.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print('Commands',[(hex(c['offset']),c['opcode']) for c in r['commands']],r['pending'])
