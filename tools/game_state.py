#!/usr/bin/env python3
"""Typed, read-only original-edition memory observations and semantic diffs.

This is not a save format or a CPU checkpoint/replay system. A capture contains
only selected memory records, without registers, stack or complete host state.
Unknown bytes and unknown flag bits remain visible instead of being discarded.
"""
import argparse
import functools
import hashlib
import json
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
DEFAULT = ROOT / 'tools/game_state_dictionary.json'
FORMATS = {'u8': 'B', 'u16': 'H', 'u32': 'I', 'i32': 'i', 'ptr32': 'I', 'bits32': 'I', 'bytes': 'B'}
SIGNEDNESS = {'u8': 'unsigned', 'u16': 'unsigned', 'u32': 'unsigned', 'i32': 'signed',
              'ptr32': 'unsigned address', 'bits32': 'context-dependent', 'bytes': 'not applicable'}

class StateError(ValueError):
    pass

def require(condition, message):
    if not condition:
        raise StateError(message)

def uint(value, maximum=0xffffffff):
    return type(value) is int and 0 <= value <= maximum

def canonical(value):
    return json.dumps(value, sort_keys=True, ensure_ascii=False, separators=(',', ':')).encode()

def width(field):
    return struct.calcsize('<' + FORMATS[field['type']])

def exact_read(reader, address, size):
    require(uint(address) and size > 0 and address + size <= 0x100000000, 'read outside x86 address space')
    try:
        value = bytes(reader(address, size))
    except Exception as exc:
        raise StateError(f'unreadable memory at {address:#x}, size {size}: {type(exc).__name__}') from exc
    require(len(value) == size, f'truncated read at {address:#x}: wanted {size}, got {len(value)}')
    return value

class Dictionary:
    def __init__(self, path=DEFAULT):
        self.path = Path(path)
        self.data = json.loads(self.path.read_text())
        self.hash = hashlib.sha256(canonical(self.data)).hexdigest()
        self.structures = self.data['structures']
        self.unknown = {}
        self._validate()

    def _validate(self):
        d = self.data
        require(d['schema_version'] == 1 and d['byte_order'] == 'little', 'unsupported dictionary schema/byte order')
        require(re.fullmatch(r'[0-9a-f]{64}', d['executable_sha256']) is not None, 'bad executable identity')
        require(isinstance(d['dictionary_version'], str) and d['dictionary_version'], 'missing dictionary version')
        fixed = []
        for key, s in self.structures.items():
            require(re.fullmatch(r'[a-z][a-z0-9_]*', key) is not None, 'bad structure identifier')
            require(uint(s['size'], 65536) and s['size'] > 0, f'{key}: invalid record size')
            require(s['stride'] >= s['size'] and uint(s['stride']), f'{key}: invalid stride')
            require(uint(s['count'], 4096) and s['count'] > 0, f'{key}: invalid count')
            loc = s['location']; extent = s['stride'] * (s['count'] - 1) + s['size']
            if loc['kind'] == 'fixed':
                require(uint(loc['address']) and loc['address'] + extent <= 0x100000000, f'{key}: invalid address')
                fixed.append((loc['address'], loc['address'] + extent, key))
            else:
                require(loc['kind'] == 'pointer', f'{key}: unknown location kind')
                require(all(uint(loc[n]) for n in ('pointer_at', 'capacity_at', 'offset')), f'{key}: invalid pointer location')
            covered = set(); names = set()
            for f in s['fields']:
                require(re.fullmatch(r'[a-z][a-z0-9_]*', f['name']) is not None and f['name'] not in names, f'{key}: duplicate/bad field')
                names.add(f['name'])
                require(f['type'] in FORMATS and f['signedness'] == SIGNEDNESS[f['type']], f'{key}.{f["name"]}: inconsistent type/signedness')
                require(uint(f['count'], 65536) and f['count'] > 0 and uint(f['offset']), f'{key}: invalid field extent')
                end = f['offset'] + width(f) * f['count']
                require(end <= s['size'], f'{key}.{f["name"]}: field exceeds record')
                positions = set(range(f['offset'], end))
                require(not covered & positions, f'{key}.{f["name"]}: overlapping fields')
                covered |= positions
                require(f['confidence'] in ('observed', 'partial', 'raw'), f'{key}: invalid confidence')
                require(f['note'] and f['units'] and f['evidence'], f'{key}: missing interpretation/evidence')
                for ref in f['evidence']:
                    require(ref in d['evidence'], f'{key}: unknown evidence {ref}')
                if 'element_names' in f:
                    require(len(f['element_names']) == f['count'], f'{key}: wrong element-name count')
                if 'shape' in f:
                    require(len(f['shape']) == 2, f'{key}: only two-dimensional field shapes supported')
                    total = 1
                    for n in f['shape']:
                        require(uint(n) and n > 0, f'{key}: bad shape'); total *= n
                    require(total == f['count'], f'{key}: shape/count mismatch')
                if 'target_structure' in f:
                    require(f['type'] == 'ptr32' and f['target_structure'] in self.structures, f'{key}: invalid pointer target')
                for mask in f.get('flags', {}):
                    n = int(mask)
                    require(n > 0 and n & (n - 1) == 0 and n < 1 << (width(f) * 8), f'{key}: invalid flag mask')
            self.unknown[key] = [i for i in range(s['size']) if i not in covered]
        for left, right in zip(sorted(fixed), sorted(fixed)[1:]):
            require(left[1] <= right[0], f'overlapping structures {left[2]}/{right[2]}')

    def check_evidence(self):
        for key, evidence in self.data['evidence'].items():
            path = ROOT / evidence['path']
            require(path.is_file(), f'{key}: missing evidence {path}')
            require(evidence['anchor'] in path.read_text(), f'{key}: evidence anchor changed in {path}')
        return len(self.data['evidence'])

    def structure(self, name, index=0):
        require(name in self.structures, f'unknown structure {name}')
        s = self.structures[name]
        require(uint(index) and index < s['count'], f'{name}: index outside [0,{s["count"]})')
        return s

    def values(self, name, data):
        """Fast scalar/array values for existing probe adapters. No coercion."""
        s = self.structure(name)
        require(len(data) == s['size'], f'{name}: expected {s["size"]} bytes, got {len(data)}')
        result = {}
        for f in s['fields']:
            values = list(struct.unpack_from('<' + str(f['count']) + FORMATS[f['type']], data, f['offset']))
            if 'shape' in f:
                require(len(f['shape']) == 2, 'only two-dimensional field shapes supported')
                values = [values[i:i+f['shape'][1]] for i in range(0, len(values), f['shape'][1])]
            result[f['name']] = values[0] if f['count'] == 1 else values
        return result

    def read_values(self, reader, name, index=0):
        s = self.structure(name, index); address, _ = self.resolve(reader, name, index)
        return self.values(name, exact_read(reader, address, s['size']))

    def resolve(self, reader, name, index=0):
        s = self.structure(name, index); loc = s['location']
        if loc['kind'] == 'fixed':
            return loc['address'] + index * s['stride'], None
        base = int.from_bytes(exact_read(reader, loc['pointer_at'], 4), 'little')
        capacity = int.from_bytes(exact_read(reader, loc['capacity_at'], 4), 'little')
        binding = {'base': base, 'capacity': capacity}
        return self._pointer_address(s, index, binding), binding

    def _pointer_address(self, s, index, binding):
        loc = s['location']; base = binding['base']; capacity = binding['capacity']
        require(uint(base) and base != 0 and uint(capacity) and base + capacity <= 0x100000000, 'invalid/null buffer binding')
        offset = loc['offset'] + index * s['stride']
        require(offset + s['size'] <= capacity, 'buffer capacity smaller than requested plane')
        return base + offset

    def capture(self, reader, selection, *, executable_sha256, label='', labels=None):
        """Read only specified records. Caller must supply the actual EXE identity."""
        require(executable_sha256 == self.data['executable_sha256'], 'executable edition mismatch')
        require(isinstance(selection, dict) and selection, 'empty/invalid capture selection')
        records = []
        for name, indices in sorted(selection.items()):
            require(isinstance(indices, (list, tuple)) and indices, f'{name}: empty/invalid indices')
            require(len(indices) == len(set(indices)), f'{name}: duplicate indices')
            for index in sorted(indices):
                s = self.structure(name, index); address, binding = self.resolve(reader, name, index)
                row = {'structure': name, 'index': index, 'address': address,
                       'bytes': exact_read(reader, address, s['size']).hex()}
                if binding is not None: row['binding'] = binding
                records.append(row)
        result = {'format': 'caocao-memory-observation', 'format_version': 1,
                  'executable_sha256': executable_sha256, 'dictionary_version': self.data['dictionary_version'],
                  'dictionary_sha256': self.hash, 'label': label, 'labels': labels or {}, 'records': records,
                  'memory_sha256': hashlib.sha256(canonical(records)).hexdigest()}
        self.validate_capture(result)
        return result

    def validate_capture(self, capture):
        require(capture.get('format') == 'caocao-memory-observation' and capture.get('format_version') == 1, 'unsupported observation format')
        require(capture.get('executable_sha256') == self.data['executable_sha256'], 'executable edition mismatch')
        require(capture.get('dictionary_sha256') == self.hash and capture.get('dictionary_version') == self.data['dictionary_version'], 'dictionary identity mismatch; do not reinterpret old observations')
        require(isinstance(capture.get('labels'), dict) and all(isinstance(k, str) and isinstance(v, str) for k,v in capture['labels'].items()), 'invalid display labels')
        require(isinstance(capture.get('records'), list) and capture['records'], 'empty/invalid records')
        require(capture.get('memory_sha256') == hashlib.sha256(canonical(capture['records'])).hexdigest(), 'observation memory checksum mismatch')
        keys = set(); ranges = []
        for r in capture['records']:
            s = self.structure(r['structure'], r['index']); key = (r['structure'], r['index'])
            require(key not in keys, f'duplicate observation {key}'); keys.add(key)
            require(isinstance(r['bytes'], str) and len(r['bytes']) == s['size'] * 2 and re.fullmatch('[0-9a-fA-F]+', r['bytes']) is not None, f'{key}: truncated/invalid record bytes')
            loc = s['location']
            expected = loc['address'] + r['index'] * s['stride'] if loc['kind'] == 'fixed' else self._pointer_address(s, r['index'], r['binding'])
            require(uint(r['address']) and r['address'] == expected, f'{key}: incorrect VA/binding')
            ranges.append((expected, expected + s['size']))
        for left, right in zip(sorted(ranges), sorted(ranges)[1:]):
            require(left[1] <= right[0], 'overlapping observation records')
        require(set(capture['labels']).issubset({f'{n}[{i}]' for n,i in keys}), 'label has no captured record')
        return keys

    def scalar(self, field, value):
        result = {'value': value}
        label = field.get('sentinels', {}).get(str(value), field.get('enum', {}).get(str(value)))
        if label is not None: result['meaning'] = label
        elif 'enum' in field: result['meaning'] = 'unclassified enum value'
        if field['type'] == 'bits32':
            result['signed_view'] = value if value < 0x80000000 else value - 0x100000000
        if field['type'] == 'ptr32':
            result['hex'] = f'0x{value:08x}'
            if value == 0: result['reference'] = 'null'
            elif 'target_structure' in field:
                target = self.structures[field['target_structure']]; loc = target['location']
                delta = value - loc['address']
                if 0 <= delta < target['stride'] * target['count'] and delta % target['stride'] == 0:
                    result['reference'] = f'{field["target_structure"]}[{delta // target["stride"]}]'
                else: result['reference'] = 'unresolved or misaligned pointer'
        if 'flags' in field:
            masks = {int(k): v for k,v in field['flags'].items()}
            result['flags'] = [v for k,v in masks.items() if value & k]
            result['unknown_bits'] = value & ~sum(masks)
        return result

    def lookup(self, address, capture=None):
        """Locate a byte in fixed records or explicitly captured runtime buffers."""
        require(uint(address), 'invalid x86 lookup address')
        candidates=[]
        if capture is not None:
            self.validate_capture(capture)
            candidates=[(r['structure'],r['index'],r['address']) for r in capture['records']]
        else:
            for name,s in self.structures.items():
                loc=s['location']
                if loc['kind']!='fixed' or address<loc['address']: continue
                index=(address-loc['address'])//s['stride']
                if index<s['count']: candidates.append((name,index,loc['address']+index*s['stride']))
        for name,index,base in candidates:
            s=self.structures[name]; relative=address-base
            if not 0<=relative<s['size']: continue
            for f in s['fields']:
                delta=relative-f['offset']
                if 0<=delta<width(f)*f['count']:
                    element=delta//width(f)
                    return {'record':f'{name}[{index}]','field':f['name'],'element':element,
                            'byte_within_element':delta%width(f),'address':address,'field_metadata':f}
            return {'record':f'{name}[{index}]','address':address,'unknown_offset':relative}
        return {'address':address,'unclassified':True,'note':'No matching record. Dynamic buffers require a capture; this dictionary covers only selected structures.'}

    def decode(self, capture):
        self.validate_capture(capture); result = []
        for r in capture['records']:
            s = self.structures[r['structure']]; data = bytes.fromhex(r['bytes']); fields = []
            key = f'{r["structure"]}[{r["index"]}]'
            for f in s['fields']:
                values = struct.unpack_from('<' + str(f['count']) + FORMATS[f['type']], data, f['offset'])
                fields.append({'name': f['name'], 'offset': f['offset'], 'address': r['address'] + f['offset'],
                               'type': f['type'], 'signedness': f['signedness'], 'confidence': f['confidence'],
                               'evidence': f['evidence'], 'units': f['units'], 'note': f['note'],
                               'elements': [self.scalar(f,v) for v in values]})
            result.append({'record': key, 'label': capture['labels'].get(key, key), 'address': r['address'], 'fields': fields,
                           'unknown_bytes': {f'0x{i:02x}': data[i] for i in self.unknown[r['structure']]}})
        return result

    def diff(self, before, after):
        require(self.validate_capture(before) == self.validate_capture(after), 'record selections differ; comparisons must use the same records')
        other = {(r['structure'],r['index']): r for r in after['records']}; changes = []
        for r in before['records']:
            target = other[r['structure'],r['index']]; s = self.structures[r['structure']]
            key = f'{r["structure"]}[{r["index"]}]'; label = before['labels'].get(key, key)
            if r['address'] != target['address']:
                changes.append({'path': key+'.@address', 'label': label, 'before': r['address'], 'after': target['address'], 'kind': 'relocation'})
            a = bytes.fromhex(r['bytes']); b = bytes.fromhex(target['bytes'])
            for f in s['fields']:
                av = struct.unpack_from('<' + str(f['count']) + FORMATS[f['type']], a, f['offset'])
                bv = struct.unpack_from('<' + str(f['count']) + FORMATS[f['type']], b, f['offset'])
                for n,(x,y) in enumerate(zip(av,bv)):
                    if x == y: continue
                    suffix = '' if f['count'] == 1 else '['+str(n)+']'
                    field_label = f['name'] + ('.'+f['element_names'][n] if 'element_names' in f else suffix)
                    changes.append({'path': key+'.'+f['name']+suffix, 'label': label+'.'+field_label,
                        'address_before': r['address']+f['offset']+n*width(f), 'address_after': target['address']+f['offset']+n*width(f),
                        'before': self.scalar(f,x), 'after': self.scalar(f,y), 'kind': 'field', 'confidence': f['confidence'], 'evidence': f['evidence']})
            for n in self.unknown[r['structure']]:
                if a[n] != b[n]:
                    changes.append({'path': key+f'.unknown[0x{n:02x}]', 'label': label+f'.unknown[0x{n:02x}]',
                                    'address_before': r['address']+n, 'address_after': target['address']+n,
                                    'before': a[n], 'after': b[n], 'kind': 'unknown_byte'})
        return changes

    def describe(self):
        lines = ['# Original game state dictionary', '',
            f'Dictionary version {self.data["dictionary_version"]}; SHA-256 `{self.hash}`.', '',
            f'Executable SHA-256: `{self.data["executable_sha256"]}`.', '', self.data['scope'], '',
            'All addresses below are original-edition x86 virtual addresses, not EXE file offsets. All numbers are little-endian.',
            'Fields marked observed have bounded probe evidence; this does not establish every caller. Partial fields retain uncertainty.',
            'Unknown bytes and flag bits are preserved. Signed views of bits32 do not change the raw unsigned word.',
            'Record extents are the inspected windows, not claims about complete C++ object sizes.', '',
            '## Usage', '',
            'The editable source is [game_state_dictionary.json](../tools/game_state_dictionary.json). The shared Python API and CLI are in [game_state.py](../tools/game_state.py).', '',
            'Run from the caocao-engine project directory:', '',
            '```sh',
            '.venv/bin/python tools/game_state.py validate',
            '.venv/bin/python tools/game_state.py lookup 0x004b2c5d',
            '.venv/bin/python tools/game_state.py --dictionary docs/evidence/game-state/dictionary-1.0.0.json decode docs/evidence/game-state/tutorial.json',
            '.venv/bin/python tools/game_state.py --dictionary docs/evidence/game-state/dictionary-1.0.0.json diff docs/evidence/game-state/cancel-before.json docs/evidence/game-state/cancel-after.json',
            '.venv/bin/python tools/game_state.py --dictionary docs/evidence/game-state/dictionary-1.0.0.json lookup 0x00818c80 --capture docs/evidence/game-state/cancel-before.json',
            '```', '',
            'The historical examples use the archived1.0.0 dictionary matching their recorded identity; current captures use the default dictionary. The final lookup uses the recorded buffer binding. Without a capture, that harness address is deliberately unclassified.', '',
            'Within an existing initialized ProbeVM, capture only the records needed for the investigation:', '',
            '```python',
            'from game_state import default_dictionary, format_changes',
            'selection = {"live_units": [0], "occupancy": [0], "rng": [0]}',
            'before = v.capture_state(selection, labels={"live_units[0]": "曹操"})',
            '# Run the probe\'s already-configured, bounded original action here.',
            'after = v.capture_state(selection)',
            'print(format_changes(default_dictionary().diff(before, after)))',
            '```', '',
            'Capture only reads memory. Observation files include executable/dictionary identities, exact bytes, buffer bindings and a memory checksum. They omit CPU context and cannot resume execution.',
            'Comparisons reject mismatched identities, corrupt bytes and different record selections. Display labels are optional annotations, not decoded identity evidence.',
            'All stored array capacity is preserved, including bytes after a target sentinel and cells outside the active map. Field notes describe when those bytes are inactive.', '',
            '## Validation and regeneration', '',
            '```sh',
            '.venv/bin/python -m unittest discover -s tests -p test_game_state.py -v',
            '.venv/bin/python tools/state_dictionary_probe.py',
            '.venv/bin/python tools/game_state.py describe --output docs/GAME-STATE-DICTIONARY.md',
            '```', '',
            'The original-code probe requires the locally supplied game and imported assets. It checks tutorial setup, cancellation, strike planning and RNG, writing diagnostics only to docs/evidence/game-state.',
            'See [verification](evidence/game-state/verification.json), [regression comparisons](evidence/game-state/regression.json), [tests](evidence/game-state/tests.log), and [JEV decisions](JEV-DECISIONS.md).',
            'Current scope does not include a complete inventory layout, all script storage, all controller fields, native C++ object memory layout, or a save/checkpoint implementation.', '']
        for name,s in self.structures.items():
            loc = s['location']; location = f'VA `0x{loc["address"]:08x}`' if loc['kind']=='fixed' else f'`u32(0x{loc["pointer_at"]:08x}) + 0x{loc["offset"]:x}`, bounded by capacity at `0x{loc["capacity_at"]:08x}`'
            lines += [f'## {name}', '', s['name']+'. '+s['scope'], '', f'{location}; size `{s["size"]}`; stride `{s["stride"]}`; count `{s["count"]}`.', '',
                      '| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |', '|---|---|---|---|---|---|']
            for f in s['fields']:
                meanings = '; '.join(k+'='+v for k,v in f.get('enum',{}).items())
                flags = '; '.join(hex(int(k))+'='+v for k,v in f.get('flags',{}).items())
                sentinel = '; '.join(k+'='+v for k,v in f.get('sentinels',{}).items())
                detail = ' '.join(filter(None,[f['note'], meanings, flags, sentinel, 'Units: '+f['units']+'.', 'Evidence: '+', '.join(f['evidence'])+'.']))
                if 'element_names' in f: detail += ' Elements: '+', '.join(f['element_names'])+'.'
                lines.append(f'| `0x{f["offset"]:x}` | {f["name"]} | {f["type"]} × {f["count"]} | {f["signedness"]} | {f["confidence"]} | {detail.replace("|", "/")} |')
            unknown = self.unknown[name]; ranges=[]
            for n in unknown:
                if ranges and ranges[-1][1] == n: ranges[-1][1] += 1
                else: ranges.append([n,n+1])
            lines += ['', 'Unknown byte intervals (end exclusive): '+(', '.join(f'`[0x{a:x},0x{b:x})`' for a,b in ranges) or 'none within this inspected window')+'.', '']
        lines += ['## Evidence index', '']
        for key,e in self.data['evidence'].items():
            lines += [f'- **{key}**: [{e["path"]}](../{e["path"]}); anchor `{e["anchor"]}`. {e["scope"]}']
        return '\n'.join(lines)+'\n'

@functools.lru_cache(maxsize=1)
def default_dictionary():
    return Dictionary()

def format_changes(changes):
    return '\n'.join(f'{r["label"]}: {json.dumps(r["before"],ensure_ascii=False)} -> {json.dumps(r["after"],ensure_ascii=False)}' for r in changes) + ('\n' if changes else 'No memory changes in selected records.\n')

def main():
    p = argparse.ArgumentParser(description=__doc__); p.add_argument('--dictionary',type=Path,default=DEFAULT)
    commands = p.add_subparsers(dest='command',required=True)
    commands.add_parser('validate')
    desc=commands.add_parser('describe'); desc.add_argument('--output',type=Path)
    lookup=commands.add_parser('lookup'); lookup.add_argument('address',type=lambda value:int(value,0)); lookup.add_argument('--capture',type=Path)
    decode=commands.add_parser('decode'); decode.add_argument('capture',type=Path)
    diff=commands.add_parser('diff'); diff.add_argument('before',type=Path); diff.add_argument('after',type=Path); diff.add_argument('--json',action='store_true')
    a=p.parse_args()
    try:
        d=Dictionary(a.dictionary)
        if a.command=='validate':
            result={'dictionary_sha256':d.hash,'structures':len(d.structures),'fields':sum(len(s['fields']) for s in d.structures.values()),'evidence_sources':d.check_evidence()}
        elif a.command=='describe':
            text=d.describe()
            if a.output:
                # This command writes generated documentation, never observations/saves.
                a.output.write_text(text); print(a.output); return
            print(text,end=''); return
        elif a.command=='lookup': result=d.lookup(a.address,json.loads(a.capture.read_text()) if a.capture else None)
        elif a.command=='decode': result=d.decode(json.loads(a.capture.read_text()))
        else:
            result=d.diff(json.loads(a.before.read_text()),json.loads(a.after.read_text()))
            if not a.json: print(format_changes(result),end=''); return
        print(json.dumps(result,ensure_ascii=False,indent=2))
    except (StateError,KeyError,TypeError,ValueError,OSError) as exc:
        p.exit(2, 'game-state error: '+str(exc)+'\n')

if __name__=='__main__':main()
