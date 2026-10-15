import {test} from 'node:test';
import assert from 'node:assert/strict';
import {terminalResult,type SpecialSaved} from '../client/special-results';
test('only registered terminal matches enter the retry queue; imports and unfinished games do not',()=>{
 const saved:SpecialSaved={snapshot:'snapshot',at:1,turn:3,phase:'player',match:{id:'match'}};
 assert.equal(terminalResult(saved),undefined);
 assert.equal(terminalResult({...saved,phase:'victory',match:undefined}),undefined);
 assert.equal(terminalResult({...saved,phase:'defeat',match:{id:'match',initial:'pending registration'}}),undefined);
 for(const phase of ['victory','defeat'])assert.deepEqual(terminalResult({...saved,phase}),{id:'match',snapshot:'snapshot'});
});
