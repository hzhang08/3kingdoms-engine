#!/usr/bin/env python3
"""Original script handlers with the original argument decoder.

No script/rule results are stubbed. The only hooks are archive reads and the
collection-file write, whose requested index/value are recorded as output.
"""
import argparse,json,struct
from pathlib import Path
from initialization_probe import prepare,state,UNIT
from executable_probe import EDITION
from eex import parse

EXTRA=[(0x40c537,0x40c63a),(0x4077d0,0x407818),(0x407907,0x407a09),
 (0x407d78,0x407fe7),(0x40c6ea,0x40c7a9),(0x409540,0x409551),
 (0x41ddf0,0x41de01),(0x409600,0x409615),(0x40b922,0x40b940),
 (0x40b8b0,0x40b8c1),(0x416319,0x4166eb),(0x40fcb1,0x40fd4a),
 (0x4183f7,0x4184b2),(0x415d92,0x415f65),(0x41856a,0x418a0d),
 (0x412230,0x412988),(0x4129a4,0x412dba),(0x4184d1,0x4184eb),
 (0x418dd0,0x418e09),(0x426a3c,0x426a71),(0x426bd5,0x426c6a),
 (0x42abf0,0x42ac04),(0x42af50,0x42af68),(0x4728a0,0x4728b6),
 (0x449b27,0x449b6b),(0x412dd6,0x413010)]

def prepare_script(source):
    v=prepare(source);v.ranges+=EXTRA
    for n in range(104):
        v.write(0x800000,v.data[1][n*25:(n+1)*25]);v.call(0x40c537,[0x800000],0x4a1140+n*25)
    v.collection=[]
    def collection(vm):vm.collection.append([vm.arg(0),vm.arg(1)]);vm.return_from_hook()
    v.hooks[0x41a1b0]=collection
    return v

def script_call(v,handler,instruction,budget=1000000):
    v.write(0x820000,instruction[2:]);v.put32(0x492f90+8,0);v.put32(0x492f90+12,0x820000)
    result=v.call(handler,[0x492f90],budget=budget)
    assert result in (0,1),f'handler result {result}'
    # A type-5 string advances the pointer across the bytes but only counts
    # its two-byte type tag in the numeric argument counter.
    expected_count=2 if handler in (0x413661,0x413c0e) else 10 if handler==0x413911 else len(instruction)-2
    assert v.u32(0x492f90+8)==expected_count,'argument decoder counter differs'
    assert v.u32(0x492f90+12)==0x820000+len(instruction)-2,'argument pointer did not reach instruction end'
    return result

def slots(v):
    result=[]
    for slot in range(115):
        b=v.read(0x492f90+0x2044+slot*64,64);uid=struct.unpack_from('<I',b)[0]
        if uid==65535:continue
        result.append(dict(slot=slot,id=uid,x=b[4],y=b[5],flags=struct.unpack_from('<I',b,8)[0],ai=b[12],target=struct.unpack_from('<H',b,14)[0],promotion_mode=b[16],level=b[17],equipment_overrides=list(struct.unpack_from('<III',b,20)),weapon_level=b[32],armor_level=b[33]))
    return result

def generate(source,return_vm=False):
    v=prepare_script(source);v.call(0x40c28c,[0,0,2]);raw=(source/'R_00.eex').read_bytes()
    before=state(v,0);script_call(v,0x416319,raw[0x5b:0x75]);equipment=dict(before=before,args=[25,0,0,0,0],after=state(v,0),collection=v.collection.copy())
    raw=(source/'S_00.eex').read_bytes();script=parse(source/'S_00.eex');steps=[]
    handlers={74:0x415d92,68:0x412230,69:0x4122f1,70:0x4125c5,71:0x4129a4,75:0x415e31,72:0x412dd6}
    def flatten(body):
        for i in body:
            if i['opcode']==1:
                assert i['test']['opcode']==2
                yield from flatten(i['body'])
            elif i['opcode'] not in (0,2):yield i
    for i in flatten(script['sections'][0]['events'][0]['body']):
        if i['opcode'] not in handlers:break
        script_call(v,handlers[i['opcode']],raw[i['offset']:i['end']])
        step={'offset':i['offset'],'opcode':i['opcode'],'args':[a['value'] for a in i['args']]}
        if i['opcode']==74:step['selected']=[v.u32(0x48b350),v.u32(0x48b354)]
        if i['opcode'] in (68,70,71,75,72):step['slots']=slots(v)
        if i['opcode']==69:step['configuration']={hex(p):list(v.read(p,n)) for p,n in [(0x494fd0,2),(0x496c94,12),(0x496e21,3),(0x496c9e,1)]}
        steps.append(step)
    result=dict(executable_sha256=EDITION,hooks={'0x41b51e':'Data.e5 archive read as in initialization probe','0x41a1b0':'Collection-file write: records requested byte index/value; no host file opened'},equipment=[equipment],deployment=steps)
    return (result,v) if return_vm else result

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_deployment.json'));a=p.parse_args()
    result=generate(a.source);a.output.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n');print('Captured',len(result['deployment']),'deployment instructions and opening equipment handler.')
