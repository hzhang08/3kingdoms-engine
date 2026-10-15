import {GameTouch} from './game-touch';
import {SpecialAccount} from './special-account';
import {SpecialStore} from './special-store';
import {terminalResult,type SpecialSaved,type RankedMatch} from './special-results';
import {MatchDiagnostics,type Selection} from './special-diagnostics';
declare global {interface Window {prototypeSpecialSelected?:(selection:Selection)=>void;}}
const matchDiagnostics=new MatchDiagnostics({getItem:key=>localStorage.getItem(key),setItem:(key,value)=>localStorage.setItem(key,value)});
window.prototypeSpecialSelected=selection=>{matchDiagnostics.selected(selection);registering=true;pause(true);queueMicrotask(()=>void registerSelected());};
const $=<T extends HTMLElement=HTMLElement>(id:string)=>document.getElementById(id)! as T;
const canvas=$<HTMLCanvasElement>('canvas'),modal=$<HTMLDialogElement>('modal');
let authorized=false,registering=false,activeMatch:RankedMatch|undefined,store:SpecialStore,accountId='';
let ready=false,english=localStorage.getItem('caocao-special-language')==='en',lastHash='',saving:Promise<void>|undefined;
const label=(zh:string,en:string)=>english?en:zh;
const call=(name:string,result:string|null='number',args:string[]=[],values:unknown[]=[])=>window.Module.ccall(name,result,args,values);
const choosing=()=>ready&&JSON.parse(call('prototype_web_ui_state','string')).choosing===true;
const status=(text:string)=>{$('status').textContent=text;};
const pause=(value:boolean)=>{if(ready)call('prototype_web_pause',null,['number'],[value||!authorized||registering?1:0]);};
const audio=new Audio('/music/track-3.mp3');audio.loop=true;audio.volume=.3;
let musicOn=false;
const account=new SpecialAccount(label,()=>{authorized=false;pause(true);audio.pause();if(modal.open)modal.close();if(ready)touch.cancel();});account.bind();
const touch=new GameTouch($('battle-viewport'),canvas,{
 active:()=>ready&&authorized&&!registering&&!modal.open&&!document.hidden,
 tap:p=>call('prototype_web_tap','number',['number','number'],[p.x,p.y]),
 inspect:p=>call('prototype_web_inspect','number',['number','number'],[p.x,p.y]),
 close:()=>call('prototype_web_close_details',null),ui:()=>JSON.parse(call('prototype_web_ui_state','string'))
},{width:1536,height:1184});
const rosterVersion='hulao-duel-v1';
type Saved=SpecialSaved;
function matchup(saved:Saved){try{const p=JSON.parse(saved.snapshot).payload;const en:Record<number,string>={0:'Cao Cao',6:'Xiahou Yuan',1:'Xiahou Dun',11:'Li Dian',119:'Lu Bu',127:'Dong Zhuo'};const name=(id:number)=>english?en[id]:p.state.units[id].name;return name(p.duel_hero??0)+[p.duel_ally,p.duel_ally2,p.duel_ally3].filter(id=>id>=0).map(id=>' + '+name(id)).join('')+' vs '+name(p.duel_enemy??127);}catch{return '';}}
async function slot(n:number|string,write?:Saved):Promise<Saved|undefined>{if(write){await store.put(n,write);return write;}return store.get(n);}
async function recoveryKeys():Promise<string[]>{return (await store.keys()).filter((key):key is string=>typeof key==='string'&&/^recovery-\d+$/.test(key)).sort();}
function snapshot():Saved{
 const raw=call('prototype_snapshot','string');if(!raw)throw Error(call('prototype_error','string'));
 const s=JSON.parse(raw);if(s.payload.scenario!==2)throw Error('Wrong special battle snapshot');
 return {snapshot:raw,at:Date.now(),turn:s.payload.state.turn,phase:s.payload.state.phase,rosterVersion,...(activeMatch?{match:{...activeMatch}}:{})};
}
async function save(n=0,quiet=false){
 while(saving)await saving;
 if(!ready||choosing())return;const s=snapshot(),hash=JSON.parse(s.snapshot).sha256;
 if(n===0&&hash===lastHash)return;
 saving=(async()=>{try{await slot(n,s);await queueResult(s);if(n===0)lastHash=hash;status(label(`已保存至此浏览器 · 回合 ${s.turn}`,`Saved in this browser · Turn ${s.turn}`));}
 catch(e){if(!quiet)throw e;status(label('自动保存失败，请导出备份。','Autosave failed. Please export a backup.'));}})();
 try{await saving;}finally{saving=undefined;}
}
function restore(raw:string,match?:RankedMatch){
 if(!call('prototype_restore','number',['string'],[raw]))throw Error(call('prototype_error','string')||'Incompatible save');
 const p=JSON.parse(raw).payload;matchDiagnostics.restored(p.duel_hero??0,p.duel_enemy??127,p.state.phase,p.duel_ally??-1,p.duel_ally2??-1,p.duel_ally3??-1);
 activeMatch=match?{...match}:undefined;lastHash='';touch.start(true);matchLabel();
}
function show(title:string){touch.cancel();pause(true);$('modal-title').textContent=title;$('modal-content').replaceChildren();if(!modal.open)modal.showModal();}
function close(){modal.close();pause(false);canvas.focus();}
$('modal-close').onclick=close;modal.addEventListener('cancel',e=>{e.preventDefault();close();});
function button(text:string,work:()=>void|Promise<void>){const b=document.createElement('button');b.textContent=text;b.onclick=()=>{Promise.resolve().then(work).catch(e=>status((e as Error).message));};return b;}
async function saves(){
 if(!ready||!authorized||registering)return;if(call('prototype_busy')){status(label('请等待动画结束。','Please wait for the animation.'));return;}
 if(!choosing())await save(0);void flushResults();show(label('本机存档','Local saves'));const content=$('modal-content');
 const note=document.createElement('p');note.className='note';note.textContent=label('存档保存在此浏览器当前账号下；特别战场胜败随账号记录。导入备份和旧存档不排名，更换设备前请导出备份。','Saves belong to this account on this browser; special match results are stored with your account. Imported backups and previous saves are unranked. Export before changing devices.');content.append(note);
 for(let n=0;n<=10;++n){const saved=await slot(n);const row=document.createElement('div');row.className='slot';const title=document.createElement('span');title.textContent=(n===0?label('自动','Auto'):String(n))+' · '+(saved?matchup(saved)+' · '+label(`回合 ${saved.turn}`,`Turn ${saved.turn}`)+' · '+new Date(saved.at).toLocaleString():label('空','Empty'));row.append(title);
  if(n>0&&!choosing())row.append(button(label('保存','Save'),async()=>{if(saved&&!confirm(label('覆盖这个存档位？','Overwrite this slot?')))return;await save(n);await saves();}));
  if(saved)row.append(button(label('读取','Load'),()=>{restore(saved.snapshot,saved.match);close();status(label('已恢复存档。','Save restored.'));}));content.append(row);
 }
 for(const key of await recoveryKeys()){const saved=await slot(key);if(!saved)continue;const row=document.createElement('div');row.className='slot';const title=document.createElement('span');title.textContent=label('保留的旧存档 · ','Retained previous save · ')+new Date(Number(key.slice(9))).toLocaleString();row.append(title,button(label('导出原始备份','Export original backup'),()=>download(saved.snapshot,'hulao-'+key+'.json')));content.append(row);}
 content.append(button(label('查看更新前的本机存档（不排名）','Previous local saves (unranked)'),legacySaves));
 const actions=document.createElement('div');actions.className='actions';if(!choosing())actions.append(button(label('导出备份','Export backup'),exportSave));actions.append(button(label('导入备份','Import backup'),()=>$('import-file').click()),button(label('导出对局诊断','Export match diagnostics'),()=>download(matchDiagnostics.export(),'hulao-match-diagnostics.json')));content.append(actions);
 const diagnosticNote=document.createElement('p');diagnosticNote.className='note';diagnosticNote.textContent=label('对局诊断仅记录本机最近 50 次选敌和读档，用于排查重新开始问题；不含账号信息，不会上传。','Match diagnostics keep the last 50 local selections and restores for restart troubleshooting. No account information or automatic upload.');content.append(diagnosticNote);
}
function download(raw:string,name:string){const url=URL.createObjectURL(new Blob([raw],{type:'application/json'}));const a=document.createElement('a');a.href=url;a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}
function exportSave(){const s=snapshot();download(s.snapshot,'hulao-duel-turn-'+s.turn+'.json');}
$<HTMLInputElement>('import-file').onchange=async e=>{const input=e.target as HTMLInputElement;const f=input.files?.[0];try{if(f){if(!authorized||registering||call('prototype_busy'))return;if(f.size>2_000_000)throw Error('Save exceeds 2 MB');if(!choosing())await save(0);void flushResults();restore(await f.text());close();status(label('已导入；现有手动存档未改动。','Imported. Existing manual slots are unchanged.'));}}catch(e){status((e as Error).message);}finally{input.value='';}};
async function restart(){if(!ready||!authorized||registering||call('prototype_busy'))return;if(!choosing())await save(0);void flushResults();show(label('重新挑战虎牢关','Restart Hulao Pass'));const p=document.createElement('p');p.className='note';p.textContent=label('选择曹操、夏侯渊、夏侯惇或李典。敌军随机为吕布或董卓；迎战董卓时，随机增加一名队友，共两人出战；迎战吕布时，另外三名武将全部加入，共四人出战，均由你控制。新对局迎战董卓不赠送恢复用豆，迎战吕布赠送3个。我军全体武将被击败或超过20回合才会失败。已有存档保持原有阵容。','Choose Cao Cao, Xiahou Yuan, Xiahou Dun, or Li Dian. Face a random Lu Bu or Dong Zhuo. Against Dong Zhuo, one random teammate joins you (two friendly units). Against Lu Bu, all three other heroes join you (four friendly units). You control every friendly hero. New matches grant no Healing Beans against Dong Zhuo and three against Lu Bu. You lose only when all friendly units fall or the 20-turn limit expires. Existing saves keep their lineup.');$('modal-content').append(p,button(label('选择武将','Choose hero'),()=>{if(!call('prototype_special_start'))throw Error(call('prototype_error','string'));lastHash='';touch.start(true);close();}));}
function applyLanguage(){
 document.documentElement.lang=english?'en':'zh-CN';localStorage.setItem('caocao-special-language',english?'en':'zh');
 $('title').textContent=label('虎牢关 · 体素对决','Hulao · Voxel Duel');$('saves').textContent=label('存档','Saves');$('ranks').textContent=label('英雄榜','Leaderboard');account.render();matchLabel();$('restart').textContent=label('重新开始','Restart');$('fullscreen').textContent=label('全屏','Full screen');$('campaign').textContent=label('四战战役 ↗','Campaign ↗');$('modal-close').textContent=label('关闭','Close');$('zoom-reset').textContent=label('复位','Reset');$('music').textContent=label(musicOn?'♫ 音乐开':'♫ 音乐',musicOn?'♫ On':'♫ Music');
 if(!ready)status(label('需要 Google 登录 · 胜败记录随账号保存','Google sign-in required · results saved to your account'));
 touch.setLanguage(english);$('wait').textContent=label('待命','Wait');$('cancel').textContent=label('取消','Cancel');$('end-turn').textContent=label('结束回合','End turn');if(ready){call('prototype_web_language',null,['number'],[english?1:0]);status(label('进度保存在此浏览器','Progress is saved in this browser'));}
}
$('language').onclick=()=>{english=!english;applyLanguage();};$('saves').onclick=()=>void saves().catch(e=>status(e.message));$('restart').onclick=()=>void restart().catch(e=>status(e.message));$('ranks').onclick=()=>void ranks().catch(e=>status(e.message));
$('fullscreen').onclick=()=>{const request=document.fullscreenElement?document.exitFullscreen():document.documentElement.requestFullscreen?.();void request?.catch(()=>status(label('可在 Safari 中添加至主屏幕全屏游玩。','In Safari, add to Home Screen for full screen.')));};
$('music').onclick=()=>{musicOn=!musicOn;if(musicOn)void audio.play().catch(()=>{musicOn=false;applyLanguage();});else audio.pause();applyLanguage();};
window.prototypeSave=()=>void save(Math.max(1,call('prototype_slot'))).catch(e=>status(e.message));
window.prototypeLoad=()=>void saves().catch(e=>status(e.message));window.prototypeSlots=window.prototypeLoad;
window.prototypeHome=window.prototypeLoad;window.prototypeRestart=()=>void restart().catch(e=>status(e.message));window.prototypeNextBattle=window.prototypeRestart;
window.prototypeExport=exportSave;window.prototypeImport=()=>$('import-file').click();window.prototypeEnemy=()=>{};
window.prototypeReady=()=>{void(async()=>{
 ready=true;call('prototype_web_ai',null,['number'],[0]);if(!call('prototype_special_start'))throw Error(call('prototype_error','string'));
 let resumed=false,unreadable=false;try{const auto=await slot(0);if(auto){restore(auto.snapshot,auto.match);resumed=true;}}catch{unreadable=true;try{const old=await slot(0);if(old)await slot('recovery-'+Date.now(),old);}catch{}status(label('旧存档不可读取，已保留；从开局进入。','Previous save unreadable and retained; starting from the setup.'));}
 call('prototype_web_mobile_text',null,['number'],[matchMedia('(pointer:coarse)').matches?1:0]);applyLanguage();$('loading').hidden=true;touch.start(true);pause(false);canvas.focus();
 if(unreadable)status(label('旧存档不可读取，已保留。请从存档列表导出备份。','Previous save could not load and is retained. Export a backup from Saves.'));
 if(resumed)status(label('已继续上次战斗 · 存档保留在本机','Resumed previous battle · saves remain on this device'));
 if(activeMatch?.initial){registering=true;void registerCurrent().catch(registrationError);}else if(resumed){await queueResult(snapshot());void flushResults();}
 setInterval(()=>{if(ready&&authorized&&!registering&&!modal.open&&!document.hidden&&!call('prototype_busy'))void save(0,true).then(()=>flushResults()).catch(()=>{});},2000);
 })().catch(fail);};
function fail(e:unknown){$('loading').hidden=false;$('load-message').textContent=(e as Error).message||'Unable to load. Please refresh.';}
async function boot(){
 $('loading').hidden=false;
 const manifest=await fetch('/engine.json',{cache:'no-cache'}).then(r=>{if(!r.ok)throw Error('Unable to read engine version');return r.json();});
 matchDiagnostics.version=manifest.version;
 const data=new Uint8Array(manifest.bytes);let offset=0;
 for(const chunk of manifest.chunks){const r=await fetch(chunk.url);if(!r.ok)throw Error('Unable to download game assets');const b=new Uint8Array(await r.arrayBuffer());const hash=Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',b)),n=>n.toString(16).padStart(2,'0')).join('');if(b.length!==chunk.bytes||hash!==chunk.sha256)throw Error('Asset verification failed. Please refresh.');data.set(b,offset);offset+=b.length;$<HTMLProgressElement>('load-progress').value=offset/manifest.bytes*95;$('load-message').textContent=label('正在下载战场资源','Loading battlefield')+` ${Math.round(offset/1048576)} / ${Math.round(manifest.bytes/1048576)} MB`;}
 window.Module={hostedGame:true,specialHulao:true,canvas,getPreloadedPackage:()=>data.buffer,locateFile:(name:string)=>name.endsWith('.wasm')?manifest.wasm:manifest.script.replace('prototype.js',name),print:()=>{},printErr:(s:string)=>console.warn(s),onAbort:()=>fail(Error('Engine failed to load. Please refresh.'))};
 const script=document.createElement('script');script.src=manifest.script;script.onerror=()=>fail(Error('Engine download failed'));document.body.append(script);
}
// Special match results have their own authenticated API and account-scoped local saves.
window.addEventListener('pagehide',()=>{
 if(!ready||choosing()||call('prototype_busy'))return;
 void save(0,true).catch(()=>{});
 // A visible result can be followed immediately by closing the tab. Send a
 // small keepalive request as well as the local retry queue; the API is idempotent.
 const result=terminalResult(snapshot());if(authorized&&result){const data=JSON.stringify({snapshot:result.snapshot});if(new TextEncoder().encode(data).length<60000)void fetch('/api/special/hulao/matches/'+result.id+'/result',{method:'POST',credentials:'same-origin',keepalive:true,headers:{'Content-Type':'application/json','X-Special-Account':accountId},body:data}).catch(()=>{});}
});
document.addEventListener('visibilitychange',()=>{if(ready){if(document.hidden){pause(true);audio.pause();if(!call('prototype_busy'))void save(0,true).catch(()=>{});}else{void verifySession();pause(modal.open);if(musicOn)void audio.play().catch(()=>{});}}});
applyLanguage();void (async()=>{const user=await account.check();if(!user)return;accountId=user.id;authorized=true;store=new SpecialStore(user.id);await boot();})().catch(e=>{if(authorized)fail(e);else $('auth-error').textContent=e.message;});
setInterval(()=>{if(authorized)void verifySession();},60000);

for(const [index,id] of ['wait','cancel','end-turn'].entries())$(id).onclick=()=>{if(!ready||!authorized||registering)return;if(!call('prototype_web_tool','number',['number'],[index]))status(label('请先完成当前动画或关闭窗口。','Finish the current animation or close the panel first.'));canvas.focus();};

let flushing:Promise<void>|undefined;
function matchLabel(){
 const target=$('match-status');if(!target)return;
 target.textContent=!ready?'':choosing()?label('选择武将，开始排名对局','Choose a hero to start a ranked match'):activeMatch?label(registering?'正在登记对局……':'排名对局 · 胜败随账号保存',registering?'Registering match...':'Ranked match · results saved to account'):label('旧存档 / 导入备份：不计排名','Previous / imported save: unranked');
}
async function queueResult(saved:Saved){
 const result=terminalResult(saved);if(!result||await store.get('recorded:'+result.id)||await store.get('result:'+result.id))return;
 await store.put('result:'+result.id,result);
}
function flushResults(){
 if(flushing)return flushing;if(!authorized)return Promise.resolve();
 flushing=(async()=>{for(const key of await store.keys())if(typeof key==='string'&&key.startsWith('result:')){
  const result=await store.get<{id:string;snapshot:string}>(key);if(!result)continue;
  try{const recorded=await account.api('/special/hulao/matches/'+result.id+'/result','POST',{snapshot:result.snapshot});await store.put('recorded:'+result.id,recorded);await store.remove(key);if(activeMatch?.id===result.id)$('match-status').textContent=label('本局'+(recorded.outcome==='victory'?'胜利':'战败')+'已计入英雄榜','This '+(recorded.outcome==='victory'?'win':'loss')+' is recorded');}
  catch(e){$('match-status').textContent=label('战绩未同步，已保留；联网后重试','Result pending, saved locally; retry when online');break;}
 }})().catch(()=>{$('match-status').textContent=label('战绩未同步，请重试','Result not synced; please retry');}).finally(()=>{flushing=undefined;});return flushing;
}
async function registerSelected(){
 try{activeMatch={id:crypto.randomUUID(),initial:call('prototype_snapshot','string')};if(!activeMatch.initial)throw Error('Starting snapshot unavailable');lastHash='';await save(0);await registerCurrent();}catch(e){registrationError(e);}
}
async function registerCurrent(){
 if(!activeMatch?.initial)return;const selected=activeMatch;registering=true;pause(true);matchLabel();
 await account.api('/special/hulao/matches','POST',{id:selected.id,snapshot:selected.initial});
 activeMatch={id:selected.id};lastHash='';await save(0);registering=false;matchLabel();if(modal.open)close();pause(document.hidden);void flushResults();
}
function registrationError(e:unknown){
 registering=true;pause(true);matchLabel();show(label('对局尚未登记','Match not registered'));const note=document.createElement('p');note.className='note';note.textContent=(e as Error).message;const explanation=document.createElement('p');explanation.className='note';explanation.textContent=label('开局已保留。请连接网络后重试，登记成功后即可开始。','Your starting position is saved. Reconnect and retry to begin.');$('modal-content').append(note,explanation,button(label('重试登记','Retry registration'),()=>registerCurrent().catch(registrationError)));
}
async function verifySession(){
 if(!authorized)return;try{const user=(await account.api('/me')).user;if(!user||user.id!==accountId||!user.username){authorized=false;pause(true);touch.cancel();audio.pause();if(modal.open)modal.close();account.user=null;account.render();$('auth-error').textContent=label('登录已过期或账号已切换。进度已保留，请重新登录或刷新。','Session expired or account changed. Progress is retained; sign in or refresh.');}}catch{} // A transient network error does not destroy the local battle.
}
$('sign-out').onclick=()=>{void(async()=>{if(ready&&!choosing()&&!call('prototype_busy'))await save(0);await flushResults();await account.api('/auth/sign-out','POST',{});authorized=false;pause(true);location.reload();})().catch(e=>status(e.message));};
async function legacySaves(){
 show(label('更新前的本机存档','Previous local saves'));const content=$('modal-content');
 const db=await new Promise<IDBDatabase>((resolve,reject)=>{const r=indexedDB.open('caocao-special-hulao-duel-v1',1);r.onupgradeneeded=()=>r.result.createObjectStore('slots');r.onsuccess=()=>resolve(r.result);r.onerror=()=>reject(r.error);});
 try{const rows=await new Promise<{key:IDBValidKey;saved:Saved}[]>((resolve,reject)=>{const tx=db.transaction('slots','readonly'),records:{key:IDBValidKey;saved:Saved}[]=[];const r=tx.objectStore('slots').openCursor();r.onsuccess=()=>{const c=r.result;if(c){records.push({key:c.key,saved:c.value});c.continue();}};tx.oncomplete=()=>resolve(records);tx.onerror=()=>reject(tx.error);});
 const note=document.createElement('p');note.className='note';note.textContent=label('这些是登录要求更新前的本机存档。可继续或导出，胜败不补计；原始存档保持不变。','These saves predate sign-in. Continue or export them without ranking; original slots are preserved.');content.append(note);
 for(const {key,saved} of rows){const row=document.createElement('div');row.className='slot';const text=document.createElement('span');text.textContent=String(key)+' · '+matchup(saved);row.append(text,button(label('读取（不排名）','Load (unranked)'),()=>{restore(saved.snapshot);close();}),button(label('导出','Export'),()=>download(saved.snapshot,'hulao-previous-'+key+'.json')));content.append(row);}
 if(!rows.length)content.append(document.createTextNode(label('没有更新前的本机存档。','No previous local saves.')));
 }finally{db.close();}
}
async function ranks(page=0){
 if(!authorized||!ready||registering||call('prototype_busy'))return;
 if(!choosing())await save(0);show(label('虎牢关 · 英雄榜','Hulao · Leaderboard'));$('modal-content').textContent=label('正在读取战绩……','Loading results...');await flushResults();
 const result=await account.api('/special/hulao/leaderboard?page='+page),content=$('modal-content');content.replaceChildren();
 const note=document.createElement('p');note.className='rank-note';note.textContent=label('有胜场者优先，仅按胜场从高到低排名；零胜场者按败场从高到低排名。同分并列，败场不影响有胜场者的排名。每局仅记录一次；未结束或导入的对局不计。','Players with wins come first, ranked only by wins. Players with zero wins rank by losses, highest first. Equal scores share a rank; losses never break a winning player’s tie. Each finished match counts once; unfinished and imported games do not count.');
 const summary=document.createElement('p');summary.className='rank-summary';summary.textContent=label('我的战绩：','Wins: ')+result.you.wins+label(' 胜 / ',' / Losses: ')+result.you.losses+label(' 败 · 排名 ',' · Rank ')+(result.you.rank??'—');content.append(note,summary);
 if(result.entries.length){const table=document.createElement('table');table.className='rank-table';const head=table.createTHead().insertRow();for(const title of [label('排名','Rank'),label('用户名','Username'),label('胜场','Wins'),label('败场','Losses')]){const cell=document.createElement('th');cell.textContent=title;cell.scope='col';head.append(cell);}const tbody=table.createTBody();
 for(const entry of result.entries){const row=tbody.insertRow();if(entry.isYou)row.className='mine';for(const value of [entry.rank,entry.username+(entry.isYou?label('（我）',' (you)'):''),entry.wins,entry.losses])row.insertCell().textContent=String(value);}content.append(table);
 }else{const empty=document.createElement('p');empty.textContent=label('还没有已完成对局。完成第一局，留下你的战绩！','No completed matches yet. Finish a match to join the leaderboard!');content.append(empty);}
 const pages=document.createElement('div');pages.className='actions';if(page)pages.append(button(label('上一页','Previous'),()=>ranks(page-1)));if(result.hasMore)pages.append(button(label('下一页','Next'),()=>ranks(page+1)));pages.append(button(label('刷新战绩','Refresh'),()=>ranks(page)));content.append(pages);
}
