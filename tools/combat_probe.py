#!/usr/bin/env python3
"""Probe original physical damage arithmetic with explicitly stubbed getters.

This does NOT verify terrain/stat lookup, equipment semantics, hit probability
or complete battle RNG consumption. Fixture metadata lists every replacement.
The original arithmetic and LCG instructions execute unmodified in a bounded VM.
"""
import argparse,hashlib,itertools,json,random,struct
from pathlib import Path
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP
from executable_probe import EDITION

def probe(exe):
    binary=exe.read_bytes()
    if hashlib.sha256(binary).hexdigest()!=EDITION:raise ValueError('unrecognized executable edition')
    pe=pefile.PE(data=binary);uc=Uc(UC_ARCH_X86,UC_MODE_32);uc.mem_map(0x400000,0x200000);uc.mem_write(0x400000,pe.get_memory_mapped_image());uc.mem_map(0x800000,0x20000)
    attacker=0x4b2c50;target=0x4b2c74
    uc.mem_write(attacker,struct.pack('<I',0));uc.mem_write(target,struct.pack('<I',1))
    current={};calls=[]
    stub_names={0x40be7d:'unit resolver',0x43e4d6:'battle slot resolver',0x43f84d:'terrain factor',0x43b470:'class group',0x43f4fe:'attack stat',0x43f595:'defense stat',0x43b4b0:'hit result',0x43e3f0:'level',0x45f670:'unit ID',0x407a09:'equipment predicate',0x407af2:'equipment percentage',0x472c90:'target HP'}
    def hook(uc,addr,size,user):
        if addr not in stub_names:
            if not (0x43bc4b<=addr<0x43bf96 or 0x47fa9e<=addr<0x47fab5 or 0x47fae7<=addr<0x47fb32):raise RuntimeError(f'unexpected execution at {addr:#x}')
            return
        sp=uc.reg_read(UC_X86_REG_ESP);this=uc.reg_read(UC_X86_REG_ECX);arg=struct.unpack('<I',uc.mem_read(sp+4,4))[0];pop=0
        if addr==0x40be7d:val=1
        elif addr==0x43e4d6:val=1
        elif addr==0x43f84d:val=current['attack_terrain' if this==attacker else 'defense_terrain']
        elif addr==0x43b470:val=current['attack_class_group' if this==attacker else 'defense_class_group']
        elif addr==0x43f4fe:val=current['attack']
        elif addr==0x43f595:val=current['defense']
        elif addr==0x43b4b0:val=current['hit'];pop=12
        elif addr==0x43e3f0:val=current['level']
        elif addr==0x45f670:val=1
        elif addr in (0x407a09,0x407af2):
            if arg not in (0x3d,0x2d):raise RuntimeError(f'unexpected equipment effect {arg:#x}')
            percent=current['mounted_resistance_percent' if arg==0x3d else 'anti_infantry_percent'];val=(percent>0) if addr==0x407a09 else percent;pop=4
        elif addr==0x472c90:val=current['target_hp']
        calls.append(hex(addr));ret=struct.unpack('<I',uc.mem_read(sp,4))[0];uc.reg_write(UC_X86_REG_EAX,int(val));uc.reg_write(UC_X86_REG_ESP,sp+4+pop);uc.reg_write(UC_X86_REG_EIP,ret)
    uc.hook_add(UC_HOOK_CODE,hook)
    fixture_random=random.Random(0x43bc4b);cases=[]
    for ac,dc in itertools.product(range(28),repeat=2):
        seed=fixture_random.getrandbits(32)
        current={'attack':fixture_random.choice([1,49,70,127,250,500]),'defense':fixture_random.choice([1,36,71,128,251,500]),'attack_terrain':fixture_random.choice([8,10,11,12]),'defense_terrain':fixture_random.choice([8,10,11,12]),'level':fixture_random.choice([1,3,7,50]),'attack_class_group':ac,'defense_class_group':dc,'target_hp':fixture_random.choice([0,1,3,20,100,500]),'mounted_resistance_percent':fixture_random.choice([0,10,25,100]),'anti_infantry_percent':fixture_random.choice([0,10,25,100]),'randomize':bool(fixture_random.randrange(2)),'counterattack':bool(fixture_random.randrange(2)),'hit':bool(fixture_random.randrange(4))}
        calls=[];sp=0x810000;uc.mem_write(sp,struct.pack('<IIII',0x81f000,1,current['randomize'],current['counterattack']));uc.reg_write(UC_X86_REG_ESP,sp);uc.reg_write(UC_X86_REG_ECX,attacker);uc.mem_write(0x48ecd4,struct.pack('<I',seed));uc.emu_start(0x43bc4b,0x81f000,count=10000)
        if uc.reg_read(UC_X86_REG_EIP)!=0x81f000:raise RuntimeError('instruction budget exceeded')
        cases.append({'input':current,'seed':seed,'damage':uc.reg_read(UC_X86_REG_EAX),'final_rng':struct.unpack('<I',uc.mem_read(0x48ecd4,4))[0]})
    return {'executable_sha256':EDITION,'routine':'0x43bc4b','stubbed_getters':{hex(a):n for a,n in stub_names.items()},'cases':cases,'limitations':['Arithmetic only. Getter values are controlled fixtures, not observed battle statistics.','Hit check is stubbed; only the subsequent damage random draw is verified.']}
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('--out',type=Path,default=Path('tests/fixtures/original_physical_arithmetic.json'));a=p.parse_args();a.out.write_text(json.dumps(probe(a.exe),indent=2)+'\n');print(a.out)
