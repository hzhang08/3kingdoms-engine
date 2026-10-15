#!/usr/bin/env python3
"""Original tutorial continues after each enabled demonstration-menu response.

Runs the verified third-lesson prefix, original menu initializer, original
callbacks, and its real caller through continuation/decline. DialogBoxParamA is
an explicit host boundary: supplied window dimensions, enabled response and
choice replace the Windows message pump, not game rules. Custom painting,
Windows font metrics, drag interaction and real-time input remain unverified.
"""
import argparse,json,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_ECX
from tutorial_actions_probe import generate as prepare
from tutorial_menu_probe import host,DIALOG
from executable_probe import EDITION
class Modal(Exception):pass
class Boundary(Exception):pass

def generate(source,return_vm=False):
    entry,v,events,capture=prepare(source,True)
    assert entry==json.loads(Path('tests/fixtures/original_tutorial_actions.json').read_text())
    v.hooks.pop(0x45bba0)
    v.ranges += [(0x45e69a,0x45e6b5),(0x46d098,0x46d0c8)]
    windows=host(v,(110,154),(0,0));v.put32(0x4862a0,0x83e100)
    def modal(vm):
        assert vm.arg(1)==0x117 and vm.arg(4)==DIALOG
        raise Modal()
    v.hooks[0x83e100]=modal
    try:v.uc.emu_start(0x45bba0,0x83f000,count=1000000)
    except Modal:pass
    assert v.uc.reg_read(UC_X86_REG_EIP)==0x83e100
    before=capture();flags=[v.u32(DIALOG+0x34+n*4) for n in range(4)]
    v.uc.mem_map(0xf00000,0x10000)
    memory=[(start,bytes(v.uc.mem_read(start,end-start+1))) for start,end,_ in v.uc.mem_regions()]
    context=v.uc.context_save();pending=[None];answer=[0]
    def choice(vm):
        events.append(dict(kind='choice',text=vm.read(vm.arg(0),512).split(b'\0')[0].decode('gbk'),default=vm.arg(1),result=answer[0],before=capture()));vm.return_from_hook(answer[0])
    def stop(vm):pending[0]='0x43419f';raise Boundary()
    v.hooks.update({0x42de12:choice,0x43419f:stop})
    def aux(address,args=()):
        sp=0xf08000;v.write(sp,struct.pack('<'+'I'*(1+len(args)),0x83f000,*args));v.uc.reg_write(UC_X86_REG_ESP,sp);v.uc.reg_write(UC_X86_REG_ECX,DIALOG)
        v.uc.emu_start(address,0x83f000,count=100000)
        assert v.uc.reg_read(UC_X86_REG_EIP)==0x83f000
    branches=[]
    for response in (2,4,255):
      for value in (0,1):
        for start,data in memory:v.uc.mem_write(start,data)
        v.uc.context_restore(context);events.clear();windows.clear();pending[0]=None;answer[0]=value;v.hits.clear()
        aux(0x45bcd0)
        enabled={e['control']:e['enabled'] for e in windows if e['kind']=='enable'}
        assert enabled=={0x507:0,0x508:1,0x509:0,0x50b:1}
        if response==255:aux(0x45bcea)
        else:aux(0x45bc23,(0x111,{2:0x508,4:0x50b}[response],0))
        assert v.read(DIALOG+0x28,1)[0]==response and capture()==before
        host_events=list(windows)
        v.uc.context_restore(context);v.return_from_hook(0,20)
        try:v.uc.emu_start(v.uc.reg_read(UC_X86_REG_EIP),0x83f000,count=1000000)
        except Boundary:pass
        assert pending[0]==('0x43419f' if value==0 else None)
        if value==1:assert v.uc.reg_read(UC_X86_REG_EIP)==0x83f000
        if return_vm and response==255 and value==0:
            terrain_memory=[(start,bytes(v.uc.mem_read(start,end-start+1))) for start,end,_ in v.uc.mem_regions()];terrain_context=v.uc.context_save();terrain_events=list(events)
        branches.append(dict(response=response,choice=value,events=list(events),after=capture(),pending=pending[0],host_events=host_events,hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
    result=dict(executable_sha256=EDITION,before=before,flags_before_init=flags,branches=branches,boundaries=__doc__)
    if return_vm:
        for start,data in terrain_memory:v.uc.mem_write(start,data)
        v.uc.context_restore(terrain_context);events[:]=terrain_events
        return result,v,events,capture
    return result
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_tutorial_menu_event.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['branches']),'original menu responses/continuations')
