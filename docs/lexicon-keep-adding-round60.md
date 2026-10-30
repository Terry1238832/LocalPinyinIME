# 词库续加第60轮（批次 580–585）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `b4306a2c7fe1db877a96edfd39ec988668f31f88`，引擎版本 1.8.3。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第51轮起累计。

- `local_core`：265694 → 266765，新增 **1071 条**。
- 权重 6000 / 5500 / 5000：0 / 1071 / 0 条。
- 最终 SHA-256：`85A0023575D46BB5B5A6B7388F7935950DFA6489D93D9A8E72FB0D1A53A61C7D`。
- CMake 上限 281456，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮补三类常见群聊与社交用语：活动接龙和人数确认、介绍朋友互相认识、群内征求意见和投票。所有候选逐条自拟，不依据测试样例或评测失败选择。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 580 | 群聊活动报名接龙与人数确认 | curate | 370 | 0 | 370 | 0 | 76.22% | 100.00% | `a27de464c09f11fb57a1e4413037274c86cc30d1` |
| 581 | 初次见面介绍朋友与互加联系方式 | curate | 322 | 0 | 322 | 0 | 77.64% | 100.00% | `9a03d84dfec3d022a2128082fefb902e4cc31886` |
| 582 | 群聊征求意见与发起投票选择 | curate | 312 | 0 | 312 | 0 | 80.77% | 100.00% | `781aacf02b260c40470c15db9bb6ae74a4413c2c` |
| 583 | 群聊活动接龙、确认人数与临时调整长句 | merge | 24 | 0 | 24 | 0 | 66.67% | 83.33% | `6d7e599796380ff9950aa032ebfd617e9d87843b` |
| 584 | 初次见面介绍朋友、互相认识与留联系方式长句 | merge | 24 | 0 | 24 | 0 | 54.17% | 75.00% | `3bba2f2fabc51e23aa2fe541c930ede5d006295e` |
| 585 | 群聊征求意见、发起投票与公布结果长句 | merge | 19 | 0 | 19 | 0 | 57.89% | 84.21% | `9c5cd8090c09095858d8c4fddbd745f5c4306bc4` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round60_exclusions.tsv`，共 407 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：31 条。
- `不自然的数量表达`：1 条。
- `语义不清的省略表达`：2 条。
- `量词搭配不自然，应说位子或名额`：1 条。
- `不完整且不自然的表达`：1 条。
- `不自然的动宾搭配`：1 条。
- `重复动词造成的不自然表达`：1 条。
- `量词搭配不自然，应说名额或位子`：1 条。
- `outside_batch_length_scope`：318 条。
- `不自然的主谓搭配`：1 条。
- `依赖表单符号的说法，不是稳定的聊天用语`：2 条。
- `依赖数字口令且汉字表达不稳定`：7 条。
- `不自然的省略表达`：1 条。
- `表单字段式短语，不是常见聊天整词`：1 条。
- `语序不自然`：6 条。
- `不自然的候补表达`：1 条。
- `动宾搭配不自然`：5 条。
- `缺少宾语的不完整表达`：1 条。
- `日常聊天中不常用的书面表达`：1 条。
- `语序不自然且不完整`：1 条。
- `语义不自然`：3 条。
- `问法不自然`：1 条。
- `缺少动作对象的说明式短语`：1 条。
- `搭配不自然`：1 条。
- `缺少谓语成分的表达`：1 条。
- `缺少后续内容的不完整表达`：2 条。
- `时态助词使用不自然`：1 条。
- `语义不清的表达`：1 条。
- `不自然的套话拼接`：1 条。
- `缺少谓语的不完整表达`：1 条。
- `语义不完整`：1 条。
- `省略后语义不清`：1 条。
- `宾语指代不清`：1 条。
- `缺少宾语中心词的不完整表达`：1 条。
- `缺少介词导致语序不自然`：1 条。
- `缺少表达对象的不完整表达`：1 条。
- `缺少谓语或结果的不完整表达`：1 条。
- `语义不完整且表达不自然`：1 条。
- `缺少结果助词的不完整表达`：1 条。
- `缺少主语的不自然表达`：1 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1022 个含多音字的词在各批复核表逐字记录；5 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

本轮逐字语境决定和默认键差异均见各批 `pronunciation_review.tsv` 与批次 JSON。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：47/47 通过。
- x86 Release 原生构建与 ctest：47/47 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 10/10 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 281456` 通过，266765 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round60_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 88.25% → 88.25% | +0.00 | 98.70% → 98.70% |
| 逐词输入·无前文 | 20626 | 79.65% → 79.65% | +0.00 | 97.56% → 97.56% |
| 留出整句 | 4126 | 64.49% → 64.49% | +0.00 | 77.61% → 77.61% |
| 口语对话·整句 | 3368 | 85.36% → 85.36% | +0.00 | 94.69% → 94.69% |
| 口语对话·逐词 | 19504 | 95.58% → 95.58% | +0.00 | 99.72% → 99.72% |
| 项目留出 | 138 | 84.78% → 84.78% | +0.00 | 89.13% → 89.13% |
| 新增用例 | 67 | 71.64% → 71.64% | +0.00 | 83.58% → 83.58% |
| 独立用例 | 62 | 53.23% → 53.23% | +0.00 | 79.03% → 79.03% |
| 日常口语·开发 | 149 | 97.32% → 97.32% | +0.00 | 99.33% → 99.33% |
| 日常口语·留出 | 150 | 90.00% → 90.00% | +0.00 | 96.67% → 96.67% |

## 改动文件

- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round60.md`
- `resources/dictionary/expansion/batch580_authored_draft.txt`
- `resources/dictionary/expansion/batch580_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch581_authored_draft.txt`
- `resources/dictionary/expansion/batch581_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch582_authored_draft.txt`
- `resources/dictionary/expansion/batch582_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch583_draft.txt`
- `resources/dictionary/expansion/batch583_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch584_draft.txt`
- `resources/dictionary/expansion/batch584_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch585_draft.txt`
- `resources/dictionary/expansion/batch585_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_580.md`
- `resources/dictionary/expansion/batch_581.md`
- `resources/dictionary/expansion/batch_582.md`
- `resources/dictionary/expansion/batch_583.md`
- `resources/dictionary/expansion/batch_584.md`
- `resources/dictionary/expansion/batch_585.md`
- `resources/dictionary/expansion/lexicon_batch_580.applied.json`
- `resources/dictionary/expansion/lexicon_batch_580.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_580.json`
- `resources/dictionary/expansion/lexicon_batch_580.tsv`
- `resources/dictionary/expansion/lexicon_batch_580.validation.json`
- `resources/dictionary/expansion/lexicon_batch_581.applied.json`
- `resources/dictionary/expansion/lexicon_batch_581.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_581.json`
- `resources/dictionary/expansion/lexicon_batch_581.tsv`
- `resources/dictionary/expansion/lexicon_batch_581.validation.json`
- `resources/dictionary/expansion/lexicon_batch_582.applied.json`
- `resources/dictionary/expansion/lexicon_batch_582.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_582.json`
- `resources/dictionary/expansion/lexicon_batch_582.tsv`
- `resources/dictionary/expansion/lexicon_batch_582.validation.json`
- `resources/dictionary/expansion/lexicon_batch_583.json`
- `resources/dictionary/expansion/lexicon_batch_583.tsv`
- `resources/dictionary/expansion/lexicon_batch_584.json`
- `resources/dictionary/expansion/lexicon_batch_584.tsv`
- `resources/dictionary/expansion/lexicon_batch_585.json`
- `resources/dictionary/expansion/lexicon_batch_585.tsv`
- `resources/dictionary/expansion/probes/batch580_probe.tsv`
- `resources/dictionary/expansion/probes/batch581_probe.tsv`
- `resources/dictionary/expansion/probes/batch582_probe.tsv`
- `resources/dictionary/expansion/probes/batch583_probe.tsv`
- `resources/dictionary/expansion/probes/batch584_probe.tsv`
- `resources/dictionary/expansion/probes/batch585_probe.tsv`
- `resources/dictionary/expansion/round60_exclusions.tsv`
- `resources/dictionary/expansion/round60_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
