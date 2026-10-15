#!/usr/bin/env python3
"""Original critical-speech gate and text selection, not attack animation.

Runs 0x405744's actual 25% roll and 0x40c02f's hero/generic selection and
backslash-n expansion. Archive loading, facing, camera, redraw, speech display
and animation are explicit non-executed boundaries. Imsg.e5 reads are supplied
read-only at the file seek/read boundary. No RNG or text selection is hooked.
"""
import argparse,json
from pathlib import Path
from battle_events_probe import prepare,LIVE
from initialization_probe import UNIT
from strike_probe import configure as strike_configure,CONTROL
from executable_probe import EDITION

def generate(source):
    _,v,_,_=prepare(source);strike_configure(v)
    v.ranges += [(0x405744,0x405889),(0x40c02f,0x40c14f),(0x40bf56,0x40bf82),(0x40dfd0,0x40dfe1),(0x406670,0x40668c),(0x43e320,0x43e331)]
    raw=(source/'Imsg.e5').read_bytes();events=[]
    def skip(pop=0):return lambda vm:vm.return_from_hook(pop=pop)
    def read(vm):
        offset,destination,count=vm.arg(0),vm.arg(1),vm.arg(2)
        assert vm.this()==0x4ae4bc and count==200 and offset%200==0 and offset+count<=len(raw)
        events.append(dict(kind='message',index=offset//200));vm.write(destination,raw[offset:offset+count]);vm.return_from_hook(count,pop=12)
    def speech(vm):
        text=vm.read(vm.arg(0),201).split(b'\0')[0];events.append(dict(kind='speech',unit=vm.arg(1),text=text.decode('gbk'),raw=text.hex()));vm.return_from_hook(pop=8)
    v.hooks.update({0x403b8f:skip(),0x435829:skip(),0x403d85:skip(4),0x455123:skip(12),0x43fcc9:skip(),0x45965d:speech,0x4044e9:skip(),0x419184:read})
    original=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);cases=[]
    for uid in range(512):
     for seed in (0,1,42,662824084,0xffffffff):
      for critical in (False,True):
        v.write(0x400000,original);v.write(0x800000,scratch);events.clear();v.load_unit(uid)
        v.put32(LIVE+20*36,uid);v.write(CONTROL,b'\0'*0x614);v.write(CONTROL,bytes([20,38]));v.put32(CONTROL+4,uid);v.put32(CONTROL+8,UNIT+uid*72);v.put32(CONTROL+12,LIVE+20*36);v.write(CONTROL+16,bytes([38,255]));v.put32(CONTROL+0x604,int(critical))
        v.put32(0x48ecd4,seed);before=v.read(LIVE,115*36),v.read(UNIT,512*72);v.hits.clear();v.call(0x405744,this=CONTROL)
        assert before==(v.read(LIVE,115*36),v.read(UNIT,512*72))
        allowed={0x403b8f,0x435829,0x403d85,0x455123,0x43fcc9,0x45965d,0x4044e9,0x419184}
        assert not (v.hits.intersection(v.hooks)-allowed)
        cases.append(dict(unit=uid,group=v.read(UNIT+uid*72+0x26,1)[0],seed=seed,critical=critical,events=list(events),final_rng=v.u32(0x48ecd4)))
    return dict(executable_sha256=EDITION,cases=cases,boundaries=__doc__)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_critical_speech.json'));a=p.parse_args();r=generate(a.source);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(',',':'))+'\n');print(len(r['cases']),'critical-speech cases;',sum(bool(c['events']) for c in r['cases']),'speeches')
