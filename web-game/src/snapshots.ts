import { fail } from './types';
export const BATTLES=['颍川之战','汜水关之战','虎牢关之战','董卓追击战'];
export function canonical(v:any):string {
 if(v===null||typeof v!=='object')return JSON.stringify(v);
 if(Array.isArray(v))return '['+v.map(canonical).join(',')+']';
 return '{'+Object.keys(v).sort().map(k=>JSON.stringify(k)+':'+canonical(v[k])).join(',')+'}';
}
export async function hash(raw:string){return Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',new TextEncoder().encode(raw))),n=>n.toString(16).padStart(2,'0')).join('');}
export function username(value:unknown){
 if(typeof value!=='string')fail(400,'请输入用户名。');const name=(value as string).normalize('NFKC').trim();
 if([...name].length<2||[...name].length>20||!/^[-_\p{L}\p{N}]+$/u.test(name))fail(400,'用户名需为 2–20 个汉字、字母、数字、下划线或短横线。');
 return {name,key:name.toLocaleLowerCase('en-US')};
}
export function inspectPayload(p:any){
 if(!p||p.schema!==10||p.engine!=='prototype-0.10'||p.ruleset!=='revised-battles-v1'||p.mode!=='opening-prototype')fail(422,'存档版本不兼容。');
 const scenario=p.scenario,state=p.state,progress=p.progress;
 if(!Number.isInteger(scenario)||scenario<0||scenario>3||!state||!Number.isInteger(state.turn)||state.turn<1||state.turn>20||!['player','enemy','ally','victory','defeat'].includes(state.phase))fail(422,'战场状态无效。');
 if(!progress||typeof progress.victory!=='boolean'||!Array.isArray(progress.completed)||!state.units||Object.keys(state.units).length>512)fail(422,'存档数据不完整。');
 const history=p.campaign?.battles??[];
 if(!Array.isArray(history)||history.length!==scenario||(scenario>0&&p.campaign.origin!==0))fail(422,'请从第一战开始，按顺序完成四战。');
 for(let n=0;n<history.length;n++)if(history[n].scenario!==n||history[n].progress?.victory!==true||history[n].progress?.active!==null||history[n].progress?.choice!==null)fail(422,'前一战的奖励尚未完成。');
 const won=state.phase==='victory'&&progress.victory===true&&progress.active===null&&progress.choice===null&&(!p.dialogue||p.dialogue.pending?.length===0);
 let total=0,defeated=0;
 for(const u of Object.values(state.units) as any[]){const b=u?.battle;if(!b||!Number.isInteger(b.hp)||b.hp<0||!Number.isInteger(b.side))fail(422,'部队数据无效。');if(b.side===2){total++;if(b.hp===0)defeated++;}}
 return {scenario,turn:state.turn as number,phase:state.phase as string,cleared:scenario+(won?1:0),battleProgress:won?0:total?Math.floor(defeated*1000/total):0,completed:scenario===3&&won};
}
export async function validateSnapshot(raw:unknown){
 if(typeof raw!=='string'||new TextEncoder().encode(raw).length>200000)fail(422,'存档为空或超过大小限制。');
 let e:any;try{e=JSON.parse(raw as string);}catch{fail(422,'存档无法解码。');}
 if(!e?.payload||typeof e.sha256!=='string'||e.sha256!==await hash(canonical(e.payload)))fail(422,'存档校验失败，原存档未改动。');
 const summary=inspectPayload(e.payload);return {raw:raw as string,hash:e.sha256 as string,payload:e.payload,summary};
}
