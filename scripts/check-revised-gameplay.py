#!/usr/bin/env python3
"""Run the finite revised-rules checks; makes no network or JEV requests."""
import argparse,json,subprocess,time
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('build');p.add_argument('label')
names=['combat','events','tactics_extended','campaign','revised']
p.add_argument('--test',choices=names,help='Refresh one changed suite while preserving other recorded results')
a=p.parse_args()
root=Path(__file__).resolve().parent.parent;out=root/'docs/evidence/revised-gameplay';out.mkdir(parents=True,exist_ok=True)
checks=out/f'checks-{a.label}.json'
results=json.loads(checks.read_text()) if a.test and checks.exists() else []
for name in ([a.test] if a.test else names):
 target=root/a.build/f'prototype_{name}_tests';runner=[str(target)]
 if target.with_suffix('.js').exists():runner=['node',str(target.with_suffix('.js'))]
 report=out/f'{name}-{a.label}.json';started=time.monotonic()
 proc=subprocess.run(runner+[str(root/'assets/prototype'),str(report)],cwd=root,text=True,capture_output=True)
 result=dict(test=name,exit_code=proc.returncode,seconds=round(time.monotonic()-started,3),stdout=proc.stdout,stderr=proc.stderr)
 results=[r for r in results if r['test']!=name]+[result]
 results.sort(key=lambda r:names.index(r['test']))
 checks.write_text(json.dumps(results,ensure_ascii=False,indent=2)+'\n')
 print(a.label,name,'PASS' if proc.returncode==0 else 'FAIL',flush=True)
 if proc.returncode:print(proc.stdout,proc.stderr);raise SystemExit(proc.returncode)
