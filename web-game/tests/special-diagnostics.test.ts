import {test} from 'node:test';
import assert from 'node:assert/strict';
import {MatchDiagnostics,matchLogKey} from '../client/special-diagnostics';
const time=()=>new Date(0).toISOString();
function store(initial:string|null=null){let raw=initial;return {getItem:()=>raw,setItem:(key:string,value:string)=>{assert.equal(key,matchLogKey);raw=value;}};}
test('defeat selection records unsigned entropy, opponent and prior phase separately from restores',()=>{
 const storage=store(),log=new MatchDiagnostics(storage,'test',time);
 log.selected({hero:6,enemy:119,previous_phase:'defeat',draw:0xffffffff});
 log.selected({hero:6,enemy:127,previous_phase:'defeat',draw:0});
 log.restored(6,119,'defeat');
 const entries=JSON.parse(new MatchDiagnostics(storage,'test',time).export()).entries;
 assert.deepEqual(entries.map((e:any)=>[e.event,e.phase,e.draw,e.enemy]),[['selected','defeat',0xffffffff,119],['selected','defeat',0,127],['restored','defeat',undefined,119]]);
});
test('corrupt and unexpected data is excluded and log is capped at fifty events',()=>{
 const bad=new MatchDiagnostics(store('invalid'),'test',time);assert.equal(JSON.parse(bad.export()).entries.length,0);
 const storage=store(JSON.stringify([{event:'restored',hero:6,enemy:119,phase:'player',at:time(),version:'old',token:'SECRET',snapshot:'SAVE'}]));
 const log=new MatchDiagnostics(storage,'test',time);assert.ok(!log.export().includes('SECRET'));assert.ok(!log.export().includes('SAVE'));
 log.selected({hero:6,enemy:119,previous_phase:'defeat',draw:0}); // Keep mismatches: they are valuable diagnostic evidence.
 assert.equal(JSON.parse(log.export()).entries.length,2);
 for(let i=0;i<60;i++)log.selected({hero:0,enemy:i%2?119:127,previous_phase:'defeat',draw:i});
 const entries=JSON.parse(log.export()).entries;assert.equal(entries.length,50);assert.equal(entries[0].draw,10);
});
test('storage failure never interrupts selection diagnostics',()=>{
 const log=new MatchDiagnostics({getItem(){throw Error('denied');},setItem(){throw Error('quota');}},'test',time);
 assert.doesNotThrow(()=>log.selected({hero:6,enemy:127,previous_phase:'defeat',draw:2}));
 assert.equal(JSON.parse(log.export()).entries.length,1);
});
test('ally identity and its independent draw survive diagnostics export and reload',()=>{
 const storage=store(),log=new MatchDiagnostics(storage,'test',time);
 log.selected({hero:6,enemy:119,previous_phase:'defeat',draw:1,ally:11,ally_draw:2});log.restored(6,119,'player',11);
 const entries=JSON.parse(new MatchDiagnostics(storage,'test',time).export()).entries;
 assert.equal(entries[0].ally,11);assert.equal(entries[0].ally_draw,2);assert.equal(entries[1].ally,11);assert.equal(entries[1].ally_draw,undefined);
});
test('second random ally is retained in selections and restores without duplicate identities',()=>{
 const values=new Map<string,string>();const store={getItem:(key:string)=>values.get(key)??null,setItem:(key:string,value:string)=>{values.set(key,value);}};
 const log=new MatchDiagnostics(store,'trio');log.selected({hero:0,enemy:119,ally:1,ally2:11,ally_draw:0,draw:1,previous_phase:'defeat'});log.restored(0,119,'player',1,11);
 const entries=JSON.parse(new MatchDiagnostics(store).export()).entries;assert.equal(entries[0].ally2,11);assert.equal(entries[1].ally2,11);assert.equal(entries[0].ally_draw,0);
});
test('all three teammates survive local diagnostics, while duplicate third teammate is discarded',()=>{
 const storage=store(),log=new MatchDiagnostics(storage,'squads',time);
 log.selected({hero:0,enemy:119,ally:6,ally2:1,ally3:11,previous_phase:'defeat',draw:1});log.restored(0,119,'player',6,1,11);log.restored(0,119,'player',6,1,6);
 const entries=JSON.parse(new MatchDiagnostics(storage).export()).entries;assert.equal(entries[0].ally3,11);assert.equal(entries[1].ally3,11);assert.equal(entries[2].ally3,undefined);
});
