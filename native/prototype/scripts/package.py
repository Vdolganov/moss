import hashlib,json,pathlib,sys,zipfile
root=pathlib.Path(__file__).resolve().parents[1]
project=root.parents[1]
sys.path.insert(0,str(project/'hardware lab/scripts'))
from inspect_elf import inspect
for launcher in (root/'packaging').glob('*.sh'):
 data=launcher.read_bytes()
 assert data.startswith(b'#!/bin/sh\n') and b'\r' not in data, f'{launcher}: LF line endings required'
out=pathlib.Path(sys.argv[1]).resolve();out.mkdir(parents=True,exist_ok=True)
binary=root/'build/moss';result=inspect(binary);tests=json.loads((root/'build/tests.json').read_text())
assert tests['sha256']==hashlib.sha256(binary.read_bytes()).hexdigest() and tests['music']=='passed' and tests['previews']==7 and tests['interaction']=='passed'
assert tests['features']=='passed'
assert tests['synth']=='passed' and tests['synthPreviews']==6
assert tests['workflow']=='passed' and tests['v05']=='passed' and tests['v051']=='passed' and tests['v052']=='passed'
version=tests['version']
device=json.loads((root/'build/device-verification.json').read_text())
assert device['release_sha256']==tests['sha256'] and device['release_smoke']['profile_passed']
assert device['song_stress']['profile_passed'] and device['normal_launcher_check']['passed']
assert result['machine']==40 and result['type']==2 and result['interpreter']=='/lib/ld-uClibc.so.0'
assert int(result['flags'],16)&0xff000000==0x05000000 and not int(result['flags'],16)&0x400
assert not result['executable_stack'] and b'GLIBC_' not in binary.read_bytes()
assert b'cortex-a9' in bytes.fromhex(inspect(root/'build/moss.o')['arm_attributes_hex'])
assert result['load_memory_bytes']<80*1024*1024
sdk=project/'native/tooling/sdk/opt/miyoo/arm-miyoo-linux-uclibcgnueabi/sysroot';exports=set()
for name in result['needed']:
 assert name in ('libSDL-1.2.so.0','libc.so.0','libm.so.0')
 lib=next(p for p in [sdk/'usr/lib'/name,sdk/'lib'/name] if p.exists());exports.update(inspect(lib)['exports'])
assert not set(result['strong_undefined'])-exports
for k in ('exports','undefined','strong_undefined','soname','arm_attributes_hex'):result.pop(k,None)
result.update(version=version,sha256=tests['sha256'],tests=tests,abi='ARMv7 Cortex-A9 softfp uClibc; SDK exports resolved',device=device)
verification=json.dumps(result,indent=2).encode();(out/f'MOSS-{version}-verification.json').write_bytes(verification)
readme=(root/'README.md').read_bytes();(out/f'MOSS-{version}-README-RU.md').write_bytes(readme)
def archive(name,files):
 hashes=''.join(hashlib.sha256(data).hexdigest()+'  '+p+'\n' for p,data in files.items());files['SHA256SUMS.txt']=hashes.encode()
 target=out/name
 with zipfile.ZipFile(target,'w',zipfile.ZIP_DEFLATED) as z:
  for path,data in files.items():
   info=zipfile.ZipInfo(path);info.create_system=3;info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=((0o100755 if path.endswith('.sh') or path.endswith('/moss') else 0o100644)<<16);z.writestr(info,data)
 with zipfile.ZipFile(target) as z:assert z.testzip() is None
 print(name,target.stat().st_size)
files={'Roms/APPS/MOSS.sh':(root/'packaging/MOSS.sh').read_bytes().replace(b'\r\n',b'\n'),'Roms/APPS/MOSS/moss':binary.read_bytes(),'Roms/APPS/MOSS/README.txt':readme,'Roms/APPS/MOSS/verification.json':verification}
for p in (root/'samples').glob('*.wav'):files['Roms/APPS/MOSS/samples/'+p.name]=p.read_bytes()
files['Roms/APPS/MOSS/cpu-performance.sh']=(root/'packaging/cpu-performance.sh').read_bytes().replace(b'\r\n',b'\n')
archive(f'MOSS-{version}-GarlicOS.zip',files)
files={'MOSS.exe':(root/'build'/tests['desktop_exe']).read_bytes(),'README-RU.md':readme}
for p in (root/'samples').glob('*.wav'):files['samples/'+p.name]=p.read_bytes()
archive(f'MOSS-{version}-Windows.zip',files)
files={}
for sub in ('src','scripts','tests','packaging','samples'):
 for p in (root/sub).rglob('*'):
  if p.is_file() and '__pycache__' not in p.parts:files['MOSS/native/prototype/'+p.relative_to(root).as_posix()]=p.read_bytes()
files['MOSS/native/prototype/README.md']=readme
files['MOSS/native/prototype/.gitattributes']=(root/'.gitattributes').read_bytes()
for p in (project/'hardware lab/src').glob('*.h'):files['MOSS/hardware lab/src/'+p.name]=p.read_bytes()
for name in ('inspect_elf.py','setup.cjs','extract_sdk.py','make_samples.py'):files['MOSS/hardware lab/scripts/'+name]=(project/'hardware lab/scripts'/name).read_bytes()
for name in ('chime.wav','air.wav','dust.wav'):files['MOSS/hardware lab/samples/'+name]=(root/'samples'/name).read_bytes()
files['MOSS/hardware lab/toolchain.lock.json']=(project/'hardware lab/toolchain.lock.json').read_bytes()
files['MOSS/docs/NATIVE-PERFORMANCE-051.md']=(project/'docs/NATIVE-PERFORMANCE-051.md').read_bytes()
for name in ('PRODUCT.md','DESIGN.md','.impeccable/design.json','design/current/native-v051-interaction.md','design/current/native-v052-interaction.md','docs/NATIVE-VERIFICATION-052.md'):
 files['MOSS/'+name]=(project/name).read_bytes()
archive(f'MOSS-{version}-source.zip',files)
print(json.dumps(result,indent=2))
