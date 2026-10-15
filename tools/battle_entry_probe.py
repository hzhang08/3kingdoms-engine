#!/usr/bin/env python3
"""Original new-battle cleanup loop (0x44d8a0..0x44d94e) and focus selection.

The surrounding Win32 setup is not emulated. EBP supplies only scratch locals;
the exit boundary replaces the next original instruction, with no rule hook.
"""
import argparse,json
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EBP
from battle_initialization_probe import probe
from executable_probe import EDITION
LIVE=0x4b2c50

def generate(source):
    initial,v=probe(source,animation_state=True)
    v.ranges += [(0x44d8a0,0x44d94e),(0x418da0,0x418dc8),(0x43fc1a,0x43fc48),(0x44c38b,0x44c601),(0x403360,0x403375)]
    v.hooks[0x44d94e]=lambda vm:vm.return_from_hook()
    original=v.read(LIVE,115*36)
    def run():
        v.uc.reg_write(UC_X86_REG_EBP,0x832000)
        v.call(0x44d8a0)
    def rows():return [list(v.read(LIVE+r['slot']*36,36)) for r in initial['battle']]
    before=rows();run();after=rows()
    focus=[]
    for allies_first in (0,1):
        v.call(0x44c38b,[0x810000,allies_first],0x4b3d08);focus.append(dict(allies_first=allies_first,tile=list(v.read(0x810000,2))))
    cases=[]
    for slot in (0,18,35):
        for side in range(4):
            for lifecycle in (1,2,3):
                for flags,status in ((0,0),(128,30),(255,255)):
                    v.write(LIVE,original);p=LIVE+slot*36;v.write(p+5,bytes([side]));v.write(p+12,bytes([lifecycle,flags,3]));v.write(p+30,bytes([status]))
                    start=list(v.read(p,36));run();cases.append(dict(before=start,after=list(v.read(p,36))))
    return dict(executable_sha256=EDITION,before=before,after=after,focus=focus,cases=cases,
        boundaries='Original cleanup loop and all its rule getters/setters execute. Parent Win32 setup, initial fade and sprite cache population are excluded. Focus flag 0 is selected by new-battle caller 0x44d9b6; flag 1 belongs to loading caller 0x44d35c.')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_battle_entry.json'));a=p.parse_args();result=generate(a.source);a.output.write_text(json.dumps(result,indent=2)+'\n');print(len(result['cases']),'entry cases;',result['focus'])
