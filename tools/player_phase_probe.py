#!/usr/bin/env python3
"""Probe automatic player-phase exit, including this edition's disabled autosave patch.
AutoDLL bytes run only in the bounded CPU emulator; no host DLL loader is used.
The first interactive message-loop instruction is an observation stop boundary.
"""
import argparse,json,hashlib
from pathlib import Path
import pefile
from battle_events_probe import prepare,LIVE,CTRL
from executable_probe import EDITION
PATCH_SHA='25816a4a462a713c380a94f5a2e6105f3ccec7a6eb4745b2f91be2374237c83b'
class InputReached(Exception):pass

def configure_patch(v,source):
    raw=(source/'AutoDLL.dll').read_bytes()
    assert hashlib.sha256(raw).hexdigest()==PATCH_SHA
    assert b'EnableAutoSave=0' in (source/'AutoDLL.ini').read_bytes()
    pe=pefile.PE(data=raw);base=pe.OPTIONAL_HEADER.ImageBase
    v.uc.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);v.write(base,pe.get_memory_mapped_image())
    v.put32(0x4cd014,0x100012b0);v.write(0x10009038,b'\0')
    v.ranges += [(0x44e0d9,0x44e1d3),(0x44e6ee,0x44e706),(0x44c38b,0x44c601),
       (0x100012b0,0x1000134b),(0x425ad0,0x425ae1)]

def generate(source):
    _,v,_,_=prepare(source);configure_patch(v,source)
    cameras=[]
    def camera(vm):cameras.append([vm.arg(0)&255,vm.arg(1)&255]);vm.return_from_hook(pop=8)
    def noop(vm):vm.return_from_hook()
    def input_boundary(vm):raise InputReached()
    v.hooks.update({0x45503f:camera,0x4759bc:noop,0x42b81b:noop,0x45461c:noop,0x455719:noop,0x44e1d3:input_boundary})
    original=v.read(LIVE,115*36);specs=[None]
    for slot in (0,14,15,34,35):
     for lifecycle in (1,2,3):
      for ai in (0,1,2,3,4,7):
       for flags in (0,2,128):specs.append(dict(slot=slot,lifecycle=lifecycle,ai=ai,flags=flags))
    cases=[]
    for spec in specs:
        v.write(LIVE,original);cameras.clear()
        if spec:
            p=LIVE+spec['slot']*36;v.write(p+4,bytes([spec['slot'],0,5,5]));v.write(p+12,bytes([spec['lifecycle'],spec['flags'],spec['ai']]))
        before=v.read(LIVE,115*36);requires_input=False
        try:v.call(0x44e0d9,this=CTRL)
        except InputReached:requires_input=True
        assert before==v.read(LIVE,115*36)
        cases.append(dict(input=spec,requires_input=requires_input,focus=list(cameras)))
    return dict(executable_sha256=EDITION,patch_sha256=PATCH_SHA,cases=cases,boundaries='Original candidate scan, focus search and AutoDLL ASave branch with EnableAutoSave=0. Camera requests captured; menu/cursor/overlay presentation omitted. Interactive path stops before the message pump. No host Windows code executes.')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_player_phase.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'player-phase gate cases; initial:',r['cases'][0])
