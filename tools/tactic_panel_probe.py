#!/usr/bin/env python3
"""Original mode-2 preview for opening tactics 23, 25 and 33.

Executes original formatters, eligibility, estimate, hit chance and placement.
Only window/font/raster calls are captured. Panels must not consume RNG or
change unit records (apart from the original display-name cache).
"""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from panel_probe import configure as panel_configure
from tactic_scoring_probe import configure as tactic_configure
from movement_probe import live_json
from initialization_probe import state,UNIT
from executable_probe import EDITION
from ui_assets import decode_ui,decode_attack_labels,decode_tactic_labels

def generate(source):
    _,v,_,_=prepare(source);events=panel_configure(v);tactic_configure(v)
    v.ranges += [(0x450ed5,0x45111c),(0x440e89,0x4410e6),(0x441439,0x44143d)]
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    specs=[{}]+[dict(job=n) for n in range(53)]
    specs += [dict(hp=hp,status=status) for hp in (0,1,10,45,90) for status in (0,8,16,24)]
    specs += [dict(grade=n) for n in range(7)]+[dict(weapon=36,armor=47)]
    for tactic in (23,25,33):
     for spec in specs:
        v.write(0x400000,original);v.write(0x800000,scratch);events.clear()
        actor,target=35,20;ap=LIVE+actor*36;bp=LIVE+target*36;au=v.u32(ap);bu=v.u32(bp)
        v.write(ap+6,bytes([9,13]));v.write(bp+6,bytes([9,17]));camera=[3,10]
        if 'job' in spec:v.write(UNIT+bu*72+0x2b,bytes([spec['job']]))
        if 'hp' in spec:v.put32(bp+16,spec['hp']);v.write(bp+30,bytes([spec['status']]))
        if 'grade' in spec:v.write(bp+24,bytes([spec['grade']]*6))
        if 'weapon' in spec:v.write(UNIT+au*72+0x2e,bytes([spec['weapon'],9,0]));v.write(UNIT+bu*72+0x31,bytes([spec['armor'],9,0]))
        v.put32(0x4b4254,camera[0]*48);v.put32(0x4b4258,camera[1]*48)
        before=v.read(LIVE,115*36),v.read(UNIT,512*72),v.u32(0x48ecd4);v.hits.clear()
        v.call(0x4404bd,[2,actor,tactic],bp)
        after=bytearray(v.read(UNIT,512*72));after[bu*72+0x37:bu*72+0x47]=before[1][bu*72+0x37:bu*72+0x47]
        assert before==(v.read(LIVE,115*36),bytes(after),v.u32(0x48ecd4))
        allowed={0x41e5db,0x41e5f6,0x47d239,0x47d1f1,0x453c17,0x40faa0,0x40f548,0x451913,0x41e6a3,0x47d38d,0x47dbc3}
        assert not(v.hits.intersection(v.hooks)-allowed)
        cases.append(dict(tactic=tactic,actor=live_json(v,actor),actor_unit=state(v,au),battle=live_json(v,target),unit=state(v,bu),name=v.read(UNIT+bu*72+8,13).split(b'\0')[0].decode('gbk'),camera=camera,events=list(events)))
    raw=(source/'Ekd5.exe').read_bytes();ui=decode_ui(raw);ui['attack_labels']=decode_attack_labels(raw);ui['tactic_labels']=decode_tactic_labels(raw)
    return dict(executable_sha256=EDITION,ui=ui,cases=cases,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_tactic_panel.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['cases']),'original tactic preview panels')
