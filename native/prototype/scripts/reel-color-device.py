"""Short, isolated REEL profiles; never replaces the installed user app."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

root = Path(__file__).resolve().parents[3]
adb = str(root / 'native/tooling/platform-tools/adb.exe')
work = root / 'work/reel-color'
work.mkdir(parents=True, exist_ok=True)
mode = sys.argv[1] if len(sys.argv) > 1 else 'color'
assert mode in ('optimization', 'color', 'micro')
revision = sys.argv[2] if len(sys.argv)>2 else 'v1'
assert re.fullmatch(r'v[0-9]+', revision)
remote = '/mnt/mmc/MOSS-Reel-Color-' + mode + '-' + revision


def shell(command):
    return subprocess.check_output([adb, 'shell', command], text=True).replace('\r', '')


def push(source, target):
    subprocess.run([adb, 'push', str(source), target], check=True, capture_output=True)


processes = shell('ps')
assert not re.search(r'\./moss\b', processes), 'MOSS is open; preserve the current session'
menu = [line.split()[1] for line in processes.splitlines() if line.rstrip().endswith(' ./main')]
assert len(menu) == 1, 'Expected the Garlic menu'
assert remote.rsplit('/', 1)[1] not in shell('ls /mnt/mmc'), 'Use a fresh isolated test directory'
assert not shell('if [ -e /mnt/mmc/CFW/retroarch/command.sh ]; then echo occupied; fi').strip(), 'A launcher is pending'
build = root / 'native/prototype/build/reel-motion'
if mode == 'micro':
    binaries={'test-arm':build/'color-test-arm'}
    profiles=[]
elif mode == 'optimization':
    binaries = {'previous': root / 'work/reel-motion/verified-device-moss',
                'optimized': build / 'moss-optimized'}
    profiles = [('previous-idle', 'previous', 17, 18, 25),
                ('optimized-idle', 'optimized', 17, 18, 25),
                ('previous-motion', 'previous', 17, 10, 25),
                ('optimized-motion', 'optimized', 17, 10, 25)]
else:
    binaries = {'previous': build / 'moss-optimized', 'candidate': build / 'moss',
                'test-arm': build / 'color-test-arm', 'test-motion': build / 'test-arm',
                'test-engine':build/'test-engine'}
    profiles = [('previous-idle', 'previous', 17, 18, 20),
                ('color-idle', 'candidate', 17, 18, 20),
                ('previous-pond', 'previous', 17, 10, 20),
                ('color-pond', 'candidate', 17, 10, 50),
                ('color-drive', 'candidate', 19, 24, 20),
                ('color-orbit', 'candidate', 17, 9, 20),
                ('color-song', 'candidate', 20, 10, 30)]
assert all(source.is_file() for source in binaries.values()), 'Build all binaries before starting'
shell('mkdir ' + remote)
hashes = {}
for name, source in binaries.items():
    push(source, remote + '/' + name)
    hashes[name] = hashlib.sha256(source.read_bytes()).hexdigest()
    shell(f'chmod 755 {remote}/{name}')
push(root / 'native/prototype/packaging/cpu-performance.sh', remote + '/cpu-performance.sh')
script = '''#!/bin/sh
PATH=/bin:/usr/bin:/sbin:/usr/sbin
export PATH SDL_NOMOUSE=1
cd REMOTE || exit 2
. ./cpu-performance.sh || exit 3
for test in test-arm test-motion test-engine; do
  if [ -x "$test" ]; then
    ./$test > "$test.log" 2>&1
    status=$?; echo "$status" > "$test.exit"
    [ "$status" -eq 0 ] || exit "$status"
  fi
done
run_profile() {
  name=$1; binary=$2; kind=$3; mask=$4; seconds=$5
  echo "$name" > current
  ./$binary --profile "$kind" "$mask" "$seconds" > "$name.log" 2>&1
  status=$?; echo "$status" > "$name.exit"
  report=$(ls -t perf/run/reports/*.json | head -n 1)
  cp "$report" "$name.json" || exit 4
  [ "$status" -eq 0 ] || exit "$status"
}
'''.replace('REMOTE', remote)
for row in profiles:
    script += 'run_profile ' + ' '.join(map(str, row)) + '\n'
if mode=='micro':script+='./test-arm --bench > bench.log 2>&1\n'
script += 'sync\necho done > complete\n'
launcher = work / (mode + '-launcher.sh')
launcher.write_text(script, newline='\n')
manifest = {'remote': remote, 'mode': mode, 'sha256': hashes, 'profiles': profiles}
(work / (mode + '-build.json')).write_text(json.dumps(manifest, indent=2) + '\n')
push(launcher, '/mnt/mmc/CFW/retroarch/command.sh')
shell('kill ' + menu[0])
print(f'Started {mode}: {sum(p[-1] for p in profiles)} seconds of audio profiles.')
