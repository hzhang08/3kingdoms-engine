#!/usr/bin/env python3
"""Original selected-unit move/menu/wait/cancel controller at host boundaries.

43dde3 and43d538 execute with supplied tile-picker inputs. The real menu
initializer and command/cancel callbacks execute around a supplied Windows
modal result; disabled commands are rejected. Attack/tactic/item execution
stops explicitly. Roster activation, camera and host dimensions are test inputs,
not a continuous turn-two or real-window acceptance run.
"""
import argparse,json,struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_ECX
from battle_events_probe import prepare,LIVE
from initialization_probe import UNIT,state
from movement_probe import live_json
from player_movement_probe import configure
from tutorial_menu_probe import host,DIALOG
from executable_probe import EDITION

class Modal(Exception):pass

def configure_action(v):
    events,control=configure(v)
    windows=host(v,(110,154),(0,0))
    v.ranges += [(0x43dde3,0x43e478),(0x4350c0,0x4350e8),
      (0x45e69a,0x45e6b5),(0x46d098,0x46d0c8),(0x40d9ee,0x40da45)]
    v.uc.mem_map(0xf00000,0x10000)
    def modal(vm):
        assert vm.arg(1)==0x117 and vm.arg(4)==DIALOG
        raise Modal()
    def clear_panel(vm):events.append(dict(kind='clear_panel'));vm.return_from_hook()
    def invisible(vm):vm.return_from_hook(0,4)
    def release(vm):
        assert vm.arg(1)==0x202
        events.append(dict(kind='release_button'));vm.return_from_hook(0,16)
    def redraw(vm):events.append(dict(kind='minimap'));vm.return_from_hook()
    def stop(vm):raise RuntimeError('unverified attack/tactic/item action reached')
    v.put32(0x4862a0,0x83e100);v.put32(0x4862c8,0x83e110);v.put32(0x4862f4,0x83e120)
    v.hooks.update({0x83e100:modal,0x83e110:invisible,0x83e120:release,
      0x453e32:clear_panel,0x44a88a:redraw,0x43dada:stop,0x43dc21:stop,0x43dcdc:stop})
    range_before=[]
    def grids():
        return dict(distance=list(v.read(0x8192c0,400)),threat=list(v.read(0x824000,400)),mask=list(v.read(0x824c80,400)))
    def observer(uc,address,size,user):
        if address==0x43dea0:range_before.append(dict(grids=grids(),preferred=v.read(0x4b2c28,1)[0]))
    v.uc.hook_add(UC_HOOK_CODE,observer)
    def aux(address,args=()):
        sp=0xf08000;v.write(sp,struct.pack('<'+'I'*(1+len(args)),0x83f000,*args))
        v.uc.reg_write(UC_X86_REG_ESP,sp);v.uc.reg_write(UC_X86_REG_ECX,DIALOG)
        v.uc.emu_start(address,0x83f000,count=100000)
        assert v.uc.reg_read(UC_X86_REG_EIP)==0x83f000
    v.put32(0x4b4250+0x3c,0);v.put32(0x4b4250+0x40,56)
    v.put32(0x4b4254,192);v.put32(0x4b4258,96)
    def answer(response):
        context=v.uc.context_save();prior=live_json(v,control['slot'])
        flags=[v.u32(DIALOG+0x34+n*4) for n in range(4)]
        aux(0x45bcd0)
        enabled={e['control']:e['enabled'] for e in windows if e['kind']=='enable'}
        if response==255:aux(0x45bcea)
        else:
            command={1:0x507,2:0x508,3:0x509,4:0x50b}[response]
            assert enabled[command]
            aux(0x45bc23,(0x111,command,0))
        assert v.read(DIALOG+0x28,1)[0]==response
        assert len(range_before)==1
        targets=v.read(0x4ac748,100);targets=list(targets[:targets.index(255)])
        result=dict(flags=flags,battle=prior,events=list(windows),response=response,event_index=len(events),
          range_before=range_before[0],grids=grids(),targets=targets)
        v.uc.context_restore(context);v.return_from_hook(0,20)
        return result
    return events,control,windows,range_before,answer

def generate(source):
    _,v,_,_=prepare(source);events,control,windows,range_before,answer=configure_action(v)
    memory=[(a,v.read(a,b-a+1)) for a,b,_ in v.uc.mem_regions()]
    specs=[dict(direction=d,flags=f,tile=tile,command=cmd)
      for d in range(4) for f in (0,4,8,12)
      for tile in ([10,6],[10,5],[10,7],[11,7],[9,6]) for cmd in (255,4)]
    base=dict(direction=2,flags=0,tile=[10,6],command=255)
    specs += [dict(base,job=job,level=level,mp=mp,status=status)
      for job in range(53) for level in (0,3,10,50) for mp in (0,33) for status in (0,4)]
    specs += [dict(base,stock=[item,count]) for item in range(17) for count in (1,255)]
    specs += [dict(base,job=0,level=0,accessory=accessory,neighbor=[slot,direction,level,hp])
      for accessory in (255,78) for slot in (18,35) for direction in range(4)
      for level in (0,50) for hp in (0,1)]
    cases=[]
    for spec in specs:
        for a,raw in memory:v.write(a,raw)
        actor=LIVE;control['slot']=0
        assert v.read(actor+12,1)[0]!=2
        old_mirror=v.read(actor+13,1)[0]&8
        v.write(actor+6,bytes([10,6]));v.write(actor+12,bytes([2,(spec['flags']&~8)|old_mirror,7,spec['direction']]))
        v.write(0x818c80+130,b'\0')
        if 'job' in spec:v.write(UNIT+0x2b,bytes([spec['job'],spec['level']]))
        if 'mp' in spec:v.put32(actor+20,spec['mp'])
        if 'status' in spec:v.write(actor+30,bytes([spec['status']]))
        if 'stock' in spec:v.write(0x4b0770+0x26b+spec['stock'][0],bytes([spec['stock'][1]]))
        if 'accessory' in spec:v.write(UNIT+0x34,bytes([spec['accessory']]))
        if 'neighbor' in spec:
            slot,direction,level,hp=spec['neighbor'];p=LIVE+slot*36
            x,y=v.read(p+6,2);v.write(0x818c80+y*20+x,b'\xff')
            nx,ny=10+(0,1,0,-1)[direction],6+(-1,0,1,0)[direction]
            assert v.read(0x818c80+ny*20+nx,1)==b'\xff'
            v.write(p+6,bytes([nx,ny]));v.put32(p+16,hp);v.write(0x818c80+ny*20+nx,bytes([slot]))
            v.write(UNIT+v.u32(p)*72+0x2b,bytes([0,level]))
        # Explicit orientation setup must also match the original sprite cache.
        v.call(0x43fd34,[int(bool(spec['flags']&8))],actor)
        before=live_json(v,0);persistent=v.read(UNIT,512*72);rng=v.u32(0x48ecd4)
        other_live=v.read(LIVE+36,114*36)
        roster=[live_json(v,n) for n in range(115) if v.read(LIVE+n*36+12,1)[0]==2]
        records=[state(v,n) for n in sorted(set(b['id'] for b in roster))]
        inventory=list(v.read(0x4b0770+0x26b,17))
        control['inputs'][:]=[spec['tile']];control['picks'].clear();control['modes'].clear()
        events.clear();windows.clear();range_before.clear();v.hits.clear();menus=[]
        try:v.call(0x43dde3,this=actor,budget=4000000)
        except Modal:
            menus.append(answer(spec['command']))
            v.uc.emu_start(v.uc.reg_read(UC_X86_REG_EIP),0x83f000,count=4000000)
        except Exception:
            print('case',len(cases),spec,'actor',live_json(v,0),'events',events[-8:],flush=True)
            raise
        assert v.uc.reg_read(UC_X86_REG_EIP)==0x83f000
        from unicorn.x86_const import UC_X86_REG_EAX
        result=v.uc.reg_read(UC_X86_REG_EAX)
        assert persistent==v.read(UNIT,512*72) and rng==v.u32(0x48ecd4)
        assert other_live==v.read(LIVE+36,114*36) and inventory==list(v.read(0x4b0770+0x26b,17))
        allowed={0x403340,0x4036f9,0x41e5db,0x41e5f6,0x42c4e0,0x4350f0,
          0x43f38b,0x43fcc9,0x44a88a,0x45137c,0x451913,0x451c03,0x4539dc,
          0x453b13,0x453e32,0x45418f,0x47467a,0x47e251,0x47fcfd,
          0x83e000,0x83e010,0x83e020,0x83e030,0x83e040,0x83e050,0x83e060,
          0x83e100,0x83e110,0x83e120}
        assert not(v.hits.intersection(v.hooks)-allowed),[hex(a) for a in sorted(v.hits.intersection(v.hooks)-allowed)]
        cases.append(dict(input=spec,before=before,unit=state(v,0),roster=roster,records=records,inventory=inventory,
          picks=list(control['picks']),menus=menus,
          events=list(events),result=result,after=live_json(v,0),occupancy=list(v.read(0x818c80,400)),
          camera=[v.u32(0x4b4254),v.u32(0x4b4258)],rng=rng,
          hooks=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
    return dict(executable_sha256=EDITION,cases=cases,boundaries=__doc__)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'))
    p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_player_action.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n')
    print(len(r['cases']),'original player movement/menu/wait/cancel cases')
