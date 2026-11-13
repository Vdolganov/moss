import json, pathlib, struct, sys
def inspect(path):
    data = pathlib.Path(path).read_bytes()
    if data[:6] != b'\x7fELF\x01\x01':
        raise ValueError('Expected ELF32 little endian: ' + str(path))
    header = struct.unpack_from('<16sHHIIIIIHHHHHH', data)
    _, kind, machine, version, entry, phoff, shoff, flags, ehsize, phsize, phnum, shsize, shnum, shstr = header
    sections = [struct.unpack_from('<IIIIIIIIII', data, shoff+i*shsize) for i in range(shnum)]
    def content(s): return data[s[4]:s[4]+s[5]]
    def string(table, at): return table[at:table.find(b'\0',at)].decode('ascii', errors='replace')
    names = content(sections[shstr]); named = {string(names,s[0]):s for s in sections}
    result = {'file':pathlib.Path(path).name,'size':len(data),'type':kind,'machine':machine,'entry':hex(entry),'flags':hex(flags),'interpreter':None,'needed':[],'soname':None,'undefined':[], 'strong_undefined':[], 'exports':[]}
    if '.interp' in named: result['interpreter'] = content(named['.interp']).rstrip(b'\0').decode()
    if '.dynamic' in named:
        dynamic = named['.dynamic']; strings = content(sections[dynamic[6]])
        for at in range(dynamic[4],dynamic[4]+dynamic[5],8):
            tag, value = struct.unpack_from('<iI',data,at)
            if tag == 1: result['needed'].append(string(strings,value))
            if tag == 14: result['soname'] = string(strings,value)
    if '.dynsym' in named:
        symbols=named['.dynsym']; strings=content(sections[symbols[6]])
        for at in range(symbols[4],symbols[4]+symbols[5],16):
            name,val,size,info,other,section=struct.unpack_from('<IIIBBH',data,at)
            if section==0 and name:
                result['undefined'].append(string(strings,name))
                if info >> 4 != 2: result['strong_undefined'].append(string(strings,name))
            elif section and name: result['exports'].append(string(strings,name))
    result['executable_stack'] = False
    result['load_memory_bytes'] = 0
    for i in range(phnum):
        p = struct.unpack_from('<IIIIIIII',data,phoff+i*phsize)
        if p[0] == 0x6474e551: result['executable_stack'] = bool(p[6]&1)
        if p[0] == 1: result['load_memory_bytes'] += p[5]
    result['arm_attributes_hex']=content(named['.ARM.attributes']).hex() if '.ARM.attributes' in named else None
    return result
if __name__=='__main__':
    for path in sys.argv[1:]:
        report = inspect(path)
        del report['exports']
        print(json.dumps(report,indent=2))
