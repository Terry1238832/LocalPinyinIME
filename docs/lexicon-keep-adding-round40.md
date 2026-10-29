# 词库续加第40轮（批次 460–465）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `c31b347bc82d35f11001856595583a24d01f140b`，引擎版本 1.6.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，本 PR 的总表从第二十九轮起累计。

- `local_core`：244018 → 245084，新增 **1066 条**。
- 权重 6000 / 5500 / 5000：0 / 1066 / 0 条。
- 最终 SHA-256：`52EEC37FE222ABD3BDA91B7AA4B3E165DB9AAF7F2B593CF4F2781CA3FDFF10F9`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮方向：日常聊天口语。本轮选择三个专题：饭菜口味与下厨搭手、来家做客与招呼去留、偏好取舍与观看讨论；每个专题一批短词和一批较长说法，共六批。均为自己逐条拟写的日常对话，不批量生成句尾、人物或数字变体；按所有层、domains及本轮前批去重，不从测试数据或评测失败案例选词。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 460 | 饭菜口味与下厨搭手聊天 | curate | 318 | 0 | 318 | 0 | 50.63% | 100.00% | `29a23ed6c158add3ca6ffcf54513fbd4431aa4f3` |
| 461 | 来家做客与招呼去留聊天 | curate | 330 | 0 | 330 | 0 | 76.97% | 100.00% | `cc43971920888cb0d5702e6603c7d293d187a34d` |
| 462 | 偏好取舍与观看讨论聊天 | curate | 358 | 0 | 358 | 0 | 76.26% | 100.00% | `3dc4d1480f66f8a1d4b91790cf179aac493b233a` |
| 463 | 饭菜口味与下厨搭手较长说法 | merge | 20 | 0 | 20 | 0 | 70.00% | 95.00% | `47a0a9fa7675f36b7c93b07ed3cd14ae6dfc09b5` |
| 464 | 来家做客与招呼去留较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `1a3d2505424a6b5eb5f235de68b71f22e72cbb55` |
| 465 | 偏好取舍与观看讨论较长说法 | merge | 20 | 0 | 20 | 0 | 80.00% | 100.00% | `ed76547e37c4c4175cc2a7bdd84ab20fad7d16bb` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round40_exclusions.tsv`，共 526 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `outside_batch_length_scope`：470 条。
- `duplicate:local_core_zh_pinyin.tsv`：23 条。
- `quality: clipped clause, incomplete condition or awkward chat wording in first draft`：10 条。
- `quality: awkward serving instruction, clipped clause or uncommon whole-chat wording`：8 条。
- `quality: forced reference, unnatural word order or overformal host reply`：6 条。
- `quality: incomplete modal wording, forced abstraction or unclear note-taking phrase`：6 条。
- `quality: prefer common first-sight wording over forced inversion`：1 条。
- `duplicate:batch460`：1 条。
- `quality: unclear omitted object in a whole-input phrase`：1 条。

按全部词库层、domains和本轮前批去重；排除不够自然的物件指代、残缺条件句、拗口倒装和长度范围不符的草稿。少量草稿重新拟为明确的汤碗装、小口小口喝、别把果核吞了、明早给你准信、我要做笔记了，原候选记录排除原因。三个专题各一短一长共六批，短词每批300–600条、长词每批10–30条。全部自拟，未看测试缺词或根据失败答案拟写候选。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

989 个含多音字的词在各批复核表逐字记录；30 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

逐词逐字复核：必须义的得取dei，记得、补语的得取de；盛饭及盛出来取cheng；剥蒜、剥皮、剥壳的口语动作取bao；吃东西的嚼劲取jiao；调味、调蘸料、调火力、调屏幕亮度均取tiao；削果皮取xiao；没收拾、玩具没收的没取mei，不混同没收财物的mo；忘带遗漏的落取la；烫着嘴、够着东西的着取zhao，对着风口及持续动作的着取zhe；铺垫太长的长取chang，楼栋长得像取zhang；配乐的乐取yue，更新的更取geng，调料的调取tiao。完整字位选择随词记录。轻声按本字，üe取lve/nve。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：46/46 通过。
- x86 Release 原生构建与 ctest：46/46 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。本轮无 2–4 字新词；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，245084 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round40_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round40.md`
- `resources/dictionary/expansion/batch460_authored_draft.txt`
- `resources/dictionary/expansion/batch460_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch461_authored_draft.txt`
- `resources/dictionary/expansion/batch461_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch462_authored_draft.txt`
- `resources/dictionary/expansion/batch462_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch463_draft.txt`
- `resources/dictionary/expansion/batch463_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch464_draft.txt`
- `resources/dictionary/expansion/batch464_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch465_draft.txt`
- `resources/dictionary/expansion/batch465_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_460.md`
- `resources/dictionary/expansion/batch_461.md`
- `resources/dictionary/expansion/batch_462.md`
- `resources/dictionary/expansion/batch_463.md`
- `resources/dictionary/expansion/batch_464.md`
- `resources/dictionary/expansion/batch_465.md`
- `resources/dictionary/expansion/lexicon_batch_460.applied.json`
- `resources/dictionary/expansion/lexicon_batch_460.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_460.json`
- `resources/dictionary/expansion/lexicon_batch_460.tsv`
- `resources/dictionary/expansion/lexicon_batch_460.validation.json`
- `resources/dictionary/expansion/lexicon_batch_461.applied.json`
- `resources/dictionary/expansion/lexicon_batch_461.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_461.json`
- `resources/dictionary/expansion/lexicon_batch_461.tsv`
- `resources/dictionary/expansion/lexicon_batch_461.validation.json`
- `resources/dictionary/expansion/lexicon_batch_462.applied.json`
- `resources/dictionary/expansion/lexicon_batch_462.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_462.json`
- `resources/dictionary/expansion/lexicon_batch_462.tsv`
- `resources/dictionary/expansion/lexicon_batch_462.validation.json`
- `resources/dictionary/expansion/lexicon_batch_463.json`
- `resources/dictionary/expansion/lexicon_batch_463.tsv`
- `resources/dictionary/expansion/lexicon_batch_464.json`
- `resources/dictionary/expansion/lexicon_batch_464.tsv`
- `resources/dictionary/expansion/lexicon_batch_465.json`
- `resources/dictionary/expansion/lexicon_batch_465.tsv`
- `resources/dictionary/expansion/probes/batch460_probe.tsv`
- `resources/dictionary/expansion/probes/batch461_probe.tsv`
- `resources/dictionary/expansion/probes/batch462_probe.tsv`
- `resources/dictionary/expansion/probes/batch463_probe.tsv`
- `resources/dictionary/expansion/probes/batch464_probe.tsv`
- `resources/dictionary/expansion/probes/batch465_probe.tsv`
- `resources/dictionary/expansion/round40_exclusions.tsv`
- `resources/dictionary/expansion/round40_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
