# 词库续加第69轮（批次 638–645）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `51faab2f432615f0662ca9cd0fea5056dde3f081`，引擎版本 1.8.3。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第51轮起累计。

- `local_core`：277106 → 278596，新增 **1490 条**。
- 权重 6000 / 5500 / 5000：0 / 1490 / 0 条。
- 最终 SHA-256：`3A932F3B8F90D15622C76E96ED9A5A525C7A073F831B985896F4277DC9846770`。
- CMake 上限 281456，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

按主题清单顺序选取尚未专门完成的四个主题，并对历史全部批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 638 | 日常聊天：家常共餐、照顾口味与饭后收尾 | curate | 360 | 0 | 360 | 0 | 76.67% | 100.00% | `3ac3999e928daaf8e41921c3057bb87961c7df76` |
| 639 | 日常聊天：追剧观影、交流剧情与即时观感 | curate | 327 | 0 | 327 | 0 | 73.39% | 100.00% | `0ce2b957737e567208875d151d02d3fd4a67a0e0` |
| 640 | 日常聊天：阅读分享、推荐书文与聊聊读后想法 | curate | 346 | 0 | 346 | 0 | 67.05% | 100.00% | `d4764da4af6b5ae4df43f491afe90cc150d72470` |
| 641 | 日常聊天：表达不同想法、解释缘由与商量折中 | curate | 409 | 0 | 409 | 0 | 82.64% | 100.00% | `8bddcb58ef1a5d83670f0746303cbd66da479863` |
| 642 | 日常聊天家常共餐与饭后收尾的长句 | merge | 16 | 0 | 16 | 0 | 87.50% | 87.50% | `2b0731412d24be2abf167a16505f855d87ab6df0` |
| 643 | 日常聊天追剧观影和交流观感的长句 | merge | 12 | 0 | 12 | 0 | 58.33% | 75.00% | `6d62ce78478d92c7116f33abbc6792272999b4e8` |
| 644 | 日常聊天阅读分享和交流读后想法的长句 | merge | 10 | 0 | 10 | 0 | 70.00% | 80.00% | `e205995b2b227c85296e39227b192e1be47806ce` |
| 645 | 日常聊天表达不同意见与商量折中的长句 | merge | 10 | 0 | 10 | 0 | 50.00% | 60.00% | `7ab81072396870eb4e1c90d2be2f38f96cae9d71` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round69_exclusions.tsv`，共 611 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：142 条。
- `outside_batch_length_scope`：457 条。
- `用字不当，改为洒汤表达`：1 条。
- `搭配不完整，改用耳熟表达`：1 条。
- `语序不自然`：1 条。
- `片段缺少上文信息`：1 条。
- `语义搭配不自然`：1 条。
- `duplicate:batch639`：1 条。
- `偏业务书面表达`：1 条。
- `偏正式的书面表达`：1 条。
- `属于较强的工作策略用语`：1 条。
- `搭配不自然`：1 条。
- `表达不自然`：1 条。
- `表达不完整`：1 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1385 个含多音字的词在各批复核表逐字记录；32 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

本轮逐字语境决定和默认键差异均见各批 `pronunciation_review.tsv` 与批次 JSON。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：47/47 通过。
- x86 Release 原生构建与 ctest：47/47 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 45/45 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 281456` 通过，278596 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round69_validation.json`；短词批次另存工具生成的 `.validation.json`。
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

- `docs/lexicon-keep-adding-round69.md`
- `resources/dictionary/expansion/batch638_authored_draft.txt`
- `resources/dictionary/expansion/batch638_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch639_authored_draft.txt`
- `resources/dictionary/expansion/batch639_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch640_authored_draft.txt`
- `resources/dictionary/expansion/batch640_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch641_authored_draft.txt`
- `resources/dictionary/expansion/batch641_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch642_draft.txt`
- `resources/dictionary/expansion/batch642_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch643_draft.txt`
- `resources/dictionary/expansion/batch643_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch644_draft.txt`
- `resources/dictionary/expansion/batch644_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch645_draft.txt`
- `resources/dictionary/expansion/batch645_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_638.md`
- `resources/dictionary/expansion/batch_639.md`
- `resources/dictionary/expansion/batch_640.md`
- `resources/dictionary/expansion/batch_641.md`
- `resources/dictionary/expansion/batch_642.md`
- `resources/dictionary/expansion/batch_643.md`
- `resources/dictionary/expansion/batch_644.md`
- `resources/dictionary/expansion/batch_645.md`
- `resources/dictionary/expansion/lexicon_batch_638.applied.json`
- `resources/dictionary/expansion/lexicon_batch_638.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_638.json`
- `resources/dictionary/expansion/lexicon_batch_638.tsv`
- `resources/dictionary/expansion/lexicon_batch_638.validation.json`
- `resources/dictionary/expansion/lexicon_batch_639.applied.json`
- `resources/dictionary/expansion/lexicon_batch_639.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_639.json`
- `resources/dictionary/expansion/lexicon_batch_639.tsv`
- `resources/dictionary/expansion/lexicon_batch_639.validation.json`
- `resources/dictionary/expansion/lexicon_batch_640.applied.json`
- `resources/dictionary/expansion/lexicon_batch_640.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_640.json`
- `resources/dictionary/expansion/lexicon_batch_640.tsv`
- `resources/dictionary/expansion/lexicon_batch_640.validation.json`
- `resources/dictionary/expansion/lexicon_batch_641.applied.json`
- `resources/dictionary/expansion/lexicon_batch_641.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_641.json`
- `resources/dictionary/expansion/lexicon_batch_641.tsv`
- `resources/dictionary/expansion/lexicon_batch_641.validation.json`
- `resources/dictionary/expansion/lexicon_batch_642.json`
- `resources/dictionary/expansion/lexicon_batch_642.tsv`
- `resources/dictionary/expansion/lexicon_batch_643.json`
- `resources/dictionary/expansion/lexicon_batch_643.tsv`
- `resources/dictionary/expansion/lexicon_batch_644.json`
- `resources/dictionary/expansion/lexicon_batch_644.tsv`
- `resources/dictionary/expansion/lexicon_batch_645.json`
- `resources/dictionary/expansion/lexicon_batch_645.tsv`
- `resources/dictionary/expansion/probes/batch638_probe.tsv`
- `resources/dictionary/expansion/probes/batch639_probe.tsv`
- `resources/dictionary/expansion/probes/batch640_probe.tsv`
- `resources/dictionary/expansion/probes/batch641_probe.tsv`
- `resources/dictionary/expansion/probes/batch642_probe.tsv`
- `resources/dictionary/expansion/probes/batch643_probe.tsv`
- `resources/dictionary/expansion/probes/batch644_probe.tsv`
- `resources/dictionary/expansion/probes/batch645_probe.tsv`
- `resources/dictionary/expansion/round69_exclusions.tsv`
- `resources/dictionary/expansion/round69_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
