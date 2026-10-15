#!/usr/bin/env python3
"""Original environment selector/RNG block for every class and random outcome.

Runs 0x449fc7..0x44a0c1 with the original caller's stack locals, followed by the
real classification getter. Explicit inputs cover five table rows, six selector
states, six forced selectors and the random mode, with all eight RNG outcomes.
Stops before presentation/destruction. No rule, RNG or getter hook is used.
"""
import argparse,json
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP
from deployment_probe import generate as prepare
from controller_initialization_probe import initialize,CTRL
from executable_probe import EDITION

def generate(source):
    _,v=prepare(source,True);initial=initialize(v)
    v.ranges += [(0x449fc7,0x44a0c1),(0x44a1d4,0x44a1f9),(0x47fae7,0x47fb62)]
    seeds={}
    for seed in range(256):
        v.put32(0x48ecd4,seed);roll=v.call(0x47fb13,[8]);seeds.setdefault(roll,seed)
    assert sorted(seeds)==list(range(8))
    rows=[];frame=0x82f000
    for region in range(5):
     for selector in range(6):
      for override in [255,0,1,2,3,4,5]:
       for roll,seed in sorted(seeds.items()):
        v.write(CTRL+4,bytes([selector]));v.write(CTRL+17,bytes([region]));v.put32(0x48ecd4,seed)
        v.put32(frame-0x128,CTRL);v.put32(frame+8,override)
        v.uc.reg_write(UC_X86_REG_EBP,frame);v.uc.reg_write(UC_X86_REG_ESP,frame-0x200)
        v.hits.clear();v.uc.emu_start(0x449fc7,0x44a0c1,count=1000)
        assert v.uc.reg_read(UC_X86_REG_EIP)==0x44a0c1 and not v.hits.intersection(v.hooks)
        previous=v.read(frame-0x114,1)[0];following=v.call(0x44a1d4,this=CTRL)&255
        assert v.read(frame-0x118,1)[0]==roll
        rows.append(dict(region=region,selector=selector,override=override,seed=seed,roll=roll,
          before_class=previous,after_class=following,after_selector=v.read(CTRL+4,1)[0],rng=v.u32(0x48ecd4)))
    return dict(executable_sha256=EDITION,initialization=initial,classification_table=[list(v.read(0x48e100+r*6,6)) for r in range(5)],cases=rows,boundaries=__doc__)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_environment.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'original environment transitions')
