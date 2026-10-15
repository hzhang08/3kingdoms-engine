#!/usr/bin/env python3
"""Original quantized color matching/darken mapping, without display APIs."""
import argparse,json
from pathlib import Path
from probe_vm import ProbeVM
from ls12 import decode
from ui_assets import darken_palette
from executable_probe import EDITION

def generate(source):
    v=ProbeVM(source,[(0x42fb10,0x42fcc6),(0x430480,0x43053e),(0x47fad0,0x47fb0a)])
    raw=decode((source/'Spalet.e5').read_bytes(),indices={0})[0].data
    quantized=bytes(c>>4 for c in raw);v.write(0x497770,quantized)
    targets={tuple(max(0,c-4) for c in quantized[n:n+3]) for n in range(3,768,3)}
    for rgb in sorted(targets):
        v.write(0x800000,bytes(rgb));index=v.call(0x42fb10,[0x800000,0x497770])&255
        v.write(0x4b1b28+(rgb[0]<<8)+(rgb[1]<<4)+rgb[2],bytes([index]))
    v.write(0x800000,bytes(range(256)));v.call(0x430480,[0x800000,256,1,4]);actual=list(v.read(0x800000,256))
    assert actual==darken_palette(raw)
    return dict(executable_sha256=EDITION,darken=actual,boundaries='Original 0x42fb10 matches every color needed by this palette after amount-4 subtraction. Original 0x430480 darkens all 256 indices; original 0x47fad0 subtraction. Source palette quantization is 0x41e1a4 (>>4). No rendering/Win32 APIs. Display palette upload/color depth remains separate.')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_palette.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print('256 original darkened palette indices matched')
