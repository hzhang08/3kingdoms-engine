#!/usr/bin/env python3
"""Differential fixtures from this edition's data loader and portrait selector.

This is an offline CPU probe, not a Windows process. The emulator permits only
these two routines and their scalar accessors; no imports or I/O can execute.
"""
import argparse, hashlib, json, struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP
from executable_probe import EDITION
from ls12 import decode

def probe(source):
    binary=(source/'Ekd5.exe').read_bytes()
    if hashlib.sha256(binary).hexdigest()!=EDITION:raise ValueError('unrecognized executable edition')
    pe=pefile.PE(data=binary);uc=Uc(UC_ARCH_X86,UC_MODE_32)
    uc.mem_map(0x400000,0x200000);uc.mem_write(0x400000,pe.get_memory_mapped_image())
    uc.mem_map(0x800000,0x30000)
    allowed=[(0x408be3,0x408f2e),(0x4075a7,0x407757),(0x409500,0x409518),(0x409540,0x409571)]
    def guard(uc,address,size,user):
        if not any(a<=address<b for a,b in allowed):raise RuntimeError(f'unexpected execution at {address:#x}')
    uc.hook_add(UC_HOOK_CODE,guard)
    def call(addr,args=(),this=0):
        sp=0x81f000;uc.mem_write(sp,struct.pack('<'+'I'*(1+len(args)),0x82f000,*args))
        uc.reg_write(UC_X86_REG_ESP,sp);uc.reg_write(UC_X86_REG_ECX,this)
        uc.emu_start(addr,0x82f000,count=10000)
        if uc.reg_read(UC_X86_REG_EIP)!=0x82f000:raise RuntimeError('instruction budget exceeded')
        return uc.reg_read(UC_X86_REG_EAX)
    data=decode((source/'Data.e5').read_bytes(),indices={0})[0].data
    units=[]
    for uid in range(512):
        raw=data[uid*32:(uid+1)*32];ptr=0x4a1b68+uid*72
        uc.mem_write(0x800000,raw);end=call(0x408be3,[uid,0x800000,0],ptr)
        if end!=0x800020:raise RuntimeError('unexpected data record length')
        r=bytes(uc.mem_read(ptr,72));face=call(0x4075a7,(),ptr)
        units.append({'id':uid,'face':face,'face_base':struct.unpack_from('<I',r,4)[0],'story_sprite':r[2],
                      'base_stats':list(r[0x15:0x1a]),'base_hp':struct.unpack_from('<I',r,0x1c)[0],
                      'base_mp':r[0x20],'class':r[0x2b],'initial_level':r[0x2c],'initial_xp':r[0x2d],
                      'initial_equipment':[r[0x2e],r[0x31],r[0x34]]})
    return {'executable_sha256':EDITION,'source_data_sha256':hashlib.sha256((source/'Data.e5').read_bytes()).hexdigest(),
            'loader_va':'0x408be3','portrait_va':'0x4075a7','campaign_flags':0,'campaign_stage':0,'units':units,
            'limitations':['Initial data, without later script/equipment/level changes.','Portrait selection at zero campaign flags/stage only.']}
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('--out',type=Path,default=Path('tests/fixtures/original_units.json'));a=p.parse_args()
    a.out.write_text(json.dumps(probe(a.source),ensure_ascii=False,indent=2)+'\n');print(a.out)
