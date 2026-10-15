#!/usr/bin/env python3
"""Original tutorial setup and entry/skip branch, with explicit collection input.

41a284 reads SVCMN.E5S in the original; the probe supplies indexed bytes without
opening or modifying that save. Original setup/getters and dispatcher execute.
Entry dialogue, choices and facing are captured requests; unexplored teaching
and reward routines STOP execution, not no-ops. Their effects are not verified.
"""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from executable_probe import EDITION

class Boundary(Exception):pass

def generate(source):
    _,v,_,_=prepare(source)
    v.ranges += [(0x433359,0x43353c),(0x434f84,0x4350ba),(0x4065a0,0x4065c0)]
    flags=[0]*108;reads=[];events=[];choice=[0];pending=[None]
    def collection(vm):
        index=vm.arg(0)&255;assert index<len(flags)
        reads.append(index);vm.return_from_hook(flags[index])
    def text(vm):
        pointer=vm.arg(0);events.append(dict(kind='dialogue',address=hex(pointer),unit=vm.arg(1)&65535,text=vm.read(pointer,2048).split(b'\0')[0].decode('gbk')));vm.return_from_hook(pop=8)
    def choose(vm):
        pointer=vm.arg(0);events.append(dict(kind='choice',address=hex(pointer),text=vm.read(pointer,512).split(b'\0')[0].decode('gbk'),default=vm.arg(1),result=choice[0]));vm.return_from_hook(choice[0])
    def face(vm):events.append(dict(kind='facing_request',args=[vm.arg(n) for n in range(6)]));vm.return_from_hook(pop=24)
    def boundary(address):
        def stop(vm):pending[0]=hex(address);raise Boundary()
        return stop
    v.hooks.update({0x41a284:collection,0x45965d:text,0x42de12:choose,0x457428:face})
    stops={0x43353c,0x4347ff,0x434d33}
    for address in stops:v.hooks[address]=boundary(address)
    collectible=[n for n in range(104) if v.call(0x409600,this=0x4a1140+n*25)]
    def setup():
        return dict(completions=v.read(0x4b2c10,1)[0],all_treasures=v.u32(0x4b2c08),
            slots=[(v.u32(p)-LIVE)//36 for p in (0x4b2c14,0x4b2c0c,0x4b2c04)],guide_position=list(v.read(0x4b2c18,2)))
    cases=[];branches=[]
    specs=[dict(completion_mask=mask,all_items=all_items) for mask in range(8) for all_items in (False,True)]
    specs += [dict(completion_mask=0,all_items=True,missing=item) for item in collectible]
    specs += [dict(completion_mask=5,all_items=True,nonzero=n) for n in (2,255)]
    for spec in specs:
        flags[:]=[spec.get('nonzero',1) if spec['completion_mask']&(1<<n) else 0 for n in range(3)]+[int(spec['all_items'])]*105
        if 'missing' in spec:flags[spec['missing']+3]=0
        before=v.read(LIVE,115*36),v.read(0x4a1b68,512*72),v.u32(0x48ecd4)
        reads.clear();v.hits.clear();v.call(0x434f84)
        assert before==(v.read(LIVE,115*36),v.read(0x4a1b68,512*72),v.u32(0x48ecd4))
        assert v.hits.intersection(v.hooks)=={0x41a284}
        cases.append(dict(input=spec,collection=flags.copy(),reads=list(reads),after=setup()))
        if 'missing' in spec or 'nonzero' in spec:continue
        for answer in (0,1):
            events.clear();reads.clear();pending[0]=None;choice[0]=answer;v.hits.clear()
            try:v.call(0x433359)
            except Boundary:pass
            assert not(v.hits.intersection(v.hooks)-({0x41a284,0x45965d,0x42de12,0x457428}|stops))
            branches.append(dict(input=spec,choice=answer,setup=setup(),events=list(events),boundary=pending[0]))
    return dict(executable_sha256=EDITION,collectible_items=collectible,setup=cases,entry_branches=branches,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_tutorial_entry.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['setup']),'setups;',len(r['entry_branches']),'entry/skip branches')
