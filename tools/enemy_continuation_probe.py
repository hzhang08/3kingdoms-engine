#!/usr/bin/env python3
"""Sequential original remaining enemy actions and source-ordered post-action calls.

Starts after the second enemy's full poison action. Uses the original priority
bytes retained by 0x44e851; executes each eligible 0x43ad1c, then the same gated
0x45b5fe/death/redraw/final-event calls as 0x44eb39..0x44ebaf. Original event
bodies outside the already verified archer event suspend. Host raster/window,
audio, numeric display and waits remain explicit boundaries. No phase-end,
player input or interactive Windows playback is claimed.
"""
import argparse,json
from pathlib import Path
from unicorn import UC_HOOK_CODE
from second_enemy_probe import generate as prepare
from battle_events_probe import LIVE,CTRL,READER
from movement_probe import live_json
from executable_probe import EDITION

def generate(source,return_vm=False):
    entry,v,events,snapshot=prepare(source,True);actions=[];post=[];captures=[];scans=0
    # Subsequent physical attacks restore a captured background through this
    # same host raster entry. There is no tactic caption in these actions.
    def restore(vm):vm.return_from_hook()
    def wait(vm):events.append(dict(kind='selection_wait',args=[vm.arg(0)]));vm.return_from_hook()
    v.hooks[0x451913]=restore;v.hooks[0x42c646]=wait
    order=sorted((s for s in range(35,115) if v.read(LIVE+s*36+12,1)[0]==2),key=lambda s:(v.read(LIVE+s*36+11,1)[0],s))
    def observe(uc,address,size,user):
        nonlocal scans
        if address==0x4100a1:scans+=1
        if address in (0x43a956,0x43aafb):
            captures.append(dict(routine=hex(address),**snapshot(),event_index=len(events),best_tile=list(v.read(0x4b2c20,2)),best_target=v.read(0x4b2c28,1)[0],best_tactic=v.read(0x4b2c44,1)[0]))
    observer=v.uc.hook_add(UC_HOOK_CODE,observe)
    allowed={0x403340,0x4036f9,0x406508,0x406541,0x406549,0x406551,0x406559,0x406569,
      0x406730,0x418f49,0x419184,0x4192c5,0x41e5db,0x41e5f6,0x41fb35,0x41fc89,
      0x42b81b,0x42c4e0,0x42c557,0x42c56b,0x42c646,0x42d5f1,0x4303c0,0x43053e,
      0x4350f0,0x43eb1f,0x43f38b,0x43fcc9,0x4404bd,0x441744,0x44a88a,
      0x44faf1,0x450984,0x45137c,0x451913,0x4519e6,0x4539dc,0x453a40,0x453b13,
      0x453e32,0x4544d3,0x45461c,0x454cef,0x47467a,0x479a01,0x47e251,0x47eac4,
      0x47fcfd,0x4807f0,0x820e00}
    def hooks():
        assert not(v.hits.intersection(v.hooks)-allowed),[hex(a) for a in v.hits.intersection(v.hooks)-allowed]
        return [hex(a) for a in sorted(v.hits.intersection(v.hooks))]
    def after_action(slot):
        nonlocal scans
        stages=[];b=live_json(v,slot)
        calls=[]
        if b['flags']&2:calls.append(('action_events',0x45b5fe,[slot],0x4b5df0))
        calls.append(('death_scan',0x44cc03,[],CTRL))
        if b['flags']&2:calls += [('actor_redraw',0x43fcc9,[],LIVE+slot*36),('final_events',0x44c601,[],CTRL)]
        for kind,addr,args,this in calls:
            events.clear();v.hits.clear();scans=0;before=snapshot();v.call(addr,args,this,budget=5000000)
            stages.append(dict(kind=kind,before=before,after=snapshot(),events=list(events),scans=scans,hooked=hooks()))
        return stages
    post=after_action(36)
    for slot in order:
        b=live_json(v,slot)
        if b['lifecycle']!=2 or b['ai']==7 or b['flags']&6:continue
        events.clear();captures.clear();v.hits.clear();before=snapshot()
        v.call(0x43ad1c,this=LIVE+slot*36,budget=22000000)
        row=dict(slot=slot,unit=b['id'],before=before,after=snapshot(),captures=list(captures),events=list(events),hooked=hooks())
        assert len(captures)==2 and captures[0]['best_tactic']==255
        row['post']=after_action(slot);actions.append(row)
    v.uc.hook_del(observer)
    assert v.u32(READER+0x38+55*4)==1 and v.u32(CTRL+0x504)==0
    report=dict(executable_sha256=EDITION,before=entry['after'],order=order,second_post=post,actions=actions,after=snapshot(),boundaries=__doc__)
    if return_vm:return report,v,events,snapshot
    return report

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_enemy_continuation.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print('original remaining enemy units',[x['unit'] for x in r['actions']],r['after']['rng'])
