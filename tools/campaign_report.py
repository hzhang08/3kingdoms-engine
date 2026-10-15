#!/usr/bin/env python3
from pathlib import Path
import json,hashlib,collections,sys
ROOT=Path(__file__).resolve().parents[1];O=ROOT/'output/campaign-atlas';S=Path('/Users/henryzhang/Downloads/sgzccz')
b=json.loads((O/'battles.json').read_text());j=json.loads((O/'jumps.json').read_text());h=json.loads((ROOT/'assets/generated/data.json').read_text())['heroes']
def nm(ids):return '、'.join(h[i]['name']+f'#{i}'for i in ids)or'无'
sources=json.loads((O/'sources.json').read_text())
for n in ('Ekd5.exe','Imsg.e5','Data.e5'):sources['files'][n]=hashlib.sha256((S/n).read_bytes()).hexdigest()
for n,v in sources['files'].items():assert hashlib.sha256((S/n).read_bytes()).hexdigest()==v,n
from ls12 import decode
raw=next(x.data for x in decode((S/'Data.e5').read_bytes())if x.index==0)
for i,r in enumerate(h):assert r['name']==raw[i*32:i*32+12].split(b'\0')[0].decode('gbk')
sources['validation']={'script_count':117,'battle_count':58,'all_original_hashes_unchanged':True,'hero_name_records_compared_to_original_Data_e5':len(h),'battle_name_table':{'source':'Imsg.e5','offset':'0xc350','stride':200,'records':58},'original_table_rows_verified':2378,'route_threshold_cases':12}
(O/'sources.json').write_text(json.dumps(sources,ensure_ascii=False,indent=2)+'\n')
intro='''# 原版曹操传：全战役路线与单位出场分析

分析日期：2026-10-10。目标：`/Users/henryzhang/Downloads/sgzccz` 这份原版资源及可执行文件。

## 先看结论

不是所有战役都保证发生。原始资源包含 **58个战斗脚本**，分为 **26个共同必经脚本、8个条件脚本、12个红/金线必经脚本、12个蓝线必经脚本**。必经指在相应路线继续推进时必经，游戏失败不算继续推进。两个赤壁之战脚本S27/S28互斥；同名的后期战役也可能属于不同路线。

- 董卓追击战：虎牢关之后选择自行追击才打。
- 濮阳壹全歼可跳过贰、叁；壹撤离则打贰，贰全歼可跳过叁。
- 柳城让郭嘉撤至指定出口、完成获救事件，才进入赤壁A；否则进入赤壁B。判据是F724，不只是一句对话选项。
- 两个赤壁脚本都包含全歼后直接合淝、逃脱后进入赤壁突围的跳转。不能简单地把「郭嘉存活」与「绝不出现突围」画等号。
- 马超迎击战后，R32检查内部路线值：**<16进入蓝线，≥16进入红/金共用战役线**。这里是原始数值，不把它改写成未经验证的画面百分比。
- 红线定军山撤退置F735，才接汉水、斜谷；否则直接樊城。夏侯渊单挑阵亡F835影响人物是否还能出战，但不单独决定跳关。
- 红/金线在R58才再按85阈值决定两个结局；它们没有另外一套后期战斗列表。

## 图片

- [全战役路线图](campaign-flow.png)：58个战斗脚本、必经/条件分类及跳关路线。
- [全部单位与条件总图](all-battles-units.png)：逐战完整名册，分列按编号向下阅读；大图建议下载后放大。
- `battle-pngs/S_00.png` 至 `S_57.png`：58张原分辨率分图，便于逐关阅读。

## 单位不是一个固定人数

我军由累计入队、永久离队、出阵上限、必选/禁选和玩家选择共同决定。每关候选池是该关可能仍在队的集合，不代表同时或自动全部上场。曹操通常自动列入必选，但在博望坡、合淝、逍遥津、定军山等指定战役可被明确禁选。选定我军的第几个槽位不是固定武将ID。

友军/敌军部署表共有 **2,378条非空记录**。其中S13的孙乾#40在友军表重复占槽，因此这个数字是部署记录数，不是不同人物总数，也不是任何时刻同时在场人数。编号普通兵亦逐个保留原始ID；连续编号只在图上压缩显示。

初始隐藏单位往往是伏兵、援军或开场演出角色，但也有找不到按ID显现命令的占位记录。42条未找到按ID显现/复活命令的隐藏记录在图上带问号，不冒充保证登场的援军。表中初始可见角色也可能在开场演出中撤退或死亡。

全战役共有 **2,490条显现/复活指令**，按事件、词法测试条件及动作合并为 **428组**。这些不是额外的2,490个士兵；尤其后期复活会复用已有单位。图中的C编号只在当前战役内有效。

## 已验证什么

1. 117份EEX均逐字节通过结构解析：没有跳过未知操作码、事件边界或未解析尾部。
2. 58个战役名直接来自Imsg.e5偏移0xc350起、步长200字节的字符串表；512个武将名重新对照原Data.e5第0块。
3. 用这份原EXE的0x412230、0x4125c5、0x4129a4，在受限CPU模拟器内执行部署相关函数，逐条核对全部2,378条记录的ID、槽位、XY及隐藏位。模拟器没有启动Windows，没有写原游戏或存档。
4. 对原0x4118d4函数，用R32真实参数字节测试0、15、16、50、85、100的两个分支，共12项通过。
5. 原选择函数0x41856a静态显示：未禁选曹操时自动放入；强制列表中的0及哨兵被忽略；其他强制角色需仍为我军。保留了反汇编摘录。
6. 原文件哈希复核无变化。没有修改Electron、native/Web游戏或任何存档。

## 证据边界

**这是全战役静态分析加部署函数验证，不是所有路线全程实机通关。** 图中出现条件是脚本中的事件测试与嵌套测试（词法条件）。还需考虑此前状态修改、前序过关/跳转、事件扫描优先级、单位生命周期和提前结束战斗；不能把这些条件当作互相独立、全部可达的完整逻辑公式。

F编号是原始事件变量。能明确解释的关键路线/死亡变量已翻译；其余保留编号，`flag-setters.json`列出每一个赋值位置、值及所在条件。局部旗标需带所属脚本解释，不能跨战役按同一个编号合并。没有凭印象补写未知剧情条件。

13份脚本共22个文本字段含GBK中未分配的A140字节对。仅为可读显示将其画作全角空格，保留原始十六进制与偏移，未改源文件；这些字形映射没有逐一在原UI确认。文字容错没有改变结构解析或跳转/单位参数。

第20回合的红线定军山事件有前序结束分支：据点未失守，或剩余敌军少于10时先结算胜利；只有未被这些过关分支终止时才走后面的撤退处理并置F735。因此「撑到20回合」本身不是保证进入汉水的条件。

## 数据与复现

- `battles.json`：每关完整部署、我军候选池/限制、出阵槽、所有显现/复活与隐藏/撤退条件及偏移。
- `all-units.csv`：便于筛选的逐单位清单。
- `party-changes.json`：全部39条加入/离开指令及条件。
- `flag-setters.json`：旗标赋值索引。
- `jumps.json`、`jump-contexts.txt`、`listings/`、`scripts.json`：原脚本跳转、可读列表、完整结构化内容。
- `original-table-verification.json`、`route-threshold-verification.json`：原代码验证记录。
- `sources.json`、`text-anomalies.json`：输入哈希、范围与文字显示异常。
- `control-flow-disassembly.txt`：相关原程序函数摘录。

在现有caocao-engine环境中执行：

```sh
.venv/bin/python tools/campaign_atlas.py
.venv/bin/python tools/campaign_rosters.py
.venv/bin/python tools/campaign_verify.py
.venv/bin/python tools/campaign_atlas_render.py
.venv/bin/python tools/campaign_report.py
```

工具新增于项目tools目录；严格主解析器未修改。PNG绘制使用Pillow及项目自带Noto Sans CJK SC Bold。没有用生成式图片工具编造文字或图表。

## JEV审阅

本次调用JEV **1次**，3个问题：必经的表述范围、伏兵/复活计数方式、是否保留未解旗标。调用成功并记录于`jev-review.request.json`和`jev-review.record.json`。它仅评审分析方法，不作为战役规则的证据，也没有接触凭证正文或运行游戏。采纳限定性表述与保留原始ID/条件的建议；结论仍由原脚本、原代码核对决定。

## 研究参考

结构解码沿用项目已有EEX定义，参考[轩辕春秋原时代脚本研究](https://xycq.org.cn/forum/viewthread.php?authoruid=25461&tid=35258)及本地缓存的ccz-editor转换代码。外部攻略/实况仅用于寻找战役命名或路线线索；本报告的人物、战役名、条件与跳转均以所提供版本的文件为依据。

## 逐战摘要

|脚本|战役|是否必经|进入条件|
|---|---|---|---|
'''
for x in b:intro+=f'|{x["script"]}|{x["name"]}|{x["route_class"]}|{x["entry_condition"]}|\n'
intro+='\n## 逐战完整单位与出场条件\n\n'
for x in b:
 intro+=f'### {x["script"]} {x["name"]}\n\n[{x["script"]}高清PNG](battle-pngs/{x["script"]}.png)\n\n**{x["route_class"]}**：{x["entry_condition"]}\n\n'
 for p in x['player_selection']:intro+=f'- 我军上限{p["max"]}；必选{nm(p["forced"])}；禁选{nm(p["excluded"])}。条件：{p["condition"]}。证据：{p["ref"]}。\n'
 intro+=f'- 我军候选池：{nm(x["player_pool"])}。\n'
 for n in x['player_notes']:intro+='- '+n+'\n'
 intro+='\n|阵营|原始ID/名字|槽位|表中初始状态|显现/复活组|\n|---|---|---|---|---|\n'
 for r in x['roster']:intro+=f'|{r["side"]}|{r["id"]} {r["name"]}|{r["slot"]}|'+('隐藏'if r['hidden']else'可见')+f'|{", ".join(r["appearances"])or"无按ID命令"}|\n'
 intro+='\n'
 for g in x['appearance_groups']:
  intro+=f'- **{g["key"]} {g["action"]}**：'+ '、'.join(g['units'])+f'。测试：{g["condition"]}。证据：'+ '、'.join(g['refs'])+'。\n'
 intro+='\n'
(O/'REPORT.md').write_text(intro)
(O/'JEV-DISPOSITION.md').write_text('# JEV decision disposition\n\nOne request / three questions; successful. Methodology suggestions accepted after checking original sources. Model output is not treated as executable evidence. Unresolved flags and static reachability limits remain explicitly marked.\n')
print('Report',len(intro),'characters; 512 names and',len(sources['files']),'original file hashes checked')
