#!/usr/bin/env python3
"""Original physical-strike animation, captured at raster/audio/wait boundaries.

Original sprite loading, mirror loops, color transformations, direction and
transient unit flags execute. Camera framing is a captured request (fixed view
supplied by case); background capture/restore, overlays, numeric glyphs and
Win32 message pumping are omitted. This does not test wall-clock playback.
"""
import argparse,hashlib,json
from unicorn.x86_const import UC_X86_REG_EIP
from pathlib import Path
from battle_events_probe import prepare,LIVE
from initialization_probe import UNIT,state
from movement_animation_probe import configure as walking_configure
from movement_probe import live_json
from strike_probe import configure as strike_configure,CONTROL
from executable_probe import EDITION
from ls12 import decode

def configure(v):
    events=walking_configure(v);strike_configure(v)
    v.uc.mem_map(0xd00000,0x20000)
    v.put32(0x4ae4c8+4,0x20000);v.put32(0x4ae4c8+12,0xd00000);v.write(0x4ae4c8+16,b'\x01')
    v.ranges += [(0x403b8f,0x405744),(0x4032b0,0x403337),(0x406730,0x40675a),
      (0x481160,0x48118f),(0x4400a2,0x44012f),(0x4351da,0x4359f5),
      (0x43e515,0x43e562),(0x4303c0,0x4306aa)]
    archives={name.upper():{b.index:b.data for b in decode((v.source/name).read_bytes())} for name in ('Unit_atk.e5','Unit_spc.e5','Unit_mov.e5')}
    opened={};matches={}
    for name,rows in archives.items():
        size=64 if name=='UNIT_ATK.E5' else 48
        for sprite,raw in rows.items():
            for index in range(len(raw)//(size*size)):
                frame=raw[index*size*size:(index+1)*size*size]
                for mirror in (False,True):
                    pixels=b''.join(frame[y*size:(y+1)*size][::-1] for y in range(size)) if mirror else frame
                    key=hashlib.sha256(pixels).hexdigest();matches.setdefault(key,[]).append(dict(archive=name,sprite=sprite,frame=index,mirror=mirror))
    # The color matcher table is separately compared at all 4096 entries.
    palette=json.loads(Path('tests/fixtures/original_combat_palette.json').read_text())
    v.write(0x497770,bytes(palette['quantized']));v.write(0x4b1b28,bytes(palette['lookup']))
    def noop(vm):vm.return_from_hook()
    def open_archive(vm):
        name=vm.read(vm.arg(0),32).split(b'\0')[0].decode('ascii');assert name in archives
        opened[vm.this()]=name;vm.return_from_hook(1,pop=4)
    def load(vm):vm.write(vm.arg(1),archives[opened[vm.this()]][vm.arg(0)]);vm.return_from_hook(1,pop=8)
    def sound(vm):events.append(dict(kind='sound',id=vm.arg(0),flag=vm.arg(1)));vm.return_from_hook(pop=8)
    def draw(vm):
        x,y,w,h,slot,ptr=[vm.arg(n) for n in range(6)];pixels=vm.read(ptr,w*h);sha=hashlib.sha256(pixels).hexdigest()
        events.append(dict(kind='frame',x=x if x<2**31 else x-2**32,y=y if y<2**31 else y-2**32,width=w,height=h,slot=slot&255,pixels_sha256=sha,matches=matches.get(sha,[])))
        vm.return_from_hook()
    def transform(vm):
        ptr,w,h=[vm.arg(n) for n in range(3)];key=hashlib.sha256(vm.read(ptr,w*h)).hexdigest()
        events.append(dict(kind='color_transform',routine=hex(vm.uc.reg_read(UC_X86_REG_EIP)),before=key,width=w,height=h,arguments=[vm.arg(n) for n in range(3,6)] if vm.uc.reg_read(UC_X86_REG_EIP)==0x43053e else [vm.arg(3)]))
        # Observation only; original pixel loop continues.
    def overlay(vm):events.append(dict(kind='overlay',x=vm.arg(0),y=vm.arg(1),slot=vm.arg(2)&255));vm.return_from_hook()
    def number(vm):events.append(dict(kind='number',slot=vm.arg(0)&255,value=vm.arg(1),foreground=vm.arg(2)&255,background=vm.arg(3)&255));vm.return_from_hook()
    def tick(vm):events.append(dict(kind='attack_tick',tick=(vm.read(0x4b3d08+5,1)[0]+1)%8)) # Observe; original write executes.
    v.hooks.update({0x41fb35:open_archive,0x41fc89:load,0x4192c5:noop,0x418f49:noop,
      0x47467a:sound,0x45137c:draw,0x4303c0:transform,0x43053e:transform,0x43eb1f:overlay,
      0x44faf1:number,0x4519e6:noop,0x406730:tick})
    v.attack_archives=archives
    return events

def generate(source):
    _,v,_,_=prepare(source);events=configure(v)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    specs=[dict(actor=20,target=38,critical=True,damage=45,mp_damage=0,position=[9,17],camera=[6,12],tick=0)]
    specs += [dict(actor=20,target=38,critical=critical,damage=damage,mp_damage=mp,position=position,camera=[6,12],tick=tick)
      for position in ([9,17],[7,15],[9,13],[11,15]) for critical in (False,True)
      for damage,mp in ((0,0),(1,0),(45,0),(90,0),(0,5)) for tick in (0,7)]
    specs += [dict(actor=20,target=38,critical=critical,damage=1,mp_damage=0,position=[9,17],camera=[6,12],tick=3,job=job)
      for job in range(53) for critical in (False,True)]
    specs += [dict(actor=20,target=38,critical=critical,damage=0,mp_damage=0,position=[9,17],camera=[6,12],tick=0,target_hp=0)
      for critical in (False,True)]
    for spec in specs:
        actor,target,critical,damage=[spec[n] for n in ('actor','target','critical','damage')]
        v.write(0x400000,original);v.write(0x800000,scratch);v.write(0x900000,v.walking_pixels);events.clear()
        uid=v.u32(LIVE+actor*36)
        if 'job' in spec:
            v.write(UNIT+uid*72+0x2b,bytes([spec['job']]))
            sprite=v.call(0x43fc48,this=LIVE+actor*36)
            v.write(0x900000+actor*0x6300,v.attack_archives['UNIT_MOV.E5'][sprite])
            v.write(LIVE+actor*36+13,bytes([v.read(LIVE+actor*36+13,1)[0]&~8]))
        if 'target_hp' in spec:v.put32(LIVE+target*36+16,spec['target_hp'])
        v.write(LIVE+actor*36+6,bytes(spec['position']));v.put32(0x4b4254,spec['camera'][0]*48);v.put32(0x4b4258,spec['camera'][1]*48);v.write(0x4b3d08+5,bytes([spec['tick']]))
        v.write(CONTROL,b'\0'*0x614);uid=v.u32(LIVE+actor*36)
        v.write(CONTROL,bytes([actor,target]));v.put32(CONTROL+4,uid);v.put32(CONTROL+8,UNIT+uid*72);v.put32(CONTROL+12,LIVE+actor*36)
        v.write(CONTROL+16,bytes([target,255]));v.put32(CONTROL+0x84,damage);v.put32(CONTROL+0x254,spec['mp_damage']);v.put32(CONTROL+0x604,int(critical))
        v.call(0x435829,[actor,target]);v.call(0x403b8f,this=CONTROL)
        old_direction=v.read(LIVE+target*36+15,1)
        v.call(0x435829,[target,actor]);v.call(0x403d85,[0],CONTROL);v.write(LIVE+target*36+15,old_direction);v.call(0x435829,[target,255])
        events.clear();before=[live_json(v,s) for s in (actor,target)];persistent=v.read(UNIT,512*72);seed=v.u32(0x48ecd4);v.hits.clear()
        v.call(0x4044e9,this=CONTROL,budget=12000000)
        assert persistent==v.read(UNIT,512*72) and seed==v.u32(0x48ecd4)
        allowed={0x41fb35,0x41fc89,0x4192c5,0x418f49,0x47467a,0x45137c,0x4303c0,0x43053e,0x43eb1f,0x44faf1,0x4519e6,0x47fcfd,0x403340,0x42c4e0,0x41e5db,0x41e5f6,0x4036f9,0x451913,0x47e251,0x406730}
        assert not(v.hits.intersection(v.hooks)-allowed),[hex(a) for a in v.hits.intersection(v.hooks)-allowed]
        cases.append(dict(before=before,actor_unit=state(v,uid),target_unit=state(v,v.u32(LIVE+target*36)),critical=critical,damage=damage,mp_damage=spec['mp_damage'],camera=spec['camera'],tick=spec['tick'],events=list(events),after=[live_json(v,s) for s in (actor,target)],final_tick=v.read(0x4b3d08+5,1)[0],rng=seed))
    return dict(executable_sha256=EDITION,cases=cases,boundaries=__doc__)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_attack_animation.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'attack animation cases')
