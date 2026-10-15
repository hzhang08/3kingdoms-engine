#!/usr/bin/env python3
"""Original mode-1 attack preview; formatter and combat getters execute.

Win32 raster calls are captured by panel_probe. No damage/chance calculation,
text formatting, placement, or RNG is substituted. Character name cache writes
are allowed; unit state and RNG must otherwise remain unchanged.
"""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from panel_probe import configure as panel_configure
from physical_resolution_probe import configure as physical_configure
from movement_probe import live_json
from initialization_probe import state,UNIT
from executable_probe import EDITION
from ui_assets import decode_ui,decode_attack_labels

def generate(source):
    _,v,_,_=prepare(source);events=panel_configure(v);physical_configure(v)
    v.ranges += [(0x450d21,0x45111c),(0x440e89,0x44143d)]
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    specs=[dict(actor=20,target=38,actor_position=[9,17],camera=[6,12])]
    specs += [dict(actor=20,target=38,actor_position=[9+dx,15+dy],camera=[6,12]) for dx,dy in ((0,-2),(-2,0),(2,0),(0,2),(-1,-1),(1,-1),(-1,1),(1,1))]
    specs += [dict(actor=20,target=38,actor_job=n,camera=[6,12]) for n in range(53)]
    specs += [dict(actor=20,target=38,target_job=n,camera=[6,12]) for n in range(53)]
    specs += [dict(actor=20,target=38,terrain=n,camera=[6,12]) for n in range(30)]
    specs += [dict(actor=20,target=38,target_hp=hp,target_mp=mp,camera=[6,12]) for hp in (0,1,10,45,90) for mp in (0,1,5)]
    specs += [dict(actor=20,target=38,item=n,camera=[6,12]) for n in range(104)]
    for spec in specs:
        v.write(0x400000,original);v.write(0x800000,scratch);events.clear()
        actor,target=spec['actor'],spec['target'];ap=LIVE+actor*36;bp=LIVE+target*36;au=v.u32(ap);bu=v.u32(bp)
        if 'actor_position' in spec:v.write(ap+6,bytes(spec['actor_position']))
        for field,ptr in (('actor_job',UNIT+au*72+0x2b),('target_job',UNIT+bu*72+0x2b)):
            if field in spec:v.write(ptr,bytes([spec[field]]))
        for field,offset in (('target_hp',16),('target_mp',20)):
            if field in spec:v.put32(bp+offset,spec[field])
        if 'terrain' in spec:
            x,y=v.read(bp+6,2);v.write(0x818640+y*20+x,bytes([spec['terrain']]))
        if 'item' in spec:v.write(UNIT+au*72+0x34,bytes([spec['item'],1,0]))
        camera=spec['camera'];v.put32(0x4b4254,camera[0]*48);v.put32(0x4b4258,camera[1]*48)
        before=v.read(LIVE,115*36),v.read(UNIT,512*72),v.u32(0x48ecd4);v.hits.clear()
        v.call(0x4404bd,[1,actor,255],bp)
        after=bytearray(v.read(UNIT,512*72));after[bu*72+0x37:bu*72+0x47]=before[1][bu*72+0x37:bu*72+0x47]
        assert before==(v.read(LIVE,115*36),bytes(after),v.u32(0x48ecd4))
        cases.append(dict(actor=live_json(v,actor),actor_unit=state(v,au),battle=live_json(v,target),unit=state(v,bu),name=v.read(UNIT+bu*72+8,13).split(b'\0')[0].decode('gbk'),camera=camera,terrain=list(v.read(0x818640,400)),events=list(events)))
    raw=(source/'Ekd5.exe').read_bytes();ui=decode_ui(raw);ui['attack_labels']=decode_attack_labels(raw)
    return dict(executable_sha256=EDITION,ui=ui,cases=cases,boundaries=__doc__)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_attack_panel.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['cases']),'attack preview panels')
