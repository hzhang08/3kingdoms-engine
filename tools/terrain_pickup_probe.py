#!/usr/bin/env python3
"""Original typed occupancy/one-time conditions and consumable inventory increment.
Exact script conditions execute, using synthetic one-unit boards. Archive hooks
only construct input. No rule under test is substituted. Animation is separate.
"""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE,READER
from prototype_terrain_assets import locations
from eex import parse
from executable_probe import EDITION
def generate(source):
 _,v,_,_=prepare(source);v.ranges += [(0x40d9c4,0x40d9ee),(0x40da45,0x40db83)]
 conditions=[];stock=[]
 for site in locations(source):
  raw=(source/site['script']).read_bytes();s=parse(source/site['script'])
  event=next(e for e in s['sections'][1]['events'] if e['offset']==site['event'])
  for slot in [0,15,34,35,114]:
   for hp,life in [(0,2),(50,2),(50,3)]:
    for claimed in [0,1]:
     for offset in [0,1]:
      v.write(LIVE,b'\0'*(115*36));v.write(0x818c80,b'\xff'*1600);v.write(READER+0x38,b'\0'*4096)
      v.put32(READER+0x38+site['flag']*4,claimed)
      p=LIVE+slot*36;v.put32(p,0);v.write(p+4,bytes([slot,0 if slot<35 else 2,site['x']+offset,site['y']]));v.write(p+12,bytes([life]));v.put32(p+16,hp)
      v.write(0x818c80+site['y']*20+site['x']+offset,bytes([slot]))
      values=[]
      for i in event['conditions']:
       v.write(0x822000,raw[i['offset']:i['end']]);v.put32(READER+8,0);v.put32(READER+12,0x822000)
       values.append(v.call(0x4109ef,this=READER))
      expected=slot<35 and hp>0 and life==2 and not claimed and not offset
      assert all(values)==expected,(site,slot,hp,life,claimed,offset,values)
      conditions.append(dict(site=site,slot=slot,hp=hp,lifecycle=life,claimed=claimed,offset=offset,eligible=all(values)))
 for item in [87,88,92]:
  for before in [0,1,2,8,50,98]:
   v.write(0x4b09db,b'\0'*17);v.write(0x4b09db+item-87,bytes([before]));rng=v.u32(0x48ecd4)
   result=v.call(0x40da45,[item],0x4b0770)&255;after=v.read(0x4b09db,17)
   assert after[item-87]==before+1 and sum(after)==before+1 and rng==v.u32(0x48ecd4)
   stock.append(dict(item=item,before=before,after=after[item-87],returned=result))
 return dict(executable_sha256=EDITION,conditions=conditions,stock=stock,boundaries=__doc__)
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_terrain_pickups.json'));a=p.parse_args()
 r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['conditions']),'original location conditions;',len(r['stock']),'stock increments')
