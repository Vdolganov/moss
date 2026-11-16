// Deterministic synthetic sources: no external sample pack is required.
const {run,hostFlags,root}=require('./toolchain.cjs');
const {execFileSync}=require('child_process');
const fs=require('fs'),path=require('path');
const output=path.resolve(process.argv[2]||path.join(root,'../../work/ambient/audio'));
fs.mkdirSync(output,{recursive:true});
run([...hostFlags(),'-I','src','tests/ambient_render.cpp','-o','build/ambient-render.exe']);
execFileSync(path.join(root,'build/ambient-render.exe'),[output],{cwd:root,stdio:'inherit'});
console.log(output);
