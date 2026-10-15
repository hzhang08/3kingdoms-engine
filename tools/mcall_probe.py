#!/usr/bin/env python3
"""Original MCALL05 preload worker, frame selection and presentation sequence.

File reads copy exact original bytes. Heap/thread/atomic operations are bounded
host substitutes; the original loader and worker execute sequentially, without
Windows. Raster calls capture positions and indexed image/mask hashes against a
deterministic background. Original frame scheduling, sound gates and cleanup run.
Mask compositing is separately exercised in a hook-free, hash-gated arithmetic VM.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from probe_vm import ProbeVM
from executable_probe import EDITION
OBJ=0x492788;BUFFER=0xd00000;HEAP=0x1000000

def configure(v,events,buffer=BUFFER,heap_base=HEAP,host_base=0x83e000):
    v.ranges += [(0x402544,0x40316a),(0x4032f0,0x40331e),(0x403470,0x40350b),
      (0x403262,0x4032a8),(0x4036a0,0x4036d8),(0x480b30,0x480bb0)]
    v.uc.mem_map(buffer,0x50000);v.uc.mem_map(heap_base,0x500000)
    raw=(v.source/'Mcall05.e5').read_bytes();position=[0];opened=[False];heap=[heap_base];allocations={};thread=[]
    def signed(n):return n if n<2**31 else n-2**32
    prior_close=v.hooks.get(0x4192c5)
    def close(vm):
        if vm.this()!=OBJ+8:
            assert prior_close is not None, "unexpected file object"
            prior_close(vm);return
        opened[0]=False;vm.return_from_hook()
    def open_file(vm):
        name=vm.read(vm.arg(0),32).split(b'\0')[0].decode('ascii');assert name.upper()=='MCALL05.E5' and vm.arg(1)==0
        position[0]=0;opened[0]=True;events.append(dict(kind='open',name=name));vm.return_from_hook(1,pop=8)
    def read_current(vm):
        ptr,count=vm.arg(0),vm.arg(1);assert opened[0] and position[0]+count<=len(raw)
        vm.write(ptr,raw[position[0]:position[0]+count]);position[0]+=count;vm.return_from_hook(count,pop=8)
    def read_at(vm):
        offset,ptr,count=[vm.arg(n) for n in range(3)];assert opened[0] and offset+count<=len(raw)
        vm.write(ptr,raw[offset:offset+count]);position[0]=offset+count;vm.return_from_hook(count,pop=12)
    def seek(vm):
        offset,origin=vm.arg(0),vm.arg(1);assert origin==0 and offset<=len(raw)
        position[0]=offset;vm.return_from_hook(offset,pop=8)
    def allocate(vm):
        size=vm.arg(0);ptr=heap[0];heap[0]+=(size+15)&~15;assert heap[0]<=heap_base+0x500000
        allocations[ptr]=size;vm.return_from_hook(ptr)
    def free(vm):
        ptr=vm.arg(0);assert ptr==0 or ptr in allocations
        vm.return_from_hook()
    def noop(vm):vm.return_from_hook()
    def atomic(vm):
        ptr,value=vm.arg(0),vm.arg(1);prior=vm.u32(ptr);vm.put32(ptr,value);vm.return_from_hook(prior,pop=8)
    def start_thread(vm):
        assert vm.arg(0)==vm.arg(1)==vm.arg(4)==0 and vm.arg(2)==0x402574 and vm.arg(3)==OBJ
        thread[:]=[vm.arg(2),vm.arg(3)];vm.put32(vm.arg(5),1);vm.return_from_hook(1,pop=24)
    def exit_thread(vm):assert vm.arg(0)==2;vm.return_from_hook(pop=4)
    def priority(vm):assert vm.arg(0)==1 and vm.arg(1)==2;vm.return_from_hook(1,pop=8)
    def thread_status(vm):assert vm.arg(0)==1;vm.put32(vm.arg(1),2);vm.return_from_hook(1,pop=8)
    for address,target,hook in ((0x486138,0x83e000,atomic),(0x48613c,0x83e020,start_thread),(0x486140,0x83e040,exit_thread),(0x486134,0x83e060,priority),(0x486148,0x83e080,thread_status)):
        target=host_base+target-0x83e000;v.put32(address,target);v.hooks[target]=hook
    v.put32(0x486144,host_base+0xa0);v.hooks[host_base+0xa0]=lambda vm:vm.return_from_hook(pop=4)
    def rectangle(vm):return [signed(vm.arg(0)),signed(vm.arg(1)),vm.arg(2),vm.arg(3)]
    def capture(vm):
        r=rectangle(vm);data=bytes((n*17+n//r[2]*3)%256 for n in range(r[2]*r[3]))
        vm.write(vm.arg(4),data);events.append(dict(kind='capture',rectangle=r,pixels_sha256=hashlib.sha256(data).hexdigest()));vm.return_from_hook()
    def restore(vm):events.append(dict(kind='restore',rectangle=rectangle(vm)));vm.return_from_hook()
    def composite(mask):
        def run(vm):
            r=rectangle(vm);size=r[2]*r[3];assert vm.arg(8 if mask else 7)==4
            src=vm.read(vm.arg(4),size);second=vm.read(vm.arg(5),size) if mask else None
            events.append(dict(kind='effect',rectangle=r,image_sha256=hashlib.sha256(src).hexdigest(),mask_sha256=hashlib.sha256(second).hexdigest() if mask else None,
              weight=None if mask else vm.arg(8)&255,amount=vm.arg(10 if mask else 11)&255,palette=vm.arg(9 if mask else 10),
              frame=vm.read(OBJ+0x58,1)[0],descriptor=list(vm.read(OBJ+0x30,20))))
            vm.return_from_hook()
        return run
    def sound(vm):events.append(dict(kind='sound',id=vm.arg(0),flag=vm.arg(1)));vm.return_from_hook(pop=8)
    def wait(vm):events.append(dict(kind='wait',ticks=vm.arg(0)));vm.return_from_hook(pop=4)
    v.hooks.update({0x4192c5:close,0x418f5c:open_file,0x419167:read_current,0x419184:read_at,0x41921f:seek,
      0x480ff0:noop,0x480fe0:allocate,0x481020:free,0x47eac4:noop,
      0x41e5db:noop,0x41e5f6:noop,0x47d1f1:noop,0x47eba5:noop,0x42c4e0:noop,
      0x47e251:capture,0x47e09b:restore,0x430369:composite(True),0x43030e:composite(False),0x47467a:sound,0x403340:wait})
    def reset_file():opened[0]=True;position[0]=len(raw)
    v.mcall_reset_file=reset_file
    return thread,raw

def generate(source):
    v=ProbeVM(source,[]);v.uc.mem_map(0,0x1000);events=[];thread,raw=configure(v,events)
    v.write(OBJ,b'\0'*0x64);v.write(0x820000,b'MCALL05.E5\0');v.hits.clear()
    assert v.call(0x402f7f,[0x820000,BUFFER,0x44c00],OBJ)==1
    assert thread==[0x402574,OBJ] and v.u32(OBJ)==1
    assert v.call(thread[0],[thread[1]],budget=2000000)==2
    header=list(v.read(OBJ+0x1c,20));assert v.call(0x402f21,this=OBJ)==len(raw)
    assert v.u32(OBJ+0x18)==90000
    state=v.read(0x400000,0x200000);scratch=v.read(0x800000,0x40000);buffer=v.read(BUFFER,0x50000);heap=v.read(HEAP,0x500000)
    cases=[]
    for center in ((312,272),(24,80),(600,80),(24,416),(600,416)):
        v.write(0x400000,state);v.write(0x800000,scratch);v.write(BUFFER,buffer);v.write(HEAP,heap);events.clear();v.hits.clear()
        v.mcall_reset_file();v.call(0x403014,list(center),OBJ,budget=3000000)
        cases.append(dict(center=center,events=list(events),after=list(v.read(OBJ,0x64)),hooked=[hex(a) for a in sorted(v.hits.intersection(v.hooks))]))
        assert sum(e['kind']=='effect' for e in events)==30 and v.u32(OBJ)==0
    return dict(executable_sha256=EDITION,resource_sha256=hashlib.sha256(raw).hexdigest(),header_after_original_load=header,cases=cases,boundaries=__doc__)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=Path('/Users/henryzhang/Downloads/sgzccz'));p.add_argument('--output',type=Path,default=Path('tests/fixtures/original_mcall.json'));a=p.parse_args()
    r=generate(a.source);a.output.write_text(json.dumps(r,separators=(',',':'))+'\n');print(len(r['cases']),'original MCALL players')
