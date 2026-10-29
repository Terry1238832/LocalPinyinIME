# 词库续加第45轮（批次 490–495）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `e84e950c228d4ba9ce42b6763c19f8b341c5addd`，引擎版本 1.7.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第42轮起累计。

- `local_core`：249255 → 250329，新增 **1074 条**。
- 权重 6000 / 5500 / 5000：0 / 1074 / 0 条。
- 最终 SHA-256：`AB742100AFF0CF3FFDB2C6DE82B638918E20C1FD27C48517C76211B370F98050`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

按聊天口语继续，自拟下厨备菜火候、洗漱出门打理、读书学习笔记三个方向，每个方向一短一长共六批。重点是生活中会整段输入的提醒、问答、商量和反馈，不穷举人物数字或句尾；不按评测失败找词，不导入第三方词库。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 490 | 备菜下厨与火候提醒聊天 | curate | 323 | 0 | 323 | 0 | 52.94% | 100.00% | `f0ffb9b13ebdee5adafdf6af489ea8bae6be3c5f` |
| 491 | 洗漱整理与出门打理聊天 | curate | 357 | 0 | 357 | 0 | 61.62% | 100.00% | `6c574c1528b809ab8d89bd13b52717289e4a5f3e` |
| 492 | 看书学习与记笔记交流聊天 | curate | 334 | 0 | 334 | 0 | 66.47% | 100.00% | `4499fe2fd653930c2241d100bd109ed9da8218d8` |
| 493 | 备菜下厨与火候提醒较长说法 | merge | 20 | 0 | 20 | 0 | 70.00% | 100.00% | `42fef9ffe1d7fb2eaaba68ed77179bba9970e300` |
| 494 | 洗漱整理与出门打理较长说法 | merge | 20 | 0 | 20 | 0 | 95.00% | 100.00% | `5c949d97f8c2d3311517e70574c1b7414e355ea3` |
| 495 | 看书学习与记笔记交流较长说法 | merge | 20 | 0 | 20 | 0 | 65.00% | 100.00% | `35a2e14222bb19442a58084795d92733d786842a` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round45_exclusions.tsv`，共 355 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `outside_batch_length_scope`：307 条。
- `quality: clipped cooking object, unnatural word order, mistaken pouring direction or unclear boiling reference`：10 条。
- `duplicate:local_core_zh_pinyin.tsv`：23 条。
- `orthography: use 黏 for the cohesive texture of noodles or grouped food`：2 条。
- `quality: clipped cup reference, unclear water movement, pump-head reference or awkward fastening request`：5 条。
- `pronunciation: replace mechanical 卡 wording with clear 夹 wording`：1 条。
- `pronunciation: replace an uncertain hair measure word with an explicit hair reference`：1 条。
- `quality: clipped learning response, unclear column alignment or weaker whole-chat seat wording`：3 条。
- `quality: unclear order of restoring a missing word and reciting again`：1 条。
- `quality: unnatural kitchen ownership or towel-cleaning object in the longer wording`：2 条。

不收草稿里含混的倒油方向、残缺的椒和白芯指代、不自然的尝一口语序、泵头和水流动作指代，另拟完整说法。物件动作与读音不够清楚的卡头发、头发一撮改成夹头发、明确后面头发。学习笔记草稿纠正听到这懂了、竖式写齐、椅子没人等不自然或残缺说法。两条长句的厨房收拾顺序和擦地宾语改为自然表达；所有旧稿记录在排除表。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

963 个含多音字的词在各批复核表逐字记录；48 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

逐词逐字复核：择菜选zhai，剥蒜的口语选bao；盛饭、盛菜、盛汤选cheng，剩下选sheng。调碗汁、调火、调温、调亮及包带调短均tiao；须要义得选dei，补语得及记得、免得选de；别切成碎末了的了是句末le，不被末了mo liao套词带偏。薄厚与薄皮选bao；熟透和熟食选shu。扎头发选za，系鞋带选ji；先放着凉一下的着是持续zhe，不是着凉zhao。袖子、肩带和章节的长选chang。行指文字行均hang，行为可以取xing；琢磨日常思考选zuo mo。归还书本用huan，仍然还想看用hai；推到这里卡了是思路停住用ka。黏一团的食物质地用黏nian，粘锅、粘住按贴附动作取zhan；发卡为fa qia；轻声按本字，üe取lve/nve。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：46/46 通过。
- x86 Release 原生构建与 ctest：46/46 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。本轮无 2–4 字新词；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，250329 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round45_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 88.23% → 88.23% | +0.00 | 98.65% → 98.65% |
| 逐词输入·无前文 | 20626 | 79.64% → 79.64% | +0.00 | 97.53% → 97.53% |
| 留出整句 | 4126 | 64.44% → 64.44% | +0.00 | 77.70% → 77.70% |
| 口语对话·整句 | 3368 | 84.80% → 84.80% | +0.00 | 94.24% → 94.24% |
| 口语对话·逐词 | 19504 | 95.50% → 95.50% | +0.00 | 99.63% → 99.63% |
| 项目留出 | 138 | 84.78% → 84.78% | +0.00 | 89.13% → 89.13% |
| 新增用例 | 67 | 71.64% → 71.64% | +0.00 | 83.58% → 83.58% |
| 独立用例 | 62 | 53.23% → 53.23% | +0.00 | 79.03% → 79.03% |
| 日常口语·开发 | 149 | 97.32% → 97.32% | +0.00 | 99.33% → 99.33% |
| 日常口语·留出 | 150 | 90.00% → 90.00% | +0.00 | 96.00% → 96.00% |

## 改动文件

- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round45.md`
- `resources/dictionary/expansion/batch490_authored_draft.txt`
- `resources/dictionary/expansion/batch490_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch491_authored_draft.txt`
- `resources/dictionary/expansion/batch491_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch492_authored_draft.txt`
- `resources/dictionary/expansion/batch492_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch493_draft.txt`
- `resources/dictionary/expansion/batch493_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch494_draft.txt`
- `resources/dictionary/expansion/batch494_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch495_draft.txt`
- `resources/dictionary/expansion/batch495_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_490.md`
- `resources/dictionary/expansion/batch_491.md`
- `resources/dictionary/expansion/batch_492.md`
- `resources/dictionary/expansion/batch_493.md`
- `resources/dictionary/expansion/batch_494.md`
- `resources/dictionary/expansion/batch_495.md`
- `resources/dictionary/expansion/lexicon_batch_490.applied.json`
- `resources/dictionary/expansion/lexicon_batch_490.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_490.json`
- `resources/dictionary/expansion/lexicon_batch_490.tsv`
- `resources/dictionary/expansion/lexicon_batch_490.validation.json`
- `resources/dictionary/expansion/lexicon_batch_491.applied.json`
- `resources/dictionary/expansion/lexicon_batch_491.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_491.json`
- `resources/dictionary/expansion/lexicon_batch_491.tsv`
- `resources/dictionary/expansion/lexicon_batch_491.validation.json`
- `resources/dictionary/expansion/lexicon_batch_492.applied.json`
- `resources/dictionary/expansion/lexicon_batch_492.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_492.json`
- `resources/dictionary/expansion/lexicon_batch_492.tsv`
- `resources/dictionary/expansion/lexicon_batch_492.validation.json`
- `resources/dictionary/expansion/lexicon_batch_493.json`
- `resources/dictionary/expansion/lexicon_batch_493.tsv`
- `resources/dictionary/expansion/lexicon_batch_494.json`
- `resources/dictionary/expansion/lexicon_batch_494.tsv`
- `resources/dictionary/expansion/lexicon_batch_495.json`
- `resources/dictionary/expansion/lexicon_batch_495.tsv`
- `resources/dictionary/expansion/probes/batch490_probe.tsv`
- `resources/dictionary/expansion/probes/batch491_probe.tsv`
- `resources/dictionary/expansion/probes/batch492_probe.tsv`
- `resources/dictionary/expansion/probes/batch493_probe.tsv`
- `resources/dictionary/expansion/probes/batch494_probe.tsv`
- `resources/dictionary/expansion/probes/batch495_probe.tsv`
- `resources/dictionary/expansion/round45_exclusions.tsv`
- `resources/dictionary/expansion/round45_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
