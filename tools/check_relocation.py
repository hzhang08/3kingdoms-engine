#!/usr/bin/env python3
"""Run the packaged Linux checkpoint with no network or development mount."""
from pathlib import Path
import hashlib,json,subprocess,uuid
from PIL import Image
root=Path(__file__).resolve().parents[1]
version='0.22'
name='caocao-relocation-'+uuid.uuid4().hex[:10]
location='/opt/test/三国 游戏'
def run(*args):return subprocess.check_output(['docker',*args],text=True).strip()
created=False
try:
    run('create','--name',name,'--network','none','--platform','linux/amd64',
        '-e','SDL_AUDIODRIVER=dummy','-e','XDG_RUNTIME_DIR=/tmp','caocao-engine-build:0.1','sleep','600')
    created=True;run('start',name);run('exec',name,'mkdir','-p',location)
    run('cp',str(root/'dist/linux')+'/.',name+':'+location)
    trace=json.loads(run('exec','-w',location,name,'./caocao_trace','assets','1',
                        '--event-snapshot','--snapshot-out','event.json'))
    expected=json.loads((root/f'docs/evidence/parity-{version}.json').read_text())['choices'][0]
    assert trace['final_hash']==expected['final_hash'] and len(trace['trace'])==expected['checkpoints']
    restored=json.loads(run('exec','-w',location,name,'./caocao_trace','assets','--restore','event.json'))
    native_event=json.loads(subprocess.check_output([str(root/'build-core/caocao_trace'),str(root/'assets/generated'),'--restore',str(root/'.cache/event-0.22.json')],text=True))
    assert restored==native_event
    run('exec','-w',location,name,'xvfb-run','-a','./run.sh','--render-event-smoke')
    bmp=root/f'.cache/relocated-event-{version}.bmp'
    run('cp',name+':'+location+'/render-event-smoke.bmp',str(bmp))
    Image.open(bmp).save(root/f'docs/evidence/linux-relocated-event-{version}.png')
    for mode in ('preview','critical','attack','damage','defeat','after-defeat','banner','reinforcement','scripted-walk','after-reinforcement','enemy-walk','enemy-ready','tactic-preview','tactic-cast','poison','tactic-damage','tactic-cost','archer-dialogue','archer-defeat','second-poison','poison-status','enemy-end','next-banner','next-turn','fire','three-fires','summon-dialogue','summon','summon-end','all-fires','confusion','confusion-direct','arrival'):
        run('exec','-w',location,name,'xvfb-run','-a','./run.sh','--render-'+mode+'-smoke')
        bmp=root/f'.cache/relocated-{mode}-{version}.bmp'
        run('cp',name+':'+location+'/render-'+mode+'-smoke.bmp',str(bmp))
        Image.open(bmp).save(root/f'docs/evidence/linux-relocated-{mode}-{version}.png')
    info=json.loads(run('inspect',name))[0]
    assert info['HostConfig']['NetworkMode']=='none' and not info['Mounts']
    result=dict(engine=trace['engine'],container_network='none',container_path=location,
                development_directory_mounted=False,event_render='passed',attack_rendering=['preview','critical','attack','damage','defeat','after-defeat'],new_rendering=['banner','reinforcement','scripted-walk','after-reinforcement','enemy-walk','enemy-ready','tactic-preview','tactic-cast','poison','tactic-damage','tactic-cost','archer-dialogue','archer-defeat','second-poison','poison-status','enemy-end','next-banner','next-turn','fire','three-fires','summon-dialogue','summon','summon-end','all-fires','confusion','confusion-direct','arrival'],restored_hash=restored['restored_hash'],
                final_hash=trace['final_hash'],checkpoints=len(trace['trace']),
                binary_sha256={n:hashlib.sha256((root/'dist/linux'/n).read_bytes()).hexdigest() for n in ('caocao','caocao_trace')},
                limitations=['not a full battle test','Debian 12 container, not all Linux distributions','wall-clock idle animation prevents pixel-identity assertion'])
    (root/f'docs/evidence/relocation-{version}.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps(result,ensure_ascii=False,indent=2))
finally:
    if created:run('rm','-f',name)
