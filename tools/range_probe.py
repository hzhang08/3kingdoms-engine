#!/usr/bin/env python3
"""Original attack-range masks, ordered target lists and AI preparation."""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from initialization_probe import state,UNIT
from movement_probe import configure as movement_configure,live_json
from executable_probe import EDITION

def configure(v):
    movement_configure(v)
    v.ranges += [(0x436586,0x437d59),(0x472c90,0x472ca1),(0x43f8be,0x43f917),
      (0x44f9a0,0x44f9b1),(0x43a1f4,0x43a4bf),(0x435182,0x4351da),
      (0x43ff6e,0x4400a2),(0x406906,0x406947),(0x407af2,0x407c0e),
      (0x41dd70,0x41dd82),(0x425ad0,0x425ae1)]

def generate(source):
    _,v,_,_=prepare(source);configure(v)
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[];getters=[];preparations=[];searches=[]
    def reset():v.write(0x400000,original);v.write(0x800000,scratch)
    def invariant():return (v.read(LIVE,115*36),v.read(UNIT,512*72),v.u32(0x48ecd4))
    def capture(slot,center,kind,mode,mask,preferred):
        w,h=v.read(0x4b422c,2);n=w*h;v.write(0x82e000,bytes(center));v.write(0x4b2c28,bytes([preferred]))
        v.write(0x8192c0,b'\xa5'*n);v.write(0x824000,b'\x5a'*n);v.write(0x824c80,b'\xa5'*n);v.write(0x4ac748,b'\xa5'*100)
        before=invariant();result=v.call(0x436586,[0x82e000,kind,mode,mask],LIVE+slot*36)&255
        assert invariant()==before
        targets=[]
        if not mask and kind!=255:
            raw=v.read(0x4ac748,100);targets=list(raw[:raw.index(255)])
        else:assert v.read(0x4ac748,100)==b'\xa5'*100
        cases.append(dict(battle=live_json(v,slot),roster=[live_json(v,n) for n in range(115) if v.read(LIVE+n*36+12,1)[0]==2],width=w,height=h,center=center,kind=kind,mode=mode,mask_only=bool(mask),preferred=preferred,result=result,targets=targets,distance=list(v.read(0x8192c0,n)),threat=list(v.read(0x824000,n)),mask=list(v.read(0x824c80,n))))
    for kind in list(range(16))+[128+k for k in range(16)]+[255]:
     for mode in (0,1,128,129):
      for mask in (0,1):
       for center in ((0,0),(19,19),(11,16),(9,11)):
        reset();capture(20,center,kind,mode,mask,35 if mode&1==0 else 18)
    # Dense mixed-allegiance board tests ordered selection, HP-zero exclusion,
    # preference matches and origin inclusion (mode bit 1).
    for kind in range(16):
     for mode in (0,1,128,129):
      for preferred in (0,18,35,50,255):
        reset();v.write(LIVE,b'\0'*(115*36));v.write(0x4b422c,bytes([9,9]));v.write(0x818c80,b'\xff'*1600)
        for slot in range(81):
            v.write(LIVE+slot*36+4,bytes([slot,slot%4,slot%9,slot//9]));v.write(LIVE+slot*36+12,b'\x02');v.put32(LIVE+slot*36+16,0 if slot%7==0 else 20);v.write(0x818c80+slot,bytes([slot]))
        capture(40,(4,4),kind,mode,0,preferred)
    for job in range(53):
     for accessory in (255,72):
        reset();v.load_unit(0);v.write(UNIT+0x2b,bytes([job]));v.write(UNIT+0x2e,bytes([255]*6+[accessory,255,255]));v.put32(LIVE,0)
        getters.append(dict(job=job,accessory=accessory,kind=v.call(0x43f8be,this=LIVE)&255))
    def preparation(slot):
        before=invariant();w,h=v.read(0x4b422c,2);n=w*h
        v.write(0x8192c0,b'\xa5'*n)
        v.call(0x43a28a,[255],LIVE+slot*36)
        assert invariant()==before
        roster=[live_json(v,n) for n in range(115) if v.read(LIVE+n*36+12,1)[0]==2]
        preparations.append(dict(battle=live_json(v,slot),unit=state(v,v.u32(LIVE+slot*36)),roster=roster,width=w,height=h,terrain=list(v.read(0x818640,n)),distance=list(v.read(0x8192c0,n)),threat=list(v.read(0x824000,n)),scores=list(v.read(0x824640,n)),flags=v.read(0x4b2c24,1)[0],best_score=v.read(0x4b2c4c,1)[0],best_tile=v.read(0x4b2c20,1)[0],best_target=v.read(0x4b2c28,1)[0],best_tactic=v.read(0x4b2c44,1)[0]))
    for slot in (18,19,20,35,36,48):reset();preparation(slot)
    for job in range(53):
     for ai in (0,1,2,3,4,7):
      for hp in (0,1,20,21,100):
        reset();v.write(UNIT+234*72+0x2b,bytes([job]));v.put32(UNIT+234*72+0x1c,100);v.write(UNIT+234*72+0x2e,b'\xff'*9);v.put32(LIVE+20*36+16,hp);v.write(LIVE+20*36+14,bytes([ai]));preparation(20)
    for slot in (18,19,20,35,36,48):
     for budget in (0,1,4,10,255):
      for zoc in (False,True):
       for synthetic in (False,True):
        reset();v.call(0x43a28a,[255],LIVE+slot*36);w,h=v.read(0x4b422c,2);n=w*h
        if synthetic:v.write(0x824000,bytes((255,1,18,20,35,48)[(x*3+y*7)%6] for y in range(h) for x in range(w)))
        threat=list(v.read(0x824000,n));b=live_json(v,slot);before=invariant();v.write(0x82e000,bytes([b['x'],b['y']]))
        value=v.call(0x438308,[0x82e000,budget,8,255,255,int(zoc)],LIVE+slot*36,budget=3000000)&255
        assert invariant()==before
        searches.append(dict(battle=b,unit=state(v,b['id']),roster=[live_json(v,n) for n in range(115) if v.read(LIVE+n*36+12,1)[0]==2],width=w,height=h,terrain=list(v.read(0x818640,n)),threat=threat,budget=budget,respect_zoc=zoc,result=value,costs=list(v.read(0x8192c0,n))))
    return dict(executable_sha256=EDITION,cases=cases,getters=getters,preparations=preparations,searches=searches,boundaries='Unhooked original 0x436586/0x437a04 range builder and target-list selection, 0x43f8be getter, 0x43a28a AI preparation, 0x438308 mode 8 lookup. Compares ordered targets, return value and all output grids. Live/persistent/RNG mutations forbidden. No actual attack, tactic, AI score/action or rendering executes.')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_ranges.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print({k:len(v) for k,v in r.items() if isinstance(v,list)})
