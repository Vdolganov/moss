// Aggregate the existing checks; preserves each suite's independent assertions.
import {spawnSync} from 'node:child_process';
import fs from 'node:fs';
const out='../../work/ux-fixes-2026-11-20';
const suites=[['model','--test','tests/model.test.mjs','tests/note-timing.test.mjs'],...['browser','regression','tab-strips','nested-pages','tape-monochrome','spacing','grid-cells','grid-pages','note-duration','ux-reliability'].map(name=>[name,`tests/${name}.mjs`])];
const results=[];
for(const [name,...args]of suites){const start=Date.now(),r=spawnSync(process.execPath,args,{encoding:'utf8',timeout:120000});fs.writeFileSync(`${out}/${name}.log`,r.stdout+r.stderr);results.push({suite:name,exit:r.status,ms:Date.now()-start,error:r.error?.message});console.log(`${r.status===0?'PASS':'FAIL'} ${name} (${Date.now()-start}ms)`);}
fs.writeFileSync(`${out}/verification.json`,JSON.stringify(results,null,2));if(results.some(r=>r.exit!==0))process.exitCode=1;
