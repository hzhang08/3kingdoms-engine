import copy
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from game_state import Dictionary, StateError, canonical, format_changes
from executable_probe import EDITION
from movement_probe import live_json
from initialization_probe import state

class Memory:
    def __init__(self): self.regions = {}
    def put(self, address, value): self.regions[address] = bytes(value)
    def read(self, address, size):
        for start, value in self.regions.items():
            if start <= address and address+size <= start+len(value):
                return value[address-start:address-start+size]
        raise ValueError('unmapped test memory')

def old_live(row):
    return dict(id=int.from_bytes(row[:4],'little'),slot=row[4],side=row[5],x=row[6],y=row[7],
        target_slot=row[8],target_x=row[9],target_y=row[10],lifecycle=row[12],flags=row[13],ai=row[14],direction=row[15],
        hp=int.from_bytes(row[16:20],'little'),mp=int.from_bytes(row[20:24],'little'),grades=list(row[24:30]),status=row[30])

def old_unit(row, uid):
    return dict(id=uid,base_stats=list(row[0x21:0x26]),stats=list(row[0x15:0x1a]),
        hp=int.from_bytes(row[0x1c:0x20],'little'),mp=row[0x20],owner=row[0x2a],
        **{'class':row[0x2b]},level=row[0x2c],xp=row[0x2d],
        equipment=[list(row[0x2e+n*3:0x31+n*3]) for n in range(3)],record_counters=list(row[0x27:0x2a]))

class DictionaryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls): cls.d = Dictionary()

    def capture(self, memory, selection, **kwargs):
        return self.d.capture(memory.read, selection, executable_sha256=EDITION, **kwargs)

    def test_schema_and_evidence(self):
        self.assertEqual(self.d.hash, hashlib.sha256(canonical(self.d.data)).hexdigest())
        self.assertGreaterEqual(self.d.check_evidence(), 20)
        for name,s in self.d.structures.items():
            known = sum(struct.calcsize('<'+{'u8':'B','u16':'H','u32':'I','i32':'i','ptr32':'I','bits32':'I','bytes':'B'}[f['type']])*f['count'] for f in s['fields'])
            self.assertEqual(known+len(self.d.unknown[name]),s['size'])

    def test_adapters_match_previous_byte_decoders(self):
        rng=random.Random(732)
        memory=Memory()
        for uid in range(512):
            raw=rng.randbytes(72);memory.put(0x4a1b68+72*uid,raw)
            self.assertEqual(state(memory,uid),old_unit(raw,uid))
        for slot in range(115):
            raw=rng.randbytes(36);memory.put(0x4b2c50+36*slot,raw)
            self.assertEqual(live_json(memory,slot),old_live(raw))

    def test_original_live_records_and_side_labels(self):
        fixture=json.loads((ROOT/'tests/fixtures/original_battle_initialization.json').read_text())
        self.assertEqual(fixture['executable_sha256'],EDITION)
        for row in fixture['battle']:
            memory=Memory();memory.put(0x4b2c50+row['slot']*36,bytes(row['live']))
            self.assertEqual(live_json(memory,row['slot']),old_live(bytes(row['live'])))
            capture=self.capture(memory,{'live_units':[row['slot']]})
            side=next(f for f in self.d.decode(capture)[0]['fields'] if f['name']=='side')['elements'][0]
            expected='player' if row['slot']==0 else 'ally' if row['slot']<35 else 'enemy'
            self.assertEqual(side['meaning'],expected)

    def test_all_original_cancellation_cases_name_only_coordinate_changes(self):
        fixture=json.loads((ROOT/'tests/fixtures/original_tutorial_actions.json').read_text())
        self.assertEqual(fixture['executable_sha256'],EDITION)
        for case in fixture['cancel_cases']:
            memory=Memory();memory.put(0x4b2c50,bytes(case['before']))
            before=self.capture(memory,{'live_units':[0]},labels={'live_units[0]':'曹操'})
            memory.put(0x4b2c50,bytes(case['after']))
            after=self.capture(memory,{'live_units':[0]})
            changes=self.d.diff(before,after)
            self.assertEqual([r['path'] for r in changes],[] if case['skip'] else ['live_units[0].y'])
            if changes:
                self.assertEqual(changes[0]['before']['value'],0)
                self.assertEqual(changes[0]['after']['value'],6)
                self.assertIn('曹操.y',format_changes(changes))

    def test_unknown_bytes_and_flag_bits_are_not_lost(self):
        memory=Memory();raw=bytearray(36);memory.put(0x4b2c50,raw)
        before=self.capture(memory,{'live_units':[0]})
        raw[13]=2|4|32|64;raw[35]=0xab;memory.put(0x4b2c50,raw)
        after=self.capture(memory,{'live_units':[0]});changes=self.d.diff(before,after)
        self.assertEqual([r['path'] for r in changes],['live_units[0].flags','live_units[0].unknown[0x23]'])
        self.assertEqual(changes[0]['after']['flags'],['acted','moved'])
        self.assertEqual(changes[0]['after']['unknown_bits'],96)
        self.assertEqual(changes[1]['after'],171)

    def test_damage_signedness_and_pointer_alignment(self):
        memory=Memory();raw=bytearray(0x614)
        struct.pack_into('<I',raw,0x84,0xffffffff)
        struct.pack_into('<I',raw,8,0x4a1b68+72*234)
        struct.pack_into('<I',raw,12,0x4b2c50+1)
        memory.put(0x4927f0,raw)
        result=self.d.decode(self.capture(memory,{'attack':[0]}))[0]
        f={f['name']:f['elements'] for f in result['fields']}
        self.assertEqual(f['hp_damage'][0],{'value':4294967295,'signed_view':-1})
        self.assertEqual(f['actor_record'][0]['reference'],'persistent_units[234]')
        self.assertEqual(f['actor_live'][0]['reference'],'unresolved or misaligned pointer')

    def test_pointer_based_planes_follow_relocated_buffer(self):
        memory=Memory();memory.put(0x4aeb3c,(0x2580).to_bytes(4,'little'))
        buffer=bytearray(0x2580);buffer[0xc80+130]=35
        memory.put(0x4aeb44,(0x810000).to_bytes(4,'little'));memory.put(0x810000,buffer)
        before=self.capture(memory,{'occupancy':[0]})
        memory.put(0x4aeb44,(0x910000).to_bytes(4,'little'));memory.put(0x910000,buffer)
        after=self.capture(memory,{'occupancy':[0]})
        self.assertEqual(before['records'][0]['address'],0x810c80)
        self.assertEqual(after['records'][0]['address'],0x910c80)
        self.assertEqual([r['kind'] for r in self.d.diff(before,after)],['relocation'])
        self.assertEqual(self.d.decode(after)[0]['fields'][0]['elements'][130]['value'],35)

    def test_invalid_memory_capture_and_identity_rejected(self):
        memory=Memory();memory.put(0x48ecd4,b'\0'*4)
        capture=self.capture(memory,{'rng':[0]})
        for field,value in [('format_version',2),('executable_sha256','0'*64),('dictionary_sha256','0'*64),('dictionary_version','old')]:
            broken=copy.deepcopy(capture);broken[field]=value
            with self.assertRaises(StateError):self.d.decode(broken)
        broken=copy.deepcopy(capture);broken['records'][0]['bytes']='01000000'
        with self.assertRaisesRegex(StateError,'checksum'):self.d.decode(broken)
        for value in (-1,True,1):
            with self.assertRaises(StateError):self.capture(memory,{'rng':[value]})
        with self.assertRaises(StateError):self.capture(memory,{'rng':[0,0]})
        with self.assertRaises(StateError):self.capture(memory,{'unknown':[0]})
        with self.assertRaises(StateError):self.d.capture(memory.read,{'rng':[0]},executable_sha256='0'*64)
        with self.assertRaises(StateError):self.d.capture(lambda a,n:b'\0',{'rng':[0]},executable_sha256=EDITION)

    def test_null_and_short_dynamic_allocations_rejected(self):
        memory=Memory();memory.put(0x4aeb44,b'\0'*4);memory.put(0x4aeb3c,(0x2580).to_bytes(4,'little'))
        with self.assertRaisesRegex(StateError,'null'):self.capture(memory,{'terrain':[0]})
        memory.put(0x4aeb44,(0x810000).to_bytes(4,'little'));memory.put(0x4aeb3c,(10).to_bytes(4,'little'))
        with self.assertRaisesRegex(StateError,'capacity'):self.capture(memory,{'terrain':[0]})

    def test_mismatched_selections_rejected(self):
        memory=Memory();memory.put(0x4b2c50,b'\0'*72)
        with self.assertRaisesRegex(StateError,'selections differ'):
            self.d.diff(self.capture(memory,{'live_units':[0]}),self.capture(memory,{'live_units':[1]}))

    def test_schema_overlaps_and_bad_signedness_rejected(self):
        mutations=[lambda d:d['structures']['live_units']['fields'][1].update(offset=0),
                   lambda d:d['structures']['live_units']['fields'][0].update(signedness='signed'),
                   lambda d:d['structures']['attack']['fields'][-1].update(offset=9999),
                   lambda d:d['structures']['live_units']['fields'][0].update(evidence=['missing'])]
        for mutate in mutations:
            data=copy.deepcopy(self.d.data);mutate(data)
            with tempfile.TemporaryDirectory() as directory:
                path=Path(directory)/'invalid.json';path.write_text(json.dumps(data))
                with self.assertRaises(StateError):Dictionary(path)

    def test_command_line_decode_and_diff(self):
        memory=Memory();memory.put(0x48ecd4,(1).to_bytes(4,'little'));before=self.capture(memory,{'rng':[0]})
        memory.put(0x48ecd4,(1103527590).to_bytes(4,'little'));after=self.capture(memory,{'rng':[0]})
        with tempfile.TemporaryDirectory() as directory:
            a=Path(directory)/'before.json';b=Path(directory)/'after.json';a.write_text(json.dumps(before));b.write_text(json.dumps(after))
            result=subprocess.run([sys.executable,str(ROOT/'tools/game_state.py'),'diff',str(a),str(b),'--json'],text=True,capture_output=True,check=True)
            self.assertEqual(json.loads(result.stdout)[0]['after']['value'],1103527590)
            result=subprocess.run([sys.executable,str(ROOT/'tools/game_state.py'),'decode',str(b)],text=True,capture_output=True,check=True)
            self.assertEqual(json.loads(result.stdout)[0]['fields'][0]['elements'][0]['value'],1103527590)

    def test_address_lookup_preserves_unknown_and_dynamic_locations(self):
        found=self.d.lookup(0x4b2c50+35*36+13)
        self.assertEqual((found['record'],found['field']),('live_units[35]','flags'))
        self.assertEqual(self.d.lookup(0x4b2c50+35)['unknown_offset'],35)
        self.assertTrue(self.d.lookup(0x818c80)['unclassified'])
        memory=Memory();memory.put(0x4aeb44,(0x910000).to_bytes(4,'little'))
        memory.put(0x4aeb3c,(0x2580).to_bytes(4,'little'));memory.put(0x910000,b'\xff'*0x2580)
        capture=self.capture(memory,{'occupancy':[0]})
        found=self.d.lookup(0x910c80+130,capture)
        self.assertEqual((found['record'],found['field'],found['element']),('occupancy[0]','bytes',130))

if __name__=='__main__':unittest.main()
