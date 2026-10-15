#!/usr/bin/env python3
"""Sequential original fire event through MCALL05 and the remaining fire tiles.

Exact script bytes drive original handlers, continuing the earlier full turn
reference. The preload worker runs synchronously between handlers. File, heap,
thread and raster boundaries are documented by mcall_probe. Its arithmetic
compositor has a separate original-code fixture. Text rendering is captured at
0x42d5f1; formatting, fallback speaker selection and camera focus execute.
Stop before the first following unit-property change (opcode 77).
"""
import argparse,json
from pathlib import Path
from fire_event_probe import generate as prepare
from mcall_probe import configure,OBJ
from deployment_probe import script_call
from eex import parse
from executable_probe import EDITION

def generate(source,return_vm=False):
    entry,v,events,snapshot=prepare(source,True)
    assert entry==json.loads(Path('tests/fixtures/original_fire_event.json').read_text())
    thread,raw=configure(v,events,buffer=0x1400000,heap_base=0x1500000,host_base=0x83e800)
    v.write(OBJ,b'\0'*0x64)
    v.put32(0x4ae4c8+4,0x50000);v.put32(0x4ae4c8+12,0x1400000)
    v.ranges += [(0x41405b,0x414257),(0x4217ff,0x4218a9),
      (0x455e44,0x455e92),(0x413911,0x413a05),(0x459777,0x45991b)]
    def format_filename(vm):
        assert vm.read(vm.arg(1),32).split(b'\0')[0]==b'MCALL%02u.E5'
        name=('MCALL%02u.E5'%vm.arg(2)).encode();vm.write(vm.arg(0),name+b'\0');vm.return_from_hook(len(name))
    v.put32(0x4862c0,0x83e8c0);v.hooks[0x83e8c0]=format_filename
    def speech(vm):
        events.append(dict(kind='speech',unit=vm.arg(1),text=vm.read(vm.arg(0),1024).split(b'\0')[0].decode('gbk'),before=snapshot()));vm.return_from_hook()
    v.hooks[0x42d5f1]=speech
    def walk(body):
        for i in body:
            if i['opcode']==1:yield from walk(i['body'])
            else:yield i
    script=(source/'S_00.eex').read_bytes();commands=[];before=snapshot()
    # Explicitly allow only the existing raster/host boundaries plus MCALL file,
    # allocation, thread and compositor-capture adapters listed in the fixture.
    allowed={0x403340,0x4036f9,0x418f49,0x418f5c,0x419167,0x419184,0x41921f,0x4192c5,0x41e5db,0x41e5f6,0x41fb35,0x41fc89,0x42c4e0,0x42c557,0x42d5f1,0x43030e,0x430369,0x43fcc9,0x45137c,0x45218f,0x454cef,0x47467a,0x47d1f1,0x47e09b,0x47e251,0x47eac4,0x47eba5,0x47fcfd,0x480b30,0x480fe0,0x480ff0,0x481020,0x83e800,0x83e820,0x83e840,0x83e860,0x83e880,0x83e8a0,0x83e8c0}
    for i in walk(parse(source/'S_00.eex')['sections'][1]['events'][2]['body']):
        if i['offset']<0x22c0 or i['opcode']==0:continue
        if i['opcode']==77:break
        handler={107:0x41405b,79:0x415f65,80:0x416129,20:0x413661,21:0x413911,33:0x413efb}[i['opcode']]
        events.clear();v.hits.clear();prior=snapshot()
        script_call(v,handler,script[i['offset']:i['end']])
        if i['opcode']==107 and i['args'][2]['value']==105:
            assert thread==[0x402574,OBJ] and v.u32(OBJ)==1
            v.call(thread[0],[thread[1]],budget=2000000)
            assert v.u32(OBJ)==2
        assert not(v.hits.intersection(v.hooks)-allowed)
        commands.append(dict(offset=i['offset'],opcode=i['opcode'],args=[a['value'] for a in i['args']],
            before=prior,after=snapshot(),events=list(events),mcall_state=v.u32(OBJ),
            hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
        print(hex(i['offset']),i['opcode'],len(events),'events',flush=True)
    report=dict(executable_sha256=EDITION,before=before,commands=commands,after=snapshot(),
        pending=dict(opcode=i['opcode'],offset=i['offset']),boundaries=__doc__)
    if return_vm:return report,v,events,snapshot
    return report

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_summon_event.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['commands']),'commands',r['pending'])
