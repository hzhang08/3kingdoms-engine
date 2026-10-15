"""Read-only Imsg.e5 fixed-record text decoder, verified at original 0x40c02f.

The 0.8 importer includes these records with their source identity and IDs.
"""
def decode_message(raw,index):
    if index<0 or (index+1)*200>len(raw):raise ValueError('Imsg.e5 record out of bounds')
    record=raw[index*200:(index+1)*200]
    encoded=record.split(b'\0')[0].replace(b'\\n',b'\n')
    return dict(id=index,text=encoded.decode('gbk'),raw=record.hex())

def decode_critical_speeches(raw):
    # 21 named heroes followed by ten groups with three generic lines each.
    return [decode_message(raw,n) for n in range(700,751)]
