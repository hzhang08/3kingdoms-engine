#!/usr/bin/env python3
"""Original first enemy completion into turn two's banner and selected event.

Input is the sequential remaining-enemy reference, now initialized by the real
new-battle controller block and opcode-69 configuration (region 1, selector 1,
limit 20). Executes 0x44eed0 onwards with its original loop stack layout: turn
increment, outcome/limit tests, environment RNG/classification, turn-start
caller, real banner/flag reset and event selector. Records the selected script
entry, then resumes its dispatcher through four speech calls and stops at
opcode 80. Changed-environment presentation also stops.
Host redraw, banner font/raster/wait and resource destructor boundaries are
explicit. Dialogue focus executes; text-window rasterization is captured. The
remaining event body, maintenance, player commands and full battle are not claimed.
"""
import argparse,json
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EBP,UC_X86_REG_ESP
from enemy_continuation_probe import generate as prepare
from battle_events_probe import CTRL,READER
from turn_banner_probe import configure as configure_banner,HOOKS as BANNER_HOOKS
from executable_probe import EDITION
class Boundary(Exception):pass

def generate(source,return_vm=False):
    entry,v,events,snapshot=prepare(source,True)
    # Correct setup leaves all previously captured enemy results unchanged.
    previous=json.loads(Path('tests/fixtures/original_enemy_continuation.json').read_text())
    assert entry==previous
    v.ranges += [(0x449f91,0x44a1f9),(0x44eed0,0x44f051),(0x44ed93,0x44eec5)]
    banner=[];trace=[];pending=None;configure_banner(v,banner)
    def stop_weather(vm):
        nonlocal pending
        pending=dict(kind='environment_presentation',address='0x44a0e5');raise Boundary()
    def stop_body(vm):
        nonlocal pending
        pending=dict(kind='script_body',body_length_offset=vm.u32(READER+12)-vm.u32(READER+0x20));raise Boundary()
    def redraw(vm):events.append(dict(kind='turn_redraw'));vm.return_from_hook()
    v.hooks.update({0x44a0e5:stop_weather,0x41084e:stop_body,0x449eca:redraw})
    def capture(kind):return dict(kind=kind,controller=list(v.read(CTRL,0x12)),**snapshot())
    def observe(uc,address,size,user):
        if address in (0x449f91,0x44efde,0x44edbe,0x4100a1):
            trace.append(capture({0x449f91:'environment_before',0x44efde:'environment_after',0x44edbe:'banner_after',0x4100a1:'event_scan'}[address]))
    observer=v.uc.hook_add(UC_HOOK_CODE,observe)
    frame=0x82f000;v.put32(frame-0xc,CTRL);v.put32(frame+8,0)
    v.uc.reg_write(UC_X86_REG_EBP,frame);v.uc.reg_write(UC_X86_REG_ESP,frame-0x20)
    before=capture('before');events.clear();v.hits.clear()
    try:v.uc.emu_start(0x44eed0,0x83f000,count=5000000)
    except Boundary:pass
    v.uc.hook_del(observer)
    assert pending==dict(kind='script_body',body_length_offset=0x20c4),pending
    allowed=BANNER_HOOKS|{0x44a0e5,0x41084e,0x449eca,0x4036f9,0x418f49,0x4807f0}
    assert not(v.hits.intersection(v.hooks)-allowed),[hex(a) for a in v.hits.intersection(v.hooks)-allowed]
    report=dict(executable_sha256=EDITION,before=before,after=capture('after'),trace=trace,banner=banner,events=list(events),pending=pending,
      classification_table=[list(v.read(0x48e100+r*6,6)) for r in range(5)],
      hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))],boundaries=__doc__)
    # Resume the selected dispatcher through its first actual speech call.
    # Stop at opcode 80, whose animation has a separate exhaustive reference.
    del v.hooks[0x41084e]
    def animation_boundary(vm):raise Boundary()
    v.hooks[0x416129]=animation_boundary
    events.clear();v.hits.clear()
    try:v.call(0x41084e,[1,0],READER,budget=2000000)
    except Boundary:pass
    assert 0x416129 in v.hits
    assert not(v.hits.intersection(v.hooks)-{0x416129,0x42d5f1,0x47eac4,0x4807f0,0x42c557,0x454cef}),[hex(a) for a in sorted(v.hits.intersection(v.hooks))]
    report['dialogue_prefix']=dict(events=list(events),after=snapshot(),hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))],boundary='opcode 80 at 0x21ad; real dispatcher/speech handler executed, text-window rendering captured')
    if return_vm:return report,v,events,snapshot
    return report
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_next_turn.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(r['before']['controller'],r['after']['controller'],r['before']['rng'],r['after']['rng'],r['pending'])
