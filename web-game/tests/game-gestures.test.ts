import {test} from 'node:test';
import assert from 'node:assert/strict';
import {GameCamera,GameGestures,type Point} from '../client/game-gestures';

const near=(a:number,b:number)=>assert.ok(Math.abs(a-b)<1e-8,`${a} != ${b}`);
function setup(){
 const camera=new GameCamera();camera.resize(390,600);
 const taps:Point[]=[],holds:Point[]=[],progress:(Point|null)[]=[];
 const gestures=new GameGestures(camera,{tap:p=>taps.push(p),hold:p=>holds.push(p),progress:p=>progress.push(p),changed:()=>{}});
 return {camera,gestures,taps,holds,progress};
}
test('tap is delivered once, on release, in logical coordinates',()=>{
 const {camera,gestures:g,taps,holds}=setup(),p=camera.screen({x:312,y:544});
 g.down(1,p,0);g.advance(200);assert.equal(taps.length,0);
 g.up(1,p,220);g.up(1,p,230);assert.equal(taps.length,1);near(taps[0].x,312);near(taps[0].y,544);assert.equal(holds.length,0);
});
test('500ms hold tolerates jitter and consumes the release',()=>{
 const {gestures:g,taps,holds}=setup();g.down(1,{x:100,y:200},0);g.move(1,{x:106,y:204});
 g.advance(499);assert.equal(holds.length,0);g.advance(500);g.advance(1500);g.up(1,{x:100,y:200},1600);
 assert.equal(holds.length,1);assert.equal(taps.length,0);
});
test('release after a throttled animation frame still becomes a hold, not an attack',()=>{
 const {gestures:g,taps,holds}=setup();g.down(1,{x:100,y:200},0);g.up(1,{x:100,y:200},700);
 assert.equal(holds.length,1);assert.equal(taps.length,0);
});
test('one-finger drag beyond 10px cancels permanently even after returning',()=>{
 const {camera,gestures:g,taps,holds}=setup(),view=camera.view;
 g.down(1,{x:100,y:200},0);g.move(1,{x:111,y:200});g.move(1,{x:100,y:200});g.advance(700);g.up(1,{x:100,y:200},800);
 assert.deepEqual(camera.view,view);assert.deepEqual(taps,[]);assert.deepEqual(holds,[]);
});
test('one-finger drag pans at the current zoom without a tap or hold',()=>{
 const {camera,gestures:g,taps,holds}=setup();camera.zoomAt(3,{x:195,y:300});
 const anchor=camera.logical({x:150,y:260});g.down(1,{x:150,y:260},0);
 g.move(1,{x:156,y:264});near(camera.center.x,480);
 g.move(1,{x:210,y:310});near(camera.screen(anchor).x,210);near(camera.screen(anchor).y,310);
 g.advance(700);g.up(1,{x:210,y:310},800);assert.deepEqual(taps,[]);assert.deepEqual(holds,[]);near(camera.zoom,3);
});
test('pinch can continue as one-finger pan without a jump or command',()=>{
 const {camera,gestures:g,taps,holds}=setup();camera.zoomAt(2,{x:195,y:300});
 g.down(1,{x:140,y:300},0);g.down(2,{x:240,y:300},20);
 g.move(2,{x:260,y:300});const before=camera.view;g.up(2,{x:260,y:300},200);
 assert.deepEqual(camera.view,before);const anchor=camera.logical({x:140,y:300});
 g.move(1,{x:170,y:310});near(camera.screen(anchor).x,170);near(camera.screen(anchor).y,310);
 g.advance(900);g.up(1,{x:170,y:310},1000);assert.deepEqual(taps,[]);assert.deepEqual(holds,[]);
});
test('pinch zooms at its midpoint and neither remaining finger can become a tap',()=>{
 const {camera,gestures:g,taps,holds}=setup();
 g.down(1,{x:145,y:300},0);g.down(2,{x:245,y:300},100);
 g.move(1,{x:95,y:300});g.move(2,{x:295,y:300});near(camera.zoom,2);
 near(camera.logical({x:195,y:300}).x,480);
 g.up(2,{x:295,y:300},200);g.advance(1500);g.move(1,{x:115,y:300});g.up(1,{x:115,y:300},1600);
 assert.deepEqual(taps,[]);assert.deepEqual(holds,[]);
});
test('two-finger pan moves the camera and is bounded at all edges',()=>{
 const {camera,gestures:g}=setup();camera.zoomAt(3,{x:195,y:300});const before=camera.center.x;
 g.down(1,{x:100,y:200},0);g.down(2,{x:200,y:200},0);
 g.move(1,{x:140,y:200});g.move(2,{x:240,y:200});near(camera.zoom,3);assert.ok(camera.center.x<before);
 camera.transform(3,{x:-10000,y:-10000},{x:0,y:0});near(camera.offset.x,0);near(camera.offset.y,0);
 camera.transform(3,{x:10000,y:10000},{x:0,y:0});near(camera.offset.x+960*camera.scale,camera.width);near(camera.offset.y+1120*camera.scale,camera.height);
});
test('cancel, focus loss or modal transition prevents delayed actions',()=>{
 const {gestures:g,taps,holds,progress}=setup();g.down(1,{x:100,y:200},0);g.cancel();g.advance(900);g.up(1,{x:100,y:200},950);
 assert.deepEqual(taps,[]);assert.deepEqual(holds,[]);assert.equal(progress.at(-1),null);
});
test('third finger and lifting pinch fingers never leave an actionable touch',()=>{
 const {gestures:g,taps,holds}=setup();for(let i=1;i<=3;i++)g.down(i,{x:i*80,y:200},0);
 g.up(1,{x:80,y:200},100);g.move(3,{x:250,y:200});g.up(2,{x:160,y:200},200);g.up(3,{x:250,y:200},900);
 assert.deepEqual(taps,[]);assert.deepEqual(holds,[]);
});
test('transformed coordinates round trip for four battle grid sizes and rotations',()=>{
 const c=new GameCamera();
 for(const [w,h] of [[390,600],[844,230],[320,430],[1024,670]])for(const zoom of [1,1.25,2,4]){
  c.resize(w,h);c.zoomAt(zoom,{x:w/2,y:h/2});
  for(const [cols,rows] of [[20,20],[20,20],[20,20],[28,20]])for(const [x,y] of [[0,0],[cols-1,rows-1],[Math.floor(cols/2),Math.floor(rows/2)]]){
   const tile=48*Math.min(1,20/cols,20/rows),p={x:(x+.5)*tile,y:88+(y+.5)*tile},back=c.logical(c.screen(p));near(back.x,p.x);near(back.y,p.y);
  }
 }
});
test('zoom clamps 1–4, resize preserves zoom and legal center, reset fits whole game',()=>{
 const c=new GameCamera();c.resize(390,600);c.zoomAt(50,{x:195,y:300});near(c.zoom,4);const v=c.view;
 c.resize(844,230);near(c.zoom,4);near(c.center.x,v.center.x);near(c.center.y,v.center.y);
 c.reset();near(c.zoom,1);assert.ok(c.offset.x>=0&&c.offset.y>=0);c.zoomAt(-1,{x:0,y:0});near(c.zoom,1);
});
test('unit information fits its complete bounds and restores the prior camera',()=>{
 const c=new GameCamera();c.resize(390,600);c.zoomAt(3,{x:200,y:240});const prior=c.view;
 const r={x:104,y:206,w:752,h:710};c.fitBounds(r);
 const tl=c.screen(r),br=c.screen({x:r.x+r.w,y:r.y+r.h});assert.ok(tl.x>=0&&tl.y>=0&&br.x<=390&&br.y<=600);
 c.restore(prior);assert.deepEqual(c.view,prior);
});
test('special Hulao landscape camera preserves all projected tile centers',()=>{
 const c=new GameCamera(1536,1184);near(c.center.x,768);near(c.center.y,592);
 for(const [w,h] of [[390,600],[844,260],[1280,800]])for(const zoom of [1,2,4]){
  c.resize(w,h);c.zoomAt(zoom,{x:w/2,y:h/2});
  for(let y=0;y<20;y++)for(let x=0;x<20;x++){const p={x:80+72*(x+.5)-3*(y+.5),y:88+48*(y+.5)},round=c.logical(c.screen(p));near(round.x,p.x);near(round.y,p.y);}
 }
 c.reset();near(c.center.x,768);near(c.center.y,592);
});
