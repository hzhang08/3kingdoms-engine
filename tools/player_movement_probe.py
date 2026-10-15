#!/usr/bin/env python3
"""Original player movement controller, with explicit tile-picker responses.

Executes 43d538 eligibility, distance search, occupied/range validation, route,
walking, moved flag and occupancy. The Windows tile picker is a supplied input
boundary, not proof of physical mouse handling. Overlay/range/redraw/dialogue
requests are captured; walking pixels use the existing original-frame checks.
Input is the original post-introduction roster with explicit AI/flag/status
changes and, for Cao Cao cases, activation at(10,6) and camera(4,2). This is
not a continuous turn-two player-input acceptance run.
"""
import argparse,json
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EBP
from battle_events_probe import prepare,LIVE
from initialization_probe import UNIT,state
from movement_animation_probe import configure as walking_configure
from movement_probe import live_json
from executable_probe import EDITION

def configure(v):
    events=walking_configure(v)
    v.ranges += [(0x43d538,0x43dada),(0x4401a8,0x440252),
      (0x442abb,0x442b03),(0x442718,0x442744),(0x43ff54,0x43ff6e)]
    inputs=[];picks=[];modes=[];context=dict(slot=None,inputs=inputs,picks=picks,modes=modes)
    def request(kind,argc,pop):
        def hook(vm):
            args=[vm.arg(n) for n in range(argc)]
            if kind=='range':args=[args[0]&65535,args[1]&255,args[2]]
            events.append(dict(kind=kind,args=args));vm.return_from_hook(pop=pop)
        return hook
    def pick(vm):
        if not inputs:raise RuntimeError('tile input exhausted at0x45418f')
        tile=inputs.pop(0);vm.write(0x82e000,bytes(tile))
        picks.append(dict(tile=tile,distance=list(vm.read(0x8192c0,400)),before=live_json(vm,context['slot'])))
        events.append(dict(kind='pick',tile=tile));vm.return_from_hook(0x82e000)
    def message(vm):
        events.append(dict(kind='message',style=vm.arg(0),text=vm.read(vm.arg(1),512).split(b'\0')[0].decode('gbk')));vm.return_from_hook(0)
    def unit_redraw(vm):
        events.append(dict(kind='unit_redraw',slot=(vm.this()-LIVE)//36));vm.return_from_hook()
    v.hooks.update({0x45418f:pick,0x4350f0:request('movement_overlay',1,4),
      0x451c03:request('blue_movement',1,4),0x4539dc:request('range',3,12),
      0x453b13:request('redraw',0,0),0x47467a:request('sound',2,8),0x42f699:message,
      0x43fcc9:unit_redraw})
    def observer(uc,address,size,user):
        if address==0x43d801:modes.append(v.read(uc.reg_read(UC_X86_REG_EBP)-4,1)[0])
    v.uc.hook_add(UC_HOOK_CODE,observer)
    v.put32(0x4b4254,48);v.put32(0x4b4258,576)
    v.put32(0x4b4264,624);v.put32(0x4b4268,384);v.write(0x4b0770+8,b'\x01')
    v.write(0x4b4230,b'\0')
    return events,context

def generate(source):
    _,v,_,_=prepare(source);events,context=configure(v)
    inputs=context['inputs'];picks=context['picks'];modes=context['modes']
    memory=[(a,v.read(a,b-a+1)) for a,b,_ in v.uc.mem_regions()]
    specs=[]
    for s in (18,20,35):
      for ai in (1,7):
       for flags in (0,2,4,128):
        for status in (0,8):
         specs.append(dict(slot=s,ai=ai,flags=flags,status=status,tiles=[[255,255]]))
         specs.append(dict(slot=s,ai=ai,flags=flags,status=status,tiles=[list(v.read(LIVE+s*36+6,2)),[255,255]]))
    for tile in ([11,17],[10,17],[11,16],[9,11],[0,0],[19,19]):
        specs.append(dict(slot=20,ai=7,flags=0,status=0,tiles=[tile,[255,255]]))
    for y in range(20):
      for x in range(20):
        specs.append(dict(slot=0,ai=7,flags=0,status=0,activate=[10,6],camera=[4,2],tiles=[[x,y],[255,255]]))
    cases=[]
    for spec in specs:
        for a,raw in memory:v.write(a,raw)
        slot=spec['slot'];context['slot']=slot;actor=LIVE+slot*36
        if 'activate' in spec:
            assert v.read(actor+12,1)[0]!=2
            x,y=spec['activate'];assert v.read(0x818c80+y*20+x,1)==b'\xff'
            v.write(actor+6,bytes([x,y]));v.write(actor+12,b'\x02');v.write(0x818c80+y*20+x,bytes([slot]))
        v.write(actor+13,bytes([spec['flags'],spec['ai']]))
        v.write(actor+30,bytes([spec['status']]))
        camera=spec.get('camera',[1,12]);v.put32(0x4b4254,camera[0]*48);v.put32(0x4b4258,camera[1]*48)
        inputs[:]=[list(t) for t in spec['tiles']];events.clear();picks.clear();modes.clear();v.hits.clear()
        before=live_json(v,slot);persistent=v.read(UNIT,512*72);rng=v.u32(0x48ecd4)
        roster=[live_json(v,n) for n in range(115) if v.read(LIVE+n*36+12,1)[0]==2];occupancy=list(v.read(0x818c80,400))
        result=v.call(0x43d538,this=actor,budget=4000000)
        assert persistent==v.read(UNIT,512*72) and rng==v.u32(0x48ecd4)
        allowed={0x45418f,0x4350f0,0x451c03,0x4539dc,0x453b13,0x47467a,0x42f699,
          0x454b7e,0x45137c,0x403340,0x43eb1f,0x43f38b,0x47fcfd,0x47e251,
          0x41e5db,0x41e5f6,0x4036f9,0x451913,0x42c4e0,0x43fcc9}
        unexpected=v.hits.intersection(v.hooks)-allowed
        assert not unexpected,[hex(a) for a in sorted(unexpected)]
        cases.append(dict(input=spec,battle=before,unit=state(v,before['id']),roster=roster,
          width=20,height=20,terrain=list(v.read(0x818640,400)),camera=camera,speed=1,
          occupancy=occupancy,picks=list(picks),modes=list(modes),result=result,
          after=live_json(v,slot),after_occupancy=list(v.read(0x818c80,400)),
          after_camera=[v.u32(0x4b4254)//48,v.u32(0x4b4258)//48],events=list(events),
          hooks=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
    return dict(executable_sha256=EDITION,cases=cases,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'))
    p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_player_movement.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n')
    print(len(r['cases']),'original player movement controller cases')
