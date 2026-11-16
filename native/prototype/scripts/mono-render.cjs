const {run,hostFlags,root}=require('./toolchain.cjs');
const fs=require('fs'),path=require('path'),{execFileSync}=require('child_process');
const output=path.resolve(process.argv[2]||path.join(root,'../../work/mono-research/audio'));
fs.mkdirSync(output,{recursive:true});
run([...hostFlags(),'tests/mono_render.cpp','-o','build/mono-render.exe']);
execFileSync(path.join(root,'build/mono-render.exe'),[output],{cwd:root,stdio:'inherit'});
