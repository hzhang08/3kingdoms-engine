#!/usr/bin/env python3
"""Hash-gated original opcode-33 placement and Mark fire-frame reference.

Executes terrain-effect slot initialization, typed script argument handling,
slot selection/replacement, terrain/effect-plane writes, camera movement and
redraw dispatch. Host pixel drawing, audio playback and waiting are captured.
The indexed fire pixels are captured at the original draw call. This is not a
Windows interactive timing or audible playback test. Removal is not covered.
"""
import argparse,hashlib,json
from pathlib import Path
from unicorn import UC_HOOK_CODE
from battle_initialization_probe import probe
from deployment_probe import script_call
from executable_probe import EDITION
from eex import parse
CTRL=0x4b3d08;CAMERA=0x4b4250;MAP=0x818000
HOOKS={0x41e5db,0x41e5f6,0x476882,0x45218f,0x47467a,0x403340,0x42c4e0,0x83e000,0x451913}

def configure(v,events):
    v.ranges += [(0x413efb,0x41405b),(0x456140,0x456501),(0x4567b0,0x4567c1),
      (0x43b420,0x43b461),(0x455da8,0x455df6),(0x453b48,0x453b7e),
      (0x45503f,0x455123),(0x454f10,0x45503f),(0x454b7e,0x454f10),
      (0x47fad0,0x47fae7),(0x45461c,0x454709),(0x453b13,0x453b48),
      (0x452744,0x452898),(0x41c2d0,0x41c2e1)]
    v.put32(CAMERA+0x14,624);v.put32(CAMERA+0x18,384)
    v.write(CAMERA+0x54,b'\xff\xff');v.write(CAMERA+0x61,b'\0')
    for a in (0x41e5db,0x41e5f6,0x476882,0x42c4e0):v.hooks[a]=lambda vm:vm.return_from_hook()
    v.put32(0x4862c8,0x83e000);v.hooks[0x83e000]=lambda vm:vm.return_from_hook(0,pop=4)
    in_camera=[False]
    def observe(uc,address,size,user):
        if address==0x45503f:
            in_camera[0]=True;events.append(dict(kind='camera_request',x=v.arg(0)&255,y=v.arg(1)&255))
        elif address==0x456415:in_camera[0]=False
    observer=v.uc.hook_add(UC_HOOK_CODE,observe)
    def redraw(vm):
        events.append(dict(kind='camera_redraw' if in_camera[0] else 'redraw',rectangle=[vm.arg(n)&255 for n in range(4)],camera=[vm.u32(CAMERA+4),vm.u32(CAMERA+8)]));vm.return_from_hook(pop=16)
    def sound(vm):events.append(dict(kind='sound',id=vm.arg(0),flag=vm.arg(1)));vm.return_from_hook(pop=8)
    def wait(vm):events.append(dict(kind='wait',ticks=vm.arg(0)));vm.return_from_hook(pop=4)
    v.hooks.update({0x45218f:redraw,0x47467a:sound,0x403340:wait})
    return observer

def snapshot(v):
    return dict(records=[list(v.read(CTRL+0x19+n*5,5)) for n in range(250)],
      terrain=list(v.read(MAP+0x640,400)),effects=list(v.read(MAP+0x1f40,400)),
      camera=[v.u32(CAMERA+4),v.u32(CAMERA+8)],rng=v.u32(0x48ecd4),tick=v.read(CTRL+5,1)[0])

def generate(source):
    _,v=probe(source,animation_state=True);v.uc.mem_map(0,0x1000)
    assert v.read(CTRL+0x19,1250)==b'\xff'*1250
    events=[];observer=configure(v,events)
    initial=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000)
    def restore():v.write(0x400000,initial);v.write(0x800000,scratch)
    def run(args,label,wire=None):
        before=snapshot(v);persistent=v.read(0x4a1b68,512*72);units=v.read(0x4b2c50,115*36)
        events.clear();v.hits.clear()
        if wire:script_call(v,0x413efb,wire)
        else:v.call(0x456140,[args[0],args[1],26,0,args[3],args[4]],0x4b5df0)
        assert persistent==v.read(0x4a1b68,512*72) and units==v.read(0x4b2c50,115*36)
        assert not(v.hits.intersection(v.hooks)-HOOKS),[hex(x) for x in v.hits.intersection(v.hooks)-HOOKS]
        return dict(label=label,args=args,before=before,after=snapshot(v),events=list(events))
    raw=(source/'S_00.eex').read_bytes();cases=[]
    def walk(body):
        for i in body:
            if i['opcode']==1:yield from walk(i['body'])
            else:yield i
    fire=[i for i in walk(parse(source/'S_00.eex')['sections'][1]['events'][2]['body']) if i['opcode']==33]
    assert len(fire)==27
    for i in fire:
        args=[a['value'] for a in i['args']];assert args[2]==0
        cases.append(run(args,f"script:{i['offset']:#x}",raw[i['offset']:i['end']]))
    for tile in ((0,0),(19,0),(0,19),(19,19),(6,4),(13,5)):
      for focus in (0,1):
       for silent in (0,1):
        restore();v.put32(CAMERA+4,336);v.put32(CAMERA+8,576)
        cases.append(run([*tile,0,focus,silent],'placement'))
    restore();cases.append(run([13,5,0,0,0],'duplicate:first'));cases.append(run([13,5,0,1,0],'duplicate:second'))
    # Capacity and first-free-slot ordering are explicit valid test inputs.
    restore();v.write(CTRL+0x19,b''.join(bytes([n%20,n//20,26,0,0]) for n in range(250)))
    cases.append(run([19,19,0,1,0],'full:no-space'));cases.append(run([3,4,0,1,0],'full:replace'))
    restore();v.write(CTRL+0x19+5,bytes([13,5,26,0,0]))
    cases.append(run([13,5,0,0,0],'free-before-match'))
    v.uc.hook_del(observer)
    # Original frame picker points directly into Mark.e5; capture all tick bytes.
    mark=(source/'Mark.e5').read_bytes();v.uc.mem_map(0x900000,0x20000);v.write(0x900000,mark)
    v.put32(0x4aeb50+4,len(mark));v.put32(0x4aeb50+12,0x900000);v.write(0x4aeb50+16,b'\x01')
    frames=[]
    def draw(vm):
        x,y,w,h,ptr,mode=[vm.arg(n) for n in range(6)];assert 0x900000<=ptr<0x900000+len(mark)
        frames.append(dict(tick=v.read(CTRL+5,1)[0],x=x,y=y,width=w,height=h,mode=mode,offset=ptr-0x900000,pixels_sha256=hashlib.sha256(vm.read(ptr,w*h)).hexdigest()));vm.return_from_hook()
    v.hooks[0x451913]=draw
    for tick in range(256):v.write(CTRL+5,bytes([tick]));v.call(0x452744,[144,104,0])
    return dict(executable_sha256=EDITION,initial_records_initialized=True,cases=cases,frames=frames,boundaries=__doc__)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_fire.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['cases']),'fire cases;',len(r['frames']),'original frame selections')
