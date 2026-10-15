#!/usr/bin/env python3
"""Original turn-start calls in the order established by 0x44ed93/0x44e851."""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE,CTRL
from executable_probe import EDITION

def generate(source):
    initial,v,raw,events=prepare(source)
    v.ranges += [(0x43c738,0x43d108),(0x41def0,0x41df0b),(0x41e030,0x41e041),
      (0x4066e0,0x406700),(0x435182,0x4351da),(0x435991,0x4359f5),
      (0x43ff54,0x43ff6e),(0x44df61,0x44e0d9),(0x407a09,0x407b41),
      (0x4082d6,0x4083ed),(0x409650,0x40969f),(0x408950,0x4089cb),(0x40709a,0x40721b),(0x406650,0x406670)]
    rows=lambda:[list(v.read(LIVE+r['slot']*36,36)) for r in initial['battle']]
    before=rows();persistent=v.read(0x4a1b68,512*72)
    # Player/allied side first, exactly as the original caller orders it.
    for slot in range(35):
        v.call(0x43c7d6,this=LIVE+slot*36);v.call(0x43c975,this=LIVE+slot*36)
    v.call(0x44df61,[0],CTRL)
    for slot in range(35):v.call(0x43cddd,this=LIVE+slot*36)
    after=rows()
    return dict(executable_sha256=EDITION,before=before,after=after,persistent_unchanged=v.read(0x4a1b68,512*72)==persistent,
        boundaries='Original first player-side turn-start status recovery, grade recovery, poison and terrain/equipment recovery calls. No hooks on these rules. Only the established construction hooks precede them. No AI/player commands execute.')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_turn_start.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,indent=2)+'\n');print('First phase changes:',sum(x!=y for x,y in zip(r['before'],r['after'])),'persistent unchanged:',r['persistent_unchanged'])
