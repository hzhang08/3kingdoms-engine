#!/usr/bin/env python3
"""Original third tutorial through movement, cancellation and menu entry.

Continues the original caller from the second lesson. Executes movement search,
scripted walking, blue-overlay flag, attack mask, facing and cancellation.
The command-menu initializer is an explicit stop, not a skipped instruction.
Host text/raster/audio/wait boundaries are inherited from the verified chain.
"""
import argparse,json
from pathlib import Path
from unicorn import UC_HOOK_CODE
from tutorial_ally_probe import generate as prepare
from executable_probe import EDITION

class Boundary(Exception):pass

def generate(source,return_vm=False):
    entry,v,events,snapshot=prepare(source,True)
    assert entry==json.loads(Path('tests/fixtures/original_tutorial_ally.json').read_text())
    v.ranges += [(0x433bc8,0x43419f),(0x451c03,0x451c19),(0x4350c0,0x4350e8),(0x442b03,0x442bf6)]
    v.hooks.pop(0x433bc8)
    def capture():return {**snapshot(),'blue_movement':v.u32(0x4b4250+0x64),'occupancy':list(v.read(0x818c80,400))}
    def speech(vm):
        events.append(dict(kind='speech',unit=vm.arg(1),text=vm.read(vm.arg(0),2048).split(b'\0')[0].decode('gbk'),before=capture()));vm.return_from_hook()
    boundary=[]
    def menu(vm):
        boundary.append(dict(address='0x45bba0',args=[vm.arg(0)&255,vm.arg(1)&255],before=capture()));raise Boundary()
    v.hooks.update({0x42d5f1:speech,0x45bba0:menu})
    cancellation=[];cancel_memory=[];observing=[True]
    def observer(uc,address,size,data):
        if address==0x4350c0 and observing[0]:
            cancellation.append(dict(stage='before',destination=list(v.read(v.arg(0),2)),args=[v.arg(1),v.arg(2)],state=capture()))
            cancel_memory[:]=[(start,bytes(v.uc.mem_read(start,end-start+1))) for start,end,_ in v.uc.mem_regions()]
        if address==0x433efe and observing[0]:cancellation.append(dict(stage='after',state=capture()))
    v.uc.hook_add(UC_HOOK_CODE,observer)
    events.clear();v.hits.clear();before=capture()
    try:v.uc.emu_start(0x433bc8,0x83f000,count=5000000)
    except Boundary:pass
    assert len(boundary)==1,'menu boundary was not reached'
    allowed={int(a,16) for a in entry['hooked']}|{0x45bba0,0x47fd13,0x454b7e}
    unexpected=v.hits.intersection(v.hooks)-allowed
    assert not unexpected,[hex(a) for a in unexpected]
    assert len(cancellation)==2 and before['rng']==capture()['rng']
    result=dict(executable_sha256=EDITION,before=before,after=capture(),events=list(events),cancellation=list(cancellation),boundary=boundary[0],hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))],boundaries=__doc__)
    if return_vm:
        menu_memory=[(start,bytes(v.uc.mem_read(start,end-start+1))) for start,end,_ in v.uc.mem_regions()]
        menu_context=v.uc.context_save()
    # The active-unit relocation branch must preserve facing/flags even when
    # they differ from the demonstrated walk. Exercise the explicit no-op flag.
    cases=[];observing[0]=False
    for direction in range(4):
      for flags in (0,6,136):
       for skip in (0,1):
        for start,data in cancel_memory:v.uc.mem_write(start,data)
        actor=0x4b2c50;v.write(actor+15,bytes([direction]));v.write(actor+13,bytes([flags]));v.write(0x82e000,bytes([10,6]))
        original=v.read(actor,115*36);persistent=v.read(0x4a1b68,512*72);rng=v.u32(0x48ecd4);occupancy=v.read(0x818c80,400)
        value=v.call(0x4350c0,[0x82e000,0,skip],actor)
        actual=v.read(actor,115*36);expected=bytearray(original)
        if not skip:expected[6:8]=bytes([10,6])
        assert actual==bytes(expected) and v.read(0x4a1b68,512*72)==persistent and v.u32(0x48ecd4)==rng
        expected_grid=bytearray(occupancy)
        if not skip:expected_grid[10]=255;expected_grid[130]=0
        assert v.read(0x818c80,400)==bytes(expected_grid)
        cases.append(dict(direction=direction,flags=flags,skip=skip,result=value,before=list(original[:36]),after=list(actual[:36]),changed_occupancy=[i for i,(a,b) in enumerate(zip(occupancy,expected_grid)) if a!=b],other_units_persistent_rng_unchanged=True))
    result['cancel_cases']=cases
    if return_vm:
        for start,data in menu_memory:v.uc.mem_write(start,data)
        v.uc.context_restore(menu_context);events[:]=result['events']
        return result,v,events,capture
    return result

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_tutorial_actions.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['events']),'events;',len(r['cancellation']),'cancellation captures;',r['boundary']['address'])
