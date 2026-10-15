#!/usr/bin/env python3
"""Verify dictionary observations against original code and retained fixtures.

Creates diagnostic captures, NOT resumable snapshots. Only isolated original
CPU routines execute. Original game files, legacy saves and native game saves
are read-only. Existing probes define all host/archive boundaries.
"""
import argparse
import hashlib
import json
from pathlib import Path
from battle_events_probe import prepare, LIVE
from executable_probe import EDITION
from game_state import default_dictionary, format_changes
from strike_probe import configure as configure_strike, CONTROL

ROOT=Path(__file__).resolve().parents[1]

def generate(source):
    _,v,_,_=prepare(source)
    dictionary=default_dictionary()
    initial=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000)
    def reset():v.write(0x400000,initial);v.write(0x800000,scratch)
    def capture(selection,label,labels=None):
        before=hashlib.sha256(v.read(0x400000,0x200000)+v.read(0x800000,0x40000)).hexdigest()
        result=v.capture_state(selection,label=label,labels=labels)
        assert before==hashlib.sha256(v.read(0x400000,0x200000)+v.read(0x800000,0x40000)).hexdigest()
        return result
    artifacts={}
    # The original tutorial counter counts completion entries, not input bits.
    v.ranges += [(0x434f84,0x4350ba)]
    v.hooks[0x41a284]=lambda vm:vm.return_from_hook(int(vm.arg(0) in (0,2)))
    v.hits.clear();v.call(0x434f84)
    assert v.hits.intersection(v.hooks)=={0x41a284}
    tutorial=capture({'tutorial':[0]},'Original tutorial setup with completion bytes[1,0,1]')
    t=dictionary.read_values(v.read,'tutorial')
    assert t['completed_routes']==2 and t['all_treasures']==0
    assert t['guide_pointer']==LIVE+25*36
    artifacts['tutorial.json']=tutorial
    # One original cancellation: coordinates and two occupancy cells change.
    reset()
    case=json.loads((ROOT/'tests/fixtures/original_tutorial_actions.json').read_text())['cancel_cases'][0]
    v.write(LIVE,bytes(case['before']));v.write(0x818c80,b'\xff'*1600)
    for slot in range(115):
        b=v.read(LIVE+slot*36,36)
        if b[12]==2:v.write(0x818c80+b[7]*20+b[6],bytes([slot]))
    v.write(0x82e000,bytes([10,6]));v.ranges += [(0x4350c0,0x4350e8),(0x442b03,0x442bf6)]
    selection={'live_units':[0],'occupancy':[0],'rng':[0]}
    before=capture(selection,'Before original movement cancellation',{'live_units[0]':'曹操'})
    persistent=v.read(0x4a1b68,512*72);v.call(0x4350c0,[0x82e000,0,0],LIVE)
    after=capture(selection,'After original movement cancellation',{'live_units[0]':'曹操'})
    assert v.read(LIVE,36)==bytes(case['after']) and persistent==v.read(0x4a1b68,512*72)
    changes=dictionary.diff(before,after)
    assert {c['path'] for c in changes}=={'live_units[0].y','occupancy[0].bytes[10]','occupancy[0].bytes[130]'}
    artifacts.update({'cancel-before.json':before,'cancel-after.json':after,'cancel-diff.json':changes,'cancel-diff.txt':format_changes(changes)})
    # Execute the first retained unhooked strike-plan case, compare all outputs.
    reset();configure_strike(v);v.write(LIVE+20*36+6,bytes([9,17]))
    uid=v.u32(LIVE+20*36);v.write(CONTROL,b'\0'*0x614)
    v.write(CONTROL,bytes([20,38]));v.put32(CONTROL+4,uid)
    v.put32(CONTROL+8,0x4a1b68+uid*72);v.put32(CONTROL+12,LIVE+20*36)
    v.write(CONTROL+16,bytes([38,255]));v.put32(CONTROL+0x600,0)
    v.put32(0x492e04,0);v.put32(0x48ecd4,1)
    selection={'attack':[0],'counter':[0],'live_units':[20,38],'rng':[0]}
    before=capture(selection,'Before original strike planning');v.hits.clear()
    v.call(0x405e03,this=CONTROL)
    assert not v.hits.intersection(v.hooks)
    after=capture(selection,'After original strike planning')
    plan=dictionary.read_values(v.read,'attack')
    expected=json.loads((ROOT/'tests/fixtures/original_strikes.json').read_text())['cases'][0]
    for name in ('hp_damage','mp_damage','armor_xp'):assert plan[name][:1]==expected[name],name
    for name in ('xp','weapon_xp','critical'):assert plan[name]==expected[name],name
    assert v.u32(0x48ecd4)==expected['strike_rng']
    artifacts.update({'strike-before.json':before,'strike-after.json':after,'strike-diff.json':dictionary.diff(before,after)})
    # Original RNG call: dictionary observes the exact wraparound state.
    reset();v.ranges.append((0x47fae7,0x47fb05));v.put32(0x48ecd4,0xffffffff)
    before=capture({'rng':[0]},'Before original RNG call');v.call(0x47fae7)
    after=capture({'rng':[0]},'After original RNG call')
    assert dictionary.read_values(v.read,'rng')['state']==(0xffffffff*1103515245+12345)&0xffffffff
    artifacts.update({'rng-before.json':before,'rng-after.json':after})
    artifacts['verification.json']=dict(executable_sha256=EDITION,dictionary_sha256=dictionary.hash,
        original_calls=['0x434f84','0x4350c0','0x405e03','0x47fae7'],
        verified=['tutorial completion count and guide pointer','movement cancellation field names and occupancy changes',
                  'strike-plan arrays, XP, critical flag and RNG match retained first case','RNG wraparound',
                  'all diagnostic captures leave emulator memory unchanged'],
        limits='Isolated original-code probes with inherited documented host boundaries. No new player interaction/full battle or resumable CPU checkpoint.')
    return artifacts

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'))
    p.add_argument('--output',type=Path,default=ROOT/'docs/evidence/game-state');a=p.parse_args()
    r=generate(a.source);a.output.mkdir(parents=True,exist_ok=True)
    # These are generated diagnostic artifacts, separate from all save folders.
    for name,value in r.items():
        (a.output/name).write_text(value if isinstance(value,str) else json.dumps(value,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps(r['verification.json'],ensure_ascii=False,indent=2))
