#!/usr/bin/env python3
"""Original scripted disappearance (opcode49 / 0x45a7b7 mode1).

Executes lifecycle, occupancy, status, mirror and AI-target cleanup, original
mode1 animation, and horizontal mirror code. Archive reads supply unchanged
LS12 bytes. Captures source pixels and rectangular blit requests, sound/wait,
minimap and tile redraw boundaries; Win32 raster/audio/window output is omitted.
"""
import argparse,hashlib,json
from pathlib import Path
from battle_events_probe import prepare,LIVE,CTRL
from initialization_probe import UNIT
from movement_probe import live_json
from attack_animation_probe import configure as animation_configure
from defeated_unit_probe import configure as removal_configure
from executable_probe import EDITION

def configure(v,events):
    removal_configure(v,events)
    v.hooks.pop(0x441744,None) # Earlier phase observer is unnecessary; execute the real body.
    v.ranges += [(0x41311c,0x4132d4),(0x45a7b7,0x45a80f),(0x441b06,0x441cab)]
    v.ranges += [(0x45137c,0x451913),(0x430a07,0x430a77),(0x481160,0x48118f)]
    mark=(v.source/'Mark.e5').read_bytes()
    regions=list(v.uc.mem_regions())
    mark_address=next(a for a in range(0x1100000,0x2000000,0x40000) if all(a+0x20000<=lo or a>hi for lo,hi,_ in regions))
    v.uc.mem_map(mark_address,0x20000);v.write(mark_address,mark)
    v.put32(0x4aeb50+4,len(mark));v.put32(0x4aeb50+12,mark_address);v.write(0x4aeb50+16,b'\x01')
    archives=v.attack_archives
    sprites={slot:v.call(0x43fc48,this=LIVE+slot*36) for slot in range(115)}
    def draw(vm):
        x,y,w,h,slot,ptr=[vm.arg(n) for n in range(6)];slot &= 255
        assert w==48 and h in range(4,49,4)
        if 0x900000<=ptr<0x900000+115*0x6300:
            name='UNIT_MOV.E5';frame=(ptr-0x900000-slot*0x6300)//2304
            assert frame==6
        else:
            name='UNIT_SPC.E5';frame=0
            assert ptr==vm.u32(0x4ae4c8+12)+0x6900
        sprite=sprites[slot];pixels=vm.read(ptr,w*h)
        assert pixels==archives[name][sprite][frame*2304:frame*2304+w*h]
        events.append(dict(kind='frame',x=x if x<2**31 else x-2**32,y=y if y<2**31 else y-2**32,width=w,height=h,slot=slot,
                           pixels_sha256=hashlib.sha256(pixels).hexdigest(),archive=name,sprite=sprite,frame=frame,mirror=False))
        # Observe only; execute original terrain mask and lower blit request.
    def raster(vm):
        x,y,w,h,ptr,mode=[vm.arg(n) for n in range(6)]
        assert mode==4
        events.append(dict(kind='raster',x=x if x<2**31 else x-2**32,y=y if y<2**31 else y-2**32,width=w,height=h,
                           pixels_sha256=hashlib.sha256(vm.read(ptr,w*h)).hexdigest()))
        vm.return_from_hook()
    v.hooks[0x45137c]=draw;v.hooks[0x451913]=raster
    return events

def generate(source):
    _,v,_,_=prepare(source);events=animation_configure(v);configure(v,events)
    v.put32(0x4b4254,96);v.put32(0x4b4258,576);v.put32(0x4b4264,624);v.put32(0x4b4268,384)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    # Includes the guide, party members, dead/inactive units and every AI class.
    specs=[dict(slot=s,flags=f,status=st,ai=a,lifecycle=2) for s in (0,18,38,25) for f in (0,6,8,128,255) for st in (0,31) for a in range(5)]
    specs += [dict(slot=25,flags=0,status=0,ai=0,lifecycle=life) for life in (1,3)]
    specs += [dict(slot=25,flags=0,status=0,ai=0,lifecycle=2,position=[10,0],terrain=t) for t in (0,1,2,7,9,10)]
    for spec in specs:
        v.write(0x400000,original);v.write(0x800000,scratch);v.write(0x900000,v.walking_pixels);events.clear()
        slot=spec['slot'];p=LIVE+slot*36;v.write(p+6,bytes(spec.get('position',[9,17])));v.write(p+13,bytes([spec['flags']]))
        v.write(p+30,bytes([spec['status']]));v.write(p+12,bytes([spec['lifecycle']]))
        v.write(LIVE+37*36+8,bytes([slot]));v.write(LIVE+37*36+14,bytes([spec['ai']]))
        x,y=v.read(p+6,2);tile=y*20+x
        v.write(0x818c80+tile,bytes([slot]))
        if 'terrain' in spec:v.write(0x818640+tile,bytes([spec['terrain']]))
        roster=[live_json(v,s) for s in range(115) if v.read(LIVE+s*36+12,1)[0] in (1,2,3)]
        uid=v.u32(p);persistent=v.read(UNIT,512*72);rng=v.u32(0x48ecd4);control=v.read(CTRL,0x530);v.hits.clear()
        sprite=v.call(0x43fc48,this=p);v.call(0x45a7b7,[uid,1,0],0x4b5df0)
        assert persistent==v.read(UNIT,512*72) and rng==v.u32(0x48ecd4) and control==v.read(CTRL,0x530)
        hooks=v.hits.intersection(v.hooks)
        allowed={0x43fcc9,0x47467a,0x403340,0x454cef,0x42c4e0,0x44a88a,0x4192c5,0x418f49,0x479a01,0x47fcfd,0x41fb35,0x41fc89,0x41e5db,0x41e5f6,0x4036f9,0x45137c,0x451913}
        assert not hooks-allowed,[hex(a) for a in sorted(hooks-allowed)]
        cases.append(dict(spec=spec,sprite=sprite,terrain=v.read(0x818640+tile,1)[0],before=roster,after=[live_json(v,b['slot']) for b in roster],events=list(events),camera=[v.u32(0x4b4254),v.u32(0x4b4258)],rng=rng,
                          occupancy=v.read(0x818c80+tile,1)[0],hooked=[hex(a) for a in sorted(hooks)]))
    return dict(executable_sha256=EDITION,cases=cases,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_disappearance.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'original scripted disappearances')
