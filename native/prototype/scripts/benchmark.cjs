// Build identical headless workloads for Windows resource limits and real ARM.
const {run, hostFlags, buildArm} = require('./toolchain.cjs');
buildArm('tests/performance.cpp', 'build/benchmark-arm', {includes:['src']});
run([...hostFlags(), '-I', 'src', 'tests/performance.cpp', '-lpsapi', '-o', 'build/performance.exe']);
