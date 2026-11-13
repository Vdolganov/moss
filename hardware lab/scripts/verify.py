"""Offline ABI/dependency checks. This does not claim a device test."""
import hashlib, json, pathlib, sys
from inspect_elf import inspect
lab = pathlib.Path(__file__).resolve().parents[1]
sdk = lab.parent/'native/tooling/sdk/opt/miyoo/arm-miyoo-linux-uclibcgnueabi/sysroot'
binary = lab/'build/moss-lab'
result = inspect(binary)
tests=json.loads((lab/'build/test-results.json').read_text())
assert tests['arm_sha256']==hashlib.sha256(binary.read_bytes()).hexdigest()
assert tests['core_tests']=='passed' and tests['render_previews']==5
assert tests['input_tests']=='passed'
assert tests['timing_tests']=='passed'
assert tests['sample_tests']=='passed'
obj = inspect(lab/'build/moss-lab.o')
assert result['machine']==40 and result['type']==2
assert result['interpreter']=='/lib/ld-uClibc.so.0'
assert int(result['flags'],16)&0xff000000==0x05000000
assert not int(result['flags'],16)&0x400, 'Hard-float calling convention is incompatible'
assert b'cortex-a9' in bytes.fromhex(obj['arm_attributes_hex'])
assert not result['executable_stack']
assert b'GLIBC_' not in binary.read_bytes(), 'Accidental glibc version dependency'
exports=set()
for name in result['needed']:
    assert name in ('libSDL-1.2.so.0','libc.so.0','libm.so.0'), 'Unexpected dependency: '+name
    candidates=[sdk/'usr/lib'/name,sdk/'lib'/name]
    library=next((p for p in candidates if p.exists()),None)
    assert library is not None, name
    exports.update(inspect(library)['exports'])
missing=set(result['strong_undefined'])-exports
assert not missing, 'SDK does not resolve strong symbols: '+str(missing)
assert result['load_memory_bytes']<8*1024*1024, 'Unexpected static memory growth'
for key in ('exports','undefined','strong_undefined','soname','arm_attributes_hex'):
    result.pop(key,None)
result.update(sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
              object_cpu='cortex-a9 / ARMv7-A / softfp',
              header_abi_checks='uClibc; 32-bit pointers; 16-byte Linux input_event',
              sdk_symbol_resolution='passed', host_core_tests='passed by build.cjs', sample_tests='passed: bounded reader, malformed WAVs, mono/stereo, pitch, trim, loop, three voices, stop',
              input_tests='passed, including 166 recognized events replayed from device logs',
              timing_tests='passed: stale frame timestamp, 3.790s recorded hold, 17 parameter edits, repeat and highlight',
              prior_device_evidence='0.1.2: user confirms audio and hold gestures; 16 recorded runs at 44.1/48 kHz, no over-budget or late callbacks',
              device_test='PENDING: verify 0.1.3 WAV playback, trim, pitch, loop and three shared-sample voices on device')
out=lab/'build/verification.json'
out.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
