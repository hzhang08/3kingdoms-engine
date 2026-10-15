#!/usr/bin/env python3
"""Run this edition's unit initialization on a bounded x86 CPU emulator.

Only archive I/O is replaced with the importer's decoded Data.e5 block. Unit
loading, growth, equipment selection, party averaging and joining execute the
original instructions. Windows code is never loaded or called.
"""
import argparse,json,struct
from pathlib import Path
from probe_vm import ProbeVM
from executable_probe import EDITION
from game_state import default_dictionary

UNIT=0x4a1b68
RANGES=[(0x408be3,0x408f2e),(0x4093ef,0x40948a),
 (0x406e17,0x406f38),(0x4072fc,0x4073a4),(0x4073b8,0x4075a7),
 (0x4095a0,0x4095d6),(0x407fe7,0x4082d6),(0x47fa89,0x47fae7),
 (0x40c14f,0x40c537),(0x41a190,0x41a1b0),(0x418d80,0x418d91),
 (0x4065d0,0x4065e1),(0x406610,0x406621),(0x40dfb0,0x40dfc6),
 (0x40dff0,0x40e026),(0x40e090,0x40e0a9),(0x47fa40,0x47fa78),
 (0x41b606,0x41b639),(0x409500,0x409518)]

def state(vm,uid):
    row=default_dictionary().read_values(vm.read,'persistent_units',uid)
    return dict(id=uid,**{key:row[key] for key in ('base_stats','stats','hp','mp','owner','class','level','xp','equipment','record_counters')})

def prepare(source):
    vm=ProbeVM(source,RANGES)
    for n in range(53):
        vm.write(0x800000,vm.data[3][n*27:(n+1)*27])
        vm.call(0x4093ef,[0x800000],0x4abfa0+n*27)
    for n in range(512):vm.load_unit(n)
    vm.put32(0x4ae4c8+12,0x810000);vm.write(0x4ae4c8+16,b'\x01')
    def archive(v):
        assert v.arg(1)==0
        assert v.read(v.arg(0),8)==b'DATA.E5\0'
        v.write(v.arg(2),v.data[0]);v.return_from_hook(1)
    vm.hooks[0x41b51e]=archive
    return vm

def generate(source):
    vm=prepare(source);growth=[];gear=[];joins=[];averages=[]
    for uid in range(512):
        for increment in (0,1,3,14,29,50):
            vm.load_unit(uid);before=state(vm,uid)
            vm.call(0x4073b8,[increment,0],UNIT+72*uid)
            growth.append(dict(before=before,increment=increment,after=state(vm,uid)))
    for attr in (0,24,25,34,35,44,45,255):
        for level in (0,1,49,50):
            vm.load_unit(0);vm.write(UNIT+0x21,bytes([attr])*5)
            vm.write(UNIT+0x2c,bytes([level]));vm.write(UNIT+0x15,bytes([254])*5)
            vm.put32(UNIT+0x1c,499);vm.write(UNIT+0x20,bytes([249]))
            before=state(vm,0);vm.call(0x4073b8,[5,0],UNIT)
            growth.append(dict(before=before,increment=5,after=state(vm,0)))
    for cls in range(53):
        for level in range(1,51):
            for lower in (0,1):
                vm.load_unit(0);vm.write(UNIT+0x2b,bytes([cls,level]))
                before=state(vm,0)
                for slot in (0,1):vm.call(0x407fe7,[slot,lower],UNIT)
                gear.append(dict(before=before,lower=lower,after=state(vm,0)))
    for levels in ([],[3],[10,1],[1,10],[50,10,1],list(range(1,27)),list(range(26,0,-1))):
        for uid in range(26):
            vm.load_unit(uid)
            if uid<len(levels):vm.write(UNIT+uid*72+0x2a,bytes([0,vm.read(UNIT+uid*72+0x2b,1)[0],levels[uid]]))
        averages.append(dict(levels=levels,result=vm.call(0x40c14f)&255))
    for uid in (0,1,3,5,10,25,146,147,234,334):
        for adjustment in (0,2,14,29,49,50,51,100):
            for n in range(26):vm.load_unit(n)
            vm.load_unit(uid);vm.call(0x40c28c,[uid,0,adjustment])
            joins.append(dict(id=uid,adjustment=adjustment,after=state(vm,uid)))
    return dict(executable_sha256=EDITION,method='bounded Unicorn x86 execution',
        hooks={'0x41b51e':'Archive I/O: copies decoded Data.e5 block 0; validates filename and block.'},
        growth=growth,default_equipment=gear,party_average=averages,join=joins)

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));ap.add_argument('--output',type=Path,default=Path('tests/fixtures/original_initialization.json'));a=ap.parse_args()
    result=generate(a.source);a.output.write_text(json.dumps(result,ensure_ascii=False,separators=(',',':'))+'\n')
    print(json.dumps({k:len(v) for k,v in result.items() if isinstance(v,list)}))
