import json,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from combat_assets import decode_message,decode_critical_speeches
from ui_assets import combat_palette

class CombatTextTests(unittest.TestCase):
    def test_original_speech_bytes(self):
        root=Path(__file__).resolve().parents[1]
        raw=Path('/Users/henryzhang/Downloads/sgzccz/Imsg.e5').read_bytes()
        fixture=json.loads((root/'tests/fixtures/original_critical_speech.json').read_text())
        self.assertEqual(len(decode_critical_speeches(raw)),51)
        for case in fixture['cases']:
            if not case['events']:continue
            message,speech=case['events']
            decoded=decode_message(raw,message['index'])
            self.assertEqual(decoded['text'],speech['text'])
            self.assertEqual(decoded['text'].encode('gbk').hex(),speech['raw'])

    def test_bounds(self):
        for index in (-1,1):
            with self.assertRaises(ValueError):decode_message(bytes(200),index)

    def test_imported_palette_and_glyphs(self):
        root=Path(__file__).resolve().parents[1]
        original=json.loads((root/'tests/fixtures/original_combat_palette.json').read_text())
        imported=json.loads((root/'assets/generated/ui.json').read_text())
        raw=(root/'assets/generated/raw/Spalet.e5/000.bin').read_bytes()
        result=combat_palette(raw)
        self.assertEqual(result, {k:original[k] for k in ('quantized','lookup')})
        self.assertEqual(result,imported['combat_palette'])
        speeches=json.loads((root/'assets/generated/combat_text.json').read_text())
        glyphs=json.loads((root/'assets/generated/font.json').read_text())['glyphs']
        for row in speeches+list(imported['attack_labels'].values()):
            for ch in row['text']:
                if ch!='\n':self.assertIn(str(ord(ch)),glyphs)
