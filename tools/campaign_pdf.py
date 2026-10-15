#!/usr/bin/env python3
"""Selectable vector campaign atlas PDF from the verified JSON analysis."""
from pathlib import Path
import sys,json,re,html
from reportlab.pdfgen import canvas
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4,A3,landscape
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import BaseDocTemplate,PageTemplate,Frame,Paragraph,Spacer,PageBreak,NextPageTemplate,Flowable,KeepTogether
ROOT=Path(__file__).resolve().parents[1];DATA=ROOT/'output/campaign-atlas';OUT=ROOT/'output/pdf/caocao-original-campaign-selectable.pdf'
pdfmetrics.registerFont(TTFont('CJK','/System/Library/Fonts/STHeiti Medium.ttc'))
pdfmetrics.registerFont(TTFont('SerifCJK',str(ROOT/'assets/generated/fonts/CaoCaoSong.ttf')))
B=json.loads((DATA/'battles.json').read_text());H=json.loads((ROOT/'assets/generated/data.json').read_text())['heroes']
INK='#172c42';TEAL='#087f83';GOLD='#986311';RED='#a24450';BLUE='#285fa0';MUTED='#52677a'
def clean(t):return str(t).replace('–','-').replace('—','-').replace('\u2011','-')
def para(t,size=10,color=INK,space=5):
 return Paragraph(html.escape(clean(t)).replace('\n','<br/>'),ParagraphStyle('p',fontName='CJK',fontSize=size,leading=size*1.55,textColor=colors.HexColor(color),wordWrap='CJK',spaceAfter=space,allowWidows=0,allowOrphans=0))
def nm(ids):return '、'.join(H[i]['name']+f'#{i}'for i in ids)or'无'
class Heading(Paragraph):
 def __init__(self,text,key,level=0):
  self.key,self.level,self.plain=key,level,text
  super().__init__(html.escape(text),ParagraphStyle('heading',fontName='CJK',fontSize=21 if level==0 else 14,leading=30 if level==0 else 21,textColor=colors.HexColor(TEAL),spaceAfter=12,keepWithNext=True))
class Doc(BaseDocTemplate):
 def afterFlowable(self,f):
  if isinstance(f,Heading):
   self.canv.bookmarkPage(f.key);self.canv.addOutlineEntry(f.plain,f.key,0,False);self.entries[f.key]=self.page

def footer(c,doc):
 w,h=c._pagesize;c.setFillColor(colors.HexColor(MUTED));c.setFont('CJK',8)
 c.drawString(36,22,'三国志曹操传 | 原版全战役与单位出场分析')
 c.drawRightString(w-36,22,str(doc.page))
class Overview(Flowable):
 def __init__(self):super().__init__();self.width=1100;self.height=724
 def draw(self):
  c=self.canv;W=self.width;HEIGHT=self.height
  def txt(x,y,t,n=10,col=INK):c.setFillColor(colors.HexColor(col));c.setFont('CJK',n);c.drawString(x,HEIGHT-y-n,clean(t))
  def arrow(pts,col=MUTED):
   c.setStrokeColor(colors.HexColor(col));c.setFillColor(colors.HexColor(col));c.setLineWidth(.65)
   p=c.beginPath();p.moveTo(pts[0][0],HEIGHT-pts[0][1])
   for x,y in pts[1:]:p.lineTo(x,HEIGHT-y)
   c.drawPath(p)
   import math
   x,y=pts[-1];xx,yy=pts[-2];a=math.atan2(y-yy,x-xx);p=c.beginPath();p.moveTo(x,HEIGHT-y)
   for a2 in (a-.6,a+.6):p.lineTo(x-4*math.cos(a2),HEIGHT-y+4*math.sin(a2))
   p.close();c.drawPath(p,fill=1,stroke=0)
  txt(0,0,'三国志曹操传 · 全战役路线图',25)
  txt(0,37,'58个战斗脚本：26共同必经 / 8条件脚本 / 12红金线必经 / 12蓝线必经',12,TEAL)
  txt(0,58,'必经指在相应路线继续推进；S编号从00起。每关完整单位与条件见后续正文，可用PDF书签跳转。',10,MUTED)
  xs=[0,280,560,840];w=242;step=30;top=126;bh=24;pos={}
  for j,t in enumerate(('共同主线 00-15','共同主线 16-31','红 / 金线 32-45','蓝线 46-57')):txt(xs[j],86,t,14,TEAL if j<2 else RED if j==2 else BLUE)
  for j,t in enumerate(('','A：接S15','B：R32路线值 ≥16','B：R32路线值 <16')):txt(xs[j],108,t,9,MUTED)
  def node(n,col,row,width=w,dx=0):
   x=xs[col]+dx;y=top+row*step;b=B[n];co=GOLD if b['route_class']=='条件战'else RED if col==2 else BLUE if col==3 else TEAL
   c.setStrokeColor(colors.HexColor(co));c.setFillColor(colors.HexColor('#f4f7fa'));c.roundRect(x,HEIGHT-y-bh,width,bh,4,fill=1,stroke=1)
   name=b['name'].replace('  ',' ')
   if n in(27,28):name='赤壁之战 '+('A'if n==27 else'B')
   txt(x+7,y+5,f'S{n:02} '+name,10 if width>150 else 8.6,co);pos[n]=(x,y,width)
  for n in range(16):node(n,0,n)
  for n in range(16,27):node(n,1,n-16)
  node(27,1,11,116);node(28,1,11,116,126)
  for n in range(29,32):node(n,1,n-17)
  for n in range(32,46):node(n,2,n-32)
  for n in range(46,58):node(n,3,n-46)
  def edge(a,b,col=MUTED):
   x,y,ww=pos[a];xx,yy,vv=pos[b];arrow([(x+ww/2,y+bh),(xx+vv/2,yy)],col)
  for n in range(15):edge(n,n+1)
  for n in range(16,26):edge(n,n+1)
  for a,b in((26,27),(26,28),(27,29),(28,29),(29,30),(30,31)):edge(a,b,GOLD if a<29 else MUTED)
  for n in range(32,45):edge(n,n+1)
  for n in range(46,57):edge(n,n+1)
  def bypass(a,b,off=9):
   x,y,ww=pos[a];xx,yy,vv=pos[b];r=max(x+ww,xx+vv)+off;arrow([(x+ww,y+12),(r,y+12),(r,yy+12),(xx+vv,yy+12)],GOLD)
  for a,b,of in((2,4,8),(6,9,16),(7,9,8),(28,30,8),(35,38,9)):bypass(a,b,of)
  x,y,ww=pos[27];xx,yy,vv=pos[30];arrow([(x,y+12),(x-8,y+12),(x-8,yy+12),(xx,yy+12)],GOLD)
  txt(10,610,'A → 接S16',10,TEAL);txt(290,583,'B → 右侧两条路线',10,GOLD)
  txt(570,551,'→ 红/金线结局（R58）',10,RED);txt(850,491,'→ 蓝线结局（R58）',10,BLUE)
  notes=[('董卓 / 濮阳','S03选择追击才打。S06全歼跳S09；否则S07。S07全歼跳S09；否则S08。'),('赤壁 / 突围','S24郭嘉获救走S27，否则S28。赤壁全歼跳S30；逃脱后走S29，再到S30。'),('汉水 / 斜谷','S35撤退置F735才走S36、S37；否则跳S38。夏侯渊阵亡F835不单独决定跳关。')]
  for i,(a,b)in enumerate(notes):txt(0,640+i*20,a+'：'+b,10,GOLD)
  txt(0,705,'原文件静态分析 + 部署函数核对；非所有路线实机通关。隐藏占位、未解旗标与前序过关影响均在正文注明。',9,MUTED)

OUT.parent.mkdir(parents=True,exist_ok=True)
poster=landscape(A3);doc=Doc(str(OUT),pagesize=A4,title='三国志曹操传：全战役路线与单位出场分析',author='Campaign analysis',leftMargin=42,rightMargin=42,topMargin=42,bottomMargin=40);doc.entries={}
doc.addPageTemplates([PageTemplate(id='poster',pagesize=poster,frames=[Frame(40,50,1110,740,leftPadding=0,rightPadding=0,topPadding=0,bottomPadding=0)],onPage=footer),PageTemplate(id='body',pagesize=A4,frames=[Frame(42,40,A4[0]-84,A4[1]-82,leftPadding=0,rightPadding=0,topPadding=0,bottomPadding=0)],onPage=footer)])
story=[Overview(),NextPageTemplate('body'),PageBreak(),Heading('阅读说明与证据范围','notes')]
# Preserve the introductory analysis and evidence boundaries, omitting PNG-only navigation.
report=(DATA/'REPORT.md').read_text();intro=report.split('## 逐战摘要')[0]
include=True
for line in intro.splitlines()[2:]:
 if line.startswith('## '):
  name=line[3:];include=name not in('图片','数据与复现','JEV审阅','研究参考')
  if include:story.append(para(name,14,TEAL,8))
 elif include and line.strip():
  line=re.sub(r'\*\*|`','',line);story.append(para(line,10))
story +=[PageBreak(),Heading('战役索引','index')]
for b in B:
 key=b['script'];label=f'{key}  {b["name"]} - {b["route_class"]}'
 story.append(Paragraph(f'<link href="#{key}" color="{TEAL}">{html.escape(label)}</link>',ParagraphStyle('index',fontName='CJK',fontSize=10,leading=17,spaceAfter=3)))
for b in B:
 story += [PageBreak(),Heading(b['script']+' '+b['name'],b['script']),para(b['route_class']+'：'+b['entry_condition'],11,GOLD),para('我军选择与资格',14,TEAL)]
 for p in b['player_selection']:
  story.append(para(f'上限{p["max"]}人；必选：{nm(p["forced"])}；禁选：{nm(p["excluded"])}。'))
  story.append(para('条件：'+p['condition']+'。来源：'+p['ref'],9,MUTED))
 story.append(para('候选池：'+nm(b['player_pool'])))
 for n in b['player_notes']:story.append(para(n,9,GOLD))
 story.append(para('固定友军 / 敌军部署表',14,TEAL))
 for side in('友军','敌军','敌援'):
  for hidden in(False,True):
   rs=[r for r in b['roster']if r['side']==side and r['hidden']==hidden]
   if rs:
    story.append(para(f'{side} / '+('初始隐藏'if hidden else'表中初始可见')+f' / {len(rs)}槽',11,GOLD if hidden else TEAL))
    # Every original entry retained; no rasterized table and no compression of IDs.
    story.append(para('；'.join(f'{r["name"]}#{r["id"]} [槽{r["slot"]}]'+(' → '+','.join(r['appearances'])if r['appearances']else'')for r in rs),10))
 story.append(para('条件 → 显现 / 复活',14,TEAL))
 if not b['appearance_groups']:story.append(para('无显式显现或复活指令。仍受出阵选择及开场处理影响。'))
 for g in b['appearance_groups']:
  story.append(KeepTogether([para(f'{g["key"]} '+['开场','战中','战后'][g['section']]+' / '+g['action'],11,TEAL),para('单位：'+'、'.join(g['units']),10)]))
  story.append(para('条件：'+g['condition'],10))
  story.append(para('原始指令：'+'、'.join(g['refs']),8.5,MUTED))
 story.append(para('我军出阵槽配置',14,TEAL))
 pg={}
 for p in b['placements']:
  k=(p['section'],p['hidden'],p['condition']);pg.setdefault(k,[]).append(f'{p["slot"]+1}@({p["xy"][0]},{p["xy"][1]})')
 for (section,hidden,cond),ls in pg.items():story.append(para(f'段{section} / '+('隐藏'if hidden else'可见')+'：'+', '.join(ls)+'。条件：'+cond,9))
 early=[r for r in b['hide_retreat_instructions']if r['section']==0]
 if early:
  story.append(para('开场二次处理',14,TEAL))
  for r in early:story.append(para(r['ref']+' '+('隐藏'if r['opcode']==49 else'撤退/死亡')+' 参数'+str(r['args'])+'；条件：'+r['condition'],9))
 story.append(para('口径：部署表不等于同时在场人数；初始隐藏不保证会增援；复活不新增人物。条件为脚本测试，仍需此前未过关且事件实际执行。F编号含义及赋值索引见随附分析数据。',8.5,MUTED))
doc.build(story)
(ROOT/'tmp/pdfs/page-index.json').write_text(json.dumps(doc.entries,ensure_ascii=False,indent=2))
print(OUT);print('pages',doc.page)
