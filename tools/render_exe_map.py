#!/usr/bin/env python3
"""Validate and render the read-only, edition-specific EXE inventory as UTF-8.

No emulator or Windows process runs here. All supplied file ranges are checked
against the source PE before the report is written. Model reviews are evidence
about our labels, never evidence that the original program executed correctly.
"""
import collections,datetime,hashlib,json
from decimal import Decimal
from pathlib import Path
import capstone,pefile
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'output/reverse-engineering'

def hx(n):return '0x'+format(n,'08X') if n is not None else 'NONE'
def span(a,b):return f'[{hx(a)}, {hx(b)})'
def merged(ranges):
    result=[]
    for a,b in sorted(ranges):
        if result and a<=result[-1][1]:result[-1][1]=max(b,result[-1][1])
        else:result.append([a,b])
    return result
def gaps(ranges,start,end):
    cursor=start;result=[]
    for a,b in merged(ranges):
        if cursor<a:result.append([cursor,a])
        cursor=max(cursor,b)
    if cursor<end:result.append([cursor,end])
    return result

def render():
    p=json.loads((OUT/'Ekd5-function-inventory.json').read_text())
    raw=Path(p['source']).read_bytes();assert hashlib.sha256(raw).hexdigest()==p['sha256']
    pe=pefile.PE(data=raw);image=pe.get_memory_mapped_image();base=p['image_base']
    cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    def offset(va):
        for s in p['sections']:
            if s['va']<=va<s['va']+s['raw_size']:return s['file_start']+va-s['va']
        return None
    def file_span(a,b):return span(offset(a),offset(b-1)+1)
    validated_blocks=0;insn_count=0
    for f in p['functions']:
        assert f['file_start']==offset(f['start']) and f['file_end']==offset(f['end']-1)+1
        assert f['end']==max(b for a,b in f['blocks']) and f['start']==min(a for a,b in f['blocks'])
        total=0
        for a,b in f['blocks']:
            pos=offset(a);end=offset(b-1)+1
            assert 0<=pos<end<=len(raw) and end-pos==b-a
            assert raw[pos:end]==image[a-base:b-base]
            decoded=list(cs.disasm(raw[pos:end],a));assert sum(i.size for i in decoded)==b-a
            total+=len(decoded);validated_blocks+=1
        assert total==f['instruction_count'];insn_count+=total
    for r in p['internal_regions']:
        decoded=list(cs.disasm(raw[r['file_start']:r['file_end']],r['start']))
        assert sum(i.size for i in decoded)==r['end']-r['start'],'internal excerpt splits an instruction'
    for r in p['resources']:
        assert r['file_end']-r['file_start']==r['bytes']
        assert hashlib.sha256(raw[r['file_start']:r['file_end']]).hexdigest()==r['sha256']
    for i in p['imports']:assert i['file_offset']==offset(i['iat'])
    curated=[f for f in p['functions'] if f['annotation']]
    by_start={f['start']:f for f in p['functions']}
    reviews=[];by_entry=collections.defaultdict(list)
    for path in sorted((ROOT/'docs/evidence/jev').glob('*exe-map*.record.json')):
        d=json.loads(path.read_text());assert d.get('http_status')==200
        answers=d['response']['answers'];cost=Decimal(d['response'].get('providerMetadata',{}).get('gateway',{}).get('cost','0'))
        review=dict(record=str(path.relative_to(ROOT)),number=int(path.name[:3]),answers=answers,cost=str(cost),questions=len(d['request']['questions']))
        assert set(answers)==set(d['request']['questions'])
        reviews.append(review)
        for key,answer in answers.items():
            if key.startswith('entry_'):by_entry[int(key[6:],16)].append((review['number'],answer['choice']))
    assert len(reviews)==29 and sum(r['questions'] for r in reviews)==285
    # The historical review covered the original map. Later labels may instead
    # have independently executed fixtures; never imply JEV reviewed those.
    unreviewed=[f['start'] for f in curated if f['start'] not in by_entry]
    all_blocks=[b for f in p['functions'] for b in f['blocks']]
    named_blocks=[b for f in curated for b in f['blocks']]
    named_bytes=sum(b-a for a,b in merged(named_blocks));all_bytes=sum(b-a for a,b in merged(all_blocks))
    code=p['sections'][0];unvisited=gaps(all_blocks,code['va'],code['va']+code['virtual_size'])
    partial=[f for f in curated if f['issues']]
    unknown=[f for f in p['functions'] if not f['annotation']]
    lines=[]
    def emit(*args):lines.extend(str(arg) for arg in args)
    def title(s):emit('',s,'='*len(s))
    emit('Ekd5.exe — binary feature/function map',
         '三国志曹操传：可执行文件功能与地址索引',
         'Generated: '+datetime.datetime.now(datetime.timezone.utc).isoformat(),
         'Format: UTF-8 plain text. This is a reverse-engineering outline, not recovered original C++ source.')
    title('1. Identity, scope and address notation')
    emit('Source (read only): '+p['source'],'SHA-256: '+p['sha256'],
         f'File size: {p["file_bytes"]} bytes ({hx(p["file_bytes"])}). Machine: PE32 / x86.',
         f'Preferred image base: {hx(base)}. Entry VA: {hx(p["entry_va"])}. Entry file offset: {hx(p["entry_file"])}.',
         'No export symbol table or PE debug directory was found. All game-function names below are analyst descriptions.',
         '',
         'Every range is [START, END): START is included; END is excluded. Hex numbers are byte addresses.',
         'FILE = byte offset in Ekd5.exe. VA = preferred in-memory virtual address. RVA = VA - image base.',
         'For file-backed section bytes: FILE = section.raw_offset + (VA - section.VA).',
         'Do not subtract only the image base to get a file offset. Example: VA0x0043BC4B -> FILE0x0003B04B.',
         'A loaded address may have no source file bytes: the zero-filled .data tail is an example.',
         '',
         f'{len(curated)} named function candidates; {len(p["internal_regions"])} separately identified internal regions;',
         f'{len(p["functions"])} total static candidates, including {len(unknown)} without a manually assigned feature name.',
         f'{len(partial)} named candidates have unresolved control-flow edges. Some other entries contain untraced exception paths.',
         f'{len(p["resources"])} embedded resource leaves and {len(p["imports"])} imported API entries are indexed.',
         '',
         'A FUNCTION RANGE is the envelope of statically reached instruction blocks, not a recovered symbol extent.',
         'Direct calls are recorded but their callee bodies are not absorbed into the caller. Indirect switch branches,',
         'callback bodies, exceptional paths and shared code can be missing. Gaps within an envelope are not proven code.',
         'The precise visited file blocks are listed separately. PARTIAL warns about an explicit unresolved edge.',
         'DIRECT-CFG means no unresolved edge in this limited traversal; it does not prove complete behavior or complete extent.',
         'The candidate index is heuristic: prologue patterns and linear-disassembly call targets can misidentify data.',
         'Do not equate the number of candidates, instruction coverage, or model approvals with percentage of game understood.',
         '',
         'Evidence levels:',
         '  observed subset: existing bounded original-code probes support the stated subset, with explicit host hooks.',
         '  boundary observations: caller arguments/requests were captured; callee internals remain incomplete.',
         '  static only: assembly, strings or API references support a descriptive role; execution is not established.',
         '  static + scoped probe: arithmetic/structure was inspected and appears in existing bounded probe paths.',
         '  static or boundary only: an unresolved/hooked region with limited interpretation.',
         'All levels apply to the written scope, never to the whole routine or full campaign.',
         'Existing probes were inspected, not all rerun for this documentation task. The checks here validate addresses and bytes.')
    title('2. Complete PE file layout')
    emit('FILE RANGE                     PREFERRED VA RANGE            SECTION / ROLE',
         f'{span(0,p["size_of_headers"])}  {span(base,base+p["size_of_headers"])}  DOS/PE headers and section table')
    roles={'.text':'machine code plus inline tables/alignment; not every byte is an instruction',
           '.rdata':'read-mostly constants, pointer tables and import address tables',
           '.data':'initialized globals and strings; larger virtual tail is zero-filled',
           '.rsrc':'Windows resource directory and embedded resource payloads',
           '.Silvana':'added import metadata, including AutoDLL.DLL!ASave; not the DLL implementation'}
    for s in p['sections']:
        emit(f'{span(s["file_start"],s["file_end"])}  {span(s["va"],s["va"]+s["virtual_size"])}  {s["name"]}: {roles[s["name"]]}',
             f'  raw={s["raw_size"]} bytes; virtual={s["virtual_size"]} bytes; PE flags={hx(s["flags"])}.')
        if s['virtual_size']>s['raw_size']:
            emit('  ZERO-FILLED, NO FILE OFFSET: VA '+span(s['va']+s['raw_size'],s['va']+s['virtual_size']))
        elif s['raw_size']>s['virtual_size']:
            emit('  Raw alignment bytes beyond virtual-size boundary: FILE '+span(s['file_start']+s['virtual_size'],s['file_end']))
    emit('The .Silvana section contains the current import descriptors and an ASave import slot at VA0x004CD014.',
         'The ASave implementation lives in an external DLL. Its behavior cannot be assigned to these metadata bytes.',
         'No overlay bytes remain after the last section in this edition.')
    title('3. Feature navigation (entry points; non-contiguous code)')
    groups=collections.defaultdict(list)
    for f in curated:groups[f['annotation']['feature']].append(f)
    for group,fs in sorted(groups.items()):
        emit(group+':')
        for f in fs:emit(f'  FILE {hx(f["file_start"])} / VA {hx(f["start"])} — {f["annotation"]["name"]}')
    title('4. Named function candidates, ordered by file offset')
    for f in curated:
        a=f['annotation'];boundary='PARTIAL' if f['issues'] else 'DIRECT-CFG'
        emit('',f'FILE {span(f["file_start"],f["file_end"])}  VA {span(f["start"],f["end"])}',
             f'  {a["feature"]} :: {a["name"]}',
             f'  Evidence: {a["semantic_status"]}; boundary: {boundary}; RVA={hx(f["rva"])}.',
             f'  Scope: {a["scope"]}',f'  Source: {a["evidence"]}',
             '  Visited FILE blocks: '+', '.join(file_span(x,y) for x,y in f['blocks'])+'.',
             f'  Decoded here: {f["instruction_count"]} instructions / {f["instruction_bytes"]} bytes.')
        if any('fs:' in i for i in f['first_instructions']):emit('  Exception frame detected: SEH-only continuations are not followed by this inventory.')
        if f['issues']:emit('  Unresolved: '+'; '.join(hx(i['address'])+' '+i['reason'] for i in f['issues'])+'.')
        elif not f['terminals']:emit('  No return/tail terminal found in traversed blocks; do not infer normal completion.')
        if f['calls']:
            known=[f'{hx(addr)}'+(' '+by_start[addr]['annotation']['name'] if addr in by_start and by_start[addr]['annotation'] else '') for addr in f['calls']]
            emit('  Direct callees (first12): '+'; '.join(known[:12])+(' ...' if len(known)>12 else ''))
        if f['imports']:emit('  Direct imported APIs: '+', '.join(sorted({i['dll']+'!'+i['name'] for i in f['imports']})))
        if f['strings']:emit('  Literal anchors (first3; GBK decode, heuristic): '+'; '.join(hx(s['address'])+' '+json.dumps(s['text'][:140],ensure_ascii=False) for s in f['strings'][:3]))
        emit('  JEV advisory reviews: '+(', '.join(f'{number:03d}={choice}' for number,choice in by_entry[f['start']]) or 'none; consult the stated source evidence')+'.')
    title('5. Internal blocks and observation points — not callable functions')
    for r in p['internal_regions']:
        emit(f'FILE {span(r["file_start"],r["file_end"])}  VA {span(r["start"],r["end"])}',
             '  '+r['name']+'. Evidence: '+r['evidence']+'.')
        if r['annotation']:emit('  '+r['annotation']['scope'])
    emit('Short excerpts identify locations only; their END is an instruction boundary, not an enclosing function end.',
         'Probe allowlists may group several functions or resume in a supplied caller frame. They are retained in the JSON',
         'under probe_ranges for traceability and were not used as function extents.')
    title('6. Selected in-memory state and data structures')
    globals_=[
      (0x48ecd4,4,'RNG state; original LCG','tools/executable_probe.py'),
      (0x4927f0,0x614,'Physical attack controller, observed layout','tools/strike_probe.py'),
      (0x492f90,None,'EEX reader/scenario state object; complete size not asserted','tools/deployment_probe.py'),
      (0x494fd4,115*64,'Deployment descriptors:115 slots x64 bytes','tools/deployment_probe.py'),
      (0x497770,None,'Quantized palette data; this row identifies base only','tools/combat_palette_probe.py'),
      (0x497af8,None,'Tactic controller; this row identifies base only','tools/tactic_plan_probe.py'),
      (0x4a1140,104*25,'Item records:104 x25 bytes','tools/deployment_probe.py'),
      (0x4a1b68,512*72,'Persistent character records:512 x72 bytes','tools/initialization_probe.py'),
      (0x4abfa0,53*27,'Class records:53 x27 bytes','tools/initialization_probe.py'),
      (0x4af4c0,68*70,'Tactic records:68 x70 bytes','tools/ai_decision_probe.py'),
      (0x4b1b28,4096,'Palette color-cube lookup:16 x16 x16','tools/combat_palette_probe.py'),
      (0x4b2c08,None,'Tutorial collection/branch flags; base only','tools/tutorial_probe.py'),
      (0x4b2c10,1,'Tutorial completion-route flag','tools/tutorial_probe.py'),
      (0x4b2c14,4,'Tutorial guide live-unit pointer','tools/tutorial_basic_probe.py'),
      (0x4b2c18,2,'Tutorial cursor coordinates','tools/tutorial_terrain_probe.py'),
      (0x4b2c50,115*36,'Live battle units:115 x36 bytes','tools/battle_initialization_probe.py'),
      (0x4b3d08,None,'Battle/phase controller; complete size not asserted','tools/phase_transition_probe.py'),
      (0x4b4250,None,'Camera/viewport/overlay state; complete size not asserted','tools/tutorial_cursor_probe.py')]
    for va,size,name,evidence in globals_:
        emit(f'VA {span(va,va+size) if size else hx(va)} | FILE {hx(offset(va)) if offset(va) is not None else "NONE (zero-filled runtime memory)"}',
             f'  {name}. Evidence: {evidence}.')
    emit('Runtime objects are populated from Data.e5 and scripts. Their memory addresses are not file offsets into those assets.',
         'R_00.eex/S_00.eex instruction offsets belong to separate files, not Ekd5.exe. Most story text/art is external.')
    title('7. Embedded Windows resources')
    emit('Exact leaf payload spans; tree/directory/alignment bytes remain covered by the .rsrc section layout.',
         'FILE RANGE                     TYPE / ID / LANGUAGE / BYTES')
    for r in p['resources']:emit(f'{span(r["file_start"],r["file_end"])}  {r["type"]} / {r["id"]} / {r["language"]} / {r["bytes"]}')
    title('8. Imported APIs — external implementations, not EXE function bodies')
    for i in p['imports']:emit(f'IAT FILE {hx(i["file_offset"])}  VA {hx(i["iat"])}  {i["dll"]}!{i["name"]}')
    title('9. Remaining candidate entries (unclassified unless an exact import thunk)')
    emit('FILE ENVELOPE                  VA ENVELOPE                    ROLE / INSTRUCTIONS / BOUNDARY',
         'These are heuristic candidates, not a recovered symbol table. Adjacent rows do not guarantee adjacent functions.')
    for f in unknown:
        thunk=f['instruction_count']==1 and bool(f['imports'])
        role=('Exact import tail-jump '+f['imports'][0]['dll']+'!'+f['imports'][0]['name']) if thunk else 'UNCLASSIFIED'
        notes='; '.join(sorted({i['reason'] for i in f['issues']})) if f['issues'] else 'direct CFG only'
        emit(f'{span(f["file_start"],f["file_end"])}  {span(f["start"],f["end"])}  {role}; {f["instruction_count"]} insn; {notes}')
    title('10. .text bytes not visited by any candidate traversal')
    emit(f'Named visited-byte union: {named_bytes} bytes. All-candidate visited-byte union: {all_bytes} bytes.',
         f'.text virtual-size bytes: {code["virtual_size"]}. These byte counts are not behavioral coverage.',
         'The following gaps may contain jump tables, alignment, unreachable code, missing functions, or mis-decoded regions.',
         'They are explicitly unclassified; no feature is assigned just from neighboring functions.')
    for a,b in unvisited:emit(f'FILE {file_span(a,b)}  VA {span(a,b)}  UNCLASSIFIED GAP ({b-a} bytes)')
    title('11. JEV delegation and analyst review')
    emit(f'The original map review made {len(reviews)} JEV API requests, all HTTP200, containing {sum(r["questions"] for r in reviews)} structured decisions.',
         'Named entries added without an entry-specific JEV review: '+(', '.join(hx(a) for a in unreviewed) or 'none')+'.',
         'Later advisory investigations are recorded separately in docs/JEV-DECISIONS.md; historical approvals do not certify changed scopes.',
         'Model: typesafe-ai/jev, through the existing authorized gateway client.',
         'Task-reported API cost: $'+str(sum(Decimal(r['cost']) for r in reviews))+' USD.',
         '020–041:219 draft entry labels/scopes.042:13 focused corrections.043–047:46 additional static entry labels.',
         '048:7 questions about offsets, zero-filled memory, range uncertainty, imports and evidence standards.',
         'These are advisory classifications of supplied evidence. JEV did not decompile the executable or run the game.',
         'The first pass missed real label errors, including swapped critical/double helpers and menu cancellation.',
         'Independent assembly/probe inspection corrected them;042 then agreed with all13 proposed corrections.',
         '048 abstained on the two address-arithmetic questions; PE parsing and independent byte checks establish those facts.',
         'Records preserve the original draft, responses, confidence and disagreements. No credential is included.',
         'Analyst adjudications and cumulative usage: docs/JEV-DECISIONS.md and docs/evidence/exe-map/review-adjudication.json.',
         '')
    for r in reviews:
        counts=collections.Counter(v['choice'] for v in r['answers'].values())
        emit(f'{r["number"]:03d}: {r["questions"]} questions; '+', '.join(f'{k}={v}' for k,v in sorted(counts.items()))+f'; cost=${r["cost"]}; {r["record"]}')
    title('12. Reproduction, validation and remaining unknowns')
    emit('Project: '+str(ROOT),
         'Dependencies are pinned in requirements.txt (pefile, capstone; existing probe tools also use Unicorn).',
         'From this project directory:',
         '  .venv/bin/python tools/exe_function_map.py --source /Users/henryzhang/Downloads/sgzccz/Ekd5.exe',
         '  .venv/bin/python tools/render_exe_map.py',
         'The first command is SHA-gated to this edition and reads the original executable without modifying it.',
         'The second validates PE file mappings, every listed visited block, resource hashes and imports, and reports review coverage.',
         'Neither command sends new JEV requests. tools/exe_map_review.py prepares optional review packets;',
         'tools/jev_decide.py sends a specifically selected packet and refuses to overwrite an earlier response.',
         '',
         'Supporting files:',
         '  output/reverse-engineering/Ekd5-function-inventory.json — exact visited blocks/calls/resources/metadata.',
         '  output/reverse-engineering/Ekd5-disassembly.txt — original instructions for named entries/internal excerpts.',
         '  output/reverse-engineering/Ekd5-map-validation.json — structural-check results and artifact hashes.',
         '  tools/exe_map_annotations.json — editable descriptive labels, evidence sources and scope limits.',
         '',
         f'Validation passed: {len(p["functions"])} candidate ranges, {validated_blocks} visited blocks, {insn_count} per-entry decoded instructions,',
         f'{len(p["resources"])} resource hashes and {len(p["imports"])} import mappings. Original SHA-256 unchanged.',
         'Counts of instructions include any code shared between candidates; they are not unique-code counts.',
         '',
         'Still unresolved: unclassified candidates; switch targets and exceptional paths not followed by this scanner;',
         'full player-command flow; general tactic/item behavior; complete victory/defeat/reward continuations;',
         'original save serialization and many later-campaign/Windows UI paths.',
         'This outline makes known locations reviewable. It does not claim a complete understanding of Ekd5.exe,',
         'a complete source-level decompilation, or completion of the gameplay acceptance checklist.')
    txt=OUT/'Ekd5-binary-feature-map.txt';txt.write_text('\n'.join(lines)+'\n')
    asm=['Ekd5.exe original x86 bytes/instructions — named entries and internal excerpts',
         'SHA256 '+p['sha256'],'End addresses are exclusive. Only reached blocks are shown; gaps are not disassembled as owned code.','']
    for f in curated:
        asm.extend(['',f'VA {span(f["start"],f["end"])} FILE {span(f["file_start"],f["file_end"])} — {f["annotation"]["name"]}'])
        for a,b in f['blocks']:
            asm.append('  visited block '+span(a,b))
            for i in cs.disasm(image[a-base:b-base],a):asm.append(f'{i.address:08X} FILE {offset(i.address):08X}  {bytes(i.bytes).hex():24s}  {i.mnemonic:8s} {i.op_str}')
    for r in p['internal_regions']:
        asm.extend(['','INTERNAL REGION '+r['name']])
        for i in cs.disasm(image[r['start']-base:r['end']-base],r['start']):asm.append(f'{i.address:08X} FILE {offset(i.address):08X}  {bytes(i.bytes).hex():24s}  {i.mnemonic:8s} {i.op_str}')
    (OUT/'Ekd5-disassembly.txt').write_text('\n'.join(asm)+'\n')
    for f in curated:
        for evidence in f['annotation']['evidence'].split('; '):
            if not evidence.startswith('PE.'):assert (ROOT/evidence).exists(),evidence
    for _,_,_,evidence in globals_:assert (ROOT/evidence).exists()
    result=dict(source_sha256=p['sha256'],source_unchanged=hashlib.sha256(Path(p['source']).read_bytes()).hexdigest()==p['sha256'],
        function_candidates=len(p['functions']),named_function_candidates=len(curated),unclassified_candidates=len(unknown),
        internal_regions=len(p['internal_regions']),named_candidates_with_unresolved_edges=len(partial),
        validated_instruction_blocks=validated_blocks,decoded_instructions_per_entry=insn_count,
        named_visited_byte_union=named_bytes,all_candidates_visited_byte_union=all_bytes,
        resources_checked=len(p['resources']),import_mappings_checked=len(p['imports']),
        jev_requests=len(reviews),jev_questions=sum(r['questions'] for r in reviews),entries_without_jev_review=unreviewed,
        artifact_sha256={name:hashlib.sha256((OUT/name).read_bytes()).hexdigest() for name in ('Ekd5-binary-feature-map.txt','Ekd5-disassembly.txt','Ekd5-function-inventory.json')})
    (OUT/'Ekd5-map-validation.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':render()
