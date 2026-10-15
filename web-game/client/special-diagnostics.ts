// Bounded, local-only matchup history. Never include snapshots, accounts or tokens.
export const matchLogKey='caocao-special-match-diagnostics-v1';
type Store=Pick<Storage,'getItem'|'setItem'>;
export type Selection={hero:number;enemy:number;previous_phase:string;draw:number;ally?:number;ally2?:number;ally3?:number;ally_draw?:number};
type Entry={at:string;version:string;event:'selected'|'restored';hero:number;enemy:number;phase:string;draw?:number;ally?:number;ally2?:number;ally3?:number;ally_draw?:number};
function clean(value:unknown):Entry|undefined{
 if(!value||typeof value!=='object')return;
 const v=value as Entry;
 if(![0,6,1,11].includes(v.hero)||![119,127].includes(v.enemy)||
 !['selected','restored'].includes(v.event)||!['setup','player','ally','enemy','victory','defeat'].includes(v.phase)||
 typeof v.at!=='string'||!Number.isFinite(Date.parse(v.at))||typeof v.version!=='string'||v.version.length>120)return;
 if(v.event==='selected'&&(typeof v.draw!=='number'||!Number.isInteger(v.draw)||v.draw<0||v.draw>0xffffffff))return;
 const ally=v.ally===-1||[0,6,1,11].includes(v.ally??-2)?v.ally:undefined;
 const ally2=v.ally2===-1||([0,6,1,11].includes(v.ally2??-2)&&v.ally2!==v.hero&&v.ally2!==ally)?v.ally2:undefined;
 const ally3=v.ally3===-1||([0,6,1,11].includes(v.ally3??-2)&&![v.hero,ally,ally2].includes(v.ally3))?v.ally3:undefined;
 const ally_draw=typeof v.ally_draw==='number'&&Number.isInteger(v.ally_draw)&&v.ally_draw>=0&&v.ally_draw<=0xffffffff?v.ally_draw:undefined;
 return {at:v.at,version:v.version,event:v.event,hero:v.hero,enemy:v.enemy,phase:v.phase,...(v.event==='selected'?{draw:v.draw}:{}),...(ally!==undefined?{ally}:{}),...(ally2!==undefined?{ally2}:{}),...(ally3!==undefined?{ally3}:{}),...(ally_draw!==undefined?{ally_draw}:{})};
}
export class MatchDiagnostics{
 private entries:Entry[]=[];
 constructor(private readonly storage:Store,public version='loading',private readonly now=()=>new Date().toISOString()){
  try{const raw=JSON.parse(storage.getItem(matchLogKey)??'[]');if(Array.isArray(raw))this.entries=raw.map(clean).filter((e):e is Entry=>!!e).slice(-50);}catch{}
 }
 private append(event:Entry['event'],hero:number,enemy:number,phase:string,draw?:number,ally?:number,ally_draw?:number,ally2?:number,ally3?:number){
  const entry=clean({event,hero,enemy,phase,draw,ally,ally_draw,ally2,ally3,at:this.now(),version:this.version});if(!entry)return;
  this.entries.push(entry);this.entries=this.entries.slice(-50);
  try{this.storage.setItem(matchLogKey,JSON.stringify(this.entries));}catch{} // Quota/private mode cannot block play.
 }
 selected(selection:Selection){this.append('selected',selection.hero,selection.enemy,selection.previous_phase,selection.draw,selection.ally,selection.ally_draw,selection.ally2,selection.ally3);}
 restored(hero:number,enemy:number,phase:string,ally?:number,ally2?:number,ally3?:number){this.append('restored',hero,enemy,phase,undefined,ally,undefined,ally2,ally3);}
 export(){return JSON.stringify({schema:1,version:this.version,entries:this.entries},null,2);}
}
