#!/usr/bin/env python3
"""Original poison MEFF loader/player and reaction callback at raster boundaries.

Runs 0x4214bf after the verified caster sequence, including MEFF parsing, target
selection, frame order, callback 0x421f04, original indexed blending, sound and
wait requests. Background capture supplies a deterministic synthetic indexed
surface; this proves the compositor independently of the SDL battlefield.
No HP/status/MP/XP application is performed by this presentation routine.
"""
import argparse,hashlib,json
from pathlib import Path
from tactic_cast_probe import prepared,CONTROL,LIVE,UNIT
from executable_probe import EDITION
from ls12 import decode

def configure(v,events):
    v.hooks.pop(0x4214bf,None)
    if not any(start<=0xd20000 and end>=0xd5ffff for start,end,_ in v.uc.mem_regions()):
        v.uc.mem_map(0xd20000,0x40000)
    v.put32(0x4ae4c8+4,0x60000)
    v.ranges += [(0x401000,0x402547),(0x4031e8,0x403262),
      (0x4214bf,0x421796),(0x421f04,0x422498),(0x42fd5a,0x430369),
      (0x430480,0x43053e),(0x45f710,0x45f720)]
    archives=dict(v.attack_archives)
    archives['MEFF.E5']={b.index:b.data for b in decode((v.source/'Meff.e5').read_bytes())}
    opened={}
    def noop(vm):vm.return_from_hook()
    def open_archive(vm):
        name=vm.read(vm.arg(0),32).split(b'\0')[0].decode('ascii');assert name in archives
        opened[vm.this()]=name;vm.return_from_hook(1,pop=4)
    def load(vm):
        name=opened[vm.this()];index=vm.arg(0)&255;raw=archives[name][index]
        vm.write(vm.arg(1),raw);events.append(dict(kind='archive',name=name,index=index,size=len(raw)))
        vm.return_from_hook(len(raw),pop=8)
    def memcpy(vm):vm.write(vm.arg(0),vm.read(vm.arg(1),vm.arg(2)));vm.return_from_hook(vm.arg(0))
    def memset(vm):vm.write(vm.arg(0),bytes([vm.arg(1)&255])*vm.arg(2));vm.return_from_hook(vm.arg(0))
    def capture(vm):
        x,y,w,h,p=[vm.arg(n) for n in range(5)]
        vm.write(p,bytes(((x+xx)*7+(y+yy)*13)%256 for yy in range(h) for xx in range(w)))
        events.append(dict(kind='capture',rectangle=[signed(x),signed(y),w,h]));vm.return_from_hook()
    def raster(vm):
        x,y,w,h,p=[vm.arg(n) for n in range(5)]
        events.append(dict(kind='raster',rectangle=[signed(x),signed(y),w,h],pixels_sha256=hashlib.sha256(vm.read(p,w*h)).hexdigest()));vm.return_from_hook()
    def blend(vm):
        x,y,w,h,src,bg,out,mode,weight,inverse,palette,amount=[vm.arg(n) for n in range(12)]
        assert mode==4 and palette==0 and inverse==0
        events.append(dict(kind='effect',rectangle=[signed(x),signed(y),w,h],weight=weight&255,amount=amount&255,
          pixels_sha256=hashlib.sha256(vm.read(src,w*h)).hexdigest()))
        # Observation only: the original pixel compositor executes.
    def stop(vm):events.append(dict(kind='stop_sound'));vm.return_from_hook()
    # The outer controller would run this caster before loading MEFF. We run it
    # once in prepared_effect instead and retain its resulting camera/facing.
    v.hooks.update({0x4218d6:noop,0x41fb35:open_archive,0x41fc89:load,
      0x4807f0:memcpy,0x480b30:memset,0x47e251:capture,0x47e09b:raster,
      0x43030e:blend,0x474652:stop,0x42c56b:noop})
    return archives['MEFF.E5'][23]

def signed(n):return n if n<2**31 else n-2**32

def generate(source):
    _,v,events,snapshot=prepared(source)
    v.cast_elapsed=10;v.call(0x4218d6,this=CONTROL,budget=5000000)
    raw=configure(v,events)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000)
    cases=[]
    specs=[{}]+[dict(damage=d,effect=e,position=xy) for d,e in ((0,0),(10,0),(0,112),(10,112)) for xy in ([0,0],[19,19])]
    for spec in specs:
        v.write(0x400000,original);v.write(0x800000,scratch);events.clear();v.hits.clear()
        if spec:
            v.put32(CONTROL+0x84,spec['damage']);v.write(CONTROL+0x255,bytes([spec['effect']]))
            v.write(LIVE+20*36+6,bytes(spec['position']))
        before=snapshot();persistent=v.read(UNIT,512*72)
        v.call(0x4214bf,this=CONTROL,budget=12000000)
        after=snapshot();assert persistent==v.read(UNIT,512*72)
        assert before['units']==after['units'] and before['rng']==after['rng'] and before['tick']==after['tick'],'presentation changed battle state'
        allowed={0x403340,0x4036f9,0x418f49,0x4192c5,0x41e5db,0x41e5f6,0x41fb35,0x41fc89,0x4218d6,0x42c4e0,0x42c56b,0x43030e,0x43fcc9,0x44faf1,0x45137c,0x454cef,0x474652,0x47467a,0x47d1f1,0x47e09b,0x47e251,0x4807f0,0x480b30,0x820e00}
        assert not(v.hits.intersection(v.hooks)-allowed)
        cases.append(dict(spec=spec,before=before,after=after,events=list(events),
          damage=v.u32(CONTROL+0x84),effect=v.read(CONTROL+0x255,1)[0],
          hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
    reactions=[]
    reaction_specs=[dict(job=j,direction=d,effect=112 if j%2 else 0,flags=0) for j in range(53) for d in range(4)]
    reaction_specs += [dict(job=6,direction=d,effect=e,flags=f) for d in range(4) for e in (0,112) for f in (2,4,6,14)]
    for spec in reaction_specs:
        v.write(0x400000,original);v.write(0x800000,scratch);events.clear()
        v.write(UNIT+234*72+0x2b,bytes([spec['job']]));v.write(LIVE+20*36+15,bytes([spec['direction']]))
        v.write(LIVE+20*36+13,bytes([spec['flags']]))
        effect=spec['effect'];v.write(CONTROL+0x255,bytes([effect]))
        before=snapshot();v.call(0x421f04,[1,0xd00000,0x44c00],CONTROL,budget=1000000)
        assert before==snapshot()
        row=next(r for r in before['units'] if r['battle']['slot']==20)
        reactions.append(dict(target=row,damage=10,effect=effect,camera=before['camera'],events=list(events)))
    # Exhaustive source/background pairs for the poison compositor parameters.
    source_pixels=bytes(i//256 for i in range(65536));background=bytes(i%256 for i in range(65536))
    v.write(0xd00000,source_pixels);v.write(0xd10000,background)
    v.call(0x42fd5a,[256,256,0xd00000,0xd10000,0xd20000,7,0,0,16],budget=18000000)
    blend_sha=hashlib.sha256(v.read(0xd20000,65536)).hexdigest()
    return dict(executable_sha256=EDITION,resource_sha256=hashlib.sha256(raw).hexdigest(),cases=cases,reactions=reactions,blend_pairs_sha256=blend_sha,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_tactic_effect.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['cases']),'original poison effects')
