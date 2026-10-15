#!/usr/bin/env python3
"""Investigate the original post-poison event/death caller sequence.

Continues the complete first original poison caller through the source-ordered
0x45b5fe, 0x44cc03, actor redraw and 0x44c601 calls at 0x44eb39..0x44ebaf.
Runs the original event dispatcher and flag write. Dialogue is captured at the
text-window boundary; raster, sound and waits retain the documented reference
boundaries. Unexpected event bodies stop before executing.
"""
import argparse,json
from pathlib import Path
from unicorn import UC_HOOK_CODE
from poison_controller_probe import generate as prepare
from battle_events_probe import LIVE,CTRL,READER
from executable_probe import EDITION
from movement_probe import live_json

class ScriptBodyBoundary(Exception):pass

def generate(source,return_vm=False):
    entry,v,events,snapshot=prepare(source,True);trace=[];stages=[];pending=None
    v.ranges += [(0x4105a8,0x41090b),(0x40fd82,0x40fdda),(0x418c00,0x418c2d),
      (0x413661,0x413911),(0x4142b2,0x41431e),(0x41eec0,0x41ef31),
      (0x4179a5,0x417a54),(0x47fcc3,0x47fcfd)]
    def script_body(vm):
        nonlocal pending
        offset=vm.u32(vm.this()+0xc)-vm.u32(vm.this()+0x20)
        if offset!=0x3b22:
            pending=dict(kind='script_body',body_length_offset=offset);raise ScriptBodyBoundary()
        trace.append(dict(kind='script_body',body_length_offset=offset))
        # Observe only: the real dispatcher executes.
    def speech(vm):
        text=vm.read(vm.arg(0),1024).split(b'\0')[0].decode('gbk')
        events.append(dict(kind='speech',unit=vm.arg(1),text=text,flags=vm.arg(2),before=snapshot(),flag55=vm.u32(READER+0x38+55*4)))
        vm.return_from_hook()
    v.hooks[0x41084e]=script_body
    v.hooks[0x42d5f1]=speech
    def redraw(vm):
        b=live_json(vm,(vm.this()-LIVE)//36)
        events.append(dict(kind='tile',battle=b,visible=vm.read(0x818c80+b['y']*20+b['x'],1)[0]!=255));vm.return_from_hook()
    v.hooks[0x43fcc9]=redraw
    assert v.u32(READER+0x38+11*4)==1 and v.u32(READER+0x38+55*4)==0
    def observe(uc,address,size,user):
        if address==0x4100a1:trace.append(dict(kind='event_scan'))
        if address==0x441744:trace.append(dict(kind='death',slot=(v.this()-LIVE)//36))
    observer=v.uc.hook_add(UC_HOOK_CODE,observe)
    for kind,address,args,this in [('action_events',0x45b5fe,[35],0x4b5df0),('death_scan',0x44cc03,[],CTRL),('actor_redraw',0x43fcc9,[],LIVE+35*36),('final_events',0x44c601,[],CTRL)]:
        events.clear();trace.clear();v.hits.clear();before=snapshot()
        try:v.call(address,args,this,budget=5000000)
        except ScriptBodyBoundary:pass
        stages.append(dict(kind=kind,before=before,after=snapshot(),events=list(events),trace=list(trace),pending=pending,flag55=v.u32(READER+0x38+55*4),
          hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
        if pending:break
        allowed={0x41084e,0x42c557,0x42d5f1,0x454cef,0x47eac4,0x4807f0,
          0x403340,0x418f49,0x42c4e0,0x43fcc9,0x441744,0x44a88a,0x47467a,0x479a01}
        assert not(v.hits.intersection(v.hooks)-allowed)
    v.uc.hook_del(observer)
    assert pending is None and len(stages)==4
    speech_events=[e for e in stages[0]['events'] if e['kind']=='speech']
    assert len(speech_events)==1 and speech_events[0]['unit']==234 and speech_events[0]['flag55']==0
    assert stages[0]['flag55']==1 and stages[0]['before']==stages[0]['after']
    assert sum(t['kind']=='event_scan' for t in stages[0]['trace'])==18
    assert sum(t['kind']=='event_scan' for t in stages[3]['trace'])==18
    assert stages[1]['trace']==[dict(kind='death',slot=20)]
    assert next(u['battle']['lifecycle'] for u in stages[1]['after']['units'] if u['battle']['id']==234)==3
    report=dict(executable_sha256=EDITION,stages=stages,boundaries=__doc__)
    if return_vm:return report,v,events,snapshot
    return report

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('.cache/original-poison-post.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print([(s['kind'],s['pending'],s['trace']) for s in r['stages']])
