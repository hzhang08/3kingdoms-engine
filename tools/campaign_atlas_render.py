#!/usr/bin/env python3
"""Render evidence-linked original battle flow and complete unit roster PNGs."""
from pathlib import Path
import json,math,collections,re,hashlib,zipfile
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'output/campaign-atlas';B=json.loads((OUT/'battles.json').read_text());H=json.loads((ROOT/'assets/generated/data.json').read_text())['heroes']
FONT=ROOT/'assets/presentation-font/NotoSansCJKsc-Bold.otf'
BG='#0b1423';PANEL='#142239';INK='#edf3fa';MUTED='#b2c3d6';TEAL='#60d6c1';GOLD='#ffcc73';RED='#f899a4';BLUE='#83b9ff';LINE='#354a67'
FONTS={}
def font(n):
 if n not in FONTS:FONTS[n]=ImageFont.truetype(str(FONT),n)
 return FONTS[n]
def wrap(t,w,n):
 out=[]
 for para in str(t).split('\n'):
  cur=''
  for c in para:
   if cur and font(n).getlength(cur+c)>w:out.append(cur);cur=c
   else:cur+=c
  out.append(cur)
 return out

def names(ids):return '、'.join(H[i]['name']+f'#{i}'for i in ids)or'无'
def compact(us):
 # Preserve each original ID. Compress only consecutive numbered generic names
 # with consecutive IDs; distinct heroes and nonconsecutive IDs remain explicit.
 result=[];group=[]
 def flush():
  if not group:return
  if len(group)>=3:result.append(f'{group[0][0]}{group[0][1]}–{group[-1][1]}（#{group[0][2]}–{group[-1][2]}）')
  else:result.extend(f'{a}{n}#{i}'for a,n,i in group)
  group.clear()
 for u in us:
  m=re.fullmatch(r'(.+?)(\d+)#(\d+)',u)
  if m:
   r=(m[1],int(m[2]),int(m[3]))
   if group and not(r[0]==group[-1][0]and r[1]==group[-1][1]+1 and r[2]==group[-1][2]+1):flush()
   group.append(r)
  else:flush();result.append(u)
 flush();return '、'.join(result)or'无'

def card_rows(b):
 rows=[]
 def add(t,n=23,c=INK,p=7):rows.append((t,n,c,p))
 def head(t):add(t,27,TEAL,10)
 add(b['script']+'  '+b['name'],36,INK,14)
 color=GOLD if b['route_class']=='条件战'else RED if b['id']in range(32,46)else BLUE if b['id']>=46 else TEAL
 add(b['route_class']+'  |  '+b['entry_condition'],25,color,12)
 head('我军：已加入且未永久离队者，由玩家选择出阵')
 for p in b['player_selection']:
  add(f'上限{p["max"]}人；必选：{names(p["forced"])}；禁选：{names(p["excluded"])}。')
  if p['condition']!='无额外条件':add('适用：'+p['condition'],21,GOLD)
 add('候选池：'+names(b['player_pool']),22,INK)
 notes=[x for x in b['player_notes']if '未找到'not in x]
 if notes:add('资格条件：'+' '.join(notes),21,GOLD)
 if b['id']==0:add('曹操在第2回合事件才显现；不是从开场就能操作。',22,GOLD)
 pg=collections.OrderedDict()
 for p in b['placements']:
  if p['section']==0 and p['hidden']:pg.setdefault(p['condition'],[]).append(str(p['slot']+1))
 for cond,ls in pg.items():add('初始隐藏的我军出阵槽：'+','.join(ls)+'；'+cond+'。槽位只有选入单位才有效。',20,MUTED)
 head('固定部署表：逐个列出单位；# 为原始武将ID')
 for side in ('友军','敌军','敌援'):
  for hidden in (False,True):
   rs=[r for r in b['roster']if r['side']==side and r['hidden']==hidden]
   if rs:add(f'{side} · '+('初始隐藏'if hidden else'表中初始可见')+f'（{len(rs)}槽）：'+compact([r['name']+f'#{r["id"]}'for r in rs]),23,GOLD if hidden else INK)
 if b['id']==13:add('孙乾#40在友军表占两个槽；保留原始重复记录，不把它当作两个不同人物。',21,GOLD)
 hidden_unknown=[r['name']+f'#{r["id"]}'for r in b['roster']if r['hidden']and not r['appearances']]
 if hidden_unknown:add('？隐藏且无按ID显现/复活命令：'+compact(hidden_unknown)+'。可能为占位、未用数据或其他机制；不计为保证登场。',21,RED)
 head('条件 → 登场 / 再登场')
 if not b['appearance_groups']:add('无显式显现或复活指令；仍受出阵选择及开局隐藏/撤退处理影响。',22,MUTED)
 for g in b['appearance_groups']:
  phase=['开场','战中','战后'][g['section']]if g['section']<3 else f'段{g["section"]}'
  add(f'{g["key"]}  {phase} · {g["action"]}：'+compact(g['units']),22,TEAL if g['action']=='显现'else BLUE,3)
  add('    ← '+g['condition'],20,MUTED,3)
  add('    '+g['refs'][0]+(f' 等{len(g["refs"])}条指令'if len(g['refs'])>1 else''),17,'#8098b5',7)
 early=[r for r in b['hide_retreat_instructions']if r['section']==0]
 if early:
  head('开场二次处理（会改变表中可见性）')
  for r in early:
   a=r['args'];selector=f'#{a[1]}'if a[0]in(-1,0)else'范围/槽位'
   add(f'{r["ref"].split(":")[1]} '+('隐藏'if r['opcode']==49 else'撤退/死亡')+f' {selector} 参数{a} ← '+r['condition'],19,MUTED)
 add('口径：部署表 ≠ 同时在场人数。条件满足仍需事件实际执行；提前通关可阻止后续援军。复活次数不计作新单位。',20,GOLD,12)
 add('F编号为原始事件变量；全部赋值位置、动态撤退条件、出阵槽及原始字节偏移见配套JSON/Markdown。静态条件不是所有分支均已实机验证。',19,MUTED)
 return rows

def render_card(b,width=1160):
 pad=34;rows=card_rows(b);height=pad*2
 for t,n,c,p in rows:height+=len(wrap(t,width-pad*2,n))*(n+10)+p
 im=Image.new('RGB',(width,height),PANEL);d=ImageDraw.Draw(im);y=pad
 d.rectangle((0,0,width-1,7),fill=GOLD if b['route_class']=='条件战'else RED if 32<=b['id']<46 else BLUE if b['id']>=46 else TEAL)
 for t,n,c,p in rows:
  for line in wrap(t,width-pad*2,n):d.text((pad,y),line,font=font(n),fill=c);y+=n+10
  y+=p
 assert y<=height-pad+1
 return im

def flow():
 W,H=3440,3200;im=Image.new('RGB',(W,H),BG);d=ImageDraw.Draw(im)
 def tx(x,y,t,n=30,c=INK,w=None):
  for l in wrap(t,w or W-x-50,n):d.text((x,y),l,font=font(n),fill=c);y+=n+12
  return y
 def arrow(points,c=LINE,width=5):
  d.line(points,fill=c,width=width,joint='curve');x,y=points[-1];px,py=points[-2];a=math.atan2(y-py,x-px);sz=12
  d.polygon([(x,y),(x-sz*math.cos(a-.6),y-sz*math.sin(a-.6)),(x-sz*math.cos(a+.6),y-sz*math.sin(a+.6))],fill=c)
 tx(70,45,'三国志曹操传 · 原版全战役与出场条件',64)
 tx(74,132,'以本机 sgzccz 原始文件为准  |  117份剧情脚本  ·  58个战斗脚本  ·  2,378条友敌军部署记录',29,MUTED)
 tx(74,190,'26 共同必经     8 条件脚本     12 红/金线必经     12 蓝线必经',36,TEAL)
 tx(74,252,'必经 = 在该路线继续推进时要打；不包含失败重开。S编号从00起，重名战役以编号区分。',27,MUTED)
 xs=[75,925,1775,2625];cw=730;ystart=465;step=108;bh=80;positions={}
 for j,title in enumerate(['共同主线  00 → 15','共同主线  16 → 31','红 / 金线  32 → 45','蓝线  46 → 57']):
  tx(xs[j],355,title,37,TEAL if j<2 else RED if j==2 else BLUE,w=cw)
 def node(n,col,row,width=None,xoff=0):
  x=xs[col]+xoff;y=ystart+row*step;w=width or cw;b=B[n];c=GOLD if b['route_class']=='条件战'else RED if col==2 else BLUE if col==3 else TEAL
  d.rounded_rectangle((x,y,x+w,y+bh),radius=13,fill=PANEL,outline=c,width=3)
  name=b['name'].replace('  ',' ')
  if n==27:name='赤壁之战 A'
  if n==28:name='赤壁之战 B'
  tx(x+17,y+11,f'{n:02}  '+name,29 if w>400 else 25,c,w=w-30)
  if w>400:
   rs=b['roster'];init=sum(not r['hidden']for r in rs);hid=len(rs)-init
   tx(x+18,y+49,f'友敌表 {len(rs)}槽 · 初隐{hid}  | '+b['route_class'],17,MUTED,w=w-30)
  positions[n]=(x,y,w,bh)
 for n in range(16):node(n,0,n)
 for n in range(16,27):node(n,1,n-16)
 node(27,1,11,350);node(28,1,11,350,380);node(29,1,12);node(30,1,13);node(31,1,14)
 for n in range(32,46):node(n,2,n-32)
 for n in range(46,58):node(n,3,n-46)
 def edge(a,b,c=LINE):
  x,y,w,h=positions[a];xx,yy,ww,hh=positions[b]
  arrow([(x+w/2,y+h),(xx+ww/2,yy)],c)
 for n in range(15):edge(n,n+1)
 for n in range(16,26):edge(n,n+1)
 edge(26,27,GOLD);edge(26,28,GOLD);edge(27,29,GOLD);edge(28,29,GOLD);edge(29,30);edge(30,31)
 for n in range(32,45):edge(n,n+1)
 for n in range(46,57):edge(n,n+1)
 def bypass(a,b,offset=28,label='',color=GOLD):
  x,y,w,h=positions[a];xx,yy,ww,hh=positions[b];right=max(x+w,xx+ww)+offset
  arrow([(x+w,y+h/2),(right,y+h/2),(right,yy+hh/2),(xx+ww,yy+hh/2)],color,4)
  # labels below diagram avoid collisions with neighboring columns
 bypass(2,4,26);bypass(6,9,45);bypass(7,9,25)
 # Alternative Chibi paths both go straight to Hefei when annihilation succeeds.
 x,y,w,h=positions[27];x2,y2,w2,h2=positions[28];dest=positions[30]
 left=xs[1]-22;arrow([(x,y+h/2),(left,y+h/2),(left,dest[1]+bh/2),(dest[0],dest[1]+bh/2)],GOLD,4)
 bypass(28,30,26);bypass(35,38,30)
 tx(xs[0]+80,ystart+16*step+6,'A → 接右列 S16 白马之战',27,TEAL,w=cw)
 tx(xs[1]+60,ystart+15*step+25,'B → R32路线判断后进入右侧分支',25,GOLD,w=cw)
 tx(xs[2]+5,410,'B：原始路线值 ≥ 16',25,RED,w=cw)
 tx(xs[3]+5,410,'B：原始路线值 < 16',25,BLUE,w=cw)
 tx(xs[2]+15,ystart+14*step+10,'→ R58结局：红/金共用这组战役',24,RED,w=cw)
 tx(xs[3]+15,ystart+12*step+10,'→ R58蓝线结局',24,BLUE,w=cw)
 tx(xs[1]+5,410,'A：接左列 S15',25,TEAL,w=cw)
 y=2280;d.line((75,y-22,W-75,y-22),fill=LINE,width=3)
 rules=[('① 董卓追击 / 濮阳跳关','S03：选择自行追击才打。S06全歼 → S09；否则打S07。S07全歼 → S09；否则打S08再到S09。'),('② 两个赤壁与突围','S24保全郭嘉 → S27；否则S28，两者互斥。赤壁全歼 → S30；逃脱结算 → S29突围 → S30。'),('③ 红/金线的汉水、斜谷','S35撤退置F735=1 → S36 → S37 → S38；未置此标志 → 直接S38。夏侯渊是否阵亡不决定这次跳关。'),('④ 单位为什么不一定出现','我军取决于入队、存活、禁选与出阵选择；友敌表还含伏兵、援军及占位。登场常受回合、位置、选项和事件变量控制。')]
 for j,(title,body) in enumerate(rules):
  col=j%2;row=j//2;xx=75+col*1690;yy=y+row*270
  d.rounded_rectangle((xx,yy,xx+1600,yy+245),radius=20,fill=PANEL)
  tx(xx+26,yy+20,title,33,GOLD,w=1548);tx(xx+26,yy+78,body,28,INK,w=1548)
 tx(75,2880,'如何读完整名册',32,TEAL)
 tx(75,2934,'配套总图与58张分图列出每个单位ID、我军候选池、必选/禁选、初始隐藏、显现与复活条件。',28,INK)
 tx(75,2984,'单位表已通过原EXE部署函数逐条核对；事件条件为静态提取，尚未逐条实机重放。未解变量与占位不作保证。',26,GOLD)
 tx(75,3045,'证据：R_00–58.eex / S_00–57.eex / Imsg.e5 / Data.e5 / Ekd5.exe  ·  2026-10-10',24,MUTED)
 tx(75,3090,'原版文件与存档均未修改。网络攻略仅作线索；结论取自本机版本。',23,MUTED)
 return im

def main():
 directory=OUT/'battle-pngs';directory.mkdir(exist_ok=True)
 cards=[]
 for b in B:
  im=render_card(b);im.save(directory/(b['script']+'.png'));cards.append(im)
 overview=flow();overview.save(OUT/'campaign-flow.png')
 # Four reading columns; each battle card is retained at full pixel resolution.
 cols=4;gap=26;w=1160;heights=[360]*cols;places=[]
 # Find balanced contiguous ranges, keeping every column in scenario order.
 import itertools
 prefix=[0]
 for im in cards:prefix.append(prefix[-1]+im.height+gap)
 cuts=min(itertools.combinations(range(1,len(cards)),3),key=lambda c:max(prefix[b]-prefix[a]for a,b in zip((0,)+c,c+(len(cards),))))
 boundaries=(0,)+cuts+(len(cards),)
 for n,im in enumerate(cards):
  c=next(c for c in range(cols)if boundaries[c]<=n<boundaries[c+1]);places.append((c*(w+gap)+gap,heights[c]));heights[c]+=im.height+gap
 atlas=Image.new('RGB',(cols*w+(cols+1)*gap,max(heights)+50),BG);d=ImageDraw.Draw(atlas)
 d.text((35,30),'三国志曹操传 · 全58战役出场名册与条件',font=font(66),fill=INK)
 d.text((40,132),'按列从上向下读。全部友敌部署槽 + 我军候选池 + 428组显现/复活条件。',font=font(36),fill=TEAL)
 d.text((40,198),'# = 原始武将ID；初隐 ≠ 一定会增援；复活 ≠ 新增人数；F = 原始事件变量（详见配套报告）。',font=font(30),fill=GOLD)
 d.text((40,253),'判定条件来自原始脚本；2,378条部署记录已用原EXE函数核对。所有分支尚未逐条实机重放。',font=font(29),fill=MUTED)
 for im,pos in zip(cards,places):atlas.paste(im,pos)
 factor=min(1,math.sqrt(79000000/(atlas.width*atlas.height)))
 atlas=atlas.resize((round(atlas.width*factor),round(atlas.height*factor)),Image.Resampling.LANCZOS)
 atlas.save(OUT/'all-battles-units.png')
 # Overview index for browsing individual cards at practical size.
 (OUT/'render-metrics.json').write_text(json.dumps({'overview':overview.size,'atlas':atlas.size,'cards':[{'script':b['script'],'size':im.size}for b,im in zip(B,cards)]},indent=2))
 print('overview',overview.size,'atlas',atlas.size,'largest card',max(im.height for im in cards))
main()
