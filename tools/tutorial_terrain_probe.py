#!/usr/bin/env python3
"""Sequential original fourth lesson through tutorial summary and farewell.

Continues the real caller after the menu demonstration. Terrain-panel body,
selection cursor setters, grayscale icons/predicates, guide walk, unit panel
requests and all dialogue/poses run in order. Raster/text/clock/audio host
boundaries are inherited; terrain preview capture uses the actual original
empty, effect-free map tile selected by this lesson. Unit panel layout remains
covered by its separate original probe. Fresh collection profile has no rewards.
"""
import argparse,json
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EIP
from tutorial_menu_event_probe import generate as prepare
from terrain_panel_probe import configure as panel_configure,RANGES
from executable_probe import EDITION

def generate(source,return_vm=False):
    entry,v,events,snapshot=prepare(source,True)
    assert entry==json.loads(Path('tests/fixtures/original_tutorial_menu_event.json').read_text())
    v.hooks.pop(0x43419f);v.ranges += [(0x43419f,0x4347ff)]+RANGES
    kind=[None];panel_events=[];old_hooks={};old_descriptors={};camera=0x4b4250
    def capture():
        state=snapshot();cursor=list(v.read(camera+0x54,2))
        if cursor!=[255,255]:cursor=[cursor[0]+v.u32(camera+4)//48,cursor[1]+v.u32(camera+8)//48]
        return {**state,'selection_cursor':cursor,'panel_kind':kind[0]}
    def speech(vm):events.append(dict(kind='speech',unit=vm.arg(1),text=vm.read(vm.arg(0),2048).split(b'\0')[0].decode('gbk'),before=capture()));vm.return_from_hook()
    old_show=v.hooks[0x4404bd];old_clear=v.hooks[0x453e32]
    def show(vm):kind[0]='unit';old_show(vm)
    def clear(vm):kind[0]=None;old_clear(vm)
    v.hooks.update({0x42d5f1:speech,0x4404bd:show,0x453e32:clear})
    def enter(vm):
        nonlocal old_hooks,old_descriptors,panel_events
        assert vm.arg(1)==0
        point=list(vm.read(vm.arg(0),2));index=point[1]*20+point[0]
        assert vm.read(0x818c80+index,1)==b'\xff' and vm.read(0x819f40+index,1)==b'\xff'
        events.append(dict(kind='terrain_panel',tile=point,before=capture()))
        old_hooks=dict(v.hooks);old_descriptors={a:v.read(a,20) for a in (0x4ae4c8,0x4aeb50)}
        panel_events=panel_configure(v,True);kind[0]='terrain'
    def leave(vm):
        events.append(dict(kind='terrain_panel_raster',events=list(panel_events),after=capture()))
        v.hooks=old_hooks
        for a,data in old_descriptors.items():v.write(a,data)
    v.hooks.update({0x45576d:enter,0x4343c5:leave})
    # Execute both scrolling and its unconditional cursor-clear call. The
    # inherited scroll mock changed camera pixels but omitted cursor lifetime.
    # Lower camera placement/raster remains an explicit host boundary.
    v.hooks.pop(0x45461c,None)
    v.hooks.pop(0x454cef,None)
    v.ranges += [(0x454cef,0x454f10)]
    events.clear();v.hits.clear();before=capture()
    v.uc.emu_start(0x43419f,0x83f000,count=5000000)
    assert v.uc.reg_read(UC_X86_REG_EIP)==0x83f000
    after=capture();assert before['rng']==after['rng'] and kind[0] is None
    result=dict(executable_sha256=EDITION,before=before,after=after,events=list(events),hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))],boundaries=__doc__)
    if return_vm:return result,v,events,capture
    return result
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_tutorial_terrain.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['events']),'events;',sum(e['kind']=='speech' for e in r['events']),'speeches; completed original tutorial')
