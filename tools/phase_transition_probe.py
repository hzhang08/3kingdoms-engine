#!/usr/bin/env python3
"""Original phase caller after the first infantry event, up to its first AI call.

Selectors, maintenance, patched player gate, priorities and caller filters execute
unmodified. Presentation/OS boundaries are recorded or omitted explicitly.
"""
import argparse,json
from pathlib import Path
from unicorn import UC_HOOK_CODE
from battle_events_probe import prepare,LIVE,CTRL,READER
from player_phase_probe import configure_patch,PATCH_SHA
from executable_probe import EDITION

class FirstAI(Exception):pass

def generate(source):
    initial,v,_,events=prepare(source);configure_patch(v,source)
    v.put32(READER+0x38+14*4,1);v.put32(READER+0x24,len(events))
    v.write(LIVE+18*36+15,b'\0') # Completed infantry event's facing command.
    v.ranges += [(0x44ed93,0x44eec5),(0x44e851,0x44ebfd),(0x44c601,0x44c658),
      (0x4180c1,0x418174),(0x410990,0x4109a1),(0x44f7b0,0x44f7c1),
      (0x43c738,0x43d108),(0x41def0,0x41df0b),(0x41e030,0x41e041),
      (0x4066e0,0x406700),(0x435182,0x4351da),(0x435991,0x4359f5),
      (0x43ff54,0x4400a2),(0x44df61,0x44e0d9),(0x407a09,0x407b41),
      (0x4082d6,0x4083ed),(0x409650,0x40969f),(0x408950,0x4089cb),
      (0x40709a,0x40721b),(0x406650,0x406670),
      (0x44da7a,0x44df61),(0x44f980,0x44f998),(0x425ab0,0x425ac1),
      (0x43a1f4,0x43a28a),(0x406906,0x406947),(0x425af0,0x425b15)]
    trace=[];cameras=[];actor=[]
    def noop(vm):vm.return_from_hook()
    def banner(vm):trace.append(dict(kind='banner',args=[vm.arg(n) for n in range(3)]));vm.return_from_hook(pop=12)
    def camera(vm):cameras.append([vm.arg(0)&255,vm.arg(1)&255]);vm.return_from_hook(pop=8)
    def first_ai(vm):actor.append(dict(slot=(vm.this()-LIVE)//36,id=vm.u32(vm.this())));raise FirstAI()
    v.hooks.update({0x449eca:noop,0x449de6:noop,0x44ac28:banner,0x47eac4:noop,
      0x45503f:camera,0x4759bc:noop,0x42b81b:noop,0x45461c:noop,0x455719:noop,
      0x43ad1c:first_ai})
    names={0x44c601:'event_batch',0x4100a1:'event_scan',0x43c7d6:'status',0x43c975:'grades',
      0x44df61:'poison',0x43cddd:'regeneration',0x44e0d9:'player_gate',0x44da7a:'priority'}
    def observe(uc,address,size,user):
        if address in names:
            row=dict(kind=names[address],phase=v.read(CTRL+0x10,1)[0])
            if row['kind'] in ('status','grades','regeneration'):row['slot']=(v.this()-LIVE)//36
            trace.append(row)
    v.uc.hook_add(UC_HOOK_CODE,observe)
    before=v.read(LIVE,115*36);persistent=v.read(0x4a1b68,512*72);rng=v.u32(0x48ecd4)
    try:v.call(0x44ed93,[0],CTRL,budget=3000000)
    except FirstAI:pass
    assert len(actor)==1 and persistent==v.read(0x4a1b68,512*72) and rng==v.u32(0x48ecd4)
    after=v.read(LIVE,115*36)
    for n in range(115):assert before[n*36:n*36+11]==after[n*36:n*36+11] and before[n*36+12:(n+1)*36]==after[n*36+12:(n+1)*36]
    return dict(executable_sha256=EDITION,patch_sha256=PATCH_SHA,trace=trace,focus=cameras,
      actor=actor[0],phase=v.read(CTRL+0x10,1)[0],rng=rng,
      input_active=v.u32(CTRL+0x520),persistent_unchanged=True,
      boundaries='Input is original post-introduction roster with completed event 0x1e00 (flag 14 and facing). All phase caller/selector/maintenance/priority rules and disabled-autosave patch execute. Turn banner and focus requests captured; window/redraw/input/cursor/OS message-pump boundaries omitted. Stops on entry to first AI action; no AI action executes.')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_phase_transition.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,indent=2)+'\n');print(r['actor'],r['focus'],'scans',sum(e['kind']=='event_scan' for e in r['trace']))
