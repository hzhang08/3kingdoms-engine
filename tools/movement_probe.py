#!/usr/bin/env python3
"""Hash-gated original movement points, zone-of-control and distance search.

No rule or rendering hooks occur in these routines. Search modes 0 and 2 are
captured; AI scoring (mode 4) and target lookup (mode 8) are outside this probe.
"""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from initialization_probe import state,UNIT
from executable_probe import EDITION
from game_state import default_dictionary

def configure(v):
    v.ranges += [(0x43f74f,0x43f84d),(0x40727b,0x4072bd),(0x40b8b0,0x40b8c0),
      (0x4066e0,0x40672b),(0x47fad0,0x47fae7),(0x407a09,0x407b41),
      (0x437d59,0x437e2e),(0x438308,0x438965),(0x435884,0x435a91),
      (0x43b420,0x43b461),(0x47fd13,0x47fd40)]
    v.put32(0x4aab68+4,0x2000);v.put32(0x4aab68+12,0x824000);v.write(0x4aab68+16,b'\x01')

def live_json(v,slot):
    row=default_dictionary().read_values(v.read,'live_units',slot)
    # Keep the established fixture shape; AI priority has its own oracle.
    return {k:value for k,value in row.items() if k!='priority'}

def generate(source):
    _,v,_,_=prepare(source);configure(v)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);search=[];points=[]
    def reset():v.write(0x400000,original);v.write(0x800000,scratch)
    def capture(slot,budget,target=(255,255),mode=0,zoc=True):
        width,height=v.read(0x4b422c,2);b=live_json(v,slot);v.write(0x82e000,bytes([b['x'],b['y']]))
        v.write(0x8192c0,b'\xa5'*(width*height))
        before=v.read(LIVE,115*36);persistent=v.read(UNIT,512*72);rng=v.u32(0x48ecd4)
        result=v.call(0x438308,[0x82e000,budget,mode,*target,int(zoc)],LIVE+slot*36,budget=3000000)&255
        assert before==v.read(LIVE,115*36) and persistent==v.read(UNIT,512*72) and rng==v.u32(0x48ecd4)
        roster=[live_json(v,n) for n in range(115) if v.read(LIVE+n*36+12,1)[0]==2]
        search.append(dict(unit=state(v,b['id']),battle=b,roster=roster,width=width,height=height,
          terrain=list(v.read(0x818640,width*height)),budget=budget,target=target,mode=mode,respect_zoc=zoc,
          result=result,costs=list(v.read(0x8192c0,width*height))))
    # Real opening roster, both short and long ranges and target early exits.
    for slot in (18,19,20,35,36,48):
      for budget in (0,4,10):
       for zoc in (False,True):reset();capture(slot,budget,zoc=zoc)
    for target in ((10,17),(11,17),(11,15),(9,11),(0,0)):
        reset();capture(20,10,target,2)
    reset();b=live_json(v,20);capture(20,10,(b['x'],b['y']),2)
    for job in range(53):
     for flags in (0,128):
      for zoc in (False,True):
        reset();v.load_unit(0);v.write(UNIT+0x2b,bytes([job]));v.write(UNIT+0x2e,b'\xff'*9)
        v.write(LIVE,b'\0'*(115*36));v.write(0x818c80,b'\xff'*1600);v.write(0x4b422c,bytes([9,9]))
        v.write(0x818640,bytes((x*5+y*7+job)%30 for y in range(9) for x in range(9)))
        # Adjacent enemy, friendly occupied cell and enemy-controlled corridor.
        for slot,x,y in ((0,4,4),(18,4,3),(35,6,4),(36,3,6)):
            v.put32(LIVE+slot*36,0);v.write(LIVE+slot*36+4,bytes([slot,slot%4,x,y]));v.write(LIVE+slot*36+12,b'\x02')
            v.write(0x818c80+y*9+x,bytes([slot]))
        v.write(LIVE+13,bytes([flags]));capture(0,7,zoc=zoc)
    for accessory in (255,62,63):
     for terrain in range(30):
        reset();v.load_unit(0);v.write(UNIT+0x2e,bytes([255]*6+[accessory,255,255]))
        v.write(LIVE,b'\0'*(115*36));v.write(0x818c80,b'\xff'*1600);v.write(0x4b422c,bytes([9,9]))
        v.write(0x818640,bytes([terrain])*81)
        for slot,x,y in ((0,4,4),(35,5,4),(36,4,2)):
            v.write(LIVE+slot*36+4,bytes([slot,0,x,y]));v.write(LIVE+slot*36+12,b'\x02');v.write(0x818c80+y*9+x,bytes([slot]))
        capture(0,6)
    reset();v.load_unit(0)
    for job in range(53):
     for accessory in (255,59,60,61):
      for grade in range(7):
       for status in (0,2):
        v.write(UNIT+0x2b,bytes([job]));v.write(UNIT+0x2e,bytes([255]*6+[accessory,255,255]))
        v.write(LIVE+29,bytes([grade,status]));value=v.call(0x43f74f,this=LIVE)&255
        points.append(dict(job=job,accessory=accessory,grade=grade,status=status,points=value))
    return dict(executable_sha256=EDITION,search=search,points=points,
      boundaries='Unhooked original movement-point getter, equipment/terrain costs, cardinal neighbors, allegiance/ZOC and 0x438308 distance search modes 0/2. Original opening roster and explicit synthetic terrain/roster/equipment/flag cases. Searches assert no unit/persistent/RNG mutations. Costs 165 mean untouched caller scratch for the origin==target early return. No player command, movement animation or AI scoring/action executes.')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_movement.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['search']),'searches;',len(r['points']),'movement-point cases')
