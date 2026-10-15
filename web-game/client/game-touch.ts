import {GameCamera,GameGestures,type Bounds,type Point,type View} from './game-gestures';

type EngineUI={details:boolean;bounds:Bounds;error:string};
type Bridge={active:()=>boolean;tap:(p:Point)=>void;inspect:(p:Point)=>number;close:()=>void;ui:()=>EngineUI};
export class GameTouch {
 readonly camera:GameCamera;
 private gestures:GameGestures;
 private details=false;
 private previous:View|null=null;
 private running=false;
 private frame=0;
 private captured=new Set<number>();
 private messageUntil=0;
 private english=false;
 setLanguage(english:boolean){this.english=english;this.cancel();this.clearMessage();document.getElementById("close-unit-info")!.textContent=english?"Close info":"关闭情报";}
 constructor(private viewport:HTMLElement,private canvas:HTMLCanvasElement,private bridge:Bridge,logical={width:960,height:1120}){
  this.camera=new GameCamera(logical.width,logical.height);
  const hold=document.getElementById('hold-progress')!,hint=document.getElementById('touch-hint')!;
  this.gestures=new GameGestures(this.camera,{
   tap:p=>{if(this.bridge.active()&&this.camera.contains(p)){this.bridge.tap(p);this.sync();}},
   hold:p=>{if(!this.bridge.active()||!this.camera.contains(p))return;
    const opened=this.bridge.inspect(p);this.sync();if(!opened)this.message(this.bridge.ui().error||(this.english?'Cannot inspect now.':'此时无法查看情报。'));else this.clearMessage();},
   progress:(p,fraction)=>{hold.hidden=!p;if(p){hold.style.left=p.x+'px';hold.style.top=p.y+'px';hold.style.setProperty('--hold-angle',fraction*360+'deg');}},
   changed:()=>this.paint()
  });
  const local=(e:PointerEvent):Point=>{const r=viewport.getBoundingClientRect();return {x:e.clientX-r.left,y:e.clientY-r.top};};
  const stop=(e:Event)=>{e.preventDefault();e.stopImmediatePropagation();};
  // Capture phase precedes SDL's target listeners. Never deliver a raw touch to
  // SDL: it would synthesize a mouse-down before a second finger/hold is known.
  for(const kind of ['pointerdown','pointermove','pointerup','pointercancel','pointerenter','pointerleave'] as const){
   viewport.addEventListener(kind,(e:PointerEvent)=>{
    if(e.pointerType!=='touch'&&e.pointerType!=='pen')return;
    stop(e);if(!this.bridge.active())return;
    if(kind==='pointerdown'){
     canvas.focus({preventScroll:true});this.captured.add(e.pointerId);viewport.setPointerCapture(e.pointerId);this.gestures.down(e.pointerId,local(e),performance.now());
    }else if(kind==='pointermove')this.gestures.move(e.pointerId,local(e));
    else if(kind==='pointerup'){
     this.gestures.up(e.pointerId,local(e),performance.now());this.captured.delete(e.pointerId);
     if(viewport.hasPointerCapture(e.pointerId))viewport.releasePointerCapture(e.pointerId);
    }else if(kind==='pointercancel')this.cancel();
   },{capture:true,passive:false});
  }
  viewport.addEventListener('lostpointercapture',e=>{if(this.captured.has(e.pointerId))this.cancel();});
  viewport.addEventListener('contextmenu',stop);
  viewport.addEventListener('click',e=>{if((e as PointerEvent).pointerType==='touch'||(e as MouseEvent & {sourceCapabilities?:{firesTouchEvents:boolean}}).sourceCapabilities?.firesTouchEvents)stop(e);},true);
  // Prevent Safari's text/callout and legacy touch-to-mouse defaults locally.
  viewport.addEventListener('touchstart',e=>e.preventDefault(),{passive:false});
  viewport.addEventListener('touchmove',e=>e.preventDefault(),{passive:false});
  const zoom=(factor:number)=>{this.cancel();this.camera.zoomAt(this.camera.zoom*factor,{x:this.camera.width/2,y:this.camera.height/2});this.paint();};
  document.getElementById('zoom-in')!.onclick=()=>zoom(1.25);
  document.getElementById('zoom-out')!.onclick=()=>zoom(.8);
  document.getElementById('zoom-reset')!.onclick=()=>{this.cancel();if(this.details)this.camera.fitBounds(this.bridge.ui().bounds);else this.camera.reset();this.paint();};
  document.getElementById('close-unit-info')!.onclick=()=>{this.cancel();this.bridge.close();this.sync();};
  new ResizeObserver(()=>this.resize()).observe(viewport);
  window.addEventListener('blur',()=>this.cancel());
  document.addEventListener('visibilitychange',()=>{if(document.hidden)this.cancel();});
  hint.textContent='长按武将查看情报 · 单指拖动 / 双指缩放';
 }
 start(reset=false){
  this.running=true;if(reset){this.cancel();this.details=false;this.previous=null;this.camera.reset();document.getElementById('close-unit-info')!.hidden=true;}
  this.clearMessage();this.resize();this.sync();cancelAnimationFrame(this.frame);this.loop();
 }
 stop(){this.running=false;cancelAnimationFrame(this.frame);this.cancel();}
 cancel(){
  this.gestures.cancel();const ids=[...this.captured];this.captured.clear();
  for(const id of ids)if(this.viewport.hasPointerCapture(id))this.viewport.releasePointerCapture(id);
 }
 private resize(){
  if(!this.running)return;this.cancel();const r=this.viewport.getBoundingClientRect();
  this.camera.resize(r.width,r.height);this.paint();
 }
 private paint(){
  const o=this.camera.offset;this.canvas.style.transform=`translate(${o.x}px,${o.y}px) scale(${this.camera.scale})`;
  const label=document.getElementById('zoom-level')!;label.textContent=Math.round(this.camera.zoom*100)+'%';
  (document.getElementById('zoom-out') as HTMLButtonElement).disabled=this.camera.zoom<=1;
  (document.getElementById('zoom-in') as HTMLButtonElement).disabled=this.camera.zoom>=4;
 }
 private sync(){
  const ui=this.bridge.ui();
  if(ui.details!==this.details){
   this.cancel();this.details=ui.details;
   if(ui.details){this.previous=this.camera.view;this.camera.fitBounds(ui.bounds);}
   else if(this.previous){this.camera.restore(this.previous);this.previous=null;}
   document.getElementById('close-unit-info')!.hidden=!ui.details;this.paint();
  }
 }
 private message(text:string){document.getElementById('touch-hint')!.textContent=text;this.messageUntil=performance.now()+2500;}
 private clearMessage(){document.getElementById('touch-hint')!.textContent=this.english?'Hold: info · Drag: pan · Pinch: zoom':'长按武将查看情报 · 单指拖动 / 双指缩放';this.messageUntil=0;}
 private loop=()=>{
  if(!this.running)return;
  if(this.bridge.active()){this.gestures.advance(performance.now());this.sync();}
  else this.cancel();
  if(this.messageUntil&&performance.now()>this.messageUntil)this.clearMessage();
  this.frame=requestAnimationFrame(this.loop);
 };
}
