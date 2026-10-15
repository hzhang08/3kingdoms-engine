#!/usr/bin/env python3
"""Original placement search, including occupied/impassable fallback ordering."""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare
from executable_probe import EDITION

def generate(source):
    _,v,_,_=prepare(source);v.ranges += [(0x43f040,0x43f38b)]
    terrain=list(v.read(0x818640,400));original=list(v.read(0x818c80,400))
    grids=[original,[255]*400,[0]*400,[0]*400,[0 if 5<=x<=15 and 5<=y<=15 else 255 for y in range(20) for x in range(20)]]
    for x,y in ((3,3),(16,3),(3,16),(16,16)):grids[3][y*20+x]=255
    cases=[]
    points=[(0,0),(19,0),(0,19),(19,19),(10,10),(13,12),(9,11),(12,19),(0,10),(19,10),(255,255)]
    for grid,occupancy in enumerate(grids):
        v.write(0x818c80,bytes(occupancy))
        for group in range(27):
            for x,y in points:
                v.call(0x43f040,[0x823000,x|(y<<8),group]);out=list(v.read(0x823000,2))
                cases.append(dict(grid=grid,group=group,xy=[x,y],result=out))
    return dict(executable_sha256=EDITION,terrain=terrain,occupancy=grids,cases=cases,
        boundaries='Original 0x43f040, terrain/occupancy readers, cardinal-neighbor helper and Manhattan fallback execute without hooks. Empty result is the original [255,255] sentinel, not a substituted tile.')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_placement.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,indent=2)+'\n');print(len(r['cases']),'placement cases')
