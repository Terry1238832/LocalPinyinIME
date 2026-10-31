# 词库续加第68轮（批次 630–637）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `e799d52cae4ec3462f94a72213f3800703ab79fc`，引擎版本 1.8.3。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第51轮起累计。

- `local_core`：275752 → 277106，新增 **1354 条**。
- 权重 6000 / 5500 / 5000：0 / 1354 / 0 条。
- 最终 SHA-256：`D574F9B140DE611174CFDB95D4AB19B7E0A06623A5196207BFD2784D2FD302C5`。
- CMake 上限 281456，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮补充转达消息、红包往来、倾听陪伴和散步邀约等日常聊天用语，逐条自拟，不根据测试缺词选词。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 630 | 日常聊天：转达消息、留言与确认回话 | curate | 308 | 0 | 308 | 0 | 80.52% | 100.00% | `358b6acfa67371c99620d4bad15dd0e284488bf1` |
| 631 | 日常聊天：红包祝福、转账确认与礼金往来 | curate | 344 | 0 | 344 | 0 | 72.09% | 100.00% | `aae7706cbe42cf5bf26952e845d1fd28d62194f4` |
| 632 | 日常聊天：倾听近况、表达理解与陪伴 | curate | 323 | 0 | 323 | 0 | 83.90% | 100.00% | `3008a57f64fcf66c76d55efad9e9203bc08cb01d` |
| 633 | 日常聊天：约散步、遛弯与轻运动同行 | curate | 324 | 0 | 324 | 0 | 74.38% | 100.00% | `83987add91cbfbb67a829e643d96b16a26ff9302` |
| 634 | 日常聊天替人转达留言与回电确认的长句 | merge | 15 | 0 | 15 | 0 | 86.67% | 100.00% | `892f779ede7337a6a395583f90723c51fd3642d9` |
| 635 | 日常聊天红包祝福转账确认与礼金往来的长句 | merge | 15 | 0 | 15 | 0 | 80.00% | 80.00% | `548172dd70fdfa862377a7d8f2c69257098ed24a` |
| 636 | 日常聊天倾听近况表达理解与陪伴的长句 | merge | 10 | 0 | 10 | 0 | 70.00% | 80.00% | `8d7c0fceb07705a057152106e70b8a3feeec17a8` |
| 637 | 日常聊天约散步遛弯与轻运动同行的长句 | merge | 15 | 0 | 15 | 0 | 80.00% | 86.67% | `4ca20f8bf95fb4370fd7427835d8d675f4e83df0` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round68_exclusions.tsv`，共 434 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：175 条。
- `表达不完整，删除`：4 条。
- `短语结构未完整，删除`：1 条。
- `outside_batch_length_scope`：237 条。
- `duplicate:batch631`：3 条。
- `祝贺说法不自然，删除`：1 条。
- `语法不自然，删除`：2 条。
- `表达不自然，删除`：2 条。
- `搭配不自然，删除`：2 条。
- `偏书面且日常聊天中少用，删除`：1 条。
- `duplicate:batch632`：3 条。
- `语义不清，删除`：1 条。
- `duplicate:batch633`：2 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1201 个含多音字的词在各批复核表逐字记录；8 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

礼金和转账按朋友间礼貌往来使用；倾听与陪伴表达保持日常口语，不涉及诊断术语。多音字按语境复核。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：47/47 通过。
- x86 Release 原生构建与 ctest：47/47 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 108/108 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 281456` 通过，277106 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round68_validation.json`；短词批次另存工具生成的 `.validation.json`。
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

- `docs/lexicon-keep-adding-round68.md`
- `resources/dictionary/expansion/batch630_authored_draft.txt`
- `resources/dictionary/expansion/batch630_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch631_authored_draft.txt`
- `resources/dictionary/expansion/batch631_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch632_authored_draft.txt`
- `resources/dictionary/expansion/batch632_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch633_authored_draft.txt`
- `resources/dictionary/expansion/batch633_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch634_draft.txt`
- `resources/dictionary/expansion/batch634_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch635_draft.txt`
- `resources/dictionary/expansion/batch635_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch636_draft.txt`
- `resources/dictionary/expansion/batch636_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch637_draft.txt`
- `resources/dictionary/expansion/batch637_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_630.md`
- `resources/dictionary/expansion/batch_631.md`
- `resources/dictionary/expansion/batch_632.md`
- `resources/dictionary/expansion/batch_633.md`
- `resources/dictionary/expansion/batch_634.md`
- `resources/dictionary/expansion/batch_635.md`
- `resources/dictionary/expansion/batch_636.md`
- `resources/dictionary/expansion/batch_637.md`
- `resources/dictionary/expansion/lexicon_batch_630.applied.json`
- `resources/dictionary/expansion/lexicon_batch_630.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_630.json`
- `resources/dictionary/expansion/lexicon_batch_630.tsv`
- `resources/dictionary/expansion/lexicon_batch_630.validation.json`
- `resources/dictionary/expansion/lexicon_batch_631.applied.json`
- `resources/dictionary/expansion/lexicon_batch_631.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_631.json`
- `resources/dictionary/expansion/lexicon_batch_631.tsv`
- `resources/dictionary/expansion/lexicon_batch_631.validation.json`
- `resources/dictionary/expansion/lexicon_batch_632.applied.json`
- `resources/dictionary/expansion/lexicon_batch_632.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_632.json`
- `resources/dictionary/expansion/lexicon_batch_632.tsv`
- `resources/dictionary/expansion/lexicon_batch_632.validation.json`
- `resources/dictionary/expansion/lexicon_batch_633.applied.json`
- `resources/dictionary/expansion/lexicon_batch_633.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_633.json`
- `resources/dictionary/expansion/lexicon_batch_633.tsv`
- `resources/dictionary/expansion/lexicon_batch_633.validation.json`
- `resources/dictionary/expansion/lexicon_batch_634.json`
- `resources/dictionary/expansion/lexicon_batch_634.tsv`
- `resources/dictionary/expansion/lexicon_batch_635.json`
- `resources/dictionary/expansion/lexicon_batch_635.tsv`
- `resources/dictionary/expansion/lexicon_batch_636.json`
- `resources/dictionary/expansion/lexicon_batch_636.tsv`
- `resources/dictionary/expansion/lexicon_batch_637.json`
- `resources/dictionary/expansion/lexicon_batch_637.tsv`
- `resources/dictionary/expansion/probes/batch630_probe.tsv`
- `resources/dictionary/expansion/probes/batch631_probe.tsv`
- `resources/dictionary/expansion/probes/batch632_probe.tsv`
- `resources/dictionary/expansion/probes/batch633_probe.tsv`
- `resources/dictionary/expansion/probes/batch634_probe.tsv`
- `resources/dictionary/expansion/probes/batch635_probe.tsv`
- `resources/dictionary/expansion/probes/batch636_probe.tsv`
- `resources/dictionary/expansion/probes/batch637_probe.tsv`
- `resources/dictionary/expansion/round68_exclusions.tsv`
- `resources/dictionary/expansion/round68_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
