#!/usr/bin/env python3
"""Original NPC damage panel and five HP/MP interpolation steps.

Runs 0x450984 with original GBK formatter, panel placement and bar routines.
Raster/background capture, cursor/window removal and host waits are captured.
Party XP panels are deliberately excluded until separately recovered.
"""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from panel_probe import configure as panel_configure
from initialization_probe import UNIT,state
from movement_probe import live_json
from executable_probe import EDITION
from ui_assets import decode_ui

HOOKS={0x41e5db,0x41e5f6,0x47d239,0x47d1f1,0x453c17,0x40faa0,0x40f548,0x451913,0x41e6a3,0x47d38d,0x47dbc3,0x47e251,0x47e09b,0x42c4e0,0x403340,0x4544d3,0x45461c,0x453e32}

def configure(v,events=None,preserve_view=False):
    events=panel_configure(v,events,preserve_view)
    v.ranges += [(0x450984,0x450ed5),(0x472840,0x472851)]
    if not preserve_view:
        v.uc.mem_map(0xd00000,0x10000);v.put32(0x4ae4c8+4,0x10000);v.put32(0x4ae4c8+12,0xd00000);v.write(0x4ae4c8+16,b'\x01')
    def skip(vm):vm.return_from_hook()
    def wait(vm):events.append(dict(kind='wait',ticks=vm.arg(0)));vm.return_from_hook(pop=4)
    def cursor(vm):events.append(dict(kind='cursor',tile=list(vm.read(vm.arg(0),2))));vm.return_from_hook(pop=4)
    def clear(kind):
        def hook(vm):events.append(dict(kind=kind));vm.return_from_hook()
        return hook
    v.hooks.update({0x47e251:skip,0x47e09b:skip,0x42c4e0:skip,0x403340:wait,
      0x4544d3:cursor,0x45461c:clear('clear_cursor'),0x453e32:clear('clear_panel')})
    return events

def generate(source):
    _,v,_,_=prepare(source);events=configure(v)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    specs=[dict(slot=slot,other=other,hp=hp,mp=mp,wait=wait_flag) for slot,other in ((38,20),(20,38),(38,255),(20,255))
      for hp,mp in ((0,0),(1,0),(45,0),(90,0),(0,3),(12,2),(-30,0),(0,-5)) for wait_flag in (False,True)]
    specs += [dict(slot=s,other=255,hp=11,mp=0,wait=True,position=[x+3,y+12]) for s in (20,38) for y in range(8) for x in range(13)]
    for spec in specs:
        slot,other,hp,mp,wait_flag=[spec[k] for k in ('slot','other','hp','mp','wait')]
        v.write(0x400000,original);v.write(0x800000,scratch);events.clear()
        v.write(LIVE+20*36+6,bytes([9,17]));v.put32(0x4b4254,144);v.put32(0x4b4258,576)
        if 'position' in spec:v.write(LIVE+slot*36+6,bytes(spec['position']))
        b=live_json(v,slot);uid=b['id'];before=v.read(LIVE,115*36),v.u32(0x48ecd4);v.hits.clear()
        v.call(0x450984,[slot,other,hp,mp,0,0,0,wait_flag])
        assert before==(v.read(LIVE,115*36),v.u32(0x48ecd4))
        assert not (v.hits.intersection(v.hooks)-HOOKS)
        cases.append(dict(battle=b,other=live_json(v,other) if other!=255 else None,unit=state(v,uid),name=v.read(UNIT+uid*72+8,13).split(b'\0')[0].decode('gbk'),hp=hp,mp=mp,wait=wait_flag,camera=[3,12],events=list(events)))
    return dict(executable_sha256=EDITION,ui=decode_ui((source/'Ekd5.exe').read_bytes()),cases=cases,boundaries=__doc__)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_damage_panel.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['cases']),'NPC damage panel sequences')
