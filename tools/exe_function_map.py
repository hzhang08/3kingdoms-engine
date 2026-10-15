#!/usr/bin/env python3
"""Read-only PE/Capstone inventory; static candidates are not recovered symbols.

Analyst annotations and existing probes establish roles. Recursive traversal
establishes spans only for reachable instructions. Indirect control flow,
exception paths and shared blocks can make a span incomplete. Probe allowlists
are reported separately and NEVER equated with function extents.
"""
import argparse,ast,hashlib,json,re
from collections import defaultdict
from pathlib import Path
import capstone,pefile
from capstone.x86 import X86_OP_IMM,X86_OP_MEM
from executable_probe import EDITION
ROOT=Path(__file__).resolve().parents[1]
INTERNAL_REGIONS=[
 (0x4259ac,0x4259c3,'Tactic completion tail within425627','tools/tactic_plan_probe.py'),
 (0x449fc7,0x44a0c1,'Environment selector internal block','tools/environment_probe.py'),
 (0x44d8a0,0x44d94e,'Initial battle unit cleanup loop within44d427','tools/battle_entry_probe.py'),
 (0x44e1d3,0x44e1e1,'Player input-loop observation point (short instruction excerpt only)','tools/player_phase_probe.py'),
 (0x44ef07,0x44ef16,'Turn-limit caption branch (short instruction excerpt only)','src/engine.cpp'),
 (0x4343c5,0x4343d4,'Terrain-lesson continuation observation point (short instruction excerpt only)','tools/tutorial_terrain_probe.py')]

def inventory(source):
 raw=source.read_bytes()
 if hashlib.sha256(raw).hexdigest()!=EDITION:raise ValueError('unsupported executable edition')
 pe=pefile.PE(data=raw);base=pe.OPTIONAL_HEADER.ImageBase;image=pe.get_memory_mapped_image()
 sections=[dict(name=s.Name.decode().strip('\0'),va=base+s.VirtualAddress,rva=s.VirtualAddress,virtual_size=s.Misc_VirtualSize,file_start=s.PointerToRawData,file_end=s.PointerToRawData+s.SizeOfRawData,raw_size=s.SizeOfRawData,flags=s.Characteristics) for s in pe.sections]
 code=next(s for s in sections if s['name']=='.text');lo=code['va'];hi=lo+code['virtual_size']
 def file_offset(va):
  for s in sections:
   if s['va']<=va<s['va']+s['raw_size']:return s['file_start']+va-s['va']
  return None
 cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);cs.detail=True;cs.skipdata=True
 ins={i.address:i for i in cs.disasm(raw[code['file_start']:code['file_start']+code['virtual_size']],lo) if i.id}
 seed=defaultdict(set);seed[base+pe.OPTIONAL_HEADER.AddressOfEntryPoint].add('PE entry point')
 callers=defaultdict(set);calls={}
 for a,i in ins.items():
  if i.mnemonic=='call' and i.operands and i.operands[0].type==X86_OP_IMM:
   target=i.operands[0].imm
   if lo<=target<hi:seed[target].add('linear-disassembly direct-call target');callers[target].add(a)
  if bytes(i.bytes)==b'\x55' and image[a-base:a-base+3]==b'\x55\x8b\xec':seed[a].add('55 8B EC prologue pattern')
 annotations=json.loads((ROOT/'tools/exe_map_annotations.json').read_text())
 for a,label in annotations.items():
  if label.get('kind')!='internal block':seed[int(a,16)].add('analyst-annotated entry')
 probe_refs=defaultdict(list);probe_ranges=[];probe_calls=defaultdict(list);probe_hooks=defaultdict(list)
 for p in sorted((ROOT/'tools').glob('*probe.py')):
  text=p.read_text();tree=ast.parse(text);doc=ast.get_docstring(tree) or ''
  for lineno,line in enumerate(text.splitlines(),1):
   for a in set(re.findall(r'0x[0-9a-fA-F]{6,8}',line)):
    n=int(a,16)
    if lo<=n<hi:probe_refs[n].append(f'{p.relative_to(ROOT)}:{lineno}')
  for n in ast.walk(tree):
   if isinstance(n,ast.Tuple) and len(n.elts)==2 and all(isinstance(e,ast.Constant) and isinstance(e.value,int) for e in n.elts):
    a,b=[e.value for e in n.elts]
    if lo<=a<b<=hi:probe_ranges.append(dict(start=a,end=b,source=f'{p.relative_to(ROOT)}:{n.lineno}',scope='numeric range used in probe; not asserted to be a function boundary'))
   if isinstance(n,ast.Call) and isinstance(n.func,ast.Attribute) and n.func.attr=='call' and n.args and isinstance(n.args[0],ast.Constant):
    a=n.args[0].value
    if isinstance(a,int) and lo<=a<hi:probe_calls[a].append(f'{p.relative_to(ROOT)}:{n.lineno}');seed[a].add('explicit original-code probe call')
   if isinstance(n,ast.Dict):
    for k,v in zip(n.keys,n.values):
     if isinstance(k,ast.Constant) and isinstance(k.value,int) and lo<=k.value<hi and (isinstance(v,(ast.Name,ast.Lambda,ast.Call))):probe_hooks[k.value].append(f'{p.relative_to(ROOT)}:{k.lineno}')
 # A probe may start at an interior block after supplying the caller frame.
 # It must not split a real function or become an independently callable entry.
 for a,_,_,_ in INTERNAL_REGIONS:seed.pop(a,None)
 def decode(a):
  if not lo<=a<hi:return None
  if a in ins:return ins[a]
  decoded=next(cs.disasm(image[a-base:a-base+16],a,count=1),None)
  return decoded if decoded and decoded.id else None
 imports={i.address:dict(dll=d.dll.decode(),name=i.name.decode() if i.name else '#'+str(i.ordinal)) for d in pe.DIRECTORY_ENTRY_IMPORT for i in d.imports}
 def trace(start):
  todo=[start];seen={};edges=[];external=[];terminals=[];issues=[];api=[]
  while todo and len(seen)<12000:
   a=todo.pop()
   if a in seen:continue
   if a!=start and a in seed:
    issues.append(dict(address=a,reason='flow reaches another candidate entry'));continue
   i=decode(a)
   if i is None:issues.append(dict(address=a,reason='outside code or undecodable'));continue
   seen[a]=i;nexta=a+i.size
   if i.group(capstone.CS_GRP_RET) or i.mnemonic in ('iret','iretd'):
    terminals.append(dict(address=a,kind='return'));continue
   if i.mnemonic in ('int3','ud2','hlt'):
    issues.append(dict(address=a,reason='trap or stop instruction'));continue
   if i.group(capstone.CS_GRP_CALL):
    if i.operands[0].type==X86_OP_IMM:external.append(i.operands[0].imm)
    elif i.operands[0].type==X86_OP_MEM and i.operands[0].mem.base==0 and i.operands[0].mem.index==0:
     ptr=i.operands[0].mem.disp
     if ptr in imports:api.append(dict(callsite=a,iat=ptr,**imports[ptr]))
   if i.group(capstone.CS_GRP_JUMP):
    if i.operands[0].type==X86_OP_IMM:
     target=i.operands[0].imm;edges.append(dict(address=a,target=target))
     if target!=start and target in seed:
      terminals.append(dict(address=a,kind='transfer to candidate entry',target=target))
      if i.mnemonic!='jmp':issues.append(dict(address=a,reason='conditional cross-entry transfer'))
     else:todo.append(target)
    elif i.operands[0].type==X86_OP_MEM and i.operands[0].mem.base==0 and i.operands[0].mem.index==0 and i.operands[0].mem.disp in imports:
     ptr=i.operands[0].mem.disp;api.append(dict(callsite=a,iat=ptr,kind='tail jump',**imports[ptr]));terminals.append(dict(address=a,kind='import tail jump'))
    else:issues.append(dict(address=a,reason='indirect jump: targets not recovered'))
    if i.mnemonic!='jmp':todo.append(nexta)
   else:todo.append(nexta)
  if todo:issues.append(dict(address=start,reason='instruction traversal budget'))
  end=max((a+i.size for a,i in seen.items()),default=start)
  ordered=sorted(seen);blocks=[]
  for a in ordered:
   e=a+seen[a].size
   if blocks and blocks[-1][1]==a:blocks[-1][1]=e
   else:blocks.append([a,e])
  strings=[]
  for i in seen.values():
   for operand in i.operands:
    ptr=operand.imm if operand.type==X86_OP_IMM else operand.mem.disp if operand.type==X86_OP_MEM and operand.mem.base==0 and operand.mem.index==0 else 0
    # Read-only literal extraction. Restrict to file-backed data/rdata.
    sec=next((s for s in sections if s['name'] in ('.data','.rdata') and s['va']<=ptr<s['va']+s['raw_size']),None)
    if not sec:continue
    val=image[ptr-base:ptr-base+200].split(b'\0')[0]
    if len(val)<4:continue
    try:text=val.decode('gbk')
    except UnicodeDecodeError:continue
    if all(ch.isprintable() or ch in '\r\n\t' for ch in text) and any(ch.isalpha() for ch in text):strings.append(dict(address=ptr,text=text))
  unique={r['address']:r for r in strings}
  return dict(start=start,end=end,file_start=file_offset(start),file_end=file_offset(end-1)+1 if end>start and file_offset(end-1) is not None else None,rva=start-base,instruction_count=len(seen),instruction_bytes=sum(i.size for i in seen.values()),blocks=blocks,issues=issues,terminals=terminals,calls=sorted(set(external)),imports=api,strings=list(unique.values())[:24],first_instructions=[f'{a:08x} {seen[a].mnemonic} {seen[a].op_str}' for a in ordered[:30]])
 rows=[]
 for a in sorted(seed):
  r=trace(a);r['seed_evidence']=sorted(seed[a]);r['direct_call_sites']=sorted(callers[a]);r['probe_references']=probe_refs[a];r['probe_calls']=probe_calls[a];r['possible_probe_hooks']=probe_hooks[a];r['annotation']=annotations.get(hex(a));rows.append(r)
 resources=[]
 if hasattr(pe,'DIRECTORY_ENTRY_RESOURCE'):
  for typ in pe.DIRECTORY_ENTRY_RESOURCE.entries:
   for name in typ.directory.entries:
    for lang in name.directory.entries:
     d=lang.data.struct;off=pe.get_offset_from_rva(d.OffsetToData);resources.append(dict(type=str(typ.name) if typ.name else pefile.RESOURCE_TYPE.get(typ.id,str(typ.id)),id=str(name.name) if name.name else name.id,language=lang.id,rva=d.OffsetToData,file_start=off,file_end=off+d.Size,bytes=d.Size,sha256=hashlib.sha256(raw[off:off+d.Size]).hexdigest()))
 internal_regions=[dict(start=a,end=b,file_start=file_offset(a),file_end=file_offset(b-1)+1,name=name,evidence=evidence,annotation=annotations.get(hex(a))) for a,b,name,evidence in INTERNAL_REGIONS]
 return dict(source=str(source),sha256=EDITION,file_bytes=len(raw),image_base=base,entry_va=base+pe.OPTIONAL_HEADER.AddressOfEntryPoint,entry_file=file_offset(base+pe.OPTIONAL_HEADER.AddressOfEntryPoint),size_of_headers=pe.OPTIONAL_HEADER.SizeOfHeaders,has_exports=hasattr(pe,'DIRECTORY_ENTRY_EXPORT'),has_debug=hasattr(pe,'DIRECTORY_ENTRY_DEBUG'),sections=sections,imports=[dict(iat=ptr,file_offset=file_offset(ptr),**val) for ptr,val in imports.items()],resources=resources,functions=rows,internal_regions=internal_regions,probe_ranges=probe_ranges)

if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz/Ekd5.exe'));p.add_argument('--out',type=Path,default=ROOT/'output/reverse-engineering/Ekd5-function-inventory.json');a=p.parse_args();r=inventory(a.source);a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(r,ensure_ascii=False,indent=2)+'\n');print(json.dumps({'candidates':len(r['functions']),'annotated':sum(f['annotation'] is not None for f in r['functions']),'resources':len(r['resources']),'output':str(a.out)}))
