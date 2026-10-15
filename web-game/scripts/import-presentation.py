#!/usr/bin/env python3
"""Read original title artwork and soundtrack; preserve the supplied source."""
import hashlib,json,sys
from pathlib import Path
from PIL import Image
root=Path(__file__).resolve().parents[2];sys.path.insert(0,str(root/'tools'))
from ls12 import decode
from import_assets import palette,EDITION
source=Path(sys.argv[1]) if len(sys.argv)>1 else Path('/Users/henryzhang/Downloads/sgzccz')
out=root/'web-game/public';(out/'music').mkdir(parents=True,exist_ok=True)
sha=lambda b:hashlib.sha256(b).hexdigest()
assert sha((source/'Ekd5.exe').read_bytes())==EDITION
raw=(source/'Logo.e5').read_bytes();palraw=(source/'Logo_p.e5').read_bytes()
pixels=decode(raw,indices={0})[0].data;pal=decode(palraw,indices={0})[0].data
assert len(pixels)==640*400 and len(pal)==738 and max(pixels)<246
im=Image.frombytes('P',(640,400),pixels);im.putpalette(palette(pal+bytes(768-len(pal))))
im.convert('RGB').save(out/'entry.png')
tracks=(source/'mp3list.txt').read_text(encoding='gbk').splitlines()
# Existing script interpreter stores the opcode36 track number. The original
# playback adapter maps it to the one-based mp3list index (script ID +1). The
# opening playable preset begins after the arrival event (track5); S01's last
# initialization request is2, S02 is3 and S03 is4. Source MP3 bytes are unchanged.
music=[]
for track in [2,3,4,5]:
 p=source/tracks[track+1].replace('\\','/');data=p.read_bytes();(out/f'music/track-{track}.mp3').write_bytes(data)
 music.append({'script_track':track,'playlist_index':track+1,'source':str(p.relative_to(source)),'sha256':sha(data),'bytes':len(data)})
manifest={'title':{'source':'Logo.e5#0','palette':'Logo_p.e5#0','source_hash':sha(raw),'palette_hash':sha(palraw),'pixels_sha256':sha(pixels),'size':[640,400]},'battle_tracks':[5,2,3,4],'music':music}
(out/'presentation-manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
# Small icon uses original title pixels, not generated art.
im.convert('RGB').crop((160,20,480,340)).resize((192,192)).save(out/'icon.png')
print('Imported original title and four unmodified MP3 tracks.')
