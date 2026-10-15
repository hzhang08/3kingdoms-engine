"""Read-only, bounded decoder for KOEI LS11/Ls12 block archives.

Format research: https://www.xycq.org.cn/forum/thread-34612-1-1.html
The 0x110 index uses big-endian (packed size, decoded size, file offset).
Equal sizes denote literal blocks; other blocks use a byte dictionary and
variable-length integer codes. Never write to the input archive.
"""
import struct
from dataclasses import dataclass

class FormatError(ValueError):
    pass

@dataclass(frozen=True)
class Block:
    index: int
    offset: int
    packed_size: int
    decoded_size: int
    data: bytes

class Bits:
    def __init__(self, data):
        self.data, self.pos = data, 0
    def read(self, n):
        if n < 0 or self.pos+n > len(self.data)*8:
            raise FormatError(f'truncated bitstream at bit {self.pos}')
        value=0
        for _ in range(n):
            value=(value<<1)|((self.data[self.pos//8]>>(7-self.pos%8))&1)
            self.pos+=1
        return value
    def code(self):
        prefix=0
        width=0
        while True:
            bit=self.read(1)
            prefix=(prefix<<1)|bit
            width+=1
            if width>24:
                raise FormatError('integer code exceeds limit')
            if not bit:
                return prefix+self.read(width)

def decode(data, limit=128*1024*1024, indices=None):
    if len(data)<0x114 or data[:4] not in (b'LS11',b'Ls12'):
        raise FormatError('not an LS11/Ls12 archive')
    dictionary=data[16:272]
    records=[]
    cursor=272
    total=0
    while True:
        if cursor+4>len(data): raise FormatError('truncated index')
        packed=struct.unpack_from('>I',data,cursor)[0]
        if packed==0: break
        if cursor+12>len(data): raise FormatError('truncated index entry')
        packed, size, offset=struct.unpack_from('>III',data,cursor)
        if not size or offset+packed>len(data): raise FormatError('block outside archive')
        total+=size
        if total>limit: raise FormatError('decoded archive exceeds safety limit')
        records.append((offset,packed,size))
        cursor+=12
    if any(o<cursor+4 for o,_,_ in records): raise FormatError('block overlaps index')
    result=[]
    for index,(offset,packed,size) in enumerate(records):
        if indices is not None and index not in indices: continue
        encoded=data[offset:offset+packed]
        if packed==size:
            output=encoded
        else:
            bits=Bits(encoded)
            output=bytearray()
            while len(output)<size:
                value=bits.code()
                if value<256:
                    output.append(dictionary[value])
                else:
                    distance=value-256
                    count=bits.code()+3
                    if not 0<distance<=len(output): raise FormatError(f'block {index}: invalid distance {distance}')
                    if len(output)+count>size: raise FormatError(f'block {index}: output overrun')
                    for _ in range(count): output.append(output[-distance])
            output=bytes(output)
        result.append(Block(index,offset,packed,size,output))
    return result

if __name__=='__main__':
    import sys,pathlib
    for path in map(pathlib.Path,sys.argv[1:]):
        blocks=decode(path.read_bytes())
        print(path.name,len(blocks),[(b.decoded_size,b.data[:16].hex()) for b in blocks[:6]])
