#!/usr/bin/env python3
"""Losslessly crop/pack approved voxel frames into the special duel runtime."""
import hashlib,json,shutil,struct,subprocess,sys
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parent.parent
SOURCE=ROOT/'assets/special-hulao';TARGET=ROOT/'assets/prototype/special-hulao'
TARGET.mkdir(parents=True,exist_ok=True)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
selection=json.loads((SOURCE/'roster.json').read_text())
assert selection['keep_ids']==[0,6,1,11,119,127]
units={};sources={};wanted=set()
for identity,slug,body_height in [(0,'caocao',332),(6,'xiahouyuan',260),(1,'xiahoudun',260),(11,'lidian',260),(119,'lubu',260),(127,'dongzhuo',260)]:
 folder=ROOT/f'output/character-art/{slug}-voxel-sprites'
 library=json.loads((folder/'manifest.json').read_text())
 assert library['character']['id']==identity and len(library['frames'])==28
 frames={'move':[],'attack':[],'special':[]}
 for frame in library['frames']:
  source=folder/frame['file'];assert sha(source)==frame['sha256'],f'Edited source: {source}'
  image=Image.open(source).convert('RGBA');bounds=image.getchannel('A').getbbox();image=image.crop(bounds)
  filename=f'unit-{identity}-{frame["group"]}-{frame["index"]:02d}.rgba';wanted.add(filename)
  raw=struct.pack('<II',*image.size)+image.tobytes();(TARGET/filename).write_bytes(raw)
  assert frame['index']==len(frames[frame['group']])
  frames[frame['group']].append({'file':filename,'w':image.width,'h':image.height,
    'pivot':[frame['pivot'][0]-bounds[0],frame['pivot'][1]-bounds[1]],
    'sha256':hashlib.sha256(raw).hexdigest(),'source_sha256':frame['sha256'],'source_rect':list(bounds)})
 assert [len(frames[g]) for g in ['move','attack','special']]==[11,12,5]
 units[str(identity)]={'name':library['character']['name'],'height':132,'body_height':body_height,'frames':frames}
 sources[str(identity)]={'library':str(folder.relative_to(ROOT)),'manifest_sha256':sha(folder/'manifest.json'),
  'native_event_sha256':sha(folder/'native-events.json'),'frames':28}
# Only generated unit files in the staging directory; keep original art/seed.
for path in TARGET.glob('unit-*.rgba'):
 if path.name not in wanted:path.unlink()
terrain=Image.open(SOURCE/'source/terrain.png').convert('RGB');assert terrain.size==(1536,1024)
terrain.save(TARGET/'terrain.bmp');shutil.copy2(SOURCE/'seed.json',TARGET/'seed.json')
manifest={'version':'hulao-voxel-duel-heroes-v1','legacy_duel_manifests':['a31262e53727bc447f0c2bb9edebd9d13c3b205253b994c09d2ab00176fa7c4b'],'logical_size':[1536,1184],'roster':selection,
 'camera':'fixed raster backdrop with projected, animated RGBA voxel miniatures',
 'units':units,'sources':sources,'terrain_sha256':sha(TARGET/'terrain.bmp'),'seed_sha256':sha(TARGET/'seed.json')}
manifest['hash']=hashlib.sha256(json.dumps(manifest,ensure_ascii=False,sort_keys=True).encode()).hexdigest()
for p in [SOURCE/'manifest.json',TARGET/'manifest.json']:p.write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
print(f'Staged 6 characters / 168 animation frames: {manifest["hash"]}')
subprocess.run([sys.executable,str(ROOT/'scripts/stage-special-camps.py')],check=True)
