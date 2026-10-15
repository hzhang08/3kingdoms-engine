#!/usr/bin/env python3
"""Original 0x4218d6 caster pose and 0x42f699 tactic caption.

Continues the original enemy walk, selector and poison plan. Archive bytes,
mirroring, direction, caption placement and GBK glyph cursor run as original
x86. Raster/window/audio/waits are captured boundaries. The host clock is an
explicit deterministic input; this does not verify real-time Windows playback.
Stops after the caster pose, before MEFF effect playback and application.
"""
import argparse,json
from pathlib import Path
from enemy_phase_probe import generate as prepare
from tactic_plan_probe import configure as plan_configure,PresentationBoundary,CONTROL,plan
from battle_events_probe import LIVE
from initialization_probe import UNIT
from executable_probe import EDITION

def configure(v,events):
    v.ranges += [(0x4218d6,0x421cd5),(0x42f190,0x42f433),(0x42f433,0x42f754),
      (0x40b922,0x40b940),(0x40b8b0,0x40b8c1),(0x41c330,0x41c341),
      (0x40eb54,0x40edc0),(0x40e499,0x40ea03),(0x40ea71,0x40eab7),
      (0x40edc0,0x40f212),(0x40f9cf,0x40fa39),(0x47fc5a,0x47fcc3)]
    v.cast_elapsed=0;v.cast_clock_calls=0
    v.put32(0x492f38+0x10,79);v.put32(0x492f38+0x18,480)
    def noop(vm):vm.return_from_hook()
    def clock(vm):
        n=vm.cast_clock_calls;vm.cast_clock_calls+=1
        vm.return_from_hook(100+(vm.cast_elapsed if n%2 else 0))
    def record(kind,argc=0,pop=0):
        def hook(vm):
            events.append(dict(kind=kind,args=[vm.arg(n) for n in range(argc)]));vm.return_from_hook(pop=pop)
        return hook
    def text(vm):
        p=vm.arg(0)
        events.append(dict(kind='caption_text',x=vm.u32(p+4)*8,y=vm.u32(p+8),text='',glyphs=[],
          foreground=vm.read(p+1,1)[0],background=vm.u32(p+0x20),opaque=bool(vm.u32(p+0x34))))
    def glyph(vm):
        code=vm.arg(2);ch=code.to_bytes(1 if code<256 else 2,'big').decode('gbk')
        events[-1]['text']+=ch;events[-1]['glyphs'].append([vm.arg(0)*8,vm.arg(1),ch]);vm.return_from_hook(0,pop=16)
    def restore(vm):
        assert vm.arg(4)==0xd00000 and vm.arg(5)==0
        events.append(dict(kind='caster_restore',slot=vm.read(CONTROL+1,1)[0],rectangle=[vm.arg(n) for n in range(4)]));vm.return_from_hook()
    v.hooks.update({0x451913:restore,0x47eb97:clock,0x42c600:noop,0x426066:noop,0x40fa1a:text,0x40f548:glyph,
      0x47d239:noop,0x47d1f1:noop,0x4735bb:record('caption_window',5,20),
      0x473623:record('caption_close'),0x42c646:record('caption_wait',1)})

def prepared(source):
    entry,v,events,snapshot=prepare(source,True);plan_configure(v)
    v.call(0x441488,[2,20,33],LIVE+35*36)
    try:v.call(0x43dc99,[33],LIVE+35*36)
    except PresentationBoundary:pass
    else:raise AssertionError('expected poison presentation boundary')
    configure(v,events)
    return entry,v,events,snapshot

def generate(source):
    _,v,events,snapshot=prepared(source)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);pixels=v.read(0x900000,115*0x6300)
    cases=[]
    specs=[{}]+[dict(job=job,target=tile,speed=speed,elapsed=(speed+1)*10) for job in range(53)
      for tile in ([9,11],[11,13],[9,17],[7,13]) for speed in (1,)]
    specs += [dict(camera=xy,flags=f) for xy in ([144,576],[0,0]) for f in (0,4,8,12,132,255)]
    specs += [dict(speed=n,elapsed=(n+1)*10) for n in (0,2)]+[dict(elapsed=n) for n in (0,1,19,20,21,100)]+[dict(tactic=t) for t in (23,25)]
    for spec in specs:
        v.write(0x400000,original);v.write(0x800000,scratch);v.write(0x900000,pixels)
        if 'camera' in spec:v.put32(0x4b4254,spec['camera'][0]);v.put32(0x4b4258,spec['camera'][1])
        if 'flags' in spec:v.write(LIVE+35*36+13,bytes([spec['flags']]))
        if 'job' in spec:v.write(UNIT+146*72+0x2b,bytes([spec['job']]))
        if 'target' in spec:v.write(LIVE+20*36+6,bytes(spec['target']))
        if 'speed' in spec:v.write(0x4b0770+0x10,bytes([spec['speed']]))
        if 'tactic' in spec:v.write(CONTROL,bytes([spec['tactic']]))
        # Keep cached walking bytes consistent with synthetic actor class.
        sprite=v.call(0x43fc48,this=LIVE+35*36);atlas=v.attack_archives['UNIT_MOV.E5'][sprite]
        v.write(0x900000+35*0x6300,atlas)
        if v.read(LIVE+35*36+13,1)[0]&8:
            for frame in (4,5,8):
                raw=atlas[frame*2304:(frame+1)*2304]
                v.write(0x900000+35*0x6300+frame*2304,b''.join(raw[y*48:(y+1)*48][::-1] for y in range(48)))
        events.clear();v.hits.clear();v.cast_clock_calls=0;v.cast_elapsed=spec.get('elapsed',(v.read(0x4b0770+0x10,1)[0]+1)*10)
        before=snapshot();planned=plan(v);records=v.read(UNIT,512*72)
        v.call(0x4218d6,this=CONTROL,budget=5000000)
        assert v.read(UNIT,512*72)==records
        assert v.cast_clock_calls==2 and before['rng']==snapshot()['rng'] and before['tick']==snapshot()['tick']
        allowed={0x403340,0x4036f9,0x40f548,0x40fa1a,0x418f49,0x4192c5,0x41e5db,0x41e5f6,0x41fb35,0x41fc89,0x426066,0x42b81b,0x42c4e0,0x42c557,0x42c600,0x42c646,0x43fcc9,0x45137c,0x451913,0x4519e6,0x45461c,0x454cef,0x4735bb,0x473623,0x47467a,0x47d239,0x47eb97,0x47fcfd,0x820e00}
        assert not(v.hits.intersection(v.hooks)-allowed),[hex(a) for a in v.hits.intersection(v.hooks)-allowed]
        cases.append(dict(spec=spec,before=before,after=snapshot(),plan=planned,events=list(events),
          elapsed=v.cast_elapsed,speed=v.read(0x4b0770+0x10,1)[0],tactic=v.read(CONTROL,1)[0],
          hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
    return dict(executable_sha256=EDITION,cases=cases,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_tactic_cast.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['cases']),'original caster animations')
