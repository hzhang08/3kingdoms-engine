#!/usr/bin/env python3
"""All 4096 original quantized combat-palette lookup entries, no display APIs."""
import argparse,json
from pathlib import Path
from probe_vm import ProbeVM
from ls12 import decode
from executable_probe import EDITION

def generate(source):
    v=ProbeVM(source,[(0x42fb10,0x42fcc6),(0x47fa9e,0x47fb0a)])
    raw=decode((source/'Spalet.e5').read_bytes(),indices={0})[0].data
    quantized=bytes(c>>4 for c in raw);v.write(0x497770,quantized);lookup=[]
    colors=[tuple(quantized[n:n+3]) for n in range(0,768,3)]
    for value in range(4096):
        rgb=(value>>8,(value>>4)&15,value&15);v.write(0x800000,bytes(rgb));actual=v.call(0x42fb10,[0x800000,0x497770])&255
        dominant=max(range(3),key=lambda c:rgb[c]);best=0;worst=total=65535
        for index in range(10,246):
            color=colors[index];delta=[abs(a-b) for a,b in zip(color,rgb)]
            if max(delta)<=worst and sum(delta)<=total and max(range(3),key=lambda c:color[c])==dominant:
                best=index;worst=max(delta);total=sum(delta)
        assert best==actual,(rgb,best,actual)
        lookup.append(actual)
    return dict(executable_sha256=EDITION,quantized=list(quantized),lookup=lookup,boundaries=__doc__)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_combat_palette.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print('4096 original combat palette colors verified')
