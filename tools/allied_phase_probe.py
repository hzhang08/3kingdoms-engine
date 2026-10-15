#!/usr/bin/env python3
"""Original allied caller, AI, walking, physical attacks and first NPC death.

Input is the separately captured completed introduction and first infantry
event. No AI decision, movement, combat, event condition or RNG is replaced.
Raster/window/audio/wait operations are explicit presentation boundaries.
Stops at the enemy-turn banner before enemy maintenance or decisions.
"""
import argparse,json
from pathlib import Path
from unicorn import UC_HOOK_CODE
from battle_events_probe import prepare,LIVE,CTRL,READER
from attack_controller_probe import configure
from defeated_unit_probe import configure as death_configure
from initialization_probe import state,UNIT
from movement_probe import live_json
from executable_probe import EDITION

class Boundary(Exception):pass

def generate(source,enemy_entry=False,return_vm=False):
    _,v,_,script_events=prepare(source);events=configure(v);death_configure(v,events)
    v.put32(READER+0x38+14*4,1);v.put32(READER+0x24,len(script_events))
    v.write(LIVE+18*36+15,b'\0');v.write(0x4b0770+8,b'\x01')
    v.put32(0x4b4254,48);v.put32(0x4b4258,576)
    v.put32(0x4b4264,624);v.put32(0x4b4268,384)
    v.ranges += [(0x44e851,0x44ebfd),(0x44c601,0x44c658),(0x44cc03,0x44cc85),
      (0x4180c1,0x418174),(0x410990,0x4109a1),(0x44f7b0,0x44f7c1),
      (0x44da7a,0x44df61),(0x44f980,0x44f998),(0x425ab0,0x425ac1),
      (0x435182,0x4351da),(0x435991,0x4359f5),(0x43a1f4,0x43a28a),
      (0x43ff54,0x4400a2),(0x406906,0x406947),(0x425ad0,0x425ae1),(0x425af0,0x425b15),
      (0x45b5fe,0x45b6d0),(0x45b910,0x45b92d),(0x45f710,0x45f71e),(0x44f9a0,0x44f9b1),
      (0x43a956,0x43ad1c),(0x442abb,0x442b03),(0x441488,0x441744),
      (0x44049e,0x4404bd),(0x454f10,0x45503f),(0x407907,0x407a09),(0x440252,0x4403aa)]
    trace=[];actions=[];death=[];transition=[];enemy=[];banner_events=[]
    if enemy_entry:
        v.ranges += [(0x43c738,0x43d108),(0x41def0,0x41df0b),(0x41e030,0x41e041),
          (0x4066e0,0x406700),(0x44df61,0x44e0d9),(0x407a09,0x407b41),
          (0x4082d6,0x4083ed),(0x409650,0x40969f),(0x408950,0x4089cb),
          (0x40709a,0x40721b),(0x406650,0x406670)]
    def snapshot():
        return dict(units=[dict(battle=live_json(v,s),unit=state(v,v.u32(LIVE+s*36)))
          for s in range(115) if v.read(LIVE+s*36+12,1)[0] in (1,2,3)],
          rng=v.u32(0x48ecd4),tick=v.read(CTRL+5,1)[0],camera=[v.u32(0x4b4254),v.u32(0x4b4258)])
    def noop(vm):vm.return_from_hook()
    def record(kind,argc=0,pop=0):
        def hook(vm):
            events.append(dict(kind=kind,args=[vm.arg(n) for n in range(argc)]));vm.return_from_hook(pop=pop)
        return hook
    def died(vm):death.append(dict(slot=(vm.this()-LIVE)//36,**snapshot()))
    def enemy_turn(vm):
        transition.append(dict(args=[vm.arg(n) for n in range(3)],**snapshot()))
        if not enemy_entry:raise Boundary()
        from turn_banner_probe import configure as banner_configure
        banner_configure(vm,banner_events);del vm.hooks[0x44ac28]
    def action_done(vm):actions[-1]['after']=snapshot()
    def event_entry(vm):
        enemy.append(dict(body_length_offset=vm.u32(vm.this()+0xc)-vm.u32(vm.this()+0x20),**snapshot()))
        raise Boundary()
    v.put32(0x4862f4,0x820e00)
    v.hooks.update({0x47eac4:noop,0x42b81b:record('cursor',1),0x820e00:record('message_pump',4,16),
      0x441744:died,0x44a88a:record('minimap'),0x44eb24:action_done,0x44ac28:enemy_turn,
      0x4544d3:record('selection_cursor',1,4),0x4404bd:record('panel',3,12),
      0x4350f0:record('movement_overlay',1,4),0x4539dc:record('range',3,12),
      0x453e32:record('clear_panel'),0x453a40:record('target',1,4),
      0x45461c:record('clear_cursor'),0x453b13:record('redraw'),
      0x42c56b:noop,0x42c646:record('selection_wait',1)})
    if enemy_entry:v.hooks[0x41084e]=event_entry
    names={0x44da7a:'priority',0x44c601:'event_batch',0x4100a1:'event_scan',
      0x45b5fe:'action_event_gate',0x44cc03:'death_scan',0x43ad1c:'ai_action',0x43a956:'movement',0x43aafb:'resolution'}
    if enemy_entry:names.update({0x43c7d6:'status',0x43c975:'grades',0x44df61:'poison',0x43cddd:'regeneration'})
    def observe(uc,address,size,user):
        if address in names:
            row=dict(kind=names[address],phase=v.read(CTRL+0x10,1)[0])
            if address in (0x43ad1c,0x43a956,0x43aafb,0x43c7d6,0x43c975,0x43cddd):row['slot']=(v.this()-LIVE)//36
            trace.append(row)
            if address==0x43ad1c and row['phase']==2:
                enemy.append(dict(slot=row['slot'],order=[s for s in range(35,115) if v.read(LIVE+s*36+12,1)[0]==2],
                  priority=list(v.read(LIVE+s*36+11,1)[0] for s in range(115)),**snapshot()));raise Boundary()
            if address==0x43ad1c:actions.append(dict(slot=row['slot'],before=snapshot()))
            if address==0x43aafb:actions[-1]['moved']=snapshot()
    observer=v.uc.hook_add(UC_HOOK_CODE,observe);v.hits.clear();initial=snapshot()
    try:v.call(0x44e851,this=CTRL,budget=60000000)
    except Boundary:pass
    assert [a['slot'] for a in actions]==[20,18,19] and [d['slot'] for d in death]==[38] and len(transition)==1
    allowed={0x403340,0x4036f9,0x406508,0x406541,0x406549,0x406551,0x406559,0x406569,
      0x406730,0x418f49,0x419184,0x4192c5,0x41e5db,0x41e5f6,0x41fb35,0x41fc89,
      0x42b81b,0x42c4e0,0x42c557,0x42c56b,0x42c646,0x42d5f1,0x4303c0,0x43053e,
      0x4350f0,0x43eb1f,0x43f38b,0x43fcc9,0x4404bd,0x441744,0x44a88a,0x44ac28,
      0x44eb24,0x44faf1,0x450984,0x45137c,0x451913,0x4519e6,0x4539dc,0x453a40,
      0x453b13,0x453e32,0x4544d3,0x45461c,0x454cef,0x47467a,0x479a01,0x47e251,
      0x47eac4,0x47fcfd,0x820e00}
    if enemy_entry:
        from turn_banner_probe import HOOKS
        allowed |= HOOKS | {0x41084e}
        assert len(enemy)==1
    assert not(v.hits.intersection(v.hooks)-allowed),[hex(a) for a in v.hits.intersection(v.hooks)-allowed]
    result=dict(executable_sha256=EDITION,initial=initial,actions=actions,death=death,transition=transition,
      trace=trace,events=events,hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))],boundaries=__doc__)
    if enemy_entry:
        from eex import parse
        from deployment_probe import script_call
        selected=next(e for e in parse(source/'S_00.eex')['sections'][1]['events'] if e['body'][0]['offset']==enemy[0]['body_length_offset']+2)
        result.update(enemy_entry=enemy[0],event_offset=selected['offset'],banner_events=list(banner_events),
          boundaries=__doc__+' Enemy-entry mode also executes the original enemy banner (explicit 640x440 client rectangle, raster/font/OS boundaries), action-flag reset and event selector. It then invokes original opcode-76/79 handlers in the selected source event order, executing original opcode-20 dialogue focus and capturing the text-window boundary, and stops before opcode 50. Maintenance and enemy AI have not run. This command-level continuation does not claim original dialogue rasterization or its interpreter dispatcher.')
        v.ranges += [(0x414f42,0x4150e7),(0x4570ee,0x457428),(0x43f040,0x43f38b),
          (0x415f65,0x416129),(0x457428,0x45777f),(0x4351da,0x435884),
          (0x413661,0x413911),(0x41eec0,0x41ef31),(0x4179a5,0x417a54),(0x47fcc3,0x47fcfd)]
        commands=[];raw=(source/'S_00.eex').read_bytes();v.hits.clear();speeches=[]
        old_speech=v.hooks[0x42d5f1]
        def script_speech(vm):
            speeches.append(dict(unit=vm.arg(1),text=vm.read(vm.arg(0),1024).split(b'\0')[0].decode('gbk'),before=snapshot()))
            vm.return_from_hook()
        v.hooks[0x42d5f1]=script_speech
        for command in selected['body']:
            op=command['opcode']
            if op==50:
                result['movement_boundary']=dict(offset=command['offset'],**snapshot());break
            before=snapshot();speeches.clear()
            script_call(v,{20:0x413661,76:0x414f42,79:0x415f65}[op],raw[command['offset']:command['end']])
            row=dict(offset=command['offset'],opcode=op,**snapshot())
            if op==20:
                row['speeches']=list(speeches)
                assert snapshot()['units']==before['units'] and snapshot()['rng']==before['rng']
            commands.append(row)
        assert not(v.hits.intersection(v.hooks)-allowed),[hex(a) for a in v.hits.intersection(v.hooks)-allowed]
        result['commands']=commands
        v.hooks[0x42d5f1]=old_speech
    if return_vm:
        v.uc.hook_del(observer)
        return result,v,events,snapshot
    return result

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_allied_phase.json'));p.add_argument('--enemy-entry',action='store_true');a=p.parse_args();r=generate(a.source,a.enemy_entry);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print('actions',[(c['slot'],c.get('after',{}).get('rng')) for c in r['actions']],'death',r['death'][0]['slot'],'enemy',r.get('enemy_entry',{}).get('slot'))
