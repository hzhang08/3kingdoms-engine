#!/usr/bin/env python3
"""Bounded original x86 roster-table oracle, with no Windows or host game I/O."""
from pathlib import Path
import json,collections,struct
from deployment_probe import prepare_script,script_call,slots
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'output/campaign-atlas';SRC=Path('/Users/henryzhang/Downloads/sgzccz')
s=json.loads((OUT/'scripts.json').read_text());b=json.loads((OUT/'battles.json').read_text())
def walk(xs):
 for i in xs:
  yield i
  yield from walk(i.get('body',[]))
v=prepare_script(SRC);v.call(0x40c28c,[0,0,2]);v.write(0x48b350,struct.pack('<15I',0,*([65535]*14)));results=[]
for battle in b:
 f=battle['script'];raw=(SRC/(f+'.eex')).read_bytes();checked=[]
 for sec in s[f]['sections']:
  for e in sec['events']:
   for i in walk(e['body']):
    op=i['opcode']
    if op not in(68,70,71):continue
    script_call(v,{68:0x412230,70:0x4125c5,71:0x4129a4}[op],raw[i['offset']:i['end']])
    if op in(70,71):
     actual={r['slot']:r for r in slots(v)}
     expected=[r for r in battle['roster']if r['ref'].endswith(f'0x{i["offset"]:04x}')]
     for r in expected:
      a=actual[r['slot']]
      assert a['id']==r['id'],(f,r,a)
      assert [a['x'],a['y']]==r['xy'],(f,r,a)
      assert bool(a['flags']&4)==r['hidden'],(f,r,a)
     checked.append({'opcode':op,'offset':i['offset'],'rows_checked':len(expected)})
 results.append({'battle':f,'checks':checked})
 print(f,'OK',sum(c['rows_checked']for c in checked),flush=True)
(OUT/'original-table-verification.json').write_text(json.dumps({'method':'Original CPU-only opcode68,70,71 handlers; checks exact IDs, slots, XY and hidden bit; does not execute campaign events','executable_sha256':v.executable_sha256,'hooks':{'0x41b51e':'read-only decoded Data.e5 blocks','0x41a1b0':'collection write captured; no host file write'},'results':results},indent=2))

# Check route branch boundary using the original R32 operands and comparison code.
v.ranges += [(0x4118d4,0x4119e7),(0x418c30,0x418c41)]
route_cases=[]
for e in s['R_32']['sections'][0]['events']:
 for i in e['conditions']:
  if i['opcode']!=55:continue
  raw=(SRC/'R_32.eex').read_bytes()[i['offset']:i['end']]
  for score in (0,15,16,50,85,100):
   v.write(0x4b0782,bytes([score]));r=script_call(v,0x4118d4,raw);args=[a['value']for a in i['args']]
   assert bool(r)==(score<16 if args[2]==1 else score>=16)
   route_cases.append({'source_offset':hex(i['offset']),'raw_args':args,'route_value':score,'passes':bool(r)})
(OUT/'route-threshold-verification.json').write_text(json.dumps({'method':'bounded original opcode55 handler with original R32 argument bytes','executable_sha256':v.executable_sha256,'cases':route_cases},indent=2))
print('Route threshold cases passed:',len(route_cases))
