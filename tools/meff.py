"""Verified MEFF#23 layout from the original 0x401000 reader."""
import hashlib,struct
from ls12 import FormatError

def decode_poison(raw):
    if len(raw)<20:raise FormatError('MEFF#23 truncated header')
    frames,images,masks,palettes,ms,w,h,sound=struct.unpack_from('<4B4I',raw)
    if (frames,images,masks,palettes,ms,w,h,sound)!=(13,13,0,0,100,75,90,19):
        raise FormatError('MEFF#23 unverified header')
    expected=20+frames*20+images*w*h
    if len(raw)!=expected:raise FormatError('MEFF#23 truncated or trailing payload')
    result=dict(resource=23,width=w,height=h,interval_ticks=ms//50,sound=100+sound,frames=[])
    for n in range(frames):
        at=20+n*20;image,palette,weight,amount,masked,dx,dy,callback=struct.unpack_from('<4BIiiB',raw,at)
        if (image,palette,weight,amount,masked,dx,dy,callback)!=(n,255,7,16,0,0,-14,1 if n==1 else 0):
            raise FormatError(f'MEFF#23:0x{at:x} unverified descriptor')
        offset=20+frames*20+image*w*h
        result['frames'].append(dict(image=image,palette=palette,weight=weight,amount=amount,dx=dx,dy=dy,callback=callback,
          descriptor_offset=at,pixel_offset=offset,pixels_sha256=hashlib.sha256(raw[offset:offset+w*h]).hexdigest()))
    return result

def decode_secondary(raw,resource):
    """Only the supplied edition's callback-free status/miss resources."""
    if resource not in (45,48,49):raise FormatError('unverified secondary MEFF resource')
    if len(raw)<20:raise FormatError(f'MEFF#{resource} truncated header')
    frames,images,masks,palettes,ms,w,h,sound=struct.unpack_from('<4B4I',raw)
    header=(13,1,0,0,50,16,16,29) if resource in (48,49) else (9,1,0,0,50,64,64,31)
    if (frames,images,masks,palettes,ms,w,h,sound)!=header:raise FormatError(f'MEFF#{resource} unverified header')
    if len(raw)!=20+frames*20+images*w*h:raise FormatError(f'MEFF#{resource} truncated or trailing payload')
    descriptors=[(-18,-18,8,16),(-15,-13,8,16),(-9,-9,7,16),(0,-7,7,16),(9,-9,7,16),(15,-14,8,16),(18,-18,8,16),(15,-24,10,10),(6,-28,10,8),(0,-29,12,8),(-6,-28,10,8),(-15,-24,10,10),(-18,-18,8,16)] if resource==49 else [(0,dy,16,amount) for dy,amount in zip((-28,-32,-30,-27,-23,-19,-15,-11,-7),(4,11,9,7,6,5,4,3,2))]
    if resource==48:descriptors=[(x,y,weight,0) for (x,y),weight in zip(((-18,-18),(-15,-13),(-9,-9),(0,-7),(9,-9),(15,-14),(18,-18),(15,-24),(6,-28),(0,-29),(-6,-28),(-15,-24),(-18,-18)),(2,1,0,0,0,1,2,3,4,4,4,3,2))]
    result=dict(resource=resource,width=w,height=h,interval_ticks=ms//50,sound=100+sound,frames=[])
    for n,(dx,dy,weight,amount) in enumerate(descriptors):
        at=20+n*20
        if struct.unpack_from('<4BIiiB',raw,at)!=(0,255,weight,amount,0,dx,dy,0):raise FormatError(f'MEFF#{resource}:0x{at:x} unverified descriptor')
        offset=20+frames*20
        result['frames'].append(dict(image=0,palette=255,weight=weight,amount=amount,dx=dx,dy=dy,callback=0,descriptor_offset=at,pixel_offset=offset,pixels_sha256=hashlib.sha256(raw[offset:offset+w*h]).hexdigest()))
    return result
