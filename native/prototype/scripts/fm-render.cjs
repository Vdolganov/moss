// Reproducible, sample-free FM audition bank and four-track demonstration.
const fs=require('fs'),path=require('path'),{execFileSync}=require('child_process');
const {root,run,hostFlags}=require('./toolchain.cjs');
const output=process.argv[2]?path.resolve(process.argv[2]):path.resolve(root,'../../media/fm');
fs.mkdirSync(output,{recursive:true});
const binary=path.join(root,'build/fm_render.exe');
run([...hostFlags(),'-Isrc','tests/fm_render.cpp','-o',binary]);
execFileSync(binary,[output],{cwd:root,stdio:'inherit'});
console.log(output);
