const fs = require('fs');
const path = require('path');
const {execFileSync} = require('child_process');
const {root, run, hostFlags, buildArm} = require('./toolchain.cjs');
fs.mkdirSync(path.join(root, 'build/granular'), {recursive:true});
run([...hostFlags(), 'tests/granular_test.cpp', '-o', 'build/granular/test.exe']);
execFileSync(path.join(root, 'build/granular/test.exe'), [], {cwd:root, stdio:'inherit'});
buildArm('tests/granular_test.cpp', 'build/granular/test-arm', {includes:['src']});
run([...hostFlags(), 'tests/granular_benchmark.cpp', '-o', 'build/granular/benchmark.exe']);
buildArm('tests/granular_benchmark.cpp', 'build/granular/benchmark-arm', {includes:['src']});
buildArm('tests/granular_device.cpp', 'build/granular/device-arm', {includes:['src']});
run([...hostFlags(), 'tests/granular_audio.cpp', '-o', 'build/granular/audio.exe']);
fs.mkdirSync(path.join(root, 'build/granular/audio'), {recursive:true});
execFileSync(path.join(root, 'build/granular/audio.exe'), ['samples', 'build/granular/audio'],
             {cwd:root, stdio:'inherit'});
