#!/usr/bin/env python3
"""Run bounded, isolated x86 arithmetic routines for differential fixtures.
No Windows loader, syscalls, UI, file writes, or helpers execute in this VM.
Only use the identified edition; every code address is hash gated.
"""
from pathlib import Path
import argparse,hashlib,json,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP
EDITION='199b1b67cf966ead46c49b430b9aab84bdc7d4f0e8dfa8e0ebeec5da280b01e0'

def probe(exe):
    data=exe.read_bytes()
    if hashlib.sha256(data).hexdigest()!=EDITION:raise ValueError('unrecognized executable edition')
    pe=pefile.PE(data=data);uc=Uc(UC_ARCH_X86,UC_MODE_32);uc.mem_map(0x400000,0x200000);uc.mem_write(0x400000,pe.get_memory_mapped_image());uc.mem_map(0x800000,0x20000)
    def call(addr,args=(),this=0):
        sp=0x810000;uc.mem_write(sp,struct.pack('<'+'I'*(len(args)+1),0x81f000,*args));uc.reg_write(UC_X86_REG_ESP,sp);uc.reg_write(UC_X86_REG_ECX,this);uc.emu_start(addr,0x81f000,count=20000)
        if uc.reg_read(UC_X86_REG_EIP)!=0x81f000:raise RuntimeError('probe instruction budget exceeded')
        return uc.reg_read(UC_X86_REG_EAX)
    rng=[]
    for seed in [0,1,42,0xffffffff,0x80000000]:
        uc.mem_write(0x48ecd4,struct.pack('<I',seed));values=[call(0x47fae7) for _ in range(32)];state=struct.unpack('<I',uc.mem_read(0x48ecd4,4))[0];rng.append({'seed':seed,'values':values,'final_state':state})
    return {'executable_sha256':EDITION,'rng':{'routine_va':'0x47fae7','state_va':'0x48ecd4','multiply':1103515245,'increment':12345,'output':'(state >> 16) & 32767','fixtures':rng},'limitations':['Arithmetic probe only; full battle RNG consumption order remains unverified.']}
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('--out',type=Path,default=Path('tests/fixtures/original_rng.json'));a=p.parse_args();a.out.write_text(json.dumps(probe(a.exe),indent=2)+'\n');print(a.out)
