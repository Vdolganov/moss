"""Start the isolated stage-one REEL laboratory through Garlic's launcher.

Requires the user's console to be connected and idle in Garlic. Does not install
or replace user applications, projects or libraries. Refuses existing test dirs.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

root = Path(__file__).resolve().parents[3]
adb = str(root / 'native/tooling/platform-tools/adb.exe')
remote = '/mnt/mmc/MOSS-Reel-Motion-v5'
work = root / 'work/reel-motion'
work.mkdir(parents=True, exist_ok=True)
mode='full' if '--full-matrix' in sys.argv else 'quick'


def shell(command):
    return subprocess.check_output([adb, 'shell', command], text=True).replace('\r', '')


def push(source, target):
    subprocess.run([adb, 'push', str(source), target], check=True, capture_output=True)


processes = shell('ps')
assert not re.search(r'\./moss\b', processes), 'MOSS is open; preserve the current session'
menu = [line.split()[1] for line in processes.splitlines() if line.rstrip().endswith(' ./main')]
assert len(menu) == 1, 'Expected the Garlic menu'
assert remote.rsplit('/', 1)[1] not in shell('ls /mnt/mmc'), 'Use a fresh isolated test directory'
assert shell('if [ -e /mnt/mmc/CFW/retroarch/command.sh ]; then echo occupied; fi').strip() == '', 'A launcher is pending'
shell(f'mkdir -p {remote}')
if mode=='quick':shell(f'echo quick > {remote}/quick')
hashes = {}
for name in ['moss', 'moss-baseline', 'test-arm']:
    source = root / 'native/prototype/build/reel-motion' / name
    push(source, remote + '/' + name)
    hashes[name] = hashlib.sha256(source.read_bytes()).hexdigest()
    shell(f'chmod 755 {remote}/{name}')
push(root / 'native/prototype/packaging/cpu-performance.sh', remote + '/cpu-performance.sh')
(work / 'build.json').write_text(json.dumps({'remote': remote, 'mode':mode, 'sha256': hashes}, indent=2) + '\n')
push(root / 'native/prototype/scripts/reel-motion-device.sh', '/mnt/mmc/CFW/retroarch/command.sh')
shell('kill ' + menu[0])
print('Started isolated ARM checks and '+('160 seconds of focused hardware profiles.' if mode=='quick' else 'the full effect matrix plus a 3-minute soak.'))
print(json.dumps(hashes, indent=2))
