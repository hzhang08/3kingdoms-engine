"""Hash-gated, bounded CPU-only oracle. No Windows loader or host APIs.

Every caller specifies the executable ranges it permits. Unexpected calls stop
the probe. Hooks must be documented individually in the generated fixture.
"""
import hashlib,struct
from pathlib import Path
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP
from executable_probe import EDITION
from ls12 import decode

class ProbeVM:
    def __init__(self,source,ranges):
        self.source=Path(source);raw=(self.source/'Ekd5.exe').read_bytes()
        if hashlib.sha256(raw).hexdigest()!=EDITION:raise ValueError('unsupported executable edition')
        self.executable_sha256=EDITION
        self.image=pefile.PE(data=raw).get_memory_mapped_image()
        self.uc=Uc(UC_ARCH_X86,UC_MODE_32);self.uc.mem_map(0x400000,0x200000);self.uc.mem_write(0x400000,self.image);self.uc.mem_map(0x800000,0x40000)
        self.ranges=list(ranges);self.hooks={};self.hits=set();self.checked=set()
        self.uc.hook_add(UC_HOOK_CODE,self.guard)
        self.data={b.index:b.data for b in decode((self.source/'Data.e5').read_bytes())}
    def guard(self,uc,address,size,user):
        self.hits.add(address)
        if address in self.hooks:self.hooks[address](self);return
        if address not in self.checked:
            if not any(a<=address<b for a,b in self.ranges):raise RuntimeError(f'unexpected execution at {address:#x}')
            self.checked.add(address)
    def read(self,address,size):return bytes(self.uc.mem_read(address,size))
    def write(self,address,value):self.uc.mem_write(address,value)
    def u32(self,address):return struct.unpack('<I',self.read(address,4))[0]
    def put32(self,address,value):self.write(address,struct.pack('<I',value&0xffffffff))
    def arg(self,n):return self.u32(self.uc.reg_read(UC_X86_REG_ESP)+4+4*n)
    def this(self):return self.uc.reg_read(UC_X86_REG_ECX)
    def return_from_hook(self,value=0,pop=0):
        sp=self.uc.reg_read(UC_X86_REG_ESP);ret=self.u32(sp)
        self.uc.reg_write(UC_X86_REG_EAX,value&0xffffffff);self.uc.reg_write(UC_X86_REG_ESP,sp+4+pop);self.uc.reg_write(UC_X86_REG_EIP,ret)
    def call(self,address,args=(),this=0,budget=1000000):
        sp=0x830000;self.write(sp,struct.pack('<'+'I'*(len(args)+1),0x83f000,*[x&0xffffffff for x in args]))
        self.uc.reg_write(UC_X86_REG_ESP,sp);self.uc.reg_write(UC_X86_REG_ECX,this)
        self.uc.emu_start(address,0x83f000,count=budget)
        if self.uc.reg_read(UC_X86_REG_EIP)!=0x83f000:raise RuntimeError(f'probe instruction budget exceeded at {self.uc.reg_read(UC_X86_REG_EIP):#x}')
        return self.uc.reg_read(UC_X86_REG_EAX)
    def load_unit(self,uid,reload=False):
        self.write(0x800000,self.data[0][uid*32:(uid+1)*32])
        return self.call(0x408be3,[uid,0x800000,int(reload)],0x4a1b68+72*uid)
    def capture_state(self,selection,*,label='',labels=None):
        """Read selected records through the evidence-backed dictionary; no writes."""
        from game_state import default_dictionary
        return default_dictionary().capture(self.read,selection,executable_sha256=self.executable_sha256,label=label,labels=labels)
