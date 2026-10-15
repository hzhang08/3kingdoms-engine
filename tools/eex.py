"""Typed EEX decoder. No recovery/resynchronization or silent skipping.

Numeric wire definitions transcribed from the original-era Style.txt research
(https://www.xycq.org.cn/forum/attachment.php?aid=23574). Structure cross-checked
against MIT-licensed cczEditor, Data/FileData.cs (see THIRD_PARTY.md).
Semantic support is a separate engine concern; structural decode is not execution.
"""
from pathlib import Path
import json,struct
from ls12 import FormatError
SCHEMA=json.loads(Path(__file__).with_name('eex_schema.json').read_text())

class Reader:
    def __init__(self,data,name): self.data,self.name=data,name
    def fail(self,p,msg): raise FormatError(f'{self.name}:0x{p:04x}: {msg}')
    def u16(self,p):
        if p+2>len(self.data): self.fail(p,'truncated u16')
        return struct.unpack_from('<H',self.data,p)[0]
    def argument(self,p,expected=None):
        start=p;t=self.u16(p);p+=2
        if expected is not None and t!=expected: self.fail(start,f'expected parameter type {expected}, got {t}')
        if t==5:
            end=self.data.find(b'\0',p)
            if end<0: self.fail(p,'unterminated GBK string')
            try: value=self.data[p:end].decode('gbk')
            except UnicodeDecodeError as e: self.fail(p,str(e))
            p=end+1
        elif t==4:
            if p+4>len(self.data): self.fail(p,'truncated i32')
            value=struct.unpack_from('<i',self.data,p)[0];p+=4
        elif t==35: # ordinary type 35 is a state enum; lists are handled explicitly
            value=self.u16(p);p+=2
        elif 1<=t<=78:
            value=self.u16(p);value=value-65536 if value>=32768 else value;p+=2
        else: self.fail(start,f'unknown parameter type {t}')
        return {'offset':start,'type':t,'value':value},p
    def instruction(self,p):
        start=p;op=self.u16(p);p+=2
        if str(op) not in SCHEMA:self.fail(start,f'unknown opcode 0x{op:04x}')
        definition=SCHEMA[str(op)];args=[]
        if op==5:
            for typ in (53,53):
                actual=self.u16(p)
                if actual!=typ:self.fail(p,f'expected variable-list tag {typ}, got {actual}')
                n=self.u16(p+2);a=p;p+=4
                if n>4096:self.fail(a,'variable list exceeds limit')
                values=[self.u16(p+i*2) for i in range(n)];p+=n*2
                args.append({'offset':a,'type':typ,'value':values})
        elif op in (70,71):
            end=start+definition['size']
            while p<end:
                arg,p=self.argument(p);args.append(arg)
            if p!=end:self.fail(start,'unit table does not end on a parameter boundary')
        else:
            for typ in definition['types']:
                arg,p=self.argument(p,typ);args.append(arg)
        result={'offset':start,'opcode':op,'args':args,'end':p}
        if op==1:
            test,p=self.instruction(p)
            length=self.u16(p);begin=p;p+=2
            children,p=self.sequence(p,begin+length)
            result.update(test=test,body=children,end=p,body_length=length)
        return result,p
    def sequence(self,p,end):
        body=[]
        while p<end:
            ins,p=self.instruction(p);body.append(ins)
        if p!=end:self.fail(p,f'block ends at {end:#x}')
        return body,p
    def parse(self):
        if self.data[:4]!=b'EEX\0':self.fail(0,'not EEX')
        first=struct.unpack_from('<I',self.data,10)[0]
        if first<14 or (first-10)%4:self.fail(10,'invalid section table')
        offsets=list(struct.unpack_from('<'+'I'*((first-10)//4),self.data,10))
        if offsets!=sorted(set(offsets)) or offsets[-1]>=len(self.data):self.fail(10,'invalid section offsets')
        sections=[]
        for sn,offset in enumerate(offsets):
            count=self.u16(offset);p=offset+2;events=[]
            for _ in range(count):
                start=p;length=self.u16(p);p+=2;conditions=[]
                while self.u16(p)!=0:
                    condition,p=self.instruction(p);conditions.append(condition)
                p+=2;body_length=self.u16(p);begin=p;p+=2
                body,p=self.sequence(p,begin+body_length)
                if self.u16(p)!=0:self.fail(p,'missing event terminator')
                p+=2
                if p!=start+length+2:self.fail(p,f'event length mismatch {start:#x}+{length}+2')
                events.append({'offset':start,'end':p,'conditions':conditions,'body':body})
            end=offsets[sn+1] if sn+1<len(offsets) else len(self.data)
            if p!=end:self.fail(p,f'section has {end-p} unparsed bytes')
            sections.append({'offset':offset,'events':events})
        return {'file':self.name,'version_bytes':self.data[4:10].hex(),'sections':sections}

def parse(path):
    path=Path(path);return Reader(path.read_bytes(),path.name).parse()

if __name__=='__main__':
    import sys
    for file in sys.argv[1:]:
        result=parse(file)
        out=Path('assets/generated/scripts')/ (Path(file).stem+'.json');out.parent.mkdir(parents=True,exist_ok=True)
        out.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
        print(out,[(s['offset'],len(s['events'])) for s in result['sections']])
