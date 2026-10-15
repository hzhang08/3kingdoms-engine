#!/usr/bin/env python3
"""Edition-gated original condition38/65/46 probes, no Windows process or saves.

Runs the original typed dispatcher and condition/range/getter callees. Existing
archive hooks are used only to prepare records; no hooks execute during cases.
Synthetic units, positions, classes and occupancy are explicit test inputs.
"""
import argparse, json, struct
from pathlib import Path
from battle_events_probe import prepare, READER, LIVE
from range_probe import configure
from movement_probe import live_json
from initialization_probe import state, UNIT
from executable_probe import EDITION

def run(source):
    _, v, _, _ = prepare(source)
    configure(v)
    v.ranges += [(0x41149d,0x411652),(0x411c36,0x411d63),
                 (0x45b2a8,0x45b371),(0x45b4a5,0x45b5fe)]
    image=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000)
    schema=json.loads(Path('tools/eex_schema.json').read_text())
    cases=[]
    def capture(op,args,units):
        v.write(0x400000,image);v.write(0x800000,scratch)
        v.write(LIVE,b'\0'*(115*36));v.write(0x818c80,b'\xff'*1600)
        for u in units:
            p=LIVE+u['slot']*36;v.put32(p,u['id'])
            v.write(p+4,bytes([u['slot'],u['side'],u['x'],u['y']]))
            v.write(p+12,bytes([u.get('lifecycle',2),0,1]));v.put32(p+16,u.get('hp',20))
            v.write(p+24,b'\x03'*6)
            if 'job' in u:v.write(UNIT+u['id']*72+0x2b,bytes([u['job']]))
            if u.get('lifecycle',2)==2:v.write(0x818c80+u['y']*20+u['x'],bytes([u['slot']]))
        raw=struct.pack('<H',op)
        for t,value in zip(schema[str(op)]['types'],args):
            raw+=struct.pack('<H',t)+struct.pack('<I' if t==4 else '<H',value&(0xffffffff if t==4 else 0xffff))
        v.write(0x820000,raw);v.put32(READER+8,0);v.put32(READER+12,0x820000)
        before=(v.read(LIVE,115*36),v.read(UNIT,512*72),v.u32(0x48ecd4));v.hits.clear()
        result=v.call(0x4109ef,this=READER)
        assert result in (0,1)
        assert v.u32(READER+12)==0x820000+len(raw)
        assert before==(v.read(LIVE,115*36),v.read(UNIT,512*72),v.u32(0x48ecd4))
        assert not v.hits.intersection(v.hooks)
        cases.append(dict(opcode=op,args=args,result=bool(result),units=[dict(battle=live_json(v,u['slot']),record=state(v,u['id'])) for u in units]))
    base=[dict(id=0,slot=0,side=0,x=8,y=8),dict(id=32,slot=15,side=1,x=9,y=9),dict(id=146,slot=35,side=2,x=10,y=10),dict(id=147,slot=36,side=3,x=11,y=11)]
    for selector in (0,32,146,147,511,1024,1025,1026):
      for rect in ((0,0,19,19),(8,8,8,8),(9,9,11,11),(10,11,11,10),(0,0,7,7)):
       for life in (1,2,3):
        for hp in (0,20):
         units=[dict(u,lifecycle=life,hp=hp) for u in base]
         capture(38,[selector,*rect],units)
    for side in range(7):
      for count in (0,1,2,4,5):
       for mode in (0,1,2):
        for region in (0,1):
         for life,hp in ((1,20),(2,0),(2,20),(3,0)):
          capture(65,[side,count,mode,region,9,9,11,11],[dict(u,lifecycle=life,hp=hp) for u in base])
    for dx,dy in ((0,1),(1,0),(1,1),(2,0),(0,0)):
      for job in (0,6,9,24,30):
       for mode in (0,1):
        for life,hp in ((1,20),(2,0),(2,20),(3,0)):
         if dx==dy==0:continue # Occupancy input must be unambiguous.
         capture(46,[0,146,mode],[dict(base[0],job=job),dict(base[2],x=8+dx,y=8+dy,lifecycle=life,hp=hp)])
    return dict(executable_sha256=EDITION,width=20,height=20,cases=cases,boundaries=__doc__,handlers={'38':'0x411c36','65':'0x41149d','46':'0x411a54'})

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));ap.add_argument('--out',type=Path,default=Path('tests/fixtures/original_prototype_scenario_events.json'));a=ap.parse_args()
    result=run(a.source);a.out.write_text(json.dumps(result,ensure_ascii=False,separators=(',',':'))+'\n');print(len(result['cases']),'original condition cases')
