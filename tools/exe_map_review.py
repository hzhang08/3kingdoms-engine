#!/usr/bin/env python3
"""Prepare immutable, bounded JEV review packets. Does not send requests.

The existing jev_decide.py client sends an explicitly chosen packet and logs
the result. JEV labels are advisory; they do not establish binary semantics.
"""
import argparse,ast,json
from pathlib import Path
import capstone,pefile
ROOT=Path(__file__).resolve().parents[1]

def prepare(first_record=20,only_new=False):
    data=json.loads((ROOT/'output/reverse-engineering/Ekd5-function-inventory.json').read_text())
    image=pefile.PE(data=Path(data['source']).read_bytes()).get_memory_mapped_image()
    cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    rows=[f for f in data['functions'] if f['annotation']]
    if only_new:
        reviewed=set()
        for path in (ROOT/'docs/evidence/jev').glob('0[234][0-9]-exe-map-*.request.json'):
            reviewed.update(json.loads(path.read_text())['questions'])
        rows=[f for f in rows if f"entry_{f['start']:08x}" not in reviewed]
    for batch in range((len(rows)+9)//10):
        entries={};questions={}
        for f in rows[batch*10:batch*10+10]:
            key=f"entry_{f['start']:08x}";assembly=[]
            for a,b in f['blocks']:
                assembly += [f'{i.address:08x} {i.mnemonic} {i.op_str}' for i in cs.disasm(image[a-data['image_base']:b-data['image_base']],a)]
            if len(assembly)>100:assembly=assembly[:65]+['... middle omitted; do not infer full semantics ...']+assembly[-35:]
            evidence=[]
            path=ROOT/f['annotation']['evidence']
            if path.exists() and path.suffix=='.py':
                text=path.read_text();lines=text.splitlines()
                evidence.append(ast.get_docstring(ast.parse(text)))
                indices=set()
                # Exact entry references, including any actual call or hook.
                for n,line in enumerate(lines):
                    if f'{f["start"]:x}' in line.lower():indices.update(range(max(0,n-3),min(len(lines),n+5)))
                evidence.append('\n'.join(f'{n+1}: {lines[n]}' for n in sorted(indices))[:8500])
            entries[key]={'draft_annotation':f['annotation'],'reachable_span':[hex(f['start']),hex(f['end'])],
                'span_caveats':f['issues'],'seed_evidence':f['seed_evidence'],
                'assembly_excerpt':assembly,'evidence_source':evidence,
                'explicit_probe_calls':f['probe_calls'],'possible_probe_hooks':f['possible_probe_hooks'],
                'literals':f['strings'][:5]}
            questions[key]={'type':'choice','instructions':f'Review only {key}. Compare draft name/scope with original assembly and probe source. Do not assume a probe allowlist or hook proves semantics.',
                'criteria':{'supported_scoped':'The proposed descriptive name is consistent with the supplied evidence when read within its stated limits. Not proof of complete function behavior.',
                    'too_broad':'The label or scope overclaims what supplied evidence establishes; narrow it or mark boundary/static only.',
                    'contradicted':'The proposed name is contradicted by the supplied assembly or probe source; correct it before publication.',
                    'unknown':'Evidence is insufficient to judge the proposed function role.'}}
        request={'model':'typesafe-ai/jev','state':{'task':'Advisory audit of a read-only Ekd5.exe function map; original executable evidence decides. No code execution requested.',
            'edition_sha256':data['sha256'],'rules':'Names are analyst descriptions, not original symbols. Spans are reachable instruction envelopes with exclusive ends, not guaranteed complete function extents. Indirect jumps, callbacks and exception paths can leave untraversed code. A hook establishes only observed caller requests. Early docs may be superseded by the supplied probe. Use unknown rather than guessing.',
            'entries':entries},'questions':questions}
        dest=ROOT/f'docs/evidence/jev/{first_record+batch:03d}-exe-map-{first_record+batch:02d}.request.json'
        with dest.open('x') as out:out.write(json.dumps(request,ensure_ascii=False,indent=2)+'\n')
        print(dest.relative_to(ROOT))

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--first-record',type=int,default=20);p.add_argument('--only-new',action='store_true');a=p.parse_args();prepare(a.first_record,a.only_new)
