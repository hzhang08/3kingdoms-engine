#!/usr/bin/env python3
"""Original character/equipment XP record mutations at display boundaries.

Character XP uses the original no-animation argument. Equipment level-up
windows and unit-window refresh are captured; no fake getters or growth rules.
This does not establish level-up animation or its interaction with live HP.
"""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from initialization_probe import state,UNIT
from physical_resolution_probe import configure as physical_configure
from executable_probe import EDITION

def configure(v,events):
    physical_configure(v)
    v.ranges += [(0x408950,0x4089cb),(0x4082d6,0x4083ef),(0x40c6ea,0x40c7a9),
      (0x4095e0,0x4096a7)]
    def equipment_level(vm):
        uid,slot=vm.arg(0),vm.arg(1)&255
        events.append(dict(kind='equipment_level',unit=uid,slot=slot,level=vm.read(UNIT+uid*72+0x2f+slot*3,1)[0]))
        vm.return_from_hook()
    def growth(vm):
        # Observe after the original level write, without replacing code.
        from unicorn.x86_const import UC_X86_REG_EBP
        ptr=vm.u32(vm.uc.reg_read(UC_X86_REG_EBP)-0x18)
        events.append(dict(kind='growth',unit=(ptr-UNIT)//72,level=vm.read(ptr+0x2c,1)[0]))
    def refresh(vm):events.append(dict(kind='refresh',unit=vm.arg(0)));vm.return_from_hook()
    v.hooks.update({0x4083ef:equipment_level,0x4074d1:growth,0x479a01:refresh})

def generate(source):
    _,v,_,_=prepare(source);events=[];configure(v,events)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000)
    xp=[];equipment=[]
    def reset():v.write(0x400000,original);v.write(0x800000,scratch);events.clear()
    def capture(uid,total):
        before=state(v,uid);live=v.read(LIVE,115*36);rng=v.u32(0x48ecd4);v.hits.clear()
        v.call(0x408950,[total,0],UNIT+uid*72)
        assert v.read(LIVE,115*36)==live and v.u32(0x48ecd4)==rng
        assert not (v.hits.intersection(v.hooks)-{0x4074d1,0x479a01})
        xp.append(dict(before=before,total=total,after=state(v,uid),events=list(events)))
    for job in range(53):
     for level in (1,3,14,15,29,49,50):
      for total in (0,99,100,101,250,500):
        reset();v.write(UNIT+0x2b,bytes([job,level]));capture(0,total)
    for uid in (1,25,26,32,234):
     for owner in (0,255):
      for total in (0,100,500):
        reset();v.write(UNIT+uid*72+0x2a,bytes([owner]));capture(uid,total)
    for attr in (0,24,25,34,35,44,45,255):
        reset();v.write(UNIT+0x21,bytes([attr])*5);v.write(UNIT+0x15,bytes([254])*5)
        v.put32(UNIT+0x1c,499);v.write(UNIT+0x20,b'\xf9');capture(0,250)
    for item in range(104):
     for slot in range(3):
      for level,old,gain in ((1,0,0),(1,99,1),(1,98,250),(2,99,1),(3,0,100),(8,99,1),(9,255,100)):
        reset();v.write(UNIT+0x2e+slot*3,bytes([item,level,old]))
        before=state(v,0);live=v.read(LIVE,115*36);rng=v.u32(0x48ecd4);v.hits.clear()
        v.call(0x4082d6,[slot,gain],UNIT)
        assert v.read(LIVE,115*36)==live and v.u32(0x48ecd4)==rng
        assert not (v.hits.intersection(v.hooks)-{0x4083ef})
        equipment.append(dict(before=before,slot=slot,gain=gain,after=state(v,0),events=list(events)))
    for slot in range(3):
        reset();v.write(UNIT+0x2e+slot*3,b'\xff'*3);before=state(v,0)
        v.call(0x4082d6,[slot,1000],UNIT)
        equipment.append(dict(before=before,slot=slot,gain=1000,after=state(v,0),events=list(events)))
    return dict(executable_sha256=EDITION,xp=xp,equipment=equipment,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_experience.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print({k:len(v) for k,v in r.items() if isinstance(v,list)})
