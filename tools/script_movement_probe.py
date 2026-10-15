#!/usr/bin/env python3
"""Original opcode-50 handler and movement wrapper after the reinforcement dialogue.

The input VM comes from the original allied caller and reinforcement handlers.
Movement decisions, paths, destination fallback, flags, facing and pixels execute
as original x86. Raster/OS/audio/wait boundaries are inherited explicitly from
allied_phase_probe. No player action or complete enemy action is claimed.
"""
import argparse,json
from pathlib import Path
from unicorn import UC_HOOK_CODE
from allied_phase_probe import generate as prepare
from deployment_probe import script_call
from initialization_probe import UNIT
from movement_probe import live_json
from battle_events_probe import LIVE
from executable_probe import EDITION
from eex import parse

def generate(source,return_vm=False):
    entry,v,events,snapshot=prepare(source,True,True)
    v.ranges += [(0x4132d4,0x4134cb),(0x459487,0x45965d),(0x45b6d0,0x45b6f2),
      (0x43b490,0x43b4b0),(0x44268c,0x442718),(0x425b50,0x425b61),(0x4401a8,0x440252),
      (0x4142b2,0x41431e)]
    # Selector boundary belongs to the suspended caller, not the unit routine.
    del v.hooks[0x41084e]
    def wait(vm):events.append(dict(kind='wait',ticks=vm.arg(0)));vm.return_from_hook(pop=4)
    def minimap(vm):events.append(dict(kind='minimap'));vm.return_from_hook()
    def redraw(vm):events.append(dict(kind='unit_redraw',slot=(vm.this()-LIVE)//36));vm.return_from_hook()
    v.hooks.update({0x403340:wait,0x44a88a:minimap,0x43fcc9:redraw})
    captures=[]
    def observe(uc,address,size,user):
        if address in (0x43ad1c,0x43a956,0x4595da):
            row=dict(routine=hex(address),**snapshot())
            if address==0x43a956:
                row.update(destination=list(v.read(0x4b2c20,2)),distance=list(v.read(0x8192c0,400)),
                  route=list(v.read(0x819900,256)))
            captures.append(row)
    observer=v.uc.hook_add(UC_HOOK_CODE,observe)
    selected=next(e for e in parse(source/'S_00.eex')['sections'][1]['events'] if e['offset']==entry['event_offset'])
    raw=(source/'S_00.eex').read_bytes();cases=[]
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);pixels=v.read(0x900000,115*0x6300)
    def capture(command=None,spec=None):
        before=snapshot();persistent=v.read(UNIT,512*72);events.clear();captures.clear();v.hits.clear()
        if command:
            args=[a['value'] for a in command['args']]
            script_call(v,0x4132d4,raw[command['offset']:command['end']])
        else:
            args=[0,spec['id'],0,*spec['destination'],spec['direction']]
            v.call(0x459487,[spec['id'],*spec['destination'],spec['direction']&0xffff],0x4b5df0,budget=3000000)
        assert v.read(UNIT,512*72)==persistent
        assert snapshot()['rng']==before['rng'] and snapshot()['tick']==before['tick']
        allowed={0x403340,0x4036f9,0x41e5db,0x41e5f6,0x42c4e0,0x43eb1f,
          0x43f38b,0x43fcc9,0x44a88a,0x45137c,0x451913,0x454b7e,0x454cef,0x47e251,0x47fcfd}
        assert not(v.hits.intersection(v.hooks)-allowed),[hex(a) for a in v.hits.intersection(v.hooks)-allowed]
        cases.append(dict(offset=command['offset'] if command else None,args=args,spec=spec,
          before=before,after=snapshot(),captures=list(captures),events=list(events),
          hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
    for command in selected['body']:
        if command['opcode']==50:capture(command)
        elif command['opcode']==11:
            # Complete the source event's trailing flag write before returning
            # a VM for later event scans. Otherwise reinforcements can repeat.
            script_call(v,0x4142b2,raw[command['offset']:command['end']])
    # Continue the source-ordered maintenance calls visible at 0x44e8e7..0x44e9b3.
    # This is a command-level continuation, not execution of the whole caller.
    before_maintenance=snapshot();events.clear();v.hits.clear()
    for slot in range(35,115):
        v.call(0x43c7d6,this=LIVE+slot*36);v.call(0x43c975,this=LIVE+slot*36)
    v.call(0x44df61,[1],0x4b3d08)
    for slot in range(35,115):v.call(0x43cddd,this=LIVE+slot*36)
    v.call(0x44da7a,[0],0x4b3d08)
    maintenance=dict(before=before_maintenance,after=snapshot(),events=list(events),
      priority=[v.read(LIVE+s*36+11,1)[0] for s in range(115)])
    assert not v.hits.intersection(v.hooks),'unexpected maintenance presentation boundary'
    if return_vm:
        v.uc.hook_del(observer)
        return dict(cases=cases,maintenance=maintenance,terrain=list(v.read(0x818640,400))),v,events,snapshot
    specs=[dict(id=32,destination=[11,5],direction=d,flags=f) for f in (0,6,14,128,255) for d in (0,1,2,3,65535)]
    specs += [dict(id=32,destination=tile,direction=2) for tile in ([14,0],[15,0],[9,16],[14,1],[13,1],[13,2],[10,16],[20,0],[0,255])]
    specs += [dict(id=32,destination=[11,5],direction=9,status=8)]
    for spec in specs:
        v.write(0x400000,original);v.write(0x800000,scratch);v.write(0x900000,pixels)
        slot=next(s for s in range(115) if v.u32(LIVE+s*36)==spec['id'] and v.read(LIVE+s*36+12,1)[0]==2)
        if 'flags' in spec:
            # Preserve pixel/flag coherence when changing the synthetic mirror.
            old=v.read(LIVE+slot*36+13,1)[0]
            v.write(LIVE+slot*36+13,bytes([spec['flags']]))
            if (old^spec['flags'])&8:
                for frame in (4,5,8):
                    ptr=0x900000+slot*0x6300+frame*0x900;b=v.read(ptr,0x900)
                    v.write(ptr,b''.join(b[y*48:(y+1)*48][::-1] for y in range(48)))
        if 'status' in spec:v.write(LIVE+slot*36+30,bytes([spec['status']]))
        capture(spec=spec)
    return dict(executable_sha256=EDITION,terrain=list(v.read(0x818640,400)),
      cases=cases,maintenance=maintenance,boundaries=__doc__+' Enemy maintenance and priority are invoked in the source caller order after the three moves. No complete caller/event-interpreter continuation or enemy action is claimed.')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'))
    p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_script_movement.json'))
    a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n')
    print('Captured',len(r['cases']),'original scripted movements')
