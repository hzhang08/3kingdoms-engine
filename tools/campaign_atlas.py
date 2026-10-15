#!/usr/bin/env python3
"""Read-only original-campaign analysis; tolerant text display, strict EEX structure."""
from pathlib import Path
import json,re,hashlib,collections
from eex import Reader
ROOT=Path(__file__).resolve().parents[1]
SOURCE=Path('/Users/henryzhang/Downloads/sgzccz')
OUT=ROOT/'output/campaign-atlas';OUT.mkdir(parents=True,exist_ok=True)
ANOMALIES=[]
class AtlasReader(Reader):
 def argument(self,p,expected=None):
  if self.u16(p)!=5:return super().argument(p,expected)
  if expected is not None and expected!=5:self.fail(p,'unexpected string type')
  end=self.data.find(b'\0',p+2)
  if end<0:self.fail(p,'unterminated string')
  raw=self.data[p+2:end]
  try:text=raw.decode('gbk')
  except UnicodeDecodeError:
   # A140 is a spacing glyph in this edition. Only display normalization;
   # never rewrite bytes, change offsets, or recover unknown instructions.
   changed=raw.replace(b'\xa1\x40',b'\xa1\xa1')
   try:text=changed.decode('gbk');note='A140 displayed as U+3000; glyph mapping not runtime-verified'
   except UnicodeDecodeError:text=changed.decode('gbk',errors='backslashreplace');note='unresolved bytes escaped in display'
   ANOMALIES.append({'file':self.name,'offset':p+2,'raw_hex':raw.hex(),'display':text,'note':note})
  return {'offset':p,'type':5,'value':text,'raw_hex':raw.hex()},end+1

def values(i):return [a['value'] for a in i['args']]
def walk(xs,parents=()):
 for i in xs:
  yield i,parents
  if 'body' in i:yield from walk(i['body'],parents+(i['test'],))
def events(sc):
 for sn,sec in enumerate(sc['sections']):
  for en,e in enumerate(sec['events']):
   for i,parents in walk(e['body']):yield sn,en,e,i,parents

def save(name,v): (OUT/name).write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n')
def main():
 scripts={};hashes={}
 for p in sorted(SOURCE.glob('*.eex')):
  raw=p.read_bytes();scripts[p.stem]=AtlasReader(raw,p.name).parse();hashes[p.name]=hashlib.sha256(raw).hexdigest()
 save('scripts.json',scripts);save('text-anomalies.json',ANOMALIES)
 save('sources.json',{'method':'strict full-byte structural parse; static control-flow and roster extraction; not all-route runtime replay','files':hashes,'executable_sha256':hashlib.sha256((SOURCE/'Ekd5.exe').read_bytes()).hexdigest()})
 style=Path(ROOT/'.cache/research/style-original.txt').read_bytes().decode('gbk');names={}
 for c in re.split(r'\n\s*\n',style.strip()):
  l=c.splitlines();names[int(l[0])]=l[1]
 save('opcode-names.json',names)
 lines=[];jump=[]
 for f,sc in scripts.items():
  ls=[]
  for sn,sec in enumerate(sc['sections']):
   ls.append(f'\nSECTION {sn}')
   for en,e in enumerate(sec['events']):
    ls.append(f'EVENT {en} 0x{e["offset"]:04x} CONDITIONS '+str([(i['opcode'],values(i)) for i in e['conditions']]))
    for i,pa in walk(e['body']):
     line=f'{i["offset"]:04x} '+('  '*len(pa))+f'{i["opcode"]} {names.get(i["opcode"],"?")} '+json.dumps(values(i),ensure_ascii=False)
     ls.append(line)
     if 'test' in i:
      t=i['test'];ls.append(f'{t["offset"]:04x} '+('  '*(len(pa)+1))+'IF '+str(t['opcode'])+' '+json.dumps(values(t),ensure_ascii=False))
     if i['opcode']==17:
      n=values(i)[0];target=f'R_{n//2+1:02}' if n%2 else f'S_{n//2:02}'
      jump.append({'file':f,'offset':i['offset'],'target':target,'event_conditions':e['conditions'],'parents':pa,'previous_lines':ls[-14:]})
  (OUT/'listings').mkdir(exist_ok=True);(OUT/'listings'/f'{f}.txt').write_text('\n'.join(ls))
 save('jumps.json',jump)
 for x in jump:
  lines.append(f'\n{x["file"]}:{x["offset"]:04x} -> {x["target"]}\n'+'\n'.join(x['previous_lines']))
 (OUT/'jump-contexts.txt').write_text('\n'.join(lines))
 print('Scripts',len(scripts),'string anomalies',len(ANOMALIES),'jumps',len(jump))
if __name__=='__main__':main()
