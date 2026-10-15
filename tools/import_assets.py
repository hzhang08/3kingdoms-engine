#!/usr/bin/env python3
"""Import the user's original files read-only; publish a complete manifest last."""
import argparse,hashlib,json,os,shutil,struct,tempfile
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import pefile
from ls12 import decode,FormatError
from eex import parse
from tactics import decode_tactics
from mcall import decode_mcall
from meff import decode_poison,decode_secondary
from ui_assets import decode_ui,decode_attack_labels,decode_turn_labels,decode_objective_labels,decode_tactic_labels,decode_tutorial_text,decode_terrain_labels,darken_palette,combat_palette,MARK_IMAGES
from combat_assets import decode_critical_speeches

EDITION='199b1b67cf966ead46c49b430b9aab84bdc7d4f0e8dfa8e0ebeec5da280b01e0'

def sha(b):return hashlib.sha256(b).hexdigest()
def canonical(v):return json.dumps(v,ensure_ascii=False,sort_keys=True,separators=(',',':')).encode()
def untile(data,w,h,tile):
    if len(data)!=w*h or w%tile or h%tile:raise FormatError('invalid tiled image dimensions')
    result=bytearray(w*h)
    for n in range(len(data)//tile**2):
        x=(n%(w//tile))*tile;y=(n//(w//tile))*tile
        for row in range(tile):result[(y+row)*w+x:(y+row)*w+x+tile]=data[n*tile**2+row*tile:n*tile**2+(row+1)*tile]
    return bytes(result)
def palette(raw):
    if len(raw)!=768:raise FormatError('opening palette must contain 256 entries')
    return b''.join(raw[i+1:i+3]+raw[i:i+1] for i in range(0,len(raw),3))

def recover_publication(out):
    """Recover a process interruption between the two directory renames."""
    backup=out.with_name(out.name+'.previous')
    if not out.exists() and backup.exists():backup.rename(out)

def publish(staging,out):
    backup=out.with_name(out.name+'.previous')
    if backup.exists():shutil.rmtree(backup)
    if out.exists():out.rename(backup)
    try:staging.rename(out)
    except OSError:
        recover_publication(out)
        raise

def run(source,out,font):
    source=source.resolve();out=out.resolve();font=font.resolve()
    if out==source or source in out.parents or out in source.parents:raise ValueError('output must be separate from original game')
    if not (source/'Ekd5.exe').is_file():raise ValueError('game directory does not contain Ekd5.exe')
    if sha((source/'Ekd5.exe').read_bytes())!=EDITION:raise ValueError('this importer is verified only for the supplied executable edition')
    licensefile=font.with_name('SourceHanSerif-LICENSE.txt')
    if not font.is_file() or not licensefile.is_file():raise ValueError('font and adjacent SourceHanSerif-LICENSE.txt are required')
    recover_publication(out)
    source_files=[{'path':str(p.relative_to(source)),'size':p.stat().st_size,'sha256':sha(p.read_bytes())} for p in sorted(source.rglob('*')) if p.is_file()]
    out.parent.mkdir(parents=True,exist_ok=True)
    staging=Path(tempfile.mkdtemp(prefix='.import-',dir=out.parent))
    assets=[];cache={}
    def write(relative,data,origin,**meta):
        p=staging/relative;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
        assets.append({'path':relative,'sha256':sha(data),'source':origin,**meta})
    def archive(name,indices=None):
        blocks=decode((source/name).read_bytes(),indices=indices)
        for b in blocks:
            cache[(name,b.index)]=b
            write(f'raw/{name}/{b.index:03}.bin',b.data,f'{name}#{b.index}',offset=b.offset,packed_size=b.packed_size)
        return {b.index:b.data for b in blocks}
    def bitmap(relative,raw,w,h,pal,origin,tile=0):
        if tile:raw=untile(raw,w,h,tile)
        im=Image.frombytes('P',(w,h),raw);im.putpalette(pal)
        p=staging/relative;p.parent.mkdir(parents=True,exist_ok=True);im.convert('RGB').save(p)
        assets.append({'path':relative,'sha256':sha(p.read_bytes()),'source':origin,'width':w,'height':h,'index_pixels_sha256':sha(raw),'tile':tile})
    scripts={name:parse(source/(name+'.eex')) for name in ('R_00','S_00')}
    for name,value in scripts.items():write('scripts/'+name+'.json',canonical(value),name+'.eex')
    pmap=palette(archive('Pmpalet.e5',{71})[71]);spal_raw=archive('Spalet.e5',{0})[0];spal=palette(spal_raw)
    ui=decode_ui((source/'Ekd5.exe').read_bytes())
    ui['palette']=[list(spal[n:n+3]) for n in range(0,768,3)]
    ui['darken']=darken_palette(spal_raw)
    ui['combat_palette']=combat_palette(spal_raw)
    ui['attack_labels']=decode_attack_labels((source/'Ekd5.exe').read_bytes())
    ui['turn_labels']=decode_turn_labels((source/'Ekd5.exe').read_bytes())
    ui['objective_labels']=decode_objective_labels((source/'Ekd5.exe').read_bytes())
    from tutorial_menu_resource import generate as command_menu_resource
    ui['command_menu']=command_menu_resource(source)
    ui['terrain_labels']=decode_terrain_labels((source/'Ekd5.exe').read_bytes())
    ui['tutorial_text']=decode_tutorial_text((source/'Ekd5.exe').read_bytes())
    ui['tactic_labels']=decode_tactic_labels((source/'Ekd5.exe').read_bytes())
    speeches=decode_critical_speeches((source/'Imsg.e5').read_bytes())
    write('combat_text.json',canonical(speeches),'Imsg.e5:records700..750')
    ui['mark_images']={name:dict(offset=offset,width=w,height=h) for name,offset,w,h in MARK_IMAGES}
    write('ui.json',canonical(ui),'Ekd5.exe:0x48bea8,0x48bdb0,0x48e058,0x48e154..0x48e178,0x48e1c8,0x48bd40,0x48e0a4,0x48e0b0;Spalet.e5#0;Mark.e5')
    mark=(source/'Mark.e5').read_bytes()
    if len(mark)!=0x1c7e0:raise FormatError('unexpected Mark.e5 size')
    write('raw/Mark.e5/000.bin',mark,'Mark.e5')
    write('raw/Hm00.e5/000.bin',(source/'Hm00.e5').read_bytes(),'Hm00.e5')
    quantized=ui['combat_palette']['quantized'];lookup=ui['combat_palette']['lookup']
    gray=[0]+[lookup[(sum(quantized[n*3:n*3+3])//3)*0x111] for n in range(1,256)]
    for n in range(4):
        offset=0x1a5c0+n*256;pixels=bytes(gray[i] for i in mark[offset:offset+256])
        bitmap('images/Mark/tactic-'+str(n)+'-gray.bmp',pixels,16,16,spal,'Mark.e5@'+hex(offset)+';Ekd5.exe:0x43087a')
    for name,offset,w,h in MARK_IMAGES:
        bitmap(f'images/Mark/{name}.bmp',mark[offset:offset+w*h],w,h,spal,f'Mark.e5:0x{offset:x}')
    scene=archive('Mmap.e5',{71})[71]
    bitmap('images/story.bmp',scene,640,400,pmap,'Mmap.e5#71',8)
    terrain=archive('Hexzmap.e5',{0})[0];w,h=terrain[0]//3,terrain[1]//3
    if w*h+2!=len(terrain):raise FormatError('terrain size mismatch')
    write('terrain.json',canonical({'width':w,'height':h,'cells':list(terrain[2:])}),'Hexzmap.e5#0')
    bitmap('images/battlefield.bmp',(source/'Hm00.e5').read_bytes(),w*48,h*48,spal,'Hm00.e5',48)
    data=archive('Data.e5');heroes=[]
    for index in range(len(data[0])//32):
        r=data[0][index*32:(index+1)*32]
        # 0x408be3 reads the face base from bytes 13..14. At campaign start,
        # 0x4075a7 selects Cao Cao frame 0, Xiahou Dun frame 223, otherwise
        # base + 7 (later campaign/expression flags can change these).
        face_base=int.from_bytes(r[13:15],'little')
        face=0 if index==0 else 223 if index==1 else face_base+7
        heroes.append({'id':index,'name':r[:12].split(b'\0')[0].decode('gbk'),'face_base':face_base,'face':face,'story_sprite':r[15], 'base_stats':list(r[18:23]),'base_hp':min(int.from_bytes(r[23:25],'little'),500),'base_mp':min(r[25],250),'class':r[26],'initial_owner':r[17],'initial_level':max(r[27],1),'initial_xp':r[28],'initial_equipment':list(r[29:32]),'raw':r.hex()})
    classes=[list(data[3][i:i+27]) for i in range(0,len(data[3]),27)]
    items=[]
    for index in range(104):
        r=data[1][index*25:(index+1)*25]
        items.append({'id':index,'name':r[:16].split(b'\0')[0].decode('gbk'),'type':r[17],'class_group':r[23],'strength':r[21],'special_type':r[18],'special_strength':r[22],'collection':r[24],'raw':r.hex()})
    tactics=decode_tactics(data[5])
    effects=archive('Meff.e5',{23,45,48,49})
    write('poison-effect.json',canonical(decode_poison(effects[23])),'Meff.e5#23')
    for resource in (45,48,49):write(f'effect-{resource}.json',canonical(decode_secondary(effects[resource],resource)),f'Meff.e5#{resource}')
    mcall=(source/'Mcall05.e5').read_bytes()
    write('raw/Mcall05.e5/000.bin',mcall,'Mcall05.e5')
    write('mcall-05.json',canonical(decode_mcall(mcall)),'Mcall05.e5')
    write('tactics.json',canonical(tactics),'Data.e5#5')
    write('data.json',canonical({'heroes':heroes,'classes':classes,'items':items,'terrain_raw':list(data[4]),'verified_fields':['name','base_stats','base_hp','base_mp','class','initial_owner','equipment_type','equipment_strength','equipment_special_type','equipment_special_strength','equipment_class_group_or_growth','equipment_collection','opening_battle_sprite_selection','terrain_movement_costs'],'unresolved_fields':['complete_item_combat_effects','item_raw_fields_19_20']}),'Data.e5')
    for name,width,height in [('Face.e5',64,80),('Unit_mov.e5',48,528),('Unit_spc.e5',48,240),('Unit_atk.e5',64,768),('Item.e5',32,32)]:
        for index,raw in archive(name).items():
            if len(raw)!=width*height:raise FormatError(f'{name}#{index}: unexpected dimensions')
            bitmap(f'images/{Path(name).stem}/{index:03}.bmp',raw,width,height,spal,f'{name}#{index}')
    # Story sprite archive has 20 frames of 48x64 each; retain all pixels.
    story_ids={0,157,181,182}
    story_images={heroes[hero]['story_sprite']*2+direction for hero in story_ids for direction in (0,1)}
    for index,raw in archive('Pmapobj.e5',story_images).items():
        bitmap(f'images/Pmapobj/{index:03}.bmp',raw,48,len(raw)//48,pmap,f'Pmapobj.e5#{index}')
    for index,raw in archive('U_select.e5').items():
        width={0:50,1:50,2:50,3:640,4:640}[index]
        bitmap(f'images/U_select/{index:03}.bmp',raw,width,len(raw)//width,spal,f'U_select.e5#{index}')
    # PE resource reading never imports or executes any DLL.
    for filename in ['Ekd5.exe','Cmdicon.dll','Itemicon.dll','Mgcicon.dll','Mapatr.dll']:
        pe=pefile.PE(data=(source/filename).read_bytes())
        for kind in getattr(pe,'DIRECTORY_ENTRY_RESOURCE',type('Empty',(),{'entries':[]})()).entries:
            if kind.id!=2:continue
            for ident in kind.directory.entries:
                for lang in ident.directory.entries:
                    d=lang.data.struct;raw=pe.get_data(d.OffsetToData,d.Size)
                    write(f'raw/pe/{filename}/{ident.id}-{lang.id}.dib',raw,f'{filename}:RT_BITMAP:{ident.id}:{lang.id}',rva=d.OffsetToData)
                    import io
                    im=Image.open(io.BytesIO(raw));p=staging/f'images/pe/{filename}-{ident.id}-{lang.id}.bmp';p.parent.mkdir(parents=True,exist_ok=True);im.convert('RGB').save(p)
                    assets.append({'path':str(p.relative_to(staging)),'sha256':sha(p.read_bytes()),'source':f'{filename}:RT_BITMAP:{ident.id}:{lang.id}'})
    for p in sorted(source.glob('*.wav')):write('audio/'+p.name,p.read_bytes(),p.name)
    tracks=(source/'mp3list.txt').read_text(encoding='gbk').splitlines()
    music={}
    for index,path in enumerate(tracks):
        if not path:continue
        p=source/Path(path.replace('\\','/'))
        if p.exists():
            relative=f'audio/track-{index}.mp3';write(relative,p.read_bytes(),str(p.relative_to(source)));music[str(index)]=relative
    write('music.json',canonical(music),'mp3list.txt')
    chars=set(canonical(scripts).decode()+''.join(h['name'] for h in heroes)+'三国志曹操传颍川之战新游戏继续存档读取保存槽导入导出确认取消返回结束回合攻击移动策略物品待机失败胜利调试尚未验证空选择说明乱码教程关闭帮助级生命精神经验士气防御敏捷敌军友军我军回合完成阶段右键点击空格上下左右')
    chars.update(''.join(t['name'] for t in tactics))
    chars.update(''.join(r['text'] for r in ui['classes']+ui['terrain']+list(ui['labels'].values())))
    chars.update(''.join(r['text'] for r in speeches+list(ui['attack_labels'].values())))
    chars.update(''.join(r['text'] for r in ui['turn_labels'].values()))
    chars.update(''.join(r['text'] for r in ui['objective_labels'].values()))
    chars.update(''.join(r['text'] for r in ui['tutorial_text'].values()))
    chars.update(''.join(r['text'] for r in ui['terrain_labels'].values()))
    chars.update(''.join(c['text'] for c in ui['command_menu']['controls']))
    chars.update(''.join(r['text'] for r in ui['tactic_labels']['types']+[ui['tactic_labels']['damage'],ui['tactic_labels']['hit']]))
    for path in list(Path('src').glob('*.cpp'))+list(Path('web').glob('*.html')):
        chars.update(path.read_text())
    chars.update(chr(i) for i in range(32,127));chars=sorted(chars-{'\n','\r','\t'})
    glyphfont=ImageFont.truetype(str(font),16);cols=64;cell=20;atlas=Image.new('RGB',(cols*cell,((len(chars)+cols-1)//cols)*cell),(247,0,255));draw=ImageDraw.Draw(atlas);draw.fontmode='1';glyphs={}
    for i,ch in enumerate(chars):
        x=i%cols*cell;y=i//cols*cell
        draw.text((x,y+16),ch,font=glyphfont,fill='white',anchor='ls');glyphs[str(ord(ch))]={'x':x,'y':y,'advance':round(glyphfont.getlength(ch))}
    p=staging/'images/font.bmp';atlas.save(p);assets.append({'path':'images/font.bmp','sha256':sha(p.read_bytes()),'source':'CaoCaoSong.ttf','font_sha256':sha(font.read_bytes())})
    write('font.json',canonical({'cell':cell,'size':16,'glyphs':glyphs}),'CaoCaoSong.ttf')
    # Draw large titles at their requested size; do not magnify 16px glyphs.
    def objective_text(body):
        for i in body:
            if i['opcode']==26:yield i['args'][0]['value']
            if 'body' in i:yield from objective_text(i['body'])
    objectives=''.join(text for script in scripts.values() for section in script['sections'] for event in section['events'] for text in objective_text(event['body']))
    banner_chars=sorted(set('0123456789 '+objectives+''.join(r['text'] for r in list(ui['turn_labels'].values())+list(ui['objective_labels'].values()))))
    for size in (32,48,64):
        cell=size+8;cols=16;atlas=Image.new('RGB',(cols*cell,((len(banner_chars)+cols-1)//cols)*cell),(247,0,255));draw=ImageDraw.Draw(atlas);draw.fontmode='1';glyphfont=ImageFont.truetype(str(font),size);glyphs={}
        for n,ch in enumerate(banner_chars):
            x=n%cols*cell;y=n//cols*cell;draw.text((x,y+size),ch,font=glyphfont,fill='white',anchor='ls');glyphs[str(ord(ch))]={'x':x,'y':y,'advance':size//2 if ord(ch)<128 else size}
        p=staging/f'images/banner-font-{size}.bmp';atlas.save(p);assets.append({'path':str(p.relative_to(staging)),'sha256':sha(p.read_bytes()),'source':'CaoCaoSong.ttf','font_sha256':sha(font.read_bytes())})
        write(f'banner-font-{size}.json',canonical({'cell':cell,'size':size,'glyphs':glyphs}),'CaoCaoSong.ttf')
    write('fonts/CaoCaoSong.ttf',font.read_bytes(),'CaoCaoSong.ttf')
    write('fonts/OFL.txt',licensefile.read_bytes(),licensefile.name)
    manifest={'schema':1,'importer':'0.19.0','sources':source_files,'assets':sorted(assets,key=lambda x:x['path'])}
    manifest['hash']=sha(canonical(manifest));(staging/'manifest.json').write_bytes(canonical(manifest))
    # Detect source changes before publishing. Never copy original saves into runtime storage.
    for item in source_files:
        if sha((source/item['path']).read_bytes())!=item['sha256']:raise RuntimeError('source changed during import: '+item['path'])
    publish(staging,out)
    print(json.dumps({'output':str(out),'manifest':manifest['hash'],'assets':len(assets),'source_files':len(source_files)},indent=2))

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('source',type=Path);parser.add_argument('--out',type=Path,default=Path('assets/generated'));parser.add_argument('--font',type=Path,required=True);args=parser.parse_args();run(args.source,args.out,args.font)
