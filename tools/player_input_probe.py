#!/usr/bin/env python3
"""Original player loop with explicit host input and window boundaries.

The unchanged ASave-disabled continuation, hover/action dispatch, movement,
menu callbacks, wait/cancel, event/death scans and remaining-unit scan execute.
World tile, button polls, timer/scroll state, focus, covered-panel state and
modal responses are supplied host inputs. Rendering/audio/windows are captured.
The actor is explicitly activated on the post-introduction board; this is not
a continuous new-game or physical mouse acceptance run.
"""
import argparse,json
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EBP,UC_X86_REG_EIP
from battle_events_probe import prepare,LIVE,CTRL,READER
from initialization_probe import UNIT,state
from movement_probe import live_json
from player_phase_probe import configure_patch,PATCH_SHA
from player_action_probe import configure_action,Modal
from executable_probe import EDITION

class Exhausted(Exception):pass
class EventBody(Exception):pass

def generate(source):
    _,v,_,script_events=prepare(source)
    events,control,windows,range_before,answer=configure_action(v)
    configure_patch(v,source)
    v.ranges += [(0x44e1d3,0x44e851),(0x453e97,0x453ea8),
      (0x44c601,0x44c658),(0x44cc03,0x44cc85),(0x4180c1,0x418174),
      (0x410990,0x4109a1),(0x44f7b0,0x44f7c1),(0x45b5fe,0x45b6d0),
      (0x45b910,0x45b92d),(0x44f9a0,0x44f9b1),(0x406730,0x40675a),
      (0x44049e,0x4404bd),(0x40be20,0x40be36),(0x42c4cb,0x42c4e0)]
    current={};trace=[];frames=[];inputs=[];polls=[];menus=[];frame=[-1]
    def snap():
        return dict(actor=live_json(v,0),camera=[v.u32(0x4b4254),v.u32(0x4b4258)],
          rng=v.u32(0x48ecd4),tick=v.read(CTRL+5,1)[0],
          outcome=v.read(CTRL+1,1)[0],retain=v.u32(CTRL+0x500),covered=v.u32(0x4b4250+0x50))
    def record(kind,argc=0,pop=0):
        def call(vm):
            events.append(dict(kind=kind,args=[vm.arg(n) for n in range(argc)]));vm.return_from_hook(pop=pop)
        return call
    def noop(vm):vm.return_from_hook()
    def tile(vm):vm.write(0x82e100,bytes(current['tile']));vm.return_from_hook(0x82e100)
    def scroll(vm):vm.return_from_hook(current.get('scroll',0))
    def timer(vm):vm.return_from_hook(current.get('timer',0))
    def focus(vm):vm.return_from_hook(current.get('focus',1))
    def poll(vm):
        if not polls:raise RuntimeError('explicit input poll exhausted')
        vm.return_from_hook(polls.pop(0))
    def message(vm):
        text=vm.read(vm.arg(1),512).split(b'\0')[0].decode('gbk')
        response=current.get('confirm',0)
        events.append(dict(kind='message',style=vm.arg(0),text=text,response=response));vm.return_from_hook(response)
    def camera(vm):
        events.append(dict(kind='camera_request',args=[vm.arg(0)&255,vm.arg(1)&255]));vm.return_from_hook(pop=8)
    def panel(vm):
        events.append(dict(kind='panel',slot=(vm.this()-LIVE)//36,args=[vm.arg(n) for n in range(3)]));vm.return_from_hook(pop=12)
    def cursor(vm):
        events.append(dict(kind='cursor',tile=list(vm.read(vm.arg(0),2)),style=vm.arg(1)));vm.return_from_hook(pop=8)
    def clear(vm):events.append(dict(kind='clear_panel'));vm.return_from_hook()
    def event(vm):
        trace.append(dict(kind='event_body',offset=vm.u32(vm.this()+12)-vm.u32(vm.this()+32)))
        raise EventBody()
    v.put32(0x486260,0x83e130);v.put32(0x4862cc,0x83e140)
    v.put32(0x4b6a68,1);v.write(0x4b4234,b'\0')
    v.hooks.update({0x47eac4:noop,0x453ea8:noop,0x454709:scroll,0x42f7f0:timer,
      0x454003:tile,0x42c56b:poll,0x83e130:focus,0x83e140:record('focus_window',1,4),
      0x449de6:record('input_enabled',1),0x45503f:camera,
      0x4759bc:record('timer_reset',1),0x42b81b:record('cursor_style',1),
      0x4404bd:panel,0x453e32:clear,0x45576d:cursor,
      0x45461c:record('clear_cursor'),0x455719:record('clear_target'),
      0x479795:record('stats_window',1),0x41084e:event,0x42f699:message})
    v.put32(READER+0x24,len(script_events))
    for n in (11,12,13,14):v.put32(READER+0x38+n*4,1)
    def observe(uc,address,size,user):
        if address==0x44e1f4:
            bp=uc.reg_read(UC_X86_REG_EBP)
            if frame[0]>=0:frames[-1]['after']=dict(snap(),hover=v.read(bp-0x14,1)[0])
            frame[0]+=1
            if frame[0]>=len(inputs):raise Exhausted()
            current.clear();current.update(inputs[frame[0]])
            polls[:]=current.get('polls',[0,0]);control['inputs'][:]=current.get('picks',[])
            v.write(CTRL+1,bytes([current.get('phase_command',0)]))
            v.put32(CTRL+0x500,current.get('retain',0));v.put32(0x4b4250+0x50,current.get('covered',0))
            v.write(0x4b4234,bytes([current.get('disabled',0)]))
            windows.clear();range_before.clear()
            frames.append(dict(input=dict(current),before=dict(snap(),hover=v.read(bp-0x14,1)[0]),event_index=len(events),trace_index=len(trace)))
        if address==0x43dde3:control['slot']=(v.this()-LIVE)//36;trace.append(dict(kind='action',slot=control['slot']))
        if address==0x44e3a9:
            frames[-1]['hover_ready']=dict(snap(),hover=v.read(uc.reg_read(UC_X86_REG_EBP)-0x14,1)[0],event_end=len(events))
        if address==0x44e6aa:
            trace.append(dict(kind='remaining_manual_units',count=v.read(uc.reg_read(UC_X86_REG_EBP)-0x30,1)[0]))
        if address in (0x45b5fe,0x44cc03,0x44c601,0x4100a1):
            row=dict(kind={0x45b5fe:'action_events',0x44cc03:'death_scan',0x44c601:'event_batch',0x4100a1:'event_scan'}[address])
            if address==0x45b5fe:row['slot']=v.arg(0)&255
            trace.append(row)
    v.uc.hook_add(UC_HOOK_CODE,observe)
    memory=[(a,v.read(a,b-a+1)) for a,b,_ in v.uc.mem_regions()]
    empty=dict(tile=[0,0]);hover=dict(tile=[10,6]);outside=dict(tile=[255,255])
    specs=[dict(name='hover-leave-return',frames=[hover,hover,empty,hover]),
      dict(name='retained-hover-outside-cancel',frames=[hover,dict(outside,retain=1,focus=1,polls=[1,0],picks=[[255,255]])]),
      dict(name='lost-focus-clears-hover',frames=[hover,dict(outside,retain=1,focus=2,polls=[1,0])]),
      dict(name='empty-click',frames=[dict(empty,polls=[1,0])]),
      dict(name='right-unit',frames=[dict(hover,polls=[2,2])]),
      dict(name='right-empty-cancel',frames=[dict(empty,polls=[2,2],confirm=0),empty]),
      dict(name='right-empty-end',frames=[dict(empty,polls=[2,2],confirm=1)]),
      dict(name='menu-end-cancel',frames=[dict(hover,phase_command=6,confirm=0),hover]),
      dict(name='menu-end-confirm',frames=[dict(hover,phase_command=6,confirm=1)]),
      dict(name='exit-command',frames=[dict(hover,phase_command=5)]),
      dict(name='covered-switch',frames=[hover,dict(tile=[11,17],covered=1),dict(empty,covered=1)]),
      dict(name='disabled-click',frames=[dict(hover,disabled=1,polls=[1,0])])]
    for dest in ([10,6],[9,6],[11,7]):
      for command in (255,4):
       for confirm in (0,1):
        specs.append(dict(name=f'action-{dest}-{command}-{confirm}',frames=[dict(hover,polls=[1,0],picks=[dest],command=command,confirm=confirm),hover]))
    for scroll in (0,1):
      for timer_value in (0,1):specs.append(dict(name=f'timer-{scroll}-{timer_value}',frames=[dict(hover,scroll=scroll,timer=timer_value)]))
    for retained in (0,1):
     for focused in (1,2):
      for covered in (0,1):
       for dest in ([0,0],[255,255]):
        specs.append(dict(name=f'retention-{retained}-{focused}-{covered}-{dest}',frames=[hover,
          dict(tile=dest,retain=retained,focus=focused,covered=covered)]))
    for slot in (18,35):
     for lifecycle in (1,2,3):
      for ai in (1,7):
       for flags in (0,2,4,128):
        for status in (0,8):
         specs.append(dict(name=f'remaining-{slot}-{lifecycle}-{ai}-{flags}-{status}',
           candidate=dict(slot=slot,lifecycle=lifecycle,ai=ai,flags=flags,status=status),
           frames=[dict(hover,polls=[1,0],picks=[[10,6]],command=4,confirm=0)]))
    cases=[]
    for spec in specs:
        for a,raw in memory:v.write(a,raw)
        v.write(LIVE+6,bytes([10,6]));v.write(LIVE+12,bytes([2,0,7,2]));v.write(0x818c80+130,b'\0')
        v.call(0x43fd34,[0],LIVE)
        if 'candidate' in spec:
            c=spec['candidate'];p=LIVE+c['slot']*36
            v.write(p+12,bytes([c['lifecycle'],c['flags'],c['ai']]));v.write(p+30,bytes([c['status']]))
            x,y=v.read(p+6,2);v.write(0x818c80+y*20+x,bytes([c['slot'] if c['lifecycle']==2 else 255]))
        events.clear();trace.clear();frames.clear();menus.clear();frame[0]=-1;current.clear();inputs[:]=spec['frames']
        v.hits.clear();persistent=v.read(UNIT,512*72);rng=v.u32(0x48ecd4)
        before=snap();roster=[live_json(v,n) for n in range(115) if v.read(LIVE+n*36+12,1)[0] in (1,2,3)]
        reason='returned';address=0x44e0d9
        try:
            while True:
                try:
                    if address==0x44e0d9:v.call(address,this=CTRL,budget=4000000)
                    else:v.uc.emu_start(address,0x83f000,count=4000000)
                    assert v.uc.reg_read(UC_X86_REG_EIP)==0x83f000
                    break
                except Modal:
                    menus.append(answer(current['command']));address=v.uc.reg_read(UC_X86_REG_EIP)
        except Exhausted:reason='input_boundary'
        except EventBody:reason='event_body'
        except Exception:
            print('failed',spec['name'],'events',events[-6:],'trace',trace[-6:],flush=True);raise
        assert v.read(UNIT,512*72)==persistent and v.u32(0x48ecd4)==rng
        assert reason!='event_body','unexpected script body needs a separate verified continuation'
        allowed={0x403340,0x4036f9,0x41e5db,0x41e5f6,0x42b81b,0x42c4e0,
          0x42c56b,0x42f699,0x42f7f0,0x4350f0,0x43f38b,0x43fcc9,0x4404bd,
          0x449de6,0x44a88a,0x45137c,0x451913,0x451c03,0x4539dc,0x453b13,
          0x453e32,0x453ea8,0x454003,0x45418f,0x45461c,0x454709,0x45503f,
          0x455719,0x45576d,0x47467a,0x4759bc,0x479795,0x47e251,0x47eac4,
          0x47fcfd,0x83e000,0x83e010,0x83e020,0x83e030,0x83e040,0x83e050,
          0x83e060,0x83e100,0x83e110,0x83e120,0x83e130,0x83e140}
        assert not(v.hits.intersection(v.hooks)-allowed),[hex(a) for a in sorted(v.hits.intersection(v.hooks)-allowed)]
        if frames and 'after' not in frames[-1]:frames[-1]['after']=snap()
        cases.append(dict(name=spec['name'],spec=spec,roster=roster,before=before,frames=list(frames),menus=list(menus),events=list(events),trace=list(trace),after=snap(),reason=reason,
          hooks=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
    return dict(executable_sha256=EDITION,patch_sha256=PATCH_SHA,cases=cases,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'))
    p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_player_input.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n')
    print(len(r['cases']),'original player input sequences')
