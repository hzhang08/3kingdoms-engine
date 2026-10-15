#!/usr/bin/env python3
"""Build a source-linked, presentation-only English catalog for the four battles."""
import hashlib,json,re
from pathlib import Path
root=Path(__file__).resolve().parents[1]; base=root/'assets/localization'; assets=root/'assets/prototype'
translations={}
def put(zh,en):
    if not zh:return
    translations[zh]=en
ui=json.loads((assets/'ui.json').read_text());data=json.loads((assets/'data.json').read_text());details=json.loads((assets/'unit-details/descriptions.json').read_text());english=json.loads((base/'data-en.json').read_text())
for kind,rows,field in [('classes',ui['classes'],'text'),('terrain',ui['terrain'],'text'),('items',data['items'],'name'),('tactics',json.loads((assets/'unit-details/tactics.json').read_text()),'name'),('class_descriptions',details['classes'],'text'),('item_descriptions',details['items'],'text')]:
    assert len(rows)==len(english[kind]),(kind,len(rows),len(english[kind]))
    for original,en in zip(rows,english[kind]):put(original[field],en)
for kind in ['equipment_types','equipment_effects']:
    assert len(details[kind])==len(english[kind])
    for zh,en in zip(details[kind],english[kind]):put(zh,en)
for id,en in english['heroes'].items():put(data['heroes'][int(id)]['name'],en)
for zh,en in english['generic_names'].items():put(zh,en)
for id,en in english['biographies'].items():put(details['biographies'][int(id)]['text'],en)
source_lines=json.loads((base/'script-lines-zh.json').read_text());script_en={int(row.split('\t',1)[0]):row.split('\t',1)[1] for row in (base/'script-lines-en.tsv').read_text().splitlines()}
assert set(script_en)==set(range(len(source_lines)))
for i,zh in enumerate(source_lines):put(zh,script_en[i])
for row in (base/'ui-en.tsv').read_text().splitlines():
    if not row or row.startswith('#'):continue
    zh,en=row.split('\t',1);put(zh.replace('\\n','\n'),en.replace('\\n','\n'))
# Numbered soldiers keep their original differentiating number.
for h in data['heroes']:
    for zh,en in english['generic_names'].items():
        if re.fullmatch(re.escape(zh)+r'\d+',h['name']):put(h['name'],en+' '+h['name'][len(zh):])
phrases=sorted(translations,key=lambda s:(-len(s.encode()),s))
def translate(s):
    if s in translations:return translations[s]
    result='';p=0
    while p<len(s):
        match=next((k for k in phrases if s.startswith(k,p)),None)
        if match:result+=translations[match];p+=len(match)
        else:result+=s[p];p+=1
    return result
sources=[*sorted(root.glob('src/prototype*.cpp')),*sorted(root.glob('src/prototype*.inc')),root/'src/unit_details.cpp']
required=set(source_lines)
required.update(t['text'].replace('\\n',' ') for t in json.loads((assets/'combat_text.json').read_text()))
def script_strings(value):
    if isinstance(value,dict):
        if isinstance(value.get('value'),str):
            required.update(line.lstrip('&').strip() for line in value['value'].splitlines() if re.search('[\u4e00-\u9fff]',line))
        for v in value.values():script_strings(v)
    elif isinstance(value,list):
        for v in value:script_strings(v)
for path in [assets/'scripts/S_00.json',*sorted((assets/'expansion').glob('S_0*.json'))]:script_strings(json.loads(path.read_text()))
for f in sources:
    for s in re.findall(r'"((?:[^"\\]|\\.)*)"',f.read_text()):
        if re.search('[\u4e00-\u9fff]',s):required.add(s.replace('\\n','\n'))
# Cover all metadata reachable in the four battles, including inherited gear.
for kind in ['classes','terrain']:
    required.update(e['text'] for e in ui[kind])
required.update(e['name'] for e in data['items'])
required.update(e['text'] for e in details['items']+details['classes'])
required.update(details['equipment_effects']);required.update(details['equipment_types'])
required.update(details['biographies'][int(i)]['text'] for i in english['biographies'])
roster={0,3,32,33,146,147,474,475,476,477,478,479,480,481,482,483,484,485}
for scenario in json.loads((assets/'expansion/scenarios.json').read_text()):roster.update(e['id'] for e in scenario['roster'])
required.update(data['heroes'][i]['name'] for i in roster)
required.update(e['name'] for e in json.loads((assets/'unit-details/tactics.json').read_text()))
missing=[s for s in sorted(required) if re.search('[\u4e00-\u9fff]',translate(s))]
source_files=[assets/'combat_text.json',assets/'ui.json',assets/'data.json',assets/'unit-details/descriptions.json',assets/'unit-details/tactics.json',assets/'scripts/S_00.json',*sorted((assets/'expansion').glob('S_0*.json'))]
catalog={'version':1,'language':'en','scope':'Four-battle presentation; original state and scripts are unchanged. Original item/class descriptions are translated, not claims that every described mechanic is implemented.','sources':{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in source_files},'translations':dict(sorted(translations.items()))}
(base/'en.json').write_text(json.dumps(catalog,ensure_ascii=False,indent=2)+'\n')
report={'entries':len(translations),'required_strings':len(required),'script_lines':len(source_lines),'biographies':len(english['biographies']),'missing':missing}
font=json.loads((assets/'unit-details/font.json').read_text())['glyphs']
assert not {c for s in translations.values() for c in s if c!='\n' and str(ord(c)) not in font},'Missing font glyphs'
(base/'coverage.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n');print(json.dumps(report,ensure_ascii=False,indent=2))
assert not missing,'Translate missing source strings before releasing'
