#!/usr/bin/env python3
"""Compare independently compiled native Linux and Wasm opening traces."""
from pathlib import Path
import subprocess,json,ast,shutil,argparse
root=Path(__file__).resolve().parents[1];out=root/'docs/evidence';out.mkdir(exist_ok=True)
node=shutil.which('node')
config=root/'.cache/emsdk/.emscripten'
if config.exists():
 for line in config.read_text().splitlines():
  if line.startswith('NODE_JS = '):node=ast.literal_eval(line.split('=',1)[1].strip()).replace('$CFGDIR',str(config.parent))
if not node:raise SystemExit('Node unavailable; run scripts/bootstrap-mac.sh first')
parser=argparse.ArgumentParser();parser.add_argument('--mac-trace',default='build-core/caocao_trace');args=parser.parse_args()
results=[]
for choice in (1,2):
 native=json.loads(subprocess.check_output(['docker','run','--rm','--network','none','--platform','linux/amd64','-v',str(root)+':/work:ro','caocao-engine-build:0.1','build-linux/caocao_trace','/work/assets/generated',str(choice)],cwd=root))
 wasm=json.loads(subprocess.check_output([node,'build-web/caocao_trace.js',str(root/'assets/generated'),str(choice)],cwd=root))
 if native!=wasm:raise SystemExit(f'Choice {choice}: trace diverges')
 mac=json.loads(subprocess.check_output([args.mac_trace,str(root/'assets/generated'),str(choice)],cwd=root))
 if native!=mac:raise SystemExit(f'Choice {choice}: macOS trace diverges')
 version='.'.join(native['engine'].split('.')[:2])
 (out/f'trace-choice-{choice}-{version}.json').write_text(json.dumps(native,ensure_ascii=False,indent=2)+'\n')
 results.append({'choice':choice,'checkpoints':len(native['trace']),'all_state_hashes_equal':True,'final_hash':native['final_hash'],'stopped_at':native['diagnostic']})
(out/f'parity-{version}.json').write_text(json.dumps({'engine':native['engine'],'manifest':native['manifest'],'platforms':['macOS arm64','Linux x86-64','WebAssembly/Node'],'scope':'Both opening choices through all three allied attacks, first enemy retirement, enemy-turn banner, reinforcement dialogue and scripted entrance walks and the first enemy tactic decision, movement, targeting preview, poison plan, caster pose, tactic caption full poison effect player/reaction, target HP application, caster MP cost and action flags. Then executes the archer death dialogue and retirement, two event batches, and the second enemy decision/walk/preview/plan/caster/caption/poison effect. MEFF#49 and the second cast status/MP application complete, followed by five physical attacks and six movement-only actions with their post-action event scans. Continues through the original turn increment, environment RNG update, action-flag-reset banner and turn-two fire-attack dialogue with speaker focus; completes the first three fire placements with original terrain/mark state, Guan Yu movement and dialogue, then preloads and plays MCALL05 across four dialogue save points, places all 27 fires and continues through enemy panic animations/dialogue then applies area confusion to 13 enemies, processes the six non-targeted AI instructions and the Cao Cao/cavalry arrival dialogue and walks, then displays the objective banner and its terminal pause, highlights Zhang Bao and Zhang Liang, and makes Xu Zijiang appear, then executes tutorial entry. Choice1 accepts the basic and ally/enemy lessons, including cursor movement, both unit panels, guide walking/poses, Liu Bei movement range and attack mask, and their dismissal/cleanup, then demonstrates the blue movement range, moves and cancels Cao Cao, opens the original command-menu demonstration (trace selects cancel), then plays the full terrain lesson, terrain panel, Liu Bei affinity panel, summary and farewell then executes opcode49 guide disappearance, opcode79 facing, opcode90 battle-start banner and variable13 completion. Choice2 declines the tutorial, completes farewell and follows the same exit commands. Both execute turn-two recovery and the no-opponent poison panel before applying11HP loss to ally210, then stop explicitly at the unported player input loop44e1d3. Tutorial choices are separate from the opening story choice. No player battle actions or victory parity.','choices':results},indent=2)+'\n')
print(json.dumps(results,indent=2))
