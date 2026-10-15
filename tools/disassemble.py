#!/usr/bin/env python3
"""Disassemble a bounded original-edition address range for local research."""
import argparse, hashlib
from pathlib import Path
import capstone, pefile
from executable_probe import EDITION

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('start',type=lambda s:int(s,0));p.add_argument('end',type=lambda s:int(s,0));p.add_argument('--exe',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz/Ekd5.exe'));a=p.parse_args()
    raw=a.exe.read_bytes()
    if hashlib.sha256(raw).hexdigest()!=EDITION:raise SystemExit('unsupported edition')
    pe=pefile.PE(data=raw);base=pe.OPTIONAL_HEADER.ImageBase;image=pe.get_memory_mapped_image()
    if not base<=a.start<a.end<=base+len(image):raise SystemExit('range outside image')
    cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    for i in cs.disasm(image[a.start-base:a.end-base],a.start):print(f'{i.address:08x} {i.mnemonic:7} {i.op_str}')
