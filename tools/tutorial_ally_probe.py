#!/usr/bin/env python3
"""Original second tutorial lesson from the verified first-lesson boundary.

Movement search, guide walking, facing, poses, cursor-plane mutations and UI
range-mask writes execute original machine code. The previously hooked UI mask
buffer starts cleared, matching the completed first lesson. Popup calls and
host raster/window/audio/waits are captured; no player input action executes.
This is evidence for a forthcoming port, not implemented native gameplay.
"""
import argparse,json
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EIP
from tutorial_basic_probe import generate as prepare
from executable_probe import EDITION

class Boundary(Exception):pass

def generate(source,return_vm=False):
    entry,v,events,snapshot=prepare(source,True)
    assert entry==json.loads(Path('tests/fixtures/original_tutorial_basic.json').read_text())
    v.ranges += [(0x4337f4,0x433bc8),(0x4350f0,0x435104),(0x452898,0x453b11)]
    # Execute these UI setters instead of the earlier raster-request hooks.
    for address in (0x4337f4,0x4350f0,0x4539dc):v.hooks.pop(address,None)
    v.put32(0x4af4a8+4,1600);v.put32(0x4af4a8+12,0x82a000);v.write(0x4af4a8+16,b'\x01')
    v.write(0x82a000,b'\0'*1600);v.put32(0x4b4250+0x68,0)
    panel=[None];range_ready=[False]
    def capture():
        return {**snapshot(),'distance':list(v.read(0x8192c0,400)),
          'range':list(v.read(0x82a000,400)),'movement':v.u32(0x4b4250+0x68),'panel_slot':panel[0],'range_computed':range_ready[0]}
    def speech(vm):
        events.append(dict(kind='speech',unit=vm.arg(1),text=vm.read(vm.arg(0),2048).split(b'\0')[0].decode('gbk'),before=capture()));vm.return_from_hook()
    def show(vm):
        panel[0]=(vm.this()-0x4b2c50)//36
        events.append(dict(kind='panel',slot=panel[0],args=[vm.arg(n)&255 for n in range(3)],before=capture()));vm.return_from_hook(pop=12)
    def clear(vm):
        panel[0]=None;events.append(dict(kind='clear_panel',before=capture()));vm.return_from_hook()
    v.hooks.update({0x42d5f1:speech,0x4404bd:show,0x453e32:clear})
    searches=[]
    def observer(uc,address,size,data):
        if address==0x438308:
            if (v.this()-0x4b2c50)//36==15 and (v.arg(2)&255)==0:range_ready[0]=True
            searches.append(dict(slot=(v.this()-0x4b2c50)//36,origin=list(v.read(v.arg(0),2)),budget=v.arg(1)&255,mode=v.arg(2)&255,target=[v.arg(3)&255,v.arg(4)&255],respect_zoc=bool(v.arg(5))))
    v.uc.hook_add(UC_HOOK_CODE,observer)
    memory=[(start,bytes(v.uc.mem_read(start,end-start+1))) for start,end,_ in v.uc.mem_regions()]
    context=v.uc.context_save()
    events.clear();v.hits.clear();before=capture()
    v.call(0x4337f4,budget=5000000)
    allowed={int(a,16) for a in entry['hooked']}|{0x4544d3,0x42c56b,0x42c646,0x47fd13,
      0x4519e6,0x44a88a,0x43f38b,0x47e251,0x451913,0x453b13}
    unexpected=v.hits.intersection(v.hooks)-allowed
    assert not unexpected,[hex(a) for a in unexpected]
    after=capture()
    assert before['rng']==after['rng']
    assert after['movement']==0 and not any(after['range']) and after['panel_slot'] is None
    result=dict(executable_sha256=EDITION,before=before,after=after,events=list(events),searches=list(searches),hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))],boundaries=__doc__)
    answer=[0];pending=[None]
    def choice(vm):
        events.append(dict(kind='choice',text=vm.read(vm.arg(0),512).split(b'\0')[0].decode('gbk'),default=vm.arg(1),result=answer[0],before=capture()));vm.return_from_hook(answer[0])
    def stop(vm):pending[0]='0x433bc8';raise Boundary()
    v.hooks.update({0x42de12:choice,0x433bc8:stop})
    def resume(value):
        for start,data in memory:v.uc.mem_write(start,data)
        v.uc.context_restore(context);events.clear();searches.clear();v.hits.clear();panel[0]=None;range_ready[0]=False;answer[0]=value;pending[0]=None
        try:v.uc.emu_start(0x4337f4,0x83f000,count=5000000)
        except Boundary:pass
        assert pending[0]==('0x433bc8' if value==0 else None)
        if value==1:assert v.uc.reg_read(UC_X86_REG_EIP)==0x83f000
        unexpected=v.hits.intersection(v.hooks)-allowed-{0x42de12,0x433bc8}
        assert not unexpected,[hex(a) for a in unexpected]
        return dict(choice=value,after=capture(),events=list(events),pending=pending[0])
    result['continuations']=[resume(0),resume(1)]
    if return_vm:
        continued=resume(0);assert continued==result['continuations'][0]
        return result,v,events,capture
    return result

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_tutorial_ally.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['events']),'events;',len(r['searches']),'searches')
