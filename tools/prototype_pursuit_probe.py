#!/usr/bin/env python3
"""Original post-controller pursuit target gate and dispatch, not a full battle.

Executes 0x43ac44..0x43acfa: target HP, effect46, scratch-grid clearing, range
getter and target selector. Facing 0x435829 and next controller 0x4064f1 are
dispatch boundaries, not replaced combat rules. Stops before action flags.
Synthetic enemy Lu Bu at10,10; first target north, candidates east/south/west.
The already-tested controller's counter gate writes preferred=actor at0x40642f
before testing dead defender HP; preferred35 reproduces that caller state.
Other preferences characterize the selector without extrapolating their reachability.
"""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from range_probe import configure
from initialization_probe import UNIT
from executable_probe import EDITION
from unicorn.x86_const import UC_X86_REG_EBP

def generate(source):
    _,v,_,_=prepare(source);configure(v)
    v.ranges += [(0x43ac44,0x43acfa),(0x47fd13,0x47fd40)]
    class Done(Exception):pass
    calls=[]
    def stop(vm):raise Done()
    def face(vm):
        calls.append(dict(kind='face',actor=vm.arg(0)&255,target=vm.arg(1)&255))
        vm.return_from_hook()
    def attack(vm):
        calls.append(dict(kind='attack',actor=vm.arg(0)&255,target=vm.arg(1)&255))
        vm.return_from_hook(pop=8)
    v.hooks.update({0x43acfa:stop,0x435829:face,0x4064f1:attack})
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000)
    results=[]
    for item in [3,27]:
     for hp in [0,1]:
      for preferred in [0,1,2,3,35,255]:
       for living in [[],[1],[1,2,3]]:
        v.write(0x400000,original);v.write(0x800000,scratch);calls.clear()
        v.write(LIVE,bytes(115*36));v.write(0x818c80,b'\xff'*1600)
        rows=[(35,119,2,10,10,100),(0,0,0,10,9,hp)]
        rows += [(k,k,0,*{1:(11,10),2:(10,11),3:(9,10)}[k],100) for k in living]
        for slot,uid,side,x,y,h in rows:
            v.put32(LIVE+slot*36,uid);v.write(LIVE+slot*36+4,bytes([slot,side,x,y]))
            v.write(LIVE+slot*36+12,b'\x02');v.put32(LIVE+slot*36+16,h)
            v.write(0x818c80+y*20+x,bytes([slot]))
        v.write(UNIT+119*72+0x2e,bytes([item,1,0,255,255,255,255,255,255]))
        v.write(UNIT+119*72+0x2b,bytes([9]))
        v.write(0x4b2c28,bytes([preferred]));v.put32(0x48ecd4,42)
        fp=0x82e100;v.uc.reg_write(UC_X86_REG_EBP,fp)
        v.put32(fp-0x10,LIVE+35*36);v.put32(fp-8,0)
        before=v.read(LIVE,115*36),v.read(UNIT,512*72)
        try:v.call(0x43ac44,budget=1000000)
        except Done:pass
        else:raise AssertionError('missing terminal pursuit boundary')
        assert before==(v.read(LIVE,115*36),v.read(UNIT,512*72)) and v.u32(0x48ecd4)==42
        results.append(dict(item=item,initial_target_hp=hp,preferred=preferred,living_targets=living,calls=list(calls),rng=42))
    return dict(executable_sha256=EDITION,routine='0x43ac44..0x43acfa',cases=results,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'))
    p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_prototype_pursuit.json'))
    a=p.parse_args();report=generate(a.source);a.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    print(len(report['cases']),'original pursuit gate/target/dispatch cases; records and RNG unchanged')
