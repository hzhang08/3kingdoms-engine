import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync,readdirSync} from 'node:fs';
import ts from 'typescript';
import {SITE_EN,siteText,readLanguage,saveLanguage,LANGUAGE_KEY,languageLocale} from '../client/site-language';
const read=(p:string)=>readFileSync(new URL(p,import.meta.url),'utf8');
const han=/\p{Script=Han}/u;
const tokens=(s:string)=>[...s.matchAll(/\{(\w+)\}/g)].map(v=>v[1]).sort();
test('every site translation keeps dynamic values and has English copy',()=>{
 for(const [zh,en] of Object.entries(SITE_EN)){
  assert.ok(en.trim(),zh);assert.ok(!han.test(en),zh);assert.deepEqual(tokens(zh),tokens(en),zh);
  assert.equal(siteText(zh,'zh'),zh);assert.equal(siteText(zh,'en'),en);
 }
 assert.equal(siteText('存档 {slot}','en',{slot:'09'}),'Save 09');
 assert.equal(siteText('已完成 {cleared} / 4 战','en',{cleared:3}),'3 / 4 battles completed');
 assert.equal(siteText('击败 {percent}% 敌军','en',{percent:75}),'75% of enemies defeated');
 assert.equal(siteText('Unrecognized diagnostic','en'),'Unrecognized diagnostic');
});
test('all explicitly marked static UI text and attributes have translations',()=>{
 const html=read('../client/index.html');
 for(const match of html.matchAll(/data-i18n(?:-[\w-]+)?="([^"]+)"/g))assert.ok(Object.hasOwn(SITE_EN,match[1]),match[1]);
});
test('all server Chinese errors have translations before the engine is loaded',()=>{
 for(const file of readdirSync(new URL('../src/',import.meta.url)).filter(f=>f.endsWith('.ts'))){
  const source=ts.createSourceFile(file,read('../src/'+file),ts.ScriptTarget.Latest,true);
  function walk(node:ts.Node){if(ts.isStringLiteral(node)&&han.test(node.text))assert.ok(Object.hasOwn(SITE_EN,node.text),file+': '+node.text);ts.forEachChild(node,walk);}walk(source);
 }
});
test('auth, loading, confirmations, dialogs and dynamic client text have catalog entries',()=>{
 const source=ts.createSourceFile('app.ts',read('../client/app.ts'),ts.ScriptTarget.Latest,true);
 const literal=(node:ts.Node)=>{if(ts.isStringLiteral(node)&&han.test(node.text))assert.ok(Object.hasOwn(SITE_EN,node.text),node.text);else if(ts.isConditionalExpression(node)){literal(node.whenTrue);literal(node.whenFalse);}};
 function walk(node:ts.Node){
  if(ts.isCallExpression(node)&&ts.isIdentifier(node.expression)){
   const fn=node.expression.text;
   if(['t','status','Error','showModal','loadingLabel'].includes(fn)&&node.arguments[0])literal(node.arguments[0]);
   if(fn==='confirm'){literal(node.arguments[0]);literal(node.arguments[1]);}
  }
  ts.forEachChild(node,walk);
 }walk(source);
});
test('existing battle preference is reused and invalid/unavailable storage falls back safely',()=>{
 let value:string|null='en';const storage={getItem:(key:string)=>{assert.equal(key,LANGUAGE_KEY);return value;},setItem:(key:string,v:string)=>{assert.equal(key,LANGUAGE_KEY);value=v;}};
 assert.equal(readLanguage(storage),'en');saveLanguage(storage,'zh');assert.equal(readLanguage(storage),'zh');
 value='invalid';assert.equal(readLanguage(storage),'zh');
 const denied={getItem:()=>{throw Error('denied');},setItem:()=>{throw Error('denied');}};
 assert.equal(readLanguage(denied),'zh');assert.doesNotThrow(()=>saveLanguage(denied,'en'));
 assert.equal(languageLocale('en'),'en-US');assert.equal(languageLocale('zh'),'zh-CN');
});
