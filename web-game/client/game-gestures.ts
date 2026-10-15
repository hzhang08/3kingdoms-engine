// Presentation-only geometry and gesture arbitration. No battle state or storage.
export type Point={x:number;y:number};
export type Bounds=Point&{w:number;h:number};
export type View={zoom:number;center:Point};
const clamp=(n:number,lo:number,hi:number)=>Math.max(lo,Math.min(hi,n));
const distance=(a:Point,b:Point)=>Math.hypot(a.x-b.x,a.y-b.y);
const middle=(a:Point,b:Point):Point=>({x:(a.x+b.x)/2,y:(a.y+b.y)/2});

export class GameCamera {
 width=1;height=1;zoom=1;center:Point={x:480,y:560};
 constructor(readonly logicalWidth=960,readonly logicalHeight=1120){this.center={x:logicalWidth/2,y:logicalHeight/2};}
 get fit(){return Math.min(this.width/this.logicalWidth,this.height/this.logicalHeight);}
 get scale(){return this.fit*this.zoom;}
 get offset():Point{return {x:this.width/2-this.center.x*this.scale,y:this.height/2-this.center.y*this.scale};}
 get view():View{return {zoom:this.zoom,center:{...this.center}};}
 restore(view:View){this.zoom=clamp(view.zoom,1,4);this.center={...view.center};this.bound();}
 reset(){this.restore({zoom:1,center:{x:this.logicalWidth/2,y:this.logicalHeight/2}});}
 resize(width:number,height:number){this.width=Math.max(1,width);this.height=Math.max(1,height);this.bound();}
 logical(p:Point):Point{const o=this.offset;return {x:(p.x-o.x)/this.scale,y:(p.y-o.y)/this.scale};}
 screen(p:Point):Point{const o=this.offset;return {x:p.x*this.scale+o.x,y:p.y*this.scale+o.y};}
 contains(p:Point){return p.x>=0&&p.y>=0&&p.x<this.logicalWidth&&p.y<this.logicalHeight;}
 zoomAt(zoom:number,anchor:Point){this.transform(zoom,this.logical(anchor),anchor);}
 transform(zoom:number,logicalAnchor:Point,screenAnchor:Point){
  this.zoom=clamp(zoom,1,4);
  this.center={x:logicalAnchor.x-(screenAnchor.x-this.width/2)/this.scale,y:logicalAnchor.y-(screenAnchor.y-this.height/2)/this.scale};this.bound();
 }
 fitBounds(r:Bounds){this.zoom=clamp(Math.min((this.width-24)/r.w,(this.height-24)/r.h)/this.fit,1,4);this.center={x:r.x+r.w/2,y:r.y+r.h/2};this.bound();}
 private bound(){
  const halfX=this.width/(2*this.scale),halfY=this.height/(2*this.scale);
  this.center.x=halfX*2>=this.logicalWidth?this.logicalWidth/2:clamp(this.center.x,halfX,this.logicalWidth-halfX);
  this.center.y=halfY*2>=this.logicalHeight?this.logicalHeight/2:clamp(this.center.y,halfY,this.logicalHeight-halfY);
 }
}

type Finger={start:Point;point:Point;at:number};
type Callbacks={tap:(p:Point)=>void;hold:(p:Point)=>void;progress:(p:Point|null,fraction:number)=>void;changed:()=>void};
export class GameGestures {
 readonly fingers=new Map<number,Finger>();
 private mode:'idle'|'tap'|'pan'|'consumed'|'pinch'='idle';
 private panAnchor:Point|null=null;
 private pinch:{ids:number[];distance:number;zoom:number;anchor:Point}|null=null;
 constructor(readonly camera:GameCamera,private events:Callbacks){}
 down(id:number,p:Point,now:number){
  this.fingers.set(id,{start:{...p},point:{...p},at:now});
  if(this.fingers.size===1){this.mode='tap';this.panAnchor=this.camera.logical(p);this.events.progress(p,0);}
  else {this.mode='pinch';this.events.progress(null,0);this.beginPinch();}
 }
 move(id:number,p:Point){
  const f=this.fingers.get(id);if(!f)return;f.point={...p};
  if(this.mode==='tap'&&distance(p,f.start)>10){this.mode='pan';this.events.progress(null,0);}
  if(this.mode==='pan'&&this.panAnchor){this.camera.transform(this.camera.zoom,this.panAnchor,p);this.events.changed();}
  if(this.mode==='pinch'&&this.pinch){
   const a=this.fingers.get(this.pinch.ids[0])?.point,b=this.fingers.get(this.pinch.ids[1])?.point;
   if(a&&b){this.camera.transform(this.pinch.zoom*distance(a,b)/this.pinch.distance,this.pinch.anchor,middle(a,b));this.events.changed();}
  }
 }
 advance(now:number){
  if(this.mode!=='tap'||this.fingers.size!==1)return;
  const f=this.fingers.values().next().value!;const fraction=Math.min(1,(now-f.at)/500);
  if(fraction>=1){this.mode='consumed';this.events.progress(null,0);this.events.hold(this.camera.logical(f.start));}
  else this.events.progress(f.start,fraction);
 }
 up(id:number,p:Point,now:number){
  if(!this.fingers.has(id))return;
  this.move(id,p);this.advance(now);
  const tap=this.mode==='tap'&&this.fingers.size===1;
  this.fingers.delete(id);this.events.progress(null,0);
  if(tap)this.events.tap(this.camera.logical(p));
  if(this.fingers.size>=2){this.mode='pinch';this.beginPinch();}
  else {this.pinch=null;this.mode=this.fingers.size?'pan':'idle';
   this.panAnchor=this.fingers.size?this.camera.logical(this.fingers.values().next().value!.point):null;
  }
 }
 cancel(){this.fingers.clear();this.mode='idle';this.pinch=null;this.panAnchor=null;this.events.progress(null,0);}
 private beginPinch(){
  const ids=[...this.fingers.keys()].slice(0,2),a=this.fingers.get(ids[0])!.point,b=this.fingers.get(ids[1])!.point;
  this.pinch={ids,distance:Math.max(1,distance(a,b)),zoom:this.camera.zoom,anchor:this.camera.logical(middle(a,b))};
 }
}
