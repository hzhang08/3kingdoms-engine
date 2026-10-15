import {GameTouch} from './game-touch';
import {readLanguage,saveLanguage,siteText,languageLocale} from './site-language';
declare global {interface Window {Module:any;prototypeReady:()=>void;prototypeSave:()=>void;prototypeLoad:()=>void;prototypeSlots:()=>void;prototypeHome:()=>void;prototypeRestart:()=>void;prototypeNextBattle:()=>void;prototypeEnemy:(raw:string)=>void;prototypeExport:()=>void;prototypeImport:()=>void;}}
const $=<T extends HTMLElement=HTMLElement>(id:string)=>document.getElementById(id)! as T;
const escape=(s:unknown)=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]!));
const battles=['颍川之战','汜水关之战','虎牢关之战','董卓追击战'];
const phases:Record<string,string>={player:'我军回合',ally:'友军回合',enemy:'敌军回合',victory:'胜利',defeat:'战败'};
type Save={slot:number;run_id:string;scenario:number;turn:number;phase:string;cleared:number;revision:number;saved_at:number;state_hash:string;snapshot?:string};
let user:{id:string;username:string|null}|null=null,saves:Save[]=[],runId='',ready=false,playing=false,busy=false,savePromise:Promise<any>|null=null,lastAutoHash='',musicOn=localStorage.getItem('caocao-music')!=='off',aiOn=true,enginePromise:Promise<void>|null=null,engineResolve:()=>void=()=>{},engineReject:(e:Error)=>void=()=>{},lastReady=false;
const modal=$<HTMLDialogElement>('modal'),audio=new Audio();audio.id='battle-music';audio.loop=true;audio.volume=.35;document.body.append(audio);
let english=readLanguage(localStorage)==='en';
const t=(source:string,values:Record<string,string|number>={})=>siteText(source,english?'en':'zh',values);
const translatedMessage=(source:string)=>{const text=t(source);return ready&&english&&text===source?call('prototype_web_translate','string',['string'],[source]):text;};
let statusText={source:'正在连接……',values:{} as Record<string,string|number>};
const renderStatus=()=>{$('status').textContent=Object.keys(statusText.values).length?t(statusText.source,statusText.values):translatedMessage(statusText.source);};
const status=(source:string,values:Record<string,string|number>={})=>{statusText={source,values};renderStatus();};
const date=(ms:number)=>new Date(ms).toLocaleString(languageLocale(english?'en':'zh'),{hour12:false});
let loadingText={source:'首次进入需要下载游戏资源，之后浏览器会缓存。',values:{} as Record<string,string|number>};
function loadingLabel(source:string,values:Record<string,string|number>={}){loadingText={source,values};$('load-detail').textContent=t(source,values);}
function renderHomeText(){
 $('welcome').replaceChildren(document.createTextNode(user?.username??t('逐鹿天下')),document.createElement('br'),document.createTextNode(t(user?.username?'别来无恙':'从此启程')));
 $('save-count').textContent=user?.username?t('{count} / 10 个手动存档位',{count:saves.filter(s=>s.slot>0).length}):t('十个手动存档位');
}
// Explicitly marked UI copy only: never walk and translate user-generated content.
function translateStatic(root: ParentNode, language: 'zh'|'en') {
 for (const el of root.querySelectorAll<HTMLElement>('[data-i18n]')) el.textContent = siteText(el.dataset.i18n!, language);
 for (const attr of ['aria-label', 'placeholder', 'content', 'alt']) {
  for (const el of root.querySelectorAll<HTMLElement>(`[data-i18n-${attr}]`)) el.setAttribute(attr, siteText(el.getAttribute(`data-i18n-${attr}`)!, language));
 }
}

let modalRenderer:(()=>void)|null=null;

const call=(name:string,result:string|null='number',args:string[]=[],values:any[]=[])=>window.Module.ccall(name,result,args,values);
const pause=(v:boolean)=>{if(ready)call('prototype_web_pause',null,['number'],[v?1:0]);};
const touch=new GameTouch($('battle-viewport'),$<HTMLCanvasElement>('canvas'),{
 active:()=>ready&&playing&&!modal.open&&!busy&&!document.hidden,
 tap:p=>{call('prototype_web_tap','number',['number','number'],[p.x,p.y]);},
 inspect:p=>call('prototype_web_inspect','number',['number','number'],[p.x,p.y]),
 close:()=>call('prototype_web_close_details',null),
 ui:()=>JSON.parse(call('prototype_web_ui_state','string'))
});
const label=(zh:string,en:string)=>english?en:zh;
let syncText={zh:'尚未保存',en:'Not yet saved'};
function syncLabel(zh:string,en:string){syncText={zh,en};$('sync-status').textContent=label(zh,en);}
const battleNamesEN=['Yingchuan','Sishui Pass','Hulao Pass','Pursuit of Dong Zhuo'];
function battleHeading(){if(!ready||!playing)return;const p=JSON.parse(call('prototype_web_progress','string'));$('battle-title').textContent=english?`${battleNamesEN[p.scenario]} · Turn ${p.turn}`:battles[p.scenario]+' · 第 '+p.turn+' 回合';}
function applyLanguage(){
 if(ready)call('prototype_web_language',null,['number'],[english?1:0]);
 document.documentElement.lang=english?'en':'zh-CN';translateStatic(document,english?'en':'zh');renderHomeText();renderStatus();loadingLabel(loadingText.source,loadingText.values);
 for(const id of ['site-language','battle-language','modal-language']){const button=$(id);button.setAttribute('aria-pressed',String(english));button.setAttribute('aria-label',english?'Switch to Chinese':'切换为英文');}
 if(modal.open)modalRenderer?.();
 touch.setLanguage(english);$('canvas').lang=english?'en':'zh-CN';
 $('back-home').textContent=label('← 返回主页','← Home');
 $('save-game').textContent=label('保存 / 读取','Saves');
 $('fullscreen').textContent=label('全屏','Full');$('fullscreen').setAttribute('aria-label',label('全屏显示','Full screen'));
 $('zoom-reset').textContent=label('复位','Reset');
 $('zoom-in').setAttribute('aria-label',label('放大游戏画面','Zoom in'));$('zoom-out').setAttribute('aria-label',label('缩小游戏画面','Zoom out'));
 $('battle-help').textContent=label('移动后选择行动 · 右键 / Control 点击查看情报 · E 结束回合 · F6 存档','Move, then choose an action · Right-click / Control-click: unit info · E: end turn · F6: saves');
 $('canvas').setAttribute('aria-label',label('战场。轻点我军移动，长按武将查看情报，单指拖动，双指缩放。','Battlefield. Tap player units to move. Hold a unit for information. Drag to pan; pinch to zoom.'));
 $('ai-toggle').textContent='JEV '+label(aiOn?'开':'关',aiOn?'On':'Off');musicLabel();battleHeading();
 $('sync-status').textContent=label(syncText.zh,syncText.en);
}
for(const id of ['site-language','battle-language','modal-language'])$(id).onclick=()=>{english=!english;saveLanguage(localStorage,english?'en':'zh');applyLanguage();};
async function api(path:string,method='GET',value?:unknown){
 const response=await fetch('/api'+path,{method,credentials:'same-origin',cache:'no-store',headers:value===undefined?{}:{'Content-Type':'application/json'},body:value===undefined?undefined:JSON.stringify(value),signal:AbortSignal.timeout(path==='/jev'?22000:20000)});
 const data=await response.json().catch(()=>({error:'服务器返回了无效响应。'}));
 if(!response.ok){const e=Object.assign(Error(data.error||'操作失败。'),{status:response.status});if(response.status===401&&user){pause(true);audio.pause();status('登录已过期。请返回主页重新登录；未同步进度保留在本机。');}throw e;}return data;
}
async function action(work:()=>Promise<void>){if(busy)return;busy=true;try{await work();}catch(e){status((e as Error).message);}finally{busy=false;}}
async function refreshSaves(){saves=(await api('/saves')).saves;$('continue-game').hidden=!saves.some(s=>s.slot===0);renderHomeText();}
const dbPromise=new Promise<IDBDatabase>((resolve,reject)=>{const r=indexedDB.open('caocao-cloud-recovery',1);r.onupgradeneeded=()=>r.result.createObjectStore('drafts');r.onsuccess=()=>resolve(r.result);r.onerror=()=>reject(r.error);});dbPromise.catch(()=>{});
async function draft(write?:any){if(!user)return null;const db=await dbPromise;return new Promise<any>((resolve,reject)=>{const t=db.transaction('drafts',write===undefined?'readonly':'readwrite'),s=t.objectStore('drafts'),r=write===undefined?s.get(user!.id):s.put(write,user!.id);t.oncomplete=()=>resolve(r.result);t.onerror=()=>reject(t.error);});}
async function initialize(){
 try{const me=await api('/me');user=me.user;
  $('login-panel').hidden=!!user;$('username-form').hidden=!user||!!user.username;$('player-menu').hidden=!user?.username;$('sign-out').hidden=!user;
  $('account-name').textContent=user?.username??'';$('rename-user').hidden=!user?.username;
  if(user?.username){renderHomeText();await refreshSaves();const local=await draft().catch(()=>null);$('recover-local').hidden=!local?.unsynced;}
  renderHomeText();status(new URL(location.href).searchParams.get('login')==='failed'?'Google 登录未完成，请重试。':'');
 }catch(e){status((e as Error).message);}
}
$('google-login').onclick=()=>action(async()=>{status('正在前往 Google……');const result=await api('/auth/sign-in/social','POST',{provider:'google',callbackURL:location.origin+'/',errorCallbackURL:location.origin+'/?login=failed'});if(!result.url)throw Error('无法发起 Google 登录。');const target=new URL(result.url);if(target.hostname!=='accounts.google.com')throw Error('登录地址无效。');location.assign(target.href);});
$('username-form').onsubmit=e=>{e.preventDefault();void action(async()=>{await api('/profile','POST',{username:$<HTMLInputElement>('username').value});await initialize();});};
function renameUser(value=user?.username??'',error=''){
 showModal('修改用户名',`<form id="rename-form"><label for="rename-input">${t('排行榜显示的名字')}</label><input id="rename-input" value="${escape(value)}" minlength="2" maxlength="20" required autocomplete="nickname"><p id="rename-error" class="dialog-copy" role="alert">${escape(translatedMessage(error))}</p><div class="dialog-actions"><button type="submit">${t('保存名字')}</button></div></form>`);
 modalRenderer=()=>renameUser($<HTMLInputElement>('rename-input').value,error);
 $('rename-form').onsubmit=e=>{e.preventDefault();void action(async()=>{try{await api('/profile','PUT',{username:$<HTMLInputElement>('rename-input').value});closeModal();await initialize();}catch(e){error=(e as Error).message;$('rename-error').textContent=translatedMessage(error);}});};
}
$('rename-user').onclick=()=>renameUser();
$('sign-out').onclick=()=>action(async()=>{if(playing)await sync(0);await api('/auth/sign-out','POST',{});pause(true);audio.pause();user=null;saves=[];runId='';location.assign('/');});
function displayGame(){playing=true;document.body.classList.remove('engine-booting');document.body.classList.add('playing');$('home').hidden=true;$('game').hidden=false;$('loading').hidden=true;touch.start(true);window.dispatchEvent(new Event('resize'));pause(false);$<HTMLCanvasElement>('canvas').focus({preventScroll:true});updateMusic();status('');}
function displayHome(){playing=false;touch.stop();document.body.classList.remove('playing','engine-booting');$('home').hidden=false;$('game').hidden=true;$('loading').hidden=true;pause(true);audio.pause();}
const mobileText=matchMedia('(any-pointer: coarse), (max-width: 720px)');
const updateMobileText=()=>{if(ready)call('prototype_web_mobile_text',null,['number'],[mobileText.matches?1:0]);};
mobileText.addEventListener('change',updateMobileText);
window.prototypeReady=()=>{ready=true;updateMobileText();applyLanguage();engineResolve();};
async function loadEngine(){
 if(ready)return;if(enginePromise)return enginePromise;
 $('home').hidden=true;$('loading').hidden=false;
 enginePromise=new Promise<void>((resolve,reject)=>{engineResolve=resolve;engineReject=reject;});
 // Install a rejection handler before asynchronous asset fetching can fail.
 enginePromise.catch(()=>{});
 try{
  const manifest=await fetch('/engine.json',{cache:'no-cache'}).then(r=>{if(!r.ok)throw Error('无法读取游戏版本。');return r.json();});
  let loaded=0;const data=new Uint8Array(manifest.bytes);let offset=0;
  for(const chunk of manifest.chunks){const response=await fetch(chunk.url);if(!response.ok)throw Error('资源下载失败，请检查网络后刷新。');const bytes=new Uint8Array(await response.arrayBuffer());const digest=Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',bytes)),n=>n.toString(16).padStart(2,'0')).join('');if(bytes.length!==chunk.bytes||digest!==chunk.sha256)throw Error('资源校验失败，请刷新后重试。');data.set(bytes,offset);offset+=bytes.length;loaded+=bytes.length;$<HTMLProgressElement>('load-progress').value=loaded/manifest.bytes*90;loadingLabel('游戏资源 {loaded} / {total} MB',{loaded:Math.round(loaded/1048576),total:Math.round(manifest.bytes/1048576)});}
  // SDL reads the CSS box while creating its framebuffer. A display:none canvas
  // gives it a permanent 0×0 surface, so initialize at full size off screen.
  document.body.classList.add('engine-booting');$('game').hidden=false;
  window.Module={hostedGame:true,canvas:$('canvas'),getPreloadedPackage:()=>data.buffer,locateFile:(name:string)=>name.endsWith('.wasm')?manifest.wasm:manifest.script.replace('prototype.js',name),print:()=>{},printErr:(s:string)=>console.warn(s),setStatus:(s:string)=>{if(s)loadingLabel('正在初始化战场……');},onAbort:()=>engineReject(Error('游戏引擎加载失败，请刷新重试。'))};
  const script=document.createElement('script');script.src=manifest.script;script.onerror=()=>engineReject(Error('游戏程序下载失败。'));document.body.append(script);
  await enginePromise;$<HTMLProgressElement>('load-progress').value=100;
 }catch(e){engineReject(e as Error);displayHome();throw e;}
}
async function stable(){if(!ready)throw Error('战场尚未准备好。');const end=Date.now()+20000;while(call('prototype_busy')){if(Date.now()>end)throw Error('动画尚未结束，请稍后重试。');await new Promise(r=>setTimeout(r,80));}}
function getSnapshot(){const raw=call('prototype_snapshot','string');if(!raw)throw Error(call('prototype_error','string')||'此时无法保存。');return raw as string;}
async function sync(n:number){
 if(!ready||!runId)return;if(savePromise)await savePromise;await stable();
 const snapshot=getSnapshot(),hash=JSON.parse(snapshot).sha256,saveRun=runId;
 if(n===0&&hash===lastAutoHash)return;
 const version=saves.find(s=>s.slot===n)?.revision??0;
 savePromise=(async()=>{await draft({runId:saveRun,snapshot,unsynced:true,at:Date.now()}).catch(()=>{});
  syncLabel('正在保存到云端……','Saving to cloud...');
  try{const r=await api('/saves/'+n,'PUT',{runId:saveRun,snapshot,revision:version});
   const record:Save={slot:n,run_id:saveRun,scenario:r.scenario,turn:r.turn,phase:r.phase,cleared:r.cleared,revision:r.revision,saved_at:r.saved_at,state_hash:r.state_hash};saves=saves.filter(s=>s.slot!==n);saves.push(record);
   if(n===0)lastAutoHash=hash;await draft({runId:saveRun,snapshot,unsynced:false,at:Date.now()}).catch(()=>{});syncLabel('云端已保存 · '+new Date(r.saved_at).toLocaleTimeString('zh-CN',{hour12:false}),'Cloud saved · '+new Date(r.saved_at).toLocaleTimeString('en-US',{hour12:false}));return r;
  }catch(e){syncLabel('云端未同步 · 进度已保留在本机','Cloud not synced · Saved locally');throw e;}
 })();try{return await savePromise;}finally{savePromise=null;}
}
async function startNew(){
 await loadEngine();if(playing)await stable();pause(true);const r=await api('/runs','POST',{});
 if(!call('prototype_web_start','number',['number'],[r.seed]))throw Error(call('prototype_error','string'));
 runId=r.id;lastAutoHash='';lastReady=false;await refreshSaves();call('prototype_set_slot','number',['number'],[1]);displayGame();await sync(0);
}
async function resume(n:number){
 await loadEngine();await stable();pause(true);const s=await api('/saves/'+n);
 if(!call('prototype_restore','number',['string'],[s.snapshot])){pause(!playing);throw Error(call('prototype_error','string')||'无法读取此存档。');}
 runId=s.run_id;lastAutoHash=n===0?s.state_hash:'';lastReady=false;await refreshSaves();call('prototype_set_slot','number',['number'],[n||1]);closeModal(false);displayGame();syncLabel('已读取云端存档 · '+new Date(s.saved_at).toLocaleString('zh-CN',{hour12:false}),'Loaded cloud save · '+new Date(s.saved_at).toLocaleString('en-US'));await sync(0);
}
function confirm(title:string,message:string,work:()=>Promise<void>,values:Record<string,string|number>={}){showModal(t(title,values),`<p class="dialog-copy">${escape(t(message))}</p><div class="dialog-actions"><button id="confirm-cancel">${t('取消')}</button><button id="confirm-yes" class="primary">${t('确认')}</button></div>`);modalRenderer=()=>confirm(title,message,work,values);$('confirm-cancel').onclick=()=>closeModal();$('confirm-yes').onclick=()=>action(async()=>{closeModal(false);await work();});}
$('new-game').onclick=()=>{if(saves.length)confirm('开始新的四战征程','新游戏从颍川之战开始，将替换自动存档。已有手动存档保留，可随时读取。',startNew);else void action(startNew);};
$('continue-game').onclick=()=>action(()=>resume(0));
async function home(){if(playing){await stable();await sync(0);}displayHome();await refreshSaves();}
$('back-home').onclick=()=>action(home);window.prototypeHome=()=>{void action(home);};
window.prototypeRestart=()=>confirm('重新开始四战征程','已有手动存档保留。新的征程将从颍川之战开始。',startNew);
window.prototypeNextBattle=()=>{void action(async()=>{await stable();pause(true);try{await sync(0);if(call('prototype_scenario')===3){await home();return;}if(!call('prototype_web_next'))throw Error(call('prototype_error','string'));lastAutoHash='';lastReady=false;await sync(0);displayGame();}finally{pause(!playing||modal.open);}});};
function showModal(title:string,content:string){touch.cancel();pause(true);modalRenderer=null;$('modal-title').textContent=t(title);$('modal-content').innerHTML=content;if(!modal.open)modal.showModal();}
function closeModal(resumeGame=true){modal.close();modalRenderer=null;if(resumeGame&&playing){pause(false);$('canvas').focus();}}
$('close-modal').onclick=()=>closeModal();modal.addEventListener('cancel',()=>{if(playing)pause(false);});
async function saveList(){if(playing)await stable();pause(true);await refreshSaves();renderSaveList();}
function renderSaveList(){
 showModal('云端存档',`<p class="dialog-copy">${t('存档随 Google 账号保留。自动存档每 30 秒更新；十个手动存档可保存不同时间点。读取旧存档不会降低你的已记录战绩。')}</p><div class="save-grid">${Array.from({length:11},(_,n)=>{const s=saves.find(v=>v.slot===n);return `<article class="save-card ${n===0?'auto':''} ${s?'':'empty'}"><h3>${n===0?t('自动存档'):t('存档 {slot}',{slot:String(n).padStart(2,'0')})}</h3>${s?`<p>${escape(t(battles[s.scenario]))} · ${t('第 {turn} 回合',{turn:s.turn})} · ${escape(t(phases[s.phase]??s.phase))}</p><p>${t('已完成 {cleared} / 4 战',{cleared:s.cleared})} · ${escape(date(s.saved_at))}</p>`:`<p>${t('空存档位')}</p><p>${t('在战斗中可将此刻的进度保存在这里。')}</p>`}<div class="actions">${s?`<button data-resume="${n}">${t('读取存档')}</button>`:''}${playing&&n>0?`<button data-save="${n}">${t(s?'覆盖保存':'保存到这里')}</button>`:''}</div></article>`;}).join('')}</div>`);
 modalRenderer=renderSaveList;
 document.querySelectorAll<HTMLButtonElement>('[data-resume]').forEach(b=>b.onclick=()=>action(()=>resume(Number(b.dataset.resume))));
 document.querySelectorAll<HTMLButtonElement>('[data-save]').forEach(b=>b.onclick=()=>{const n=Number(b.dataset.save),write=async()=>{await sync(n);call('prototype_set_slot','number',['number'],[n]);await saveList();status('已保存到存档 {slot}。',{slot:n});};if(saves.some(s=>s.slot===n))confirm('覆盖存档 {slot}','确认用当前进度替换这个存档？其他存档不受影响。',write,{slot:n});else void action(write);});
}
$('view-saves').onclick=()=>action(saveList);$('save-game').onclick=()=>action(saveList);window.prototypeSlots=()=>{void action(saveList);};
window.prototypeSave=()=>{void action(async()=>{await sync(call('prototype_slot')||1);status('已保存到选择的云端存档位。');});};
window.prototypeLoad=()=>{void action(saveList);};
window.prototypeExport=()=>{void action(async()=>{await stable();const raw=getSnapshot(),a=document.createElement('a'),url=URL.createObjectURL(new Blob([raw],{type:'application/json'}));a.href=url;a.download='caocao-backup.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);});};
window.prototypeImport=()=>status('网页征程请从云端存档恢复。原生备份不计入本网站排行榜。');
async function ranks(kind='progress',page=0){const result=await api('/leaderboard?kind='+kind+'&page='+page);renderRanks(result,kind,page);}
function renderRanks(result:any,kind:string,page:number){const done=kind==='completed';showModal('英雄榜',`<div class="tabs" role="tablist"><button id="rank-progress" role="tab" aria-selected="${!done}">${t('征战中')}</button><button id="rank-completed" role="tab" aria-selected="${done}">${t('四战通关')}</button></div><p class="rank-note">${t(done?'按首次四战通关的云端记录时间排名，越早通关越靠前。':'先按已完成战役数，再按当前战场击败敌军的比例排名；相同时按记录时间先后。通关者进入四战通关榜。')}</p>${result.entries.length?`<table class="rank-table"><thead><tr><th>${t('排名')}</th><th>${t('用户名')}</th><th>${t(done?'通关时间':'征程')}</th></tr></thead><tbody>${result.entries.map((r:any)=>`<tr><td>${r.rank}</td><td>${escape(r.username)}</td><td>${done?`${escape(date(r.completed_at))}<small>${t('四战全部完成')}</small>`:`${t('{cleared} / 4 战完成',{cleared:r.cleared})}<small>${escape(t(battles[Math.min(r.cleared,3)]))} · ${t('击败 {percent}% 敌军',{percent:Math.floor(r.battle_progress/10)})}</small>`}</td></tr>`).join('')}</tbody></table>`:`<div class="empty-message">${t(done?'尚无四战通关记录。':'英雄榜尚待落笔。')}<br>${t(done?'等待第一位名将。':'开始新游戏，留下你的征程。')}</div>`}<div class="pager">${page?`<button id="rank-prev">${t('上一页')}</button>`:''}${result.hasMore?`<button id="rank-next">${t('下一页')}</button>`:''}</div>`);modalRenderer=()=>renderRanks(result,kind,page);$('rank-progress').onclick=()=>action(()=>ranks('progress'));$('rank-completed').onclick=()=>action(()=>ranks('completed'));if(page)$('rank-prev').onclick=()=>action(()=>ranks(kind,page-1));if(result.hasMore)$('rank-next').onclick=()=>action(()=>ranks(kind,page+1));}
$('view-ranks').onclick=()=>action(()=>ranks());
function updateMusic(){if(!playing||!musicOn){audio.pause();return;}const progress=JSON.parse(call('prototype_web_progress','string'));if(['victory','defeat'].includes(progress.phase)){audio.pause();return;}const src='/music/track-'+[5,2,3,4][progress.scenario]+'.mp3';if(!audio.src.endsWith(src)){audio.src=src;audio.load();}if(audio.paused)void audio.play().catch(()=>{$('music-toggle').textContent=label('♫ 点击播放','♫ Play music');});}
function musicLabel(){$('music-toggle').textContent=english?'♫ '+(musicOn?'On':'Off'):'♫ 音乐 '+(musicOn?'开':'关');$('music-toggle').setAttribute('aria-pressed',String(musicOn));}
$('music-toggle').onclick=()=>{musicOn=!musicOn;if(['♫ 点击播放','♫ Play music'].includes($('music-toggle').textContent??''))musicOn=true;localStorage.setItem('caocao-music',musicOn?'on':'off');musicLabel();updateMusic();$('canvas').focus();};musicLabel();
$('ai-toggle').onclick=()=>{aiOn=!aiOn;call('prototype_web_ai',null,['number'],[aiOn?1:0]);$('ai-toggle').textContent='JEV '+label(aiOn?'开':'关',aiOn?'On':'Off');$('ai-toggle').setAttribute('aria-pressed',String(aiOn));$('canvas').focus();};
window.prototypeEnemy=raw=>{const request=JSON.parse(raw),forRun=runId;void (async()=>{let result;try{result=await api('/jev','POST',{runId:forRun,request});}catch{result={key:request.key,choice:'a0',source:'fallback'};}if(runId!==forRun||!ready)return;call('prototype_web_enemy','number',['string'],[JSON.stringify(result)]);if(result.source==='fallback')syncLabel('JEV 暂不可用 · 使用本地判断','JEV unavailable · Local AI');})();};
$('fullscreen').onclick=()=>{void $('game').requestFullscreen?.().catch(()=>status('此浏览器不支持全屏。'));};
$('canvas').oncontextmenu=e=>e.preventDefault();
window.addEventListener('keydown',e=>{if(!playing||modal.open||!['F5','F6','F9'].includes(e.key))return;e.preventDefault();e.stopImmediatePropagation();if(e.key==='F5')window.prototypeSave();else window.prototypeSlots();},true);
setInterval(()=>{if(!ready||!playing||busy||modal.open)return;const p=JSON.parse(call('prototype_web_progress','string'));battleHeading();updateMusic();if(p.ready&&!lastReady&&!call('prototype_busy')){lastReady=true;void action(async()=>{await sync(0);status(p.scenario===3?'四战通关！你的完成时间已记录到英雄榜。':'本战胜利！完成整备后，点击下一战继续。');});}},1000);
setInterval(()=>{if(playing&&!busy&&!modal.open&&!savePromise&&!call('prototype_busy'))void sync(0).catch(e=>status(e.message));},30000);
document.addEventListener('visibilitychange',()=>{if(!ready||!playing)return;if(document.hidden){pause(true);audio.pause();if(!call('prototype_busy'))void sync(0).catch(()=>{});}else if(!modal.open){pause(false);updateMusic();}});
window.addEventListener('beforeunload',e=>{if(playing){e.preventDefault();e.returnValue='';}});
$('recover-local').onclick=()=>action(async()=>{const d=await draft();if(!d?.unsynced)throw Error('没有待恢复的本机进度。');await loadEngine();pause(true);if(!call('prototype_restore','number',['string'],[d.snapshot]))throw Error(call('prototype_error','string'));runId=d.runId;lastAutoHash='';await refreshSaves();displayGame();status('已恢复本机进度。请选一个手动存档位保存到云端。');});
function showAbout(){showModal('说明与隐私',`<div class="dialog-copy"><p>${t('这是原生 C++ 游戏的四战网页体验版。战斗在你的浏览器中运行，装备仅能在战后整备时更换。')}</p><p>${t('使用 Google 登录后，服务保存账号标识、邮箱、你选择的用户名和游戏存档。排行榜只公开用户名、战役进度及通关时间，不公开邮箱。退出登录后，其他账号无法读取你的云端存档。')}</p><p>${t('开启 JEV 时，游戏向服务端提交战场数据以选择敌军行动；JEV 不接收 Google 邮箱或账号凭据。服务不可用时使用本地判断。')}</p><p>${t('云端自动保存约每 30 秒一次，并在战役切换时保存。手动存档须点击保存。网络中断时，本机保留未同步进度；请在退出浏览器前确认云端保存成功。')}</p><p>${t('本版本不是完整原作，含四场战役及原型简化规则。美术与音乐版权属于原权利人。')}<a href="/FONT-LICENSE.txt">${t('字体许可')}</a> · <a href="/SDL-LICENSE.txt">${t('SDL 许可')}</a></p></div>`);modalRenderer=showAbout;}
$('about').onclick=showAbout;
applyLanguage();
void initialize();
