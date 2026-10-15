#!/usr/bin/env python3
"""Hash-gated original masked compositor, no Windows or host API calls.

Execute only 0x42fd5a..0x43030d in code-only executable pages. All other mapped
pages are non-executable; instruction budget and return address are checked.
The palette/lookup come from the previously verified opening palette importer.
Every source/background pair is tested for all actual mask values and amounts,
plus mask 255 and amount 0 boundary cases. Also retain all 30 full-frame hashes (including the unmasked compositor).
"""
import argparse, hashlib, json, struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_PROT_READ, UC_PROT_WRITE, UC_PROT_EXEC
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP
from executable_probe import EDITION
from mcall import decode_mcall
from ls12 import decode
from ui_assets import combat_palette

def generate(source):
    exe=(source/'Ekd5.exe').read_bytes()
    assert hashlib.sha256(exe).hexdigest()==EDITION
    image=pefile.PE(data=exe).get_memory_mapped_image()
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    u.mem_map(0x400000,0x200000,UC_PROT_READ|UC_PROT_WRITE)
    u.mem_write(0x400000,image)
    # Fill the rest of the executable page with traps, allowing this one routine.
    u.mem_write(0x42f000,b'\xcc'*0x2000)
    u.mem_write(0x42fd5a,image[0x2fd5a:0x300c8])
    u.mem_write(0x4300c8,image[0x300c8:0x3030e])
    u.mem_protect(0x42f000,0x2000,UC_PROT_READ|UC_PROT_EXEC)
    u.mem_map(0x800000,0x100000,UC_PROT_READ|UC_PROT_WRITE)
    pal=combat_palette(next(b.data for b in decode((source/'Spalet.e5').read_bytes()) if b.index==0))
    u.mem_write(0x497770,bytes(pal['quantized']));u.mem_write(0x4b1b28,bytes(pal['lookup']))
    def blend(src,mask,bg,amount,masked=True):
        assert len(src)==len(mask)==len(bg) and len(src)<=90000
        u.mem_write(0x800000,src);u.mem_write(0x820000,mask);u.mem_write(0x840000,bg)
        args=[len(src),1,0x800000,0x820000,0x840000,0x860000,0,amount] if masked else [len(src),1,0x800000,0x840000,0x860000,16,0,0,amount]
        u.mem_write(0x8f0000,struct.pack('<'+'I'*(len(args)+1),0x430fff,*args))
        u.reg_write(UC_X86_REG_ESP,0x8f0000)
        u.emu_start(0x4300c8 if masked else 0x42fd5a,0x430fff,count=len(src)*260+100)
        assert u.reg_read(UC_X86_REG_EIP)==0x430fff,'original compositor exhausted instruction budget'
        return bytes(u.mem_read(0x860000,len(src)))
    raw=(source/'Mcall05.e5').read_bytes();meta=decode_mcall(raw)
    masks=sorted(set(raw[620+30*90000:])|{255})
    src=bytes(n//256 for n in range(65536));bg=bytes(n%256 for n in range(65536))
    pairs=[]
    for amount in range(17):
        for mask in masks:
            output=blend(src,bytes([mask])*len(src),bg,amount)
            pairs.append(dict(amount=amount,mask=mask,pixels_sha256=hashlib.sha256(output).hexdigest()))
        print('verified masked amount',amount,flush=True)
    frames=[];bg=bytes((n*17+n//300*3)%256 for n in range(90000))
    for f in meta['frames']:
        output=blend(raw[f['pixel_offset']:f['pixel_offset']+90000],raw[f['mask_offset']:f['mask_offset']+90000] if f['masked'] else bytes(90000),bg,f['amount'],f['masked'])
        frames.append(dict(frame=f['image'],pixels_sha256=hashlib.sha256(output).hexdigest()))
    return dict(executable_sha256=EDITION,resource_sha256=meta['source_sha256'],pairs=pairs,frames=frames,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_mcall_blend.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['pairs'])*65536,'palette pairs and',len(r['frames']),'original full-frame blends')
