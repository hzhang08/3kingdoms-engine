#!/usr/bin/env python3
"""Execute the original phase caller around a supplied completed first attack.

The first AI call is an explicit input boundary: its separately compared movement
and attack result is installed. Caller filtering, action-event gate, death scan,
redraw request, event selector and next actor selection then execute originally.
The probe stops before the second AI action or a required death handler.
"""
import argparse,json,hashlib
from pathlib import Path
from unicorn import UC_HOOK_CODE
from battle_events_probe import prepare,LIVE,CTRL,READER
from executable_probe import EDITION
from initialization_probe import UNIT

class Boundary(Exception):pass

def generate(source):
    fixture=Path('tests/fixtures/original_attack_controller.json')
    attack=json.loads(fixture.read_text())['cases'][0]
    _,v,_,events=prepare(source)
    v.put32(READER+0x38+14*4,1);v.put32(READER+0x24,len(events))
    v.write(LIVE+18*36+15,b'\0')
    v.ranges += [(0x44e851,0x44ebfd),(0x44c601,0x44c658),(0x44cc03,0x44cc85),
      (0x4180c1,0x418174),(0x410990,0x4109a1),(0x44f7b0,0x44f7c1),
      (0x44da7a,0x44df61),(0x44f980,0x44f998),(0x425ab0,0x425ac1),
      (0x435182,0x4351da),(0x435991,0x4359f5),(0x43a1f4,0x43a28a),
      (0x43ff54,0x4400a2),(0x406906,0x406947),(0x425ad0,0x425ae1),(0x425af0,0x425b15),
      (0x45b5fe,0x45b6d0),(0x45b910,0x45b92d),(0x45f710,0x45f71e),(0x44f9a0,0x44f9b1),
      (0x43fcc9,0x43fd34),(0x451a93,0x451ab5)]
    trace=[];calls=[];death=[]
    def pump(vm):vm.return_from_hook()
    def cursor(vm):trace.append(dict(kind='cursor',value=vm.arg(0)));vm.return_from_hook()
    def send_message(vm):
        trace.append(dict(kind='message_pump',message=vm.arg(1)));vm.return_from_hook(pop=16)
    def redraw(vm):trace.append(dict(kind='redraw',args=[vm.arg(n) for n in range(4)]));vm.return_from_hook(pop=16)
    def died(vm):death.append((vm.this()-LIVE)//36);raise Boundary()
    def action(vm):
        slot=(vm.this()-LIVE)//36;calls.append(slot);trace.append(dict(kind='ai_action_boundary',slot=slot))
        if len(calls)>1:raise Boundary()
        assert slot==attack['spec']['actor']==20
        # Explicit completed first-action input, not an execution of 0x43ad1c.
        vm.write(0x818c80+17*20+12,b'\xff');vm.write(0x818c80+17*20+9,b'\x14')
        vm.write(LIVE+20*36+6,bytes(attack['spec']['position']))
        vm.write(LIVE+20*36+13,b'\x06');vm.write(LIVE+20*36+15,b'\0')
        vm.put32(LIVE+38*36+16,45);vm.put32(0x48ecd4,attack['rng'])
        vm.put32(0x4b4254,attack['camera'][0]);vm.put32(0x4b4258,attack['camera'][1])
        vm.write(CTRL+5,b'\x06');vm.return_from_hook()
    v.put32(0x4862f4,0x820e00)
    v.hooks.pop(0x43fcc9,None) # Execute the redraw wrapper; capture only its raster call.
    v.hooks.update({0x47eac4:pump,0x42b81b:cursor,0x43ad1c:action,0x820e00:send_message,0x45218f:redraw,0x441744:died})
    names={0x44da7a:'priority',0x44c601:'event_batch',0x4100a1:'event_scan',0x45b5fe:'action_event_gate',0x44cc03:'death_scan',0x43fcc9:'unit_redraw'}
    def observe(uc,address,size,user):
        if address in names:trace.append(dict(kind=names[address],phase=v.read(CTRL+0x10,1)[0]))
    v.uc.hook_add(UC_HOOK_CODE,observe)
    before=v.read(UNIT,512*72)
    v.hits.clear()
    try:v.call(0x44e851,this=CTRL,budget=6000000)
    except Boundary:pass
    assert calls==[20,18] and not death
    assert v.hits.intersection(v.hooks)=={0x47eac4,0x42b81b,0x43ad1c,0x820e00,0x45218f}
    assert before==v.read(UNIT,512*72) and v.u32(0x48ecd4)==attack['rng']
    return dict(executable_sha256=EDITION,attack_fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
      trace=trace,actions=calls,phase=v.read(CTRL+0x10,1)[0],rng=v.u32(0x48ecd4),
      next_unit=v.u32(LIVE+18*36),priority_recomputed=sum(e['kind']=='priority' for e in trace),
      boundaries=__doc__)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_post_action.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r,indent=2))
