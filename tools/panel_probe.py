#!/usr/bin/env python3
"""Original normal unit panel layout, values and raster requests, no Win32."""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from ai_decision_probe import configure as ai_configure
from movement_probe import live_json
from initialization_probe import state,UNIT
from executable_probe import EDITION
from ui_assets import decode_ui

def configure(v,events=None,preserve_view=False):
    source=v.source
    if not preserve_view:ai_configure(v)
    v.ranges += [(0x4403aa,0x440e77),(0x44fc4b,0x450036),(0x451a4f,0x451ab5),
      (0x455e44,0x455e92),(0x40eb54,0x40edc0),(0x4430e0,0x443100),
      (0x45111c,0x45137c),(0x453e81,0x453e97),(0x43ff54,0x43ff6e),(0x406650,0x406670),(0x407757,0x407818),
      (0x40e499,0x40ea03),(0x40ea71,0x40eab7),(0x40edc0,0x40f212),
      (0x40faa0,0x40fb44),(0x47fc5a,0x47fc7d)]
    regions=list(v.uc.mem_regions())
    buffer=next(a for a in [0x900000]+list(range(0x2200000,0x2600000,0x40000)) if all(a+0x30000<=lo or a>hi for lo,hi,_ in regions))
    mark=(source/'Mark.e5').read_bytes()
    v.uc.mem_map(buffer,0x30000);v.put32(0x4aeb50+4,len(mark));v.put32(0x4aeb50+12,buffer);v.write(0x4aeb50+16,b'\x01')
    v.write(buffer,mark)
    text_object=0x492f38;v.put32(text_object+0x10,79);v.put32(text_object+0x18,480)
    if not preserve_view:
        v.put32(0x4b4254,48);v.put32(0x4b4258,576);v.put32(0x4b4264,624);v.put32(0x4b4268,384);v.put32(0x4b428c,0);v.put32(0x4b4290,56)
    if events is None:events=[]
    def noop(vm):vm.return_from_hook()
    def record(kind,count,pop=0):
        def hook(vm):events.append(dict(kind=kind,args=[vm.arg(n)&255 if kind=='window' else vm.arg(n) for n in range(count)]));vm.return_from_hook(pop=pop)
        return hook
    def formatted(vm):
        # Observe the call but execute the original formatter and GBK cursor.
        p=vm.arg(0)
        events.append(dict(kind='text',x=vm.u32(p+4)*8,y=vm.u32(p+8),text='',glyphs=[],foreground=vm.read(p+1,1)[0],background=vm.u32(p+0x20),opaque=bool(vm.u32(p+0x34))))
    def glyph(vm):
        code=vm.arg(2);raw=code.to_bytes(1 if code<256 else 2,'big');ch=raw.decode('gbk')
        events[-1]['text']+=ch
        events[-1]['glyphs'].append([vm.arg(0)*8,vm.arg(1),ch])
        vm.return_from_hook(0,pop=16)
    def icon(vm):events.append(dict(kind='icon',args=[vm.arg(n) for n in range(4)]+[vm.arg(4)-buffer,vm.arg(5)]));vm.return_from_hook()
    def pattern(vm):events.append(dict(kind='pattern',args=[vm.arg(0),vm.arg(1),vm.arg(2)-buffer]));vm.return_from_hook()
    v.hooks.update({0x41e5db:noop,0x41e5f6:noop,0x47d239:noop,0x47d1f1:noop,
      0x453c17:record('window',4,16),0x40faa0:formatted,0x40f548:glyph,0x451913:icon,
      0x41e6a3:record('bar_background',6),0x47d38d:pattern,0x47dbc3:record('bar_fill',5)})
    return events

def generate(source):
    _,v,_,_=prepare(source);events=configure(v)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    specs=[dict(slot=n) for n in range(115) if v.read(LIVE+n*36+12,1)[0]==2]+[dict(slot=0)]
    specs += [dict(slot=20,position=[x+1,y+12],camera=[1,12]) for y in range(8) for x in range(13)]
    specs += [dict(slot=20,job=n) for n in range(53)]
    specs += [dict(slot=20,terrain=n) for n in range(30)]
    specs += [dict(slot=0,side=side,flags=flags,xp=xp,position=[7,16]) for side in range(4) for flags in (0,2,4) for xp in (0,99,255)]
    specs += [dict(slot=20,hp=hp,mp=mp) for hp in (0,1,2,44,89,90) for mp in (0,1,5,10)]
    for spec in specs:
        slot=spec['slot']
        v.write(0x400000,original);v.write(0x800000,scratch);events.clear()
        ptr=LIVE+slot*36;uid=v.u32(ptr);record=UNIT+uid*72
        for field,offset in [('side',5),('flags',13)]:
            if field in spec:v.write(ptr+offset,bytes([spec[field]]))
        for field,offset in [('hp',16),('mp',20)]:
            if field in spec:v.put32(ptr+offset,spec[field])
        for field,offset in [('job',0x2b),('xp',0x2d)]:
            if field in spec:v.write(record+offset,bytes([spec[field]]))
        if 'position' in spec:v.write(ptr+6,bytes(spec['position']))
        if 'terrain' in spec:
            x,y=v.read(ptr+6,2);v.write(0x818640+y*20+x,bytes([spec['terrain']]))
        x,y=v.read(ptr+6,2);camera=spec.get('camera',[max(0,min(7,x-6)),max(0,min(12,y-4))]);v.put32(0x4b4254,camera[0]*48);v.put32(0x4b4258,camera[1]*48)
        before=v.read(LIVE,115*36),v.read(UNIT,512*72),v.u32(0x48ecd4)
        v.call(0x4404bd,[0,255,255],LIVE+slot*36)
        after=bytearray(v.read(UNIT,512*72));uid=v.u32(LIVE+slot*36)
        # 0x407757 refreshes this record's cached digit-stripped display name.
        after[uid*72+0x37:uid*72+0x47]=before[1][uid*72+0x37:uid*72+0x47]
        assert before==(v.read(LIVE,115*36),bytes(after),v.u32(0x48ecd4))
        b=live_json(v,slot);cases.append(dict(battle=b,unit=state(v,b['id']),name=v.read(record+8,13).split(b'\0')[0].decode('gbk'),camera=camera,terrain=list(v.read(0x818640,400)),events=list(events)))
    return dict(executable_sha256=EDITION,ui=decode_ui((source/'Ekd5.exe').read_bytes()),cases=cases,boundaries='Original mode-0 panel placement, class/name/HP/MP/XP/terrain/affinity getters, bar scaling and drawing requests. The original printf, GBK decoder, wrapping and cursor execute unmodified; glyph rasterization is captured at 0x40f548. Window raster composition captured at the rectangle boundary. Only the original digit-stripped name cache is refreshed; all other unit bytes and RNG remain unchanged.')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_panel.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['cases']),'original panels')
