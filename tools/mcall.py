"""MCALL05 descriptors and original indexed image/mask planes (0x402805).

Only the supplied edition is supported. Trailing descriptor bytes are retained
verbatim: the original reader ignores them and some contain nonzero values.
"""
import hashlib
import struct
from ls12 import FormatError

RESOURCE_SHA = 'fe86b9b67f0672cd5812e755e579b5eb49228eef31702aa0c06606fcc3fb8ada'

def decode_mcall(raw):
    if len(raw) < 20:
        raise FormatError('MCALL05 truncated header')
    frames, images, masks, palettes, ms, w, h, sound = struct.unpack_from('<4B4I', raw)
    if (frames, images, masks, palettes, ms, w, h, sound) != (30,30,12,0,100,300,300,12):
        raise FormatError('MCALL05 unverified header')
    if len(raw) != 20 + frames*20 + (images+masks)*w*h:
        raise FormatError('MCALL05 truncated or trailing payload')
    if hashlib.sha256(raw).hexdigest() != RESOURCE_SHA:
        raise FormatError('MCALL05 unsupported resource edition')
    result = dict(resource=105, width=w, height=h, interval_ticks=ms//50,
                  sound=100+sound, source_sha256=RESOURCE_SHA, frames=[])
    for n in range(frames):
        at = 20+n*20
        image, palette, mask_or_weight, amount, masked, dx, dy, sound_mode = struct.unpack_from('<4BIiiB', raw, at)
        offset = 20+frames*20+image*w*h
        mask_offset = 20+frames*20+(images+mask_or_weight)*w*h if masked else None
        result['frames'].append(dict(image=image, palette=palette, masked=bool(masked),
            mask=mask_or_weight if masked else None, weight=None if masked else mask_or_weight,
            amount=amount, dx=dx, dy=dy, sound_mode=sound_mode, descriptor_offset=at,
            descriptor=list(raw[at:at+20]), pixel_offset=offset, mask_offset=mask_offset,
            pixels_sha256=hashlib.sha256(raw[offset:offset+w*h]).hexdigest(),
            mask_sha256=hashlib.sha256(raw[mask_offset:mask_offset+w*h]).hexdigest() if masked else None))
    return result
