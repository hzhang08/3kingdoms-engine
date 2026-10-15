#!/usr/bin/env python3
"""Original terrain panel layout, predicates, glyphs and indexed icon pixels.

Wait=0 follows the tutorial caller. Original cursor setters and panel body run;
map redraw is a raster boundary. A clean original Hm00 tile is supplied to the
framebuffer capture (no units/fire overlay in these isolated cases). Original
quantized palette/cube fixture initializes the grayscale transform. Windows
font pixels, full-screen composition and modal input are outside this probe.
"""
import argparse,hashlib,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from panel_probe import configure as base_configure
from executable_probe import EDITION

RANGES=[(0x45576d,0x455c81),(0x43087a,0x430948),(0x435182,0x4351c0),
        (0x420ab6,0x420bbd),(0x4544d3,0x454709),(0x453b48,0x453b6d),(0x40e090,0x40e0a1)]

def configure(v,chained=False):
    if not chained:events=base_configure(v)
    else:
        events=[]
        v.ranges += [(0x40eb54,0x40edc0),(0x40e499,0x40ea03),(0x40ea71,0x40eab7),(0x40edc0,0x40f212),(0x40faa0,0x40fb44),(0x47fc5a,0x47fc7d)]
        text_object=0x492f38;v.put32(text_object+0x10,79);v.put32(text_object+0x18,480)
        def window(vm):events.append(dict(kind='window',args=[vm.arg(n)&255 for n in range(4)]));vm.return_from_hook(pop=16)
        def formatted(vm):
            p=vm.arg(0);events.append(dict(kind='text',x=vm.u32(p+4)*8,y=vm.u32(p+8),text='',glyphs=[],foreground=vm.read(p+1,1)[0],background=vm.u32(p+0x20),opaque=bool(vm.u32(p+0x34))))
        def glyph(vm):
            code=vm.arg(2);ch=code.to_bytes(1 if code<256 else 2,'big').decode('gbk');events[-1]['text']+=ch;events[-1]['glyphs'].append([vm.arg(0)*8,vm.arg(1),ch]);vm.return_from_hook(0,pop=16)
        v.hooks.update({0x453c17:window,0x40faa0:formatted,0x40f548:glyph})
    v.ranges+=RANGES
    mark=0x1100000 if chained else 0x900000;preview=0x1200000 if chained else 0xd00000
    if chained:
        v.uc.mem_map(mark,0x30000);v.write(mark,(v.source/'Mark.e5').read_bytes());v.put32(0x4aeb50+4,0x30000);v.put32(0x4aeb50+12,mark);v.write(0x4aeb50+16,b'\x01')
    for a in (0x4544d3,0x45461c):v.hooks.pop(a,None)
    v.uc.mem_map(preview,0x10000);v.put32(0x4ae4c8+12,preview)
    palette=json.loads(Path('tests/fixtures/original_combat_palette.json').read_text())
    v.write(0x497770,bytes(palette['quantized']));v.write(0x4b1b28,bytes(palette['lookup']))
    image=(v.source/'Hm00.e5').read_bytes();camera=0x4b4250
    v.write(camera+0x54,b'\xff\xff');v.write(camera+0x61,b'\0');v.put32(camera+0x50,0)
    def copy(vm):vm.write(vm.arg(1),vm.read(vm.arg(0),vm.arg(2)));vm.return_from_hook()
    def noop(vm):vm.return_from_hook()
    def redraw(vm):vm.return_from_hook(pop=16)
    def capture(vm):
        x,y,w,h,ptr=[vm.arg(n) for n in range(5)]
        tx=(x-v.u32(camera+0x3c)+v.u32(camera+4))//48;ty=(y-v.u32(camera+0x40)+v.u32(camera+8))//48
        assert w==h==48 and 0<=tx<20 and 0<=ty<20
        pixels=image[(ty*20+tx)*2304:(ty*20+tx+1)*2304];vm.write(ptr,pixels)
        events.append(dict(kind='tile_capture',tile=[tx,ty],screen=[x,y],pixels_sha256=hashlib.sha256(pixels).hexdigest()));vm.return_from_hook()
    def icon(vm):
        x,y,w,h,pointer,mode=[vm.arg(n) for n in range(6)];pixels=vm.read(pointer,w*h)
        if pointer==preview:kind='terrain_tile'
        elif mark<=pointer<mark+0x30000:kind='icon'
        else:kind='gray_icon'
        event=dict(kind=kind,args=[x,y,w,h,pointer-mark if kind=='icon' else 0,mode],pixels_sha256=hashlib.sha256(pixels).hexdigest())
        if kind=='gray_icon':event['pixels']=list(pixels)
        events.append(event);vm.return_from_hook()
    v.hooks.update({0x45218f:redraw,0x47e251:capture,0x47fcfd:copy,0x451913:icon,0x476882:noop})
    return events

def generate(source):
    _,v,_,_=prepare(source);events=configure(v);camera=0x4b4250
    before=(v.read(LIVE,115*36),v.read(0x4a1b68,512*72),v.u32(0x48ecd4));cases=[]
    specs=[dict(tile=[x,y],terrain=0,camera=[0,0]) for y in range(8) for x in range(13)]
    specs += [dict(tile=[11,4],terrain=t,camera=[3,0]) for t in range(30)]
    specs += [dict(tile=[19,19],terrain=t,camera=[7,12]) for t in (0,5,18,29)]
    for spec in specs:
        x,y=spec['tile'];cx,cy=spec['camera'];v.put32(camera+4,cx*48);v.put32(camera+8,cy*48);v.write(camera+0x54,b'\xff\xff')
        v.write(0x818640+y*20+x,bytes([spec['terrain']]));v.write(0x82e000,bytes([x,y]));events.clear();v.hits.clear()
        v.call(0x45576d,[0x82e000,0],camera)
        assert before==(v.read(LIVE,115*36),v.read(0x4a1b68,512*72),v.u32(0x48ecd4))
        cases.append(dict(**spec,events=list(events),cursor=list(v.read(camera+0x54,2)),hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
    v.write(0x82e000,bytes(range(256)));v.call(0x43087a,[0x82e000,256,1]);gray=list(v.read(0x82e000,256))
    predicates=[]
    for terrain in range(30):predicates.append(dict(terrain=terrain,recovery=v.call(0x435182,[terrain]),impassable=v.call(0x455c3a,[terrain]),tactics=[v.call(a,[terrain]) for a in (0x420ab6,0x420b17,0x420b98,0x420b42)]))
    return dict(executable_sha256=EDITION,cases=cases,grayscale=gray,predicates=predicates,boundaries=__doc__)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_terrain_panel.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['cases']),'original terrain panels;',len(r['grayscale']),'palette indices;',len(r['predicates']),'terrain predicates')
