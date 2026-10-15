#!/usr/bin/env python3
"""Original area selector/status mutation and MEFF48 compositor boundary cases.

The isolated area probe intercepts presentation 0x43e69f but runs occupancy,
side/lifecycle filtering and status mutation. The separate sequential status
fixture executes that presentation too. The blend probe runs only executable
compositor pages, with no host or rule hooks. Source game files are read-only.
"""
import argparse,hashlib,json,struct
from pathlib import Path
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_PROT_READ,UC_PROT_WRITE,UC_PROT_EXEC
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
from battle_events_probe import prepare,LIVE
from executable_probe import EDITION
from ui_assets import combat_palette
from ls12 import decode

def generate(source):
    _,v,_,_=prepare(source)
    v.ranges += [(0x45ade5,0x45afae),(0x4066c0,0x4066e0)]
    events=[]
    def present(vm):
        slot,category,value=[vm.arg(n)&255 for n in range(3)]
        events.append(dict(slot=slot,category=category,value=value,status=vm.read(LIVE+slot*36+30,1)[0]))
        vm.return_from_hook()
    v.hooks[0x43e69f]=present
    original=v.read(LIVE,115*36);persistent=v.read(0x4a1b68,512*72);rng=v.u32(0x48ecd4)
    cases=[]
    specs=[dict(filter=f,side=s,life=life,status=status,rect=[0,0,19,19])
        for f in range(8) for s in (0,1,2,3,255) for life in (0,1,2,3,4) for status in (0,8,16)]
    specs += [dict(filter=6,side=2,life=2,status=16,rect=r)
        for r in ([5,5,4,4],[8,9,8,9],[9,9,9,9],[0,0,7,8])]
    for spec in specs:
        v.write(LIVE,original);v.write(0x818c80,b'\xff'*1600);events.clear();v.hits.clear()
        # Four units with slot order intentionally different from map order.
        slots=(35,36,37,38);positions=((8,9),(9,9),(9,8),(8,8))
        for slot,xy in zip(slots,positions):
            p=LIVE+slot*36
            v.write(p+5,bytes([spec['side'],*xy]));v.write(p+12,bytes([spec['life']]))
            v.write(p+30,bytes([spec['status']]))
            v.write(0x818c80+xy[1]*20+xy[0],bytes([slot]))
        before=[list(v.read(LIVE+n*36,36)) for n in slots]
        v.call(0x45ade5,[*spec['rect'],spec['filter'],6,8],0x4b5df0)
        assert v.read(0x4a1b68,512*72)==persistent and v.u32(0x48ecd4)==rng
        assert not(v.hits.intersection(v.hooks)-{0x43e69f,0x43fcc9,0x479a01})
        cases.append(dict(spec=spec,before=before,after=[list(v.read(LIVE+n*36,36)) for n in slots],presentations=list(events)))
    raw=(source/'Ekd5.exe').read_bytes();assert hashlib.sha256(raw).hexdigest()==EDITION
    image=pefile.PE(data=raw).get_memory_mapped_image();u=Uc(UC_ARCH_X86,UC_MODE_32)
    u.mem_map(0x400000,0x200000,UC_PROT_READ|UC_PROT_WRITE);u.mem_write(0x400000,image)
    u.mem_write(0x42f000,b'\xcc'*0x2000);u.mem_write(0x42fd5a,image[0x2fd5a:0x300c8]);u.mem_protect(0x42f000,0x2000,UC_PROT_READ|UC_PROT_EXEC)
    u.mem_map(0x800000,0x100000,UC_PROT_READ|UC_PROT_WRITE)
    pal=combat_palette(next(b.data for b in decode((source/'Spalet.e5').read_bytes()) if b.index==0))
    u.mem_write(0x497770,bytes(pal['quantized']));u.mem_write(0x4b1b28,bytes(pal['lookup']))
    u.mem_write(0x800000,bytes(n//256 for n in range(65536)));u.mem_write(0x820000,bytes(n%256 for n in range(65536)))
    blends=[]
    for weight in range(5):
        u.mem_write(0x8f0000,struct.pack('<10I',0x430fff,256,256,0x800000,0x820000,0x840000,weight,0,0,0));u.reg_write(UC_X86_REG_ESP,0x8f0000)
        u.emu_start(0x42fd5a,0x430fff,count=18000000);assert u.reg_read(UC_X86_REG_EIP)==0x430fff
        blends.append(dict(weight=weight,amount=0,pixels_sha256=hashlib.sha256(u.mem_read(0x840000,65536)).hexdigest()))
    effect=next(b.data for b in decode((source/'Meff.e5').read_bytes(),indices={48}))
    return dict(executable_sha256=EDITION,resource_sha256=hashlib.sha256(effect).hexdigest(),cases=cases,blends=blends,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_confusion.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'area cases and',len(r['blends'])*65536,'palette pairs')
