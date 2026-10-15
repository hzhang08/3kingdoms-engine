import sys,struct,unittest,json,hashlib,tempfile
from unittest.mock import patch
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from ls12 import decode,FormatError
from eex import parse,Reader
from import_assets import untile,palette,publish,recover_publication
SOURCE=Path('/Users/henryzhang/Downloads/sgzccz')

def pack_codes(values):
 bits=''
 for v in values:
  n=(v+2).bit_length()-1;bits+='1'*(n-1)+'0'+format(v-((1<<n)-2),f'0{n}b')
 bits+='0'*((-len(bits))%8)
 return int(bits,2).to_bytes(len(bits)//8,'big')
def archive(encoded,size):
 return b'Ls12'+b' '*12+bytes(range(256))+struct.pack('>IIII',len(encoded),size,288,0)+encoded

class DecoderTests(unittest.TestCase):
 @unittest.skipUnless(SOURCE.exists(),'original game unavailable')
 def test_tutorial_text_and_glyph_coverage(self):
  from ui_assets import decode_tutorial_text
  assets=ROOT/'assets/generated';actual=json.loads((assets/'ui.json').read_text())['tutorial_text']
  self.assertEqual(actual,decode_tutorial_text((SOURCE/'Ekd5.exe').read_bytes()))
  glyphs=json.loads((assets/'font.json').read_text())['glyphs']
  texts={row['text'] for row in actual.values()}
  fixture=json.loads((ROOT/'tests/fixtures/original_tutorial_basic.json').read_text())
  for branch in [fixture,*fixture['alternatives']]:
   for event in branch['events']:
    if event['kind'] in ('speech','choice'):self.assertIn(event['text'],texts)
  for event in json.loads((ROOT/'tests/fixtures/original_tutorial_ally.json').read_text())['events']:
   if event['kind']=='speech':self.assertIn(event['text'],texts)
  for event in json.loads((ROOT/'tests/fixtures/original_tutorial_actions.json').read_text())['events']:
   if event['kind']=='speech':self.assertIn(event['text'],texts)
  for event in json.loads((ROOT/'tests/fixtures/original_tutorial_terrain.json').read_text())['events']:
   if event['kind']=='speech':self.assertIn(event['text'],texts)
  for text in texts:
   # The verified lesson saves one complete page at each interaction point.
   self.assertLessEqual(len(text.split('\n')),3)
   for line in text.split('\n'):
    self.assertLessEqual(sum(8 if ord(ch)<128 else 16 for ch in line),480)
    for ch in line:self.assertIn(str(ord(ch)),glyphs)
 def test_terrain_panel_assets(self):
  from ui_assets import decode_terrain_labels
  assets=ROOT/'assets/generated';ui=json.loads((assets/'ui.json').read_text())
  if SOURCE.exists():self.assertEqual(ui['terrain_labels'],decode_terrain_labels((SOURCE/'Ekd5.exe').read_bytes()))
  glyphs=json.loads((assets/'font.json').read_text())['glyphs']
  for row in [*ui['terrain_labels'].values(),*ui['terrain']]:
   for ch in row['text']:self.assertIn(str(ord(ch)),glyphs)
  manifest=json.loads((assets/'manifest.json').read_text());images={a['path']:a for a in manifest['assets']}
  fixture=json.loads((ROOT/'tests/fixtures/original_terrain_panel.json').read_text())
  for case in fixture['cases']:
   gray=0
   for event in case['events']:
    if event['kind']=='gray_icon':name=f'tactic-{gray}-gray';gray+=1
    elif event['kind']=='icon':name=f"tactic-{(event['args'][4]-0x1a5c0)//256}"
    else:continue
    self.assertEqual(images['images/Mark/'+name+'.bmp']['index_pixels_sha256'],event['pixels_sha256'])
 @unittest.skipUnless(SOURCE.exists(),'original game unavailable')
 def test_mcall_original_loader(self):
  from mcall import decode_mcall
  raw=(SOURCE/'Mcall05.e5').read_bytes();meta=decode_mcall(raw)
  fixture=json.loads((ROOT/'tests/fixtures/original_mcall.json').read_text())
  self.assertEqual(meta,json.loads((ROOT/'assets/generated/mcall-05.json').read_text()))
  self.assertEqual((ROOT/'assets/generated/raw/Mcall05.e5/000.bin').read_bytes(),raw)
  frames=[e for e in fixture['cases'][0]['events'] if e['kind']=='effect']
  self.assertEqual(len(frames),30)
  for f,e in zip(meta['frames'],frames):
   self.assertEqual(f['descriptor'],e['descriptor'])
   self.assertEqual(f['pixels_sha256'],e['image_sha256'])
   self.assertEqual(f['mask_sha256'],e['mask_sha256'])
  for invalid in (b'',raw[:19],raw[:-1],raw+b'\0',bytes([29])+raw[1:],raw[:620]+bytes([raw[620]^1])+raw[621:]):
   with self.assertRaises(FormatError):decode_mcall(invalid)
 def test_failed_publication_restores_previous_assets(self):
  with tempfile.TemporaryDirectory() as folder:
   root=Path(folder);old=root/'generated';new=root/'staging'
   old.mkdir();new.mkdir();(old/'marker').write_text('old');(new/'marker').write_text('new')
   rename=Path.rename
   def fail_staging(path,target):
    if path==new:raise OSError('simulated interrupted publication')
    return rename(path,target)
   with patch.object(Path,'rename',fail_staging):
    with self.assertRaises(OSError):publish(new,old)
   self.assertEqual((old/'marker').read_text(),'old')
   self.assertEqual((new/'marker').read_text(),'new')
 def test_restart_recovers_interrupted_publication(self):
  with tempfile.TemporaryDirectory() as folder:
   out=Path(folder)/'generated';backup=Path(folder)/'generated.previous'
   backup.mkdir();(backup/'marker').write_text('last published')
   recover_publication(out)
   self.assertEqual((out/'marker').read_text(),'last published')
   recover_publication(out)
   self.assertEqual((out/'marker').read_text(),'last published')
 def test_compressed_overlap(self):
  b=decode(archive(pack_codes([65,66,67,259,3]),9));self.assertEqual(b[0].data,b'ABCABCABC');self.assertEqual(b[0].offset,288)
 def test_uncompressed(self):self.assertEqual(decode(archive(b'abc',3))[0].data,b'abc')
 def test_reject_truncated(self):
  for data in [b'Ls12',archive(b'abcdef',6)[:-1],archive(pack_codes([257,0]),6)]:
   with self.assertRaises(FormatError):decode(data)
 def test_reject_bomb(self):
  with self.assertRaises(FormatError):decode(archive(b'x',1000000000))
 def test_untile_reversible(self):self.assertEqual(untile(bytes(range(16)),4,4,2),bytes([0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15]))
 def test_palette_brg(self):self.assertEqual(palette(bytes([255,247,0])*256)[:3],bytes([247,0,255]))
 @unittest.skipUnless(SOURCE.exists(),'original game unavailable')
 def test_original_scripts_exhaustive(self):
  r=parse(SOURCE/'R_00.eex');s=parse(SOURCE/'S_00.eex');self.assertEqual([len(x['events']) for x in s['sections']],[1,18,1]);self.assertEqual(s['sections'][1]['offset'],7678)
  seen=[]
  def walk(body):
   for x in body:
    seen.append(x)
    if 'body'in x:walk([x['test']]);walk(x['body'])
  for script in [r,s]:
   for section in script['sections']:
    for event in section['events']:walk(event['conditions']);walk(event['body'])
  self.assertEqual(sum(x['opcode']==75 for x in seen),15)
  self.assertEqual([x['args'][0]['value'] for x in seen if x['opcode']==18],['正是吾心所愿\n似乎言之过早'])
 @unittest.skipUnless((ROOT/'assets/generated/ui.json').exists(),'import required')
 def test_original_ui_assets(self):
  from ui_assets import decode_ui,darken_palette,MARK_IMAGES
  from PIL import Image
  ui=json.loads((ROOT/'assets/generated/ui.json').read_text())
  expected=json.loads((ROOT/'tests/fixtures/original_panel.json').read_text())['ui']
  for key in expected:self.assertEqual(ui[key],expected[key])
  self.assertEqual(decode_ui((SOURCE/'Ekd5.exe').read_bytes()),expected)
  self.assertEqual(ui['darken'],json.loads((ROOT/'tests/fixtures/original_palette.json').read_text())['darken'])
  raw=decode((SOURCE/'Spalet.e5').read_bytes(),indices={0})[0].data
  self.assertEqual(darken_palette(raw),ui['darken'])
  mark=(SOURCE/'Mark.e5').read_bytes();colors=palette(raw)
  glyphs=json.loads((ROOT/'assets/generated/font.json').read_text())['glyphs']
  for row in ui['classes']+ui['terrain']+list(ui['labels'].values()):
   for ch in row['text']:self.assertIn(str(ord(ch)),glyphs)
  for name,offset,w,h in MARK_IMAGES:
   actual=Image.open(ROOT/f'assets/generated/images/Mark/{name}.bmp').convert('RGB').tobytes()
   expected=b''.join(colors[n*3:n*3+3] for n in mark[offset:offset+w*h])
   self.assertEqual(actual,expected,name)
 @unittest.skipUnless((ROOT/'assets/generated/font.json').exists(),'import required')
 def test_atlas_has_no_missing_glyphs(self):
  from PIL import ImageFont
  font=ImageFont.truetype(str(ROOT/'assets/generated/fonts/CaoCaoSong.ttf'),16)
  missing=font.getmask(chr(0x10ffff),mode='1')
  absent=(missing.size,bytes(missing))
  glyphs=json.loads((ROOT/'assets/generated/font.json').read_text())['glyphs']
  for code in glyphs:
   ch=chr(int(code))
   if ch.isspace():continue
   actual=font.getmask(ch,mode='1')
   self.assertNotEqual((actual.size,bytes(actual)),absent,f'missing atlas glyph U+{ord(ch):04X} {ch}')
   self.assertTrue(any(actual),f'blank atlas glyph U+{ord(ch):04X} {ch}')
 def test_unknown_opcode_exact_offset(self):
  with self.assertRaisesRegex(FormatError,'fixture.eex:0x0000: unknown opcode'):
   Reader(b'\xff\xff','fixture.eex').instruction(0)
 @unittest.skipUnless((ROOT/'assets/generated/data.json').exists(),'import required')
 def test_original_unit_loader_and_portraits(self):
  fixtures=json.loads((ROOT/'tests/fixtures/original_units.json').read_text())
  heroes=json.loads((ROOT/'assets/generated/data.json').read_text())['heroes']
  for expected in fixtures['units']:
   for field,value in expected.items():self.assertEqual(heroes[expected['id']][field],value,f"unit {expected['id']}: {field}")
 @unittest.skipUnless((ROOT/'assets/generated/manifest.json').exists(),'import required')
 def test_manifest_and_source_preserved(self):
  manifest=json.loads((ROOT/'assets/generated/manifest.json').read_text());expected=manifest.pop('hash');canonical=json.dumps(manifest,ensure_ascii=False,sort_keys=True,separators=(',',':')).encode();self.assertEqual(hashlib.sha256(canonical).hexdigest(),expected)
  for asset in manifest['assets']:
   p=ROOT/'assets/generated'/asset['path'];self.assertEqual(hashlib.sha256(p.read_bytes()).hexdigest(),asset['sha256'],asset['path'])
  if SOURCE.exists():
   for source in manifest['sources']:self.assertEqual(hashlib.sha256((SOURCE/source['path']).read_bytes()).hexdigest(),source['sha256'],source['path'])

 @unittest.skipUnless((ROOT/'assets/generated/ui.json').exists(),'import required')
 def test_banner_text_and_glyph_coverage(self):
  assets=ROOT/'assets/generated';labels=json.loads((assets/'ui.json').read_text())['turn_labels']
  self.assertEqual({k:r['text'] for k,r in labels.items()},{'player':'我军阶段','enemy':'敌军阶段','final':'最终回合','turn':'第%2u回合'})
  self.assertEqual([labels[k]['address'] for k in ('player','enemy','final','turn')],[0x48e154,0x48e160,0x48e16c,0x48e178])
  objective=json.loads((assets/'ui.json').read_text())['objective_labels']
  self.assertEqual({k:r['text'] for k,r in objective.items()},{'title':'胜利条件','limit':'限制回合数%3u','start':'开始作战'})
  self.assertEqual([objective[k]['address'] for k in ('title','limit')],[0x48e1d4,0x48e1e0])
  for size in (32,48,64):
   atlas=json.loads((assets/f'banner-font-{size}.json').read_text());self.assertEqual(atlas['size'],size)
   for ch in '0123456789 击毙张宝和张梁！'+''.join(r['text'] for r in [*labels.values(),*objective.values()]):self.assertIn(str(ord(ch)),atlas['glyphs'])
  atlas=json.loads((assets/'font.json').read_text())
  for p in [*Path(ROOT/'src').glob('*.cpp'),*Path(ROOT/'web').glob('*.html')]:
   for ch in set(p.read_text()):
    if ord(ch)>127:self.assertTrue(str(ord(ch)) in atlas['glyphs'],f'{p.name}: missing glyph {ch}')

 @unittest.skipUnless((ROOT/'assets/generated/ui.json').exists(),'import required')
 def test_tactic_labels_and_glyph_coverage(self):
  from ui_assets import decode_tactic_labels
  assets=ROOT/'assets/generated';labels=json.loads((assets/'ui.json').read_text())['tactic_labels']
  self.assertEqual(labels,decode_tactic_labels((SOURCE/'Ekd5.exe').read_bytes()))
  self.assertEqual(labels,json.loads((ROOT/'tests/fixtures/original_tactic_panel.json').read_text())['ui']['tactic_labels'])
  glyphs=json.loads((assets/'font.json').read_text())['glyphs']
  for row in labels['types']+[labels['damage'],labels['hit']]:
   for ch in row['text']:self.assertIn(str(ord(ch)),glyphs)

 @unittest.skipUnless((ROOT/'assets/generated/images/Mark/caption.bmp').exists(),'import required')
 def test_caption_pixels_and_tactic_names(self):
  from PIL import Image
  assets=ROOT/'assets/generated';ui=json.loads((assets/'ui.json').read_text());meta=ui['mark_images']['caption']
  self.assertEqual(meta,dict(offset=0x18dc0,width=48,height=48))
  raw=(SOURCE/'Mark.e5').read_bytes()[0x18dc0:0x196c0]
  reference=json.loads((ROOT/'tests/fixtures/original_caption_window.json').read_text())['cases'][0]['events'][0]
  self.assertEqual(hashlib.sha256(raw).hexdigest(),reference['pixels_sha256'])
  image=Image.open(assets/'images/Mark/caption.bmp').convert('RGB')
  self.assertEqual(image.size,(48,48));self.assertEqual(image.tobytes(),bytes(c for index in raw for c in ui['palette'][index]))
  tactics=json.loads((assets/'tactics.json').read_text());glyphs=json.loads((assets/'font.json').read_text())['glyphs']
  for index in (23,25,33):
   for ch in tactics[index]['name']:self.assertIn(str(ord(ch)),glyphs)

 @unittest.skipUnless((ROOT/'assets/generated/images/Mark/fire.bmp').exists(),'import required')
 def test_fire_pixels(self):
  from PIL import Image
  assets=ROOT/'assets/generated';ui=json.loads((assets/'ui.json').read_text())
  self.assertEqual(ui['mark_images']['fire'],dict(offset=0x8bc0,width=48,height=192))
  raw=(SOURCE/'Mark.e5').read_bytes()[0x8bc0:0xafc0]
  image=Image.open(assets/'images/Mark/fire.bmp').convert('RGB')
  self.assertEqual(image.size,(48,192))
  self.assertEqual(image.tobytes(),bytes(c for index in raw for c in ui['palette'][index]))

 def test_poison_effect_layout(self):
  from meff import decode_poison
  raw=decode((SOURCE/'Meff.e5').read_bytes(),indices={23})[0].data
  effect=decode_poison(raw);self.assertEqual(effect,json.loads((ROOT/'assets/generated/poison-effect.json').read_text()))
  fixture=json.loads((ROOT/'tests/fixtures/original_tactic_effect.json').read_text())
  self.assertEqual(hashlib.sha256(raw).hexdigest(),fixture['resource_sha256'])
  frames=[e for e in fixture['cases'][0]['events'] if e['kind']=='effect']
  self.assertEqual([f['pixels_sha256'] for f in effect['frames']],[f['pixels_sha256'] for f in frames])
  for invalid in (raw[:-1],raw+b'\0',bytes([14])+raw[1:],raw[:20]+bytes([255])+raw[21:]):
   with self.assertRaises(FormatError):decode_poison(invalid)

 def test_confusion_effect(self):
  from meff import decode_secondary
  raw=next(b.data for b in decode((SOURCE/'Meff.e5').read_bytes(),indices={48}))
  meta=decode_secondary(raw,48);fixture=json.loads((ROOT/'tests/fixtures/original_confusion.json').read_text())
  self.assertEqual(meta,json.loads((ROOT/'assets/generated/effect-48.json').read_text()))
  self.assertEqual(hashlib.sha256(raw).hexdigest(),fixture['resource_sha256'])
  self.assertEqual(len(meta['frames']),13)
  self.assertEqual(sum(f['weight']==0 for f in meta['frames']),3)
  for invalid in (raw[:-1],raw+b'\0',bytes([255])+raw[1:],raw[:20]+bytes([255])+raw[21:]):
   with self.assertRaises(FormatError):decode_secondary(invalid,48)

 def test_secondary_effects(self):
  from meff import decode_secondary
  fixture=json.loads((ROOT/'tests/fixtures/original_secondary_effect.json').read_text())
  for block in decode((SOURCE/'Meff.e5').read_bytes(),indices={45,49}):
   raw=block.data;meta=decode_secondary(raw,block.index)
   self.assertEqual(meta,json.loads((ROOT/f'assets/generated/effect-{block.index}.json').read_text()))
   self.assertEqual(hashlib.sha256(raw).hexdigest(),fixture['resources'][str(block.index)])
   row=next(c for c in fixture['cases'] if c['resource']==block.index)
   frames=[e for e in row['events'] if e['kind']=='effect']
   self.assertEqual([f['pixels_sha256'] for f in meta['frames']],[f['pixels_sha256'] for f in frames])
   self.assertEqual([(f['weight'],f['amount']) for f in meta['frames']],[(f['weight'],f['amount']) for f in frames])
   for invalid in (raw[:-1],raw+b'\0',bytes([255])+raw[1:],raw[:20]+bytes([255])+raw[21:]):
    with self.assertRaises(FormatError):decode_secondary(invalid,block.index)
   with self.assertRaises(FormatError):decode_secondary(raw,50)

if __name__=='__main__':unittest.main()
