# 词库续加第43轮（批次 478–483）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `34e62e74f6dd7d2b1b0960acf734045979b14538`，引擎版本 1.7.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第42轮起累计。

开始本轮前，独立合并提交 `34e62e74f6dd7d2b1b0960acf734045979b14538` 已吸收 main 的 1.7.0 发布提交 `1193631bb00e5735a1a07916d8a090e6fe5ca6a4`，无冲突。版本及发布文件的变化来自 main，没有自行改版或打 tag、打包。加词前先按顺序完成 x64/x86 Release 构建及各46/46 ctest，之后才开始本轮入库；本轮已有词条逐行保留，词语搭配模型、神经重排模型及其测试源文件均未改动。

- `local_core`：247124 → 248233，新增 **1109 条**。
- 权重 6000 / 5500 / 5000：0 / 1109 / 0 条。
- 最终 SHA-256：`D1DAA286D3CC764877DCFF5926E5A860C0625465162EBE2A85517EFB08B697D0`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮方向：日常聊天口语。逐条自拟包裹收寄退换、饭店排队点单打包、家电小故障与上门预约三类日常服务沟通，每类一短一长共六批。只按生活常识选自然、现在会整段输入的请求与反馈，不穷举人物、金额和语气词变体；按所有层、domains及本轮前批去重，不从测试数据或失败用例找词。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 478 | 包裹收寄与退换沟通聊天 | curate | 345 | 0 | 345 | 0 | 66.38% | 100.00% | `047d3d23d8aeb556a51ea7ad1dba693e30b51941` |
| 479 | 饭店排队与点单打包聊天 | curate | 331 | 0 | 331 | 0 | 65.56% | 100.00% | `56a27a1803762097b0434d85bb69d3b447b1ea92` |
| 480 | 家电小故障与上门预约聊天 | curate | 373 | 0 | 373 | 0 | 64.88% | 100.00% | `15f4ef788f3a74e5e6ace762c553cc7e3d9cd883` |
| 481 | 包裹收寄与退换沟通较长说法 | merge | 20 | 0 | 20 | 0 | 90.00% | 100.00% | `b8650a3a9dee35480d9efa3b8608b460710f8a23` |
| 482 | 饭店排队与点单打包较长说法 | merge | 20 | 0 | 20 | 0 | 60.00% | 100.00% | `95aa809df9c97223010c74ee33a8735d051c200f` |
| 483 | 家电小故障与上门预约较长说法 | merge | 20 | 0 | 20 | 0 | 80.00% | 100.00% | `ef76ded20aa4ae70629bbe4448dc2924be2d86ee` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round43_exclusions.tsv`，共 479 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：33 条。
- `outside_batch_length_scope`：408 条。
- `quality: stacked particles, unclear packaging reference or awkward delivery reply`：4 条。
- `quality: unclear package contents, self-directed availability question or unnatural delivery phrasing`：6 条。
- `quality: overly generic reply without enough fixed or context-specific chat value`：7 条。
- `quality: imprecise serving action, awkward self-service phrase or clipped restroom request`：3 条。
- `quality: unclear appointment phrase, clipped appliance action or less natural diagnostic wording`：7 条。
- `pronunciation: defer the 卡 context rather than guess the contextual reading`：1 条。
- `quality: imprecise level description, unnatural hose state or clipped waiting reply`：3 条。
- `quality: clipped movement predicate, imprecise switch phrase or weakly natural reply`：4 条。
- `quality: imprecise parcel support, clipped request closure or weak waiting advice`：3 条。

按全部现有层、domains及本轮前批词面去重；剔除语气叠加、指代不清、过于泛化的答复、残缺谓语和不够自然的服务沟通写法。机械卡的候选暂不定音收录；自然但超过短词长度范围的草稿不硬截短。个别短稿重新拟成里面是易碎品、用手拧不动、线头露出来了、地面不平吧、管子被压扁了、你修吧我等着；长稿明确托住底部、关闭申请以及提前订位提醒。三个专题各一短一长共六批，每批均逐条自拟，没有第三方词库或测试数据取词，不穷举人物、数量和语气变体。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1022 个含多音字的词在各批复核表逐字记录；26 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

逐词逐字复核：重新放上秤的重放取chong，称重、重量的重取zhong；长度的长取chang；盛饭盛汤、盛一碗取cheng；默认把没过读为mo guo的优惠券和保修期语境均表示尚未过期，没取mei。得等、还得等、得固定、得换新等必须义取dei，胶带贴得挺乱等程度补语取de；还在派送、还没到等取hai，不混用归还的huan。调模式、调平机器取tiao；门弹开取tan；箱子摔着、碰着棱角的着取zhao，立着放、等着、拿着等状态取zhe。线头露出来为日常口语，露选择lou；滤网的滤写lv，拧螺丝取ning；轻声按本字，üe写lve/nve。所有字位选择随词记录，不依默认词组连读替代语义审定。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：46/46 通过。
- x86 Release 原生构建与 ctest：46/46 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。本轮无 2–4 字新词；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，248233 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round43_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round43.md`
- `resources/dictionary/expansion/batch478_authored_draft.txt`
- `resources/dictionary/expansion/batch478_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch479_authored_draft.txt`
- `resources/dictionary/expansion/batch479_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch480_authored_draft.txt`
- `resources/dictionary/expansion/batch480_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch481_draft.txt`
- `resources/dictionary/expansion/batch481_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch482_draft.txt`
- `resources/dictionary/expansion/batch482_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch483_draft.txt`
- `resources/dictionary/expansion/batch483_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_478.md`
- `resources/dictionary/expansion/batch_479.md`
- `resources/dictionary/expansion/batch_480.md`
- `resources/dictionary/expansion/batch_481.md`
- `resources/dictionary/expansion/batch_482.md`
- `resources/dictionary/expansion/batch_483.md`
- `resources/dictionary/expansion/lexicon_batch_478.applied.json`
- `resources/dictionary/expansion/lexicon_batch_478.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_478.json`
- `resources/dictionary/expansion/lexicon_batch_478.tsv`
- `resources/dictionary/expansion/lexicon_batch_478.validation.json`
- `resources/dictionary/expansion/lexicon_batch_479.applied.json`
- `resources/dictionary/expansion/lexicon_batch_479.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_479.json`
- `resources/dictionary/expansion/lexicon_batch_479.tsv`
- `resources/dictionary/expansion/lexicon_batch_479.validation.json`
- `resources/dictionary/expansion/lexicon_batch_480.applied.json`
- `resources/dictionary/expansion/lexicon_batch_480.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_480.json`
- `resources/dictionary/expansion/lexicon_batch_480.tsv`
- `resources/dictionary/expansion/lexicon_batch_480.validation.json`
- `resources/dictionary/expansion/lexicon_batch_481.json`
- `resources/dictionary/expansion/lexicon_batch_481.tsv`
- `resources/dictionary/expansion/lexicon_batch_482.json`
- `resources/dictionary/expansion/lexicon_batch_482.tsv`
- `resources/dictionary/expansion/lexicon_batch_483.json`
- `resources/dictionary/expansion/lexicon_batch_483.tsv`
- `resources/dictionary/expansion/probes/batch478_probe.tsv`
- `resources/dictionary/expansion/probes/batch479_probe.tsv`
- `resources/dictionary/expansion/probes/batch480_probe.tsv`
- `resources/dictionary/expansion/probes/batch481_probe.tsv`
- `resources/dictionary/expansion/probes/batch482_probe.tsv`
- `resources/dictionary/expansion/probes/batch483_probe.tsv`
- `resources/dictionary/expansion/round43_exclusions.tsv`
- `resources/dictionary/expansion/round43_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
