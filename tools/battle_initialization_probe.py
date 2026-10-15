#!/usr/bin/env python3
"""Investigate live battle construction, with explicit rendering/I/O boundaries."""
import argparse,json,struct
from pathlib import Path
from deployment_probe import generate,script_call
from initialization_probe import state,UNIT
from eex import parse
from controller_initialization_probe import initialize as initialize_controller

RANGES=[(0x44cc85,0x44d092),(0x44274c,0x4429e9),(0x4429f9,0x442abb),
 (0x442b03,0x442bf6),(0x443100,0x443196),(0x449ae8,0x449b27),
 (0x449b7b,0x449bac),(0x43fbd0,0x43fc1a),(0x406670,0x4066b5),
 (0x418c90,0x418ca1),(0x418cd0,0x418cf1),(0x45f710,0x45f71e),
 (0x43b470,0x43b48e),(0x43f040,0x43f200),(0x43f37a,0x43f38b),(0x4065a0,0x4065c6),
 (0x435884,0x4359f5),(0x443070,0x443086),(0x40721b,0x40727b),
 (0x407c0e,0x407d78),(0x4065f0,0x406611),(0x40678e,0x4067f6),
 (0x40681a,0x406906),(0x409520,0x409531),(0x409560,0x409571),
 (0x4095e0,0x409600),(0x40c674,0x40c6ea),(0x41dd70,0x41dd81),
 (0x41ddb0,0x41ddc1),(0x4327a0,0x4327b1),(0x41df70,0x41df8e),
 (0x4089cb,0x408be3),(0x43f6cd,0x43f74f),(0x459d8b,0x459e5d),
 (0x456079,0x4560f0),(0x45f670,0x45f681),(0x442669,0x44268c)]

def probe(source,return_vm=False,animation_state=False):
    result,v=generate(source,True);v.ranges+=RANGES
    result['controller_initialization']=initialize_controller(v)
    # Imported Hm00 terrain grid, empty occupancy; no game process is run.
    terrain=json.loads(Path('assets/generated/terrain.json').read_text())
    v.put32(0x4aeb38+4,0x2580);v.put32(0x4aeb38+12,0x818000);v.write(0x4aeb38+16,b'\x01')
    v.write(0x818640,bytes(terrain['cells']));v.write(0x818c80,b'\xff'*1600)
    # 0x44c188 clears the separate terrain-effect plane to the unused sentinel.
    v.write(0x819f40,b'\xff'*1600)
    v.write(0x4b422c,bytes([terrain['width'],terrain['height']]))
    v.write(0x4ae4e0,v.data[4])
    for n in range(115):v.write(0x4b2c50+n*36+4,bytes([n]))
    def redraw(vm):vm.return_from_hook()
    # 0x43fcc9 only calls tile drawing; 0x479a01 only updates a Win32 unit window.
    v.hooks[0x43fcc9]=redraw;v.hooks[0x479a01]=redraw
    # Sprite mirroring changes both pixels and a state flag. Execute its flag
    # logic; only the pixel-transform and pixel-copy helpers are omitted.
    v.ranges += [(0x43fd34,0x43fe2c),(0x425af0,0x425b21),(0x442718,0x44274c)]
    v.hooks[0x41ee53]=redraw;v.hooks[0x47fcfd]=redraw
    v.call(0x44cc85,this=0x4b5df0)
    v.ranges += [(0x43fc48,0x43fcc9),(0x406947,0x406c5d)]
    result['battle']=[]
    for n in range(115):
        b=v.read(0x4b2c50+n*36,36)
        if b[12]==0:continue
        uid=struct.unpack_from('<I',b)[0]
        result['battle'].append({'slot':n,'unit':state(v,uid),'live':list(b),'sprite':v.call(0x43fc48,this=0x4b2c50+n*36),'max_hp':v.call(0x40721b,this=UNIT+72*uid),'max_mp':v.call(0x40724a,this=UNIT+72*uid)})
    result['hooks'].update({'0x43fcc9':'tile redraw omitted','0x479a01':'Win32 unit-window update omitted','0x41ee53':'sprite pixel transform omitted','0x47fcfd':'sprite pixel copy omitted'})
    v.ranges += [(0x4150e7,0x41557b),(0x45aa5c,0x45adc1),(0x43e4d6,0x43e515),(0x472c90,0x472ca1)]
    v.write(0x4ac63c,b'\x03');result['hp_commands']=[]
    def walk(body):
        for i in body:
            if i['opcode']==1:yield from walk(i['body'])
            else:yield i
    raw=(source/'S_00.eex').read_bytes()
    for i in walk(parse(source/'S_00.eex')['sections'][0]['events'][0]['body']):
        if i['opcode']!=77:continue
        a=[x['value'] for x in i['args']];uid=a[1]
        live=next(x for x in result['battle'] if x['unit']['id']==uid);ptr=0x4b2c50+live['slot']*36
        before=list(v.read(ptr,36));script_call(v,0x4150e7,raw[i['offset']:i['end']])
        result['hp_commands'].append(dict(offset=i['offset'],args=a,before=before,after=list(v.read(ptr,36))))
    if animation_state:
        return result,v
    # The original idle-frame selector returns a pointer into a sprite atlas.
    # It only computes an address; no image data or renderer is hooked here.
    v.ranges += [(0x43e515,0x43e562),(0x43e572,0x43e69f),(0x43e320,0x43e331),(0x44015a,0x4401a8),(0x4066e0,0x406700),(0x41c2d0,0x41c2e1)]
    v.put32(0x4aab80+4,115*0x6300);v.put32(0x4aab80+12,0x900000);v.write(0x4aab80+16,b'\x01');result['frames']=[]
    for row in result['battle']:
        n=row['slot'];b=v.read(0x4b2c50+n*36,36)
        for tick in range(8):
            v.write(0x4b3d08+5,bytes([tick]))
            pointer=v.call(0x43e572,[n]);frame=(pointer-0x900000-n*0x6300)//0x900
            assert 0<=frame<=10 and (pointer-0x900000)%0x900==0
            result['frames'].append(dict(slot=n,live=list(b),tick=tick,frame=frame))
    v.ranges += [(0x43f7cf,0x43f84d),(0x407a09,0x407af2)]
    result['movement_costs']=[]
    # Exercise every class and terrain, both without and with terrain-cost
    # reduction equipment. Persistent records are direct probe inputs here.
    accessory=next(n for n in range(104) if v.data[1][n*25+17]==34)
    for job in range(53):
        for item in (255,accessory):
            v.load_unit(0);v.write(UNIT+0x2b,bytes([job]));v.write(UNIT+0x34,bytes([item,255,255]));v.put32(0x4b2c50,0)
            result['movement_costs'].append(dict(unit=state(v,0),costs=[v.call(0x43f7cf,[terrain],0x4b2c50)&255 for terrain in range(30)]))
    result['npc_growth']=[]
    for uid in (3,32,33,146,147,157,173,174,210,234,258,334,474):
        for level in (1,14,15,16,29,30,31,49,50):
            for mode in range(3):
                v.load_unit(uid);v.put32(0x4b2c50,uid);before=state(v,uid)
                v.call(0x442890,[mode,level],0x4b2c50)
                result['npc_growth'].append(dict(before=before,level=level,promotion_mode=mode,after=state(v,uid)))
    return (result,v) if return_vm else result

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_battle_initialization.json'));a=p.parse_args()
    result=probe(a.source);a.output.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n');print('Constructed',len(result['battle']),'battle units.')
