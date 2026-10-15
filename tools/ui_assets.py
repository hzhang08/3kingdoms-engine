"""Read-only, edition-gated UI string tables from the PE image."""
import hashlib,struct
import pefile
from executable_probe import EDITION

def decode_ui(raw):
    if hashlib.sha256(raw).hexdigest()!=EDITION:raise ValueError('unsupported executable edition')
    pe=pefile.PE(data=raw);base=pe.OPTIONAL_HEADER.ImageBase;image=pe.get_memory_mapped_image()
    def text(address):return image[address-base:address-base+256].split(b'\0')[0].decode('gbk')
    def table(address,count):
        return [dict(id=n,address=struct.unpack_from('<I',image,address-base+n*4)[0],text=text(struct.unpack_from('<I',image,address-base+n*4)[0])) for n in range(count)]
    return dict(classes=table(0x48bea8,53),terrain=table(0x48bdb0,30),labels={key:dict(address=a,text=text(a)) for key,a in [('level',0x48e058),('experience',0x48e060),('maximum',0x48e064),('allied',0x48e06c),('enemy',0x48e074)]})

def decode_attack_labels(raw):
    if hashlib.sha256(raw).hexdigest()!=EDITION:raise ValueError('unsupported executable edition')
    image=pefile.PE(data=raw).get_memory_mapped_image()
    return {key:dict(address=a,text=image[a-0x400000:a-0x400000+128].split(b'\0')[0].decode('gbk'))
            for key,a in [('damage',0x48e088),('hit',0x48e094)]}

def decode_turn_labels(raw):
    if hashlib.sha256(raw).hexdigest()!=EDITION:raise ValueError('unsupported executable edition')
    image=pefile.PE(data=raw).get_memory_mapped_image()
    return {key:dict(address=a,text=image[a-0x400000:a-0x400000+128].split(b'\0')[0].decode('gbk'))
            for key,a in [('player',0x48e154),('enemy',0x48e160),('final',0x48e16c),('turn',0x48e178)]}

def decode_objective_labels(raw):
    if hashlib.sha256(raw).hexdigest()!=EDITION:raise ValueError('unsupported executable edition')
    image=pefile.PE(data=raw).get_memory_mapped_image()
    return {key:dict(address=a,text=image[a-0x400000:a-0x400000+128].split(b'\0')[0].decode('gbk'))
            for key,a in [('title',0x48e1d4),('limit',0x48e1e0),('start',0x48e1c8)]}

def decode_tactic_labels(raw):
    if hashlib.sha256(raw).hexdigest()!=EDITION:raise ValueError('unsupported executable edition')
    image=pefile.PE(data=raw).get_memory_mapped_image()
    def text(a):return dict(address=a,text=image[a-0x400000:a-0x400000+128].split(b'\0')[0].decode('gbk'))
    return dict(damage=text(0x48e0a4),hit=text(0x48e0b0),
      types=[text(struct.unpack_from('<I',image,0x48bd40-0x400000+n*4)[0]) for n in range(28)])

def decode_terrain_labels(raw):
    if hashlib.sha256(raw).hexdigest()!=EDITION:raise ValueError('unsupported executable edition')
    image=pefile.PE(data=raw).get_memory_mapped_image()
    return {key:dict(address=a,text=image[a-0x400000:a-0x400000+128].split(b'\0')[0].decode('gbk')) for key,a in [('recovery',0x48e364),('impassable',0x48e370)]}

def decode_tutorial_text(raw):
    if hashlib.sha256(raw).hexdigest()!=EDITION:raise ValueError('unsupported executable edition')
    image=pefile.PE(data=raw).get_memory_mapped_image()
    addresses=[0x48c970,0x48c994,0x48c9ac,0x48c9c4,0x48c9dc,0x48ca10,0x48ca48,0x48ca98,0x48cac4,0x48cafc,0x48cb50,0x48cb70,0x48cb94,0x48cbe0,0x48cc24,0x48cc50,0x48cc98,0x48ccb4,0x48ccc8,0x48cd08,0x48cd40,0x48cd58,0x48cd88,0x48cdb8,0x48cdec,0x48ce24,0x48ce3c,0x48ce58,0x48ce7c,0x48cea8,0x48cedc,0x48cf24,0x48cf58,0x48cf78,0x48cfc0,0x48cff0,0x48d030,0x48d050,0x48d09c,0x48d0d0,0x48d10c,0x48d158,0x48d17c,0x48d1cc,0x48d1f8,0x48d210,0x48d258,0x48d298,0x48d2c0,0x48d2e4,0x48d310,0x48d34c,0x48d374,0x48d39c,0x48d3d0,0x48d3f8,0x48d414,0x48d448,0x48d48c,0x48d4a8,0x48d4c0,0x48d4dc,0x48d524,0x48d554,0x48d584,0x48d5a0,0x48d5d0,0x48d614,0x48d64c,0x48d67c,0x48d6a8,0x48d6dc,0x48d720,0x48d748,0x48d774,0x48d794,0x48d7c8,0x48d7fc,0x48d83c,0x48d858,0x48d888,0x48d8b4,0x48d8e4,0x48d91c]
    return {str(a):dict(address=a,text=image[a-0x400000:a-0x400000+1024].split(b'\0')[0].decode('gbk')) for a in addresses}

def darken_palette(raw,amount=4):
    """Original 0x41e1a4 quantization, 0x42fb10 match, 0x430480 darken.

    Matching preserves dominant component, minimizes both worst and total
    component error, and replaces ties with the later palette entry.
    """
    if len(raw)!=768:raise ValueError('invalid palette')
    colors=[tuple(c>>4 for c in raw[n:n+3]) for n in range(0,768,3)]
    def match(wanted):
        dominant=max(range(3),key=lambda c:wanted[c]);best=0;worst=total=65535
        for index in range(10,246):
            rgb=colors[index];delta=[abs(a-b) for a,b in zip(wanted,rgb)]
            if max(delta)<=worst and sum(delta)<=total and max(range(3),key=lambda c:rgb[c])==dominant:
                best=index;worst=max(delta);total=sum(delta)
        return best
    return [0]+[match(tuple(max(0,c-amount) for c in color)) for color in colors[1:]]

def combat_palette(raw):
    if len(raw)!=768:raise ValueError('invalid combat palette')
    quantized=[c>>4 for c in raw];colors=[tuple(quantized[n:n+3]) for n in range(0,768,3)];lookup=[]
    for value in range(4096):
        wanted=(value>>8,(value>>4)&15,value&15)
        dominant=max(range(3),key=lambda c:wanted[c]);best=0;worst=total=65535
        for index in range(10,246):
            rgb=colors[index];delta=[abs(a-b) for a,b in zip(wanted,rgb)]
            if max(delta)<=worst and sum(delta)<=total and max(range(3),key=lambda c:rgb[c])==dominant:
                best=index;worst=max(delta);total=sum(delta)
        lookup.append(best)
    return dict(quantized=quantized,lookup=lookup)

MARK_IMAGES=[('hp',384,24,24),('mp',960,24,24),('hp-pattern',128,8,8),('mp-pattern',64,8,8),('loss-pattern',0,8,8),
             ('caption',0x18dc0,48,48),('range',0xcc0,48,48),('cursor-neighbor',0x15c0,48,48),('cursor',0x1ec0,48,48),('fire',0x8bc0,48,192)]

MARK_IMAGES += [("tactic-"+str(n),0x1a5c0+n*256,16,16) for n in range(4)]
