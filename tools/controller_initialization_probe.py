#!/usr/bin/env python3
"""Execute the opening controller initialization block from the original EXE.

The input is the configuration written by the actual opcode-69 handler. The
bounded straight-line block 0x44d4d5..0x44d583 and original limit getter run
without hooks; the surrounding Windows UI and resource setup are not executed.
"""
from unicorn.x86_const import UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP
CTRL=0x4b3d08

def initialize(v):
    v.ranges += [(0x44d4d5,0x44d583),(0x44f900,0x44f919)]
    config=dict(region=v.read(0x496e21,1)[0],selector=v.read(0x496e22,1)[0],limit=v.read(0x496c9e,1)[0])
    before=list(v.read(CTRL,0x12));rng=v.u32(0x48ecd4)
    frame=0x82f000;v.put32(frame-0x130,CTRL)
    v.uc.reg_write(UC_X86_REG_EBP,frame);v.uc.reg_write(UC_X86_REG_ESP,frame-0x200)
    v.hits.clear();v.uc.emu_start(0x44d4d5,0x44d583,count=1000)
    assert v.uc.reg_read(UC_X86_REG_EIP)==0x44d583 and not v.hits.intersection(v.hooks)
    # Original new-battle terrain-effect table: 250 five-byte empty records.
    v.ranges.append((0x44d6ee,0x44d7a3))
    v.uc.emu_start(0x44d6ee,0x44d7a3,count=20000)
    assert v.uc.reg_read(UC_X86_REG_EIP)==0x44d7a3
    assert v.read(CTRL+0x19,1250)==b"\xff"*1250
    after=list(v.read(CTRL,0x12))
    assert after[1:6]==[0,0,config['limit'],config['selector'],0] and after[16:18]==[0,config['region']]
    assert v.u32(0x48ecd4)==rng
    return dict(configuration=config,before=before,after=after,rng=rng,boundaries=__doc__)
