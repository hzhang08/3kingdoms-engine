from pathlib import Path
import json,collections
root=Path(__file__).resolve().parents[1]
summary={}
for name in ['R_00','S_00']:
 script=json.loads((root/'assets/generated/scripts'/f'{name}.json').read_text());counts=collections.Counter();lines=[]
 def walk(items,depth=0):
  for i in items:
   counts[i['opcode']]+=1
   args=json.dumps([a['value'] for a in i['args']],ensure_ascii=False)
   lines.append(f'{i["offset"]:04x}  '+('  '*depth)+f'{i["opcode"]:3d} (0x{i["opcode"]:02x}) {args}')
   if 'body'in i:walk([i['test']],depth+1);walk(i['body'],depth+2)
 for number,section in enumerate(script['sections']):
  lines.append(f'\nSECTION {number} at 0x{section["offset"]:04x}')
  for event in section['events']:
   lines.append(f'  EVENT at 0x{event["offset"]:04x} through 0x{event["end"]:04x}')
   walk(event['conditions'],1);walk(event['body'],1)
 (root/'docs'/f'{name}-listing.txt').write_text('\n'.join(lines)+'\n')
 summary[name]={'instructions_including_structure':sum(counts.values()),'opcodes':dict(sorted(counts.items())),'events':[len(s['events']) for s in script['sections']]}
(root/'docs/instruction-inventory.json').write_text(json.dumps(summary,indent=2)+'\n')
print({name:data['instructions_including_structure'] for name,data in summary.items()})
