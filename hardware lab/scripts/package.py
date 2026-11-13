import hashlib, json, pathlib, subprocess, sys, zipfile
lab=pathlib.Path(__file__).resolve().parents[1]
out=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else lab/'build'
out.mkdir(parents=True,exist_ok=True)
subprocess.run([sys.executable,str(lab/'scripts/verify.py')],check=True)
verification=json.loads((lab/'build/verification.json').read_text())
app='Roms/APPS/MOSS-LAB/'
files={
 'Roms/APPS/MOSS-LAB.sh':(lab/'packaging/MOSS-LAB.sh').read_bytes().replace(b'\r\n',b'\n'),
 app+'moss-lab':(lab/'build/moss-lab').read_bytes(),
 app+'README.txt':(lab/'INSTALL-RU.md').read_bytes(),
 app+'verification.json':(lab/'build/verification.json').read_bytes(),
 'START-HERE-RU.md':(lab/'INSTALL-RU.md').read_bytes(),
}
assert files['Roms/APPS/MOSS-LAB.sh'].startswith(b'#!/bin/sh\n')
assert b'\r' not in files['Roms/APPS/MOSS-LAB.sh']
for p in sorted((lab/'samples').glob('*.wav')): files[app+'samples/'+p.name]=p.read_bytes()
hashes=''.join(hashlib.sha256(data).hexdigest()+'  '+name+'\n' for name,data in files.items())
files['SHA256SUMS.txt']=hashes.encode()
package=out/'MOSS-LAB-0.1.3-GarlicOS-1.3.9.zip'
with zipfile.ZipFile(package,'w',zipfile.ZIP_DEFLATED) as z:
 for name,data in files.items():
  item=zipfile.ZipInfo(name);item.create_system=3;item.compress_type=zipfile.ZIP_DEFLATED
  item.external_attr=((0o100755 if name.endswith('.sh') or name.endswith('/moss-lab') else 0o100644)<<16)
  z.writestr(item,data)
 z.writestr(app+'reports/',b'')
with zipfile.ZipFile(package) as z:
 assert z.testzip() is None
 for name,data in files.items(): assert z.read(name)==data
 assert hashlib.sha256(z.read(app+'moss-lab')).hexdigest()==verification['sha256']
source=out/'MOSS-LAB-0.1.3-source.zip'
with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED) as z:
 for directory in ('src','tests','scripts','packaging','samples'):
  for p in sorted((lab/directory).rglob('*')):
   if p.is_file() and '__pycache__' not in p.parts:z.write(p,'hardware lab/'+p.relative_to(lab).as_posix())
 for name in ('README.md','INSTALL-RU.md','DEVICE-NOTES.md','toolchain.lock.json','.clang-format'):
  z.write(lab/name,'hardware lab/'+name)
(out/'MOSS-LAB-INSTALL-RU.md').write_bytes((lab/'INSTALL-RU.md').read_bytes())
(out/'MOSS-LAB-verification.json').write_bytes((lab/'build/verification.json').read_bytes())
(out/'MOSS-LAB-SHA256.txt').write_text(hashlib.sha256(package.read_bytes()).hexdigest()+'  '+package.name+'\n')
print('Verified SD package:',package,package.stat().st_size,'bytes')
print('Source package:',source,source.stat().st_size,'bytes')
