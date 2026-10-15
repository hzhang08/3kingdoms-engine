"""This edition's Data.e5#5 loader, matching original 0x40ddaf/0x41b84f."""
from ls12 import FormatError

def decode_tactics(raw):
    if len(raw)!=73*70:raise FormatError('unexpected tactic table size')
    result=[]
    # Original loader uses 68 records; retain the complete archive separately.
    for index in range(68):
        r=raw[index*70:(index+1)*70]
        memory=list(r[:10])+[0,r[12],r[13],r[14],r[15],r[11],r[16]]+list(r[17:])
        result.append(dict(id=index,name=r[:10].split(b'\0')[0].decode('gbk'),fields=memory,raw=r.hex()))
    return result
