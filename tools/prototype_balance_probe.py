#!/usr/bin/env python3
"""Read-only original x86 deployment/construction oracle for fixed prototype parties."""
import json,struct
from pathlib import Path
from deployment_probe import prepare_script,script_call,slots
from initialization_probe import state,UNIT
from battle_initialization_probe import RANGES
from controller_initialization_probe import initialize
from eex import parse
from prototype_scenarios import walk,PARTY,EDITION
source=Path('/Users/henryzhang/Downloads/sgzccz')
result={'executable_sha256':EDITION,'method':'original handlers and 0x44cc85; declared standalone party levels; stage zero NPC promotion policy','scenarios':[]}
for n in range(4):
 v=prepare_script(source);party=[0] if n==0 else PARTY;level=[3,10,10,20][n]
 for uid in party:
  v.call(0x40c28c,[uid,0,2]);v.write(UNIT+uid*72+0x2c,bytes([level]))
 v.write(0x48b350,struct.pack('<15I',*party,*([65535]*(15-len(party)))))
 raw=(source/f'S_{n:02}.eex').read_bytes();executed=[]
 handlers={68:0x412230,69:0x4122f1,70:0x4125c5,71:0x4129a4,75:0x415e31,72:0x412dd6}
 for i in walk(parse(source/f'S_{n:02}.eex')['sections'][0]['events'][0]['body']):
  if i['opcode'] in handlers:
   script_call(v,handlers[i['opcode']],raw[i['offset']:i['end']]);executed.append({'offset':i['offset'],'opcode':i['opcode']})
  elif i['opcode'] not in (0,1,2,74):break
 descriptors=slots(v)
 v.ranges+=RANGES;initialize(v)
 t=json.loads(Path('assets/generated/terrain.json' if n==0 else f'assets/expansion/terrain-{n}.json').read_text())
 v.put32(0x4aeb38+4,0x2580);v.put32(0x4aeb38+12,0x818000);v.write(0x4aeb38+16,b'\x01')
 v.write(0x818640,bytes(t['cells']));v.write(0x818c80,b'\xff'*1600);v.write(0x819f40,b'\xff'*1600)
 v.write(0x4b422c,bytes([t['width'],t['height']]));v.write(0x4ae4e0,v.data[4])
 for slot in range(115):v.write(0x4b2c50+slot*36+4,bytes([slot]))
 def omit(vm):vm.return_from_hook()
 for addr in (0x43fcc9,0x479a01,0x41ee53,0x47fcfd):v.hooks[addr]=omit
 v.ranges += [(0x43fd34,0x43fe2c),(0x425af0,0x425b21),(0x442718,0x44274c)]
 v.call(0x44cc85,this=0x4b5df0)
 units=[state(v,d['id']) for d in descriptors if d['slot']>=15]
 result['scenarios'].append(dict(scenario=n,party=party,party_level=level,instructions=executed,descriptors=descriptors,npcs=units))
 print(n,'NPCs',len(units),'levels',sorted(set(x['level'] for x in units)))
result['hooks']={'0x41b51e':'decoded Data archive','0x41a1b0':'collection write captured','0x43fcc9':'tile draw omitted','0x479a01':'Win32 update omitted','0x41ee53':'sprite transform omitted','0x47fcfd':'pixel copy omitted'}
Path('docs/evidence/balance/original-scenario-initialization.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
