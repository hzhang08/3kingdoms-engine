#!/usr/bin/env python3
"""Sequential original tutorial entry and first lesson, before the next lesson.

Actual objective-event VM continues through original dialogue focus, facing,
sprite poses, cursor plane writes, waits and continuation choice. Popup layout,
text/raster/audio/window output are captured at documented host boundaries.
Collection bytes are explicit fresh-profile inputs; legacy saves are untouched.
"""
import argparse,json
from pathlib import Path
from objective_event_probe import generate as prepare
from movement_probe import live_json
from executable_probe import EDITION

class Boundary(Exception):pass

def generate(source,return_vm=False):
    entry,v,events,snapshot=prepare(source,True)
    assert entry==json.loads(Path('tests/fixtures/original_objective_event.json').read_text())
    v.ranges += [(0x433359,0x4337f4),(0x434f84,0x4350ba),(0x44049e,0x4404bd)]
    choices=[];pending=[None];answers=[]
    def capture():return {**snapshot(),'tutorial_cursor':list(v.read(0x4b2c18,2))}
    def collection(vm):vm.return_from_hook(int(vm.arg(0)==26)) # original R_00 equips collectible item23
    def speech(vm):events.append(dict(kind='speech',unit=vm.arg(1),text=vm.read(vm.arg(0),2048).split(b'\0')[0].decode('gbk'),before=capture()));vm.return_from_hook()
    def choice(vm):
        if not answers:raise AssertionError('unexpected original tutorial choice')
        result=answers.pop(0)
        row=dict(kind='choice',text=vm.read(vm.arg(0),512).split(b'\0')[0].decode('gbk'),default=vm.arg(1),result=result,before=capture())
        choices.append(row);events.append(row);vm.return_from_hook(result)
    def panel(vm):events.append(dict(kind='panel',slot=(vm.this()-0x4b2c50)//36,args=[vm.arg(n)&255 for n in range(3)],before=capture()));vm.return_from_hook(pop=12)
    def clear(vm):events.append(dict(kind='clear_panel',before=capture()));vm.return_from_hook()
    def stop(vm):pending[0]='0x4337f4';raise Boundary()
    v.hooks.update({0x41a284:collection,0x42d5f1:speech,0x42de12:choice,0x4404bd:panel,0x453e32:clear,0x4337f4:stop})
    memory=[(start,bytes(v.uc.mem_read(start,end-start+1))) for start,end,_ in v.uc.mem_regions()]
    context=v.uc.context_save()
    allowed={0x41a284,0x42de12,0x4404bd,0x453e32,0x4337f4,0x42d5f1,0x403340,0x4036f9,0x418f49,0x4192c5,0x41e5db,0x41e5f6,0x41fb35,0x41fc89,0x42c4e0,0x42c557,0x43fcc9,0x454cef,0x47467a,0x479a01,0x47fcfd,0x45137c,0x43eb1f,0x43f38b,0x44a88a,0x451913,0x454b7e,0x47e251,0x47eac4,0x4807f0,0x4519e6,0x45218f,0x476882,0x83e000,0x453b13,0x45461c,0x406730}
    def run(inputs):
        for start,data in memory:v.uc.mem_write(start,data)
        v.uc.context_restore(context)
        events.clear();v.hits.clear();choices.clear();answers[:]=inputs;pending[0]=None;before=capture()
        try:v.call(0x433359,budget=5000000)
        except Boundary:pass
        assert pending[0]==('0x4337f4' if inputs==[0,0] else None),pending
        assert not answers,answers
        assert not(v.hits.intersection(v.hooks)-allowed),[hex(a) for a in v.hits.intersection(v.hooks)-allowed]
        return dict(choices=inputs,before=before,after=capture(),events=list(events),pending=pending[0],hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))])
    result=run([0,0]);result['alternatives']=[run([0,1]),run([1])]
    result=dict(executable_sha256=EDITION,**result,boundaries=__doc__)
    if return_vm:
        # Restore the real accepted-lesson boundary after capturing alternatives.
        continued=run([0,0]);assert continued['after']==result['after']
        return result,v,events,capture
    return result

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_tutorial_basic.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['events']),'events',r['pending'])
