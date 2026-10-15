#!/usr/bin/env python3
"""Execute the original section-1 event selector and every opening condition.

The interpreter object is populated with the original file/vtable and its
documented in-memory fields. No condition, getter, selector or outcome routine
is hooked. Existing asset-I/O hooks only apply to construction of the input.
"""
import argparse,json,struct
from pathlib import Path
from battle_initialization_probe import probe
from executable_probe import EDITION
from eex import parse
READER=0x492f90
LIVE=0x4b2c50
CTRL=0x4b3d08

def prepare(source):
    initial,v=probe(source,animation_state=True)
    v.uc.mem_map(0,0x1000) # SEH chain used by original selector.
    v.ranges += [(0x40fb50,0x40fd82),(0x41004d,0x41023c),
        (0x410910,0x410990),(0x4109ef,0x4112df),(0x4112e9,0x41149d),
        (0x411652,0x411748),(0x4117a1,0x4118d4),(0x411a54,0x411c36),
        (0x411d63,0x411db9),(0x411fdc,0x412006),(0x4184b2,0x4184d1),
        (0x41c330,0x41c341),(0x42b510,0x42b51b),(0x44c658,0x44cc03),
        (0x44f7d0,0x44f89b),(0x4807f0,0x480bb0),(0x45afca,0x45b2a8),
        (0x45b371,0x45b4a5),(0x406710,0x40672b),(0x4359f5,0x435ac0),
        (0x477adc,0x477b63),(0x403360,0x403375),(0x43fc1a,0x43fc48)]
    # Restore final introduction state captured by the separate action oracle.
    actions=json.loads(Path('tests/fixtures/original_battle_actions.json').read_text())
    for row in actions['chain'][-1]['after']:
        v.write(LIVE+row[4]*36,bytes(row))
    v.write(LIVE+14,b'\x07')
    # Rebuild occupancy from final live state, including scripted removals.
    v.write(0x818c80,b'\xff'*1600)
    for n in range(115):
        b=v.read(LIVE+n*36,36)
        if b[12]==2:v.write(0x818c80+b[7]*20+b[6],bytes([n]))
    raw=(source/'S_00.eex').read_bytes();v.write(0x800000,raw)
    v.put32(READER,0x486610);v.put32(READER+4,1)
    v.write(READER+0x10,b'\xff\xff');v.put32(READER+0x14,-1)
    v.put32(READER+0x20,0x800000);v.put32(READER+0x28,1)
    v.put32(READER+0x2c,0);v.put32(READER+0x30,10);v.put32(READER+0x34,0)
    # Original generic script storage blocks 5/6 are outcome latches.
    v.put32(READER+0x1c,0x821000)
    for n in range(7):v.put32(0x821000+n*4,0x821100+n*16)
    v.write(0x821100,b'\0'*112)
    v.write(READER+0x38,b'\0'*4096);v.put32(READER+0x38+500*4,1)
    v.write(0x4b2c48,b'\x01') # Neighbor scratch already constructed; omit CRT atexit registration.
    v.write(CTRL+1,b'\0\0');v.write(CTRL+0x10,b'\0')
    return initial,v,raw,parse(source/'S_00.eex')['sections'][1]['events']

def generate(source):
    initial,v,raw,events=prepare(source)
    state=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000)
    cases=[]
    specs=[dict(turn=t,phase=p,variables=vs) for t in (1,2,3,20,21) for p in (0,1,2) for vs in ([],[14],[11,12,13,14])]
    specs += [dict(turn=1,phase=0,variables=[11,12,13,14],units=[dict(id=uid,hp=hp,lifecycle=life)])
        for uid in (0,32,3,33,146,147,234) for hp in (0,1) for life in (1,2,3,4)]
    specs += [dict(turn=2,phase=0,variables=vs,units=[dict(id=146,hp=0),dict(id=147,hp=0)])
        for vs in ([],[50],[51],[50,51],[50,51,100],[0,1,11,12,13,14,50,51,100])]
    specs += [dict(turn=2,phase=0,variables=[11,12,13,14],units=[dict(id=0,x=x,y=y,lifecycle=2)])
        for x in range(7,14) for y in range(7,14)]
    # A moved party unit cannot share a tile. These synthetic later-battle
    # positions explicitly retire any original occupant before relocation.
    for spec in specs:
        for u in list(spec.get('units',[])):
            if 'x' not in u:continue
            for row in initial['battle']:
                b=v.read(LIVE+row['slot']*36,36)
                if b[12]==2 and list(b[6:8])==[u['x'],u['y']]:
                    spec['units'].append(dict(id=row['unit']['id'],hp=0,lifecycle=3))
    specs += [dict(turn=1,phase=0,variables=[],all_enemies_removed=True),
              dict(turn=1,phase=0,variables=[],outcome=1),
              dict(turn=1,phase=0,variables=[],victory=1,defeat=1)]
    id_slots={r['unit']['id']:r['slot'] for r in initial['battle']}
    for spec in specs:
        v.write(0x400000,state);v.write(0x800000,scratch)
        v.write(CTRL+2,bytes([spec['turn']-1]));v.write(CTRL+0x10,bytes([spec['phase']]))
        v.write(CTRL+1,bytes([spec.get('outcome',0)]))
        v.put32(0x821150,spec.get('victory',0));v.put32(0x821160,spec.get('defeat',0))
        for n in spec['variables']:v.put32(READER+0x38+n*4,1)
        for u in spec.get('units',[]):
            p=LIVE+id_slots[u['id']]*36
            for field,offset in [('x',6),('y',7),('lifecycle',12)]:
                if field in u:v.write(p+offset,bytes([u[field]]))
            if 'hp' in u:v.put32(p+16,u['hp'])
        if spec.get('all_enemies_removed'):
            for n in range(35,115):v.write(LIVE+n*36+12,b'\x03')
        v.write(0x818c80,b'\xff'*1600)
        for n in range(115):
            b=v.read(LIVE+n*36,36)
            if b[12]==2:v.write(0x818c80+b[7]*20+b[6],bytes([n]))
        before=v.read(0x821100,112)
        result=v.call(0x4100a1,this=READER)
        selected=v.u32(READER+12)-0x800000 if result else None
        if selected is not None:
            selected=next(e['offset'] for e in events if e['body'][0]['offset']-2==selected)
        after=v.read(0x821100,112)
        # Independently expose each actual condition result, without using it
        # to substitute the original selector result above.
        v.write(0x821100,before);conditions=[]
        for e in events:
            row=[]
            for i in e['conditions']:
                v.put32(READER+8,0);v.put32(READER+12,0x800000+i['offset'])
                value=v.call(0x4109ef,this=READER)
                assert value in (0,1),(i,value)
                assert v.u32(READER+12)==0x800000+i['end']
                row.append(value)
            conditions.append(row)
        assert v.read(0x821100,112)==after
        cases.append(dict(input=spec,selected=selected,conditions=conditions,victory=v.u32(0x821150),defeat=v.u32(0x821160)))
    return dict(executable_sha256=EDITION,cases=cases,
        configuration=list(v.read(READER+0x3d04,8)),
        boundaries='Original selector 0x4100a1, dispatcher, typed argument readers, all section-1 conditions, occupancy/HP getters, and victory/defeat latches execute without rule hooks. Input is the separately captured post-introduction state; unit/phase/flag mutations are explicit test inputs. No event body or turn simulation is claimed.')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_battle_events.json'));a=p.parse_args()
    result=generate(a.source);a.output.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n');print(len(result['cases']),'event-selection cases')
