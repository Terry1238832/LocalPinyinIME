# 词库续加第41轮（批次 466–471）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `04293e26240d7361cbb16b3bdbeda91d1f0b3df0`，引擎版本 1.6.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，本 PR 的总表从第二十九轮起累计。

- `local_core`：245084 → 246105，新增 **1021 条**。
- 权重 6000 / 5500 / 5000：0 / 1021 / 0 条。
- 最终 SHA-256：`23D9B7A2F258361E4408CE550AF7A62657F78808B717FE806740B5DC7A4A3319`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮方向：日常聊天口语。自拟宠物照看互动、亲子日常搭话、散步锻炼交流三个方向，每个方向一短一长共六批。只按生活常识挑选真实常见的提醒、问答和商量说法；全部层、domains及本轮前批按词面去重，不批量替换人物、数字或句尾，不从测试案例选词。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 466 | 宠物互动与喂养出门聊天 | curate | 306 | 0 | 306 | 0 | 42.48% | 100.00% | `0b92475a33829fe2e1e97ce2df97805dc6923730` |
| 467 | 孩子日常提醒与亲子搭话聊天 | curate | 327 | 0 | 327 | 0 | 74.01% | 100.00% | `914f4fc98d5683a8258338a19456cbc988911191` |
| 468 | 散步锻炼与慢跑交流聊天 | curate | 329 | 0 | 329 | 0 | 72.04% | 100.00% | `9df940011dca65f379f045795d41fa64dafbcd2a` |
| 469 | 宠物互动与喂养出门较长说法 | merge | 20 | 0 | 20 | 0 | 35.00% | 95.00% | `6b2af7ceb1e36897317505548a51121845685a75` |
| 470 | 孩子日常提醒与亲子搭话较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `1f11ab5916489f8fd7a6e63b6025628823b8a05b` |
| 471 | 散步锻炼与慢跑交流较长说法 | merge | 19 | 0 | 19 | 0 | 94.74% | 100.00% | `6d24f02c0c4da428faeca782df804238d18e0dc8` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round41_exclusions.tsv`，共 359 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `outside_batch_length_scope`：291 条。
- `quality: unnatural pet description, particle stacking or unclear self-help wording`：6 条。
- `quality: clipped pet movement, unclear object reference or weakly natural whole-chat wording`：8 条。
- `pronunciation: avoid the contextually ambiguous 着 wording by authoring a clear wire-biting reminder`：1 条。
- `duplicate:local_core_zh_pinyin.tsv`：18 条。
- `pronunciation: defer mechanical 卡 context rather than guess`：1 条。
- `quality: unclear physical referent, awkward question particle or weaker whole-chat word order`：8 条。
- `quality: clipped reference, unnatural child-directed wording or stacked particles`：4 条。
- `quality: forced exercise phrasing, clipped word or unclear motion description`：8 条。
- `quality: awkward stride description, unclear object or unsuitable exercise chat wording`：10 条。
- `quality: ambiguous roadside reference, unnatural distance wording or incomplete count predicate`：4 条。

按所有既有层、domains及本轮前批词面去重。排除不自然的物件指代、残缺动作谓语、用错的询问语气、含混的运动量说法和不够自然的倒装。三类专题每类一短一长，共六批。几条草稿改拟为小口小口吃、过了马路再聊、步子别太大、每天动一动、不用练太狠，以及完整的路边位置与散步距离表述，旧稿逐条写入排除表。猫砂、亲子和运动说法均逐条独立拟写，不穷举人物、数字或句尾，不从测试数据或失败案例取词。 线别让它咬着的着缺少充分语境，原稿不收，另拟明确的别让它咬电线。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

944 个含多音字的词在各批复核表逐字记录；20 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

逐词逐字读音复核：指甲和牵引绳长度、两边一样长取chang，队长取zhang；调风档、灯光取tiao；必须义的得取dei，补语及记得取de；系鞋带取ji；大家都会的都取dou；吃东西嚼完取jiao；扎头发、扎猫粮袋取za；睡着取zhao，黏着不放、坐着凉快的着取zhe，踩着、压着耳朵的着取zhao，分别按持续状态或结果判断，不被着凉一词带偏；别扭得太猛按扭动身体而非别扭脾气判断，完整字位记录；刨猫砂取pao，藏玩具取cang，遛弯和溜达取liu，睡觉的觉取jiao。轻声按本字，üe取lve/nve。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：46/46 通过。
- x86 Release 原生构建与 ctest：46/46 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。本轮无 2–4 字新词；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，246105 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round41_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round41.md`
- `resources/dictionary/expansion/batch466_authored_draft.txt`
- `resources/dictionary/expansion/batch466_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch467_authored_draft.txt`
- `resources/dictionary/expansion/batch467_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch468_authored_draft.txt`
- `resources/dictionary/expansion/batch468_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch469_draft.txt`
- `resources/dictionary/expansion/batch469_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch470_draft.txt`
- `resources/dictionary/expansion/batch470_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch471_draft.txt`
- `resources/dictionary/expansion/batch471_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_466.md`
- `resources/dictionary/expansion/batch_467.md`
- `resources/dictionary/expansion/batch_468.md`
- `resources/dictionary/expansion/batch_469.md`
- `resources/dictionary/expansion/batch_470.md`
- `resources/dictionary/expansion/batch_471.md`
- `resources/dictionary/expansion/lexicon_batch_466.applied.json`
- `resources/dictionary/expansion/lexicon_batch_466.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_466.json`
- `resources/dictionary/expansion/lexicon_batch_466.tsv`
- `resources/dictionary/expansion/lexicon_batch_466.validation.json`
- `resources/dictionary/expansion/lexicon_batch_467.applied.json`
- `resources/dictionary/expansion/lexicon_batch_467.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_467.json`
- `resources/dictionary/expansion/lexicon_batch_467.tsv`
- `resources/dictionary/expansion/lexicon_batch_467.validation.json`
- `resources/dictionary/expansion/lexicon_batch_468.applied.json`
- `resources/dictionary/expansion/lexicon_batch_468.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_468.json`
- `resources/dictionary/expansion/lexicon_batch_468.tsv`
- `resources/dictionary/expansion/lexicon_batch_468.validation.json`
- `resources/dictionary/expansion/lexicon_batch_469.json`
- `resources/dictionary/expansion/lexicon_batch_469.tsv`
- `resources/dictionary/expansion/lexicon_batch_470.json`
- `resources/dictionary/expansion/lexicon_batch_470.tsv`
- `resources/dictionary/expansion/lexicon_batch_471.json`
- `resources/dictionary/expansion/lexicon_batch_471.tsv`
- `resources/dictionary/expansion/probes/batch466_probe.tsv`
- `resources/dictionary/expansion/probes/batch467_probe.tsv`
- `resources/dictionary/expansion/probes/batch468_probe.tsv`
- `resources/dictionary/expansion/probes/batch469_probe.tsv`
- `resources/dictionary/expansion/probes/batch470_probe.tsv`
- `resources/dictionary/expansion/probes/batch471_probe.tsv`
- `resources/dictionary/expansion/round41_exclusions.tsv`
- `resources/dictionary/expansion/round41_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
