#!/usr/bin/env python3
"""Read-only extraction of this edition's original command-menu dialog template.

This decodes resource metadata, not dynamic enablement, positioning, input,
button painting, or the original modal loop. No PE module is loaded/executed.
"""
import argparse,hashlib,json,struct
from pathlib import Path
import pefile
from executable_probe import EDITION

def generate(source):
    raw=(source/'Ekd5.exe').read_bytes()
    if hashlib.sha256(raw).hexdigest()!=EDITION:raise ValueError('unsupported executable edition')
    pe=pefile.PE(data=raw)
    group=next(e for e in pe.DIRECTORY_ENTRY_RESOURCE.entries if e.id==5)
    resource=next(e for e in group.directory.entries if e.id==0x117)
    if len(resource.directory.entries)!=1:raise ValueError('ambiguous dialog language')
    lang=resource.directory.entries[0];entry=lang.data.struct;b=pe.get_data(entry.OffsetToData,entry.Size);pos=0
    def unpack(fmt):
        nonlocal pos
        size=struct.calcsize(fmt)
        if pos+size>len(b):raise ValueError('truncated dialog template')
        result=struct.unpack_from(fmt,b,pos);pos+=size;return result
    def field():
        first,=unpack('<H')
        if first==0:return None
        if first==0xffff:return {'ordinal':unpack('<H')[0]}
        words=[first]
        while True:
            word,=unpack('<H')
            if not word:break
            words.append(word)
        return struct.pack('<'+'H'*len(words),*words).decode('utf-16le')
    style,extended,count,x,y,w,h=unpack('<IIHhhhh')
    if count!=5 or style!=0x800000c0:raise ValueError('unverified command dialog template')
    dialog=dict(style=style,extended_style=extended,bounds_dlu=[x,y,w,h],menu=field(),window_class=field(),title=field())
    if style&0x40:dialog['font']={'point_size':unpack('<H')[0],'face':field()}
    controls=[]
    for _ in range(count):
        pos=(pos+3)&~3
        style,extended,x,y,w,h,id=unpack('<IIhhhhH')
        control=dict(id=id,style=style,extended_style=extended,bounds_dlu=[x,y,w,h],window_class=field(),text=field())
        extra,=unpack('<H')
        if extra:raise ValueError('unverified control creation data')
        controls.append(control)
    if pos!=len(b):raise ValueError('unconsumed dialog template bytes')
    return dict(executable_sha256=EDITION,resource_type=5,resource_id=0x117,language=lang.id,rva=entry.OffsetToData,byte_size=len(b),resource_sha256=hashlib.sha256(b).hexdigest(),dialog=dialog,controls=controls,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_tutorial_menu_resource.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,indent=2)+'\n');print([(hex(c['id']),c['text'],c['bounds_dlu']) for c in r['controls']])
